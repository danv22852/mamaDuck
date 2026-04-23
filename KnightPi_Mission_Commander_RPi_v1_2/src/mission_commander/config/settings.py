from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    app_name: str = "KnightPi Mission Commander"
    app_host: str = "0.0.0.0"
    app_port: int = 8080

    esp32_serial_port: str = "COM5"
    esp32_baud_rate: int = 115200
    esp32_timeout_seconds: float = 2.0

    cv_service_url: str = "http://127.0.0.1:5080"
    cv_timeout_seconds: float = 10.0
    cv_detector_kind: str = "cv"
    cv_save_debug_image: bool = True

    artifact_root: str = "artifacts"
    recent_event_limit: int = 100

    model_config = SettingsConfigDict(
        env_prefix="MISSION_COMMANDER_",
        extra="ignore"
    )


settings = Settings()
