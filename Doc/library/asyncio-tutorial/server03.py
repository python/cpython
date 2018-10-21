import asyncio
from asyncio import StreamReader, StreamWriter
from typing import Dict, Callable
from utils import new_messages


async def main():
    server = await asyncio.start_server(
        client_connected_cb=client_connected,
        host='localhost',
        port='9011',
    )
    async with server:
        await server.serve_forever()


async def client_connected(reader: StreamReader, writer: StreamWriter):

    def connect(msg):
        print(msg.get('username'))

    def joinroom(msg):
        print('joining room:', msg.get('username'))

    def leaveroom(msg):
        print('leaving room:', msg.get('username'))

    def chat(msg):
        print(f'chat sent to room {msg.get("room")}: {msg.get("message")}')

    handlers: Dict[str, Callable] = dict(
        connect=connect,
        joinroom=joinroom,
        leaveroom=leaveroom,
        chat=chat,
    )

    async for msg in new_messages(reader):
        action = msg.get('action')
        if not action:
            continue

        handler = handlers.get(action)
        if not handler:
            continue

        handler(msg)


if __name__ == '__main__':
    asyncio.run(main())

