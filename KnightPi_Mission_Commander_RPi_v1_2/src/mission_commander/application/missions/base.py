from abc import ABC, abstractmethod
from typing import Any

from mission_commander.core.application_state import ApplicationState
from mission_commander.domain.enums import MissionPhase


class MissionExecutionContext:
    def __init__(
        self,
        app_state: ApplicationState,
        mission_id: str,
        mission_name: str,
        parameters: dict[str, Any],
        robot_service,
        vision_service,
        set_step_callback,
        set_phase_callback,
        wait_callback,
        checkpoint_callback
    ) -> None:
        self.app_state = app_state
        self.mission_id = mission_id
        self.mission_name = mission_name
        self.parameters = parameters
        self.robot_service = robot_service
        self.vision_service = vision_service
        self._set_step_callback = set_step_callback
        self._set_phase_callback = set_phase_callback
        self._wait_callback = wait_callback
        self._checkpoint_callback = checkpoint_callback

    def set_step(self, step: str, detail: str | None = None) -> None:
        self._set_step_callback(step, detail)

    def set_phase(
        self,
        phase: MissionPhase,
        detail: str | None = None,
        timeout_seconds: float | None = None
    ) -> None:
        self._set_phase_callback(phase, detail, timeout_seconds)

    def wait(self, seconds: float) -> None:
        self._wait_callback(seconds)

    def checkpoint(self) -> None:
        self._checkpoint_callback()


class MissionDefinition(ABC):
    @property
    @abstractmethod
    def mission_name(self) -> str:
        pass

    @property
    @abstractmethod
    def friendly_name(self) -> str:
        pass

    def default_parameters(self, app_state: ApplicationState) -> dict[str, Any]:
        return {}

    @abstractmethod
    def execute(self, context: MissionExecutionContext) -> None:
        pass
