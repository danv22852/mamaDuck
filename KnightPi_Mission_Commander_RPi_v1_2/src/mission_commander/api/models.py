from pydantic import BaseModel, Field


class Esp32TextCommandRequest(BaseModel):
    command: str = Field(..., min_length=1)


class Esp32JsonCommandRequest(BaseModel):
    payload: dict


class TimedMoveRequest(BaseModel):
    duration_ms: int = Field(..., gt=0)
    speed: int = Field(..., ge=1, le=255)
