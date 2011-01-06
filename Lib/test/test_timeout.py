"""Unit tests for socket timeout feature."""

import unittest
from test import support

# This requires the 'network' resource as given on the regrtest command line.
skip_expected = not support.is_resource_enabled('network')

import time
import errno
import socket


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
        self.assertEqual(type(self.sock.gettimeout()), type(1.0))

        self.sock.settimeout(3.9)
        self.assertEqual(type(self.sock.gettimeout()), type(1.0))

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
        self.sock.setblocking(1)
        self.assertEqual(self.sock.gettimeout(), None)
        self.sock.setblocking(0)
        self.assertEqual(self.sock.gettimeout(), 0.0)

        self.sock.settimeout(10)
        self.sock.setblocking(0)
        self.assertEqual(self.sock.gettimeout(), 0.0)
        self.sock.setblocking(1)
        self.assertEqual(self.sock.gettimeout(), None)

    def testBlockingThenTimeout(self):
        # Test setblocking() followed by settimeout()
        self.sock.setblocking(0)
        self.sock.settimeout(1)
        self.assertEqual(self.sock.gettimeout(), 1)

        self.sock.setblocking(1)
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

    localhost = '127.0.0.1'

    def setUp(self):
        raise NotImplementedError()

    tearDown = setUp

    def _sock_operation(self, count, timeout, method, *args):
        """
        Test the specified socket method.

        The method is run at most `count` times and must raise a socket.timeout
        within `timeout` + self.fuzz seconds.
        """
        self.sock.settimeout(timeout)
        method = getattr(self.sock, method)
        for i in range(count):
            t1 = time.time()
            try:
                method(*args)
            except socket.timeout as e:
                delta = time.time() - t1
                break
        else:
            self.fail('socket.timeout was not raised')
        # These checks should account for timing unprecision
        self.assertLess(delta, timeout + self.fuzz)
        self.assertGreater(delta, timeout - 1.0)


class TCPTimeoutTestCase(TimeoutTestCase):
    """TCP test case for socket.socket() timeout functions"""

    def setUp(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addr_remote = ('www.python.org.', 80)

    def tearDown(self):
        self.sock.close()

    def testConnectTimeout(self):
        # Choose a private address that is unlikely to exist to prevent
        # failures due to the connect succeeding before the timeout.
        # Use a dotted IP address to avoid including the DNS lookup time
        # with the connect time.  This avoids failing the assertion that
        # the timeout occurred fast enough.
        addr = ('10.0.0.0', 12345)
        with support.transient_internet(addr[0]):
            self._sock_operation(1, 0.001, 'connect', addr)

    def testRecvTimeout(self):
        # Test recv() timeout
        with support.transient_internet(self.addr_remote[0]):
            self.sock.connect(self.addr_remote)
            self._sock_operation(1, 1.5, 'recv', 1024)

    def testAcceptTimeout(self):
        # Test accept() timeout
        support.bind_port(self.sock, self.localhost)
        self.sock.listen(5)
        self._sock_operation(1, 1.5, 'accept')

    def testSend(self):
        # Test send() timeout
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as serv:
            support.bind_port(serv, self.localhost)
            serv.listen(5)
            self.sock.connect(serv.getsockname())
            # Send a lot of data in order to bypass buffering in the TCP stack.
            self._sock_operation(100, 1.5, 'send', b"X" * 200000)

    def testSendto(self):
        # Test sendto() timeout
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as serv:
            support.bind_port(serv, self.localhost)
            serv.listen(5)
            self.sock.connect(serv.getsockname())
            # The address argument is ignored since we already connected.
            self._sock_operation(100, 1.5, 'sendto', b"X" * 200000,
                                 serv.getsockname())

    def testSendall(self):
        # Test sendall() timeout
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as serv:
            support.bind_port(serv, self.localhost)
            serv.listen(5)
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
        support.bind_port(self.sock, self.localhost)
        self._sock_operation(1, 1.5, 'recvfrom', 1024)


def test_main():
    support.requires('network')
    support.run_unittest(
        CreationTestCase,
        TCPTimeoutTestCase,
        UDPTimeoutTestCase,
    )

if __name__ == "__main__":
    test_main()
