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

_have_ssl = False
try:
    import _ssl
    from _ssl import *
    _have_ssl = True
except ImportError:
    pass

import os, sys

try:
    from errno import EBADF
except ImportError:
    EBADF = 9

__all__ = ["getfqdn"]
__all__.extend(os._get_exports_list(_socket))
if _have_ssl:
    __all__.extend(os._get_exports_list(_ssl))

_realsocket = socket
if _have_ssl:
    _realssl = ssl
    def ssl(sock, keyfile=None, certfile=None):
        if hasattr(sock, "_sock"):
            sock = sock._sock
        return _realssl(sock, keyfile, certfile)

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


_socketmethods = (
    'bind', 'connect', 'connect_ex', 'fileno', 'listen',
    'getpeername', 'getsockname', 'getsockopt', 'setsockopt',
    'sendall', 'setblocking',
    'settimeout', 'gettimeout', 'shutdown')

if sys.platform == "riscos":
    _socketmethods = _socketmethods + ('sleeptaskw',)

# All the method names that must be delegated to either the real socket
# object or the _closedsocket object.
_delegate_methods = ("recv", "recvfrom", "recv_into", "recvfrom_into",
                     "send", "sendto")

class _closedsocket(object):
    __slots__ = []
    def _dummy(*args):
        raise error(EBADF, 'Bad file descriptor')
    # All _delegate_methods must also be initialized here.
    send = recv = recv_into = sendto = recvfrom = recvfrom_into = _dummy
    __getattr__ = _dummy

class _socketobject(object):

    __doc__ = _realsocket.__doc__

    __slots__ = ["_sock", "__weakref__"] + list(_delegate_methods)

    def __init__(self, family=AF_INET, type=SOCK_STREAM, proto=0, _sock=None):
        if _sock is None:
            _sock = _realsocket(family, type, proto)
        self._sock = _sock
        for method in _delegate_methods:
            setattr(self, method, getattr(_sock, method))

    def close(self):
        self._sock = _closedsocket()
        dummy = self._sock._dummy
        for method in _delegate_methods:
            setattr(self, method, dummy)
    close.__doc__ = _realsocket.close.__doc__

    def accept(self):
        sock, addr = self._sock.accept()
        return _socketobject(_sock=sock), addr
    accept.__doc__ = _realsocket.accept.__doc__

    def dup(self):
        """dup() -> socket object

        Return a new socket object connected to the same system resource."""
        return _socketobject(_sock=self._sock)

    def makefile(self, mode='r', bufsize=-1):
        """makefile([mode[, bufsize]]) -> file object

        Return a regular file object corresponding to the socket.  The mode
        and bufsize arguments are as for the built-in open() function."""
        return _fileobject(self._sock, mode, bufsize)

    family = property(lambda self: self._sock.family, doc="the socket family")
    type = property(lambda self: self._sock.type, doc="the socket type")
    proto = property(lambda self: self._sock.proto, doc="the socket protocol")

    _s = ("def %s(self, *args): return self._sock.%s(*args)\n\n"
          "%s.__doc__ = _realsocket.%s.__doc__\n")
    for _m in _socketmethods:
        exec _s % (_m, _m, _m, _m)
    del _m, _s

socket = SocketType = _socketobject

class _fileobject(object):
    """Faux file object attached to a socket object."""

    default_bufsize = 8192
    name = "<socket>"

    __slots__ = ["mode", "bufsize", "softspace",
                 # "closed" is a property, see below
                 "_sock", "_rbufsize", "_wbufsize", "_rbuf", "_wbuf",
                 "_close"]

    def __init__(self, sock, mode='rb', bufsize=-1, close=False):
        self._sock = sock
        self.mode = mode # Not actually used in this version
        if bufsize < 0:
            bufsize = self.default_bufsize
        self.bufsize = bufsize
        self.softspace = False
        if bufsize == 0:
            self._rbufsize = 1
        elif bufsize == 1:
            self._rbufsize = self.default_bufsize
        else:
            self._rbufsize = bufsize
        self._wbufsize = bufsize
        self._rbuf = "" # A string
        self._wbuf = [] # A list of strings
        self._close = close

    def _getclosed(self):
        return self._sock is None
    closed = property(_getclosed, doc="True if the file is closed")

    def close(self):
        try:
            if self._sock:
                self.flush()
        finally:
            if self._close:
                self._sock.close()
            self._sock = None

    def __del__(self):
        try:
            self.close()
        except:
            # close() may fail if __init__ didn't complete
            pass

    def flush(self):
        if self._wbuf:
            buffer = "".join(self._wbuf)
            self._wbuf = []
            self._sock.sendall(buffer)

    def fileno(self):
        return self._sock.fileno()

    def write(self, data):
        data = str(data) # XXX Should really reject non-string non-buffers
        if not data:
            return
        self._wbuf.append(data)
        if (self._wbufsize == 0 or
            self._wbufsize == 1 and '\n' in data or
            self._get_wbuf_len() >= self._wbufsize):
            self.flush()

    def writelines(self, list):
        # XXX We could do better here for very long lists
        # XXX Should really reject non-string non-buffers
        self._wbuf.extend(filter(None, map(str, list)))
        if (self._wbufsize <= 1 or
            self._get_wbuf_len() >= self._wbufsize):
            self.flush()

    def _get_wbuf_len(self):
        buf_len = 0
        for x in self._wbuf:
            buf_len += len(x)
        return buf_len

    def read(self, size=-1):
        data = self._rbuf
        if size < 0:
            # Read until EOF
            buffers = []
            if data:
                buffers.append(data)
            self._rbuf = ""
            if self._rbufsize <= 1:
                recv_size = self.default_bufsize
            else:
                recv_size = self._rbufsize
            while True:
                data = self._sock.recv(recv_size)
                if not data:
                    break
                buffers.append(data)
            return "".join(buffers)
        else:
            # Read until size bytes or EOF seen, whichever comes first
            buf_len = len(data)
            if buf_len >= size:
                self._rbuf = data[size:]
                return data[:size]
            buffers = []
            if data:
                buffers.append(data)
            self._rbuf = ""
            while True:
                left = size - buf_len
                recv_size = max(self._rbufsize, left)
                data = self._sock.recv(recv_size)
                if not data:
                    break
                buffers.append(data)
                n = len(data)
                if n >= left:
                    self._rbuf = data[left:]
                    buffers[-1] = data[:left]
                    break
                buf_len += n
            return "".join(buffers)

    def readline(self, size=-1):
        data = self._rbuf
        if size < 0:
            # Read until \n or EOF, whichever comes first
            if self._rbufsize <= 1:
                # Speed up unbuffered case
                assert data == ""
                buffers = []
                recv = self._sock.recv
                while data != "\n":
                    data = recv(1)
                    if not data:
                        break
                    buffers.append(data)
                return "".join(buffers)
            nl = data.find('\n')
            if nl >= 0:
                nl += 1
                self._rbuf = data[nl:]
                return data[:nl]
            buffers = []
            if data:
                buffers.append(data)
            self._rbuf = ""
            while True:
                data = self._sock.recv(self._rbufsize)
                if not data:
                    break
                buffers.append(data)
                nl = data.find('\n')
                if nl >= 0:
                    nl += 1
                    self._rbuf = data[nl:]
                    buffers[-1] = data[:nl]
                    break
            return "".join(buffers)
        else:
            # Read until size bytes or \n or EOF seen, whichever comes first
            nl = data.find('\n', 0, size)
            if nl >= 0:
                nl += 1
                self._rbuf = data[nl:]
                return data[:nl]
            buf_len = len(data)
            if buf_len >= size:
                self._rbuf = data[size:]
                return data[:size]
            buffers = []
            if data:
                buffers.append(data)
            self._rbuf = ""
            while True:
                data = self._sock.recv(self._rbufsize)
                if not data:
                    break
                buffers.append(data)
                left = size - buf_len
                nl = data.find('\n', 0, left)
                if nl >= 0:
                    nl += 1
                    self._rbuf = data[nl:]
                    buffers[-1] = data[:nl]
                    break
                n = len(data)
                if n >= left:
                    self._rbuf = data[left:]
                    buffers[-1] = data[:left]
                    break
                buf_len += n
            return "".join(buffers)

    def readlines(self, sizehint=0):
        total = 0
        list = []
        while True:
            line = self.readline()
            if not line:
                break
            list.append(line)
            total += len(line)
            if sizehint and total >= sizehint:
                break
        return list

    # Iterator protocols

    def __iter__(self):
        return self

    def next(self):
        line = self.readline()
        if not line:
            raise StopIteration
        return line
