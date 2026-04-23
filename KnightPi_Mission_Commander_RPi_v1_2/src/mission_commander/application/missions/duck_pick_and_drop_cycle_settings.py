from pydantic import BaseModel, Field


class DuckPickAndDropCycleSettings(BaseModel):
    dry_run_no_motion: bool = False

    search_target_timeout_seconds: float = Field(60.0, gt=0)
    approach_target_timeout_seconds: float = Field(60.0, gt=0)
    pickup_target_timeout_seconds: float = Field(45.0, gt=0)
    search_drop_zone_timeout_seconds: float = Field(60.0, gt=0)
    move_to_drop_zone_timeout_seconds: float = Field(60.0, gt=0)
    drop_target_timeout_seconds: float = Field(45.0, gt=0)

    search_scan_angle_deg: float = 360.0
    search_scan_steps: int = 48
    search_scan_rotate_speed: int = Field(255, ge=1, le=255)
    search_scan_step_rotate_ms: int = Field(200, ge=1)
    search_scan_rotation: str = "CCW"

    rotation_speed: int = Field(255, ge=1, le=255)
    rotate_ms_per_degree: float = Field(8.0, gt=0)
    center_tolerance_percent: float = Field(8.0, ge=0)

    forward_speed: int = Field(255, ge=1, le=255)
    backward_speed: int = Field(255, ge=1, le=255)
    forward_ms_per_mm: float = Field(2.0, gt=0)
    backward_ms_per_mm: float = Field(2.0, gt=0)

    arm_pickup_distance_mm: float = Field(120.0, gt=0)
    pickup_distance_margin_mm: float = Field(50.0, ge=0)
    distance_tolerance_mm: float = Field(20.0, ge=0)

    pickup_verify_distance_delta_mm: float = Field(30.0, ge=0)
    pickup_recovery_backoff_mm: float = Field(50.0, ge=0)

    drop_search_step_angle_deg: float = Field(15.0, gt=0)
    drop_search_rotation_speed: int = Field(255, ge=1, le=255)
    drop_search_step_rotate_ms: int = Field(150, ge=1)
    drop_zone_center_tolerance_percent: float = Field(8.0, ge=0)
    drop_zone_bottom_target_percent: float = Field(20.0, ge=0)

    detector_kind: str = "cv"
    save_debug_image: bool = True

    pickup_args: dict = Field(default_factory=dict)
    drop_args: dict = Field(default_factory=dict)
