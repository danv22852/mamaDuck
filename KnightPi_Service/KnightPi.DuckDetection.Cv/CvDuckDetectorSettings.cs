namespace KnightPi.DuckDetection.Cv;

public sealed class CvDuckDetectorSettings
{
    // yellow duck thresholds
    public int MinHue { get; init; } = 18;
    public int MaxHue { get; init; } = 40;
    public int MinSaturation { get; init; } = 80;
    public int MaxSaturation { get; init; } = 255;
    public int MinValue { get; init; } = 80;
    public int MaxValue { get; init; } = 255;

    // blue floor line thresholds
    public int BlueMinHue { get; init; } = 95;
    public int BlueMaxHue { get; init; } = 140;
    public int BlueMinSaturation { get; init; } = 70;
    public int BlueMaxSaturation { get; init; } = 255;
    public int BlueMinValue { get; init; } = 50;
    public int BlueMaxValue { get; init; } = 255;

    // purple drop-zone thresholds
    public int PurpleMinHue { get; init; } = 125;
    public int PurpleMaxHue { get; init; } = 170;
    public int PurpleMinSaturation { get; init; } = 50;
    public int PurpleMaxSaturation { get; init; } = 255;
    public int PurpleMinValue { get; init; } = 50;
    public int PurpleMaxValue { get; init; } = 255;

    public double CenterTolerancePercent { get; init; } = 10.0;

    // duck candidate filtering
    public double MinAreaPercentOfFrame { get; init; } = 0.20;
    public double MaxAreaPercentOfFrame { get; init; } = 90.0;
    public double MinAspectRatio { get; init; } = 0.40;
    public double MaxAspectRatio { get; init; } = 2.50;
    public double MinExtent { get; init; } = 0.20;
    public double MaxExtent { get; init; } = 1.00;

    // duck mask morphology
    public int OpenKernelSize { get; init; } = 5;
    public int CloseKernelSize { get; init; } = 9;

    // blue boundary mask morphology
    public int BlueOpenKernelSize { get; init; } = 5;
    public int BlueCloseKernelSize { get; init; } = 11;
    public int BlueDilateKernelSize { get; init; } = 13;

    // purple mask morphology
    public int PurpleOpenKernelSize { get; init; } = 5;
    public int PurpleCloseKernelSize { get; init; } = 11;

    // scoring
    public double AreaWeight { get; init; } = 0.55;
    public double ExtentWeight { get; init; } = 0.20;
    public double BottomWeight { get; init; } = 0.25;

    // navigable region / lane guidance
    public double GuideRowPercent { get; init; } = 0.75;
    public int BottomCenterSeedSearchRadiusPixels { get; init; } = 80;

    // drop zone
    public int MinDropZoneAreaPixels { get; init; } = 1200;
    public double DropZoneAlignmentToleranceDeg { get; init; } = 12.0;

    // hough lines
    public int HoughThreshold { get; init; } = 40;
    public double HoughMinLineLength { get; init; } = 80;
    public double HoughMaxLineGap { get; init; } = 25;
}