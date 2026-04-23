import json
import threading
import time
import uuid
from dataclasses import dataclass, field
from datetime import datetime, UTC
from typing import Any, Callable

import serial

from mission_commander.adapters.esp32.protocol import (
    normalize_outbound_json,
    normalize_outbound_text,
)


NotificationCallback = Callable[[dict[str, Any]], None]


@dataclass
class _PendingJsonResponse:
    transaction_id: str
    timeout_seconds: float
    created_monotonic: float = field(default_factory=time.monotonic)
    event: threading.Event = field(default_factory=threading.Event)
    response_line: str | None = None
    response_json: dict[str, Any] | None = None
    error: BaseException | None = None


@dataclass
class _PendingTextResponse:
    timeout_seconds: float
    created_monotonic: float = field(default_factory=time.monotonic)
    event: threading.Event = field(default_factory=threading.Event)
    response_line: str | None = None
    response_json: dict[str, Any] | None = None
    error: BaseException | None = None


class Esp32SerialClient:
    def __init__(self, port: str, baud_rate: int, timeout_seconds: float = 2.0) -> None:
        self._port = port
        self._baud_rate = baud_rate
        self._timeout_seconds = timeout_seconds

        self._serial: serial.Serial | None = None
        self._serial_poll_timeout_seconds = 0.2

        self._write_lock = threading.Lock()
        self._state_lock = threading.Lock()

        self._reader_thread: threading.Thread | None = None
        self._reader_stop_event: threading.Event | None = None

        self._pending_json: dict[str, _PendingJsonResponse] = {}
        self._pending_text: _PendingTextResponse | None = None

        self._notification_subscriptions: dict[str, dict[str, Any]] = {}

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
        with self._state_lock:
            if self.connected:
                return

            self._serial = serial.Serial(
                port=self._port,
                baudrate=self._baud_rate,
                timeout=self._serial_poll_timeout_seconds,
                write_timeout=self._timeout_seconds
            )

            time.sleep(0.2)
            self._serial.reset_input_buffer()
            self._serial.reset_output_buffer()
            self._drain_startup_noise_locked()

            self._reader_stop_event = threading.Event()
            self._reader_thread = threading.Thread(
                target=self._reader_loop,
                name="esp32-serial-reader",
                daemon=True
            )
            self._reader_thread.start()

    def disconnect(self) -> None:
        reader_thread: threading.Thread | None = None
        reader_stop_event: threading.Event | None = None
        serial_instance: serial.Serial | None = None

        with self._state_lock:
            reader_thread = self._reader_thread
            reader_stop_event = self._reader_stop_event
            serial_instance = self._serial

            self._reader_thread = None
            self._reader_stop_event = None
            self._serial = None

        if reader_stop_event is not None:
            reader_stop_event.set()

        if serial_instance is not None:
            try:
                if serial_instance.is_open:
                    serial_instance.close()
            except Exception:
                pass

        if reader_thread is not None and reader_thread.is_alive():
            reader_thread.join(timeout=1.0)

        self._fail_all_pending(RuntimeError("ESP32 serial client disconnected"))

    def subscribe_notification(
        self,
        topic: str,
        callback: NotificationCallback
    ) -> str:
        subscription_id = uuid.uuid4().hex
        normalized_topic = self._normalize_topic(topic)

        with self._state_lock:
            self._notification_subscriptions[subscription_id] = {
                "topic": normalized_topic,
                "callback": callback
            }

        return subscription_id

    def unsubscribe_notification(self, subscription_id: str) -> None:
        with self._state_lock:
            self._notification_subscriptions.pop(subscription_id, None)

    def send_text_command(self, command_text: str) -> dict[str, Any]:
        self._ensure_connected()

        pending = _PendingTextResponse(timeout_seconds=self._timeout_seconds)

        with self._state_lock:
            if self._pending_text is not None:
                raise RuntimeError("A legacy text request is already in flight")
            self._pending_text = pending

        try:
            with self._write_lock:
                self._ensure_connected()
                assert self._serial is not None

                outbound = normalize_outbound_text(command_text)
                self._serial.write(outbound)
                self._serial.flush()

            if not pending.event.wait(timeout=pending.timeout_seconds):
                with self._state_lock:
                    if self._pending_text is pending:
                        self._pending_text = None
                raise TimeoutError("Timed out waiting for ESP32 text response")

            if pending.error is not None:
                raise pending.error

            parsed_json = pending.response_json
            ok = True
            if isinstance(parsed_json, dict) and parsed_json.get("status") == "ERROR":
                ok = False

            return {
                "ok": ok,
                "port": self._port,
                "baudRate": self._baud_rate,
                "sent": command_text.strip(),
                "response": pending.response_line,
                "responseJson": parsed_json
            }
        finally:
            with self._state_lock:
                if self._pending_text is pending:
                    self._pending_text = None

    def send_json_command(self, payload: dict[str, Any]) -> dict[str, Any]:
        self._ensure_connected()

        outbound_payload = dict(payload)
        transaction_id = str(outbound_payload.get("transactionId") or uuid.uuid4())
        outbound_payload["transactionId"] = transaction_id

        timeout_seconds = self._resolve_timeout_seconds(outbound_payload)
        pending = _PendingJsonResponse(
            transaction_id=transaction_id,
            timeout_seconds=timeout_seconds
        )

        with self._state_lock:
            if transaction_id in self._pending_json:
                raise RuntimeError(f"Duplicate in-flight transactionId: {transaction_id}")
            self._pending_json[transaction_id] = pending

        try:
            with self._write_lock:
                self._ensure_connected()
                assert self._serial is not None

                outbound = normalize_outbound_json(outbound_payload)
                self._serial.write(outbound)
                self._serial.flush()

            if not pending.event.wait(timeout=timeout_seconds):
                with self._state_lock:
                    current = self._pending_json.get(transaction_id)
                    if current is pending:
                        self._pending_json.pop(transaction_id, None)
                raise TimeoutError(
                    f"Timed out waiting for matching ESP32 JSON response. "
                    f"transactionId={transaction_id} command={outbound_payload.get('command')}"
                )

            if pending.error is not None:
                raise pending.error

            response_json = pending.response_json or {}

            return {
                "ok": response_json.get("status") != "ERROR",
                "port": self._port,
                "baudRate": self._baud_rate,
                "sent": outbound_payload,
                "response": pending.response_line,
                "responseJson": response_json,
                "transactionId": transaction_id,
                "timeoutSeconds": timeout_seconds
            }
        finally:
            with self._state_lock:
                current = self._pending_json.get(transaction_id)
                if current is pending:
                    self._pending_json.pop(transaction_id, None)

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

        deadline = time.monotonic() + 1.0
        while time.monotonic() < deadline:
            raw = self._serial.readline()
            if not raw:
                continue

    def _reader_loop(self) -> None:
        while True:
            stop_event = self._reader_stop_event
            serial_instance = self._serial

            if stop_event is None or stop_event.is_set():
                return

            if serial_instance is None:
                return

            try:
                raw = serial_instance.readline()
            except serial.SerialException as ex:
                self._fail_all_pending(RuntimeError(f"ESP32 serial read failed: {ex}"))
                return
            except Exception as ex:
                self._fail_all_pending(RuntimeError(f"ESP32 serial reader crashed: {ex}"))
                return

            if not raw:
                continue

            line = raw.decode("utf-8", errors="replace").strip()
            if not line:
                continue

            self._dispatch_inbound_line(line)

    def _dispatch_inbound_line(self, line: str) -> None:
        parsed: dict[str, Any] | None = None

        try:
            candidate = json.loads(line)
            if isinstance(candidate, dict):
                parsed = candidate
        except Exception:
            parsed = None

        if parsed is None:
            self._try_resolve_pending_text(line=line, parsed_json=None)
            return

        if parsed.get("type") == "NOTIFICATION":
            self._publish_notification(parsed, line)
            return

        transaction_id = parsed.get("transactionId")
        if transaction_id is not None and self._try_resolve_pending_json(str(transaction_id), line, parsed):
            return

        self._try_resolve_pending_text(line=line, parsed_json=parsed)

    def _try_resolve_pending_json(
        self,
        transaction_id: str,
        line: str,
        parsed_json: dict[str, Any]
    ) -> bool:
        with self._state_lock:
            pending = self._pending_json.pop(transaction_id, None)

        if pending is None:
            return False

        pending.response_line = line
        pending.response_json = parsed_json
        pending.event.set()
        return True

    def _try_resolve_pending_text(
        self,
        line: str,
        parsed_json: dict[str, Any] | None
    ) -> bool:
        with self._state_lock:
            pending = self._pending_text
            if pending is None:
                return False
            self._pending_text = None

        pending.response_line = line
        pending.response_json = parsed_json
        pending.event.set()
        return True

    def _publish_notification(self, parsed_json: dict[str, Any], line: str) -> None:
        topic = self._extract_notification_topic(parsed_json)
        notification = {
            "topic": topic,
            "raw": parsed_json,
            "line": line,
            "receivedAtUtc": datetime.now(UTC).isoformat(),
            "data": parsed_json.get("data") if isinstance(parsed_json.get("data"), dict) else {}
        }

        with self._state_lock:
            subscribers = list(self._notification_subscriptions.items())

        for _, subscription in subscribers:
            subscribed_topic = subscription["topic"]
            if subscribed_topic != "*" and subscribed_topic != topic:
                continue

            callback = subscription["callback"]
            try:
                callback(notification)
            except Exception:
                pass

    def _extract_notification_topic(self, parsed_json: dict[str, Any]) -> str:
        event_name = parsed_json.get("event")
        if isinstance(event_name, str) and event_name.strip():
            return self._normalize_topic(event_name)

        command_name = parsed_json.get("command")
        if isinstance(command_name, str) and command_name.strip():
            return self._normalize_topic(command_name)

        return "UNKNOWN_NOTIFICATION"

    def _normalize_topic(self, topic: str | None) -> str:
        if topic is None:
            return "*"
        normalized = str(topic).strip()
        if not normalized:
            return "*"
        return normalized.upper()

    def _fail_all_pending(self, error: BaseException) -> None:
        with self._state_lock:
            pending_json = list(self._pending_json.values())
            pending_text = self._pending_text

            self._pending_json = {}
            self._pending_text = None

        for pending in pending_json:
            pending.error = error
            pending.event.set()

        if pending_text is not None:
            pending_text.error = error
            pending_text.event.set()

    def _resolve_timeout_seconds(self, payload: dict[str, Any]) -> float:
        default_timeout = max(self._timeout_seconds, 3.0)

        command_name = str(payload.get("command") or "").upper()
        args = payload.get("args") or {}
        if not isinstance(args, dict):
            args = {}

        if command_name in {"PING", "STATUS", "STOP", "CANCEL", "US_MONITOR_ON", "US_MONITOR_OFF"}:
            return default_timeout

        if command_name in {"MOVE_FW", "MOVE_BW", "MOVE_LEFT", "MOVE_RIGHT", "ROTATE_CW", "ROTATE_CCW"}:
            duration_ms = self._safe_int(args.get("durationMs"), fallback=0)
            return max(default_timeout, (duration_ms / 1000.0) + 3.0)

        if command_name == "US_SCAN":
            steps = max(1, self._safe_int(args.get("steps"), fallback=1))
            step_rotate_ms = max(1, self._safe_int(args.get("stepRotateMs"), fallback=1))
            estimated_seconds = 3.0 + (steps * ((step_rotate_ms / 1000.0) + 0.30))
            return max(default_timeout, estimated_seconds)

        if command_name in {"PICK_ANGLES_SPEED", "DROP_ANGLES_SPEED", "MOVE_ARM_ANGLES_SPEED"}:
            return max(default_timeout, 20.0)

        return default_timeout

    def _safe_int(self, value: Any, fallback: int) -> int:
        try:
            return int(value)
        except Exception:
            return fallback
