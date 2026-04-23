from fastapi import APIRouter, Depends, HTTPException

from mission_commander.api.dependencies import get_app_state
from mission_commander.api.models import (
    Esp32JsonCommandRequest,
    Esp32TextCommandRequest,
    TimedMoveRequest,
)
from mission_commander.application.robot_service import RobotService
from mission_commander.core.application_state import ApplicationState

router = APIRouter()


def get_robot_service(app_state: ApplicationState = Depends(get_app_state)) -> RobotService:
    return RobotService(app_state)


@router.post("/manual/esp32/connect")
async def manual_esp32_connect(robot_service: RobotService = Depends(get_robot_service)):
    try:
        return robot_service.connect_esp32().model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/esp32/disconnect")
async def manual_esp32_disconnect(robot_service: RobotService = Depends(get_robot_service)):
    try:
        return robot_service.disconnect_esp32().model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/esp32/ping")
async def manual_esp32_ping(robot_service: RobotService = Depends(get_robot_service)):
    try:
        return robot_service.ping_esp32().model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/esp32/send-text")
async def manual_esp32_send_text(
    request: Esp32TextCommandRequest,
    robot_service: RobotService = Depends(get_robot_service)
):
    try:
        result = robot_service.send_text_command(request.command).model_dump()
        result["detail"] = "ESP32 v2.3 is JSON-only. This endpoint is kept only for manual diagnostics."
        return result
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/esp32/status")
async def manual_esp32_status(robot_service: RobotService = Depends(get_robot_service)):
    try:
        return robot_service.status().model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/esp32/send-json")
async def manual_esp32_send_json(
    request: Esp32JsonCommandRequest,
    robot_service: RobotService = Depends(get_robot_service)
):
    try:
        return robot_service.send_json_command(request.payload).model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/drive/move-forward")
async def manual_move_forward(
    request: TimedMoveRequest,
    robot_service: RobotService = Depends(get_robot_service)
):
    try:
        return robot_service.move_forward(request.duration_ms, request.speed).model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/drive/move-backward")
async def manual_move_backward(
    request: TimedMoveRequest,
    robot_service: RobotService = Depends(get_robot_service)
):
    try:
        return robot_service.move_backward(request.duration_ms, request.speed).model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/drive/rotate-cw")
async def manual_rotate_cw(
    request: TimedMoveRequest,
    robot_service: RobotService = Depends(get_robot_service)
):
    try:
        return robot_service.rotate_cw(request.duration_ms, request.speed).model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/drive/rotate-ccw")
async def manual_rotate_ccw(
    request: TimedMoveRequest,
    robot_service: RobotService = Depends(get_robot_service)
):
    try:
        return robot_service.rotate_ccw(request.duration_ms, request.speed).model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))


@router.post("/manual/drive/stop")
async def manual_stop(robot_service: RobotService = Depends(get_robot_service)):
    try:
        return robot_service.stop().model_dump()
    except Exception as ex:
        raise HTTPException(status_code=500, detail=str(ex))
