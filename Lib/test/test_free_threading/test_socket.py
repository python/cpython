import socket
import unittest

from test import support
from test.support import threading_helper

N = 200 if support.check_sanitizer(thread=True) else 1000


@threading_helper.requires_working_threading()
class TestSocketTimeoutRaces(unittest.TestCase):
    # gh-153935: the socket timeout was read/written non-atomically, so reading
    # it (gettimeout()/getblocking() or during a socket operation) raced with
    # settimeout()/setblocking() on the same socket.

    def new_socket(self):
        s = socket.socket()
        self.addCleanup(s.close)
        return s

    def check_race(self, read, write):
        def reader():
            for _ in range(N):
                read()

        def writer():
            for _ in range(N):
                write()

        threading_helper.run_concurrently([reader, writer])

    def test_gettimeout_vs_setblocking(self):
        s = self.new_socket()
        self.check_race(s.gettimeout, lambda: s.setblocking(False))

    def test_gettimeout_vs_settimeout(self):
        s = self.new_socket()
        self.check_race(s.gettimeout, lambda: s.settimeout(1.0))

    def test_getblocking_vs_settimeout(self):
        s = self.new_socket()
        self.check_race(s.getblocking, lambda: s.settimeout(0))

    def test_operation_vs_settimeout(self):
        a, b = socket.socketpair()
        self.addCleanup(a.close)
        self.addCleanup(b.close)
        a.setblocking(False)

        def recv():
            try:
                a.recv(64)
            except OSError:
                pass

        self.check_race(recv, lambda: a.settimeout(0))

    def test_sendall_vs_settimeout(self):
        a, b = socket.socketpair()
        self.addCleanup(a.close)
        self.addCleanup(b.close)
        a.setblocking(False)

        def sendall():
            try:
                a.sendall(b"x" * 64)
            except OSError:
                pass

        self.check_race(sendall, lambda: a.settimeout(0))

    def test_connect_vs_settimeout(self):
        lsock = socket.socket()
        lsock.bind(("127.0.0.1", 0))
        lsock.listen()
        self.addCleanup(lsock.close)
        addr = lsock.getsockname()
        s = self.new_socket()

        self.check_race(lambda: s.connect_ex(addr), lambda: s.settimeout(0))


if __name__ == "__main__":
    unittest.main()
