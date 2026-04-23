import asyncio

from mission_commander.application.missions.base import MissionDefinition, MissionExecutionContext
from mission_commander.application.missions.duck_pick_and_drop_cycle_settings import DuckPickAndDropCycleSettings
from mission_commander.domain.cv_models import CvAnalysisResult
from mission_commander.domain.enums import MissionPhase


class DuckPickAndDropCycleMission(MissionDefinition):
    @property
    def mission_name(self) -> str:
        return "duck_pick_and_drop_cycle"

    @property
    def friendly_name(self) -> str:
        return "Duck Pick and Drop Cycle"

    def default_parameters(self, app_state) -> dict:
        return {
            "dry_run_no_motion": False,

            "search_target_timeout_seconds": 60.0,
            "approach_target_timeout_seconds": 60.0,
            "pickup_target_timeout_seconds": 45.0,
            "search_drop_zone_timeout_seconds": 60.0,
            "move_to_drop_zone_timeout_seconds": 60.0,
            "drop_target_timeout_seconds": 45.0,

            "search_scan_angle_deg": 360.0,
            "search_scan_steps": 48,
            "search_scan_rotate_speed": 255,
            "search_scan_step_rotate_ms": 200,
            "search_scan_rotation": "CCW",

            "rotation_speed": 255,
            "rotate_ms_per_degree": 8.0,
            "center_tolerance_percent": 8.0,

            "forward_speed": 255,
            "backward_speed": 255,
            "forward_ms_per_mm": 2.0,
            "backward_ms_per_mm": 2.0,

            "arm_pickup_distance_mm": 120.0,
            "pickup_distance_margin_mm": 50.0,
            "distance_tolerance_mm": 20.0,

            "pickup_verify_distance_delta_mm": 30.0,
            "pickup_recovery_backoff_mm": 50.0,

            "drop_search_step_angle_deg": 15.0,
            "drop_search_rotation_speed": 255,
            "drop_search_step_rotate_ms": 150,
            "drop_zone_center_tolerance_percent": 8.0,
            "drop_zone_bottom_target_percent": 20.0,

            "detector_kind": app_state.settings.cv_detector_kind,
            "save_debug_image": app_state.settings.cv_save_debug_image,

            "pickup_args": {
                "pickupPose": {
                    "chassisAngleDeg": 88,
                    "shoulderAngleDeg": 70,
                    "elbowAngleDeg": 95,
                    "wristAngleDeg": 95,
                    "openClawAngleDeg": 150,
                    "closeClawAngleDeg": 90
                },
                "liftPose": {
                    "chassisAngleDeg": 88,
                    "shoulderAngleDeg": 45,
                    "elbowAngleDeg": 70,
                    "wristAngleDeg": 95
                },
                "pickupSpeedPercent": {
                    "chassis": 50,
                    "shoulder": 50,
                    "elbow": 50,
                    "wrist": 50,
                    "claw": 50
                },
                "liftSpeedPercent": {
                    "chassis": 50,
                    "shoulder": 50,
                    "elbow": 50,
                    "wrist": 50,
                    "claw": 50
                },
                "pickupMotionMode": "ORDERED",
                "liftMotionMode": "ORDERED",
                "pickupServoOrder": ["CLAW", "CHASSIS", "SHOULDER", "ELBOW", "WRIST"],
                "liftServoOrder": ["SHOULDER", "ELBOW", "WRIST", "CHASSIS"]
            },

            "drop_args": {
                "dropPose": {
                    "chassisAngleDeg": 88,
                    "shoulderAngleDeg": 70,
                    "elbowAngleDeg": 95,
                    "wristAngleDeg": 95,
                    "openClawAngleDeg": 150
                },
                "retreatPose": {
                    "chassisAngleDeg": 88,
                    "shoulderAngleDeg": 45,
                    "elbowAngleDeg": 70,
                    "wristAngleDeg": 95
                },
                "dropSpeedPercent": {
                    "chassis": 50,
                    "shoulder": 50,
                    "elbow": 50,
                    "wrist": 50,
                    "claw": 50
                },
                "retreatSpeedPercent": {
                    "chassis": 50,
                    "shoulder": 50,
                    "elbow": 50,
                    "wrist": 50,
                    "claw": 50
                },
                "dropMotionMode": "ORDERED",
                "retreatMotionMode": "ORDERED",
                "dropServoOrder": ["CHASSIS", "SHOULDER", "ELBOW", "WRIST"],
                "retreatServoOrder": ["SHOULDER", "ELBOW", "WRIST", "CHASSIS"]
            }
        }

    def execute(self, context: MissionExecutionContext) -> None:
        settings = DuckPickAndDropCycleSettings.model_validate(context.parameters)

        context.set_step("MISSION_INIT", "starting duck pick and drop cycle")
        context.robot_service.connect_esp32()
        context.wait(0.5)

        while True:
            analysis = self._search_target(context, settings)
            if analysis is None:
                return

            analysis = self._approach_target(context, analysis, settings)

            pickup_ok = self._pickup_target(context, analysis, settings)
            if not pickup_ok:
                continue

            self._search_and_move_to_drop_zone(context, settings)
            self._drop_target(context, settings)

    def _search_target(self, context: MissionExecutionContext, settings: DuckPickAndDropCycleSettings) -> CvAnalysisResult | None:
        context.set_phase(
            MissionPhase.SEARCH_TARGET,
            "searching target",
            settings.search_target_timeout_seconds
        )
        context.set_step("SEARCH_TARGET_SCAN", "performing target search")
        context.checkpoint()

        if settings.dry_run_no_motion:
            context.app_state.run_logger.log_event(
                run_id=context.mission_id,
                kind="INFO",
                category="dry-run",
                message="dry-run enabled, skipping ultrasonic rotation scan"
            )
            analysis = self._analyze_scene(context, settings)
            if self._has_eligible_duck(analysis):
                context.set_step("SEARCH_TARGET_FOUND", "eligible duck found")
                return analysis
            context.set_step("SEARCH_TARGET_DONE", "no eligible duck found in current scene")
            return None

        scan_result = context.robot_service.us_scan(
            scan_angle_deg=settings.search_scan_angle_deg,
            steps=settings.search_scan_steps,
            step_rotate_ms=settings.search_scan_step_rotate_ms,
            rotate_speed=settings.search_scan_rotate_speed,
            rotation=settings.search_scan_rotation
        )

        points = self._extract_us_scan_points(scan_result)
        if not points:
            raise RuntimeError("search scan returned no points")

        candidate_points = sorted(
            [p for p in points if p["distanceMm"] > 0],
            key=lambda p: p["distanceMm"]
        )

        current_heading = 0.0

        for point in candidate_points:
            context.checkpoint()

            target_heading = float(point["angleDeg"])
            context.set_step("SEARCH_TARGET_ROTATE", f"rotating to scan heading {target_heading:.1f} deg")

            current_heading = self._rotate_to_absolute_heading(
                context=context,
                current_heading=current_heading,
                target_heading=target_heading,
                settings=settings
            )

            analysis = self._analyze_scene(context, settings)

            if self._has_eligible_duck(analysis):
                context.set_step("SEARCH_TARGET_FOUND", "eligible duck found")
                return analysis

        context.set_step("SEARCH_TARGET_DONE", "no eligible duck found in scan")
        return None

    def _approach_target(
        self,
        context: MissionExecutionContext,
        analysis: CvAnalysisResult,
        settings: DuckPickAndDropCycleSettings
    ) -> CvAnalysisResult:
        context.set_phase(
            MissionPhase.APPROACH_TARGET,
            "approaching target",
            settings.approach_target_timeout_seconds
        )
        context.set_step("APPROACH_TARGET", "approaching target")

        if settings.dry_run_no_motion:
            context.app_state.run_logger.log_event(
                run_id=context.mission_id,
                kind="INFO",
                category="dry-run",
                message="dry-run enabled, skipping target approach motion"
            )
            return analysis

        for _ in range(12):
            context.checkpoint()

            analysis = self._realign_to_duck(context, analysis, settings)

            distance_mm = self._read_us_distance_mm(context)
            target_distance_mm = settings.arm_pickup_distance_mm + settings.pickup_distance_margin_mm
            delta_mm = distance_mm - target_distance_mm

            if abs(delta_mm) <= settings.distance_tolerance_mm:
                break

            if delta_mm > 0:
                self._move_forward_mm(context, delta_mm, settings)
            else:
                self._move_backward_mm(context, abs(delta_mm), settings)

            context.wait(0.4)
            analysis = self._analyze_scene(context, settings)

        final_distance_mm = self._read_us_distance_mm(context)
        final_delta_mm = final_distance_mm - settings.arm_pickup_distance_mm

        if abs(final_delta_mm) > settings.distance_tolerance_mm:
            if final_delta_mm > 0:
                self._move_forward_mm(context, final_delta_mm, settings)
            else:
                self._move_backward_mm(context, abs(final_delta_mm), settings)

            context.wait(0.4)
            analysis = self._analyze_scene(context, settings)

        return analysis

    def _pickup_target(
        self,
        context: MissionExecutionContext,
        analysis_before_pickup: CvAnalysisResult,
        settings: DuckPickAndDropCycleSettings
    ) -> bool:
        context.set_phase(
            MissionPhase.PICKUP_TARGET,
            "picking target",
            settings.pickup_target_timeout_seconds
        )
        context.set_step("PICKUP_TARGET", "executing pickup")
        context.checkpoint()

        if settings.dry_run_no_motion:
            context.app_state.run_logger.log_event(
                run_id=context.mission_id,
                kind="INFO",
                category="dry-run",
                message="dry-run enabled, skipping pickup actuation"
            )
            return True

        distance_before = self._read_us_distance_mm(context)

        pickup_result = context.robot_service.pick_angles_speed(settings.pickup_args)
        pickup_json = self._extract_response_json(pickup_result)

        if pickup_json.get("status") != "OK":
            raise RuntimeError("pickup command failed")

        context.wait(1.0)

        analysis_after = self._analyze_scene(context, settings)
        distance_after = self._read_us_distance_mm(context)

        if self._pickup_looks_successful(
            analysis_before_pickup,
            analysis_after,
            distance_before,
            distance_after,
            settings.pickup_verify_distance_delta_mm
        ):
            context.set_step("PICKUP_CONFIRMED", "pickup appears successful")
            return True

        context.set_phase(MissionPhase.RECOVERY, "pickup verification failed")
        context.set_step("PICKUP_RECOVERY", "pickup verification failed, backing off")
        self._move_backward_mm(context, settings.pickup_recovery_backoff_mm, settings)
        context.wait(0.5)
        return False

    def _search_and_move_to_drop_zone(
        self,
        context: MissionExecutionContext,
        settings: DuckPickAndDropCycleSettings
    ) -> None:
        context.set_phase(
            MissionPhase.SEARCH_DROP_ZONE,
            "searching drop zone",
            settings.search_drop_zone_timeout_seconds
        )
        context.set_step("SEARCH_DROP_ZONE", "searching for drop zone")
        context.checkpoint()

        if settings.dry_run_no_motion:
            context.app_state.run_logger.log_event(
                run_id=context.mission_id,
                kind="INFO",
                category="dry-run",
                message="dry-run enabled, checking drop zone only in current heading"
            )
            analysis = self._analyze_scene(context, settings)
            if not analysis.drop_zone.detected:
                raise RuntimeError("drop zone not found in current heading during dry run")
            return

        scan_steps = max(1, int(round(360.0 / settings.drop_search_step_angle_deg)))
        analysis = None

        for index in range(scan_steps):
            context.checkpoint()

            if index > 0:
                self._rotate_by_degrees(context, settings.drop_search_step_angle_deg, settings)

            analysis = self._analyze_scene(context, settings)
            if analysis.drop_zone.detected:
                break

        if analysis is None or not analysis.drop_zone.detected:
            raise RuntimeError("drop zone not found")

        context.set_phase(
            MissionPhase.MOVE_TO_DROP_ZONE,
            "moving toward drop zone",
            settings.move_to_drop_zone_timeout_seconds
        )
        context.set_step("MOVE_TO_DROP_ZONE", "moving toward drop zone")

        for _ in range(12):
            context.checkpoint()

            if not analysis.drop_zone.detected:
                raise RuntimeError("drop zone lost during approach")

            horizontal_offset = analysis.drop_zone.horizontal_offset_percent

            if abs(horizontal_offset) > settings.drop_zone_center_tolerance_percent:
                self._rotate_from_horizontal_offset(context, horizontal_offset, settings)
                context.wait(0.3)
                analysis = self._analyze_scene(context, settings)
                continue

            if analysis.drop_zone.bottom_offset_percent_from_bottom <= settings.drop_zone_bottom_target_percent:
                context.set_step("DROP_ZONE_REACHED", "drop zone approach complete")
                return

            self._move_forward_mm(context, 60.0, settings)
            context.wait(0.4)
            analysis = self._analyze_scene(context, settings)

        raise RuntimeError("unable to reach drop zone within approach limit")

    def _drop_target(self, context: MissionExecutionContext, settings: DuckPickAndDropCycleSettings) -> None:
        context.set_phase(
            MissionPhase.DROP_TARGET,
            "dropping target",
            settings.drop_target_timeout_seconds
        )
        context.set_step("DROP_TARGET", "executing drop")
        context.checkpoint()

        if settings.dry_run_no_motion:
            context.app_state.run_logger.log_event(
                run_id=context.mission_id,
                kind="INFO",
                category="dry-run",
                message="dry-run enabled, skipping drop actuation"
            )
            context.wait(0.5)
            return

        drop_result = context.robot_service.drop_angles_speed(settings.drop_args)
        drop_json = self._extract_response_json(drop_result)

        if drop_json.get("status") != "OK":
            raise RuntimeError("drop command failed")

        context.wait(1.0)

    def _realign_to_duck(
        self,
        context: MissionExecutionContext,
        analysis: CvAnalysisResult,
        settings: DuckPickAndDropCycleSettings
    ) -> CvAnalysisResult:
        for _ in range(8):
            context.checkpoint()

            if not self._has_eligible_duck(analysis):
                raise RuntimeError("target lost during approach")

            if abs(analysis.horizontal_offset_percent) <= settings.center_tolerance_percent:
                return analysis

            self._rotate_from_horizontal_offset(context, analysis.horizontal_offset_percent, settings)
            context.wait(0.3)
            analysis = self._analyze_scene(context, settings)

        return analysis

    def _analyze_scene(self, context: MissionExecutionContext, settings: DuckPickAndDropCycleSettings) -> CvAnalysisResult:
        result = asyncio.run(
            context.vision_service.analyze_image(
                detector_kind=settings.detector_kind,
                save_debug_image=settings.save_debug_image
            )
        )

        if not result.ok:
            raise RuntimeError(result.detail or "vision analysis failed")

        analysis = context.app_state.telemetry_store.get_snapshot().last_cv_result
        if analysis is None:
            raise RuntimeError("no CV result available")

        return analysis

    def _has_eligible_duck(self, analysis: CvAnalysisResult) -> bool:
        return (
            analysis.success and
            analysis.has_duck and
            analysis.eligible_duck_candidates > 0 and
            analysis.blue_boundary.bottom_center_inside_traversable_region
        )

    def _rotate_from_horizontal_offset(
        self,
        context: MissionExecutionContext,
        horizontal_offset_percent: float,
        settings: DuckPickAndDropCycleSettings
    ) -> None:
        delta_deg = abs(horizontal_offset_percent)
        if delta_deg <= 0.0:
            return

        if horizontal_offset_percent < 0:
            self._rotate_by_degrees(context, -delta_deg, settings)
        else:
            self._rotate_by_degrees(context, delta_deg, settings)

    def _rotate_to_absolute_heading(
        self,
        context: MissionExecutionContext,
        current_heading: float,
        target_heading: float,
        settings: DuckPickAndDropCycleSettings
    ) -> float:
        delta = (target_heading - current_heading + 540.0) % 360.0 - 180.0
        self._rotate_by_degrees(context, delta, settings)
        return target_heading

    def _rotate_by_degrees(
        self,
        context: MissionExecutionContext,
        delta_deg: float,
        settings: DuckPickAndDropCycleSettings
    ) -> None:
        if abs(delta_deg) < 0.5:
            return

        if settings.dry_run_no_motion:
            context.app_state.run_logger.log_event(
                run_id=context.mission_id,
                kind="INFO",
                category="dry-run",
                message="dry-run enabled, skipping rotate",
                data={"deltaDeg": delta_deg}
            )
            context.wait(0.3)
            return

        duration_ms = max(1, int(round(abs(delta_deg) * settings.rotate_ms_per_degree)))

        if delta_deg > 0:
            rotate_result = context.robot_service.rotate_cw(duration_ms, settings.rotation_speed)
        else:
            rotate_result = context.robot_service.rotate_ccw(duration_ms, settings.rotation_speed)

        self._extract_response_json(rotate_result)
        context.wait(0.3)

    def _move_forward_mm(
        self,
        context: MissionExecutionContext,
        distance_mm: float,
        settings: DuckPickAndDropCycleSettings
    ) -> None:
        if settings.dry_run_no_motion:
            context.app_state.run_logger.log_event(
                run_id=context.mission_id,
                kind="INFO",
                category="dry-run",
                message="dry-run enabled, skipping forward move",
                data={"distanceMm": distance_mm}
            )
            context.wait(0.3)
            return

        duration_ms = max(1, int(round(distance_mm * settings.forward_ms_per_mm)))
        move_result = context.robot_service.move_forward(duration_ms, settings.forward_speed)
        self._extract_response_json(move_result)

    def _move_backward_mm(
        self,
        context: MissionExecutionContext,
        distance_mm: float,
        settings: DuckPickAndDropCycleSettings
    ) -> None:
        if settings.dry_run_no_motion:
            context.app_state.run_logger.log_event(
                run_id=context.mission_id,
                kind="INFO",
                category="dry-run",
                message="dry-run enabled, skipping backward move",
                data={"distanceMm": distance_mm}
            )
            context.wait(0.3)
            return

        duration_ms = max(1, int(round(distance_mm * settings.backward_ms_per_mm)))
        move_result = context.robot_service.move_backward(duration_ms, settings.backward_speed)
        self._extract_response_json(move_result)

    def _read_us_distance_mm(self, context: MissionExecutionContext) -> float:
        result = context.robot_service.read_distance_mm()
        response_json = self._extract_response_json(result)
        return float(response_json["data"]["distanceMm"])

    def _extract_us_scan_points(self, command_result) -> list[dict]:
        response_json = self._extract_response_json(command_result)
        data = response_json.get("data", {})
        return list(data.get("points", []))

    def _extract_response_json(self, command_result) -> dict:
        if not command_result.data:
            raise RuntimeError(command_result.detail or "robot command failed")

        response_json = command_result.data.get("responseJson")
        if not response_json:
            raise RuntimeError("robot response did not contain JSON payload")

        if response_json.get("status") == "ERROR":
            error = response_json.get("error") or {}
            message = error.get("message") or command_result.detail or "robot command failed"
            raise RuntimeError(str(message))

        if not command_result.ok:
            raise RuntimeError(command_result.detail or "robot command failed")

        return response_json

    def _pickup_looks_successful(
        self,
        before_analysis: CvAnalysisResult,
        after_analysis: CvAnalysisResult,
        before_distance_mm: float,
        after_distance_mm: float,
        verify_distance_delta_mm: float
    ) -> bool:
        distance_changed = abs(after_distance_mm - before_distance_mm) >= verify_distance_delta_mm
        duck_changed = before_analysis.has_duck != after_analysis.has_duck
        eligible_changed = before_analysis.eligible_duck_candidates != after_analysis.eligible_duck_candidates
        return distance_changed or duck_changed or eligible_changed
