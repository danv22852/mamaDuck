import uvicorn
from mission_commander.api.app import create_app
from mission_commander.config.settings import settings

app = create_app()

if __name__ == "__main__":
    uvicorn.run(app, host=settings.app_host, port=settings.app_port)
