"""Mock socket module used by the smtpd and smtplib tests.
"""

# imported for _GLOBAL_DEFAULT_TIMEOUT
import socket as socket_module

# Mock socket module
_defaulttimeout = None
_reply_data = None

# This is used to queue up data to be read through socket.makefile, typically
# *before* the socket object is even created. It is intended to handle a single
# line which the socket will feed on recv() or makefile().
def reply_with(line):
    global _reply_data
    _reply_data = line


class MockFile:
    """Mock file object returned by MockSocket.makefile().
    """
    def __init__(self, lines):
        self.lines = lines
    def readline(self):
        return self.lines.pop(0) + b'\r\n'
    def close(self):
        pass


class MockSocket:
    """Mock socket object used by smtpd and smtplib tests.
    """
    def __init__(self):
        global _reply_data
        self.output = []
        self.lines = []
        if _reply_data:
            self.lines.append(_reply_data)
            _reply_data = None
        self.conn = None
        self.timeout = None

    def queue_recv(self, line):
        self.lines.append(line)

    def recv(self, bufsize, flags=None):
        data = self.lines.pop(0) + b'\r\n'
        return data

    def fileno(self):
        return 0

    def settimeout(self, timeout):
        if timeout is None:
            self.timeout = _defaulttimeout
        else:
            self.timeout = timeout

    def gettimeout(self):
        return self.timeout

    def setsockopt(self, level, optname, value):
        pass

    def getsockopt(self, level, optname, buflen=None):
        return 0

    def bind(self, address):
        pass

    def accept(self):
        self.conn = MockSocket()
        return self.conn, 'c'

    def getsockname(self):
        return ('0.0.0.0', 0)

    def setblocking(self, flag):
        pass

    def listen(self, backlog):
        pass

    def makefile(self, mode='r', bufsize=-1):
        handle = MockFile(self.lines)
        return handle

    def sendall(self, buffer, flags=None):
        self.last = data
        self.output.append(data)
        return len(data)

    def send(self, data, flags=None):
        self.last = data
        self.output.append(data)
        return len(data)

    def getpeername(self):
        return 'peer'

    def close(self):
        pass


def socket(family=None, type=None, proto=None):
    return MockSocket()


def create_connection(address, timeout=socket_module._GLOBAL_DEFAULT_TIMEOUT,
                      source_address=None):
    try:
        int_port = int(address[1])
    except ValueError:
        raise error
    ms = MockSocket()
    if timeout is socket_module._GLOBAL_DEFAULT_TIMEOUT:
        timeout = getdefaulttimeout()
    ms.settimeout(timeout)
    return ms


def setdefaulttimeout(timeout):
    global _defaulttimeout
    _defaulttimeout = timeout


def getdefaulttimeout():
    return _defaulttimeout


def getfqdn():
    return ""


def gethostname():
    pass


def gethostbyname(name):
    return ""


class gaierror(Exception):
    pass


class error(Exception):
    pass


# Constants
AF_INET = None
SOCK_STREAM = None
SOL_SOCKET = None
SO_REUSEADDR = None
