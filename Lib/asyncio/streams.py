"""Stream-related things."""

__all__ = ['StreamReader', 'StreamWriter', 'StreamReaderProtocol',
           'open_connection', 'start_server',
           'IncompleteReadError',
           ]

import socket
import sys

if hasattr(socket, 'AF_UNIX'):
    __all__.extend(['open_unix_connection', 'start_unix_server'])

from . import coroutines
from . import events
from . import futures
from . import protocols
from .coroutines import coroutine
from .log import logger


_DEFAULT_LIMIT = 2**16
_PY35 = sys.version_info >= (3, 5)


class IncompleteReadError(EOFError):
    """
    Incomplete read error. Attributes:

    - partial: read bytes string before the end of stream was reached
    - expected: total number of expected bytes
    """
    def __init__(self, partial, expected):
        EOFError.__init__(self, "%s bytes read on a total of %s expected bytes"
                                % (len(partial), expected))
        self.partial = partial
        self.expected = expected


@coroutine
def open_connection(host=None, port=None, *,
                    loop=None, limit=_DEFAULT_LIMIT, **kwds):
    """A wrapper for create_connection() returning a (reader, writer) pair.

    The reader returned is a StreamReader instance; the writer is a
    StreamWriter instance.

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
    protocol = StreamReaderProtocol(reader, loop=loop)
    transport, _ = yield from loop.create_connection(
        lambda: protocol, host, port, **kwds)
    writer = StreamWriter(transport, protocol, reader, loop)
    return reader, writer


@coroutine
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


if hasattr(socket, 'AF_UNIX'):
    # UNIX Domain Sockets are supported on this platform

    @coroutine
    def open_unix_connection(path=None, *,
                             loop=None, limit=_DEFAULT_LIMIT, **kwds):
        """Similar to `open_connection` but works with UNIX Domain Sockets."""
        if loop is None:
            loop = events.get_event_loop()
        reader = StreamReader(limit=limit, loop=loop)
        protocol = StreamReaderProtocol(reader, loop=loop)
        transport, _ = yield from loop.create_unix_connection(
            lambda: protocol, path, **kwds)
        writer = StreamWriter(transport, protocol, reader, loop)
        return reader, writer


    @coroutine
    def start_unix_server(client_connected_cb, path=None, *,
                          loop=None, limit=_DEFAULT_LIMIT, **kwds):
        """Similar to `start_server` but works with UNIX Domain Sockets."""
        if loop is None:
            loop = events.get_event_loop()

        def factory():
            reader = StreamReader(limit=limit, loop=loop)
            protocol = StreamReaderProtocol(reader, client_connected_cb,
                                            loop=loop)
            return protocol

        return (yield from loop.create_unix_server(factory, path, **kwds))


class FlowControlMixin(protocols.Protocol):
    """Reusable flow control logic for StreamWriter.drain().

    This implements the protocol methods pause_writing(),
    resume_reading() and connection_lost().  If the subclass overrides
    these it must call the super methods.

    StreamWriter.drain() must wait for _drain_helper() coroutine.
    """

    def __init__(self, loop=None):
        if loop is None:
            self._loop = events.get_event_loop()
        else:
            self._loop = loop
        self._paused = False
        self._drain_waiter = None
        self._connection_lost = False

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

    @coroutine
    def _drain_helper(self):
        if self._connection_lost:
            raise ConnectionResetError('Connection lost')
        if not self._paused:
            return
        waiter = self._drain_waiter
        assert waiter is None or waiter.cancelled()
        waiter = futures.Future(loop=self._loop)
        self._drain_waiter = waiter
        yield from waiter


class StreamReaderProtocol(FlowControlMixin, protocols.Protocol):
    """Helper class to adapt between Protocol and StreamReader.

    (This is a helper class instead of making StreamReader itself a
    Protocol subclass, because the StreamReader has other potential
    uses, and to prevent the user of the StreamReader to accidentally
    call inappropriate methods of the protocol.)
    """

    def __init__(self, stream_reader, client_connected_cb=None, loop=None):
        super().__init__(loop=loop)
        self._stream_reader = stream_reader
        self._stream_writer = None
        self._client_connected_cb = client_connected_cb

    def connection_made(self, transport):
        self._stream_reader.set_transport(transport)
        if self._client_connected_cb is not None:
            self._stream_writer = StreamWriter(transport, self,
                                               self._stream_reader,
                                               self._loop)
            res = self._client_connected_cb(self._stream_reader,
                                            self._stream_writer)
            if coroutines.iscoroutine(res):
                self._loop.create_task(res)

    def connection_lost(self, exc):
        if exc is None:
            self._stream_reader.feed_eof()
        else:
            self._stream_reader.set_exception(exc)
        super().connection_lost(exc)

    def data_received(self, data):
        self._stream_reader.feed_data(data)

    def eof_received(self):
        self._stream_reader.feed_eof()


class StreamWriter:
    """Wraps a Transport.

    This exposes write(), writelines(), [can_]write_eof(),
    get_extra_info() and close().  It adds drain() which returns an
    optional Future on which you can wait for flow control.  It also
    adds a transport property which references the Transport
    directly.
    """

    def __init__(self, transport, protocol, reader, loop):
        self._transport = transport
        self._protocol = protocol
        # drain() expects that the reader has a exception() method
        assert reader is None or isinstance(reader, StreamReader)
        self._reader = reader
        self._loop = loop

    def __repr__(self):
        info = [self.__class__.__name__, 'transport=%r' % self._transport]
        if self._reader is not None:
            info.append('reader=%r' % self._reader)
        return '<%s>' % ' '.join(info)

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

    @coroutine
    def drain(self):
        """Flush the write buffer.

        The intended use is to write

          w.write(data)
          yield from w.drain()
        """
        if self._reader is not None:
            exc = self._reader.exception()
            if exc is not None:
                raise exc
        yield from self._protocol._drain_helper()


class StreamReader:

    def __init__(self, limit=_DEFAULT_LIMIT, loop=None):
        # The line length limit is  a security feature;
        # it also doubles as half the buffer limit.
        self._limit = limit
        if loop is None:
            self._loop = events.get_event_loop()
        else:
            self._loop = loop
        self._buffer = bytearray()
        self._eof = False    # Whether we're done.
        self._waiter = None  # A future used by _wait_for_data()
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

    def _wakeup_waiter(self):
        """Wakeup read() or readline() function waiting for data or EOF."""
        waiter = self._waiter
        if waiter is not None:
            self._waiter = None
            if not waiter.cancelled():
                waiter.set_result(None)

    def set_transport(self, transport):
        assert self._transport is None, 'Transport already set'
        self._transport = transport

    def _maybe_resume_transport(self):
        if self._paused and len(self._buffer) <= self._limit:
            self._paused = False
            self._transport.resume_reading()

    def feed_eof(self):
        self._eof = True
        self._wakeup_waiter()

    def at_eof(self):
        """Return True if the buffer is empty and 'feed_eof' was called."""
        return self._eof and not self._buffer

    def feed_data(self, data):
        assert not self._eof, 'feed_data after feed_eof'

        if not data:
            return

        self._buffer.extend(data)
        self._wakeup_waiter()

        if (self._transport is not None and
            not self._paused and
            len(self._buffer) > 2*self._limit):
            try:
                self._transport.pause_reading()
            except NotImplementedError:
                # The transport can't be paused.
                # We'll just have to buffer all data.
                # Forget the transport so we don't keep trying.
                self._transport = None
            else:
                self._paused = True

    @coroutine
    def _wait_for_data(self, func_name):
        """Wait until feed_data() or feed_eof() is called."""
        # StreamReader uses a future to link the protocol feed_data() method
        # to a read coroutine. Running two read coroutines at the same time
        # would have an unexpected behaviour. It would not possible to know
        # which coroutine would get the next data.
        if self._waiter is not None:
            raise RuntimeError('%s() called while another coroutine is '
                               'already waiting for incoming data' % func_name)

        self._waiter = futures.Future(loop=self._loop)
        try:
            yield from self._waiter
        finally:
            self._waiter = None

    @coroutine
    def readline(self):
        if self._exception is not None:
            raise self._exception

        line = bytearray()
        not_enough = True

        while not_enough:
            while self._buffer and not_enough:
                ichar = self._buffer.find(b'\n')
                if ichar < 0:
                    line.extend(self._buffer)
                    self._buffer.clear()
                else:
                    ichar += 1
                    line.extend(self._buffer[:ichar])
                    del self._buffer[:ichar]
                    not_enough = False

                if len(line) > self._limit:
                    self._maybe_resume_transport()
                    raise ValueError('Line is too long')

            if self._eof:
                break

            if not_enough:
                yield from self._wait_for_data('readline')

        self._maybe_resume_transport()
        return bytes(line)

    @coroutine
    def read(self, n=-1):
        if self._exception is not None:
            raise self._exception

        if not n:
            return b''

        if n < 0:
            # This used to just loop creating a new waiter hoping to
            # collect everything in self._buffer, but that would
            # deadlock if the subprocess sends more than self.limit
            # bytes.  So just call self.read(self._limit) until EOF.
            blocks = []
            while True:
                block = yield from self.read(self._limit)
                if not block:
                    break
                blocks.append(block)
            return b''.join(blocks)
        else:
            if not self._buffer and not self._eof:
                yield from self._wait_for_data('read')

        if n < 0 or len(self._buffer) <= n:
            data = bytes(self._buffer)
            self._buffer.clear()
        else:
            # n > 0 and len(self._buffer) > n
            data = bytes(self._buffer[:n])
            del self._buffer[:n]

        self._maybe_resume_transport()
        return data

    @coroutine
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
                partial = b''.join(blocks)
                raise IncompleteReadError(partial, len(partial) + n)
            blocks.append(block)
            n -= len(block)

        return b''.join(blocks)

    if _PY35:
        @coroutine
        def __aiter__(self):
            return self

        @coroutine
        def __anext__(self):
            val = yield from self.readline()
            if val == b'':
                raise StopAsyncIteration
            return val
