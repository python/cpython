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

from _socket import *

import os, sys

if (sys.platform.lower().startswith("win")
    or (hasattr(os, 'uname') and os.uname()[0] == "BeOS")):

    # be sure this happens only once, even in the face of reload():
    try:
        _realsocketcall
    except NameError:
        _realsocketcall = socket

    def socket(family, type, proto=0):
        return _socketobject(_realsocketcall(family, type, proto))


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

class _socketobject:

    def __init__(self, sock):
        self._sock = sock

    def close(self):
        self._sock = 0

    def __del__(self):
        self.close()

    def accept(self):
        sock, addr = self._sock.accept()
        return _socketobject(sock), addr

    def dup(self):
        return _socketobject(self._sock)

    def makefile(self, mode='r', bufsize=-1):
        return _fileobject(self._sock, mode, bufsize)

    _s = "def %s(self, *args): return apply(self._sock.%s, args)\n\n"
    for _m in ('bind', 'connect', 'connect_ex', 'fileno', 'listen',
               'getpeername', 'getsockname',
               'getsockopt', 'setsockopt',
               'recv', 'recvfrom', 'send', 'sendto',
               'setblocking',
               'shutdown'):
        exec _s % (_m, _m)


class _fileobject:

    def __init__(self, sock, mode, bufsize):
        self._sock = sock
        self._mode = mode
        if bufsize < 0:
            bufsize = 512
        self._rbufsize = max(1, bufsize)
        self._wbufsize = bufsize
        self._wbuf = self._rbuf = ""

    def close(self):
        try:
            if self._sock:
                self.flush()
        finally:
            self._sock = 0

    def __del__(self):
        self.close()

    def flush(self):
        if self._wbuf:
            self._sock.send(self._wbuf)
            self._wbuf = ""

    def fileno(self):
        return self._sock.fileno()

    def write(self, data):
        self._wbuf = self._wbuf + data
        if self._wbufsize == 1:
            if '\n' in data:
                self.flush()
        else:
            if len(self._wbuf) >= self._wbufsize:
                self.flush()

    def writelines(self, list):
        filter(self._sock.send, list)
        self.flush()

    def read(self, n=-1):
        if n >= 0:
            k = len(self._rbuf)
            if n <= k:
                data = self._rbuf[:n]
                self._rbuf = self._rbuf[n:]
                return data
            n = n - k
            L = [self._rbuf]
            self._rbuf = ""
            while n > 0:
                new = self._sock.recv(max(n, self._rbufsize))
                if not new: break
                k = len(new)
                if k > n:
                    L.append(new[:n])
                    self._rbuf = new[n:]
                    break
                L.append(new)
                n = n - k
            return "".join(L)
        k = max(512, self._rbufsize)
        L = [self._rbuf]
        self._rbuf = ""
        while 1:
            new = self._sock.recv(k)
            if not new: break
            L.append(new)
            k = min(k*2, 1024**2)
        return "".join(L)

    def readline(self, limit=-1):
        data = ""
        i = self._rbuf.find('\n')
        while i < 0 and not (0 < limit <= len(self._rbuf)):
            new = self._sock.recv(self._rbufsize)
            if not new: break
            i = new.find('\n')
            if i >= 0: i = i + len(self._rbuf)
            self._rbuf = self._rbuf + new
        if i < 0: i = len(self._rbuf)
        else: i = i+1
        if 0 <= limit < len(self._rbuf): i = limit
        data, self._rbuf = self._rbuf[:i], self._rbuf[i:]
        return data

    def readlines(self, sizehint = 0):
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
