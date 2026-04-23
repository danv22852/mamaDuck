import json
from typing import Any


def normalize_outbound_text(command_text: str) -> bytes:
    return (command_text.strip() + "\r\n").encode("utf-8")


def normalize_outbound_json(payload: dict[str, Any]) -> bytes:
    json_text = json.dumps(payload, separators=(",", ":"))
    return (json_text + "\r\n").encode("utf-8")
