import asyncio

from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from mission_commander.api.connection_manager import ConnectionManager
from mission_commander.api.dependencies import get_app_state

router = APIRouter()
manager = ConnectionManager()


@router.websocket("/ws/telemetry")
async def telemetry_websocket(websocket: WebSocket):
    app_state = get_app_state()
    await manager.connect(websocket)

    try:
        while True:
            snapshot = app_state.telemetry_store.get_snapshot().model_dump()
            snapshot["telemetryConnectionCount"] = manager.connection_count

            await websocket.send_json(snapshot)
            await asyncio.sleep(0.5)

    except WebSocketDisconnect:
        manager.disconnect(websocket)
    except Exception:
        manager.disconnect(websocket)
        raise
