"""
Test suite for SocketServer.py.
"""

import contextlib
import imp
import os
import select
import signal
import socket
import select
import errno
import tempfile
import unittest
import SocketServer

import test.test_support
from test.test_support import reap_children, reap_threads, verbose
try:
    import threading
except ImportError:
    threading = None

test.test_support.requires("network")

TEST_STR = "hello world\n"
HOST = test.test_support.HOST

HAVE_UNIX_SOCKETS = hasattr(socket, "AF_UNIX")
requires_unix_sockets = unittest.skipUnless(HAVE_UNIX_SOCKETS,
                                            'requires Unix sockets')
HAVE_FORKING = hasattr(os, "fork") and os.name != "os2"
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
        # Don't raise an exception; it would be caught by the test harness.
        os._exit(72)
    yield None
    pid2, status = os.waitpid(pid, 0)
    testcase.assertEqual(pid2, pid)
    testcase.assertEqual(72 << 8, status)


def close_server(server):
    server.server_close()

    if hasattr(server, 'active_children'):
        # ForkingMixIn: Manually reap all child processes, since server_close()
        # calls waitpid() in non-blocking mode using the WNOHANG flag.
        for pid in server.active_children.copy():
            try:
                os.waitpid(pid, 0)
            except ChildProcessError:
                pass
        server.active_children.clear()


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
            except os.error:
                pass
        self.test_files[:] = []

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
                close_server(self)
                raise

        class MyHandler(hdlrbase):
            def handle(self):
                line = self.rfile.readline()
                self.wfile.write(line)

        if verbose: print "creating server"
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
        t.daemon = True  # In case this function raises.
        t.start()
        if verbose: print "server running"
        for i in range(3):
            if verbose: print "test client", i
            testfunc(svrcls.address_family, addr)
        if verbose: print "waiting for server"
        server.shutdown()
        t.join()
        close_server(server)
        self.assertRaises(socket.error, server.socket.fileno)
        if verbose: print "done"

    def stream_examine(self, proto, addr):
        s = socket.socket(proto, socket.SOCK_STREAM)
        s.connect(addr)
        s.sendall(TEST_STR)
        buf = data = receive(s, 100)
        while data and '\n' not in buf:
            data = receive(s, 100)
            buf += data
        self.assertEqual(buf, TEST_STR)
        s.close()

    def dgram_examine(self, proto, addr):
        s = socket.socket(proto, socket.SOCK_DGRAM)
        if HAVE_UNIX_SOCKETS and proto == socket.AF_UNIX:
            s.bind(self.pickaddr(proto))
        s.sendto(TEST_STR, addr)
        buf = data = receive(s, 100)
        while data and '\n' not in buf:
            data = receive(s, 100)
            buf += data
        self.assertEqual(buf, TEST_STR)
        s.close()

    def test_TCPServer(self):
        self.run_server(SocketServer.TCPServer,
                        SocketServer.StreamRequestHandler,
                        self.stream_examine)

    def test_ThreadingTCPServer(self):
        self.run_server(SocketServer.ThreadingTCPServer,
                        SocketServer.StreamRequestHandler,
                        self.stream_examine)

    @requires_forking
    def test_ForkingTCPServer(self):
        with simple_subprocess(self):
            self.run_server(SocketServer.ForkingTCPServer,
                            SocketServer.StreamRequestHandler,
                            self.stream_examine)

    @requires_unix_sockets
    def test_UnixStreamServer(self):
        self.run_server(SocketServer.UnixStreamServer,
                        SocketServer.StreamRequestHandler,
                        self.stream_examine)

    @requires_unix_sockets
    def test_ThreadingUnixStreamServer(self):
        self.run_server(SocketServer.ThreadingUnixStreamServer,
                        SocketServer.StreamRequestHandler,
                        self.stream_examine)

    @requires_unix_sockets
    @requires_forking
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

    @requires_forking
    def test_ForkingUDPServer(self):
        with simple_subprocess(self):
            self.run_server(SocketServer.ForkingUDPServer,
                            SocketServer.DatagramRequestHandler,
                            self.dgram_examine)

    @contextlib.contextmanager
    def mocked_select_module(self):
        """Mocks the select.select() call to raise EINTR for first call"""
        old_select = select.select

        class MockSelect:
            def __init__(self):
                self.called = 0

            def __call__(self, *args):
                self.called += 1
                if self.called == 1:
                    # raise the exception on first call
                    raise select.error(errno.EINTR, os.strerror(errno.EINTR))
                else:
                    # Return real select value for consecutive calls
                    return old_select(*args)

        select.select = MockSelect()
        try:
            yield select.select
        finally:
            select.select = old_select

    def test_InterruptServerSelectCall(self):
        with self.mocked_select_module() as mock_select:
            pid = self.run_server(SocketServer.TCPServer,
                                  SocketServer.StreamRequestHandler,
                                  self.stream_examine)
            # Make sure select was called again:
            self.assertGreater(mock_select.called, 1)

    @requires_unix_sockets
    def test_UnixDatagramServer(self):
        self.run_server(SocketServer.UnixDatagramServer,
                        SocketServer.DatagramRequestHandler,
                        self.dgram_examine)

    @requires_unix_sockets
    def test_ThreadingUnixDatagramServer(self):
        self.run_server(SocketServer.ThreadingUnixDatagramServer,
                        SocketServer.DatagramRequestHandler,
                        self.dgram_examine)

    @requires_unix_sockets
    @requires_forking
    def test_ForkingUnixDatagramServer(self):
        self.run_server(ForkingUnixDatagramServer,
                        SocketServer.DatagramRequestHandler,
                        self.dgram_examine)

    @reap_threads
    def test_shutdown(self):
        # Issue #2302: shutdown() should always succeed in making an
        # other thread leave serve_forever().
        class MyServer(SocketServer.TCPServer):
            pass

        class MyHandler(SocketServer.StreamRequestHandler):
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
            close_server(s)

    def test_tcpserver_bind_leak(self):
        # Issue #22435: the server socket wouldn't be closed if bind()/listen()
        # failed.
        # Create many servers for which bind() will fail, to see if this result
        # in FD exhaustion.
        for i in range(1024):
            with self.assertRaises(OverflowError):
                SocketServer.TCPServer((HOST, -1),
                                       SocketServer.StreamRequestHandler)


class MiscTestCase(unittest.TestCase):

    def test_shutdown_request_called_if_verify_request_false(self):
        # Issue #26309: BaseServer should call shutdown_request even if
        # verify_request is False

        class MyServer(SocketServer.TCPServer):
            def verify_request(self, request, client_address):
                return False

            shutdown_called = 0
            def shutdown_request(self, request):
                self.shutdown_called += 1
                SocketServer.TCPServer.shutdown_request(self, request)

        server = MyServer((HOST, 0), SocketServer.StreamRequestHandler)
        s = socket.socket(server.address_family, socket.SOCK_STREAM)
        s.connect(server.server_address)
        s.close()
        server.handle_request()
        self.assertEqual(server.shutdown_called, 1)
        close_server(server)


def test_main():
    if imp.lock_held():
        # If the import lock is held, the threads will hang
        raise unittest.SkipTest("can't run when import lock is held")

    test.test_support.run_unittest(SocketServerTest)

if __name__ == "__main__":
    test_main()
