"""Control runtime for the sampling profiler."""

import os
import selectors
import socket
import warnings

from .errors import ControlError, ControlURIError


class ProfilerControl:
    def __init__(self):
        self.enabled = True
        self.running = True


def parse_control_uri(uri, *, allowed_schemes=("unix",)):
    if ":" not in uri:
        raise ControlURIError("control URI must include a scheme")

    scheme, path = uri.split(":", 1)
    if scheme not in allowed_schemes:
        expected = ", ".join(allowed_schemes)
        raise ControlURIError(
            f"unsupported control URI scheme {scheme!r}; "
            f"expected one of: {expected}"
        )
    if not path:
        raise ControlURIError("control URI path must not be empty")
    return scheme, path


_MAX_OUTBUF_BYTES = 64 * 1024
_MAX_INBUF_BYTES = 4 * 1024
_MAX_CONNECTIONS = 8
_SOCKET_PERMISSIONS = 0o600


class _Connection:
    def __init__(self, sock):
        self.sock = sock
        self.inbuf = bytearray()
        self.outbuf = bytearray()
        self.close_after_write = False


class ControlServer:
    def __init__(self, uri):
        self.uri = uri
        self.control = ProfilerControl()
        _, self._path = parse_control_uri(uri)
        self.selector = selectors.DefaultSelector()
        self._connections = set()
        self._listener = None
        self._created_stat = None

    def start(self):
        self._listener = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            self._listener.bind(self._path)
            os.chmod(self._path, _SOCKET_PERMISSIONS)
            self._created_stat = os.lstat(self._path)
            self._listener.listen(socket.SOMAXCONN)
            self._listener.setblocking(False)
            self.selector.register(self._listener, selectors.EVENT_READ, None)
        except OSError as exc:
            self._close_listener()
            raise ControlError(
                f"failed to start control socket {self._path!r}: {exc}"
            ) from exc

    def stop(self):
        for conn in list(self._connections):
            self._close_connection(conn)
        self.selector.close()
        self._close_listener()

    def _close_listener(self):
        listener, self._listener = self._listener, None
        if listener is not None:
            listener.close()

        created_stat, self._created_stat = self._created_stat, None
        if created_stat is None:
            return
        try:
            current_stat = os.lstat(self._path)
            if (current_stat.st_ino, current_stat.st_dev) == (
                created_stat.st_ino,
                created_stat.st_dev,
            ):
                os.unlink(self._path)
        except OSError:
            pass

    def poll(self, timeout):
        try:
            ready = self.selector.select(timeout)
        except OSError as exc:
            warnings.warn(
                f"control selector.select() failed: {exc}",
                RuntimeWarning,
                stacklevel=2,
            )
            return

        for key, events in ready:
            if key.fileobj is self._listener:
                self._accept_connection()
            else:
                self._handle_connection(key.data, events)

    def _accept_connection(self):
        try:
            sock, _addr = self._listener.accept()
        except BlockingIOError:
            return
        except OSError as exc:
            warnings.warn(
                f"control accept failed: {exc}",
                RuntimeWarning,
                stacklevel=2,
            )
            return

        if len(self._connections) >= _MAX_CONNECTIONS:
            sock.close()
            return

        try:
            sock.setblocking(False)
            conn = _Connection(sock)
            self.selector.register(sock, selectors.EVENT_READ, conn)
        except OSError:
            sock.close()
            return

        self._connections.add(conn)

    def _handle_connection(self, conn, events):
        if events & selectors.EVENT_READ:
            self._read_connection(conn)
        if conn in self._connections and events & selectors.EVENT_WRITE:
            self._flush_connection(conn)

    def _read_connection(self, conn):
        try:
            chunk = conn.sock.recv(_MAX_INBUF_BYTES)
        except (BlockingIOError, InterruptedError):
            return
        except OSError:
            self._close_connection(conn)
            return

        if not chunk:
            self._close_connection(conn)
            return

        conn.inbuf.extend(chunk)
        if len(conn.inbuf) > _MAX_INBUF_BYTES:
            self._close_connection(conn)
            return

        while True:
            newline = conn.inbuf.find(b"\n")
            if newline == -1:
                break
            raw = conn.inbuf.take_bytes(newline + 1)
            line = raw[:-1].decode("utf-8", "replace").strip()
            self._dispatch(conn, line)
            if conn not in self._connections or conn.close_after_write:
                break

        if conn in self._connections:
            self._flush_connection(conn)

    def _dispatch(self, conn, command):
        match command:
            case "enable":
                self.control.enabled = True
                reply = "ok\n"
            case "disable":
                self.control.enabled = False
                reply = "ok\n"
            case "ping":
                reply = "ok\n"
            case "status":
                reply = f"ok enabled={self.control.enabled}\n"
            case "quit":
                self.control.running = False
                conn.close_after_write = True
                reply = "ok\n"
            case _:
                reply = "err unknown_command\n"

        conn.outbuf.extend(reply.encode("ascii"))
        if len(conn.outbuf) > _MAX_OUTBUF_BYTES:
            self._close_connection(conn)

    def _flush_connection(self, conn):
        while conn.outbuf:
            try:
                sent = conn.sock.send(conn.outbuf)
            except (BlockingIOError, InterruptedError):
                break
            except OSError:
                self._close_connection(conn)
                return

            if sent == 0:
                self._close_connection(conn)
                return

            del conn.outbuf[:sent]

        if not conn.outbuf and conn.close_after_write:
            self._close_connection(conn)
            return

        events = selectors.EVENT_READ
        if conn.outbuf:
            events |= selectors.EVENT_WRITE
        try:
            self.selector.modify(conn.sock, events, conn)
        except (KeyError, OSError):
            self._close_connection(conn)

    def _close_connection(self, conn):
        if conn not in self._connections:
            return
        self._connections.discard(conn)

        try:
            self.selector.unregister(conn.sock)
        except (KeyError, OSError):
            pass

        conn.sock.close()
