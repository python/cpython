from . import events
from . import protocols
from . import streams

_DEFAULT_LIMIT = 2 ** 16  # 64 KiB


async def connect(host=None, port=None, *,
                  loop=None, limit=_DEFAULT_LIMIT, **kwds):
    if loop is None:
        loop = events.get_running_loop()

    stream = Stream(limit=limit, loop=loop)


async def serve(client_connected_cb, host=None, port=None, *,
                loop=None, limit=_DEFAULT_LIMIT, **kwds):
    if loop is None:
        loop = events.get_running_loop()

    def factory():
        reader = Stream(limit=limit, loop=loop)
        protocol = StreamReaderProtocol(reader, client_connected_cb,
                                        loop=loop)
        return protocol

    return await loop.create_server(factory, host, port, **kwds)


class Stream:

    def __init__(self, limit, loop):
        self._limit = limit
        self._loop = loop
        self._transport = None
        self._protocol = None

    def _on_connection_made(self, transport):
        self._transport = transport

    def _on_connection_lost(self, exc):
        pass

    def _on_pause_writing(self):
        pass

    def _on_resume_writing(self):
        pass

    def _on_get_buffer(self, sizehint):
        pass

    def _on_buffer_updated(self, nbytes):
        pass

    def _on_eof(self):
        pass


class _BaseStreamProtocol(protocols.BufferedProtocol):
    def __init__(self, stream):
        self._stream = stream
        self._paused = False
        self._drain_waiter = None
        self._connection_lost = False

    def connection_made(self, transport):
        self._stream._on_connection_made(transport)

    def connection_lost(self, exc):
        self._stream._on_connection_lost(exc)

    def pause_writing(self):
        self._stream._on_pause_writing()

    def resume_writing(self):
        self._stream._on_resume_writing()

    def get_buffer(self, sizehint):
        return self._stream._on_get_buffer(sizehint)

    def buffer_updated(self, nbytes):
        self._stream._on_buffer_updated(nbytes)

    def eof_received(self):
        self._stream._on_eof()


class _ClientStreamProtocol(_BaseStreamProtocol):
    def __init__(self, stream):
        self._stream = stream


class _ServerStreamProtocol(_BaseStreamProtocol):
    def __init__(self, stream):
        self._stream = stream
