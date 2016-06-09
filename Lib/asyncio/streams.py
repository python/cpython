"""Stream-related things."""

__all__ = ['StreamReader', 'StreamWriter', 'StreamReaderProtocol',
           'open_connection', 'start_server',
           'IncompleteReadError',
           'LimitOverrunError',
           ]

import socket

if hasattr(socket, 'AF_UNIX'):
    __all__.extend(['open_unix_connection', 'start_unix_server'])

from . import coroutines
from . import compat
from . import events
from . import protocols
from .coroutines import coroutine
from .log import logger


_DEFAULT_LIMIT = 2 ** 16


class IncompleteReadError(EOFError):
    """
    Incomplete read error. Attributes:

    - partial: read bytes string before the end of stream was reached
    - expected: total number of expected bytes (or None if unknown)
    """
    def __init__(self, partial, expected):
        super().__init__("%d bytes read on a total of %r expected bytes"
                         % (len(partial), expected))
        self.partial = partial
        self.expected = expected


class LimitOverrunError(Exception):
    """Reached the buffer limit while looking for a separator.

    Attributes:
    - consumed: total number of to be consumed bytes.
    """
    def __init__(self, message, consumed):
        super().__init__(message)
        self.consumed = consumed


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
        waiter = self._loop.create_future()
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
        self._over_ssl = False

    def connection_made(self, transport):
        self._stream_reader.set_transport(transport)
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
        if self._stream_reader is not None:
            if exc is None:
                self._stream_reader.feed_eof()
            else:
                self._stream_reader.set_exception(exc)
        super().connection_lost(exc)
        self._stream_reader = None
        self._stream_writer = None

    def data_received(self, data):
        self._stream_reader.feed_data(data)

    def eof_received(self):
        self._stream_reader.feed_eof()
        if self._over_ssl:
            # Prevent a warning in SSLProtocol.eof_received:
            # "returning true from eof_received()
            # has no effect when using ssl"
            return False
        return True


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
        # drain() expects that the reader has an exception() method
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
        if self._transport is not None:
            if self._transport.is_closing():
                # Yield to the event loop so connection_lost() may be
                # called.  Without this, _drain_helper() would return
                # immediately, and code that calls
                #     write(...); yield from drain()
                # in a loop would never call connection_lost(), so it
                # would not see an error when the socket is closed.
                yield
        yield from self._protocol._drain_helper()


class StreamReader:

    def __init__(self, limit=_DEFAULT_LIMIT, loop=None):
        # The line length limit is  a security feature;
        # it also doubles as half the buffer limit.

        if limit <= 0:
            raise ValueError('Limit cannot be <= 0')

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

    def __repr__(self):
        info = ['StreamReader']
        if self._buffer:
            info.append('%d bytes' % len(self._buffer))
        if self._eof:
            info.append('eof')
        if self._limit != _DEFAULT_LIMIT:
            info.append('l=%d' % self._limit)
        if self._waiter:
            info.append('w=%r' % self._waiter)
        if self._exception:
            info.append('e=%r' % self._exception)
        if self._transport:
            info.append('t=%r' % self._transport)
        if self._paused:
            info.append('paused')
        return '<%s>' % ' '.join(info)

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
        """Wakeup read*() functions waiting for data or EOF."""
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
                len(self._buffer) > 2 * self._limit):
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
        """Wait until feed_data() or feed_eof() is called.

        If stream was paused, automatically resume it.
        """
        # StreamReader uses a future to link the protocol feed_data() method
        # to a read coroutine. Running two read coroutines at the same time
        # would have an unexpected behaviour. It would not possible to know
        # which coroutine would get the next data.
        if self._waiter is not None:
            raise RuntimeError('%s() called while another coroutine is '
                               'already waiting for incoming data' % func_name)

        assert not self._eof, '_wait_for_data after EOF'

        # Waiting for data while paused will make deadlock, so prevent it.
        if self._paused:
            self._paused = False
            self._transport.resume_reading()

        self._waiter = self._loop.create_future()
        try:
            yield from self._waiter
        finally:
            self._waiter = None

    @coroutine
    def readline(self):
        """Read chunk of data from the stream until newline (b'\n') is found.

        On success, return chunk that ends with newline. If only partial
        line can be read due to EOF, return incomplete line without
        terminating newline. When EOF was reached while no bytes read, empty
        bytes object is returned.

        If limit is reached, ValueError will be raised. In that case, if
        newline was found, complete line including newline will be removed
        from internal buffer. Else, internal buffer will be cleared. Limit is
        compared against part of the line without newline.

        If stream was paused, this function will automatically resume it if
        needed.
        """
        sep = b'\n'
        seplen = len(sep)
        try:
            line = yield from self.readuntil(sep)
        except IncompleteReadError as e:
            return e.partial
        except LimitOverrunError as e:
            if self._buffer.startswith(sep, e.consumed):
                del self._buffer[:e.consumed + seplen]
            else:
                self._buffer.clear()
            self._maybe_resume_transport()
            raise ValueError(e.args[0])
        return line

    @coroutine
    def readuntil(self, separator=b'\n'):
        """Read data from the stream until ``separator`` is found.

        On success, the data and separator will be removed from the
        internal buffer (consumed). Returned data will include the
        separator at the end.

        Configured stream limit is used to check result. Limit sets the
        maximal length of data that can be returned, not counting the
        separator.

        If an EOF occurs and the complete separator is still not found,
        an IncompleteReadError exception will be raised, and the internal
        buffer will be reset.  The IncompleteReadError.partial attribute
        may contain the separator partially.

        If the data cannot be read because of over limit, a
        LimitOverrunError exception  will be raised, and the data
        will be left in the internal buffer, so it can be read again.
        """
        seplen = len(separator)
        if seplen == 0:
            raise ValueError('Separator should be at least one-byte string')

        if self._exception is not None:
            raise self._exception

        # Consume whole buffer except last bytes, which length is
        # one less than seplen. Let's check corner cases with
        # separator='SEPARATOR':
        # * we have received almost complete separator (without last
        #   byte). i.e buffer='some textSEPARATO'. In this case we
        #   can safely consume len(separator) - 1 bytes.
        # * last byte of buffer is first byte of separator, i.e.
        #   buffer='abcdefghijklmnopqrS'. We may safely consume
        #   everything except that last byte, but this require to
        #   analyze bytes of buffer that match partial separator.
        #   This is slow and/or require FSM. For this case our
        #   implementation is not optimal, since require rescanning
        #   of data that is known to not belong to separator. In
        #   real world, separator will not be so long to notice
        #   performance problems. Even when reading MIME-encoded
        #   messages :)

        # `offset` is the number of bytes from the beginning of the buffer
        # where there is no occurrence of `separator`.
        offset = 0

        # Loop until we find `separator` in the buffer, exceed the buffer size,
        # or an EOF has happened.
        while True:
            buflen = len(self._buffer)

            # Check if we now have enough data in the buffer for `separator` to
            # fit.
            if buflen - offset >= seplen:
                isep = self._buffer.find(separator, offset)

                if isep != -1:
                    # `separator` is in the buffer. `isep` will be used later
                    # to retrieve the data.
                    break

                # see upper comment for explanation.
                offset = buflen + 1 - seplen
                if offset > self._limit:
                    raise LimitOverrunError(
                        'Separator is not found, and chunk exceed the limit',
                        offset)

            # Complete message (with full separator) may be present in buffer
            # even when EOF flag is set. This may happen when the last chunk
            # adds data which makes separator be found. That's why we check for
            # EOF *ater* inspecting the buffer.
            if self._eof:
                chunk = bytes(self._buffer)
                self._buffer.clear()
                raise IncompleteReadError(chunk, None)

            # _wait_for_data() will resume reading if stream was paused.
            yield from self._wait_for_data('readuntil')

        if isep > self._limit:
            raise LimitOverrunError(
                'Separator is found, but chunk is longer than limit', isep)

        chunk = self._buffer[:isep + seplen]
        del self._buffer[:isep + seplen]
        self._maybe_resume_transport()
        return bytes(chunk)

    @coroutine
    def read(self, n=-1):
        """Read up to `n` bytes from the stream.

        If n is not provided, or set to -1, read until EOF and return all read
        bytes. If the EOF was received and the internal buffer is empty, return
        an empty bytes object.

        If n is zero, return empty bytes object immediatelly.

        If n is positive, this function try to read `n` bytes, and may return
        less or equal bytes than requested, but at least one byte. If EOF was
        received before any byte is read, this function returns empty byte
        object.

        Returned value is not limited with limit, configured at stream
        creation.

        If stream was paused, this function will automatically resume it if
        needed.
        """

        if self._exception is not None:
            raise self._exception

        if n == 0:
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

        if not self._buffer and not self._eof:
            yield from self._wait_for_data('read')

        # This will work right even if buffer is less than n bytes
        data = bytes(self._buffer[:n])
        del self._buffer[:n]

        self._maybe_resume_transport()
        return data

    @coroutine
    def readexactly(self, n):
        """Read exactly `n` bytes.

        Raise an IncompleteReadError if EOF is reached before `n` bytes can be
        read. The IncompleteReadError.partial attribute of the exception will
        contain the partial read bytes.

        if n is zero, return empty bytes object.

        Returned value is not limited with limit, configured at stream
        creation.

        If stream was paused, this function will automatically resume it if
        needed.
        """
        if n < 0:
            raise ValueError('readexactly size can not be less than zero')

        if self._exception is not None:
            raise self._exception

        if n == 0:
            return b''

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

        assert n == 0

        return b''.join(blocks)

    if compat.PY35:
        @coroutine
        def __aiter__(self):
            return self

        @coroutine
        def __anext__(self):
            val = yield from self.readline()
            if val == b'':
                raise StopAsyncIteration
            return val

    if compat.PY352:
        # In Python 3.5.2 and greater, __aiter__ should return
        # the asynchronous iterator directly.
        def __aiter__(self):
            return self
