"""Async sockets, build on top of Sam Rushing's excellent async library"""

import asyncore
import socket
from socket import AF_INET, SOCK_STREAM
import string
import cStringIO
import mimetools
import httplib


__version__ = "0.9"
__author__ = "jvr"

BUFSIZE = 512

VERBOSE = 1

class Server(asyncore.dispatcher):

    """Generic asynchronous server class"""

    def __init__(self, port, handler_class, backlog=1, host=""):
        """arguments:
        - port: the port to listen to
        - handler_class: class to handle requests
        - backlog: backlog queue size (optional) (don't fully understand, see socket docs)
        - host: host name (optional: can be empty to use default host name)
        """
        if VERBOSE:
            print "Starting", self.__class__.__name__
        self.handler_class = handler_class
        asyncore.dispatcher.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.bind((host, port))
        self.listen(backlog)

    def handle_accept(self):
        conn, addr = self.accept()
        if VERBOSE:
            print 'Incoming Connection from %s:%d' % addr
        self.handler_class(conn)


class ProxyServer(Server):

    """Generic proxy server class"""

    def __init__(self, port, handler_class, proxyaddr=None, closepartners=0):
        """arguments:
        - port: the port to listen to
        - handler_class: proxy class to handle requests
        - proxyaddr: a tuple containing the address and
          port of a remote host to connect to (optional)
        - closepartners: boolean, specifies whether we should close
          all proxy connections or not (optional). http seems to *not*
          want this, but telnet does...
        """
        Server.__init__(self, port, handler_class, 1, "")
        self.proxyaddr = proxyaddr
        self.closepartners = closepartners

    def handle_accept(self):
        conn, addr = self.accept()
        if VERBOSE:
            print 'Incoming Connection from %s:%d' % addr
        self.handler_class(conn, self.proxyaddr, closepartner=self.closepartners)


class Connection(asyncore.dispatcher):

    """Generic connection class"""

    def __init__(self, sock_or_address=None, readfunc=None, terminator=None):
        """arguments:
        - sock_or_address: either a socket, or a tuple containing the name
        and port number of a remote host
        - readfunc: callback function (optional). Will be called whenever
          there is some data available, or when an appropraite terminator
          is found.
        - terminator: string which specifies when a read is complete (optional)"""
        self._out_buffer = ""
        self._in_buffer = ""
        self.readfunc = readfunc
        self.terminator = terminator
        asyncore.dispatcher.__init__(self)
        if hasattr(sock_or_address, "fileno"):
            self.set_socket(sock_or_address)
        else:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.setblocking(0)
            self.set_socket(sock)
            if sock_or_address:
                self.connect(sock_or_address)

    # public methods
    def send(self, data):
        self._out_buffer = self._out_buffer + data

    def recv(self, bytes=-1):
        if bytes == -1:
            bytes = len(self._in_buffer)
        data = self._in_buffer[:bytes]
        self._in_buffer = self._in_buffer[bytes:]
        return data

    def set_terminator(self, terminator):
        self.terminator = terminator

    # override this if you want to control the incoming data stream
    def handle_incoming_data(self, data):
        if self.readfunc:
            if self.terminator:
                self._in_buffer = self._in_buffer + data
                pos = string.find(self._in_buffer, self.terminator)
                if pos < 0:
                    return
                data = self._in_buffer[:pos]
                self._in_buffer = self._in_buffer[pos + len(self.terminator):]
                self.readfunc(data)
            else:
                self.readfunc(self._in_buffer + data)
                self._in_buffer = ""
        else:
            self._in_buffer = self._in_buffer + data

    # internal muck
    def handle_read(self):
        data = asyncore.dispatcher.recv(self, BUFSIZE)
        if data:
            if VERBOSE > 2:
                print "incoming -> %x %r" % (id(self), data)
            self.handle_incoming_data(data)

    def handle_write(self):
        if self._out_buffer:
            sent = self.socket.send(self._out_buffer[:BUFSIZE])
            if VERBOSE > 2:
                print "outgoing -> %x %r" % (id(self), self._out_buffer[:sent])
            self._out_buffer = self._out_buffer[sent:]

    def close(self):
        if self.readfunc and self._in_buffer:
            self.readfunc(self._in_buffer)
            self._in_buffer = ""
        #elif VERBOSE > 1 and self._in_buffer:
        #       print "--- there is unread data: %r", (self._in_buffer,)
        asyncore.dispatcher.close(self)

    def handle_close(self):
        self.close()

    def handle_connect(self):
        pass


class ConnectionUI:

    """Glue to let a connection tell things to the UI in a standardized way.

    The protocoll defines four functions, which the connection will call:

            def settotal(int total): gets called when the connection knows the data size
            def setcurrent(int current): gets called when some new data has arrived
            def done(): gets called when the transaction is complete
            def error(type, value, tb): gets called wheneven an error occured
    """

    def __init__(self, settotal_func, setcurrent_func, done_func, error_func):
        self.settotal = settotal_func
        self.setcurrent = setcurrent_func
        self.done = done_func
        self.error = error_func


class HTTPError(socket.error): pass


class HTTPClient(Connection, httplib.HTTP):

    """Asynchronous HTTP connection"""

    def __init__(self, (host, port), datahandler, ui=None):
        Connection.__init__(self, (host, port))
        self.datahandler = datahandler
        self.ui = ui
        self.buf = ""
        self.doneheaders = 0
        self.done = 0
        self.headers = None
        self.length = None
        self.pos = 0

    def getreply(self):
        raise TypeError, "getreply() is not supported in async HTTP connection"

    def handle_incoming_data(self, data):
        assert not self.done
        if not self.doneheaders:
            self.buf = self.buf + data
            pos = string.find(self.buf, "\r\n\r\n")
            if pos >= 0:
                self.handle_reply(self.buf[:pos+4])
                length = self.headers.getheader("Content-Length")
                if length is not None:
                    self.length = int(length)
                    if self.ui is not None:
                        self.ui.settotal(self.length)
                else:
                    self.length = -1
                self.doneheaders = 1
                self.handle_data(self.buf[pos+4:])
                self.buf = ""
        else:
            self.handle_data(data)

    def handle_reply(self, data):
        f = cStringIO.StringIO(data)
        ver, code, msg = string.split(f.readline(), None, 2)
        code = int(code)
        msg = string.strip(msg)
        if code <> 200:
            # Hm, this is what *I* need, but probably not correct...
            raise HTTPError, (code, msg)
        self.headers = mimetools.Message(f)

    def handle_data(self, data):
        self.pos = self.pos + len(data)
        if self.ui is not None:
            self.ui.setcurrent(self.pos)
        self.datahandler(data)
        if self.pos >= self.length:
            self.datahandler("")
            self.done = 1
            if self.ui is not None:
                self.ui.done()

    def handle_error(self, type, value, tb):
        if self.ui is not None:
            self.ui.error(type, value, tb)
        else:
            Connection.handle_error(self, type, value, tb)

    def log(self, message):
        if VERBOSE:
            print 'LOG:', message


class PyMessage:

    def __init__(self):
        self._buf = ""
        self._len = None
        self._checksum = None

    def feed(self, data):
        self._buf = self._buf + data
        if self._len is None:
            if len(self._buf) >= 8:
                import struct
                self._len, self._checksum = struct.unpack("ll", self._buf[:8])
                self._buf = self._buf[8:]
        if self._len is not None:
            if len(self._buf) >= self._len:
                import zlib
                data = self._buf[:self._len]
                leftover = self._buf[self._len:]
                self._buf = None
                assert self._checksum == zlib.adler32(data), "corrupt data"
                self.data = data
                return 1, leftover
            else:
                return 0, None
        else:
            return 0, None


class PyConnection(Connection):

    def __init__(self, sock_or_address):
        Connection.__init__(self, sock_or_address)
        self.currentmessage = PyMessage()

    def handle_incoming_data(self, data):
        while data:
            done, data = self.currentmessage.feed(data)
            if done:
                import cPickle
                self.handle_object(cPickle.loads(self.currentmessage.data))
                self.currentmessage = PyMessage()

    def handle_object(self, object):
        print 'unhandled object:', repr(object)

    def send(self, object):
        import cPickle, zlib, struct
        data = cPickle.dumps(object, 1)
        length = len(data)
        checksum = zlib.adler32(data)
        data = struct.pack("ll", length, checksum) + data
        Connection.send(self, data)


class Echo(Connection):

    """Simple echoing connection: it sends everything back it receives."""

    def handle_incoming_data(self, data):
        self.send(data)


class Proxy(Connection):

    """Generic proxy connection"""

    def __init__(self, sock_or_address=None, proxyaddr=None, closepartner=0):
        """arguments:
        - sock_or_address is either a socket or a tuple containing the
        name and port number of a remote host
        - proxyaddr: a tuple containing a name and a port number of a
          remote host (optional).
        - closepartner: boolean, specifies whether we should close
          the proxy connection (optional)"""

        Connection.__init__(self, sock_or_address)
        self.other = None
        self.proxyaddr = proxyaddr
        self.closepartner = closepartner

    def close(self):
        if self.other:
            other = self.other
            self.other = None
            other.other = None
            if self.closepartner:
                other.close()
        Connection.close(self)

    def handle_incoming_data(self, data):
        if not self.other:
            # pass data for possible automatic remote host detection
            # (see HTTPProxy)
            data = self.connectproxy(data)
        self.other.send(data)

    def connectproxy(self, data):
        other = self.__class__(self.proxyaddr, closepartner=self.closepartner)
        self.other = other
        other.other = self
        return data


class HTTPProxy(Proxy):

    """Simple, useless, http proxy. It figures out itself where to connect to."""

    def connectproxy(self, data):
        if VERBOSE:
            print "--- proxy request", repr(data)
        addr, data = de_proxify(data)
        other = Proxy(addr)
        self.other = other
        other.other = self
        return data


# helper for HTTPProxy
def de_proxify(data):
    import re
    req_pattern = "GET http://([a-zA-Z0-9-_.]+)(:([0-9]+))?"
    m = re.match(req_pattern, data)
    host, dummy, port = m.groups()
    if not port:
        port = 80
    else:
        port = int(port)
    # change "GET http://xx.xx.xx/yy" into "GET /yy"
    data = re.sub(req_pattern, "GET ", data)
    return (host, port), data


# if we're running "under W", let's register the socket poller to the event loop
try:
    import W
except:
    pass
else:
    W.getapplication().addidlefunc(asyncore.poll)


## testing muck
#testserver = Server(10000, Connection)
#echoserver = Server(10007, Echo)
#httpproxyserver = Server(8088, HTTPProxy, 5)
#asyncore.close_all()
