import asyncio
from asyncio import StreamReader, StreamWriter

async def main():
    server = await asyncio.start_server(
        client_connected_cb=client_connected,
        host='localhost',
        port='9011',
    )
    async with server:
        await server.serve_forever()

async def client_connected(reader: StreamReader, writer: StreamWriter):
    print('New client connected!')

if __name__ == '__main__':
    asyncio.run(main())
