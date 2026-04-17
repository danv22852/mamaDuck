namespace KnightPi.DuckDetection.Abstractions;

public sealed class DuckDetectionRequest
{
    public required string ImagePath { get; init; }

    // optional path where a debug image with overlays will be saved
    public string? DebugImageOutputPath { get; init; }
}