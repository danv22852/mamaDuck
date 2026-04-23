import uuid

from mission_commander.core.application_state import ApplicationState
from mission_commander.domain.models import CommandResult


class RobotService:
    def __init__(self, app_state: ApplicationState) -> None:
        self._app_state = app_state

    def connect_esp32(self) -> CommandResult:
        self._app_state.esp32_client.connect()
        self._app_state.telemetry_store.set_esp32_connected(
            self._app_state.esp32_client.connected
        )

        return CommandResult(
            ok=True,
            action="ESP32_CONNECT",
            detail="esp32 connected",
            data={
                "connected": self._app_state.esp32_client.connected,
                "port": self._app_state.esp32_client.port,
                "baudRate": self._app_state.esp32_client.baud_rate
            }
        )

    def disconnect_esp32(self) -> CommandResult:
        self._app_state.esp32_client.disconnect()
        self._app_state.telemetry_store.set_esp32_connected(False)

        return CommandResult(
            ok=True,
            action="ESP32_DISCONNECT",
            detail="esp32 disconnected",
            data={
                "connected": self._app_state.esp32_client.connected
            }
        )

    def ping_esp32(self) -> CommandResult:
        return self.send_json_command(
            self._build_json_command("PING")
        )

    def send_text_command(self, command_text: str) -> CommandResult:
        response = self._app_state.esp32_client.send_text_command(command_text)

        return CommandResult(
            ok=bool(response.get("ok", False)),
            action="ESP32_SEND_TEXT",
            detail="text command sent",
            data=response
        )

    def send_json_command(self, payload: dict) -> CommandResult:
        response = self._app_state.esp32_client.send_json_command(payload)
        response_json = response.get("responseJson") or {}
        status = response_json.get("status")
        detail = self._get_response_detail(response_json)

        return CommandResult(
            ok=bool(response.get("ok", False)) and status != "ERROR",
            action="ESP32_SEND_JSON",
            detail=detail or "json command sent",
            data=response
        )

    def _build_json_command(self, command_name: str, args: dict | None = None) -> dict:
        return {
            "transactionId": str(uuid.uuid4()),
            "command": command_name,
            "args": args or {}
        }

    def _get_response_detail(self, response_json: dict | None) -> str | None:
        if not response_json:
            return None

        error = response_json.get("error") or {}
        if isinstance(error, dict) and error.get("message"):
            return str(error["message"])

        command_name = response_json.get("command")
        status = response_json.get("status")
        if command_name and status:
            return f"{command_name} {status}"

        return None

    def status(self) -> CommandResult:
        return self.send_json_command(
            self._build_json_command("STATUS")
        )

    def cancel(self, target: str = "ALL") -> CommandResult:
        return self.send_json_command(
            self._build_json_command(
                "CANCEL",
                {
                    "target": target
                }
            )
        )

    def stop(self) -> CommandResult:
        return self.send_json_command(
            self._build_json_command("STOP")
        )

    def us_monitor_on(self, alarm_distance_mm: float | None = None) -> CommandResult:
        args: dict[str, float] = {}
        if alarm_distance_mm is not None:
            args["alarmDistanceMm"] = alarm_distance_mm

        return self.send_json_command(
            self._build_json_command("US_MONITOR_ON", args)
        )

    def us_monitor_off(self) -> CommandResult:
        return self.send_json_command(
            self._build_json_command("US_MONITOR_OFF")
        )

    def read_distance_mm(self) -> CommandResult:
        return self.us_scan(
            scan_angle_deg=1.0,
            steps=1,
            step_rotate_ms=1,
            rotate_speed=1,
            rotation="CCW"
        )

    def us_scan(
        self,
        scan_angle_deg: float,
        steps: int,
        step_rotate_ms: int,
        rotate_speed: int,
        rotation: str = "CCW"
    ) -> CommandResult:
        return self.send_json_command(
            self._build_json_command(
                "US_SCAN",
                {
                    "scanAngleDeg": scan_angle_deg,
                    "steps": steps,
                    "stepRotateMs": step_rotate_ms,
                    "rotateSpeed": rotate_speed,
                    "rotation": rotation
                }
            )
        )

    def pick_angles_speed(self, args: dict) -> CommandResult:
        return self.send_json_command(
            self._build_json_command("PICK_ANGLES_SPEED", args)
        )

    def drop_angles_speed(self, args: dict) -> CommandResult:
        return self.send_json_command(
            self._build_json_command("DROP_ANGLES_SPEED", args)
        )

    def move_arm_angles_speed(self, args: dict) -> CommandResult:
        return self.send_json_command(
            self._build_json_command("MOVE_ARM_ANGLES_SPEED", args)
        )

    def move_forward(self, duration_ms: int, speed: int) -> CommandResult:
        return self.send_json_command(
            self._build_json_command(
                "MOVE_FW",
                {
                    "durationMs": duration_ms,
                    "speed": speed
                }
            )
        )

    def move_backward(self, duration_ms: int, speed: int) -> CommandResult:
        return self.send_json_command(
            self._build_json_command(
                "MOVE_BW",
                {
                    "durationMs": duration_ms,
                    "speed": speed
                }
            )
        )

    def rotate_cw(self, duration_ms: int, speed: int) -> CommandResult:
        return self.send_json_command(
            self._build_json_command(
                "ROTATE_CW",
                {
                    "durationMs": duration_ms,
                    "speed": speed
                }
            )
        )

    def rotate_ccw(self, duration_ms: int, speed: int) -> CommandResult:
        return self.send_json_command(
            self._build_json_command(
                "ROTATE_CCW",
                {
                    "durationMs": duration_ms,
                    "speed": speed
                }
            )
        )
