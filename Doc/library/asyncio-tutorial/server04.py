import asyncio
from asyncio import StreamReader, StreamWriter
from collections import defaultdict
from weakref import WeakValueDictionary, WeakSet
from typing import Dict, Callable, Set
from utils import new_messages

WRITERS: Dict[str, StreamWriter] = WeakValueDictionary()
ROOMS: Dict[str, Set[StreamWriter]] = defaultdict(WeakSet)


async def main():
    server = await asyncio.start_server(
        client_connected_cb=client_connected,
        host='localhost',
        port='9011',
    )
    async with server:
        await server.serve_forever()


async def client_connected(reader: StreamReader, writer: StreamWriter):
    addr = writer.get_extra_info('peername')
    WRITERS[addr] = writer

    def connect(msg):
        print(f"User connected: {msg.get('username')}")

    def joinroom(msg):
        room_name = msg["room"]
        print('joining room:', room_name)
        room = ROOMS[room_name]
        room.add(writer)

    def leaveroom(msg):
        room_name = msg["room"]
        print('leaving room:', msg.get('room'))
        room = ROOMS[room_name]
        room.discard(writer)

    def chat(msg):
        print(f'chat sent to room {msg.get("room")}: {msg.get("message")}')
        # TODO: distribute the message

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
