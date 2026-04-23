from mission_commander.application.missions.base import MissionDefinition, MissionExecutionContext


class Esp32PingMission(MissionDefinition):
    @property
    def mission_name(self) -> str:
        return "esp32_ping_test"

    @property
    def friendly_name(self) -> str:
        return "ESP32 Ping Test"

    def default_parameters(self, app_state) -> dict:
        return {
            "connect_first": True
        }

    def execute(self, context: MissionExecutionContext) -> None:
        if context.parameters.get("connect_first", True):
            context.set_step("CONNECT_ESP32")
            context.wait(0.3)
            context.robot_service.connect_esp32()

        context.set_step("PING_ESP32")
        context.wait(0.3)
        context.robot_service.ping_esp32()
