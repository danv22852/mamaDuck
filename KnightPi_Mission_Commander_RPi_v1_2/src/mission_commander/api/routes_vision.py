from fastapi import APIRouter, Depends, HTTPException

from mission_commander.api.dependencies import get_app_state
from mission_commander.application.vision_service import VisionService
from mission_commander.core.application_state import ApplicationState

router = APIRouter()


def get_vision_service(
    app_state: ApplicationState = Depends(get_app_state)
) -> VisionService:
    return VisionService(app_state)


@router.post("/vision/capture")
async def capture_image(
    vision_service: VisionService = Depends(get_vision_service)
):
    try:
        return vision_service.capture_image().model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/vision/analyze")
async def analyze_image(
    vision_service: VisionService = Depends(get_vision_service)
):
    try:
        result = await vision_service.analyze_image()
        return result.model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))
