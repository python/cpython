"Socket wrapper for BeOS, which does not support dup()."

# (And hence, fromfd() and makefile() are unimplemented in C....)

# XXX Living dangerously here -- close() is implemented by deleting a
# reference.  Thus we rely on the real _socket module to close on
# deallocation, and also hope that nobody keeps a reference to our _sock
# member.



try:
    from _socket import *
except ImportError:
    from socket import *

_realsocketcall = socket


def socket(family, type, proto=0):
    return _socketobject(_realsocketcall(family, type, proto))


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
    for _m in ('bind', 'connect', 'fileno', 'listen',
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
            while len(self._rbuf) < n:
                new = self._sock.recv(self._rbufsize)
                if not new: break
                self._rbuf = self._rbuf + new
            data, self._rbuf = self._rbuf[:n], self._rbuf[n:]
            return data
        while 1:
            new = self._sock.recv(self._rbufsize)
            if not new: break
            self._rbuf = self._rbuf + new
        data, self._rbuf = self._rbuf, ""
        return data

    def readline(self):
        import string
        data = ""
        i = string.find(self._rbuf, '\n')
        while i < 0:
            new = self._sock.recv(self._rbufsize)
            if not new: break
            i = string.find(new, '\n')
            if i >= 0: i = i + len(self._rbuf)
            self._rbuf = self._rbuf + new
        if i < 0: i = len(self._rbuf)
        else: i = i+1
        data, self._rbuf = self._rbuf[:i], self._rbuf[i:]
        return data

    def readlines(self):
        list = []
        while 1:
            line = self.readline()
            if not line: break
            list.append(line)
        return list
