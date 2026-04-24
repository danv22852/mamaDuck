namespace KnightPi.DuckDetection.Abstractions;

public sealed class BoundingBox
{
    public int X { get; init; }
    public int Y { get; init; }
    public int Width { get; init; }
    public int Height { get; init; }
}