import httpx

from mission_commander.domain.cv_models import CvAnalysisResult


class CvServiceClient:
    def __init__(self, base_url: str, timeout_seconds: float = 10.0) -> None:
        self._base_url = base_url.rstrip("/")
        self._timeout = httpx.Timeout(timeout_seconds)
        self._client: httpx.AsyncClient | None = None

    async def startup(self) -> None:
        if self._client is None:
            self._client = httpx.AsyncClient(
                base_url=self._base_url,
                timeout=self._timeout
            )

    async def shutdown(self) -> None:
        if self._client is not None:
            await self._client.aclose()
            self._client = None

    @property
    def is_ready(self) -> bool:
        return self._client is not None

    async def analyze_image(
        self,
        jpeg_bytes: bytes,
        detector_kind: str = "cv",
        save_debug_image: bool = True
    ) -> CvAnalysisResult:
        if self._client is None:
            raise RuntimeError("CV service client is not started")

        files = {
            "File": ("capture.jpg", jpeg_bytes, "image/jpeg")
        }

        data = {
            "DetectorKind": detector_kind,
            "SaveDebugImage": str(save_debug_image).lower()
        }

        response = await self._client.post(
            "/api/DuckDetection/analyze",
            files=files,
            data=data
        )
        response.raise_for_status()

        payload = response.json()
        return CvAnalysisResult.model_validate(payload)

    def analyze_image_sync(
        self,
        jpeg_bytes: bytes,
        detector_kind: str = "cv",
        save_debug_image: bool = True
    ) -> CvAnalysisResult:
        files = {
            "File": ("capture.jpg", jpeg_bytes, "image/jpeg")
        }

        data = {
            "DetectorKind": detector_kind,
            "SaveDebugImage": str(save_debug_image).lower()
        }

        with httpx.Client(
            base_url=self._base_url,
            timeout=self._timeout
        ) as client:
            response = client.post(
                "/api/DuckDetection/analyze",
                files=files,
                data=data
            )
            response.raise_for_status()

        payload = response.json()
        return CvAnalysisResult.model_validate(payload)
