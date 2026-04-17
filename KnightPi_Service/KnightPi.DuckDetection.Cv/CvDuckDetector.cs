using KnightPi.DuckDetection.Abstractions;
using OpenCvSharp;

namespace KnightPi.DuckDetection.Cv;

public sealed class CvDuckDetector : IDuckDetector
{
    private readonly CvDuckDetectorSettings _settings;

    public CvDuckDetector(CvDuckDetectorSettings settings)
    {
        _settings = settings ?? throw new ArgumentNullException(nameof(settings));
    }

    public DuckDetectionResult Analyze(DuckDetectionRequest request)
    {
        if (request is null)
        {
            throw new ArgumentNullException(nameof(request));
        }

        if (string.IsNullOrWhiteSpace(request.ImagePath))
        {
            return BuildError("ImagePath is required.");
        }

        if (!File.Exists(request.ImagePath))
        {
            return BuildError($"Image file does not exist: {request.ImagePath}");
        }

        try
        {
            using Mat source = Cv2.ImRead(request.ImagePath, ImreadModes.Color);
            if (source.Empty())
            {
                return BuildError($"Unable to read image: {request.ImagePath}");
            }

            int frameWidth = source.Width;
            int frameHeight = source.Height;

            using Mat hsv = new();
            Cv2.CvtColor(source, hsv, ColorConversionCodes.BGR2HSV);

            using Mat blueMask = CreateMask(
                hsv,
                _settings.BlueMinHue,
                _settings.BlueMaxHue,
                _settings.BlueMinSaturation,
                _settings.BlueMaxSaturation,
                _settings.BlueMinValue,
                _settings.BlueMaxValue,
                _settings.BlueOpenKernelSize,
                _settings.BlueCloseKernelSize);

            using Mat blueBarrierMask = DilateMask(blueMask, _settings.BlueDilateKernelSize);
            using Mat traversableMask = ComputeTraversableRegionMask(blueBarrierMask, frameWidth, frameHeight);

            BlueBoundaryInfo blueBoundaryInfo = ComputeBlueBoundaryInfo(
                blueMask,
                traversableMask,
                frameWidth,
                frameHeight);

            using Mat purpleMask = CreateMask(
                hsv,
                _settings.PurpleMinHue,
                _settings.PurpleMaxHue,
                _settings.PurpleMinSaturation,
                _settings.PurpleMaxSaturation,
                _settings.PurpleMinValue,
                _settings.PurpleMaxValue,
                _settings.PurpleOpenKernelSize,
                _settings.PurpleCloseKernelSize);

            DropZoneInfo dropZoneInfo = ComputeDropZoneInfo(
                purpleMask,
                frameWidth,
                frameHeight);

            using Mat yellowMask = CreateMask(
                hsv,
                _settings.MinHue,
                _settings.MaxHue,
                _settings.MinSaturation,
                _settings.MaxSaturation,
                _settings.MinValue,
                _settings.MaxValue,
                _settings.OpenKernelSize,
                _settings.CloseKernelSize);

            List<Candidate> candidates = FindDuckCandidates(
                yellowMask,
                traversableMask,
                blueBoundaryInfo,
                frameWidth,
                frameHeight);

            Candidate? selectedCandidate = SelectPrimaryCandidate(candidates);

            DuckDetectionResult result = BuildResult(
                selectedCandidate,
                candidates,
                blueBoundaryInfo,
                dropZoneInfo,
                frameWidth,
                frameHeight,
                request.DebugImageOutputPath);

            if (!string.IsNullOrWhiteSpace(request.DebugImageOutputPath))
            {
                SaveDebugImage(
                    source,
                    blueMask,
                    traversableMask,
                    purpleMask,
                    result,
                    request.DebugImageOutputPath!);
            }

            return result;
        }
        catch (Exception ex)
        {
            return BuildError($"CV detector failed: {ex.Message}");
        }
    }

    private Mat CreateMask(
        Mat hsv,
        int minHue,
        int maxHue,
        int minSaturation,
        int maxSaturation,
        int minValue,
        int maxValue,
        int openKernelSize,
        int closeKernelSize)
    {
        Mat raw = new();
        Cv2.InRange(
            hsv,
            new Scalar(minHue, minSaturation, minValue),
            new Scalar(maxHue, maxSaturation, maxValue),
            raw);

        Mat opened = new();
        Mat closed = new();

        using Mat openKernel = Cv2.GetStructuringElement(
            MorphShapes.Rect,
            new Size(openKernelSize, openKernelSize));

        using Mat closeKernel = Cv2.GetStructuringElement(
            MorphShapes.Rect,
            new Size(closeKernelSize, closeKernelSize));

        Cv2.MorphologyEx(raw, opened, MorphTypes.Open, openKernel);
        Cv2.MorphologyEx(opened, closed, MorphTypes.Close, closeKernel);

        raw.Dispose();
        opened.Dispose();

        return closed;
    }

    private static Mat DilateMask(Mat inputMask, int kernelSize)
    {
        Mat dilated = new();

        using Mat kernel = Cv2.GetStructuringElement(
            MorphShapes.Rect,
            new Size(kernelSize, kernelSize));

        Cv2.Dilate(inputMask, dilated, kernel);
        return dilated;
    }

    private Mat ComputeTraversableRegionMask(Mat blueBarrierMask, int frameWidth, int frameHeight)
    {
        Mat traversable = new(frameHeight, frameWidth, MatType.CV_8UC1, Scalar.Black);

        Point? seed = FindBottomCenterSeed(blueBarrierMask, frameWidth, frameHeight);
        if (seed is null)
        {
            return traversable;
        }

        FloodFillFreeSpace(blueBarrierMask, traversable, seed.Value);
        return traversable;
    }

    private Point? FindBottomCenterSeed(Mat blueBarrierMask, int frameWidth, int frameHeight)
    {
        int centerX = frameWidth / 2;
        int startY = frameHeight - 2;

        var barrierIdx = blueBarrierMask.GetGenericIndexer<byte>();

        for (int radius = 0; radius <= _settings.BottomCenterSeedSearchRadiusPixels; radius++)
        {
            for (int dx = -radius; dx <= radius; dx++)
            {
                int x = centerX + dx;
                if (x < 0 || x >= frameWidth)
                {
                    continue;
                }

                int y = startY;
                if (barrierIdx[y, x] == 0)
                {
                    return new Point(x, y);
                }
            }
        }

        return null;
    }

    private static void FloodFillFreeSpace(Mat blueBarrierMask, Mat traversableMask, Point seed)
    {
        int width = blueBarrierMask.Width;
        int height = blueBarrierMask.Height;

        var barrierIdx = blueBarrierMask.GetGenericIndexer<byte>();
        var traversableIdx = traversableMask.GetGenericIndexer<byte>();

        Queue<Point> queue = new();
        queue.Enqueue(seed);
        traversableIdx[seed.Y, seed.X] = 255;

        int[] dx = [1, -1, 0, 0];
        int[] dy = [0, 0, 1, -1];

        while (queue.Count > 0)
        {
            Point current = queue.Dequeue();

            for (int i = 0; i < 4; i++)
            {
                int nx = current.X + dx[i];
                int ny = current.Y + dy[i];

                if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                {
                    continue;
                }

                if (traversableIdx[ny, nx] != 0)
                {
                    continue;
                }

                if (barrierIdx[ny, nx] != 0)
                {
                    continue;
                }

                traversableIdx[ny, nx] = 255;
                queue.Enqueue(new Point(nx, ny));
            }
        }
    }

    private BlueBoundaryInfo ComputeBlueBoundaryInfo(
        Mat blueMask,
        Mat traversableMask,
        int frameWidth,
        int frameHeight)
    {
        bool blueDetected = TryGetMaskBoundingBox(blueMask, out Rect blueRect);
        bool traversableDetected = CountNonZero(traversableMask) > 0;

        int guideRowY = Math.Clamp((int)Math.Round(frameHeight * _settings.GuideRowPercent), 0, frameHeight - 1);

        bool bottomCenterInside = false;
        if (traversableDetected)
        {
            var traversableIdx = traversableMask.GetGenericIndexer<byte>();
            bottomCenterInside = traversableIdx[frameHeight - 2, frameWidth / 2] != 0;
        }

        GetLaneRunMetrics(
            traversableMask,
            frameWidth,
            guideRowY,
            out double laneCenterOffsetPercent,
            out double laneWidthPercent);

        GetNearestBoundaryMetrics(
            blueMask,
            frameWidth,
            frameHeight,
            out double nearestDistancePixels,
            out double nearestDistancePercentHeight,
            out double nearestHorizontalOffsetPercent);

        GetDominantBoundaryAngle(
            blueMask,
            out double dominantBoundaryAngleDeg);

        return new BlueBoundaryInfo
        {
            Detected = blueDetected,
            TraversableRegionDetected = traversableDetected,
            BottomCenterInsideTraversableRegion = bottomCenterInside,
            GuideRowY = guideRowY,
            GuideRowPercent = _settings.GuideRowPercent * 100.0,
            LaneCenterOffsetPercent = laneCenterOffsetPercent,
            LaneWidthPercent = laneWidthPercent,
            NearestBoundaryDistancePixels = nearestDistancePixels,
            NearestBoundaryDistancePercentOfFrameHeight = nearestDistancePercentHeight,
            NearestBoundaryHorizontalOffsetPercent = nearestHorizontalOffsetPercent,
            DominantBoundaryAngleDeg = dominantBoundaryAngleDeg,
            BoundaryApproachErrorDeg = Math.Abs(dominantBoundaryAngleDeg),
            BoundingBox = blueDetected
                ? new BoundingBox
                {
                    X = blueRect.X,
                    Y = blueRect.Y,
                    Width = blueRect.Width,
                    Height = blueRect.Height
                }
                : new BoundingBox()
        };
    }

    private void GetLaneRunMetrics(
        Mat traversableMask,
        int frameWidth,
        int guideRowY,
        out double laneCenterOffsetPercent,
        out double laneWidthPercent)
    {
        laneCenterOffsetPercent = 0.0;
        laneWidthPercent = 0.0;

        var idx = traversableMask.GetGenericIndexer<byte>();
        int frameCenterX = frameWidth / 2;

        int bestRunStart = -1;
        int bestRunEnd = -1;
        int currentStart = -1;

        for (int x = 0; x < frameWidth; x++)
        {
            bool inside = idx[guideRowY, x] != 0;

            if (inside && currentStart < 0)
            {
                currentStart = x;
            }

            bool runEnded = (!inside || x == frameWidth - 1) && currentStart >= 0;
            if (!runEnded)
            {
                continue;
            }

            int endX = inside && x == frameWidth - 1 ? x : x - 1;

            bool currentContainsFrameCenter = currentStart <= frameCenterX && endX >= frameCenterX;
            bool bestContainsFrameCenter = bestRunStart >= 0 && bestRunStart <= frameCenterX && bestRunEnd >= frameCenterX;

            if (bestRunStart < 0 ||
                (currentContainsFrameCenter && !bestContainsFrameCenter) ||
                ((currentContainsFrameCenter == bestContainsFrameCenter) &&
                 ((endX - currentStart) > (bestRunEnd - bestRunStart))))
            {
                bestRunStart = currentStart;
                bestRunEnd = endX;
            }

            currentStart = -1;
        }

        if (bestRunStart < 0)
        {
            return;
        }

        double laneCenterX = (bestRunStart + bestRunEnd) / 2.0;
        double offsetPixels = laneCenterX - (frameWidth / 2.0);
        laneCenterOffsetPercent = offsetPixels / (frameWidth / 2.0) * 100.0;

        double laneWidthPixels = (bestRunEnd - bestRunStart) + 1;
        laneWidthPercent = laneWidthPixels / frameWidth * 100.0;
    }

    private static void GetNearestBoundaryMetrics(
        Mat blueMask,
        int frameWidth,
        int frameHeight,
        out double nearestDistancePixels,
        out double nearestDistancePercentHeight,
        out double nearestHorizontalOffsetPercent)
    {
        nearestDistancePixels = 0.0;
        nearestDistancePercentHeight = 0.0;
        nearestHorizontalOffsetPercent = 0.0;

        var idx = blueMask.GetGenericIndexer<byte>();

        Point reference = new(frameWidth / 2, frameHeight - 2);

        bool found = false;
        double bestDistanceSquared = double.MaxValue;
        Point bestPoint = new();

        for (int y = 0; y < frameHeight; y++)
        {
            for (int x = 0; x < frameWidth; x++)
            {
                if (idx[y, x] == 0)
                {
                    continue;
                }

                double dx = x - reference.X;
                double dy = y - reference.Y;
                double distanceSquared = (dx * dx) + (dy * dy);

                if (distanceSquared < bestDistanceSquared)
                {
                    bestDistanceSquared = distanceSquared;
                    bestPoint = new Point(x, y);
                    found = true;
                }
            }
        }

        if (!found)
        {
            return;
        }

        nearestDistancePixels = Math.Sqrt(bestDistanceSquared);
        nearestDistancePercentHeight = nearestDistancePixels / frameHeight * 100.0;

        double horizontalOffsetPixels = bestPoint.X - (frameWidth / 2.0);
        nearestHorizontalOffsetPercent = horizontalOffsetPixels / (frameWidth / 2.0) * 100.0;
    }

    private void GetDominantBoundaryAngle(Mat blueMask, out double dominantBoundaryAngleDeg)
    {
        dominantBoundaryAngleDeg = 0.0;

        LineSegmentPoint[] lines = Cv2.HoughLinesP(
            blueMask,
            1,
            Math.PI / 180.0,
            _settings.HoughThreshold,
            _settings.HoughMinLineLength,
            _settings.HoughMaxLineGap);

        if (lines.Length == 0)
        {
            return;
        }

        double bestLength = -1.0;
        LineSegmentPoint bestLine = lines[0];

        foreach (LineSegmentPoint line in lines)
        {
            double dx = line.P2.X - line.P1.X;
            double dy = line.P2.Y - line.P1.Y;
            double length = Math.Sqrt((dx * dx) + (dy * dy));

            if (length > bestLength)
            {
                bestLength = length;
                bestLine = line;
            }
        }

        dominantBoundaryAngleDeg = NormalizeLineAngleDeg(
            Math.Atan2(bestLine.P2.Y - bestLine.P1.Y, bestLine.P2.X - bestLine.P1.X) * 180.0 / Math.PI);
    }

    private DropZoneInfo ComputeDropZoneInfo(Mat purpleMask, int frameWidth, int frameHeight)
    {
        Cv2.FindContours(
            purpleMask,
            out Point[][] contours,
            out _,
            RetrievalModes.External,
            ContourApproximationModes.ApproxSimple);

        Point[]? bestContour = null;
        double bestArea = 0.0;

        foreach (Point[] contour in contours)
        {
            double area = Cv2.ContourArea(contour);
            if (area < _settings.MinDropZoneAreaPixels)
            {
                continue;
            }

            if (area > bestArea)
            {
                bestArea = area;
                bestContour = contour;
            }
        }

        if (bestContour is null)
        {
            return new DropZoneInfo
            {
                Detected = false
            };
        }

        Rect rect = Cv2.BoundingRect(bestContour);
        RotatedRect rotatedRect = Cv2.MinAreaRect(bestContour);

        double centerOffsetPixels = rotatedRect.Center.X - (frameWidth / 2.0);
        double centerOffsetPercent = centerOffsetPixels / (frameWidth / 2.0) * 100.0;

        double bottomOffsetPixels = frameHeight - rotatedRect.Center.Y;
        double bottomOffsetPercent = bottomOffsetPixels / frameHeight * 100.0;

        double widthPercent = rect.Width / (double)frameWidth * 100.0;
        double heightPercent = rect.Height / (double)frameHeight * 100.0;
        double areaPercent = bestArea / (frameWidth * frameHeight) * 100.0;

        double longEdgeAngleDeg = GetLongEdgeAngleDeg(rotatedRect);
        double alignmentErrorDeg = Math.Abs(longEdgeAngleDeg);

        return new DropZoneInfo
        {
            Detected = true,
            BoundingBox = new BoundingBox
            {
                X = rect.X,
                Y = rect.Y,
                Width = rect.Width,
                Height = rect.Height
            },
            CenterPoint = CreateImagePoint(
                (int)Math.Round(rotatedRect.Center.X),
                (int)Math.Round(rotatedRect.Center.Y),
                frameWidth,
                frameHeight),
            HorizontalOffsetPercent = centerOffsetPercent,
            BottomOffsetPercentFromBottom = bottomOffsetPercent,
            WidthPercentOfFrame = widthPercent,
            HeightPercentOfFrame = heightPercent,
            AreaPercentOfFrame = areaPercent,
            LongEdgeAngleDeg = longEdgeAngleDeg,
            AlignmentErrorDeg = alignmentErrorDeg,
            IsHorizontallyAligned = alignmentErrorDeg <= _settings.DropZoneAlignmentToleranceDeg
        };
    }

    private List<Candidate> FindDuckCandidates(
        Mat yellowMask,
        Mat traversableMask,
        BlueBoundaryInfo blueBoundaryInfo,
        int frameWidth,
        int frameHeight)
    {
        List<Candidate> candidates = [];

        Cv2.FindContours(
            yellowMask,
            out Point[][] contours,
            out _,
            RetrievalModes.External,
            ContourApproximationModes.ApproxSimple);

        double frameArea = frameWidth * frameHeight;
        bool boundaryActive = blueBoundaryInfo.Detected && blueBoundaryInfo.TraversableRegionDetected;

        foreach (Point[] contour in contours)
        {
            if (contour.Length < 3)
            {
                continue;
            }

            double contourArea = Cv2.ContourArea(contour);
            if (contourArea <= 0.0)
            {
                continue;
            }

            Rect rect = Cv2.BoundingRect(contour);
            if (rect.Width <= 0 || rect.Height <= 0)
            {
                continue;
            }

            double boxArea = rect.Width * rect.Height;
            double areaPercent = contourArea / frameArea * 100.0;
            double aspectRatio = rect.Width / (double)rect.Height;
            double extent = contourArea / boxArea;

            if (areaPercent < _settings.MinAreaPercentOfFrame ||
                areaPercent > _settings.MaxAreaPercentOfFrame)
            {
                continue;
            }

            if (aspectRatio < _settings.MinAspectRatio ||
                aspectRatio > _settings.MaxAspectRatio)
            {
                continue;
            }

            if (extent < _settings.MinExtent ||
                extent > _settings.MaxExtent)
            {
                continue;
            }

            int bottomCenterX = rect.X + (rect.Width / 2);
            int bottomCenterY = rect.Y + rect.Height - 1;

            bool isInsideBoundary = !boundaryActive || IsPointInsideMask(traversableMask, bottomCenterX, bottomCenterY);

            double horizontalOffsetPixels = bottomCenterX - (frameWidth / 2.0);
            double horizontalOffsetPercent = horizontalOffsetPixels / (frameWidth / 2.0) * 100.0;

            double bottomOffsetPixels = frameHeight - (rect.Y + rect.Height);
            double bottomOffsetPercent = bottomOffsetPixels / frameHeight * 100.0;

            double bottomnessScore = 1.0 - Math.Clamp(bottomOffsetPercent / 100.0, 0.0, 1.0);
            double areaScore = Math.Min(1.0, areaPercent / 20.0);
            double extentScore = Math.Min(1.0, extent);

            double score =
                (_settings.AreaWeight * areaScore) +
                (_settings.ExtentWeight * extentScore) +
                (_settings.BottomWeight * bottomnessScore);

            candidates.Add(new Candidate
            {
                BoundingRect = rect,
                MatchScore = score,
                IsInsideBoundary = isInsideBoundary,
                HorizontalOffsetPercent = horizontalOffsetPercent,
                BottomOffsetPercentFromBottom = bottomOffsetPercent,
                WidthPercentOfFrame = rect.Width / (double)frameWidth * 100.0,
                HeightPercentOfFrame = rect.Height / (double)frameHeight * 100.0,
                AreaPercentOfFrame = boxArea / frameArea * 100.0,
                BottomCenterPoint = CreateImagePoint(bottomCenterX, bottomCenterY, frameWidth, frameHeight)
            });
        }

        return candidates;
    }

    private static Candidate? SelectPrimaryCandidate(List<Candidate> candidates)
    {
        Candidate? best = null;

        foreach (Candidate candidate in candidates)
        {
            if (!candidate.IsInsideBoundary)
            {
                continue;
            }

            if (best is null || candidate.MatchScore > best.MatchScore)
            {
                best = candidate;
            }
        }

        return best;
    }

    private DuckDetectionResult BuildResult(
        Candidate? selectedCandidate,
        List<Candidate> candidates,
        BlueBoundaryInfo blueBoundaryInfo,
        DropZoneInfo dropZoneInfo,
        int frameWidth,
        int frameHeight,
        string? debugImageOutputPath)
    {
        List<DuckCandidateResult> candidateResults = [];
        int eligibleCount = 0;

        foreach (Candidate candidate in candidates)
        {
            if (candidate.IsInsideBoundary)
            {
                eligibleCount++;
            }

            candidateResults.Add(new DuckCandidateResult
            {
                BoundingBox = new BoundingBox
                {
                    X = candidate.BoundingRect.X,
                    Y = candidate.BoundingRect.Y,
                    Width = candidate.BoundingRect.Width,
                    Height = candidate.BoundingRect.Height
                },
                BottomCenterPoint = candidate.BottomCenterPoint,
                IsInsideBoundary = candidate.IsInsideBoundary,
                HorizontalOffsetPercent = candidate.HorizontalOffsetPercent,
                BottomOffsetPercentFromBottom = candidate.BottomOffsetPercentFromBottom,
                WidthPercentOfFrame = candidate.WidthPercentOfFrame,
                HeightPercentOfFrame = candidate.HeightPercentOfFrame,
                AreaPercentOfFrame = candidate.AreaPercentOfFrame,
                MatchScore = candidate.MatchScore
            });
        }

        if (selectedCandidate is null)
        {
            return new DuckDetectionResult
            {
                Success = true,
                ErrorMessage = null,
                HasDuck = false,
                HorizontalPosition = DuckHorizontalPosition.None,
                HorizontalOffsetNormalized = 0.0,
                HorizontalOffsetPercent = 0.0,
                WidthPercentOfFrame = 0.0,
                HeightPercentOfFrame = 0.0,
                AreaPercentOfFrame = 0.0,
                MatchScore = 0.0,
                BottomOffsetPercentFromBottom = 0.0,
                DuckBottomCenterPoint = new ImagePoint(),
                TotalDuckCandidates = candidates.Count,
                EligibleDuckCandidates = eligibleCount,
                RejectedDuckCandidatesOutsideBoundary = candidates.Count - eligibleCount,
                DuckCandidates = candidateResults,
                BlueBoundary = blueBoundaryInfo,
                DropZone = dropZoneInfo,
                FrameSize = new ImageSize
                {
                    Width = frameWidth,
                    Height = frameHeight
                },
                DuckBoundingBox = new BoundingBox(),
                DebugImageOutputPath = debugImageOutputPath
            };
        }

        DuckHorizontalPosition position;
        if (Math.Abs(selectedCandidate.HorizontalOffsetPercent) <= _settings.CenterTolerancePercent)
        {
            position = DuckHorizontalPosition.Center;
        }
        else if (selectedCandidate.HorizontalOffsetPercent < 0)
        {
            position = DuckHorizontalPosition.Left;
        }
        else
        {
            position = DuckHorizontalPosition.Right;
        }

        return new DuckDetectionResult
        {
            Success = true,
            ErrorMessage = null,
            HasDuck = true,
            HorizontalPosition = position,
            HorizontalOffsetNormalized = selectedCandidate.HorizontalOffsetPercent / 100.0,
            HorizontalOffsetPercent = selectedCandidate.HorizontalOffsetPercent,
            WidthPercentOfFrame = selectedCandidate.WidthPercentOfFrame,
            HeightPercentOfFrame = selectedCandidate.HeightPercentOfFrame,
            AreaPercentOfFrame = selectedCandidate.AreaPercentOfFrame,
            MatchScore = selectedCandidate.MatchScore,
            BottomOffsetPercentFromBottom = selectedCandidate.BottomOffsetPercentFromBottom,
            DuckBottomCenterPoint = selectedCandidate.BottomCenterPoint,
            TotalDuckCandidates = candidates.Count,
            EligibleDuckCandidates = eligibleCount,
            RejectedDuckCandidatesOutsideBoundary = candidates.Count - eligibleCount,
            DuckCandidates = candidateResults,
            BlueBoundary = blueBoundaryInfo,
            DropZone = dropZoneInfo,
            FrameSize = new ImageSize
            {
                Width = frameWidth,
                Height = frameHeight
            },
            DuckBoundingBox = new BoundingBox
            {
                X = selectedCandidate.BoundingRect.X,
                Y = selectedCandidate.BoundingRect.Y,
                Width = selectedCandidate.BoundingRect.Width,
                Height = selectedCandidate.BoundingRect.Height
            },
            DebugImageOutputPath = debugImageOutputPath
        };
    }

    private static bool IsPointInsideMask(Mat mask, int x, int y)
    {
        if (x < 0 || x >= mask.Width || y < 0 || y >= mask.Height)
        {
            return false;
        }

        var idx = mask.GetGenericIndexer<byte>();
        return idx[y, x] != 0;
    }

    private static int CountNonZero(Mat mask)
    {
        return Cv2.CountNonZero(mask);
    }

    private static bool TryGetMaskBoundingBox(Mat mask, out Rect rect)
    {
        var idx = mask.GetGenericIndexer<byte>();

        int minX = mask.Width;
        int minY = mask.Height;
        int maxX = -1;
        int maxY = -1;

        for (int y = 0; y < mask.Height; y++)
        {
            for (int x = 0; x < mask.Width; x++)
            {
                if (idx[y, x] == 0)
                {
                    continue;
                }

                if (x < minX) minX = x;
                if (y < minY) minY = y;
                if (x > maxX) maxX = x;
                if (y > maxY) maxY = y;
            }
        }

        if (maxX < 0 || maxY < 0)
        {
            rect = new Rect();
            return false;
        }

        rect = new Rect(minX, minY, (maxX - minX) + 1, (maxY - minY) + 1);
        return true;
    }

    private static ImagePoint CreateImagePoint(int x, int y, int frameWidth, int frameHeight)
    {
        return new ImagePoint
        {
            X = x,
            Y = y,
            XPercentOfFrame = x / (double)frameWidth * 100.0,
            YPercentOfFrame = y / (double)frameHeight * 100.0
        };
    }

    private static double NormalizeLineAngleDeg(double angleDeg)
    {
        while (angleDeg > 90.0)
        {
            angleDeg -= 180.0;
        }

        while (angleDeg < -90.0)
        {
            angleDeg += 180.0;
        }

        return angleDeg;
    }

    private static double GetLongEdgeAngleDeg(RotatedRect rotatedRect)
    {
        Point2f[] points = rotatedRect.Points();

        double bestLength = -1.0;
        double bestAngle = 0.0;

        for (int i = 0; i < 4; i++)
        {
            Point2f p1 = points[i];
            Point2f p2 = points[(i + 1) % 4];

            double dx = p2.X - p1.X;
            double dy = p2.Y - p1.Y;
            double length = Math.Sqrt((dx * dx) + (dy * dy));

            if (length > bestLength)
            {
                bestLength = length;
                bestAngle = Math.Atan2(dy, dx) * 180.0 / Math.PI;
            }
        }

        return NormalizeLineAngleDeg(bestAngle);
    }

    private static DuckDetectionResult BuildError(string message)
    {
        return new DuckDetectionResult
        {
            Success = false,
            ErrorMessage = message,
            HasDuck = false,
            HorizontalPosition = DuckHorizontalPosition.None,
            HorizontalOffsetNormalized = 0.0,
            HorizontalOffsetPercent = 0.0,
            WidthPercentOfFrame = 0.0,
            HeightPercentOfFrame = 0.0,
            AreaPercentOfFrame = 0.0,
            MatchScore = 0.0,
            BottomOffsetPercentFromBottom = 0.0,
            DuckBottomCenterPoint = new ImagePoint(),
            TotalDuckCandidates = 0,
            EligibleDuckCandidates = 0,
            RejectedDuckCandidatesOutsideBoundary = 0,
            DuckCandidates = Array.Empty<DuckCandidateResult>(),
            BlueBoundary = new BlueBoundaryInfo(),
            DropZone = new DropZoneInfo(),
            FrameSize = new ImageSize(),
            DuckBoundingBox = new BoundingBox()
        };
    }

    private void SaveDebugImage(
        Mat source,
        Mat blueMask,
        Mat traversableMask,
        Mat purpleMask,
        DuckDetectionResult result,
        string outputPath)
    {
        string? directory = Path.GetDirectoryName(outputPath);
        if (!string.IsNullOrWhiteSpace(directory))
        {
            Directory.CreateDirectory(directory);
        }

        using Mat debug = source.Clone();

        int frameWidth = debug.Width;
        int frameHeight = debug.Height;
        int imageCenterX = frameWidth / 2;
        int imageCenterY = frameHeight / 2;

        Scalar blue = new(255, 0, 0);
        Scalar green = new(0, 255, 0);
        Scalar red = new(0, 0, 255);
        Scalar yellow = new(0, 255, 255);
        Scalar purple = new(255, 0, 255);
        Scalar cyan = new(255, 255, 0);
        Scalar white = new(255, 255, 255);

        Cv2.Line(
            debug,
            new Point(imageCenterX, 0),
            new Point(imageCenterX, frameHeight - 1),
            blue,
            2);

        Cv2.Line(
            debug,
            new Point(0, imageCenterY),
            new Point(frameWidth - 1, imageCenterY),
            blue,
            2);

        Cv2.PutText(
            debug,
            $"IMG CX={imageCenterX}",
            new Point(imageCenterX + 8, 25),
            HersheyFonts.HersheySimplex,
            0.5,
            blue,
            1);

        Cv2.PutText(
            debug,
            $"IMG CY={imageCenterY}",
            new Point(10, imageCenterY - 8),
            HersheyFonts.HersheySimplex,
            0.5,
            blue,
            1);

        int guideRowY = result.BlueBoundary.GuideRowY;
        if (guideRowY >= 0 && guideRowY < frameHeight)
        {
            Cv2.Line(
                debug,
                new Point(0, guideRowY),
                new Point(frameWidth - 1, guideRowY),
                cyan,
                1);

            Cv2.PutText(
                debug,
                $"GuideRow y={guideRowY} laneOffset={result.BlueBoundary.LaneCenterOffsetPercent:F1}% laneWidth={result.BlueBoundary.LaneWidthPercent:F1}%",
                new Point(10, Math.Max(guideRowY - 8, 15)),
                HersheyFonts.HersheySimplex,
                0.5,
                cyan,
                1);
        }

        DrawDetectedBlueLines(debug, blueMask, blue);

        if (result.BlueBoundary.Detected)
        {
            string boundaryLabel =
                $"BlueBoundary angle={result.BlueBoundary.DominantBoundaryAngleDeg:F1} " +
                $"approachErr={result.BlueBoundary.BoundaryApproachErrorDeg:F1} " +
                $"nearest={result.BlueBoundary.NearestBoundaryDistancePercentOfFrameHeight:F1}%";

            Cv2.PutText(
                debug,
                boundaryLabel,
                new Point(10, 20),
                HersheyFonts.HersheySimplex,
                0.55,
                blue,
                2);
        }

        foreach (DuckCandidateResult candidate in result.DuckCandidates)
        {
            Rect rect = new(
                candidate.BoundingBox.X,
                candidate.BoundingBox.Y,
                candidate.BoundingBox.Width,
                candidate.BoundingBox.Height);

            bool isSelected =
                result.HasDuck &&
                rect.X == result.DuckBoundingBox.X &&
                rect.Y == result.DuckBoundingBox.Y &&
                rect.Width == result.DuckBoundingBox.Width &&
                rect.Height == result.DuckBoundingBox.Height;

            if (isSelected)
            {
                DrawSelectedDuckDebug(debug, candidate, frameWidth, frameHeight, green, white);
            }
            else
            {
                DrawRejectedDuckDebug(debug, candidate, red, white);
            }
        }

        if (result.BlueBoundary.TraversableRegionDetected)
        {
            string traversableText =
                $"Traversable bottom-center-inside={result.BlueBoundary.BottomCenterInsideTraversableRegion}";
            Cv2.PutText(
                debug,
                traversableText,
                new Point(10, 45),
                HersheyFonts.HersheySimplex,
                0.55,
                cyan,
                2);
        }

        if (result.DropZone.Detected)
        {
            Rect rect = new(
                result.DropZone.BoundingBox.X,
                result.DropZone.BoundingBox.Y,
                result.DropZone.BoundingBox.Width,
                result.DropZone.BoundingBox.Height);

            Cv2.Rectangle(debug, rect, purple, 2);

            Cv2.Circle(
                debug,
                new Point(result.DropZone.CenterPoint.X, result.DropZone.CenterPoint.Y),
                6,
                purple,
                -1);

            Cv2.Line(
                debug,
                new Point(imageCenterX, imageCenterY),
                new Point(result.DropZone.CenterPoint.X, result.DropZone.CenterPoint.Y),
                purple,
                1);

            string dropLabel =
                $"DROP ZONE x={result.DropZone.HorizontalOffsetPercent:F1}% " +
                $"bottom={result.DropZone.BottomOffsetPercentFromBottom:F1}% " +
                $"angle={result.DropZone.LongEdgeAngleDeg:F1} " +
                $"alignErr={result.DropZone.AlignmentErrorDeg:F1}";

            Cv2.PutText(
                debug,
                dropLabel,
                new Point(10, 70),
                HersheyFonts.HersheySimplex,
                0.55,
                purple,
                2);
        }

        Cv2.ImWrite(outputPath, debug);

        string baseDirectory = Path.GetDirectoryName(outputPath) ?? "";
        string fileNameWithoutExtension = Path.GetFileNameWithoutExtension(outputPath);
        string extension = Path.GetExtension(outputPath);

        Cv2.ImWrite(Path.Combine(baseDirectory, $"{fileNameWithoutExtension}.blue-mask{extension}"), blueMask);
        Cv2.ImWrite(Path.Combine(baseDirectory, $"{fileNameWithoutExtension}.traversable-mask{extension}"), traversableMask);
        Cv2.ImWrite(Path.Combine(baseDirectory, $"{fileNameWithoutExtension}.purple-mask{extension}"), purpleMask);
    }

    private static void DrawSelectedDuckDebug(
        Mat debug,
        DuckCandidateResult candidate,
        int frameWidth,
        int frameHeight,
        Scalar green,
        Scalar textColor)
    {
        Rect rect = new(
            candidate.BoundingBox.X,
            candidate.BoundingBox.Y,
            candidate.BoundingBox.Width,
            candidate.BoundingBox.Height);

        int centerX = rect.X + (rect.Width / 2);
        int centerY = rect.Y + (rect.Height / 2);
        int bottomY = rect.Y + rect.Height - 1;

        Cv2.Rectangle(debug, rect, green, 3);

        Cv2.Line(
            debug,
            new Point(centerX, rect.Y),
            new Point(centerX, rect.Y + rect.Height),
            green,
            2);

        Cv2.Line(
            debug,
            new Point(rect.X, centerY),
            new Point(rect.X + rect.Width, centerY),
            green,
            2);

        DrawDottedHorizontalLine(
            debug,
            rect.X,
            rect.X + rect.Width,
            bottomY,
            green,
            2,
            8,
            6);

        Cv2.Circle(debug, new Point(centerX, centerY), 5, green, -1);
        Cv2.Circle(debug, new Point(candidate.BottomCenterPoint.X, candidate.BottomCenterPoint.Y), 5, green, -1);

        int imageCenterX = frameWidth / 2;
        int imageCenterY = frameHeight / 2;
        Cv2.Line(
            debug,
            new Point(imageCenterX, imageCenterY),
            new Point(centerX, centerY),
            green,
            1);

        Cv2.Line(
            debug,
            new Point(imageCenterX, frameHeight - 2),
            new Point(candidate.BottomCenterPoint.X, candidate.BottomCenterPoint.Y),
            green,
            1);

        string title = "Duck TO PICKUP";
        string geom1 =
            $"xOff={candidate.HorizontalOffsetPercent:F1}% " +
            $"bottomOff={candidate.BottomOffsetPercentFromBottom:F1}%";
        string geom2 =
            $"w={candidate.WidthPercentOfFrame:F1}% " +
            $"h={candidate.HeightPercentOfFrame:F1}% " +
            $"a={candidate.AreaPercentOfFrame:F1}%";

        int textX = Math.Max(rect.X, 10);
        int textY = Math.Max(rect.Y - 28, 20);

        Cv2.PutText(
            debug,
            title,
            new Point(textX, textY),
            HersheyFonts.HersheySimplex,
            0.65,
            green,
            2);

        Cv2.PutText(
            debug,
            geom1,
            new Point(textX, textY + 18),
            HersheyFonts.HersheySimplex,
            0.5,
            green,
            1);

        Cv2.PutText(
            debug,
            geom2,
            new Point(textX, textY + 36),
            HersheyFonts.HersheySimplex,
            0.5,
            green,
            1);

        Cv2.PutText(
            debug,
            $"cx={centerX}, cy={centerY}",
            new Point(centerX + 8, centerY - 8),
            HersheyFonts.HersheySimplex,
            0.45,
            textColor,
            1);

        Cv2.PutText(
            debug,
            $"bottomY={bottomY}",
            new Point(candidate.BottomCenterPoint.X + 8, candidate.BottomCenterPoint.Y - 8),
            HersheyFonts.HersheySimplex,
            0.45,
            textColor,
            1);
    }

    private static void DrawRejectedDuckDebug(
        Mat debug,
        DuckCandidateResult candidate,
        Scalar red,
        Scalar textColor)
    {
        Rect rect = new(
            candidate.BoundingBox.X,
            candidate.BoundingBox.Y,
            candidate.BoundingBox.Width,
            candidate.BoundingBox.Height);

        Cv2.Rectangle(debug, rect, red, 2);

        Cv2.Circle(
            debug,
            new Point(candidate.BottomCenterPoint.X, candidate.BottomCenterPoint.Y),
            4,
            red,
            -1);

        int textX = Math.Max(rect.X, 10);
        int textY = Math.Max(rect.Y - 10, 20);

        Cv2.PutText(
            debug,
            "Duck OUTSIDE Box",
            new Point(textX, textY),
            HersheyFonts.HersheySimplex,
            0.55,
            red,
            2);

        Cv2.PutText(
            debug,
            $"xOff={candidate.HorizontalOffsetPercent:F1}% bottomOff={candidate.BottomOffsetPercentFromBottom:F1}%",
            new Point(textX, textY + 18),
            HersheyFonts.HersheySimplex,
            0.45,
            red,
            1);
    }

    private static void DrawDottedHorizontalLine(
        Mat image,
        int x1,
        int x2,
        int y,
        Scalar color,
        int thickness,
        int dashLength,
        int gapLength)
    {
        int x = x1;

        while (x < x2)
        {
            int dashEnd = Math.Min(x + dashLength, x2);
            Cv2.Line(
                image,
                new Point(x, y),
                new Point(dashEnd, y),
                color,
                thickness);

            x += dashLength + gapLength;
        }
    }

    private void DrawDetectedBlueLines(Mat debug, Mat blueMask, Scalar blue)
    {
        LineSegmentPoint[] lines = Cv2.HoughLinesP(
            blueMask,
            1,
            Math.PI / 180.0,
            _settings.HoughThreshold,
            _settings.HoughMinLineLength,
            _settings.HoughMaxLineGap);

        foreach (LineSegmentPoint line in lines)
        {
            Cv2.Line(debug, line.P1, line.P2, blue, 4);

            double dx = line.P2.X - line.P1.X;
            double dy = line.P2.Y - line.P1.Y;
            double length = Math.Sqrt((dx * dx) + (dy * dy));
            double angle = NormalizeLineAngleDeg(Math.Atan2(dy, dx) * 180.0 / Math.PI);

            int midX = (line.P1.X + line.P2.X) / 2;
            int midY = (line.P1.Y + line.P2.Y) / 2;

            Cv2.PutText(
                debug,
                $"B angle={angle:F1} len={length:F0}",
                new Point(midX + 6, midY - 6),
                HersheyFonts.HersheySimplex,
                0.45,
                blue,
                2);
        }
    }

    private sealed class Candidate
    {
        public required Rect BoundingRect { get; init; }
        public required ImagePoint BottomCenterPoint { get; init; }

        public bool IsInsideBoundary { get; init; }

        public double HorizontalOffsetPercent { get; init; }
        public double BottomOffsetPercentFromBottom { get; init; }

        public double WidthPercentOfFrame { get; init; }
        public double HeightPercentOfFrame { get; init; }
        public double AreaPercentOfFrame { get; init; }

        public double MatchScore { get; init; }
    }
}