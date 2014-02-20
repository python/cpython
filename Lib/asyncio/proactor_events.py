"""Event loop using a proactor and related classes.

A proactor is a "notify-on-completion" multiplexer.  Currently a
proactor is only implemented on Windows with IOCP.
"""

__all__ = ['BaseProactorEventLoop']

import socket

from . import base_events
from . import constants
from . import futures
from . import transports
from .log import logger


class _ProactorBasePipeTransport(transports._FlowControlMixin,
                                 transports.BaseTransport):
    """Base class for pipe and socket transports."""

    def __init__(self, loop, sock, protocol, waiter=None,
                 extra=None, server=None):
        super().__init__(extra)
        self._set_extra(sock)
        self._loop = loop
        self._sock = sock
        self._protocol = protocol
        self._server = server
        self._buffer = None  # None or bytearray.
        self._read_fut = None
        self._write_fut = None
        self._pending_write = 0
        self._conn_lost = 0
        self._closing = False  # Set when close() called.
        self._eof_written = False
        if self._server is not None:
            self._server.attach(self)
        self._loop.call_soon(self._protocol.connection_made, self)
        if waiter is not None:
            self._loop.call_soon(waiter.set_result, None)

    def _set_extra(self, sock):
        self._extra['pipe'] = sock

    def close(self):
        if self._closing:
            return
        self._closing = True
        self._conn_lost += 1
        if not self._buffer and self._write_fut is None:
            self._loop.call_soon(self._call_connection_lost, None)
        if self._read_fut is not None:
            self._read_fut.cancel()

    def _fatal_error(self, exc, message='Fatal error on pipe transport'):
        if not isinstance(exc, (BrokenPipeError, ConnectionResetError)):
            self._loop.call_exception_handler({
                'message': message,
                'exception': exc,
                'transport': self,
                'protocol': self._protocol,
            })
        self._force_close(exc)

    def _force_close(self, exc):
        if self._closing:
            return
        self._closing = True
        self._conn_lost += 1
        if self._write_fut:
            self._write_fut.cancel()
        if self._read_fut:
            self._read_fut.cancel()
        self._write_fut = self._read_fut = None
        self._pending_write = 0
        self._buffer = None
        self._loop.call_soon(self._call_connection_lost, exc)

    def _call_connection_lost(self, exc):
        try:
            self._protocol.connection_lost(exc)
        finally:
            # XXX If there is a pending overlapped read on the other
            # end then it may fail with ERROR_NETNAME_DELETED if we
            # just close our end.  First calling shutdown() seems to
            # cure it, but maybe using DisconnectEx() would be better.
            if hasattr(self._sock, 'shutdown'):
                self._sock.shutdown(socket.SHUT_RDWR)
            self._sock.close()
            server = self._server
            if server is not None:
                server.detach(self)
                self._server = None

    def get_write_buffer_size(self):
        size = self._pending_write
        if self._buffer is not None:
            size += len(self._buffer)
        return size


class _ProactorReadPipeTransport(_ProactorBasePipeTransport,
                                 transports.ReadTransport):
    """Transport for read pipes."""

    def __init__(self, loop, sock, protocol, waiter=None,
                 extra=None, server=None):
        super().__init__(loop, sock, protocol, waiter, extra, server)
        self._read_fut = None
        self._paused = False
        self._loop.call_soon(self._loop_reading)

    def pause_reading(self):
        if self._closing:
            raise RuntimeError('Cannot pause_reading() when closing')
        if self._paused:
            raise RuntimeError('Already paused')
        self._paused = True

    def resume_reading(self):
        if not self._paused:
            raise RuntimeError('Not paused')
        self._paused = False
        if self._closing:
            return
        self._loop.call_soon(self._loop_reading, self._read_fut)

    def _loop_reading(self, fut=None):
        if self._paused:
            return
        data = None

        try:
            if fut is not None:
                assert self._read_fut is fut or (self._read_fut is None and
                                                 self._closing)
                self._read_fut = None
                data = fut.result()  # deliver data later in "finally" clause

            if self._closing:
                # since close() has been called we ignore any read data
                data = None
                return

            if data == b'':
                # we got end-of-file so no need to reschedule a new read
                return

            # reschedule a new read
            self._read_fut = self._loop._proactor.recv(self._sock, 4096)
        except ConnectionAbortedError as exc:
            if not self._closing:
                self._fatal_error(exc, 'Fatal read error on pipe transport')
        except ConnectionResetError as exc:
            self._force_close(exc)
        except OSError as exc:
            self._fatal_error(exc, 'Fatal read error on pipe transport')
        except futures.CancelledError:
            if not self._closing:
                raise
        else:
            self._read_fut.add_done_callback(self._loop_reading)
        finally:
            if data:
                self._protocol.data_received(data)
            elif data is not None:
                keep_open = self._protocol.eof_received()
                if not keep_open:
                    self.close()


class _ProactorBaseWritePipeTransport(_ProactorBasePipeTransport,
                                      transports.WriteTransport):
    """Transport for write pipes."""

    def write(self, data):
        if not isinstance(data, (bytes, bytearray, memoryview)):
            raise TypeError('data argument must be byte-ish (%r)',
                            type(data))
        if self._eof_written:
            raise RuntimeError('write_eof() already called')

        if not data:
            return

        if self._conn_lost:
            if self._conn_lost >= constants.LOG_THRESHOLD_FOR_CONNLOST_WRITES:
                logger.warning('socket.send() raised exception.')
            self._conn_lost += 1
            return

        # Observable states:
        # 1. IDLE: _write_fut and _buffer both None
        # 2. WRITING: _write_fut set; _buffer None
        # 3. BACKED UP: _write_fut set; _buffer a bytearray
        # We always copy the data, so the caller can't modify it
        # while we're still waiting for the I/O to happen.
        if self._write_fut is None:  # IDLE -> WRITING
            assert self._buffer is None
            # Pass a copy, except if it's already immutable.
            self._loop_writing(data=bytes(data))
            # XXX Should we pause the protocol at this point
            # if len(data) > self._high_water?  (That would
            # require keeping track of the number of bytes passed
            # to a send() that hasn't finished yet.)
        elif not self._buffer:  # WRITING -> BACKED UP
            # Make a mutable copy which we can extend.
            self._buffer = bytearray(data)
            self._maybe_pause_protocol()
        else:  # BACKED UP
            # Append to buffer (also copies).
            self._buffer.extend(data)
            self._maybe_pause_protocol()

    def _loop_writing(self, f=None, data=None):
        try:
            assert f is self._write_fut
            self._write_fut = None
            self._pending_write = 0
            if f:
                f.result()
            if data is None:
                data = self._buffer
                self._buffer = None
            if not data:
                if self._closing:
                    self._loop.call_soon(self._call_connection_lost, None)
                if self._eof_written:
                    self._sock.shutdown(socket.SHUT_WR)
                # Now that we've reduced the buffer size, tell the
                # protocol to resume writing if it was paused.  Note that
                # we do this last since the callback is called immediately
                # and it may add more data to the buffer (even causing the
                # protocol to be paused again).
                self._maybe_resume_protocol()
            else:
                self._write_fut = self._loop._proactor.send(self._sock, data)
                if not self._write_fut.done():
                    assert self._pending_write == 0
                    self._pending_write = len(data)
                    self._write_fut.add_done_callback(self._loop_writing)
                    self._maybe_pause_protocol()
                else:
                    self._write_fut.add_done_callback(self._loop_writing)
        except ConnectionResetError as exc:
            self._force_close(exc)
        except OSError as exc:
            self._fatal_error(exc, 'Fatal write error on pipe transport')

    def can_write_eof(self):
        return True

    def write_eof(self):
        self.close()

    def abort(self):
        self._force_close(None)


class _ProactorWritePipeTransport(_ProactorBaseWritePipeTransport):
    def __init__(self, *args, **kw):
        super().__init__(*args, **kw)
        self._read_fut = self._loop._proactor.recv(self._sock, 16)
        self._read_fut.add_done_callback(self._pipe_closed)

    def _pipe_closed(self, fut):
        if fut.cancelled():
            # the transport has been closed
            return
        assert fut.result() == b''
        if self._closing:
            assert self._read_fut is None
            return
        assert fut is self._read_fut, (fut, self._read_fut)
        self._read_fut = None
        if self._write_fut is not None:
            self._force_close(BrokenPipeError())
        else:
            self.close()


class _ProactorDuplexPipeTransport(_ProactorReadPipeTransport,
                                   _ProactorBaseWritePipeTransport,
                                   transports.Transport):
    """Transport for duplex pipes."""

    def can_write_eof(self):
        return False

    def write_eof(self):
        raise NotImplementedError


class _ProactorSocketTransport(_ProactorReadPipeTransport,
                               _ProactorBaseWritePipeTransport,
                               transports.Transport):
    """Transport for connected sockets."""

    def _set_extra(self, sock):
        self._extra['socket'] = sock
        try:
            self._extra['sockname'] = sock.getsockname()
        except (socket.error, AttributeError):
            pass
        if 'peername' not in self._extra:
            try:
                self._extra['peername'] = sock.getpeername()
            except (socket.error, AttributeError):
                pass

    def can_write_eof(self):
        return True

    def write_eof(self):
        if self._closing or self._eof_written:
            return
        self._eof_written = True
        if self._write_fut is None:
            self._sock.shutdown(socket.SHUT_WR)


class BaseProactorEventLoop(base_events.BaseEventLoop):

    def __init__(self, proactor):
        super().__init__()
        logger.debug('Using proactor: %s', proactor.__class__.__name__)
        self._proactor = proactor
        self._selector = proactor   # convenient alias
        self._self_reading_future = None
        self._accept_futures = {}   # socket file descriptor => Future
        proactor.set_loop(self)
        self._make_self_pipe()

    def _make_socket_transport(self, sock, protocol, waiter=None,
                               extra=None, server=None):
        return _ProactorSocketTransport(self, sock, protocol, waiter,
                                        extra, server)

    def _make_duplex_pipe_transport(self, sock, protocol, waiter=None,
                                    extra=None):
        return _ProactorDuplexPipeTransport(self,
                                            sock, protocol, waiter, extra)

    def _make_read_pipe_transport(self, sock, protocol, waiter=None,
                                  extra=None):
        return _ProactorReadPipeTransport(self, sock, protocol, waiter, extra)

    def _make_write_pipe_transport(self, sock, protocol, waiter=None,
                                   extra=None):
        # We want connection_lost() to be called when other end closes
        return _ProactorWritePipeTransport(self,
                                           sock, protocol, waiter, extra)

    def close(self):
        if self._proactor is not None:
            self._close_self_pipe()
            self._proactor.close()
            self._proactor = None
            self._selector = None
            super().close()
        self._accept_futures.clear()

    def sock_recv(self, sock, n):
        return self._proactor.recv(sock, n)

    def sock_sendall(self, sock, data):
        return self._proactor.send(sock, data)

    def sock_connect(self, sock, address):
        try:
            base_events._check_resolved_address(sock, address)
        except ValueError as err:
            fut = futures.Future(loop=self)
            fut.set_exception(err)
            return fut
        else:
            return self._proactor.connect(sock, address)

    def sock_accept(self, sock):
        return self._proactor.accept(sock)

    def _socketpair(self):
        raise NotImplementedError

    def _close_self_pipe(self):
        if self._self_reading_future is not None:
            self._self_reading_future.cancel()
            self._self_reading_future = None
        self._ssock.close()
        self._ssock = None
        self._csock.close()
        self._csock = None
        self._internal_fds -= 1

    def _make_self_pipe(self):
        # A self-socket, really. :-)
        self._ssock, self._csock = self._socketpair()
        self._ssock.setblocking(False)
        self._csock.setblocking(False)
        self._internal_fds += 1
        self.call_soon(self._loop_self_reading)

    def _loop_self_reading(self, f=None):
        try:
            if f is not None:
                f.result()  # may raise
            f = self._proactor.recv(self._ssock, 4096)
        except:
            self.close()
            raise
        else:
            self._self_reading_future = f
            f.add_done_callback(self._loop_self_reading)

    def _write_to_self(self):
        self._csock.send(b'x')

    def _start_serving(self, protocol_factory, sock, ssl=None, server=None):
        if ssl:
            raise ValueError('IocpEventLoop is incompatible with SSL.')

        def loop(f=None):
            try:
                if f is not None:
                    conn, addr = f.result()
                    protocol = protocol_factory()
                    self._make_socket_transport(
                        conn, protocol,
                        extra={'peername': addr}, server=server)
                f = self._proactor.accept(sock)
            except OSError as exc:
                if sock.fileno() != -1:
                    self.call_exception_handler({
                        'message': 'Accept failed',
                        'exception': exc,
                        'socket': sock,
                    })
                    sock.close()
            except futures.CancelledError:
                sock.close()
            else:
                self._accept_futures[sock.fileno()] = f
                f.add_done_callback(loop)

        self.call_soon(loop)

    def _process_events(self, event_list):
        pass    # XXX hard work currently done in poll

    def _stop_serving(self, sock):
        for future in self._accept_futures.values():
            future.cancel()
        self._proactor._stop_serving(sock)
        sock.close()
