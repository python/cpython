# Wrapper module for _socket, providing some additional facilities
# implemented in Python.

"""\
This module provides socket operations and some related functions.
On Unix, it supports IP (Internet Protocol) and Unix domain sockets.
On other systems, it only supports IP. Functions specific for a
socket are available as methods of the socket object.

Functions:

socket() -- create a new socket object
socketpair() -- create a pair of new socket objects [*]
fromfd() -- create a socket object from an open file descriptor [*]
gethostname() -- return the current hostname
gethostbyname() -- map a hostname to its IP number
gethostbyaddr() -- map an IP number or hostname to DNS info
getservbyname() -- map a service name and a protocol name to a port number
getprotobyname() -- mape a protocol name (e.g. 'tcp') to a number
ntohs(), ntohl() -- convert 16, 32 bit int from network to host byte order
htons(), htonl() -- convert 16, 32 bit int from host to network byte order
inet_aton() -- convert IP addr string (123.45.67.89) to 32-bit packed format
inet_ntoa() -- convert 32-bit packed format IP to string (123.45.67.89)
ssl() -- secure socket layer support (only available if configured)
socket.getdefaulttimeout() -- get the default timeout value
socket.setdefaulttimeout() -- set the default timeout value
create_connection() -- connects to an address, with an optional timeout

 [*] not available on all platforms!

Special objects:

SocketType -- type object for socket objects
error -- exception raised for I/O errors
has_ipv6 -- boolean value indicating if IPv6 is supported

Integer constants:

AF_INET, AF_UNIX -- socket domains (first argument to socket() call)
SOCK_STREAM, SOCK_DGRAM, SOCK_RAW -- socket types (second argument)

Many other constants may be defined; these may be used in calls to
the setsockopt() and getsockopt() methods.
"""

import _socket
from _socket import *

try:
    import _ssl
    import ssl as _realssl
except ImportError:
    # no SSL support
    pass
else:
    def ssl(sock, keyfile=None, certfile=None):
        # we do an internal import here because the ssl
        # module imports the socket module
        warnings.warn("socket.ssl() is deprecated.  Use ssl.wrap_socket() instead.",
                      DeprecationWarning, stacklevel=2)
        return _realssl.sslwrap_simple(sock, keyfile, certfile)

    # we need to import the same constants we used to...
    from _ssl import SSLError as sslerror
    from _ssl import \
         RAND_add, \
         RAND_egd, \
         RAND_status, \
         SSL_ERROR_ZERO_RETURN, \
         SSL_ERROR_WANT_READ, \
         SSL_ERROR_WANT_WRITE, \
         SSL_ERROR_WANT_X509_LOOKUP, \
         SSL_ERROR_SYSCALL, \
         SSL_ERROR_SSL, \
         SSL_ERROR_WANT_CONNECT, \
         SSL_ERROR_EOF, \
         SSL_ERROR_INVALID_ERROR_CODE

import os, sys, io

try:
    from errno import EBADF
except ImportError:
    EBADF = 9

__all__ = ["getfqdn"]
__all__.extend(os._get_exports_list(_socket))


_realsocket = socket

# WSA error codes
if sys.platform.lower().startswith("win"):
    errorTab = {}
    errorTab[10004] = "The operation was interrupted."
    errorTab[10009] = "A bad file handle was passed."
    errorTab[10013] = "Permission denied."
    errorTab[10014] = "A fault occurred on the network??" # WSAEFAULT
    errorTab[10022] = "An invalid operation was attempted."
    errorTab[10035] = "The socket operation would block"
    errorTab[10036] = "A blocking operation is already in progress."
    errorTab[10048] = "The network address is in use."
    errorTab[10054] = "The connection has been reset."
    errorTab[10058] = "The network has been shut down."
    errorTab[10060] = "The operation timed out."
    errorTab[10061] = "Connection refused."
    errorTab[10063] = "The name is too long."
    errorTab[10064] = "The host is down."
    errorTab[10065] = "The host is unreachable."
    __all__.append("errorTab")


# True if os.dup() can duplicate socket descriptors.
# (On Windows at least, os.dup only works on files)
_can_dup_socket = hasattr(_socket.socket, "dup")

if _can_dup_socket:
    def fromfd(fd, family=AF_INET, type=SOCK_STREAM, proto=0):
        nfd = os.dup(fd)
        return socket(family, type, proto, fileno=nfd)

class SocketCloser:

    """Helper to manage socket close() logic for makefile().

    The OS socket should not be closed until the socket and all
    of its makefile-children are closed.  If the refcount is zero
    when socket.close() is called, this is easy: Just close the
    socket.  If the refcount is non-zero when socket.close() is
    called, then the real close should not occur until the last
    makefile-child is closed.
    """

    def __init__(self, sock):
        self._sock = sock
        self._makefile_refs = 0
        # Test whether the socket is open.
        try:
            sock.fileno()
            self._socket_open = True
        except error:
            self._socket_open = False

    def socket_close(self):
        self._socket_open = False
        self.close()

    def makefile_open(self):
        self._makefile_refs += 1

    def makefile_close(self):
        self._makefile_refs -= 1
        self.close()

    def close(self):
        if not (self._socket_open or self._makefile_refs):
            self._sock._real_close()


class socket(_socket.socket):

    """A subclass of _socket.socket adding the makefile() method."""

    __slots__ = ["__weakref__", "_closer"]
    if not _can_dup_socket:
        __slots__.append("_base")

    def __init__(self, family=AF_INET, type=SOCK_STREAM, proto=0, fileno=None):
        if fileno is None:
            _socket.socket.__init__(self, family, type, proto)
        else:
            _socket.socket.__init__(self, family, type, proto, fileno)
        # Defer creating a SocketCloser until makefile() is actually called.
        self._closer = None

    def __repr__(self):
        """Wrap __repr__() to reveal the real class name."""
        s = _socket.socket.__repr__(self)
        if s.startswith("<socket object"):
            s = "<%s.%s%s" % (self.__class__.__module__,
                              self.__class__.__name__,
                              s[7:])
        return s

    def accept(self):
        """Wrap accept() to give the connection the right type."""
        conn, addr = _socket.socket.accept(self)
        fd = conn.fileno()
        nfd = fd
        if _can_dup_socket:
            nfd = os.dup(fd)
        wrapper = socket(self.family, self.type, self.proto, fileno=nfd)
        if fd == nfd:
            wrapper._base = conn  # Keep the base alive
        else:
            conn.close()
        return wrapper, addr

    def makefile(self, mode="r", buffering=None, *,
                 encoding=None, newline=None):
        """Return an I/O stream connected to the socket.

        The arguments are as for io.open() after the filename,
        except the only mode characters supported are 'r', 'w' and 'b'.
        The semantics are similar too.  (XXX refactor to share code?)
        """
        for c in mode:
            if c not in {"r", "w", "b"}:
                raise ValueError("invalid mode %r (only r, w, b allowed)")
        writing = "w" in mode
        reading = "r" in mode or not writing
        assert reading or writing
        binary = "b" in mode
        rawmode = ""
        if reading:
            rawmode += "r"
        if writing:
            rawmode += "w"
        if self._closer is None:
            self._closer = SocketCloser(self)
        raw = SocketIO(self, rawmode, self._closer)
        if buffering is None:
            buffering = -1
        if buffering < 0:
            buffering = io.DEFAULT_BUFFER_SIZE
        if buffering == 0:
            if not binary:
                raise ValueError("unbuffered streams must be binary")
            raw.name = self.fileno()
            raw.mode = mode
            return raw
        if reading and writing:
            buffer = io.BufferedRWPair(raw, raw, buffering)
        elif reading:
            buffer = io.BufferedReader(raw, buffering)
        else:
            assert writing
            buffer = io.BufferedWriter(raw, buffering)
        if binary:
            buffer.name = self.fileno()
            buffer.mode = mode
            return buffer
        text = io.TextIOWrapper(buffer, encoding, newline)
        text.name = self.fileno()
        text.mode = mode
        return text

    def close(self):
        if self._closer is None:
            self._real_close()
        else:
            self._closer.socket_close()

    # _real_close calls close on the _socket.socket base class.

    if not _can_dup_socket:
        def _real_close(self):
            _socket.socket.close(self)
            base = getattr(self, "_base", None)
            if base is not None:
                self._base = None
                base.close()
    else:
        def _real_close(self):
            _socket.socket.close(self)


class SocketIO(io.RawIOBase):

    """Raw I/O implementation for stream sockets.

    This class supports the makefile() method on sockets.  It provides
    the raw I/O interface on top of a socket object.
    """

    # XXX More docs

    def __init__(self, sock, mode, closer):
        if mode not in ("r", "w", "rw"):
            raise ValueError("invalid mode: %r" % mode)
        io.RawIOBase.__init__(self)
        self._sock = sock
        self._mode = mode
        self._closer = closer
        self._reading = "r" in mode
        self._writing = "w" in mode
        closer.makefile_open()

    def readinto(self, b):
        self._checkClosed()
        self._checkReadable()
        return self._sock.recv_into(b)

    def write(self, b):
        self._checkClosed()
        self._checkWritable()
        return self._sock.send(b)

    def readable(self):
        return self._reading and not self.closed

    def writable(self):
        return self._writing and not self.closed

    def fileno(self):
        return self._sock.fileno()

    def close(self):
        if self.closed:
            return
        self._closer.makefile_close()
        io.RawIOBase.close(self)


def getfqdn(name=''):
    """Get fully qualified domain name from name.

    An empty argument is interpreted as meaning the local host.

    First the hostname returned by gethostbyaddr() is checked, then
    possibly existing aliases. In case no FQDN is available, hostname
    from gethostname() is returned.
    """
    name = name.strip()
    if not name or name == '0.0.0.0':
        name = gethostname()
    try:
        hostname, aliases, ipaddrs = gethostbyaddr(name)
    except error:
        pass
    else:
        aliases.insert(0, hostname)
        for name in aliases:
            if '.' in name:
                break
        else:
            name = hostname
    return name


def create_connection(address, timeout=None):
    """Connect to address (host, port) with an optional timeout.

    Provides access to socketobject timeout for higher-level
    protocols.  Passing a timeout will set the timeout on the
    socket instance (if not present, or passed as None, the
    default global timeout setting will be used).
    """

    msg = "getaddrinfo returns an empty list"
    host, port = address
    for res in getaddrinfo(host, port, 0, SOCK_STREAM):
        af, socktype, proto, canonname, sa = res
        sock = None
        try:
            sock = socket(af, socktype, proto)
            if timeout is not None:
                sock.settimeout(timeout)
            sock.connect(sa)
            return sock

        except error as err:
            msg = err
            if sock is not None:
                sock.close()

    raise error(msg)
