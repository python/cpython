"""Stream-related things."""

__all__ = ['StreamReader', 'StreamWriter', 'StreamReaderProtocol',
           'open_connection', 'start_server',
           ]

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
    writer = StreamWriter(transport, protocol, reader, loop)
    return reader, writer


@tasks.coroutine
def start_server(client_connected_cb, host=None, port=None, *,
                 loop=None, limit=_DEFAULT_LIMIT, **kwds):
    """Start a socket server, call back for each client connected.

    The first parameter, `client_connected_cb`, takes two parameters:
    client_reader, client_writer.  client_reader is a StreamReader
    object, while client_writer is a StreamWriter object.  This
    parameter can either be a plain callback function or a coroutine;
    if it is a coroutine, it will be automatically converted into a
    Task.

    The rest of the arguments are all the usual arguments to
    loop.create_server() except protocol_factory; most common are
    positional host and port, with various optional keyword arguments
    following.  The return value is the same as loop.create_server().

    Additional optional keyword arguments are loop (to set the event loop
    instance to use) and limit (to set the buffer limit passed to the
    StreamReader).

    The return value is the same as loop.create_server(), i.e. a
    Server object which can be used to stop the service.
    """
    if loop is None:
        loop = events.get_event_loop()

    def factory():
        reader = StreamReader(limit=limit, loop=loop)
        protocol = StreamReaderProtocol(reader, client_connected_cb,
                                        loop=loop)
        return protocol

    return (yield from loop.create_server(factory, host, port, **kwds))


class StreamReaderProtocol(protocols.Protocol):
    """Trivial helper class to adapt between Protocol and StreamReader.

    (This is a helper class instead of making StreamReader itself a
    Protocol subclass, because the StreamReader has other potential
    uses, and to prevent the user of the StreamReader to accidentally
    call inappropriate methods of the protocol.)
    """

    def __init__(self, stream_reader, client_connected_cb=None, loop=None):
        self._stream_reader = stream_reader
        self._stream_writer = None
        self._drain_waiter = None
        self._paused = False
        self._client_connected_cb = client_connected_cb
        self._loop = loop  # May be None; we may never need it.

    def connection_made(self, transport):
        self._stream_reader.set_transport(transport)
        if self._client_connected_cb is not None:
            self._stream_writer = StreamWriter(transport, self,
                                               self._stream_reader,
                                               self._loop)
            res = self._client_connected_cb(self._stream_reader,
                                            self._stream_writer)
            if tasks.iscoroutine(res):
                tasks.Task(res, loop=self._loop)

    def connection_lost(self, exc):
        if exc is None:
            self._stream_reader.feed_eof()
        else:
            self._stream_reader.set_exception(exc)
        # Also wake up the writing side.
        if self._paused:
            waiter = self._drain_waiter
            if waiter is not None:
                self._drain_waiter = None
                if not waiter.done():
                    if exc is None:
                        waiter.set_result(None)
                    else:
                        waiter.set_exception(exc)

    def data_received(self, data):
        self._stream_reader.feed_data(data)

    def eof_received(self):
        self._stream_reader.feed_eof()

    def pause_writing(self):
        assert not self._paused
        self._paused = True

    def resume_writing(self):
        assert self._paused
        self._paused = False
        waiter = self._drain_waiter
        if waiter is not None:
            self._drain_waiter = None
            if not waiter.done():
                waiter.set_result(None)


class StreamWriter:
    """Wraps a Transport.

    This exposes write(), writelines(), [can_]write_eof(),
    get_extra_info() and close().  It adds drain() which returns an
    optional Future on which you can wait for flow control.  It also
    adds a transport attribute which references the Transport
    directly.
    """

    def __init__(self, transport, protocol, reader, loop):
        self._transport = transport
        self._protocol = protocol
        self._reader = reader
        self._loop = loop

    @property
    def transport(self):
        return self._transport

    def write(self, data):
        self._transport.write(data)

    def writelines(self, data):
        self._transport.writelines(data)

    def write_eof(self):
        return self._transport.write_eof()

    def can_write_eof(self):
        return self._transport.can_write_eof()

    def close(self):
        return self._transport.close()

    def get_extra_info(self, name, default=None):
        return self._transport.get_extra_info(name, default)

    def drain(self):
        """This method has an unusual return value.

        The intended use is to write

          w.write(data)
          yield from w.drain()

        When there's nothing to wait for, drain() returns (), and the
        yield-from continues immediately.  When the transport buffer
        is full (the protocol is paused), drain() creates and returns
        a Future and the yield-from will block until that Future is
        completed, which will happen when the buffer is (partially)
        drained and the protocol is resumed.
        """
        if self._reader._exception is not None:
            raise self._reader._exception
        if self._transport._conn_lost:  # Uses private variable.
            raise ConnectionResetError('Connection lost')
        if not self._protocol._paused:
            return ()
        waiter = self._protocol._drain_waiter
        assert waiter is None or waiter.cancelled()
        waiter = futures.Future(loop=self._loop)
        self._protocol._drain_waiter = waiter
        return waiter


class StreamReader:

    def __init__(self, limit=_DEFAULT_LIMIT, loop=None):
        # The line length limit is  a security feature;
        # it also doubles as half the buffer limit.
        self._limit = limit
        if loop is None:
            loop = events.get_event_loop()
        self._loop = loop
        # TODO: Use a bytearray for a buffer, like the transport.
        self._buffer = collections.deque()  # Deque of bytes objects.
        self._byte_count = 0  # Bytes in buffer.
        self._eof = False  # Whether we're done.
        self._waiter = None  # A future.
        self._exception = None
        self._transport = None
        self._paused = False

    def exception(self):
        return self._exception

    def set_exception(self, exc):
        self._exception = exc

        waiter = self._waiter
        if waiter is not None:
            self._waiter = None
            if not waiter.cancelled():
                waiter.set_exception(exc)

    def set_transport(self, transport):
        assert self._transport is None, 'Transport already set'
        self._transport = transport

    def _maybe_resume_transport(self):
        if self._paused and self._byte_count <= self._limit:
            self._paused = False
            self._transport.resume_reading()

    def feed_eof(self):
        self._eof = True
        waiter = self._waiter
        if waiter is not None:
            self._waiter = None
            if not waiter.cancelled():
                waiter.set_result(True)

    def feed_data(self, data):
        if not data:
            return

        self._buffer.append(data)
        self._byte_count += len(data)

        waiter = self._waiter
        if waiter is not None:
            self._waiter = None
            if not waiter.cancelled():
                waiter.set_result(False)

        if (self._transport is not None and
            not self._paused and
            self._byte_count > 2*self._limit):
            try:
                self._transport.pause_reading()
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
            while self._buffer and not_enough:
                data = self._buffer.popleft()
                ichar = data.find(b'\n')
                if ichar < 0:
                    parts.append(data)
                    parts_size += len(data)
                else:
                    ichar += 1
                    head, tail = data[:ichar], data[ichar:]
                    if tail:
                        self._buffer.appendleft(tail)
                    not_enough = False
                    parts.append(head)
                    parts_size += len(head)

                if parts_size > self._limit:
                    self._byte_count -= parts_size
                    self._maybe_resume_transport()
                    raise ValueError('Line is too long')

            if self._eof:
                break

            if not_enough:
                assert self._waiter is None
                self._waiter = futures.Future(loop=self._loop)
                try:
                    yield from self._waiter
                finally:
                    self._waiter = None

        line = b''.join(parts)
        self._byte_count -= parts_size
        self._maybe_resume_transport()

        return line

    @tasks.coroutine
    def read(self, n=-1):
        if self._exception is not None:
            raise self._exception

        if not n:
            return b''

        if n < 0:
            while not self._eof:
                assert not self._waiter
                self._waiter = futures.Future(loop=self._loop)
                try:
                    yield from self._waiter
                finally:
                    self._waiter = None
        else:
            if not self._byte_count and not self._eof:
                assert not self._waiter
                self._waiter = futures.Future(loop=self._loop)
                try:
                    yield from self._waiter
                finally:
                    self._waiter = None

        if n < 0 or self._byte_count <= n:
            data = b''.join(self._buffer)
            self._buffer.clear()
            self._byte_count = 0
            self._maybe_resume_transport()
            return data

        parts = []
        parts_bytes = 0
        while self._buffer and parts_bytes < n:
            data = self._buffer.popleft()
            data_bytes = len(data)
            if n < parts_bytes + data_bytes:
                data_bytes = n - parts_bytes
                data, rest = data[:data_bytes], data[data_bytes:]
                self._buffer.appendleft(rest)

            parts.append(data)
            parts_bytes += data_bytes
            self._byte_count -= data_bytes
            self._maybe_resume_transport()

        return b''.join(parts)

    @tasks.coroutine
    def readexactly(self, n):
        if self._exception is not None:
            raise self._exception

        # There used to be "optimized" code here.  It created its own
        # Future and waited until self._buffer had at least the n
        # bytes, then called read(n).  Unfortunately, this could pause
        # the transport if the argument was larger than the pause
        # limit (which is twice self._limit).  So now we just read()
        # into a local buffer.

        blocks = []
        while n > 0:
            block = yield from self.read(n)
            if not block:
                break
            blocks.append(block)
            n -= len(block)

        # TODO: Raise EOFError if we break before n == 0?  (That would
        # be a change in specification, but I've always had to add an
        # explicit size check to the caller.)

        return b''.join(blocks)
