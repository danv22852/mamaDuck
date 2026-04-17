using System.Text.Json;
using KnightPi.DuckDetection.Abstractions;
using KnightPi.DuckDetection.Core;

try
{
    if (args.Length < 2)
    {
        WriteUsage();
        return;
    }

    DuckDetectorKind detectorKind = ParseDetectorKind(args[0]);
    string imagePath = args[1];
    string? debugOutputPath = args.Length >= 3 ? args[2] : null;

    IDuckDetectorFactory factory = new DuckDetectorFactory();
    IDuckDetector detector = factory.Create(detectorKind);

    DuckDetectionRequest request = new()
    {
        ImagePath = imagePath,
        DebugImageOutputPath = debugOutputPath
    };

    DuckDetectionResult result = detector.Analyze(request);

    PrintHumanReadableResult(result);
    Console.WriteLine();
    PrintJsonResult(result);
}
catch (Exception ex)
{
    Console.WriteLine("Unhandled error:");
    Console.WriteLine(ex.ToString());
}

static DuckDetectorKind ParseDetectorKind(string rawValue)
{
    return rawValue.Trim().ToLowerInvariant() switch
    {
        "cv" => DuckDetectorKind.Cv,
        "ml" => DuckDetectorKind.Ml,
        _ => throw new ArgumentException("First argument must be 'cv' or 'ml'.")
    };
}

static void WriteUsage()
{
    Console.WriteLine("Usage:");
    Console.WriteLine("  KnightPi.DuckDetection.ConsoleDemo <cv|ml> <imagePath> [debugImageOutputPath]");
    Console.WriteLine();
    Console.WriteLine("Examples:");
    Console.WriteLine(@"  KnightPi.DuckDetection.ConsoleDemo cv C:\images\duck1.jpg");
    Console.WriteLine(@"  KnightPi.DuckDetection.ConsoleDemo cv C:\images\duck1.jpg C:\images\duck1.debug.jpg");
}

static void PrintHumanReadableResult(DuckDetectionResult result)
{
    Console.WriteLine("=== Duck Detection Result ===");
    Console.WriteLine($"Success: {result.Success}");

    if (!string.IsNullOrWhiteSpace(result.ErrorMessage))
    {
        Console.WriteLine($"ErrorMessage: {result.ErrorMessage}");
    }

    Console.WriteLine($"HasDuck: {result.HasDuck}");
    Console.WriteLine($"HorizontalPosition: {result.HorizontalPosition}");
    Console.WriteLine($"HorizontalOffsetNormalized: {result.HorizontalOffsetNormalized:F4}");
    Console.WriteLine($"HorizontalOffsetPercent: {result.HorizontalOffsetPercent:F2}%");
    Console.WriteLine($"WidthPercentOfFrame: {result.WidthPercentOfFrame:F2}%");
    Console.WriteLine($"HeightPercentOfFrame: {result.HeightPercentOfFrame:F2}%");
    Console.WriteLine($"AreaPercentOfFrame: {result.AreaPercentOfFrame:F2}%");
    Console.WriteLine($"MatchScore: {result.MatchScore:F4}");
    Console.WriteLine($"FrameSize: {result.FrameSize.Width} x {result.FrameSize.Height}");
    Console.WriteLine(
        $"DuckBoundingBox: X={result.DuckBoundingBox.X}, Y={result.DuckBoundingBox.Y}, W={result.DuckBoundingBox.Width}, H={result.DuckBoundingBox.Height}");

    if (!string.IsNullOrWhiteSpace(result.DebugImageOutputPath))
    {
        Console.WriteLine($"DebugImageOutputPath: {result.DebugImageOutputPath}");
    }
}

static void PrintJsonResult(DuckDetectionResult result)
{
    JsonSerializerOptions options = new()
    {
        WriteIndented = true
    };

    string json = JsonSerializer.Serialize(result, options);

    Console.WriteLine("=== JSON ===");
    Console.WriteLine(json);
}