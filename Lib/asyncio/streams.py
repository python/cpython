"""Stream-related things."""

__all__ = ['StreamReader', 'StreamReaderProtocol', 'open_connection']

import collections

from . import events
from . import futures
from . import protocols
from . import tasks


_DEFAULT_LIMIT = 2**16


@tasks.coroutine
def open_connection(host=None, port=None, *,
                    loop=None, limit=_DEFAULT_LIMIT, **kwds):
    """A wrapper for create_connection() returning a (reader, writer) pair.

    The reader returned is a StreamReader instance; the writer is a
    Transport.

    The arguments are all the usual arguments to create_connection()
    except protocol_factory; most common are positional host and port,
    with various optional keyword arguments following.

    Additional optional keyword arguments are loop (to set the event loop
    instance to use) and limit (to set the buffer limit passed to the
    StreamReader).

    (If you want to customize the StreamReader and/or
    StreamReaderProtocol classes, just copy the code -- there's
    really nothing special here except some convenience.)
    """
    if loop is None:
        loop = events.get_event_loop()
    reader = StreamReader(limit=limit, loop=loop)
    protocol = StreamReaderProtocol(reader)
    transport, _ = yield from loop.create_connection(
        lambda: protocol, host, port, **kwds)
    return reader, transport  # (reader, writer)


class StreamReaderProtocol(protocols.Protocol):
    """Trivial helper class to adapt between Protocol and StreamReader.

    (This is a helper class instead of making StreamReader itself a
    Protocol subclass, because the StreamReader has other potential
    uses, and to prevent the user of the StreamReader to accidentally
    call inappropriate methods of the protocol.)
    """

    def __init__(self, stream_reader):
        self.stream_reader = stream_reader

    def connection_made(self, transport):
        self.stream_reader.set_transport(transport)

    def connection_lost(self, exc):
        if exc is None:
            self.stream_reader.feed_eof()
        else:
            self.stream_reader.set_exception(exc)

    def data_received(self, data):
        self.stream_reader.feed_data(data)

    def eof_received(self):
        self.stream_reader.feed_eof()


class StreamReader:

    def __init__(self, limit=_DEFAULT_LIMIT, loop=None):
        # The line length limit is  a security feature;
        # it also doubles as half the buffer limit.
        self.limit = limit
        if loop is None:
            loop = events.get_event_loop()
        self.loop = loop
        self.buffer = collections.deque()  # Deque of bytes objects.
        self.byte_count = 0  # Bytes in buffer.
        self.eof = False  # Whether we're done.
        self.waiter = None  # A future.
        self._exception = None
        self._transport = None
        self._paused = False

    def exception(self):
        return self._exception

    def set_exception(self, exc):
        self._exception = exc

        waiter = self.waiter
        if waiter is not None:
            self.waiter = None
            if not waiter.cancelled():
                waiter.set_exception(exc)

    def set_transport(self, transport):
        assert self._transport is None, 'Transport already set'
        self._transport = transport

    def _maybe_resume_transport(self):
        if self._paused and self.byte_count <= self.limit:
            self._paused = False
            self._transport.resume()

    def feed_eof(self):
        self.eof = True
        waiter = self.waiter
        if waiter is not None:
            self.waiter = None
            if not waiter.cancelled():
                waiter.set_result(True)

    def feed_data(self, data):
        if not data:
            return

        self.buffer.append(data)
        self.byte_count += len(data)

        waiter = self.waiter
        if waiter is not None:
            self.waiter = None
            if not waiter.cancelled():
                waiter.set_result(False)

        if (self._transport is not None and
            not self._paused and
            self.byte_count > 2*self.limit):
            try:
                self._transport.pause()
            except NotImplementedError:
                # The transport can't be paused.
                # We'll just have to buffer all data.
                # Forget the transport so we don't keep trying.
                self._transport = None
            else:
                self._paused = True

    @tasks.coroutine
    def readline(self):
        if self._exception is not None:
            raise self._exception

        parts = []
        parts_size = 0
        not_enough = True

        while not_enough:
            while self.buffer and not_enough:
                data = self.buffer.popleft()
                ichar = data.find(b'\n')
                if ichar < 0:
                    parts.append(data)
                    parts_size += len(data)
                else:
                    ichar += 1
                    head, tail = data[:ichar], data[ichar:]
                    if tail:
                        self.buffer.appendleft(tail)
                    not_enough = False
                    parts.append(head)
                    parts_size += len(head)

                if parts_size > self.limit:
                    self.byte_count -= parts_size
                    self._maybe_resume_transport()
                    raise ValueError('Line is too long')

            if self.eof:
                break

            if not_enough:
                assert self.waiter is None
                self.waiter = futures.Future(loop=self.loop)
                try:
                    yield from self.waiter
                finally:
                    self.waiter = None

        line = b''.join(parts)
        self.byte_count -= parts_size
        self._maybe_resume_transport()

        return line

    @tasks.coroutine
    def read(self, n=-1):
        if self._exception is not None:
            raise self._exception

        if not n:
            return b''

        if n < 0:
            while not self.eof:
                assert not self.waiter
                self.waiter = futures.Future(loop=self.loop)
                try:
                    yield from self.waiter
                finally:
                    self.waiter = None
        else:
            if not self.byte_count and not self.eof:
                assert not self.waiter
                self.waiter = futures.Future(loop=self.loop)
                try:
                    yield from self.waiter
                finally:
                    self.waiter = None

        if n < 0 or self.byte_count <= n:
            data = b''.join(self.buffer)
            self.buffer.clear()
            self.byte_count = 0
            self._maybe_resume_transport()
            return data

        parts = []
        parts_bytes = 0
        while self.buffer and parts_bytes < n:
            data = self.buffer.popleft()
            data_bytes = len(data)
            if n < parts_bytes + data_bytes:
                data_bytes = n - parts_bytes
                data, rest = data[:data_bytes], data[data_bytes:]
                self.buffer.appendleft(rest)

            parts.append(data)
            parts_bytes += data_bytes
            self.byte_count -= data_bytes
            self._maybe_resume_transport()

        return b''.join(parts)

    @tasks.coroutine
    def readexactly(self, n):
        if self._exception is not None:
            raise self._exception

        if n <= 0:
            return b''

        while self.byte_count < n and not self.eof:
            assert not self.waiter
            self.waiter = futures.Future(loop=self.loop)
            try:
                yield from self.waiter
            finally:
                self.waiter = None

        return (yield from self.read(n))
