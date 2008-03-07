"""
Test suite for SocketServer.py.
"""

import contextlib
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

TEST_STR = "hello world\n"
HOST = "localhost"

HAVE_UNIX_SOCKETS = hasattr(socket, "AF_UNIX")
HAVE_FORKING = hasattr(os, "fork") and os.name != "os2"

def signal_alarm(n):
    """Call signal.alarm when it exists (i.e. not on Windows)."""
    if hasattr(signal, 'alarm'):
        signal.alarm(n)

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


@contextlib.contextmanager
def simple_subprocess(testcase):
    pid = os.fork()
    if pid == 0:
        # Don't throw an exception; it would be caught by the test harness.
        os._exit(72)
    yield None
    pid2, status = os.waitpid(pid, 0)
    testcase.assertEquals(pid2, pid)
    testcase.assertEquals(72 << 8, status)


class SocketServerTest(unittest.TestCase):
    """Test all socket servers."""

    def setUp(self):
        signal_alarm(20)  # Kill deadlocks after 20 seconds.
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
        signal_alarm(0)  # Didn't deadlock.

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

    def make_server(self, addr, svrcls, hdlrbase):
        class MyServer(svrcls):
            def handle_error(self, request, client_address):
                self.close_request(request)
                self.server_close()
                raise

        class MyHandler(hdlrbase):
            def handle(self):
                line = self.rfile.readline()
                self.wfile.write(line)

        if verbose: print "creating server"
        server = MyServer(addr, MyHandler)
        self.assertEquals(server.server_address, server.socket.getsockname())
        return server

    def run_server(self, svrcls, hdlrbase, testfunc):
        server = self.make_server(self.pickaddr(svrcls.address_family),
                                  svrcls, hdlrbase)
        # We had the OS pick a port, so pull the real address out of
        # the server.
        addr = server.server_address
        if verbose:
            print "server created"
            print "ADDR =", addr
            print "CLASS =", svrcls
        t = threading.Thread(
            name='%s serving' % svrcls,
            target=server.serve_forever,
            # Short poll interval to make the test finish quickly.
            # Time between requests is short enough that we won't wake
            # up spuriously too many times.
            kwargs={'poll_interval':0.01})
        t.setDaemon(True)  # In case this function raises.
        t.start()
        if verbose: print "server running"
        for i in range(3):
            if verbose: print "test client", i
            testfunc(svrcls.address_family, addr)
        if verbose: print "waiting for server"
        server.shutdown()
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
        def test_ForkingTCPServer(self):
            with simple_subprocess(self):
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
                with simple_subprocess(self):
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
            with simple_subprocess(self):
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
    signal_alarm(3)  # Shutdown shouldn't take more than 3 seconds.
