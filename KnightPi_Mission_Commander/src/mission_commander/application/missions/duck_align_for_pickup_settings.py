from pydantic import BaseModel, Field


class DuckAlignForPickupSettings(BaseModel):
    connect_first: bool = True
    detector_kind: str = "cv"
    save_debug_image: bool = True

    max_search_turns: int = Field(4, ge=1, le=8)
    search_turn_degrees: float = Field(90.0, gt=0, le=180)
    search_rotation_speed: int = Field(255, ge=1, le=255)

    rotation_speed: int = Field(255, ge=1, le=255)
    rotate_ms_per_degree: float = Field(8.0, gt=0)
    rotate_ms_per_offset_percent: float = Field(10.0, gt=0)
    center_tolerance_percent: float = Field(8.0, ge=0)
    min_center_rotate_ms: int = Field(50, ge=1)
    max_center_rotate_ms: int = Field(50, ge=1)

    approach_forward_duration_ms: int = Field(10000, ge=100)
    approach_forward_speed: int = Field(255, ge=1, le=255)
    approach_alarm_distance_mm: float = Field(160.0, gt=0)

    max_initial_alignment_attempts: int = Field(20, ge=1, le=60)
    max_final_alignment_attempts: int = Field(20, ge=1, le=60)


