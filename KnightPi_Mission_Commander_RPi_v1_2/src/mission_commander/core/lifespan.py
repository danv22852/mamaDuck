from contextlib import asynccontextmanager
import os

from fastapi import FastAPI

from mission_commander.api.dependencies import get_app_state
from mission_commander.adapters.camera.mock_camera_client import MockCameraClient
from mission_commander.adapters.camera.picamera_client import PiCameraClient
from mission_commander.adapters.cv.cv_service_client import CvServiceClient


@asynccontextmanager
async def lifespan(app: FastAPI):
    app_state = get_app_state()

    use_mock_camera = os.getenv("MISSION_COMMANDER_USE_MOCK_CAMERA", "true").lower() == "true"

    if use_mock_camera:
        app_state.camera_client = MockCameraClient()
    else:
        app_state.camera_client = PiCameraClient()

    app_state.cv_service_client = CvServiceClient(
        base_url=app_state.settings.cv_service_url,
        timeout_seconds=app_state.settings.cv_timeout_seconds
    )

    app_state.camera_client.startup()
    await app_state.cv_service_client.startup()

    app_state.telemetry_store.set_camera_ready(app_state.camera_client.is_ready)
    app_state.telemetry_store.set_cv_service_ready(app_state.cv_service_client.is_ready)

    try:
        yield
    finally:
        await app_state.cv_service_client.shutdown()
        app_state.camera_client.shutdown()
