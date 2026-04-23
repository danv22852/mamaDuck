import json
import threading
import time
from typing import Any

import serial

from mission_commander.adapters.esp32.protocol import (
    normalize_outbound_json,
    normalize_outbound_text,
)


class Esp32SerialClient:
    def __init__(self, port: str, baud_rate: int, timeout_seconds: float = 2.0) -> None:
        self._port = port
        self._baud_rate = baud_rate
        self._timeout_seconds = timeout_seconds
        self._serial: serial.Serial | None = None
        self._lock = threading.Lock()

    @property
    def connected(self) -> bool:
        return self._serial is not None and self._serial.is_open

    @property
    def port(self) -> str:
        return self._port

    @property
    def baud_rate(self) -> int:
        return self._baud_rate

    def connect(self) -> None:
        with self._lock:
            if self.connected:
                return

            self._serial = serial.Serial(
                port=self._port,
                baudrate=self._baud_rate,
                timeout=self._timeout_seconds,
                write_timeout=self._timeout_seconds
            )

            time.sleep(0.2)
            self._serial.reset_input_buffer()
            self._serial.reset_output_buffer()
            self._drain_startup_noise_locked()

    def disconnect(self) -> None:
        with self._lock:
            if self._serial is not None:
                try:
                    if self._serial.is_open:
                        self._serial.close()
                finally:
                    self._serial = None

    def send_text_command(self, command_text: str) -> dict[str, Any]:
        with self._lock:
            self._ensure_connected()

            assert self._serial is not None

            outbound = normalize_outbound_text(command_text)
            self._serial.write(outbound)
            self._serial.flush()

            response_line = self._read_line_locked()

            parsed_json = None
            try:
                parsed_json = json.loads(response_line)
            except Exception:
                parsed_json = None

            ok = True
            if parsed_json is not None and parsed_json.get("status") == "ERROR":
                ok = False

            return {
                "ok": ok,
                "port": self._port,
                "baudRate": self._baud_rate,
                "sent": command_text.strip(),
                "response": response_line,
                "responseJson": parsed_json
            }

    def send_json_command(self, payload: dict[str, Any]) -> dict[str, Any]:
        with self._lock:
            self._ensure_connected()

            assert self._serial is not None

            outbound = normalize_outbound_json(payload)
            self._serial.write(outbound)
            self._serial.flush()

            response_line, parsed_json, skipped_lines = self._read_json_response_locked(
                expected_transaction_id=payload.get("transactionId")
            )

            return {
                "ok": parsed_json.get("status") != "ERROR",
                "port": self._port,
                "baudRate": self._baud_rate,
                "sent": payload,
                "response": response_line,
                "responseJson": parsed_json,
                "skippedLines": skipped_lines
            }

    def ping(self) -> dict[str, Any]:
        return self.send_json_command(
            {
                "transactionId": "ping",
                "command": "PING",
                "args": {}
            }
        )

    def _ensure_connected(self) -> None:
        if not self.connected:
            raise RuntimeError(f"ESP32 serial client is not connected. Port={self._port}")

    def _drain_startup_noise_locked(self) -> None:
        assert self._serial is not None

        original_timeout = self._serial.timeout
        try:
            self._serial.timeout = 0.05
            deadline = time.monotonic() + 1.0
            while time.monotonic() < deadline:
                raw = self._serial.readline()
                if not raw:
                    continue
        finally:
            self._serial.timeout = original_timeout

    def _read_json_response_locked(self, expected_transaction_id: str | None) -> tuple[str, dict[str, Any], list[str]]:
        skipped_lines: list[str] = []
        deadline = time.monotonic() + self._timeout_seconds

        while time.monotonic() < deadline:
            line = self._read_line_locked()

            try:
                parsed = json.loads(line)
            except Exception:
                skipped_lines.append(line)
                continue

            if not isinstance(parsed, dict):
                skipped_lines.append(line)
                continue

            if parsed.get("type") == "NOTIFICATION":
                skipped_lines.append(line)
                continue

            if expected_transaction_id is not None:
                response_transaction_id = parsed.get("transactionId")
                if response_transaction_id != expected_transaction_id:
                    skipped_lines.append(line)
                    continue

            return line, parsed, skipped_lines

        raise TimeoutError("Timed out waiting for matching ESP32 JSON response")

    def _read_line_locked(self) -> str:
        assert self._serial is not None

        raw = self._serial.readline()
        if not raw:
            raise TimeoutError("Timed out waiting for ESP32 response")

        return raw.decode("utf-8", errors="replace").strip()
