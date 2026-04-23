from mission_commander.core.application_state import ApplicationState
from mission_commander.domain.models import CommandResult


class VisionService:
    def __init__(self, app_state: ApplicationState) -> None:
        self._app_state = app_state

    def capture_image(self) -> CommandResult:
        jpeg_bytes = self._app_state.camera_client.capture_jpeg()
        self._app_state.telemetry_store.set_camera_ready(self._app_state.camera_client.is_ready)

        return CommandResult(
            ok=True,
            action="CAMERA_CAPTURE",
            detail="image captured",
            data={
                "bytes": len(jpeg_bytes)
            }
        )

    async def analyze_image(
        self,
        detector_kind: str | None = None,
        save_debug_image: bool | None = None
    ) -> CommandResult:
        jpeg_bytes = self._app_state.camera_client.capture_jpeg()

        analysis = await self._app_state.cv_service_client.analyze_image(
            jpeg_bytes=jpeg_bytes,
            detector_kind=detector_kind or self._app_state.settings.cv_detector_kind,
            save_debug_image=(
                save_debug_image
                if save_debug_image is not None
                else self._app_state.settings.cv_save_debug_image
            )
        )

        self._app_state.telemetry_store.set_camera_ready(self._app_state.camera_client.is_ready)
        self._app_state.telemetry_store.set_cv_service_ready(self._app_state.cv_service_client.is_ready)
        self._app_state.telemetry_store.set_last_cv_result(analysis)

        return CommandResult(
            ok=analysis.success,
            action="CV_ANALYZE_IMAGE",
            detail="image analyzed" if analysis.success else analysis.error_message,
            data=analysis.model_dump(by_alias=True)
        )
