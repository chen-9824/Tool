import asyncio
import websockets

# 处理连接的协程
async def echo(websocket, path):
    # 获取客户端的 IP 和端口
    client_ip, client_port = websocket.remote_address
    print(f"New connection from {client_ip}:{client_port}")
    
    # 处理客户端消息
    async for message in websocket:
        print(f"Message from {client_ip}:{client_port}: {message}")
        await websocket.send(f"Echo: {message}")

# 启动 WebSocket 服务器
async def main():
    async with websockets.serve(echo, "localhost", 8765):
    #async with websockets.serve(echo, "0.0.0.0", 8765):
        await asyncio.Future()  # 运行服务器直到手动终止

# 运行服务器
asyncio.run(main())