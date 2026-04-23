from abc import ABC, abstractmethod


class CameraClient(ABC):
    @abstractmethod
    def startup(self) -> None:
        pass

    @abstractmethod
    def shutdown(self) -> None:
        pass

    @abstractmethod
    def capture_jpeg(self) -> bytes:
        pass

    @property
    @abstractmethod
    def is_ready(self) -> bool:
        pass
