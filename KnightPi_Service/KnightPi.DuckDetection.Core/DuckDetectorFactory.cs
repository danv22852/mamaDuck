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
                    MinHue = 18,
                    MaxHue = 40,
                    MinSaturation = 80,
                    MaxSaturation = 255,
                    MinValue = 80,
                    MaxValue = 255,

                    BlueMinHue = 95,
                    BlueMaxHue = 140,
                    BlueMinSaturation = 70,
                    BlueMaxSaturation = 255,
                    BlueMinValue = 50,
                    BlueMaxValue = 255,

                    PurpleMinHue = 125,
                    PurpleMaxHue = 170,
                    PurpleMinSaturation = 50,
                    PurpleMaxSaturation = 255,
                    PurpleMinValue = 50,
                    PurpleMaxValue = 255,

                    CenterTolerancePercent = 10.0
                }),

            DuckDetectorKind.Ml => new MlDuckDetector(),

            _ => throw new ArgumentOutOfRangeException(nameof(kind), kind, "Unsupported detector kind.")
        };
    }
}