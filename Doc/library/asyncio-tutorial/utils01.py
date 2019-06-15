import json
from asyncio import (
    StreamReader, StreamWriter, IncompleteReadError, CancelledError
)
from typing import AsyncGenerator, Dict


async def new_messages(reader: StreamReader) -> AsyncGenerator[Dict, None]:
    try:
        while True:
            size_prefix = await reader.readexactly(3)
            size = int.from_bytes(size_prefix, byteorder='little')
            message = await reader.readexactly(size)
            try:
                yield json.loads(message)
            except json.decoder.JSONDecodeError:
                continue
    except (OSError, IncompleteReadError, CancelledError):
        # The connection is dead, leave.
        return


async def send_message(writer: StreamWriter, message: Dict):
    payload = json.dumps(message).encode()
    size_prefix = len(payload).to_bytes(3, byteorder='little')
    await writer.writelines([size_prefix, payload])
