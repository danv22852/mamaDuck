import asyncio

from mission_commander.application.missions.base import MissionDefinition, MissionExecutionContext


class DuckDetectOnceMission(MissionDefinition):
    @property
    def mission_name(self) -> str:
        return "duck_detect_once"

    @property
    def friendly_name(self) -> str:
        return "Duck Detect Once"

    def default_parameters(self, app_state) -> dict:
        return {
            "connect_esp32": True,
            "detector_kind": app_state.settings.cv_detector_kind,
            "save_debug_image": app_state.settings.cv_save_debug_image,
            "require_duck": True
        }

    def execute(self, context: MissionExecutionContext) -> None:
        if context.parameters.get("connect_esp32", True):
            context.set_step("CONNECT_ESP32")
            context.wait(0.3)
            context.robot_service.connect_esp32()

        context.set_step("ANALYZE_SCENE")
        context.wait(0.3)

        result = asyncio.run(
            context.vision_service.analyze_image(
                detector_kind=context.parameters.get("detector_kind"),
                save_debug_image=context.parameters.get("save_debug_image")
            )
        )

        if not result.ok:
            raise RuntimeError(result.detail or "vision analysis failed")

        analysis = context.app_state.telemetry_store.get_snapshot().last_cv_result
        if analysis is None:
            raise RuntimeError("no CV result available")

        if context.parameters.get("require_duck", True) and not analysis.has_duck:
            raise RuntimeError("no duck detected")
