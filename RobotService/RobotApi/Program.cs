using Microsoft.AspNetCore.Mvc;

var builder = WebApplication.CreateBuilder(args);

// Add services
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

var app = builder.Build();

// Standard Swagger Middleware
app.UseSwagger();
app.UseSwaggerUI();

app.MapGet("/", () => "The Robot Service is Alive!");

// .NET 10 Minimal API File Upload
app.MapPost("/upload", async ([FromForm] IFormFile file) => 
{
    if (file is not { Length: > 0 }) 
        return Results.BadRequest("No file sent or file is empty.");

    // Define and create the upload directory
    var uploadPath = Path.Combine(Directory.GetCurrentDirectory(), "uploads");
    if (!Directory.Exists(uploadPath))
    {
        Directory.CreateDirectory(uploadPath);
    }

    // Generate a safe path
    var filePath = Path.Combine(uploadPath, file.FileName);

    // Save the file
    using var stream = new FileStream(filePath, FileMode.Create);
    await file.CopyToAsync(stream);

    return Results.Ok(new { 
        message = "Success!", 
        savedAt = filePath,
        size = file.Length 
    });
})
.DisableAntiforgery() // REQUIRED in .NET 10 for external devices like Raspberry Pi
.WithOpenApi();

app.Run("http://0.0.0.0:5043");