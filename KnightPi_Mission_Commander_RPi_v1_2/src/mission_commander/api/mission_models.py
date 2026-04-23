from pydantic import BaseModel, Field


class StartMissionRequest(BaseModel):
    mission_name: str = Field(..., min_length=1)
    mission_id: str | None = None
    parameters: dict = Field(default_factory=dict)
