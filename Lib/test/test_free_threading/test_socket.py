import socket
import unittest

from test import support
from test.support import threading_helper

N = 200 if support.check_sanitizer(thread=True) else 1000


@threading_helper.requires_working_threading()
class TestSocketTimeoutRaces(unittest.TestCase):
    # gh-153852: the socket timeout was read/written non-atomically, so reading
    # it with gettimeout()/getblocking() raced with settimeout()/setblocking()
    # on the same socket.

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


if __name__ == "__main__":
    unittest.main()
