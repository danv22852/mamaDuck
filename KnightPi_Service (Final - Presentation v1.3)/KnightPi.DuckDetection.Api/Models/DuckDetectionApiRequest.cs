using Microsoft.AspNetCore.Http;

namespace KnightPi.DuckDetection.Api.Models;

public sealed class DuckDetectionApiRequest
{
    public IFormFile? File { get; init; }

    // "cv" or "ml"
    public string? DetectorKind { get; init; }

    public bool SaveDebugImage { get; init; }
}