"""
Test suite for SocketServer.py.
"""

import errno
import imp
import os
import select
import signal
import socket
import tempfile
import threading
import time
import unittest
import SocketServer

import test.test_support
from test.test_support import reap_children, verbose, TestSkipped
from test.test_support import TESTFN as TEST_FILE

test.test_support.requires("network")

NREQ = 3
TEST_STR = "hello world\n"
HOST = "localhost"

HAVE_UNIX_SOCKETS = hasattr(socket, "AF_UNIX")
HAVE_FORKING = hasattr(os, "fork") and os.name != "os2"


def receive(sock, n, timeout=20):
    r, w, x = select.select([sock], [], [], timeout)
    if sock in r:
        return sock.recv(n)
    else:
        raise RuntimeError, "timed out on %r" % (sock,)

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
        # We had the OS pick a port, so pull the real address out of
        # the server.
        self.addr = svr.server_address
        self.port = self.addr[1]
        if self.addr != svr.socket.getsockname():
            raise RuntimeError('server_address was %s, expected %s' %
                               (self.addr, svr.socket.getsockname()))
        self.ready.set()
        if verbose: print "thread: serving three times"
        svr.serve_a_few()
        if verbose: print "thread: done"


class SocketServerTest(unittest.TestCase):
    """Test all socket servers."""

    def setUp(self):
        signal.alarm(20)  # Kill deadlocks after 20 seconds.
        self.port_seed = 0
        self.test_files = []

    def tearDown(self):
        reap_children()

        for fn in self.test_files:
            try:
                os.remove(fn)
            except os.error:
                pass
        self.test_files[:] = []
        signal.alarm(0)  # Didn't deadlock.

    def pickaddr(self, proto):
        if proto == socket.AF_INET:
            return (HOST, 0)
        else:
            # XXX: We need a way to tell AF_UNIX to pick its own name
            # like AF_INET provides port==0.
            dir = None
            if os.name == 'os2':
                dir = '\socket'
            fn = tempfile.mktemp(prefix='unix_socket.', dir=dir)
            if os.name == 'os2':
                # AF_UNIX socket names on OS/2 require a specific prefix
                # which can't include a drive letter and must also use
                # backslashes as directory separators
                if fn[1] == ':':
                    fn = fn[2:]
                if fn[0] in (os.sep, os.altsep):
                    fn = fn[1:]
                if os.sep == '/':
                    fn = fn.replace(os.sep, os.altsep)
                else:
                    fn = fn.replace(os.altsep, os.sep)
            self.test_files.append(fn)
            return fn

    def run_server(self, svrcls, hdlrbase, testfunc):
        class MyHandler(hdlrbase):
            def handle(self):
                line = self.rfile.readline()
                self.wfile.write(line)

        addr = self.pickaddr(svrcls.address_family)
        if verbose:
            print "ADDR =", addr
            print "CLASS =", svrcls
        t = ServerThread(addr, svrcls, MyHandler)
        if verbose: print "server created"
        t.start()
        if verbose: print "server running"
        t.ready.wait(10)
        self.assert_(t.ready.isSet(),
                     "%s not ready within a reasonable time" % svrcls)
        addr = t.addr
        for i in range(NREQ):
            if verbose: print "test client", i
            testfunc(svrcls.address_family, addr)
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

    def test_TCPServer(self):
        self.run_server(SocketServer.TCPServer,
                        SocketServer.StreamRequestHandler,
                        self.stream_examine)

    def test_ThreadingTCPServer(self):
        self.run_server(SocketServer.ThreadingTCPServer,
                        SocketServer.StreamRequestHandler,
                        self.stream_examine)

    if HAVE_FORKING:
        def test_ThreadingTCPServer(self):
            self.run_server(SocketServer.ForkingTCPServer,
                            SocketServer.StreamRequestHandler,
                            self.stream_examine)

    if HAVE_UNIX_SOCKETS:
        def test_UnixStreamServer(self):
            self.run_server(SocketServer.UnixStreamServer,
                            SocketServer.StreamRequestHandler,
                            self.stream_examine)

        def test_ThreadingUnixStreamServer(self):
            self.run_server(SocketServer.ThreadingUnixStreamServer,
                            SocketServer.StreamRequestHandler,
                            self.stream_examine)

        if HAVE_FORKING:
            def test_ForkingUnixStreamServer(self):
                self.run_server(ForkingUnixStreamServer,
                                SocketServer.StreamRequestHandler,
                                self.stream_examine)

    def test_UDPServer(self):
        self.run_server(SocketServer.UDPServer,
                        SocketServer.DatagramRequestHandler,
                        self.dgram_examine)

    def test_ThreadingUDPServer(self):
        self.run_server(SocketServer.ThreadingUDPServer,
                        SocketServer.DatagramRequestHandler,
                        self.dgram_examine)

    if HAVE_FORKING:
        def test_ForkingUDPServer(self):
            self.run_server(SocketServer.ForkingUDPServer,
                            SocketServer.DatagramRequestHandler,
                            self.dgram_examine)

    # Alas, on Linux (at least) recvfrom() doesn't return a meaningful
    # client address so this cannot work:

    # if HAVE_UNIX_SOCKETS:
    #     def test_UnixDatagramServer(self):
    #         self.run_server(SocketServer.UnixDatagramServer,
    #                         SocketServer.DatagramRequestHandler,
    #                         self.dgram_examine)
    #
    #     def test_ThreadingUnixDatagramServer(self):
    #         self.run_server(SocketServer.ThreadingUnixDatagramServer,
    #                         SocketServer.DatagramRequestHandler,
    #                         self.dgram_examine)
    #
    #     if HAVE_FORKING:
    #         def test_ForkingUnixDatagramServer(self):
    #             self.run_server(SocketServer.ForkingUnixDatagramServer,
    #                             SocketServer.DatagramRequestHandler,
    #                             self.dgram_examine)


def test_main():
    if imp.lock_held():
        # If the import lock is held, the threads will hang
        raise TestSkipped("can't run when import lock is held")

    test.test_support.run_unittest(SocketServerTest)

if __name__ == "__main__":
    test_main()
    signal.alarm(3)  # Shutdown shouldn't take more than 3 seconds.
