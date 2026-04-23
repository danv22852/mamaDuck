from mission_commander.application.missions.base import MissionDefinition, MissionExecutionContext
from mission_commander.application.missions.duck_align_for_pickup_settings import DuckAlignForPickupSettings
from mission_commander.domain.cv_models import CvAnalysisResult, DuckCandidateResult
from mission_commander.domain.enums import MissionPhase


class DuckAlignForPickupMission(MissionDefinition):
    @property
    def mission_name(self) -> str:
        return "duck_align_for_pickup"

    @property
    def friendly_name(self) -> str:
        return "Duck Align For Pickup"

    def default_parameters(self, app_state) -> dict:
        return {
            "connect_first": True,
            "detector_kind": app_state.settings.cv_detector_kind,
            "save_debug_image": app_state.settings.cv_save_debug_image,
            "max_search_turns": 4,
            "search_turn_degrees": 90.0,
            "search_rotation_speed": 255,
            "rotation_speed": 255,
            "rotate_ms_per_degree": 8.0,
            "rotate_ms_per_offset_percent": 10.0,
            "center_tolerance_percent": 8.0,
            "min_center_rotate_ms": 50,
            "max_center_rotate_ms": 50,
            "approach_forward_duration_ms": 10000,
            "approach_forward_speed": 255,
            "approach_alarm_distance_mm": 160.0,
            "max_initial_alignment_attempts": 20,
            "max_final_alignment_attempts": 20
        }

    def execute(self, context: MissionExecutionContext) -> None:
        settings = DuckAlignForPickupSettings.model_validate(context.parameters)

        context.set_step("MISSION_INIT", "starting duck align for pickup mission")

        if settings.connect_first:
            context.set_step("CONNECT_ESP32", "connecting to esp32")
            context.robot_service.connect_esp32()
            context.wait(0.5)

        context.set_phase(
            MissionPhase.SEARCH_TARGET,
            "looking for duck candidate suitable for pickup"
        )

        analysis, candidate = self._find_target_by_rotating(context, settings)
        if analysis is None or candidate is None:
            context.set_step("MISSION_COMPLETE", "no pickup candidate found after full search")
            return

        context.set_phase(
            MissionPhase.APPROACH_TARGET,
            "performing initial visual alignment"
        )

        initial_analysis, initial_candidate = self._align_candidate_horizontally(
            context=context,
            settings=settings,
            max_attempts=settings.max_initial_alignment_attempts,
            detail_prefix="initial alignment"
        )

        if initial_analysis is None or initial_candidate is None:
            context.set_step("MISSION_COMPLETE", "duck could not be aligned for approach")
            return

        context.set_step("APPROACH_WITH_US_MONITOR", "moving forward until obstacle alarm or timeout")
        self._approach_forward_until_obstacle(context, settings)

        context.set_phase(
            MissionPhase.APPROACH_TARGET,
            "performing final visual alignment near pickup position"
        )

        final_analysis, final_candidate = self._align_candidate_horizontally(
            context=context,
            settings=settings,
            max_attempts=settings.max_final_alignment_attempts,
            detail_prefix="final alignment"
        )

        if final_analysis is None or final_candidate is None:
            raise RuntimeError("lost duck candidate during final alignment")

        context.set_step("PICKUP_READY", "robot is visually aligned in pickup-ready position")

    def _find_target_by_rotating(
        self,
        context: MissionExecutionContext,
        settings: DuckAlignForPickupSettings
    ) -> tuple[CvAnalysisResult | None, DuckCandidateResult | None]:
        for search_index in range(settings.max_search_turns + 1):
            context.set_step(
                "ANALYZE_SCENE",
                f"capture/analyze attempt {search_index + 1} during search"
            )

            analysis = self._analyze(context, settings)
            candidate = self._select_best_pickup_candidate(analysis)

            if candidate is not None:
                return analysis, candidate

            if search_index >= settings.max_search_turns:
                break

            rotate_ms = int(round(settings.search_turn_degrees * settings.rotate_ms_per_degree))
            context.set_step(
                "SEARCH_ROTATE",
                f"no valid duck found, rotating {settings.search_turn_degrees:.0f} degrees"
            )
            self._rotate_for_search(context, settings, rotate_ms)
            context.wait(0.35)

        return None, None

    def _align_candidate_horizontally(
        self,
        context: MissionExecutionContext,
        settings: DuckAlignForPickupSettings,
        max_attempts: int,
        detail_prefix: str
    ) -> tuple[CvAnalysisResult | None, DuckCandidateResult | None]:
        latest_analysis: CvAnalysisResult | None = None
        latest_candidate: DuckCandidateResult | None = None

        previous_abs_offset: float | None = None
        current_direction: str | None = None
        rotate_ms = 50

        for attempt in range(max_attempts):
            context.set_step(
                "ANALYZE_ALIGNMENT",
                f"{detail_prefix}: capture/analyze attempt {attempt + 1}"
            )

            analysis = self._analyze(context, settings)
            candidate = self._select_best_pickup_candidate(analysis)

            latest_analysis = analysis
            latest_candidate = candidate

            if candidate is None:
                return None, None

            offset_percent = candidate.horizontal_offset_percent
            abs_offset = abs(offset_percent)

            if abs_offset <= settings.center_tolerance_percent:
                return analysis, candidate

            if current_direction is None:
                current_direction = "CCW" if offset_percent < 0 else "CW"
            elif previous_abs_offset is not None and abs_offset > previous_abs_offset:
                current_direction = "CW" if current_direction == "CCW" else "CCW"

            if current_direction == "CW":
                context.set_step(
                    "ROTATE_TO_CENTER",
                    f"{detail_prefix}: offset {offset_percent:.2f}%, rotating CW for {rotate_ms} ms"
                )
                result = context.robot_service.rotate_cw(rotate_ms, settings.rotation_speed)
            else:
                context.set_step(
                    "ROTATE_TO_CENTER",
                    f"{detail_prefix}: offset {offset_percent:.2f}%, rotating CCW for {rotate_ms} ms"
                )
                result = context.robot_service.rotate_ccw(rotate_ms, settings.rotation_speed)

            if not result.ok:
                raise RuntimeError(result.detail or "failed to rotate for alignment")

            previous_abs_offset = abs_offset
            context.wait(0.30)

        return None, None

    def _approach_forward_until_obstacle(
        self,
        context: MissionExecutionContext,
        settings: DuckAlignForPickupSettings
    ) -> None:
        context.pop_robot_notifications("US_MONITOR_ALARM")

        monitor_on = context.robot_service.us_monitor_on(settings.approach_alarm_distance_mm)
        if not monitor_on.ok:
            raise RuntimeError(monitor_on.detail or "failed to enable ultrasonic monitor")

        move_result = None
        try:
            move_result = context.robot_service.move_forward(
                settings.approach_forward_duration_ms,
                settings.approach_forward_speed
            )
        finally:
            monitor_off = context.robot_service.us_monitor_off()
            if not monitor_off.ok:
                raise RuntimeError(monitor_off.detail or "failed to disable ultrasonic monitor")

        context.wait(0.40)

        alarms = context.pop_robot_notifications("US_MONITOR_ALARM")

        if alarms:
            context.app_state.run_logger.log_event(
                run_id=context.mission_id,
                kind="INFO",
                category="approach",
                message="ultrasonic monitor alarm received during forward approach",
                data={"alarmCount": len(alarms), "latestAlarm": alarms[-1]}
            )
            return

        if move_result is None:
            raise RuntimeError("forward approach did not return a result")

        if not move_result.ok:
            raise RuntimeError(move_result.detail or "forward approach failed")

        context.app_state.run_logger.log_event(
            run_id=context.mission_id,
            kind="WARN",
            category="approach",
            message="forward approach completed without ultrasonic alarm",
            data=move_result.data if isinstance(move_result.data, dict) else None
        )

    def _rotate_for_search(
        self,
        context: MissionExecutionContext,
        settings: DuckAlignForPickupSettings,
        rotate_ms: int
    ) -> None:
        result = context.robot_service.rotate_ccw(rotate_ms, settings.search_rotation_speed)
        if not result.ok:
            raise RuntimeError(result.detail or "failed search rotation")

    def _analyze(
        self,
        context: MissionExecutionContext,
        settings: DuckAlignForPickupSettings
    ) -> CvAnalysisResult:
        result = context.vision_service.analyze_image_sync(
            detector_kind=settings.detector_kind,
            save_debug_image=settings.save_debug_image
        )

        if not result.ok:
            raise RuntimeError(result.detail or "vision analysis failed")

        data = result.data or {}
        return CvAnalysisResult.model_validate(data)

    def _select_best_pickup_candidate(
        self,
        analysis: CvAnalysisResult
    ) -> DuckCandidateResult | None:
        eligible_candidates = [
            candidate
            for candidate in analysis.duck_candidates
            if candidate.is_inside_boundary
        ]

        if not eligible_candidates:
            return None

        eligible_candidates.sort(
            key=lambda candidate: (
                candidate.bottom_offset_percent_from_bottom,
                abs(candidate.horizontal_offset_percent),
                -candidate.match_score
            )
        )
        return eligible_candidates[0]

    def _estimate_centering_rotate_ms(
        self,
        settings: DuckAlignForPickupSettings,
        offset_percent: float
    ) -> int:
        estimated = int(round(abs(offset_percent) * settings.rotate_ms_per_offset_percent))
        if estimated < settings.min_center_rotate_ms:
            return settings.min_center_rotate_ms
        if estimated > settings.max_center_rotate_ms:
            return settings.max_center_rotate_ms
        return estimated




