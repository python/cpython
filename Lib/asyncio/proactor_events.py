"""Event loop using a proactor and related classes.

A proactor is a "notify-on-completion" multiplexer.  Currently a
proactor is only implemented on Windows with IOCP.
"""

__all__ = 'BaseProactorEventLoop',

import io
import os
import socket
import warnings

from . import base_events
from . import constants
from . import events
from . import futures
from . import protocols
from . import sslproto
from . import transports
from .log import logger


class _ProactorBasePipeTransport(transports._FlowControlMixin,
                                 transports.BaseTransport):
    """Base class for pipe and socket transports."""

    def __init__(self, loop, sock, protocol, waiter=None,
                 extra=None, server=None):
        super().__init__(extra, loop)
        self._set_extra(sock)
        self._sock = sock
        self.set_protocol(protocol)
        self._server = server
        self._buffer = None  # None or bytearray.
        self._read_fut = None
        self._write_fut = None
        self._pending_write = 0
        self._conn_lost = 0
        self._closing = False  # Set when close() called.
        self._eof_written = False
        if self._server is not None:
            self._server._attach()
        self._loop.call_soon(self._protocol.connection_made, self)
        if waiter is not None:
            # only wake up the waiter when connection_made() has been called
            self._loop.call_soon(futures._set_result_unless_cancelled,
                                 waiter, None)

    def __repr__(self):
        info = [self.__class__.__name__]
        if self._sock is None:
            info.append('closed')
        elif self._closing:
            info.append('closing')
        if self._sock is not None:
            info.append(f'fd={self._sock.fileno()}')
        if self._read_fut is not None:
            info.append(f'read={self._read_fut!r}')
        if self._write_fut is not None:
            info.append(f'write={self._write_fut!r}')
        if self._buffer:
            info.append(f'write_bufsize={len(self._buffer)}')
        if self._eof_written:
            info.append('EOF written')
        return '<{}>'.format(' '.join(info))

    def _set_extra(self, sock):
        self._extra['pipe'] = sock

    def set_protocol(self, protocol):
        self._protocol = protocol

    def get_protocol(self):
        return self._protocol

    def is_closing(self):
        return self._closing

    def close(self):
        if self._closing:
            return
        self._closing = True
        self._conn_lost += 1
        if not self._buffer and self._write_fut is None:
            self._loop.call_soon(self._call_connection_lost, None)
        if self._read_fut is not None:
            self._read_fut.cancel()
            self._read_fut = None

    def __del__(self):
        if self._sock is not None:
            warnings.warn(f"unclosed transport {self!r}", ResourceWarning,
                          source=self)
            self.close()

    def _fatal_error(self, exc, message='Fatal error on pipe transport'):
        try:
            if isinstance(exc, base_events._FATAL_ERROR_IGNORE):
                if self._loop.get_debug():
                    logger.debug("%r: %s", self, message, exc_info=True)
            else:
                self._loop.call_exception_handler({
                    'message': message,
                    'exception': exc,
                    'transport': self,
                    'protocol': self._protocol,
                })
        finally:
            self._force_close(exc)

    def _force_close(self, exc):
        if self._empty_waiter is not None:
            if exc is None:
                self._empty_waiter.set_result(None)
            else:
                self._empty_waiter.set_exception(exc)
        if self._closing:
            return
        self._closing = True
        self._conn_lost += 1
        if self._write_fut:
            self._write_fut.cancel()
            self._write_fut = None
        if self._read_fut:
            self._read_fut.cancel()
            self._read_fut = None
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
            self._sock = None
            server = self._server
            if server is not None:
                server._detach()
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
        self._loop_reading_cb = None
        self._paused = True
        super().__init__(loop, sock, protocol, waiter, extra, server)

        self._reschedule_on_resume = False
        self._loop.call_soon(self._loop_reading)
        self._paused = False

    def set_protocol(self, protocol):
        if isinstance(protocol, protocols.BufferedProtocol):
            self._loop_reading_cb = self._loop_reading__get_buffer
        else:
            self._loop_reading_cb = self._loop_reading__data_received

        super().set_protocol(protocol)

        if self.is_reading():
            # reset reading callback / buffers / self._read_fut
            self.pause_reading()
            self.resume_reading()

    def is_reading(self):
        return not self._paused and not self._closing

    def pause_reading(self):
        if self._closing or self._paused:
            return
        self._paused = True

        if self._read_fut is not None and not self._read_fut.done():
            # TODO: This is an ugly hack to cancel the current read future
            # *and* avoid potential race conditions, as read cancellation
            # goes through `future.cancel()` and `loop.call_soon()`.
            # We then use this special attribute in the reader callback to
            # exit *immediately* without doing any cleanup/rescheduling.
            self._read_fut.__asyncio_cancelled_on_pause__ = True

            self._read_fut.cancel()
            self._read_fut = None
            self._reschedule_on_resume = True

        if self._loop.get_debug():
            logger.debug("%r pauses reading", self)

    def resume_reading(self):
        if self._closing or not self._paused:
            return
        self._paused = False
        if self._reschedule_on_resume:
            self._loop.call_soon(self._loop_reading, self._read_fut)
            self._reschedule_on_resume = False
        if self._loop.get_debug():
            logger.debug("%r resumes reading", self)

    def _loop_reading__on_eof(self):
        if self._loop.get_debug():
            logger.debug("%r received EOF", self)

        try:
            keep_open = self._protocol.eof_received()
        except Exception as exc:
            self._fatal_error(
                exc, 'Fatal error: protocol.eof_received() call failed.')
            return

        if not keep_open:
            self.close()

    def _loop_reading(self, fut=None):
        self._loop_reading_cb(fut)

    def _loop_reading__data_received(self, fut):
        if (fut is not None and
                getattr(fut, '__asyncio_cancelled_on_pause__', False)):
            return

        if self._paused:
            self._reschedule_on_resume = True
            return

        data = None
        try:
            if fut is not None:
                assert self._read_fut is fut or (self._read_fut is None and
                                                 self._closing)
                self._read_fut = None
                if fut.done():
                    # deliver data later in "finally" clause
                    data = fut.result()
                else:
                    # the future will be replaced by next proactor.recv call
                    fut.cancel()

            if self._closing:
                # since close() has been called we ignore any read data
                data = None
                return

            if data == b'':
                # we got end-of-file so no need to reschedule a new read
                return

            # reschedule a new read
            self._read_fut = self._loop._proactor.recv(self._sock, 32768)
        except ConnectionAbortedError as exc:
            if not self._closing:
                self._fatal_error(exc, 'Fatal read error on pipe transport')
            elif self._loop.get_debug():
                logger.debug("Read error on pipe transport while closing",
                             exc_info=True)
        except ConnectionResetError as exc:
            self._force_close(exc)
        except OSError as exc:
            self._fatal_error(exc, 'Fatal read error on pipe transport')
        except futures.CancelledError:
            if not self._closing:
                raise
        else:
            self._read_fut.add_done_callback(self._loop_reading__data_received)
        finally:
            if data:
                self._protocol.data_received(data)
            elif data == b'':
                self._loop_reading__on_eof()

    def _loop_reading__get_buffer(self, fut):
        if (fut is not None and
                getattr(fut, '__asyncio_cancelled_on_pause__', False)):
            return

        if self._paused:
            self._reschedule_on_resume = True
            return

        nbytes = None
        if fut is not None:
            assert self._read_fut is fut or (self._read_fut is None and
                                             self._closing)
            self._read_fut = None
            try:
                if fut.done():
                    nbytes = fut.result()
                else:
                    # the future will be replaced by next proactor.recv call
                    fut.cancel()
            except ConnectionAbortedError as exc:
                if not self._closing:
                    self._fatal_error(
                        exc, 'Fatal read error on pipe transport')
                elif self._loop.get_debug():
                    logger.debug("Read error on pipe transport while closing",
                                 exc_info=True)
            except ConnectionResetError as exc:
                self._force_close(exc)
            except OSError as exc:
                self._fatal_error(exc, 'Fatal read error on pipe transport')
            except futures.CancelledError:
                if not self._closing:
                    raise

            if nbytes is not None:
                if nbytes == 0:
                    # we got end-of-file so no need to reschedule a new read
                    self._loop_reading__on_eof()
                else:
                    try:
                        self._protocol.buffer_updated(nbytes)
                    except Exception as exc:
                        self._fatal_error(
                            exc,
                            'Fatal error: '
                            'protocol.buffer_updated() call failed.')
                        return

        if self._closing or nbytes == 0:
            # since close() has been called we ignore any read data
            return

        try:
            buf = self._protocol.get_buffer(-1)
            if not len(buf):
                raise RuntimeError('get_buffer() returned an empty buffer')
        except Exception as exc:
            self._fatal_error(
                exc, 'Fatal error: protocol.get_buffer() call failed.')
            return

        try:
            # schedule a new read
            self._read_fut = self._loop._proactor.recv_into(self._sock, buf)
            self._read_fut.add_done_callback(self._loop_reading__get_buffer)
        except ConnectionAbortedError as exc:
            if not self._closing:
                self._fatal_error(exc, 'Fatal read error on pipe transport')
            elif self._loop.get_debug():
                logger.debug("Read error on pipe transport while closing",
                             exc_info=True)
        except ConnectionResetError as exc:
            self._force_close(exc)
        except OSError as exc:
            self._fatal_error(exc, 'Fatal read error on pipe transport')
        except futures.CancelledError:
            if not self._closing:
                raise


class _ProactorBaseWritePipeTransport(_ProactorBasePipeTransport,
                                      transports.WriteTransport):
    """Transport for write pipes."""

    _start_tls_compatible = True

    def __init__(self, *args, **kw):
        super().__init__(*args, **kw)
        self._empty_waiter = None

    def write(self, data):
        if not isinstance(data, (bytes, bytearray, memoryview)):
            raise TypeError(
                f"data argument must be a bytes-like object, "
                f"not {type(data).__name__}")
        if self._eof_written:
            raise RuntimeError('write_eof() already called')
        if self._empty_waiter is not None:
            raise RuntimeError('unable to write; sendfile is in progress')

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
            if self._empty_waiter is not None and self._write_fut is None:
                self._empty_waiter.set_result(None)
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

    def _make_empty_waiter(self):
        if self._empty_waiter is not None:
            raise RuntimeError("Empty waiter is already set")
        self._empty_waiter = self._loop.create_future()
        if self._write_fut is None:
            self._empty_waiter.set_result(None)
        return self._empty_waiter

    def _reset_empty_waiter(self):
        self._empty_waiter = None


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

    _sendfile_compatible = constants._SendfileMode.TRY_NATIVE

    def _set_extra(self, sock):
        self._extra['socket'] = sock

        try:
            self._extra['sockname'] = sock.getsockname()
        except (socket.error, AttributeError):
            if self._loop.get_debug():
                logger.warning(
                    "getsockname() failed on %r", sock, exc_info=True)

        if 'peername' not in self._extra:
            try:
                self._extra['peername'] = sock.getpeername()
            except (socket.error, AttributeError):
                if self._loop.get_debug():
                    logger.warning("getpeername() failed on %r",
                                   sock, exc_info=True)

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

    def _make_ssl_transport(
            self, rawsock, protocol, sslcontext, waiter=None,
            *, server_side=False, server_hostname=None,
            extra=None, server=None,
            ssl_handshake_timeout=None):
        ssl_protocol = sslproto.SSLProtocol(
                self, protocol, sslcontext, waiter,
                server_side, server_hostname,
                ssl_handshake_timeout=ssl_handshake_timeout)
        _ProactorSocketTransport(self, rawsock, ssl_protocol,
                                 extra=extra, server=server)
        return ssl_protocol._app_transport

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
        if self.is_running():
            raise RuntimeError("Cannot close a running event loop")
        if self.is_closed():
            return

        # Call these methods before closing the event loop (before calling
        # BaseEventLoop.close), because they can schedule callbacks with
        # call_soon(), which is forbidden when the event loop is closed.
        self._stop_accept_futures()
        self._close_self_pipe()
        self._proactor.close()
        self._proactor = None
        self._selector = None

        # Close the event loop
        super().close()

    async def sock_recv(self, sock, n):
        return await self._proactor.recv(sock, n)

    async def sock_recv_into(self, sock, buf):
        return await self._proactor.recv_into(sock, buf)

    async def sock_sendall(self, sock, data):
        return await self._proactor.send(sock, data)

    async def sock_connect(self, sock, address):
        return await self._proactor.connect(sock, address)

    async def sock_accept(self, sock):
        return await self._proactor.accept(sock)

    async def _sock_sendfile_native(self, sock, file, offset, count):
        try:
            fileno = file.fileno()
        except (AttributeError, io.UnsupportedOperation) as err:
            raise events.SendfileNotAvailableError("not a regular file")
        try:
            fsize = os.fstat(fileno).st_size
        except OSError as err:
            raise events.SendfileNotAvailableError("not a regular file")
        blocksize = count if count else fsize
        if not blocksize:
            return 0  # empty file

        blocksize = min(blocksize, 0xffff_ffff)
        end_pos = min(offset + count, fsize) if count else fsize
        offset = min(offset, fsize)
        total_sent = 0
        try:
            while True:
                blocksize = min(end_pos - offset, blocksize)
                if blocksize <= 0:
                    return total_sent
                await self._proactor.sendfile(sock, file, offset, blocksize)
                offset += blocksize
                total_sent += blocksize
        finally:
            if total_sent > 0:
                file.seek(offset)

    async def _sendfile_native(self, transp, file, offset, count):
        resume_reading = transp.is_reading()
        transp.pause_reading()
        await transp._make_empty_waiter()
        try:
            return await self.sock_sendfile(transp._sock, file, offset, count,
                                            fallback=False)
        finally:
            transp._reset_empty_waiter()
            if resume_reading:
                transp.resume_reading()

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
        self._ssock, self._csock = socket.socketpair()
        self._ssock.setblocking(False)
        self._csock.setblocking(False)
        self._internal_fds += 1
        self.call_soon(self._loop_self_reading)

    def _loop_self_reading(self, f=None):
        try:
            if f is not None:
                f.result()  # may raise
            f = self._proactor.recv(self._ssock, 4096)
        except futures.CancelledError:
            # _close_self_pipe() has been called, stop waiting for data
            return
        except Exception as exc:
            self.call_exception_handler({
                'message': 'Error on reading from the event loop self pipe',
                'exception': exc,
                'loop': self,
            })
        else:
            self._self_reading_future = f
            f.add_done_callback(self._loop_self_reading)

    def _write_to_self(self):
        self._csock.send(b'\0')

    def _start_serving(self, protocol_factory, sock,
                       sslcontext=None, server=None, backlog=100,
                       ssl_handshake_timeout=None):

        def loop(f=None):
            try:
                if f is not None:
                    conn, addr = f.result()
                    if self._debug:
                        logger.debug("%r got a new connection from %r: %r",
                                     server, addr, conn)
                    protocol = protocol_factory()
                    if sslcontext is not None:
                        self._make_ssl_transport(
                            conn, protocol, sslcontext, server_side=True,
                            extra={'peername': addr}, server=server,
                            ssl_handshake_timeout=ssl_handshake_timeout)
                    else:
                        self._make_socket_transport(
                            conn, protocol,
                            extra={'peername': addr}, server=server)
                if self.is_closed():
                    return
                f = self._proactor.accept(sock)
            except OSError as exc:
                if sock.fileno() != -1:
                    self.call_exception_handler({
                        'message': 'Accept failed on a socket',
                        'exception': exc,
                        'socket': sock,
                    })
                    sock.close()
                elif self._debug:
                    logger.debug("Accept failed on socket %r",
                                 sock, exc_info=True)
            except futures.CancelledError:
                sock.close()
            else:
                self._accept_futures[sock.fileno()] = f
                f.add_done_callback(loop)

        self.call_soon(loop)

    def _process_events(self, event_list):
        # Events are processed in the IocpProactor._poll() method
        pass

    def _stop_accept_futures(self):
        for future in self._accept_futures.values():
            future.cancel()
        self._accept_futures.clear()

    def _stop_serving(self, sock):
        future = self._accept_futures.pop(sock.fileno(), None)
        if future:
            future.cancel()
        self._proactor._stop_serving(sock)
        sock.close()
