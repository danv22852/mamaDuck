from mission_commander.domain.enums import MissionStatus, MissionPhase
from mission_commander.domain.models import MissionSnapshot


class MissionState:
    def __init__(self) -> None:
        self._snapshot = MissionSnapshot()

    def get_snapshot(self) -> MissionSnapshot:
        return self._snapshot.model_copy()

    def start(self, mission_id: str, mission_name: str) -> MissionSnapshot:
        self._snapshot.mission_id = mission_id
        self._snapshot.mission_name = mission_name
        self._snapshot.phase = MissionPhase.NONE
        self._snapshot.status = MissionStatus.PREPARING
        self._snapshot.current_step = "MISSION_ACCEPTED"
        self._snapshot.detail = "mission accepted and preparing"
        return self.get_snapshot()

    def set_phase(self, phase: MissionPhase, detail: str | None = None) -> MissionSnapshot:
        self._snapshot.phase = phase
        if detail:
            self._snapshot.detail = detail
        return self.get_snapshot()

    def mark_running(
        self,
        step: str,
        detail: str | None = None,
        phase: MissionPhase | None = None
    ) -> MissionSnapshot:
        self._snapshot.status = MissionStatus.RUNNING
        self._snapshot.current_step = step
        self._snapshot.detail = detail or "mission running"
        if phase is not None:
            self._snapshot.phase = phase
        return self.get_snapshot()

    def pause(self) -> MissionSnapshot:
        self._snapshot.status = MissionStatus.PAUSED
        self._snapshot.detail = "mission paused"
        return self.get_snapshot()

    def resume(self) -> MissionSnapshot:
        self._snapshot.status = MissionStatus.RUNNING
        self._snapshot.detail = "mission resumed"
        return self.get_snapshot()

    def abort(self) -> MissionSnapshot:
        self._snapshot.status = MissionStatus.ABORTING
        self._snapshot.detail = "mission abort requested"
        return self.get_snapshot()

    def complete(self) -> MissionSnapshot:
        self._snapshot.status = MissionStatus.COMPLETED
        self._snapshot.detail = "mission completed"
        return self.get_snapshot()

    def fail(self, detail: str) -> MissionSnapshot:
        self._snapshot.status = MissionStatus.FAILED
        self._snapshot.detail = detail
        return self.get_snapshot()
