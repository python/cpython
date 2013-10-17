"""Event loop using a proactor and related classes.

A proactor is a "notify-on-completion" multiplexer.  Currently a
proactor is only implemented on Windows with IOCP.
"""

import socket

from . import base_events
from . import constants
from . import futures
from . import transports
from .log import logger


class _ProactorBasePipeTransport(transports.BaseTransport):
    """Base class for pipe and socket transports."""

    def __init__(self, loop, sock, protocol, waiter=None,
                 extra=None, server=None):
        super().__init__(extra)
        self._set_extra(sock)
        self._loop = loop
        self._sock = sock
        self._protocol = protocol
        self._server = server
        self._buffer = []
        self._read_fut = None
        self._write_fut = None
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

    def _fatal_error(self, exc):
        logger.exception('Fatal error for %s', self)
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
        self._buffer = []
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


class _ProactorReadPipeTransport(_ProactorBasePipeTransport,
                                 transports.ReadTransport):
    """Transport for read pipes."""

    def __init__(self, loop, sock, protocol, waiter=None,
                 extra=None, server=None):
        super().__init__(loop, sock, protocol, waiter, extra, server)
        self._read_fut = None
        self._paused = False
        self._loop.call_soon(self._loop_reading)

    def pause(self):
        assert not self._closing, 'Cannot pause() when closing'
        assert not self._paused, 'Already paused'
        self._paused = True

    def resume(self):
        assert self._paused, 'Not paused'
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
                self._fatal_error(exc)
        except ConnectionResetError as exc:
            self._force_close(exc)
        except OSError as exc:
            self._fatal_error(exc)
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


class _ProactorWritePipeTransport(_ProactorBasePipeTransport,
                                  transports.WriteTransport):
    """Transport for write pipes."""

    def write(self, data):
        assert isinstance(data, bytes), repr(data)
        if self._eof_written:
            raise IOError('write_eof() already called')

        if not data:
            return

        if self._conn_lost:
            if self._conn_lost >= constants.LOG_THRESHOLD_FOR_CONNLOST_WRITES:
                logger.warning('socket.send() raised exception.')
            self._conn_lost += 1
            return
        self._buffer.append(data)
        if self._write_fut is None:
            self._loop_writing()

    def _loop_writing(self, f=None):
        try:
            assert f is self._write_fut
            self._write_fut = None
            if f:
                f.result()
            data = b''.join(self._buffer)
            self._buffer = []
            if not data:
                if self._closing:
                    self._loop.call_soon(self._call_connection_lost, None)
                if self._eof_written:
                    self._sock.shutdown(socket.SHUT_WR)
                return
            self._write_fut = self._loop._proactor.send(self._sock, data)
            self._write_fut.add_done_callback(self._loop_writing)
        except ConnectionResetError as exc:
            self._force_close(exc)
        except OSError as exc:
            self._fatal_error(exc)

    def can_write_eof(self):
        return True

    def write_eof(self):
        self.close()

    def abort(self):
        self._force_close(None)


class _ProactorDuplexPipeTransport(_ProactorReadPipeTransport,
                                   _ProactorWritePipeTransport,
                                   transports.Transport):
    """Transport for duplex pipes."""

    def can_write_eof(self):
        return False

    def write_eof(self):
        raise NotImplementedError


class _ProactorSocketTransport(_ProactorReadPipeTransport,
                               _ProactorWritePipeTransport,
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
        return _ProactorWritePipeTransport(self, sock, protocol, waiter, extra)

    def close(self):
        if self._proactor is not None:
            self._close_self_pipe()
            self._proactor.close()
            self._proactor = None
            self._selector = None

    def sock_recv(self, sock, n):
        return self._proactor.recv(sock, n)

    def sock_sendall(self, sock, data):
        return self._proactor.send(sock, data)

    def sock_connect(self, sock, address):
        return self._proactor.connect(sock, address)

    def sock_accept(self, sock):
        return self._proactor.accept(sock)

    def _socketpair(self):
        raise NotImplementedError

    def _close_self_pipe(self):
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
            f.add_done_callback(self._loop_self_reading)

    def _write_to_self(self):
        self._csock.send(b'x')

    def _start_serving(self, protocol_factory, sock, ssl=None, server=None):
        assert not ssl, 'IocpEventLoop is incompatible with SSL.'

        def loop(f=None):
            try:
                if f is not None:
                    conn, addr = f.result()
                    protocol = protocol_factory()
                    self._make_socket_transport(
                        conn, protocol,
                        extra={'peername': addr}, server=server)
                f = self._proactor.accept(sock)
            except OSError:
                if sock.fileno() != -1:
                    logger.exception('Accept failed')
                    sock.close()
            except futures.CancelledError:
                sock.close()
            else:
                f.add_done_callback(loop)

        self.call_soon(loop)

    def _process_events(self, event_list):
        pass    # XXX hard work currently done in poll

    def _stop_serving(self, sock):
        self._proactor._stop_serving(sock)
        sock.close()
