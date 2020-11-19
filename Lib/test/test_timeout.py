"""Unit tests for socket timeout feature."""

import functools
import unittest
from test import support
from test.support import socket_helper

# This requires the 'network' resource as given on the regrtest command line.
skip_expected = not support.is_resource_enabled('network')

import time
import errno
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

        The method is run at most `count` times and must raise a socket.timeout
        within `timeout` + self.fuzz seconds.
        """
        self.sock.settimeout(timeout)
        method = getattr(self.sock, method)
        for i in range(count):
            t1 = time.monotonic()
            try:
                method(*args)
            except socket.timeout as e:
                delta = time.monotonic() - t1
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
        self.addr_remote = resolve_address('www.python.org.', 80)

    def tearDown(self):
        self.sock.close()

    @unittest.skipIf(True, 'need to replace these hosts; see bpo-35518')
    def testConnectTimeout(self):
        # Testing connect timeout is tricky: we need to have IP connectivity
        # to a host that silently drops our packets.  We can't simulate this
        # from Python because it's a function of the underlying TCP/IP stack.
        # So, the following Snakebite host has been defined:
        blackhole = resolve_address('blackhole.snakebite.net', 56666)

        # Blackhole has been configured to silently drop any incoming packets.
        # No RSTs (for TCP) or ICMP UNREACH (for UDP/ICMP) will be sent back
        # to hosts that attempt to connect to this address: which is exactly
        # what we need to confidently test connect timeout.

        # However, we want to prevent false positives.  It's not unreasonable
        # to expect certain hosts may not be able to reach the blackhole, due
        # to firewalling or general network configuration.  In order to improve
        # our confidence in testing the blackhole, a corresponding 'whitehole'
        # has also been set up using one port higher:
        whitehole = resolve_address('whitehole.snakebite.net', 56667)

        # This address has been configured to immediately drop any incoming
        # packets as well, but it does it respectfully with regards to the
        # incoming protocol.  RSTs are sent for TCP packets, and ICMP UNREACH
        # is sent for UDP/ICMP packets.  This means our attempts to connect to
        # it should be met immediately with ECONNREFUSED.  The test case has
        # been structured around this premise: if we get an ECONNREFUSED from
        # the whitehole, we proceed with testing connect timeout against the
        # blackhole.  If we don't, we skip the test (with a message about not
        # getting the required RST from the whitehole within the required
        # timeframe).

        # For the records, the whitehole/blackhole configuration has been set
        # up using the 'pf' firewall (available on BSDs), using the following:
        #
        #   ext_if="bge0"
        #
        #   blackhole_ip="35.8.247.6"
        #   whitehole_ip="35.8.247.6"
        #   blackhole_port="56666"
        #   whitehole_port="56667"
        #
        #   block return in log quick on $ext_if proto { tcp udp } \
        #       from any to $whitehole_ip port $whitehole_port
        #   block drop in log quick on $ext_if proto { tcp udp } \
        #       from any to $blackhole_ip port $blackhole_port
        #

        skip = True
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        timeout = support.LOOPBACK_TIMEOUT
        sock.settimeout(timeout)
        try:
            sock.connect((whitehole))
        except socket.timeout:
            pass
        except OSError as err:
            if err.errno == errno.ECONNREFUSED:
                skip = False
        finally:
            sock.close()
            del sock

        if skip:
            self.skipTest(
                "We didn't receive a connection reset (RST) packet from "
                "{}:{} within {} seconds, so we're unable to test connect "
                "timeout against the corresponding {}:{} (which is "
                "configured to silently drop packets)."
                    .format(
                        whitehole[0],
                        whitehole[1],
                        timeout,
                        blackhole[0],
                        blackhole[1],
                    )
            )

        # All that hard work just to test if connect times out in 0.001s ;-)
        self.addr_remote = blackhole
        with socket_helper.transient_internet(self.addr_remote[0]):
            self._sock_operation(1, 0.001, 'connect', self.addr_remote)

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


def test_main():
    support.requires('network')
    support.run_unittest(
        CreationTestCase,
        TCPTimeoutTestCase,
        UDPTimeoutTestCase,
    )

if __name__ == "__main__":
    test_main()
