import asyncio
import websockets


async def main():
    uri = "ws://localhost:8080/ws/telemetry"

    async with websockets.connect(uri) as websocket:
        while True:
            message = await websocket.recv()
            print(message)


if __name__ == "__main__":
    asyncio.run(main())
