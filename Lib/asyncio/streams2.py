from . import events
from . import exceptions
from . import protocols

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
        protocol = _StreamProtocol(reader, client_connected_cb,
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
        self._over_ssl = False

    def connection_made(self, transport):
        self._stream.set_transport(transport)
        self._over_ssl = transport.get_extra_info('sslcontext') is not None
        if self._client_connected_cb is not None:
            self._stream_writer = StreamWriter(transport, self,
                                               self._stream_reader,
                                               self._loop)
            res = self._client_connected_cb(self._stream_reader,
                                            self._stream_writer)
            if coroutines.iscoroutine(res):
                self._loop.create_task(res)

    def connection_lost(self, exc):
        self._connection_lost = True
        # Wake up the writer if currently paused.
        if not self._paused:
            return
        waiter = self._drain_waiter
        if waiter is None:
            return
        self._drain_waiter = None
        if waiter.done():
            return
        if exc is None:
            waiter.set_result(None)
        else:
            waiter.set_exception(exc)

    def pause_writing(self):
        assert not self._paused
        self._paused = True
        if self._loop.get_debug():
            logger.debug("%r pauses writing", self)

    def resume_writing(self):
        assert self._paused
        self._paused = False
        if self._loop.get_debug():
            logger.debug("%r resumes writing", self)

        waiter = self._drain_waiter
        if waiter is not None:
            self._drain_waiter = None
            if not waiter.done():
                waiter.set_result(None)

    def get_buffer(self, sizehint):
        return self._stream._on_get_buffer(sizehint)

    def buffer_updated(self, nbytes):
        self._stream._on_buffer_updated(nbytes)

    def eof_received(self):
        self._stream._on_eof()
        if self._over_ssl:
            # Prevent a warning in SSLProtocol.eof_received:
            # "returning true from eof_received()
            # has no effect when using ssl"
            return False
        return True


class _ClientStreamProtocol(_BaseStreamProtocol):
    def __init__(self, stream):
        self._stream = stream


class _ServerStreamProtocol(_BaseStreamProtocol):
    def __init__(self, stream):
        self._stream = stream
