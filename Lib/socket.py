# Wrapper module for _socket, providing some additional facilities
# implemented in Python.

"""\
This module provides socket operations and some related functions.
On Unix, it supports IP (Internet Protocol) and Unix domain sockets.
On other systems, it only supports IP. Functions specific for a
socket are available as methods of the socket object.

Functions:

socket() -- create a new socket object
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

 [*] not available on all platforms!

Special objects:

SocketType -- type object for socket objects
error -- exception raised for I/O errors

Integer constants:

AF_INET, AF_UNIX -- socket domains (first argument to socket() call)
SOCK_STREAM, SOCK_DGRAM, SOCK_RAW -- socket types (second argument)

Many other constants may be defined; these may be used in calls to
the setsockopt() and getsockopt() methods.
"""

import _socket
from _socket import *

SSL_EXISTS = 1
try:
    import _ssl
    from _ssl import *
except ImportError:
    SSL_EXISTS = 0

import os, sys

__all__ = ["getfqdn"]
__all__.extend(os._get_exports_list(_socket))
# XXX shouldn't there be something similar to the above for _ssl exports?

if (sys.platform.lower().startswith("win")
    or (hasattr(os, 'uname') and os.uname()[0] == "BeOS")
    or sys.platform=="riscos"):

    _realsocketcall = _socket.socket

    def socket(family, type, proto=0):
        return _socketobject(_realsocketcall(family, type, proto))

    if SSL_EXISTS:
        _realsslcall = _ssl.ssl
        def ssl(sock, keyfile=None, certfile=None):
            if hasattr(sock, "_sock"):
                sock = sock._sock
            return _realsslcall(sock, keyfile, certfile)

del _socket
if SSL_EXISTS:
    del _ssl
del SSL_EXISTS

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
del os, sys


def getfqdn(name=''):
    """Get fully qualified domain name from name.

    An empty argument is interpreted as meaning the local host.

    First the hostname returned by gethostbyaddr() is checked, then
    possibly existing aliases. In case no FQDN is available, hostname
    is returned.
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


#
# These classes are used by the socket() defined on Windows and BeOS
# platforms to provide a best-effort implementation of the cleanup
# semantics needed when sockets can't be dup()ed.
#
# These are not actually used on other platforms.
#

_socketmethods = (
    'bind', 'connect', 'connect_ex', 'fileno', 'listen',
    'getpeername', 'getsockname', 'getsockopt', 'setsockopt',
    'recv', 'recvfrom', 'send', 'sendall', 'sendto', 'setblocking',
    'settimeout', 'gettimeout', 'shutdown')

class _socketobject:

    class _closedsocket:
        def __getattr__(self, name):
            raise error(9, 'Bad file descriptor')

    def __init__(self, sock):
        self._sock = sock

    def close(self):
        # Avoid referencing globals here
        self._sock = self.__class__._closedsocket()

    def __del__(self):
        self.close()

    def accept(self):
        sock, addr = self._sock.accept()
        return _socketobject(sock), addr

    def dup(self):
        return _socketobject(self._sock)

    def makefile(self, mode='r', bufsize=-1):
        return _fileobject(self._sock, mode, bufsize)

    _s = "def %s(self, *args): return self._sock.%s(*args)\n\n"
    for _m in _socketmethods:
        exec _s % (_m, _m)


class _fileobject:
    """Implements a file object on top of a regular socket object."""

    def __init__(self, sock, mode='rb', bufsize=8192):
        self._sock = sock
        self._mode = mode
        if bufsize <= 0:
            bufsize = 512
        self._rbufsize = bufsize
        self._wbufsize = bufsize
        self._rbuf = [ ]
        self._wbuf = [ ]

    def close(self):
        try:
            if self._sock:
                self.flush()
        finally:
            self._sock = None

    def __del__(self):
        self.close()

    def flush(self):
        if self._wbuf:
            buffer = ''.join(self._wbuf)
            self._sock.sendall(buffer)
            self._wbuf = [ ]

    def fileno (self):
        return self._sock.fileno()

    def write(self, data):
        self._wbuf.append (data)
        # A _wbufsize of 1 means we're doing unbuffered IO.
        # Flush accordingly.
        if self._wbufsize == 1:
            if '\n' in data:
                self.flush ()
        elif self.__get_wbuf_len() >= self._wbufsize:
            self.flush()

    def writelines(self, list):
        filter(self._sock.sendall, list)
        self.flush()

    def __get_wbuf_len (self):
        buf_len = 0
        for i in [len(x) for x in self._wbuf]:
            buf_len += i
        return buf_len

    def __get_rbuf_len(self):
        buf_len = 0
        for i in [len(x) for x in self._rbuf]:
            buf_len += i
        return buf_len

    def read(self, size=-1):
        buf_len = self.__get_rbuf_len()
        while size < 0 or buf_len < size:
            recv_size = max(self._rbufsize, size - buf_len)
            data = self._sock.recv(recv_size)
            if not data:
                break
            buf_len += len(data)
            self._rbuf.append(data)
        data = ''.join(self._rbuf)
        # Clear the rbuf at the end so we're not affected by
        # an exception during a recv
        self._rbuf = [ ]
        if buf_len > size and size >= 0:
           self._rbuf.append(data[size:])
           data = data[:size]
        return data

    def readline(self, size=-1):
        index = -1
        buf_len = self.__get_rbuf_len()
        if len (self._rbuf):
            index = min([x.find('\n') for x in self._rbuf])
        while index < 0 and (size < 0 or buf_len < size):
            recv_size = max(self._rbufsize, size - buf_len)
            data = self._sock.recv(recv_size)
            if not data:
                break
            buf_len += len(data)
            self._rbuf.append(data)
            index = data.find('\n')
        data = ''.join(self._rbuf)
        self._rbuf = [ ]
        index = data.find('\n')
        if index >= 0:
            index += 1
        elif buf_len > size:
            index = size
        else:
            index = buf_len
        self._rbuf.append(data[index:])
        data = data[:index]
        return data

    def readlines(self, sizehint=0):
        total = 0
        list = []
        while 1:
            line = self.readline()
            if not line: break
            list.append(line)
            total += len(line)
            if sizehint and total >= sizehint:
                break
        return list
