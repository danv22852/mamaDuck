namespace KnightPi.DuckDetection.Abstractions;

public sealed class DuckDetectionResult
{
    public bool Success { get; init; }
    public string? ErrorMessage { get; init; }

    public bool HasDuck { get; init; }

    public DuckHorizontalPosition HorizontalPosition { get; init; }

    // negative = left, positive = right, 0 = center
    public double HorizontalOffsetNormalized { get; init; }

    // negative = left %, positive = right %
    public double HorizontalOffsetPercent { get; init; }

    public double WidthPercentOfFrame { get; init; }
    public double HeightPercentOfFrame { get; init; }
    public double AreaPercentOfFrame { get; init; }

    public double MatchScore { get; init; }

    // new duck navigation fields
    public double BottomOffsetPercentFromBottom { get; init; }
    public ImagePoint DuckBottomCenterPoint { get; init; } = new();

    public int TotalDuckCandidates { get; init; }
    public int EligibleDuckCandidates { get; init; }
    public int RejectedDuckCandidatesOutsideBoundary { get; init; }

    public IReadOnlyList<DuckCandidateResult> DuckCandidates { get; init; } = Array.Empty<DuckCandidateResult>();

    public BlueBoundaryInfo BlueBoundary { get; init; } = new();
    public DropZoneInfo DropZone { get; init; } = new();

    public ImageSize FrameSize { get; init; } = new();
    public BoundingBox DuckBoundingBox { get; init; } = new();

    public string? DebugImageOutputPath { get; init; }
}