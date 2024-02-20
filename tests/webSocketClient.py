import time
import asyncio
try:
    import websockets
except ModuleNotFoundError:
    print("⚠️  Warning: Test not executed.")
    print("      Python websockets module not installed and it is needed in this test.")
    exit(0)

async def checkEcho(conn, value):
    # print('Sending ', value)
    await conn.send(value)
    time.sleep(0.1)
    result = await conn.recv()
    # print('Received', result)
    if result != value:
        print('Error incorrect response:', result)
        exit(1)

async def main():
    uri = "ws://localhost:8080"
    async with websockets.connect(uri) as websocket:
        if "Hello from echo websocket" !=  (await websocket.recv()):
            print('Error incorrect hello message')
            exit(1)
        await checkEcho(websocket, 'Hello!')
        await checkEcho(websocket, 'This is other text with no ASCII chars ññññáéÇ')
        await websocket.send('close')
        time.sleep(0.1)
        result = (await websocket.recv())
        if "disconnecting" !=  result:
            print('Error incorrect disconnected message:', result)
            exit(1)

    async with websockets.connect(uri) as websocket:
        if "Hello from echo websocket" !=  (await websocket.recv()):
            print('Error incorrect hello message')
            exit(1)
        await checkEcho(websocket, 'Second try!')
        await checkEcho(websocket, b'Text as bynary array')
        await websocket.send('end')
        time.sleep(0.1)
        result = (await websocket.recv())
        if "finished" !=  result:
            print('Error incorrect finished message:', result)
            exit(1)

if __name__ == "__main__":
    asyncio.run(main())
