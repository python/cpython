import asyncio
from asyncio import StreamReader, StreamWriter
from collections import defaultdict
from weakref import WeakValueDictionary, WeakSet
from typing import Dict, Callable, Set, MutableMapping, DefaultDict
from utils import new_messages, send_message
import json

ROOMS: DefaultDict[str, Set[StreamWriter]] = defaultdict(WeakSet)


async def main():
    server = await asyncio.start_server(client_connected, 'localhost', '9011')
    async with server:
        await server.serve_forever()


async def client_connected(reader: StreamReader, writer: StreamWriter):
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
        payload = json.dumps(msg).encode()
        room = ROOMS[msg["room"]]
        for friend in room:
            asyncio.create_task(send_message(friend, payload))

    handlers: Dict[str, Callable[[Dict], None]] = dict(
        connect=connect,
        joinroom=joinroom,
        leaveroom=leaveroom,
        chat=chat,
    )

    async for msg in new_messages(reader):
        action = msg.get('action')
        handler = handlers.get(action)
        handler(msg)


if __name__ == '__main__':
    asyncio.run(main())
