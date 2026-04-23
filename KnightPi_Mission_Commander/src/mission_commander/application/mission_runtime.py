import threading
import time
from typing import Any

from mission_commander.application.robot_service import RobotService
from mission_commander.application.safety_service import SafetyService
from mission_commander.application.vision_service import VisionService
from mission_commander.application.missions.base import MissionDefinition, MissionExecutionContext
from mission_commander.core.application_state import ApplicationState
from mission_commander.domain.enums import MissionStatus, MissionPhase


class MissionRuntime:
    def __init__(self, app_state: ApplicationState) -> None:
        self._app_state = app_state
        self._thread: threading.Thread | None = None
        self._pause_event = threading.Event()
        self._abort_event = threading.Event()
        self._lock = threading.Lock()
        self._active_phase_deadline_utc_epoch: float | None = None
        self._active_phase_name: str | None = None

        self._robot_notification_lock = threading.Lock()
        self._pending_robot_notifications: list[dict[str, Any]] = []
        self._notification_subscription_id: str | None = None

    @property
    def is_running(self) -> bool:
        return self._thread is not None and self._thread.is_alive()

    def start(
        self,
        mission: MissionDefinition,
        mission_id: str,
        mission_name: str,
        parameters: dict
    ) -> bool:
        with self._lock:
            if self.is_running:
                return False

            self._pause_event.clear()
            self._abort_event.clear()
            self._active_phase_deadline_utc_epoch = None
            self._active_phase_name = None

            with self._robot_notification_lock:
                self._pending_robot_notifications = []

            self._thread = threading.Thread(
                target=self._run_mission,
                args=(mission, mission_id, mission_name, parameters),
                name="mission-runtime",
                daemon=True
            )
            self._thread.start()
            return True

    def pause(self) -> None:
        self._pause_event.set()

    def resume(self) -> None:
        self._pause_event.clear()

    def abort(self) -> None:
        self._abort_event.set()

    def _run_mission(
        self,
        mission: MissionDefinition,
        mission_id: str,
        mission_name: str,
        parameters: dict
    ) -> None:
        robot_service = RobotService(self._app_state)
        vision_service = VisionService(self._app_state)
        safety_service = SafetyService(self._app_state)

        merged_parameters = mission.default_parameters(self._app_state)
        merged_parameters.update(parameters or {})

        self._app_state.run_logger.start_run(
            run_id=mission_id,
            run_kind="mission",
            run_name=mission_name,
            parameters=merged_parameters
        )
        self._app_state.run_logger.log_event(
            run_id=mission_id,
            kind="INFO",
            category="mission",
            message="mission execution started",
            data={"missionName": mission_name}
        )

        self._subscribe_to_robot_notifications(mission_id)

        context = MissionExecutionContext(
            app_state=self._app_state,
            mission_id=mission_id,
            mission_name=mission_name,
            parameters=merged_parameters,
            robot_service=robot_service,
            vision_service=vision_service,
            set_step_callback=self._set_running_step,
            set_phase_callback=lambda phase, detail, timeout_seconds=None: self._set_phase(
                mission_id,
                phase,
                detail,
                timeout_seconds
            ),
            wait_callback=self._wait_with_pause_abort,
            checkpoint_callback=self._check_pause_abort_timeout,
            peek_robot_notifications_callback=self._peek_robot_notifications,
            pop_robot_notifications_callback=self._pop_robot_notifications
        )

        try:
            mission.execute(context)

            self._set_running_step("MISSION_COMPLETE", "mission execution finished")
            self._wait_with_pause_abort(0.2)

            snapshot = self._app_state.mission_state.complete()
            self._app_state.telemetry_store.set_mission(snapshot)

            self._app_state.run_logger.log_event(
                run_id=mission_id,
                kind="INFO",
                category="mission",
                message="mission execution completed"
            )
            self._app_state.run_logger.finish_run(
                run_id=mission_id,
                outcome="SUCCESS",
                summary_data=self._app_state.telemetry_store.get_snapshot().model_dump(by_alias=True)
            )

        except RuntimeError as ex:
            safety_service.safe_stop(mission_id, str(ex))
            snapshot = self._app_state.mission_state.fail(str(ex))
            self._app_state.telemetry_store.set_mission(snapshot)

            self._app_state.run_logger.log_event(
                run_id=mission_id,
                kind="ERROR",
                category="mission",
                message="mission runtime error",
                data={"error": str(ex)}
            )
            self._app_state.run_logger.finish_run(
                run_id=mission_id,
                outcome="FAILED",
                summary_data={"error": str(ex)}
            )

        except Exception as ex:
            safety_service.safe_stop(mission_id, str(ex))
            snapshot = self._app_state.mission_state.fail(f"mission execution failed: {ex}")
            self._app_state.telemetry_store.set_mission(snapshot)

            self._app_state.run_logger.log_event(
                run_id=mission_id,
                kind="ERROR",
                category="mission",
                message="mission unhandled exception",
                data={"error": str(ex)}
            )
            self._app_state.run_logger.finish_run(
                run_id=mission_id,
                outcome="FAILED",
                summary_data={"error": str(ex)}
            )

        finally:
            self._unsubscribe_from_robot_notifications()
            with self._robot_notification_lock:
                self._pending_robot_notifications = []

    def _set_phase(
        self,
        mission_id: str,
        phase: MissionPhase,
        detail: str | None = None,
        timeout_seconds: float | None = None
    ) -> None:
        snapshot = self._app_state.mission_state.set_phase(phase, detail)
        self._app_state.telemetry_store.set_mission(snapshot)

        self._active_phase_name = phase.value
        if timeout_seconds is not None and timeout_seconds > 0:
            self._active_phase_deadline_utc_epoch = time.time() + timeout_seconds
        else:
            self._active_phase_deadline_utc_epoch = None

        self._app_state.run_logger.log_event(
            run_id=mission_id,
            kind="INFO",
            category="phase",
            message=f"phase changed to {phase.value}",
            data={
                "phase": phase.value,
                "detail": detail,
                "timeoutSeconds": timeout_seconds
            }
        )

    def _set_running_step(self, step: str, detail: str | None = None) -> None:
        snapshot = self._app_state.mission_state.mark_running(step, detail)
        self._app_state.telemetry_store.set_mission(snapshot)

    def _wait_with_pause_abort(self, total_seconds: float) -> None:
        elapsed = 0.0
        interval = 0.2

        while elapsed < total_seconds:
            self._check_pause_abort_timeout()
            time.sleep(interval)
            elapsed += interval

    def _check_pause_abort_timeout(self) -> None:
        if self._abort_event.is_set():
            snapshot = self._app_state.mission_state.abort()
            self._app_state.telemetry_store.set_mission(snapshot)
            raise RuntimeError("mission aborted")

        if (
            self._active_phase_deadline_utc_epoch is not None and
            time.time() > self._active_phase_deadline_utc_epoch
        ):
            raise RuntimeError(
                f"phase timeout exceeded for {self._active_phase_name or 'UNKNOWN_PHASE'}"
            )

        while self._pause_event.is_set():
            current = self._app_state.mission_state.get_snapshot()

            if current.status != MissionStatus.PAUSED:
                snapshot = self._app_state.mission_state.pause()
                self._app_state.telemetry_store.set_mission(snapshot)

            if self._abort_event.is_set():
                snapshot = self._app_state.mission_state.abort()
                self._app_state.telemetry_store.set_mission(snapshot)
                raise RuntimeError("mission aborted")

            time.sleep(0.2)

        current = self._app_state.mission_state.get_snapshot()
        if current.status == MissionStatus.PAUSED:
            snapshot = self._app_state.mission_state.resume()
            self._app_state.telemetry_store.set_mission(snapshot)

    def _subscribe_to_robot_notifications(self, mission_id: str) -> None:
        self._unsubscribe_from_robot_notifications()

        self._notification_subscription_id = self._app_state.esp32_client.subscribe_notification(
            "*",
            lambda notification: self._on_robot_notification(mission_id, notification)
        )

    def _unsubscribe_from_robot_notifications(self) -> None:
        if self._notification_subscription_id is None:
            return

        try:
            self._app_state.esp32_client.unsubscribe_notification(self._notification_subscription_id)
        finally:
            self._notification_subscription_id = None

    def _on_robot_notification(self, mission_id: str, notification: dict[str, Any]) -> None:
        topic = str(notification.get("topic") or "UNKNOWN_NOTIFICATION").upper()

        with self._robot_notification_lock:
            self._pending_robot_notifications.append(notification)
            if len(self._pending_robot_notifications) > 100:
                self._pending_robot_notifications = self._pending_robot_notifications[-100:]

        self._app_state.run_logger.log_event(
            run_id=mission_id,
            kind="WARN" if topic == "US_MONITOR_ALARM" else "INFO",
            category="robot_notification",
            message=f"robot notification received: {topic}",
            data=notification
        )

    def _peek_robot_notifications(self, topic: str | None = None) -> list[dict[str, Any]]:
        normalized_topic = self._normalize_topic(topic)

        with self._robot_notification_lock:
            notifications = list(self._pending_robot_notifications)

        if normalized_topic is None:
            return notifications

        return [
            notification
            for notification in notifications
            if str(notification.get("topic") or "").upper() == normalized_topic
        ]

    def _pop_robot_notifications(self, topic: str | None = None) -> list[dict[str, Any]]:
        normalized_topic = self._normalize_topic(topic)

        with self._robot_notification_lock:
            if normalized_topic is None:
                popped = list(self._pending_robot_notifications)
                self._pending_robot_notifications = []
                return popped

            remaining: list[dict[str, Any]] = []
            popped: list[dict[str, Any]] = []

            for notification in self._pending_robot_notifications:
                if str(notification.get("topic") or "").upper() == normalized_topic:
                    popped.append(notification)
                else:
                    remaining.append(notification)

            self._pending_robot_notifications = remaining
            return popped

    def _normalize_topic(self, topic: str | None) -> str | None:
        if topic is None:
            return None

        normalized = str(topic).strip().upper()
        return normalized or None
