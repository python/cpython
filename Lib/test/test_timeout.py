"""Unit tests for socket timeout feature."""

import functools
import unittest
from test import support
from test.support import socket_helper

import time
import socket


@functools.lru_cache()
def resolve_address(host, port):
    """Resolve an (host, port) to an address.

    We must perform name resolution before timeout tests, otherwise it will be
    performed by connect().
    """
    with socket_helper.transient_internet(host):
        return socket.getaddrinfo(host, port, socket.AF_INET,
                                  socket.SOCK_STREAM)[0][4]


class CreationTestCase(unittest.TestCase):
    """Test case for socket.gettimeout() and socket.settimeout()"""

    def setUp(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def tearDown(self):
        self.sock.close()

    def testObjectCreation(self):
        # Test Socket creation
        self.assertEqual(self.sock.gettimeout(), None,
                         "timeout not disabled by default")

    def testFloatReturnValue(self):
        # Test return value of gettimeout()
        self.sock.settimeout(7.345)
        self.assertEqual(self.sock.gettimeout(), 7.345)

        self.sock.settimeout(3)
        self.assertEqual(self.sock.gettimeout(), 3)

        self.sock.settimeout(None)
        self.assertEqual(self.sock.gettimeout(), None)

    def testReturnType(self):
        # Test return type of gettimeout()
        self.sock.settimeout(1)
        self.assertIs(type(self.sock.gettimeout()), float)

        self.sock.settimeout(3.9)
        self.assertIs(type(self.sock.gettimeout()), float)

    def testTypeCheck(self):
        # Test type checking by settimeout()
        self.sock.settimeout(0)
        self.sock.settimeout(0)
        self.sock.settimeout(0.0)
        self.sock.settimeout(None)
        self.assertRaises(TypeError, self.sock.settimeout, "")
        self.assertRaises(TypeError, self.sock.settimeout, "")
        self.assertRaises(TypeError, self.sock.settimeout, ())
        self.assertRaises(TypeError, self.sock.settimeout, [])
        self.assertRaises(TypeError, self.sock.settimeout, {})
        self.assertRaises(TypeError, self.sock.settimeout, 0j)

    def testRangeCheck(self):
        # Test range checking by settimeout()
        self.assertRaises(ValueError, self.sock.settimeout, -1)
        self.assertRaises(ValueError, self.sock.settimeout, -1)
        self.assertRaises(ValueError, self.sock.settimeout, -1.0)

    def testTimeoutThenBlocking(self):
        # Test settimeout() followed by setblocking()
        self.sock.settimeout(10)
        self.sock.setblocking(True)
        self.assertEqual(self.sock.gettimeout(), None)
        self.sock.setblocking(False)
        self.assertEqual(self.sock.gettimeout(), 0.0)

        self.sock.settimeout(10)
        self.sock.setblocking(False)
        self.assertEqual(self.sock.gettimeout(), 0.0)
        self.sock.setblocking(True)
        self.assertEqual(self.sock.gettimeout(), None)

    def testBlockingThenTimeout(self):
        # Test setblocking() followed by settimeout()
        self.sock.setblocking(False)
        self.sock.settimeout(1)
        self.assertEqual(self.sock.gettimeout(), 1)

        self.sock.setblocking(True)
        self.sock.settimeout(1)
        self.assertEqual(self.sock.gettimeout(), 1)


class TimeoutTestCase(unittest.TestCase):
    # There are a number of tests here trying to make sure that an operation
    # doesn't take too much longer than expected.  But competing machine
    # activity makes it inevitable that such tests will fail at times.
    # When fuzz was at 1.0, I (tim) routinely saw bogus failures on Win2K
    # and Win98SE.  Boosting it to 2.0 helped a lot, but isn't a real
    # solution.
    fuzz = 2.0

    localhost = socket_helper.HOST

    def setUp(self):
        raise NotImplementedError()

    tearDown = setUp

    def _sock_operation(self, count, timeout, method, *args):
        """
        Test the specified socket method.

        The method is run at most `count` times and must raise a TimeoutError
        within `timeout` + self.fuzz seconds.
        """
        self.sock.settimeout(timeout)
        method = getattr(self.sock, method)
        for i in range(count):
            t1 = time.monotonic()
            try:
                method(*args)
            except TimeoutError as e:
                delta = time.monotonic() - t1
                break
        else:
            self.fail('TimeoutError was not raised')
        # These checks should account for timing unprecision
        self.assertLess(delta, timeout + self.fuzz)
        self.assertGreater(delta, timeout - 1.0)


class TCPTimeoutTestCase(TimeoutTestCase):
    """TCP test case for socket.socket() timeout functions"""

    def setUp(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addr_remote = resolve_address('www.python.org.', 80)

    def tearDown(self):
        self.sock.close()

    def testRecvTimeout(self):
        # Test recv() timeout
        with socket_helper.transient_internet(self.addr_remote[0]):
            self.sock.connect(self.addr_remote)
            self._sock_operation(1, 1.5, 'recv', 1024)

    def testAcceptTimeout(self):
        # Test accept() timeout
        socket_helper.bind_port(self.sock, self.localhost)
        self.sock.listen()
        self._sock_operation(1, 1.5, 'accept')

    def testSend(self):
        # Test send() timeout
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as serv:
            socket_helper.bind_port(serv, self.localhost)
            serv.listen()
            self.sock.connect(serv.getsockname())
            # Send a lot of data in order to bypass buffering in the TCP stack.
            self._sock_operation(100, 1.5, 'send', b"X" * 200000)

    def testSendto(self):
        # Test sendto() timeout
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as serv:
            socket_helper.bind_port(serv, self.localhost)
            serv.listen()
            self.sock.connect(serv.getsockname())
            # The address argument is ignored since we already connected.
            self._sock_operation(100, 1.5, 'sendto', b"X" * 200000,
                                 serv.getsockname())

    def testSendall(self):
        # Test sendall() timeout
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as serv:
            socket_helper.bind_port(serv, self.localhost)
            serv.listen()
            self.sock.connect(serv.getsockname())
            # Send a lot of data in order to bypass buffering in the TCP stack.
            self._sock_operation(100, 1.5, 'sendall', b"X" * 200000)


class UDPTimeoutTestCase(TimeoutTestCase):
    """UDP test case for socket.socket() timeout functions"""

    def setUp(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def tearDown(self):
        self.sock.close()

    def testRecvfromTimeout(self):
        # Test recvfrom() timeout
        # Prevent "Address already in use" socket exceptions
        socket_helper.bind_port(self.sock, self.localhost)
        self._sock_operation(1, 1.5, 'recvfrom', 1024)


def setUpModule():
    support.requires('network')
    support.requires_working_socket(module=True)


if __name__ == "__main__":
    unittest.main()
