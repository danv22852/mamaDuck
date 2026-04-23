from mission_commander.adapters.esp32.serial_client import Esp32SerialClient
from mission_commander.application.run_logger import RunLogger
from mission_commander.config.settings import settings
from mission_commander.core.mission_state import MissionState
from mission_commander.core.telemetry_store import TelemetryStore


class ApplicationState:
    def __init__(self) -> None:
        self.settings = settings
        self.mission_state = MissionState()
        self.telemetry_store = TelemetryStore(
            recent_event_limit=settings.recent_event_limit
        )
        self.run_logger = RunLogger(
            artifact_root=settings.artifact_root,
            telemetry_store=self.telemetry_store
        )
        self.esp32_client = Esp32SerialClient(
            port=settings.esp32_serial_port,
            baud_rate=settings.esp32_baud_rate,
            timeout_seconds=settings.esp32_timeout_seconds
        )
        self.camera_client = None
        self.cv_service_client = None
