import uuid

from mission_commander.application.mission_runtime import MissionRuntime
from mission_commander.application.missions.registry import build_default_mission_registry
from mission_commander.core.application_state import ApplicationState
from mission_commander.domain.models import CommandResult


class MissionCoordinator:
    def __init__(self, app_state: ApplicationState) -> None:
        self._app_state = app_state

        if not hasattr(self._app_state, "mission_runtime"):
            self._app_state.mission_runtime = MissionRuntime(self._app_state)

        if not hasattr(self._app_state, "mission_registry"):
            self._app_state.mission_registry = build_default_mission_registry()

    def list_missions(self) -> list[dict]:
        return self._app_state.mission_registry.list_missions()

    def start_mission(
        self,
        mission_name: str,
        mission_id: str | None = None,
        parameters: dict | None = None
    ) -> CommandResult:
        if self._app_state.mission_runtime.is_running:
            return CommandResult(
                ok=False,
                action="MISSION_START",
                detail="mission already running",
                data=self._app_state.telemetry_store.get_snapshot().model_dump(by_alias=True)
            )

        mission = self._app_state.mission_registry.get(mission_name)
        if mission is None:
            return CommandResult(
                ok=False,
                action="MISSION_START",
                detail=f"unknown mission: {mission_name}",
                data={
                    "availableMissions": self.list_missions()
                }
            )

        resolved_mission_id = mission_id or str(uuid.uuid4())

        snapshot = self._app_state.mission_state.start(resolved_mission_id, mission_name)
        self._app_state.telemetry_store.set_mission(snapshot)

        started = self._app_state.mission_runtime.start(
            mission=mission,
            mission_id=resolved_mission_id,
            mission_name=mission_name,
            parameters=parameters or {}
        )

        return CommandResult(
            ok=started,
            action="MISSION_START",
            detail="mission started" if started else "mission failed to start",
            data=self._app_state.telemetry_store.get_snapshot().model_dump(by_alias=True)
        )

    def pause_mission(self) -> CommandResult:
        self._app_state.mission_runtime.pause()

        snapshot = self._app_state.mission_state.pause()
        self._app_state.telemetry_store.set_mission(snapshot)

        return CommandResult(
            ok=True,
            action="MISSION_PAUSE",
            detail="mission paused",
            data=snapshot.model_dump(by_alias=True)
        )

    def resume_mission(self) -> CommandResult:
        self._app_state.mission_runtime.resume()

        snapshot = self._app_state.mission_state.resume()
        self._app_state.telemetry_store.set_mission(snapshot)

        return CommandResult(
            ok=True,
            action="MISSION_RESUME",
            detail="mission resumed",
            data=snapshot.model_dump(by_alias=True)
        )

    def abort_mission(self) -> CommandResult:
        self._app_state.mission_runtime.abort()

        snapshot = self._app_state.mission_state.abort()
        self._app_state.telemetry_store.set_mission(snapshot)

        return CommandResult(
            ok=True,
            action="MISSION_ABORT",
            detail="mission abort requested",
            data=snapshot.model_dump(by_alias=True)
        )

    def get_status(self) -> dict:
        return self._app_state.telemetry_store.get_snapshot().model_dump(by_alias=True)
