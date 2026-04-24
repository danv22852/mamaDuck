using KnightPi.DuckDetection.Abstractions;

namespace KnightPi.DuckDetection.Ml;

public sealed class MlDuckDetector : IDuckDetector
{
    public DuckDetectionResult Analyze(DuckDetectionRequest request)
    {
        if (request is null)
        {
            throw new ArgumentNullException(nameof(request));
        }

        return new DuckDetectionResult
        {
            Success = false,
            ErrorMessage = "ML detector is not implemented yet.",
            HasDuck = false,
            HorizontalPosition = DuckHorizontalPosition.None,
            HorizontalOffsetNormalized = 0.0,
            HorizontalOffsetPercent = 0.0,
            WidthPercentOfFrame = 0.0,
            HeightPercentOfFrame = 0.0,
            AreaPercentOfFrame = 0.0,
            MatchScore = 0.0,
            FrameSize = new ImageSize(),
            DuckBoundingBox = new BoundingBox(),
            DebugImageOutputPath = request.DebugImageOutputPath
        };
    }
}