"""Event loop using a selector and related classes.

A selector is a "notify-when-ready" multiplexer.  For a subclass which
also includes support for signal handling, see the unix_events sub-module.
"""

import collections
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

    def _start_serving(self, protocol_factory, sock, ssl=None, server=None):
        self.add_reader(sock.fileno(), self._accept_connection,
                        protocol_factory, sock, ssl, server)

    def _accept_connection(self, protocol_factory, sock, ssl=None,
                           server=None):
        try:
            conn, addr = sock.accept()
            conn.setblocking(False)
        except (BlockingIOError, InterruptedError):
            pass  # False alarm.
        except Exception:
            # Bad error. Stop serving.
            self.remove_reader(sock.fileno())
            sock.close()
            # There's nowhere to send the error, so just log it.
            # TODO: Someone will want an error handler for this.
            logger.exception('Accept failed')
        else:
            if ssl:
                self._make_ssl_transport(
                    conn, protocol_factory(), ssl, None,
                    server_side=True, extra={'peername': addr}, server=server)
            else:
                self._make_socket_transport(
                    conn, protocol_factory(), extra={'peername': addr},
                    server=server)
        # It's now up to the protocol to handle the connection.

    def add_reader(self, fd, callback, *args):
        """Add a reader callback."""
        handle = events.make_handle(callback, args)
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
        handle = events.make_handle(callback, args)
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
        # That address better not require a lookup!  We're not calling
        # self.getaddrinfo() for you here.  But verifying this is
        # complicated; the socket module doesn't have a pattern for
        # IPv6 addresses (there are too many forms, apparently).
        fut = futures.Future(loop=self)
        self._sock_connect(fut, False, sock, address)
        return fut

    def _sock_connect(self, fut, registered, sock, address):
        # TODO: Use getaddrinfo() to look up the address, to avoid the
        # trap of hanging the entire event loop when the address
        # requires doing a DNS lookup.  (OTOH, the caller should
        # already have done this, so it would be nice if we could
        # easily tell whether the address needs looking up or not.  I
        # know how to do this for IPv4, but IPv6 addresses have many
        # syntaxes.)
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
                    raise OSError(err, 'Connect call failed')
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


class _SelectorTransport(transports.Transport):

    max_size = 256 * 1024  # Buffer size passed to recv().

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
        self._buffer = collections.deque()
        self._conn_lost = 0
        self._closing = False  # Set when close() called.
        if server is not None:
            server.attach(self)

    def abort(self):
        self._force_close(None)

    def close(self):
        if self._closing:
            return
        self._closing = True
        self._conn_lost += 1
        self._loop.remove_reader(self._sock_fd)
        if not self._buffer:
            self._loop.call_soon(self._call_connection_lost, None)

    def _fatal_error(self, exc):
        # should be called from exception handler only
        logger.exception('Fatal error for %s', self)
        self._force_close(exc)

    def _force_close(self, exc):
        if self._buffer:
            self._buffer.clear()
            self._loop.remove_writer(self._sock_fd)

        if self._closing:
            return

        self._closing = True
        self._conn_lost += 1
        self._loop.remove_reader(self._sock_fd)
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

    def pause(self):
        assert not self._closing, 'Cannot pause() when closing'
        assert not self._paused, 'Already paused'
        self._paused = True
        self._loop.remove_reader(self._sock_fd)

    def resume(self):
        assert self._paused, 'Not paused'
        self._paused = False
        if self._closing:
            return
        self._loop.add_reader(self._sock_fd, self._read_ready)

    def _read_ready(self):
        try:
            data = self._sock.recv(self.max_size)
        except (BlockingIOError, InterruptedError):
            pass
        except ConnectionResetError as exc:
            self._force_close(exc)
        except Exception as exc:
            self._fatal_error(exc)
        else:
            if data:
                self._protocol.data_received(data)
            else:
                keep_open = self._protocol.eof_received()
                if not keep_open:
                    self.close()

    def write(self, data):
        assert isinstance(data, bytes), repr(type(data))
        assert not self._eof, 'Cannot call write() after write_eof()'
        if not data:
            return

        if self._conn_lost:
            if self._conn_lost >= constants.LOG_THRESHOLD_FOR_CONNLOST_WRITES:
                logger.warning('socket.send() raised exception.')
            self._conn_lost += 1
            return

        if not self._buffer:
            # Attempt to send it right away first.
            try:
                n = self._sock.send(data)
            except (BlockingIOError, InterruptedError):
                n = 0
            except (BrokenPipeError, ConnectionResetError) as exc:
                self._force_close(exc)
                return
            except OSError as exc:
                self._fatal_error(exc)
                return
            else:
                data = data[n:]
                if not data:
                    return
            # Start async I/O.
            self._loop.add_writer(self._sock_fd, self._write_ready)

        self._buffer.append(data)

    def _write_ready(self):
        data = b''.join(self._buffer)
        assert data, 'Data should not be empty'

        self._buffer.clear()
        try:
            n = self._sock.send(data)
        except (BlockingIOError, InterruptedError):
            self._buffer.append(data)
        except (BrokenPipeError, ConnectionResetError) as exc:
            self._loop.remove_writer(self._sock_fd)
            self._force_close(exc)
        except Exception as exc:
            self._loop.remove_writer(self._sock_fd)
            self._fatal_error(exc)
        else:
            data = data[n:]
            if not data:
                self._loop.remove_writer(self._sock_fd)
                if self._closing:
                    self._call_connection_lost(None)
                elif self._eof:
                    self._sock.shutdown(socket.SHUT_WR)
                return

            self._buffer.append(data)  # Try again later.

    def write_eof(self):
        if self._eof:
            return
        self._eof = True
        if not self._buffer:
            self._sock.shutdown(socket.SHUT_WR)

    def can_write_eof(self):
        return True


class _SelectorSslTransport(_SelectorTransport):

    def __init__(self, loop, rawsock, protocol, sslcontext, waiter=None,
                 server_side=False, server_hostname=None,
                 extra=None, server=None):
        if server_side:
            assert isinstance(
                sslcontext, ssl.SSLContext), 'Must pass an SSLContext'
        else:
            # Client-side may pass ssl=True to use a default context.
            # The default is the same as used by urllib.
            if sslcontext is None:
                sslcontext = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
                sslcontext.options |= ssl.OP_NO_SSLv2
                sslcontext.set_default_verify_paths()
                sslcontext.verify_mode = ssl.CERT_REQUIRED
        wrap_kwargs = {
            'server_side': server_side,
            'do_handshake_on_connect': False,
        }
        if server_hostname is not None and not server_side and ssl.HAS_SNI:
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
            self._sock.close()
            if self._waiter is not None:
                self._waiter.set_exception(exc)
            return
        except BaseException as exc:
            self._sock.close()
            if self._waiter is not None:
                self._waiter.set_exception(exc)
            raise

        # Verify hostname if requested.
        peercert = self._sock.getpeercert()
        if (self._server_hostname is not None and
            self._sslcontext.verify_mode == ssl.CERT_REQUIRED):
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

        self._loop.remove_reader(self._sock_fd)
        self._loop.remove_writer(self._sock_fd)
        self._loop.add_reader(self._sock_fd, self._on_ready)
        self._loop.add_writer(self._sock_fd, self._on_ready)
        self._loop.call_soon(self._protocol.connection_made, self)
        if self._waiter is not None:
            self._loop.call_soon(self._waiter.set_result, None)

    def pause(self):
        # XXX This is a bit icky, given the comment at the top of
        # _on_ready().  Is it possible to evoke a deadlock?  I don't
        # know, although it doesn't look like it; write() will still
        # accept more data for the buffer and eventually the app will
        # call resume() again, and things will flow again.

        assert not self._closing, 'Cannot pause() when closing'
        assert not self._paused, 'Already paused'
        self._paused = True
        self._loop.remove_reader(self._sock_fd)

    def resume(self):
        assert self._paused, 'Not paused'
        self._paused = False
        if self._closing:
            return
        self._loop.add_reader(self._sock_fd, self._on_ready)

    def _on_ready(self):
        # Because of renegotiations (?), there's no difference between
        # readable and writable.  We just try both.  XXX This may be
        # incorrect; we probably need to keep state about what we
        # should do next.

        # First try reading.
        if not self._closing and not self._paused:
            try:
                data = self._sock.recv(self.max_size)
            except (BlockingIOError, InterruptedError,
                    ssl.SSLWantReadError, ssl.SSLWantWriteError):
                pass
            except ConnectionResetError as exc:
                self._force_close(exc)
            except Exception as exc:
                self._fatal_error(exc)
            else:
                if data:
                    self._protocol.data_received(data)
                else:
                    try:
                        self._protocol.eof_received()
                    finally:
                        self.close()

        # Now try writing, if there's anything to write.
        if self._buffer:
            data = b''.join(self._buffer)
            self._buffer.clear()
            try:
                n = self._sock.send(data)
            except (BlockingIOError, InterruptedError,
                    ssl.SSLWantReadError, ssl.SSLWantWriteError):
                n = 0
            except (BrokenPipeError, ConnectionResetError) as exc:
                self._loop.remove_writer(self._sock_fd)
                self._force_close(exc)
                return
            except Exception as exc:
                self._loop.remove_writer(self._sock_fd)
                self._fatal_error(exc)
                return

            if n < len(data):
                self._buffer.append(data[n:])

        if self._closing and not self._buffer:
            self._loop.remove_writer(self._sock_fd)
            self._call_connection_lost(None)

    def write(self, data):
        assert isinstance(data, bytes), repr(type(data))
        if not data:
            return

        if self._conn_lost:
            if self._conn_lost >= constants.LOG_THRESHOLD_FOR_CONNLOST_WRITES:
                logger.warning('socket.send() raised exception.')
            self._conn_lost += 1
            return

        self._buffer.append(data)
        # We could optimize, but the callback can do this for now.

    def can_write_eof(self):
        return False

    def close(self):
        if self._closing:
            return
        self._closing = True
        self._conn_lost += 1
        self._loop.remove_reader(self._sock_fd)


class _SelectorDatagramTransport(_SelectorTransport):

    def __init__(self, loop, sock, protocol, address=None, extra=None):
        super().__init__(loop, sock, protocol, extra)

        self._address = address
        self._loop.add_reader(self._sock_fd, self._read_ready)
        self._loop.call_soon(self._protocol.connection_made, self)

    def _read_ready(self):
        try:
            data, addr = self._sock.recvfrom(self.max_size)
        except (BlockingIOError, InterruptedError):
            pass
        except Exception as exc:
            self._fatal_error(exc)
        else:
            self._protocol.datagram_received(data, addr)

    def sendto(self, data, addr=None):
        assert isinstance(data, bytes), repr(type(data))
        if not data:
            return

        if self._address:
            assert addr in (None, self._address)

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
            except ConnectionRefusedError as exc:
                if self._address:
                    self._fatal_error(exc)
                return
            except (BlockingIOError, InterruptedError):
                self._loop.add_writer(self._sock_fd, self._sendto_ready)
            except Exception as exc:
                self._fatal_error(exc)
                return

        self._buffer.append((data, addr))

    def _sendto_ready(self):
        while self._buffer:
            data, addr = self._buffer.popleft()
            try:
                if self._address:
                    self._sock.send(data)
                else:
                    self._sock.sendto(data, addr)
            except ConnectionRefusedError as exc:
                if self._address:
                    self._fatal_error(exc)
                return
            except (BlockingIOError, InterruptedError):
                self._buffer.appendleft((data, addr))  # Try again later.
                break
            except Exception as exc:
                self._fatal_error(exc)
                return

        if not self._buffer:
            self._loop.remove_writer(self._sock_fd)
            if self._closing:
                self._call_connection_lost(None)

    def _force_close(self, exc):
        if self._address and isinstance(exc, ConnectionRefusedError):
            self._protocol.connection_refused(exc)

        super()._force_close(exc)
