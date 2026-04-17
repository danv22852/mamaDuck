namespace KnightPi.DuckDetection.Abstractions;

public sealed class DuckCandidateResult
{
    public BoundingBox BoundingBox { get; init; } = new();
    public ImagePoint BottomCenterPoint { get; init; } = new();

    public bool IsInsideBoundary { get; init; }

    public double HorizontalOffsetPercent { get; init; }
    public double BottomOffsetPercentFromBottom { get; init; }

    public double WidthPercentOfFrame { get; init; }
    public double HeightPercentOfFrame { get; init; }
    public double AreaPercentOfFrame { get; init; }

    public double MatchScore { get; init; }
}