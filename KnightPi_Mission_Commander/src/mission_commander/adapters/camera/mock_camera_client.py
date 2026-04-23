import os
import threading

from mission_commander.adapters.camera.camera_client import CameraClient


class MockCameraClient(CameraClient):
    def __init__(self) -> None:
        self._ready = False
        self._lock = threading.Lock()
        self._image_path = os.getenv(
            "MISSION_COMMANDER_MOCK_IMAGE_PATH",
            r"testdata\duck.jpg"
        )

    def startup(self) -> None:
        with self._lock:
            if not os.path.exists(self._image_path):
                raise RuntimeError(
                    f"Mock camera image file does not exist: {self._image_path}"
                )

            self._ready = True

    def shutdown(self) -> None:
        with self._lock:
            self._ready = False

    def capture_jpeg(self) -> bytes:
        if not self._ready:
            raise RuntimeError("Mock camera is not ready")

        with open(self._image_path, "rb") as f:
            return f.read()

    @property
    def is_ready(self) -> bool:
        return self._ready
