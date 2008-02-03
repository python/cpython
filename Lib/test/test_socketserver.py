"""
Test suite for SocketServer.py.
"""

import os
import socket
import errno
import imp
import select
import time
import threading
from functools import wraps
import unittest
import SocketServer

import test.test_support
from test.test_support import reap_children, verbose, TestSkipped
from test.test_support import TESTFN as TEST_FILE

test.test_support.requires("network")

NREQ = 3
DELAY = 0.5
TEST_STR = "hello world\n"
HOST = "localhost"

HAVE_UNIX_SOCKETS = hasattr(socket, "AF_UNIX")
HAVE_FORKING = hasattr(os, "fork") and os.name != "os2"


class MyMixinHandler:
    def handle(self):
        time.sleep(DELAY)
        line = self.rfile.readline()
        time.sleep(DELAY)
        self.wfile.write(line)


def receive(sock, n, timeout=20):
    r, w, x = select.select([sock], [], [], timeout)
    if sock in r:
        return sock.recv(n)
    else:
        raise RuntimeError, "timed out on %r" % (sock,)


class MyStreamHandler(MyMixinHandler, SocketServer.StreamRequestHandler):
    pass

class MyDatagramHandler(MyMixinHandler,
    SocketServer.DatagramRequestHandler):
    pass

if HAVE_UNIX_SOCKETS:
    class ForkingUnixStreamServer(SocketServer.ForkingMixIn,
                                  SocketServer.UnixStreamServer):
        pass

    class ForkingUnixDatagramServer(SocketServer.ForkingMixIn,
                                    SocketServer.UnixDatagramServer):
        pass


class MyMixinServer:
    def serve_a_few(self):
        for i in range(NREQ):
            self.handle_request()

    def handle_error(self, request, client_address):
        self.close_request(request)
        self.server_close()
        raise


class ServerThread(threading.Thread):
    def __init__(self, addr, svrcls, hdlrcls):
        threading.Thread.__init__(self)
        self.__addr = addr
        self.__svrcls = svrcls
        self.__hdlrcls = hdlrcls
        self.ready = threading.Event()

    def run(self):
        class svrcls(MyMixinServer, self.__svrcls):
            pass
        if verbose: print "thread: creating server"
        svr = svrcls(self.__addr, self.__hdlrcls)
        # pull the address out of the server in case it changed
        # this can happen if another process is using the port
        addr = svr.server_address
        if addr:
            self.__addr = addr
            if self.__addr != svr.socket.getsockname():
                raise RuntimeError('server_address was %s, expected %s' %
                                       (self.__addr, svr.socket.getsockname()))
        self.ready.set()
        if verbose: print "thread: serving three times"
        svr.serve_a_few()
        if verbose: print "thread: done"


class ForgivingTCPServer(SocketServer.TCPServer):
    # prevent errors if another process is using the port we want
    def server_bind(self):
        host, default_port = self.server_address
        # this code shamelessly stolen from test.test_support
        # the ports were changed to protect the innocent
        import sys
        for port in [default_port, 3434, 8798, 23833]:
            try:
                self.server_address = host, port
                SocketServer.TCPServer.server_bind(self)
                break
            except socket.error, (err, msg):
                if err != errno.EADDRINUSE:
                    raise
                print >> sys.__stderr__, \
                    "WARNING: failed to listen on port %d, trying another: " % port


class SocketServerTest(unittest.TestCase):
    """Test all socket servers."""

    def setUp(self):
        self.port_seed = 0
        self.test_files = []

    def tearDown(self):
        time.sleep(DELAY)
        reap_children()

        for fn in self.test_files:
            try:
                os.remove(fn)
            except os.error:
                pass
        self.test_files[:] = []

    def pickport(self):
        self.port_seed += 1
        return 10000 + (os.getpid() % 1000)*10 + self.port_seed

    def pickaddr(self, proto):
        if proto == socket.AF_INET:
            return (HOST, self.pickport())
        else:
            fn = TEST_FILE + str(self.pickport())
            if os.name == 'os2':
                # AF_UNIX socket names on OS/2 require a specific prefix
                # which can't include a drive letter and must also use
                # backslashes as directory separators
                if fn[1] == ':':
                    fn = fn[2:]
                if fn[0] in (os.sep, os.altsep):
                    fn = fn[1:]
                fn = os.path.join('\socket', fn)
                if os.sep == '/':
                    fn = fn.replace(os.sep, os.altsep)
                else:
                    fn = fn.replace(os.altsep, os.sep)
            self.test_files.append(fn)
            return fn

    def run_servers(self, proto, servers, hdlrcls, testfunc):
        for svrcls in servers:
            addr = self.pickaddr(proto)
            if verbose:
                print "ADDR =", addr
                print "CLASS =", svrcls
            t = ServerThread(addr, svrcls, hdlrcls)
            if verbose: print "server created"
            t.start()
            if verbose: print "server running"
            for i in range(NREQ):
                t.ready.wait(10*DELAY)
                self.assert_(t.ready.isSet(),
                    "Server not ready within a reasonable time")
                if verbose: print "test client", i
                testfunc(proto, addr)
            if verbose: print "waiting for server"
            t.join()
            if verbose: print "done"

    def stream_examine(self, proto, addr):
        s = socket.socket(proto, socket.SOCK_STREAM)
        s.connect(addr)
        s.sendall(TEST_STR)
        buf = data = receive(s, 100)
        while data and '\n' not in buf:
            data = receive(s, 100)
            buf += data
        self.assertEquals(buf, TEST_STR)
        s.close()

    def dgram_examine(self, proto, addr):
        s = socket.socket(proto, socket.SOCK_DGRAM)
        s.sendto(TEST_STR, addr)
        buf = data = receive(s, 100)
        while data and '\n' not in buf:
            data = receive(s, 100)
            buf += data
        self.assertEquals(buf, TEST_STR)
        s.close()

    def test_TCPServers(self):
        # Test SocketServer.TCPServer
        servers = [ForgivingTCPServer, SocketServer.ThreadingTCPServer]
        if HAVE_FORKING:
            servers.append(SocketServer.ForkingTCPServer)
        self.run_servers(socket.AF_INET, servers,
                         MyStreamHandler, self.stream_examine)

    def test_UDPServers(self):
        # Test SocketServer.UDPServer
        servers = [SocketServer.UDPServer,
                   SocketServer.ThreadingUDPServer]
        if HAVE_FORKING:
            servers.append(SocketServer.ForkingUDPServer)
        self.run_servers(socket.AF_INET, servers, MyDatagramHandler,
                         self.dgram_examine)

    def test_stream_servers(self):
        # Test SocketServer's stream servers
        if not HAVE_UNIX_SOCKETS:
            return
        servers = [SocketServer.UnixStreamServer,
                   SocketServer.ThreadingUnixStreamServer]
        if HAVE_FORKING:
            servers.append(ForkingUnixStreamServer)
        self.run_servers(socket.AF_UNIX, servers, MyStreamHandler,
                         self.stream_examine)

    # Alas, on Linux (at least) recvfrom() doesn't return a meaningful
    # client address so this cannot work:

    # def test_dgram_servers(self):
    #     # Test SocketServer.UnixDatagramServer
    #     if not HAVE_UNIX_SOCKETS:
    #         return
    #     servers = [SocketServer.UnixDatagramServer,
    #                SocketServer.ThreadingUnixDatagramServer]
    #     if HAVE_FORKING:
    #         servers.append(ForkingUnixDatagramServer)
    #     self.run_servers(socket.AF_UNIX, servers, MyDatagramHandler,
    #                      self.dgram_examine)


def test_main():
    if imp.lock_held():
        # If the import lock is held, the threads will hang
        raise TestSkipped("can't run when import lock is held")

    test.test_support.run_unittest(SocketServerTest)

if __name__ == "__main__":
    test_main()
