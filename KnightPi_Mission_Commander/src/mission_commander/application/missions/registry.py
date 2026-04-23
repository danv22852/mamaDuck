from mission_commander.application.missions.base import MissionDefinition
from mission_commander.application.missions.esp32_ping_mission import Esp32PingMission
from mission_commander.application.missions.duck_detect_once_mission import DuckDetectOnceMission
from mission_commander.application.missions.duck_pick_and_drop_cycle_mission import DuckPickAndDropCycleMission
from mission_commander.application.missions.duck_align_for_pickup_mission import DuckAlignForPickupMission


class MissionRegistry:
    def __init__(self) -> None:
        self._missions: dict[str, MissionDefinition] = {}

    def register(self, mission: MissionDefinition) -> None:
        self._missions[mission.mission_name] = mission

    def get(self, mission_name: str) -> MissionDefinition | None:
        return self._missions.get(mission_name)

    def list_missions(self) -> list[dict]:
        return [
            {
                "mission_name": mission.mission_name,
                "friendly_name": mission.friendly_name
            }
            for mission in self._missions.values()
        ]


def build_default_mission_registry() -> MissionRegistry:
    registry = MissionRegistry()
    registry.register(Esp32PingMission())
    registry.register(DuckDetectOnceMission())
    registry.register(DuckPickAndDropCycleMission())
    registry.register(DuckAlignForPickupMission())
    return registry
