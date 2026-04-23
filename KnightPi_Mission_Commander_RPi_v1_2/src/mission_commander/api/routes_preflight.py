from fastapi import APIRouter, Depends, HTTPException

from mission_commander.api.dependencies import get_app_state
from mission_commander.application.preflight_service import PreflightService
from mission_commander.core.application_state import ApplicationState

router = APIRouter()


def get_preflight_service(
    app_state: ApplicationState = Depends(get_app_state)
) -> PreflightService:
    return PreflightService(app_state)


@router.post("/preflight/run")
async def run_preflight(
    preflight_service: PreflightService = Depends(get_preflight_service)
):
    try:
        return (await preflight_service.run()).model_dump(by_alias=True)
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))
