from enum import Enum
from pydantic import BaseModel, Field


class DuckHorizontalPosition(str, Enum):
    NONE = "None"
    LEFT = "Left"
    CENTER = "Center"
    RIGHT = "Right"


class BoundingBox(BaseModel):
    x: int = 0
    y: int = 0
    width: int = 0
    height: int = 0


class ImagePoint(BaseModel):
    x: int = 0
    y: int = 0
    x_percent_of_frame: float = Field(0.0, alias="xPercentOfFrame")
    y_percent_of_frame: float = Field(0.0, alias="yPercentOfFrame")

    model_config = {
        "populate_by_name": True
    }


class ImageSize(BaseModel):
    width: int = 0
    height: int = 0


class BlueBoundaryInfo(BaseModel):
    detected: bool = False
    traversable_region_detected: bool = Field(False, alias="traversableRegionDetected")
    bottom_center_inside_traversable_region: bool = Field(False, alias="bottomCenterInsideTraversableRegion")
    guide_row_y: int = Field(0, alias="guideRowY")
    guide_row_percent: float = Field(0.0, alias="guideRowPercent")
    lane_center_offset_percent: float = Field(0.0, alias="laneCenterOffsetPercent")
    lane_width_percent: float = Field(0.0, alias="laneWidthPercent")
    nearest_boundary_distance_pixels: float = Field(0.0, alias="nearestBoundaryDistancePixels")
    nearest_boundary_distance_percent_of_frame_height: float = Field(0.0, alias="nearestBoundaryDistancePercentOfFrameHeight")
    nearest_boundary_horizontal_offset_percent: float = Field(0.0, alias="nearestBoundaryHorizontalOffsetPercent")
    dominant_boundary_angle_deg: float = Field(0.0, alias="dominantBoundaryAngleDeg")
    boundary_approach_error_deg: float = Field(0.0, alias="boundaryApproachErrorDeg")
    bounding_box: BoundingBox = Field(default_factory=BoundingBox, alias="boundingBox")

    model_config = {
        "populate_by_name": True
    }


class DropZoneInfo(BaseModel):
    detected: bool = False
    bounding_box: BoundingBox = Field(default_factory=BoundingBox, alias="boundingBox")
    center_point: ImagePoint = Field(default_factory=ImagePoint, alias="centerPoint")
    horizontal_offset_percent: float = Field(0.0, alias="horizontalOffsetPercent")
    bottom_offset_percent_from_bottom: float = Field(0.0, alias="bottomOffsetPercentFromBottom")
    width_percent_of_frame: float = Field(0.0, alias="widthPercentOfFrame")
    height_percent_of_frame: float = Field(0.0, alias="heightPercentOfFrame")
    area_percent_of_frame: float = Field(0.0, alias="areaPercentOfFrame")
    long_edge_angle_deg: float = Field(0.0, alias="longEdgeAngleDeg")
    alignment_error_deg: float = Field(0.0, alias="alignmentErrorDeg")
    is_horizontally_aligned: bool = Field(False, alias="isHorizontallyAligned")

    model_config = {
        "populate_by_name": True
    }


class DuckCandidateResult(BaseModel):
    bounding_box: BoundingBox = Field(default_factory=BoundingBox, alias="boundingBox")
    bottom_center_point: ImagePoint = Field(default_factory=ImagePoint, alias="bottomCenterPoint")
    is_inside_boundary: bool = Field(False, alias="isInsideBoundary")
    horizontal_offset_percent: float = Field(0.0, alias="horizontalOffsetPercent")
    bottom_offset_percent_from_bottom: float = Field(0.0, alias="bottomOffsetPercentFromBottom")
    width_percent_of_frame: float = Field(0.0, alias="widthPercentOfFrame")
    height_percent_of_frame: float = Field(0.0, alias="heightPercentOfFrame")
    area_percent_of_frame: float = Field(0.0, alias="areaPercentOfFrame")
    match_score: float = Field(0.0, alias="matchScore")

    model_config = {
        "populate_by_name": True
    }


class CvAnalysisResult(BaseModel):
    success: bool = False
    error_message: str | None = Field(None, alias="errorMessage")
    has_duck: bool = Field(False, alias="hasDuck")
    horizontal_position: DuckHorizontalPosition = Field(DuckHorizontalPosition.NONE, alias="horizontalPosition")
    horizontal_offset_normalized: float = Field(0.0, alias="horizontalOffsetNormalized")
    horizontal_offset_percent: float = Field(0.0, alias="horizontalOffsetPercent")
    width_percent_of_frame: float = Field(0.0, alias="widthPercentOfFrame")
    height_percent_of_frame: float = Field(0.0, alias="heightPercentOfFrame")
    area_percent_of_frame: float = Field(0.0, alias="areaPercentOfFrame")
    match_score: float = Field(0.0, alias="matchScore")
    bottom_offset_percent_from_bottom: float = Field(0.0, alias="bottomOffsetPercentFromBottom")
    duck_bottom_center_point: ImagePoint = Field(default_factory=ImagePoint, alias="duckBottomCenterPoint")
    total_duck_candidates: int = Field(0, alias="totalDuckCandidates")
    eligible_duck_candidates: int = Field(0, alias="eligibleDuckCandidates")
    rejected_duck_candidates_outside_boundary: int = Field(0, alias="rejectedDuckCandidatesOutsideBoundary")
    duck_candidates: list[DuckCandidateResult] = Field(default_factory=list, alias="duckCandidates")
    blue_boundary: BlueBoundaryInfo = Field(default_factory=BlueBoundaryInfo, alias="blueBoundary")
    drop_zone: DropZoneInfo = Field(default_factory=DropZoneInfo, alias="dropZone")
    frame_size: ImageSize = Field(default_factory=ImageSize, alias="frameSize")
    duck_bounding_box: BoundingBox = Field(default_factory=BoundingBox, alias="duckBoundingBox")
    debug_image_output_path: str | None = Field(None, alias="debugImageOutputPath")

    model_config = {
        "populate_by_name": True
    }
