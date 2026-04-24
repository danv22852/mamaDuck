using KnightPi.DuckDetection.Abstractions;
using KnightPi.DuckDetection.Api.Models;
using Microsoft.AspNetCore.Mvc;

namespace KnightPi.DuckDetection.Api.Controllers;

[ApiController]
[Route("api/[controller]")]
public sealed class DuckDetectionController : ControllerBase
{
    private readonly IDuckDetectorFactory _detectorFactory;
    private readonly ILogger<DuckDetectionController> _logger;

    public DuckDetectionController(
        IDuckDetectorFactory detectorFactory,
        ILogger<DuckDetectionController> logger)
    {
        _detectorFactory = detectorFactory;
        _logger = logger;
    }

    [HttpPost("analyze")]
    [Consumes("multipart/form-data")]
    [RequestSizeLimit(10_000_000)]
    public async Task<ActionResult<DuckDetectionResult>> Analyze(
        [FromForm] DuckDetectionApiRequest request,
        CancellationToken cancellationToken)
    {
        if (request.File is null || request.File.Length == 0)
        {
            return BadRequest(new DuckDetectionResult
            {
                Success = false,
                ErrorMessage = "A non-empty image file is required.",
                HasDuck = false,
                HorizontalPosition = DuckHorizontalPosition.None
            });
        }

        DuckDetectorKind detectorKind;
        try
        {
            detectorKind = ParseDetectorKind(request.DetectorKind);
        }
        catch (ArgumentException ex)
        {
            return BadRequest(new DuckDetectionResult
            {
                Success = false,
                ErrorMessage = ex.Message,
                HasDuck = false,
                HorizontalPosition = DuckHorizontalPosition.None
            });
        }

        string uploadsRoot = Path.Combine(
            Path.GetTempPath(),
            "KnightPi.DuckDetection",
            "uploads");

        string debugRoot = Path.Combine(
            Path.GetTempPath(),
            "KnightPi.DuckDetection",
            "debug");

        Directory.CreateDirectory(uploadsRoot);
        Directory.CreateDirectory(debugRoot);

        string extension = Path.GetExtension(request.File.FileName);
        if (string.IsNullOrWhiteSpace(extension))
        {
            extension = ".jpg";
        }

        string fileId = Guid.NewGuid().ToString("N");
        string imagePath = Path.Combine(uploadsRoot, $"{fileId}{extension}");

        string? debugImagePath = null;
        if (request.SaveDebugImage)
        {
            debugImagePath = Path.Combine(debugRoot, $"{fileId}.debug.jpg");
        }

        try
        {
            await using (FileStream stream = new(imagePath, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                await request.File.CopyToAsync(stream, cancellationToken);
            }

            IDuckDetector detector = _detectorFactory.Create(detectorKind);

            DuckDetectionRequest detectionRequest = new()
            {
                ImagePath = imagePath,
                DebugImageOutputPath = debugImagePath
            };

            DuckDetectionResult result = detector.Analyze(detectionRequest);

            return Ok(result);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Duck detection request failed.");

            return StatusCode(StatusCodes.Status500InternalServerError, new DuckDetectionResult
            {
                Success = false,
                ErrorMessage = $"Internal server error: {ex.Message}",
                HasDuck = false,
                HorizontalPosition = DuckHorizontalPosition.None
            });
        }
        finally
        {
            TryDeleteFile(imagePath);
        }
    }

    private static DuckDetectorKind ParseDetectorKind(string? rawValue)
    {
        if (string.IsNullOrWhiteSpace(rawValue))
        {
            return DuckDetectorKind.Cv;
        }

        return rawValue.Trim().ToLowerInvariant() switch
        {
            "cv" => DuckDetectorKind.Cv,
            "ml" => DuckDetectorKind.Ml,
            _ => throw new ArgumentException("detectorKind must be 'cv' or 'ml'.")
        };
    }

    private static void TryDeleteFile(string path)
    {
        try
        {
            if (System.IO.File.Exists(path))
            {
                System.IO.File.Delete(path);
            }
        }
        catch
        {
        }
    }
}