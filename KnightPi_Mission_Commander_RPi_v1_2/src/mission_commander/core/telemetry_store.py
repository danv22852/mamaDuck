from mission_commander.domain.models import (
    TelemetrySnapshot,
    MissionSnapshot,
    EventRecord,
    PreflightReport,
)
from mission_commander.domain.cv_models import CvAnalysisResult


class TelemetryStore:
    def __init__(self, recent_event_limit: int = 100) -> None:
        self._recent_event_limit = recent_event_limit
        self._snapshot = TelemetrySnapshot(
            mission=MissionSnapshot()
        )

    def get_snapshot(self) -> TelemetrySnapshot:
        return self._snapshot.model_copy()

    def set_mission(self, mission: MissionSnapshot) -> None:
        self._snapshot.mission = mission

    def set_esp32_connected(self, value: bool) -> None:
        self._snapshot.esp32_connected = value

    def set_camera_ready(self, value: bool) -> None:
        self._snapshot.camera_ready = value

    def set_cv_service_ready(self, value: bool) -> None:
        self._snapshot.cv_service_ready = value

    def set_last_cv_result(self, result: CvAnalysisResult | None) -> None:
        self._snapshot.last_cv_result = result

    def set_preflight_report(self, report: PreflightReport | None) -> None:
        self._snapshot.last_preflight_report = report

    def append_event(self, event: EventRecord) -> None:
        self._snapshot.recent_events.append(event)
        if len(self._snapshot.recent_events) > self._recent_event_limit:
            self._snapshot.recent_events = self._snapshot.recent_events[-self._recent_event_limit:]
