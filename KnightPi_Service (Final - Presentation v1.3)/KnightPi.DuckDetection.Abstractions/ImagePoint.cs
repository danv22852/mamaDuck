namespace KnightPi.DuckDetection.Abstractions;

public sealed class ImagePoint
{
    public int X { get; init; }
    public int Y { get; init; }

    public double XPercentOfFrame { get; init; }
    public double YPercentOfFrame { get; init; }
}