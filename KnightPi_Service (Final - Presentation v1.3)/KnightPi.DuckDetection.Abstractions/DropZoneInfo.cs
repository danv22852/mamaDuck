namespace KnightPi.DuckDetection.Abstractions;

public sealed class DropZoneInfo
{
    public bool Detected { get; init; }

    public BoundingBox BoundingBox { get; init; } = new();
    public ImagePoint CenterPoint { get; init; } = new();

    public double HorizontalOffsetPercent { get; init; }
    public double BottomOffsetPercentFromBottom { get; init; }

    public double WidthPercentOfFrame { get; init; }
    public double HeightPercentOfFrame { get; init; }
    public double AreaPercentOfFrame { get; init; }

    // angle of the long edge of the minimum-area rectangle
    public double LongEdgeAngleDeg { get; init; }

    // 0 = well aligned to a horizontal approach in the image
    public double AlignmentErrorDeg { get; init; }
    public bool IsHorizontallyAligned { get; init; }
}