from fastapi import APIRouter, Depends, HTTPException

from mission_commander.api.dependencies import get_app_state
from mission_commander.api.mission_models import StartMissionRequest
from mission_commander.application.mission_coordinator import MissionCoordinator
from mission_commander.core.application_state import ApplicationState

router = APIRouter()


def get_mission_coordinator(
    app_state: ApplicationState = Depends(get_app_state)
) -> MissionCoordinator:
    return MissionCoordinator(app_state)


@router.get("/missions")
async def list_missions(
    mission_coordinator: MissionCoordinator = Depends(get_mission_coordinator)
):
    try:
        return mission_coordinator.list_missions()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/mission/start")
async def start_mission(
    request: StartMissionRequest,
    mission_coordinator: MissionCoordinator = Depends(get_mission_coordinator)
):
    try:
        return mission_coordinator.start_mission(
            mission_name=request.mission_name,
            mission_id=request.mission_id,
            parameters=request.parameters
        ).model_dump(by_alias=True)
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/mission/pause")
async def pause_mission(
    mission_coordinator: MissionCoordinator = Depends(get_mission_coordinator)
):
    try:
        return mission_coordinator.pause_mission().model_dump(by_alias=True)
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/mission/resume")
async def resume_mission(
    mission_coordinator: MissionCoordinator = Depends(get_mission_coordinator)
):
    try:
        return mission_coordinator.resume_mission().model_dump(by_alias=True)
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/mission/abort")
async def abort_mission(
    mission_coordinator: MissionCoordinator = Depends(get_mission_coordinator)
):
    try:
        return mission_coordinator.abort_mission().model_dump(by_alias=True)
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.get("/mission/status")
async def mission_status(
    mission_coordinator: MissionCoordinator = Depends(get_mission_coordinator)
):
    try:
        return mission_coordinator.get_status()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))
