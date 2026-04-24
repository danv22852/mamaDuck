namespace KnightPi.DuckDetection.Abstractions;

public interface IDuckDetector
{
    DuckDetectionResult Analyze(DuckDetectionRequest request);
}