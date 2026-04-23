import json
from datetime import datetime, UTC
from pathlib import Path

from mission_commander.domain.models import EventRecord


class RunLogger:
    def __init__(self, artifact_root: str, telemetry_store) -> None:
        self._artifact_root = Path(artifact_root)
        self._telemetry_store = telemetry_store

    def start_run(
        self,
        run_id: str,
        run_kind: str,
        run_name: str,
        parameters: dict | None = None
    ) -> Path:
        run_dir = self._artifact_root / "runs" / run_id
        run_dir.mkdir(parents=True, exist_ok=True)

        summary = {
            "runId": run_id,
            "runKind": run_kind,
            "runName": run_name,
            "startedAtUtc": self._utc_now(),
            "parameters": parameters or {}
        }

        (run_dir / "summary.json").write_text(
            json.dumps(summary, indent=2),
            encoding="utf-8"
        )

        return run_dir

    def log_event(
        self,
        run_id: str,
        kind: str,
        category: str,
        message: str,
        data: dict | None = None
    ) -> None:
        event = EventRecord(
            timestamp_utc=self._utc_now(),
            run_id=run_id,
            kind=kind,
            category=category,
            message=message,
            data=data
        )

        run_dir = self._artifact_root / "runs" / run_id
        run_dir.mkdir(parents=True, exist_ok=True)

        with (run_dir / "events.jsonl").open("a", encoding="utf-8") as f:
            f.write(json.dumps(event.model_dump(), ensure_ascii=False) + "\n")

        self._telemetry_store.append_event(event)

    def finish_run(
        self,
        run_id: str,
        outcome: str,
        summary_data: dict | None = None
    ) -> None:
        run_dir = self._artifact_root / "runs" / run_id
        run_dir.mkdir(parents=True, exist_ok=True)

        finish_payload = {
            "finishedAtUtc": self._utc_now(),
            "outcome": outcome,
            "summaryData": summary_data or {}
        }

        (run_dir / "finish.json").write_text(
            json.dumps(finish_payload, indent=2),
            encoding="utf-8"
        )

    def _utc_now(self) -> str:
        return datetime.now(UTC).isoformat()
