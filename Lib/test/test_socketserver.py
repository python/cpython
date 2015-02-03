"""
Test suite for socketserver.
"""

import contextlib
import os
import select
import signal
import socket
import select
import errno
import tempfile
import unittest
import socketserver

import test.support
from test.support import reap_children, reap_threads, verbose
try:
    import threading
except ImportError:
    threading = None

test.support.requires("network")

TEST_STR = b"hello world\n"
HOST = test.support.HOST

HAVE_UNIX_SOCKETS = hasattr(socket, "AF_UNIX")
requires_unix_sockets = unittest.skipUnless(HAVE_UNIX_SOCKETS,
                                            'requires Unix sockets')
HAVE_FORKING = hasattr(os, "fork")
requires_forking = unittest.skipUnless(HAVE_FORKING, 'requires forking')

def signal_alarm(n):
    """Call signal.alarm when it exists (i.e. not on Windows)."""
    if hasattr(signal, 'alarm'):
        signal.alarm(n)

# Remember real select() to avoid interferences with mocking
_real_select = select.select

def receive(sock, n, timeout=20):
    r, w, x = _real_select([sock], [], [], timeout)
    if sock in r:
        return sock.recv(n)
    else:
        raise RuntimeError("timed out on %r" % (sock,))

if HAVE_UNIX_SOCKETS:
    class ForkingUnixStreamServer(socketserver.ForkingMixIn,
                                  socketserver.UnixStreamServer):
        pass

    class ForkingUnixDatagramServer(socketserver.ForkingMixIn,
                                    socketserver.UnixDatagramServer):
        pass


@contextlib.contextmanager
def simple_subprocess(testcase):
    pid = os.fork()
    if pid == 0:
        # Don't raise an exception; it would be caught by the test harness.
        os._exit(72)
    yield None
    pid2, status = os.waitpid(pid, 0)
    testcase.assertEqual(pid2, pid)
    testcase.assertEqual(72 << 8, status)


@unittest.skipUnless(threading, 'Threading required for this test.')
class SocketServerTest(unittest.TestCase):
    """Test all socket servers."""

    def setUp(self):
        signal_alarm(60)  # Kill deadlocks after 60 seconds.
        self.port_seed = 0
        self.test_files = []

    def tearDown(self):
        signal_alarm(0)  # Didn't deadlock.
        reap_children()

        for fn in self.test_files:
            try:
                os.remove(fn)
            except OSError:
                pass
        self.test_files[:] = []

    def pickaddr(self, proto):
        if proto == socket.AF_INET:
            return (HOST, 0)
        else:
            # XXX: We need a way to tell AF_UNIX to pick its own name
            # like AF_INET provides port==0.
            dir = None
            fn = tempfile.mktemp(prefix='unix_socket.', dir=dir)
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

        if verbose: print("creating server")
        server = MyServer(addr, MyHandler)
        self.assertEqual(server.server_address, server.socket.getsockname())
        return server

    @reap_threads
    def run_server(self, svrcls, hdlrbase, testfunc):
        server = self.make_server(self.pickaddr(svrcls.address_family),
                                  svrcls, hdlrbase)
        # We had the OS pick a port, so pull the real address out of
        # the server.
        addr = server.server_address
        if verbose:
            print("ADDR =", addr)
            print("CLASS =", svrcls)

        t = threading.Thread(
            name='%s serving' % svrcls,
            target=server.serve_forever,
            # Short poll interval to make the test finish quickly.
            # Time between requests is short enough that we won't wake
            # up spuriously too many times.
            kwargs={'poll_interval':0.01})
        t.daemon = True  # In case this function raises.
        t.start()
        if verbose: print("server running")
        for i in range(3):
            if verbose: print("test client", i)
            testfunc(svrcls.address_family, addr)
        if verbose: print("waiting for server")
        server.shutdown()
        t.join()
        server.server_close()
        if verbose: print("done")

    def stream_examine(self, proto, addr):
        s = socket.socket(proto, socket.SOCK_STREAM)
        s.connect(addr)
        s.sendall(TEST_STR)
        buf = data = receive(s, 100)
        while data and b'\n' not in buf:
            data = receive(s, 100)
            buf += data
        self.assertEqual(buf, TEST_STR)
        s.close()

    def dgram_examine(self, proto, addr):
        s = socket.socket(proto, socket.SOCK_DGRAM)
        s.sendto(TEST_STR, addr)
        buf = data = receive(s, 100)
        while data and b'\n' not in buf:
            data = receive(s, 100)
            buf += data
        self.assertEqual(buf, TEST_STR)
        s.close()

    def test_TCPServer(self):
        self.run_server(socketserver.TCPServer,
                        socketserver.StreamRequestHandler,
                        self.stream_examine)

    def test_ThreadingTCPServer(self):
        self.run_server(socketserver.ThreadingTCPServer,
                        socketserver.StreamRequestHandler,
                        self.stream_examine)

    @requires_forking
    def test_ForkingTCPServer(self):
        with simple_subprocess(self):
            self.run_server(socketserver.ForkingTCPServer,
                            socketserver.StreamRequestHandler,
                            self.stream_examine)

    @requires_unix_sockets
    def test_UnixStreamServer(self):
        self.run_server(socketserver.UnixStreamServer,
                        socketserver.StreamRequestHandler,
                        self.stream_examine)

    @requires_unix_sockets
    def test_ThreadingUnixStreamServer(self):
        self.run_server(socketserver.ThreadingUnixStreamServer,
                        socketserver.StreamRequestHandler,
                        self.stream_examine)

    @requires_unix_sockets
    @requires_forking
    def test_ForkingUnixStreamServer(self):
        with simple_subprocess(self):
            self.run_server(ForkingUnixStreamServer,
                            socketserver.StreamRequestHandler,
                            self.stream_examine)

    def test_UDPServer(self):
        self.run_server(socketserver.UDPServer,
                        socketserver.DatagramRequestHandler,
                        self.dgram_examine)

    def test_ThreadingUDPServer(self):
        self.run_server(socketserver.ThreadingUDPServer,
                        socketserver.DatagramRequestHandler,
                        self.dgram_examine)

    @requires_forking
    def test_ForkingUDPServer(self):
        with simple_subprocess(self):
            self.run_server(socketserver.ForkingUDPServer,
                            socketserver.DatagramRequestHandler,
                            self.dgram_examine)

    # Alas, on Linux (at least) recvfrom() doesn't return a meaningful
    # client address so this cannot work:

    # @requires_unix_sockets
    # def test_UnixDatagramServer(self):
    #     self.run_server(socketserver.UnixDatagramServer,
    #                     socketserver.DatagramRequestHandler,
    #                     self.dgram_examine)
    #
    # @requires_unix_sockets
    # def test_ThreadingUnixDatagramServer(self):
    #     self.run_server(socketserver.ThreadingUnixDatagramServer,
    #                     socketserver.DatagramRequestHandler,
    #                     self.dgram_examine)
    #
    # @requires_unix_sockets
    # @requires_forking
    # def test_ForkingUnixDatagramServer(self):
    #     self.run_server(socketserver.ForkingUnixDatagramServer,
    #                     socketserver.DatagramRequestHandler,
    #                     self.dgram_examine)

    @reap_threads
    def test_shutdown(self):
        # Issue #2302: shutdown() should always succeed in making an
        # other thread leave serve_forever().
        class MyServer(socketserver.TCPServer):
            pass

        class MyHandler(socketserver.StreamRequestHandler):
            pass

        threads = []
        for i in range(20):
            s = MyServer((HOST, 0), MyHandler)
            t = threading.Thread(
                name='MyServer serving',
                target=s.serve_forever,
                kwargs={'poll_interval':0.01})
            t.daemon = True  # In case this function raises.
            threads.append((t, s))
        for t, s in threads:
            t.start()
            s.shutdown()
        for t, s in threads:
            t.join()
            s.server_close()

    def test_tcpserver_bind_leak(self):
        # Issue #22435: the server socket wouldn't be closed if bind()/listen()
        # failed.
        # Create many servers for which bind() will fail, to see if this result
        # in FD exhaustion.
        for i in range(1024):
            with self.assertRaises(OverflowError):
                socketserver.TCPServer((HOST, -1),
                                       socketserver.StreamRequestHandler)


class MiscTestCase(unittest.TestCase):

    def test_all(self):
        # objects defined in the module should be in __all__
        expected = []
        for name in dir(socketserver):
            if not name.startswith('_'):
                mod_object = getattr(socketserver, name)
                if getattr(mod_object, '__module__', None) == 'socketserver':
                    expected.append(name)
        self.assertCountEqual(socketserver.__all__, expected)


if __name__ == "__main__":
    unittest.main()
