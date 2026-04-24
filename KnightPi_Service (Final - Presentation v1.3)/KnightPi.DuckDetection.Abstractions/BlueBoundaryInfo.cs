namespace KnightPi.DuckDetection.Abstractions;

public sealed class BlueBoundaryInfo
{
    public bool Detected { get; init; }
    public bool TraversableRegionDetected { get; init; }
    public bool BottomCenterInsideTraversableRegion { get; init; }

    public int GuideRowY { get; init; }
    public double GuideRowPercent { get; init; }

    public double LaneCenterOffsetPercent { get; init; }
    public double LaneWidthPercent { get; init; }

    public double NearestBoundaryDistancePixels { get; init; }
    public double NearestBoundaryDistancePercentOfFrameHeight { get; init; }
    public double NearestBoundaryHorizontalOffsetPercent { get; init; }

    // angle relative to image horizontal axis
    // 0 = horizontal, 90 = vertical
    public double DominantBoundaryAngleDeg { get; init; }

    // 0 is ideal if the robot is facing a boundary head-on
    public double BoundaryApproachErrorDeg { get; init; }

    public BoundingBox BoundingBox { get; init; } = new();
}