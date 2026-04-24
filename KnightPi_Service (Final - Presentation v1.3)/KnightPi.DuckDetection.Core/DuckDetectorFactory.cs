using KnightPi.DuckDetection.Abstractions;
using KnightPi.DuckDetection.Cv;
using KnightPi.DuckDetection.Ml;

namespace KnightPi.DuckDetection.Core;

public sealed class DuckDetectorFactory : IDuckDetectorFactory
{
    public IDuckDetector Create(DuckDetectorKind kind)
    {
        return kind switch
        {
            DuckDetectorKind.Cv => new CvDuckDetector(
                new CvDuckDetectorSettings
                {
                    MinHue = 22,
                    MaxHue = 36,
                    MinSaturation = 140,
                    MaxSaturation = 255,
                    MinValue = 120,
                    MaxValue = 255,

                    BlueMinHue = 105,
                    BlueMaxHue = 128,
                    BlueMinSaturation = 120,
                    BlueMaxSaturation = 255,
                    BlueMinValue = 25,
                    BlueMaxValue = 170,

                    PurpleMinHue = 125,
                    PurpleMaxHue = 170,
                    PurpleMinSaturation = 50,
                    PurpleMaxSaturation = 255,
                    PurpleMinValue = 50,
                    PurpleMaxValue = 255,

                    CenterTolerancePercent = 10.0,

                    MinAreaPercentOfFrame = 0.20,
                    MaxAreaPercentOfFrame = 12.0,
                    MinAspectRatio = 0.40,
                    MaxAspectRatio = 2.50,
                    MinExtent = 0.20,
                    MaxExtent = 1.00
                }),

            DuckDetectorKind.Ml => new MlDuckDetector(),

            _ => throw new ArgumentOutOfRangeException(nameof(kind), kind, "Unsupported detector kind.")
        };
    }
}