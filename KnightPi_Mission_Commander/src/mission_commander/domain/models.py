from pydantic import BaseModel, Field
from mission_commander.domain.cv_models import CvAnalysisResult
from mission_commander.domain.enums import MissionStatus, MissionPhase


class EventRecord(BaseModel):
    timestamp_utc: str
    run_id: str | None = None
    kind: str
    category: str
    message: str
    data: dict | None = None


class PreflightCheckResult(BaseModel):
    name: str
    ok: bool
    detail: str | None = None
    data: dict | None = None


class PreflightReport(BaseModel):
    run_id: str
    success: bool
    started_at_utc: str
    finished_at_utc: str
    checks: list[PreflightCheckResult] = Field(default_factory=list)
    summary: dict | None = None


class MissionSnapshot(BaseModel):
    mission_id: str | None = None
    mission_name: str | None = None
    phase: MissionPhase = MissionPhase.NONE
    status: MissionStatus = MissionStatus.IDLE
    current_step: str | None = None
    detail: str | None = None


class TelemetrySnapshot(BaseModel):
    mission: MissionSnapshot
    esp32_connected: bool = False
    camera_ready: bool = False
    cv_service_ready: bool = False
    last_cv_result: CvAnalysisResult | None = None
    last_preflight_report: PreflightReport | None = None
    recent_events: list[EventRecord] = Field(default_factory=list)


class CommandResult(BaseModel):
    ok: bool
    action: str
    detail: str | None = None
    data: dict | None = None
