import io
import threading

from mission_commander.adapters.camera.camera_client import CameraClient

try:
    from picamera2 import Picamera2
except Exception:
    Picamera2 = None


class PiCameraClient(CameraClient):
    def __init__(self) -> None:
        self._camera = None
        self._ready = False
        self._lock = threading.Lock()

    def startup(self) -> None:
        if Picamera2 is None:
            raise RuntimeError("Picamera2 is not installed in this environment")

        with self._lock:
            if self._ready:
                return

            self._camera = Picamera2()
            config = self._camera.create_still_configuration(
                main={"size": (640, 480)}
            )
            self._camera.configure(config)
            self._camera.start()
            self._ready = True

    def shutdown(self) -> None:
        with self._lock:
            if self._camera is not None:
                try:
                    self._camera.stop()
                finally:
                    self._camera = None
                    self._ready = False

    def capture_jpeg(self) -> bytes:
        if not self._ready or self._camera is None:
            raise RuntimeError("Camera is not ready")

        with self._lock:
            stream = io.BytesIO()
            image = self._camera.capture_image()
            image.save(stream, format="JPEG")
            return stream.getvalue()

    @property
    def is_ready(self) -> bool:
        return self._ready
