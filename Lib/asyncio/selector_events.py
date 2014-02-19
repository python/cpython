"""Event loop using a selector and related classes.

A selector is a "notify-when-ready" multiplexer.  For a subclass which
also includes support for signal handling, see the unix_events sub-module.
"""

__all__ = ['BaseSelectorEventLoop']

import collections
import errno
import socket
try:
    import ssl
except ImportError:  # pragma: no cover
    ssl = None

from . import base_events
from . import constants
from . import events
from . import futures
from . import selectors
from . import transports
from .log import logger


class BaseSelectorEventLoop(base_events.BaseEventLoop):
    """Selector event loop.

    See events.EventLoop for API specification.
    """

    def __init__(self, selector=None):
        super().__init__()

        if selector is None:
            selector = selectors.DefaultSelector()
        logger.debug('Using selector: %s', selector.__class__.__name__)
        self._selector = selector
        self._make_self_pipe()

    def _make_socket_transport(self, sock, protocol, waiter=None, *,
                               extra=None, server=None):
        return _SelectorSocketTransport(self, sock, protocol, waiter,
                                        extra, server)

    def _make_ssl_transport(self, rawsock, protocol, sslcontext, waiter, *,
                            server_side=False, server_hostname=None,
                            extra=None, server=None):
        return _SelectorSslTransport(
            self, rawsock, protocol, sslcontext, waiter,
            server_side, server_hostname, extra, server)

    def _make_datagram_transport(self, sock, protocol,
                                 address=None, extra=None):
        return _SelectorDatagramTransport(self, sock, protocol, address, extra)

    def close(self):
        if self._selector is not None:
            self._close_self_pipe()
            self._selector.close()
            self._selector = None
            super().close()

    def _socketpair(self):
        raise NotImplementedError

    def _close_self_pipe(self):
        self.remove_reader(self._ssock.fileno())
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
        self.add_reader(self._ssock.fileno(), self._read_from_self)

    def _read_from_self(self):
        try:
            self._ssock.recv(1)
        except (BlockingIOError, InterruptedError):
            pass

    def _write_to_self(self):
        try:
            self._csock.send(b'x')
        except (BlockingIOError, InterruptedError):
            pass

    def _start_serving(self, protocol_factory, sock,
                       sslcontext=None, server=None):
        self.add_reader(sock.fileno(), self._accept_connection,
                        protocol_factory, sock, sslcontext, server)

    def _accept_connection(self, protocol_factory, sock,
                           sslcontext=None, server=None):
        try:
            conn, addr = sock.accept()
            conn.setblocking(False)
        except (BlockingIOError, InterruptedError, ConnectionAbortedError):
            pass  # False alarm.
        except OSError as exc:
            # There's nowhere to send the error, so just log it.
            # TODO: Someone will want an error handler for this.
            if exc.errno in (errno.EMFILE, errno.ENFILE,
                             errno.ENOBUFS, errno.ENOMEM):
                # Some platforms (e.g. Linux keep reporting the FD as
                # ready, so we remove the read handler temporarily.
                # We'll try again in a while.
                self.call_exception_handler({
                    'message': 'socket.accept() out of system resource',
                    'exception': exc,
                    'socket': sock,
                })
                self.remove_reader(sock.fileno())
                self.call_later(constants.ACCEPT_RETRY_DELAY,
                                self._start_serving,
                                protocol_factory, sock, sslcontext, server)
            else:
                raise  # The event loop will catch, log and ignore it.
        else:
            if sslcontext:
                self._make_ssl_transport(
                    conn, protocol_factory(), sslcontext, None,
                    server_side=True, extra={'peername': addr}, server=server)
            else:
                self._make_socket_transport(
                    conn, protocol_factory(), extra={'peername': addr},
                    server=server)
        # It's now up to the protocol to handle the connection.

    def add_reader(self, fd, callback, *args):
        """Add a reader callback."""
        handle = events.Handle(callback, args, self)
        try:
            key = self._selector.get_key(fd)
        except KeyError:
            self._selector.register(fd, selectors.EVENT_READ,
                                    (handle, None))
        else:
            mask, (reader, writer) = key.events, key.data
            self._selector.modify(fd, mask | selectors.EVENT_READ,
                                  (handle, writer))
            if reader is not None:
                reader.cancel()

    def remove_reader(self, fd):
        """Remove a reader callback."""
        try:
            key = self._selector.get_key(fd)
        except KeyError:
            return False
        else:
            mask, (reader, writer) = key.events, key.data
            mask &= ~selectors.EVENT_READ
            if not mask:
                self._selector.unregister(fd)
            else:
                self._selector.modify(fd, mask, (None, writer))

            if reader is not None:
                reader.cancel()
                return True
            else:
                return False

    def add_writer(self, fd, callback, *args):
        """Add a writer callback.."""
        handle = events.Handle(callback, args, self)
        try:
            key = self._selector.get_key(fd)
        except KeyError:
            self._selector.register(fd, selectors.EVENT_WRITE,
                                    (None, handle))
        else:
            mask, (reader, writer) = key.events, key.data
            self._selector.modify(fd, mask | selectors.EVENT_WRITE,
                                  (reader, handle))
            if writer is not None:
                writer.cancel()

    def remove_writer(self, fd):
        """Remove a writer callback."""
        try:
            key = self._selector.get_key(fd)
        except KeyError:
            return False
        else:
            mask, (reader, writer) = key.events, key.data
            # Remove both writer and connector.
            mask &= ~selectors.EVENT_WRITE
            if not mask:
                self._selector.unregister(fd)
            else:
                self._selector.modify(fd, mask, (reader, None))

            if writer is not None:
                writer.cancel()
                return True
            else:
                return False

    def sock_recv(self, sock, n):
        """XXX"""
        fut = futures.Future(loop=self)
        self._sock_recv(fut, False, sock, n)
        return fut

    def _sock_recv(self, fut, registered, sock, n):
        # _sock_recv() can add itself as an I/O callback if the operation can't
        # be done immediately. Don't use it directly, call sock_recv().
        fd = sock.fileno()
        if registered:
            # Remove the callback early.  It should be rare that the
            # selector says the fd is ready but the call still returns
            # EAGAIN, and I am willing to take a hit in that case in
            # order to simplify the common case.
            self.remove_reader(fd)
        if fut.cancelled():
            return
        try:
            data = sock.recv(n)
        except (BlockingIOError, InterruptedError):
            self.add_reader(fd, self._sock_recv, fut, True, sock, n)
        except Exception as exc:
            fut.set_exception(exc)
        else:
            fut.set_result(data)

    def sock_sendall(self, sock, data):
        """XXX"""
        fut = futures.Future(loop=self)
        if data:
            self._sock_sendall(fut, False, sock, data)
        else:
            fut.set_result(None)
        return fut

    def _sock_sendall(self, fut, registered, sock, data):
        fd = sock.fileno()

        if registered:
            self.remove_writer(fd)
        if fut.cancelled():
            return

        try:
            n = sock.send(data)
        except (BlockingIOError, InterruptedError):
            n = 0
        except Exception as exc:
            fut.set_exception(exc)
            return

        if n == len(data):
            fut.set_result(None)
        else:
            if n:
                data = data[n:]
            self.add_writer(fd, self._sock_sendall, fut, True, sock, data)

    def sock_connect(self, sock, address):
        """XXX"""
        fut = futures.Future(loop=self)
        try:
            base_events._check_resolved_address(sock, address)
        except ValueError as err:
            fut.set_exception(err)
        else:
            self._sock_connect(fut, False, sock, address)
        return fut

    def _sock_connect(self, fut, registered, sock, address):
        fd = sock.fileno()
        if registered:
            self.remove_writer(fd)
        if fut.cancelled():
            return
        try:
            if not registered:
                # First time around.
                sock.connect(address)
            else:
                err = sock.getsockopt(socket.SOL_SOCKET, socket.SO_ERROR)
                if err != 0:
                    # Jump to the except clause below.
                    raise OSError(err, 'Connect call failed %s' % (address,))
        except (BlockingIOError, InterruptedError):
            self.add_writer(fd, self._sock_connect, fut, True, sock, address)
        except Exception as exc:
            fut.set_exception(exc)
        else:
            fut.set_result(None)

    def sock_accept(self, sock):
        """XXX"""
        fut = futures.Future(loop=self)
        self._sock_accept(fut, False, sock)
        return fut

    def _sock_accept(self, fut, registered, sock):
        fd = sock.fileno()
        if registered:
            self.remove_reader(fd)
        if fut.cancelled():
            return
        try:
            conn, address = sock.accept()
            conn.setblocking(False)
        except (BlockingIOError, InterruptedError):
            self.add_reader(fd, self._sock_accept, fut, True, sock)
        except Exception as exc:
            fut.set_exception(exc)
        else:
            fut.set_result((conn, address))

    def _process_events(self, event_list):
        for key, mask in event_list:
            fileobj, (reader, writer) = key.fileobj, key.data
            if mask & selectors.EVENT_READ and reader is not None:
                if reader._cancelled:
                    self.remove_reader(fileobj)
                else:
                    self._add_callback(reader)
            if mask & selectors.EVENT_WRITE and writer is not None:
                if writer._cancelled:
                    self.remove_writer(fileobj)
                else:
                    self._add_callback(writer)

    def _stop_serving(self, sock):
        self.remove_reader(sock.fileno())
        sock.close()


class _SelectorTransport(transports._FlowControlMixin,
                         transports.Transport):

    max_size = 256 * 1024  # Buffer size passed to recv().

    _buffer_factory = bytearray  # Constructs initial value for self._buffer.

    def __init__(self, loop, sock, protocol, extra, server=None):
        super().__init__(extra)
        self._extra['socket'] = sock
        self._extra['sockname'] = sock.getsockname()
        if 'peername' not in self._extra:
            try:
                self._extra['peername'] = sock.getpeername()
            except socket.error:
                self._extra['peername'] = None
        self._loop = loop
        self._sock = sock
        self._sock_fd = sock.fileno()
        self._protocol = protocol
        self._server = server
        self._buffer = self._buffer_factory()
        self._conn_lost = 0  # Set when call to connection_lost scheduled.
        self._closing = False  # Set when close() called.
        if self._server is not None:
            self._server.attach(self)

    def abort(self):
        self._force_close(None)

    def close(self):
        if self._closing:
            return
        self._closing = True
        self._loop.remove_reader(self._sock_fd)
        if not self._buffer:
            self._conn_lost += 1
            self._loop.call_soon(self._call_connection_lost, None)

    def _fatal_error(self, exc, message='Fatal error on transport'):
        # Should be called from exception handler only.
        if not isinstance(exc, (BrokenPipeError, ConnectionResetError)):
            self._loop.call_exception_handler({
                'message': message,
                'exception': exc,
                'transport': self,
                'protocol': self._protocol,
            })
        self._force_close(exc)

    def _force_close(self, exc):
        if self._conn_lost:
            return
        if self._buffer:
            self._buffer.clear()
            self._loop.remove_writer(self._sock_fd)
        if not self._closing:
            self._closing = True
            self._loop.remove_reader(self._sock_fd)
        self._conn_lost += 1
        self._loop.call_soon(self._call_connection_lost, exc)

    def _call_connection_lost(self, exc):
        try:
            self._protocol.connection_lost(exc)
        finally:
            self._sock.close()
            self._sock = None
            self._protocol = None
            self._loop = None
            server = self._server
            if server is not None:
                server.detach(self)
                self._server = None

    def get_write_buffer_size(self):
        return len(self._buffer)


class _SelectorSocketTransport(_SelectorTransport):

    def __init__(self, loop, sock, protocol, waiter=None,
                 extra=None, server=None):
        super().__init__(loop, sock, protocol, extra, server)
        self._eof = False
        self._paused = False

        self._loop.add_reader(self._sock_fd, self._read_ready)
        self._loop.call_soon(self._protocol.connection_made, self)
        if waiter is not None:
            self._loop.call_soon(waiter.set_result, None)

    def pause_reading(self):
        if self._closing:
            raise RuntimeError('Cannot pause_reading() when closing')
        if self._paused:
            raise RuntimeError('Already paused')
        self._paused = True
        self._loop.remove_reader(self._sock_fd)

    def resume_reading(self):
        if not self._paused:
            raise RuntimeError('Not paused')
        self._paused = False
        if self._closing:
            return
        self._loop.add_reader(self._sock_fd, self._read_ready)

    def _read_ready(self):
        try:
            data = self._sock.recv(self.max_size)
        except (BlockingIOError, InterruptedError):
            pass
        except Exception as exc:
            self._fatal_error(exc, 'Fatal read error on socket transport')
        else:
            if data:
                self._protocol.data_received(data)
            else:
                keep_open = self._protocol.eof_received()
                if keep_open:
                    # We're keeping the connection open so the
                    # protocol can write more, but we still can't
                    # receive more, so remove the reader callback.
                    self._loop.remove_reader(self._sock_fd)
                else:
                    self.close()

    def write(self, data):
        if not isinstance(data, (bytes, bytearray, memoryview)):
            raise TypeError('data argument must be byte-ish (%r)',
                            type(data))
        if self._eof:
            raise RuntimeError('Cannot call write() after write_eof()')
        if not data:
            return

        if self._conn_lost:
            if self._conn_lost >= constants.LOG_THRESHOLD_FOR_CONNLOST_WRITES:
                logger.warning('socket.send() raised exception.')
            self._conn_lost += 1
            return

        if not self._buffer:
            # Optimization: try to send now.
            try:
                n = self._sock.send(data)
            except (BlockingIOError, InterruptedError):
                pass
            except Exception as exc:
                self._fatal_error(exc, 'Fatal write error on socket transport')
                return
            else:
                data = data[n:]
                if not data:
                    return
            # Not all was written; register write handler.
            self._loop.add_writer(self._sock_fd, self._write_ready)

        # Add it to the buffer.
        self._buffer.extend(data)
        self._maybe_pause_protocol()

    def _write_ready(self):
        assert self._buffer, 'Data should not be empty'

        try:
            n = self._sock.send(self._buffer)
        except (BlockingIOError, InterruptedError):
            pass
        except Exception as exc:
            self._loop.remove_writer(self._sock_fd)
            self._buffer.clear()
            self._fatal_error(exc, 'Fatal write error on socket transport')
        else:
            if n:
                del self._buffer[:n]
            self._maybe_resume_protocol()  # May append to buffer.
            if not self._buffer:
                self._loop.remove_writer(self._sock_fd)
                if self._closing:
                    self._call_connection_lost(None)
                elif self._eof:
                    self._sock.shutdown(socket.SHUT_WR)

    def write_eof(self):
        if self._eof:
            return
        self._eof = True
        if not self._buffer:
            self._sock.shutdown(socket.SHUT_WR)

    def can_write_eof(self):
        return True


class _SelectorSslTransport(_SelectorTransport):

    _buffer_factory = bytearray

    def __init__(self, loop, rawsock, protocol, sslcontext, waiter=None,
                 server_side=False, server_hostname=None,
                 extra=None, server=None):
        if ssl is None:
            raise RuntimeError('stdlib ssl module not available')

        if server_side:
            if not sslcontext:
                raise ValueError('Server side ssl needs a valid SSLContext')
        else:
            if not sslcontext:
                # Client side may pass ssl=True to use a default
                # context; in that case the sslcontext passed is None.
                # The default is the same as used by urllib with
                # cadefault=True.
                if hasattr(ssl, '_create_stdlib_context'):
                    sslcontext = ssl._create_stdlib_context(
                        cert_reqs=ssl.CERT_REQUIRED,
                        check_hostname=bool(server_hostname))
                else:
                    # Fallback for Python 3.3.
                    sslcontext = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
                    sslcontext.options |= ssl.OP_NO_SSLv2
                    sslcontext.set_default_verify_paths()
                    sslcontext.verify_mode = ssl.CERT_REQUIRED

        wrap_kwargs = {
            'server_side': server_side,
            'do_handshake_on_connect': False,
        }
        if server_hostname and not server_side and ssl.HAS_SNI:
            wrap_kwargs['server_hostname'] = server_hostname
        sslsock = sslcontext.wrap_socket(rawsock, **wrap_kwargs)

        super().__init__(loop, sslsock, protocol, extra, server)

        self._server_hostname = server_hostname
        self._waiter = waiter
        self._rawsock = rawsock
        self._sslcontext = sslcontext
        self._paused = False

        # SSL-specific extra info.  (peercert is set later)
        self._extra.update(sslcontext=sslcontext)

        self._on_handshake()

    def _on_handshake(self):
        try:
            self._sock.do_handshake()
        except ssl.SSLWantReadError:
            self._loop.add_reader(self._sock_fd, self._on_handshake)
            return
        except ssl.SSLWantWriteError:
            self._loop.add_writer(self._sock_fd, self._on_handshake)
            return
        except Exception as exc:
            self._loop.remove_reader(self._sock_fd)
            self._loop.remove_writer(self._sock_fd)
            self._sock.close()
            if self._waiter is not None:
                self._waiter.set_exception(exc)
            return
        except BaseException as exc:
            self._loop.remove_reader(self._sock_fd)
            self._loop.remove_writer(self._sock_fd)
            self._sock.close()
            if self._waiter is not None:
                self._waiter.set_exception(exc)
            raise

        self._loop.remove_reader(self._sock_fd)
        self._loop.remove_writer(self._sock_fd)

        peercert = self._sock.getpeercert()
        if not hasattr(self._sslcontext, 'check_hostname'):
            # Verify hostname if requested, Python 3.4+ uses check_hostname
            # and checks the hostname in do_handshake()
            if (self._server_hostname and
                self._sslcontext.verify_mode != ssl.CERT_NONE):
                try:
                    ssl.match_hostname(peercert, self._server_hostname)
                except Exception as exc:
                    self._sock.close()
                    if self._waiter is not None:
                        self._waiter.set_exception(exc)
                    return

        # Add extra info that becomes available after handshake.
        self._extra.update(peercert=peercert,
                           cipher=self._sock.cipher(),
                           compression=self._sock.compression(),
                           )

        self._read_wants_write = False
        self._write_wants_read = False
        self._loop.add_reader(self._sock_fd, self._read_ready)
        self._loop.call_soon(self._protocol.connection_made, self)
        if self._waiter is not None:
            self._loop.call_soon(self._waiter.set_result, None)

    def pause_reading(self):
        # XXX This is a bit icky, given the comment at the top of
        # _read_ready().  Is it possible to evoke a deadlock?  I don't
        # know, although it doesn't look like it; write() will still
        # accept more data for the buffer and eventually the app will
        # call resume_reading() again, and things will flow again.

        if self._closing:
            raise RuntimeError('Cannot pause_reading() when closing')
        if self._paused:
            raise RuntimeError('Already paused')
        self._paused = True
        self._loop.remove_reader(self._sock_fd)

    def resume_reading(self):
        if not self._paused:
            raise ('Not paused')
        self._paused = False
        if self._closing:
            return
        self._loop.add_reader(self._sock_fd, self._read_ready)

    def _read_ready(self):
        if self._write_wants_read:
            self._write_wants_read = False
            self._write_ready()

            if self._buffer:
                self._loop.add_writer(self._sock_fd, self._write_ready)

        try:
            data = self._sock.recv(self.max_size)
        except (BlockingIOError, InterruptedError, ssl.SSLWantReadError):
            pass
        except ssl.SSLWantWriteError:
            self._read_wants_write = True
            self._loop.remove_reader(self._sock_fd)
            self._loop.add_writer(self._sock_fd, self._write_ready)
        except Exception as exc:
            self._fatal_error(exc, 'Fatal read error on SSL transport')
        else:
            if data:
                self._protocol.data_received(data)
            else:
                try:
                    keep_open = self._protocol.eof_received()
                    if keep_open:
                        logger.warning('returning true from eof_received() '
                                       'has no effect when using ssl')
                finally:
                    self.close()

    def _write_ready(self):
        if self._read_wants_write:
            self._read_wants_write = False
            self._read_ready()

            if not (self._paused or self._closing):
                self._loop.add_reader(self._sock_fd, self._read_ready)

        if self._buffer:
            try:
                n = self._sock.send(self._buffer)
            except (BlockingIOError, InterruptedError,
                    ssl.SSLWantWriteError):
                n = 0
            except ssl.SSLWantReadError:
                n = 0
                self._loop.remove_writer(self._sock_fd)
                self._write_wants_read = True
            except Exception as exc:
                self._loop.remove_writer(self._sock_fd)
                self._buffer.clear()
                self._fatal_error(exc, 'Fatal write error on SSL transport')
                return

            if n:
                del self._buffer[:n]

        self._maybe_resume_protocol()  # May append to buffer.

        if not self._buffer:
            self._loop.remove_writer(self._sock_fd)
            if self._closing:
                self._call_connection_lost(None)

    def write(self, data):
        if not isinstance(data, (bytes, bytearray, memoryview)):
            raise TypeError('data argument must be byte-ish (%r)',
                            type(data))
        if not data:
            return

        if self._conn_lost:
            if self._conn_lost >= constants.LOG_THRESHOLD_FOR_CONNLOST_WRITES:
                logger.warning('socket.send() raised exception.')
            self._conn_lost += 1
            return

        if not self._buffer:
            self._loop.add_writer(self._sock_fd, self._write_ready)

        # Add it to the buffer.
        self._buffer.extend(data)
        self._maybe_pause_protocol()

    def can_write_eof(self):
        return False


class _SelectorDatagramTransport(_SelectorTransport):

    _buffer_factory = collections.deque

    def __init__(self, loop, sock, protocol, address=None, extra=None):
        super().__init__(loop, sock, protocol, extra)
        self._address = address
        self._loop.add_reader(self._sock_fd, self._read_ready)
        self._loop.call_soon(self._protocol.connection_made, self)

    def get_write_buffer_size(self):
        return sum(len(data) for data, _ in self._buffer)

    def _read_ready(self):
        try:
            data, addr = self._sock.recvfrom(self.max_size)
        except (BlockingIOError, InterruptedError):
            pass
        except OSError as exc:
            self._protocol.error_received(exc)
        except Exception as exc:
            self._fatal_error(exc, 'Fatal read error on datagram transport')
        else:
            self._protocol.datagram_received(data, addr)

    def sendto(self, data, addr=None):
        if not isinstance(data, (bytes, bytearray, memoryview)):
            raise TypeError('data argument must be byte-ish (%r)',
                            type(data))
        if not data:
            return

        if self._address and addr not in (None, self._address):
            raise ValueError('Invalid address: must be None or %s' %
                             (self._address,))

        if self._conn_lost and self._address:
            if self._conn_lost >= constants.LOG_THRESHOLD_FOR_CONNLOST_WRITES:
                logger.warning('socket.send() raised exception.')
            self._conn_lost += 1
            return

        if not self._buffer:
            # Attempt to send it right away first.
            try:
                if self._address:
                    self._sock.send(data)
                else:
                    self._sock.sendto(data, addr)
                return
            except (BlockingIOError, InterruptedError):
                self._loop.add_writer(self._sock_fd, self._sendto_ready)
            except OSError as exc:
                self._protocol.error_received(exc)
                return
            except Exception as exc:
                self._fatal_error(exc,
                                  'Fatal write error on datagram transport')
                return

        # Ensure that what we buffer is immutable.
        self._buffer.append((bytes(data), addr))
        self._maybe_pause_protocol()

    def _sendto_ready(self):
        while self._buffer:
            data, addr = self._buffer.popleft()
            try:
                if self._address:
                    self._sock.send(data)
                else:
                    self._sock.sendto(data, addr)
            except (BlockingIOError, InterruptedError):
                self._buffer.appendleft((data, addr))  # Try again later.
                break
            except OSError as exc:
                self._protocol.error_received(exc)
                return
            except Exception as exc:
                self._fatal_error(exc,
                                  'Fatal write error on datagram transport')
                return

        self._maybe_resume_protocol()  # May append to buffer.
        if not self._buffer:
            self._loop.remove_writer(self._sock_fd)
            if self._closing:
                self._call_connection_lost(None)
