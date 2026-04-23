from fastapi import FastAPI
from mission_commander.api.routes_health import router as health_router
from mission_commander.api.routes_mission import router as mission_router
from mission_commander.api.routes_manual import router as manual_router
from mission_commander.api.routes_preflight import router as preflight_router
from mission_commander.api.routes_vision import router as vision_router
from mission_commander.api.websocket import router as websocket_router
from mission_commander.config.settings import settings
from mission_commander.core.lifespan import lifespan


def create_app() -> FastAPI:
    app = FastAPI(title=settings.app_name, lifespan=lifespan)

    app.include_router(health_router, prefix="/api")
    app.include_router(mission_router, prefix="/api")
    app.include_router(manual_router, prefix="/api")
    app.include_router(preflight_router, prefix="/api")
    app.include_router(vision_router, prefix="/api")
    app.include_router(websocket_router)

    return app
