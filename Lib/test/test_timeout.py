"""Unit tests for socket timeout feature."""

import unittest
from test import test_support

# This requires the 'network' resource as given on the regrtest command line.
skip_expected = not test_support.is_resource_enabled('network')

import time
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
        self.sock.settimeout(0L)
        self.sock.settimeout(0.0)
        self.sock.settimeout(None)
        self.assertRaises(TypeError, self.sock.settimeout, "")
        self.assertRaises(TypeError, self.sock.settimeout, u"")
        self.assertRaises(TypeError, self.sock.settimeout, ())
        self.assertRaises(TypeError, self.sock.settimeout, [])
        self.assertRaises(TypeError, self.sock.settimeout, {})
        self.assertRaises(TypeError, self.sock.settimeout, 0j)

    def testRangeCheck(self):
        # Test range checking by settimeout()
        self.assertRaises(ValueError, self.sock.settimeout, -1)
        self.assertRaises(ValueError, self.sock.settimeout, -1L)
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
    """Test case for socket.socket() timeout functions"""

    # There are a number of tests here trying to make sure that an operation
    # doesn't take too much longer than expected.  But competing machine
    # activity makes it inevitable that such tests will fail at times.
    # When fuzz was at 1.0, I (tim) routinely saw bogus failures on Win2K
    # and Win98SE.  Boosting it to 2.0 helped a lot, but isn't a real
    # solution.
    fuzz = 2.0

    def setUp(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addr_remote = ('www.python.org.', 80)
        self.localhost = '127.0.0.1'

    def tearDown(self):
        self.sock.close()

    def testConnectTimeout(self):
        # Choose a private address that is unlikely to exist to prevent
        # failures due to the connect succeeding before the timeout.
        # Use a dotted IP address to avoid including the DNS lookup time
        # with the connect time.  This avoids failing the assertion that
        # the timeout occurred fast enough.
        addr = ('10.0.0.0', 12345)

        # Test connect() timeout
        _timeout = 0.001
        self.sock.settimeout(_timeout)

        _t1 = time.time()
        self.failUnlessRaises(socket.error, self.sock.connect, addr)
        _t2 = time.time()

        _delta = abs(_t1 - _t2)
        self.assert_(_delta < _timeout + self.fuzz,
                     "timeout (%g) is more than %g seconds more than expected (%g)"
                     %(_delta, self.fuzz, _timeout))

    def testRecvTimeout(self):
        # Test recv() timeout
        _timeout = 0.02
        self.sock.connect(self.addr_remote)
        self.sock.settimeout(_timeout)

        _t1 = time.time()
        self.failUnlessRaises(socket.error, self.sock.recv, 1024)
        _t2 = time.time()

        _delta = abs(_t1 - _t2)
        self.assert_(_delta < _timeout + self.fuzz,
                     "timeout (%g) is %g seconds more than expected (%g)"
                     %(_delta, self.fuzz, _timeout))

    def testAcceptTimeout(self):
        # Test accept() timeout
        _timeout = 2
        self.sock.settimeout(_timeout)
        # Prevent "Address already in use" socket exceptions
        test_support.bind_port(self.sock, self.localhost)
        self.sock.listen(5)

        _t1 = time.time()
        self.failUnlessRaises(socket.error, self.sock.accept)
        _t2 = time.time()

        _delta = abs(_t1 - _t2)
        self.assert_(_delta < _timeout + self.fuzz,
                     "timeout (%g) is %g seconds more than expected (%g)"
                     %(_delta, self.fuzz, _timeout))

    def testRecvfromTimeout(self):
        # Test recvfrom() timeout
        _timeout = 2
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(_timeout)
        # Prevent "Address already in use" socket exceptions
        test_support.bind_port(self.sock, self.localhost)

        _t1 = time.time()
        self.failUnlessRaises(socket.error, self.sock.recvfrom, 8192)
        _t2 = time.time()

        _delta = abs(_t1 - _t2)
        self.assert_(_delta < _timeout + self.fuzz,
                     "timeout (%g) is %g seconds more than expected (%g)"
                     %(_delta, self.fuzz, _timeout))

    def testSend(self):
        # Test send() timeout
        # couldn't figure out how to test it
        pass

    def testSendto(self):
        # Test sendto() timeout
        # couldn't figure out how to test it
        pass

    def testSendall(self):
        # Test sendall() timeout
        # couldn't figure out how to test it
        pass


def test_main():
    test_support.requires('network')
    test_support.run_unittest(CreationTestCase, TimeoutTestCase)

if __name__ == "__main__":
    test_main()
