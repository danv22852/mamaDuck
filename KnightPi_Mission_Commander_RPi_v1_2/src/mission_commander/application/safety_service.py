from mission_commander.application.robot_service import RobotService


class SafetyService:
    def __init__(self, app_state) -> None:
        self._app_state = app_state
        self._robot_service = RobotService(app_state)

    def safe_stop(self, run_id: str, reason: str) -> None:
        self._app_state.run_logger.log_event(
            run_id=run_id,
            kind="WARN",
            category="safety",
            message="safe stop requested",
            data={"reason": reason}
        )

        try:
            self._robot_service.stop()
        except Exception as ex:
            self._app_state.run_logger.log_event(
                run_id=run_id,
                kind="ERROR",
                category="safety",
                message="failed to issue STOP during safe stop",
                data={"error": str(ex)}
            )
