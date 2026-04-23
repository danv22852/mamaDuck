import uuid
from datetime import datetime, UTC

from mission_commander.application.robot_service import RobotService
from mission_commander.application.vision_service import VisionService
from mission_commander.domain.models import PreflightCheckResult, PreflightReport


class PreflightService:
    def __init__(self, app_state) -> None:
        self._app_state = app_state
        self._robot_service = RobotService(app_state)
        self._vision_service = VisionService(app_state)

    async def run(self) -> PreflightReport:
        run_id = f"preflight-{uuid.uuid4()}"
        started_at = self._utc_now()

        self._app_state.run_logger.start_run(
            run_id=run_id,
            run_kind="preflight",
            run_name="preflight",
            parameters={}
        )

        checks: list[PreflightCheckResult] = []

        checks.append(self._check_camera(run_id))
        checks.append(self._check_cv_client(run_id))
        checks.append(self._check_mission_registry(run_id))
        checks.append(await self._check_vision_pipeline(run_id))
        checks.append(self._check_esp32(run_id))

        success = all(c.ok for c in checks)

        report = PreflightReport(
            run_id=run_id,
            success=success,
            started_at_utc=started_at,
            finished_at_utc=self._utc_now(),
            checks=checks,
            summary={
                "checkCount": len(checks),
                "passedCount": len([c for c in checks if c.ok]),
                "failedCount": len([c for c in checks if not c.ok])
            }
        )

        self._app_state.telemetry_store.set_preflight_report(report)
        self._app_state.run_logger.finish_run(
            run_id=run_id,
            outcome="SUCCESS" if success else "FAILED",
            summary_data=report.model_dump()
        )

        return report

    def _check_camera(self, run_id: str) -> PreflightCheckResult:
        ok = self._app_state.camera_client is not None and self._app_state.camera_client.is_ready
        result = PreflightCheckResult(
            name="camera_ready",
            ok=ok,
            detail="camera client initialized" if ok else "camera client not ready"
        )
        self._log_check(run_id, result)
        return result

    def _check_cv_client(self, run_id: str) -> PreflightCheckResult:
        ok = self._app_state.cv_service_client is not None and self._app_state.cv_service_client.is_ready
        result = PreflightCheckResult(
            name="cv_client_ready",
            ok=ok,
            detail="cv client initialized" if ok else "cv client not ready"
        )
        self._log_check(run_id, result)
        return result

    def _check_mission_registry(self, run_id: str) -> PreflightCheckResult:
        registry = getattr(self._app_state, "mission_registry", None)
        ok = registry is not None and registry.get("duck_pick_and_drop_cycle") is not None
        result = PreflightCheckResult(
            name="mission_registered",
            ok=ok,
            detail="duck_pick_and_drop_cycle mission registered" if ok else "required mission not registered"
        )
        self._log_check(run_id, result)
        return result

    async def _check_vision_pipeline(self, run_id: str) -> PreflightCheckResult:
        try:
            result = await self._vision_service.analyze_image()
            ok = result.ok
            detail = result.detail or "vision pipeline executed"
            data = {
                "action": result.action
            }
        except Exception as ex:
            ok = False
            detail = str(ex)
            data = None

        check = PreflightCheckResult(
            name="vision_pipeline",
            ok=ok,
            detail=detail,
            data=data
        )
        self._log_check(run_id, check)
        return check

    def _check_esp32(self, run_id: str) -> PreflightCheckResult:
        try:
            self._robot_service.connect_esp32()
            ping_result = self._robot_service.ping_esp32()
            ok = ping_result.ok
            detail = "esp32 connect + ping succeeded" if ok else (ping_result.detail or "esp32 ping failed")
            data = ping_result.data
        except Exception as ex:
            ok = False
            detail = str(ex)
            data = None

        check = PreflightCheckResult(
            name="esp32_connect_ping",
            ok=ok,
            detail=detail,
            data=data
        )
        self._log_check(run_id, check)
        return check

    def _log_check(self, run_id: str, result: PreflightCheckResult) -> None:
        self._app_state.run_logger.log_event(
            run_id=run_id,
            kind="INFO" if result.ok else "ERROR",
            category="preflight",
            message=f"{result.name}: {result.detail}",
            data=result.model_dump()
        )

    def _utc_now(self) -> str:
        return datetime.now(UTC).isoformat()
