namespace KnightPi.DuckDetection.Abstractions;

public interface IDuckDetectorFactory
{
    IDuckDetector Create(DuckDetectorKind kind);
}