import sys
from asyncio import (StreamReader, StreamWriter, IncompleteReadError, Future,
    get_running_loop)

if sys.platform == 'win32':
    from signal import signal, SIGBREAK, SIGTERM, SIGINT
else:
    SIGBREAK = None
    from signal import signal, SIGTERM, SIGINT

from typing import AsyncGenerator


async def messages(reader: StreamReader) -> AsyncGenerator[bytes, None]:
    """Async generator to return messages as they come in."""
    try:
        while True:
            size_prefix = await reader.readexactly(4)
            size = int.from_bytes(size_prefix, byteorder='little')
            message = await reader.readexactly(size)
            yield message
    except (IncompleteReadError, ConnectionAbortedError, ConnectionResetError):
        return


async def send_message(writer: StreamWriter, message: bytes):
    """To close the connection, use an empty message."""
    if not message:
        writer.close()
        await writer.wait_closed()
        return
    size_prefix = len(message).to_bytes(4, byteorder='little')
    writer.write(size_prefix)
    writer.write(message)
    await writer.drain()


def install_signal_handling(fut: Future):
    """Given future will be set a signal is received. This
    can be used to control the shutdown sequence."""
    if sys.platform == 'win32':
        sigs = SIGBREAK, SIGINT
        loop = get_running_loop()

        def busyloop():
            """Required to handle CTRL-C quickly on Windows
            https://bugs.python.org/issue23057 """
            loop.call_later(0.1, busyloop)

        loop.call_later(0.1, busyloop)
    else:
        sigs = SIGTERM, SIGINT

    # Signal handlers. Windows is a bit tricky
    for s in sigs:
        signal(
            s,
            lambda *args: loop.call_soon_threadsafe(fut.set_result, None)
        )
