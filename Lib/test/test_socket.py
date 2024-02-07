import unittest
from test import test_support

import errno
import itertools
import socket
import select
import time
import traceback
import Queue
import sys
import os
import array
import contextlib
import signal
import math
import weakref
try:
    import _socket
except ImportError:
    _socket = None


MAIN_TIMEOUT = 60.0


def try_address(host, port=0, family=socket.AF_INET):
    """Try to bind a socket on the given host:port and return True
    if that has been possible."""
    try:
        sock = socket.socket(family, socket.SOCK_STREAM)
        sock.bind((host, port))
    except (socket.error, socket.gaierror):
        return False
    else:
        sock.close()
        return True

HOST = test_support.HOST
MSG = b'Michael Gilfix was here\n'
SUPPORTS_IPV6 = test_support.IPV6_ENABLED

try:
    import thread
    import threading
except ImportError:
    thread = None
    threading = None

HOST = test_support.HOST
MSG = 'Michael Gilfix was here\n'

def _is_fd_in_blocking_mode(sock):
    return not bool(
        fcntl.fcntl(sock, fcntl.F_GETFL, os.O_NONBLOCK) & os.O_NONBLOCK)


HAVE_SOCKET_CAN = _have_socket_can()

class SocketTCPTest(unittest.TestCase):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.port = test_support.bind_port(self.serv)
        self.serv.listen(1)

    def tearDown(self):
        self.serv.close()
        self.serv = None

class SocketUDPTest(unittest.TestCase):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.port = test_support.bind_port(self.serv)

    def tearDown(self):
        self.serv.close()
        self.serv = None

class ThreadableTest:
    """Threadable Test class

    The ThreadableTest class makes it easy to create a threaded
    client/server pair from an existing unit test. To create a
    new threaded class from an existing unit test, use multiple
    inheritance:

        class NewClass (OldClass, ThreadableTest):
            pass

    This class defines two new fixture functions with obvious
    purposes for overriding:

        clientSetUp ()
        clientTearDown ()

    Any new test functions within the class must then define
    tests in pairs, where the test name is preceded with a
    '_' to indicate the client portion of the test. Ex:

        def testFoo(self):
            # Server portion

        def _testFoo(self):
            # Client portion

    Any exceptions raised by the clients during their tests
    are caught and transferred to the main thread to alert
    the testing framework.

    Note, the server setup function cannot call any blocking
    functions that rely on the client thread during setup,
    unless serverExplicitReady() is called just before
    the blocking call (such as in setting up a client/server
    connection and performing the accept() in setUp().
    """

    def __init__(self):
        # Swap the true setup function
        self.__setUp = self.setUp
        self.__tearDown = self.tearDown
        self.setUp = self._setUp
        self.tearDown = self._tearDown

    def serverExplicitReady(self):
        """This method allows the server to explicitly indicate that
        it wants the client thread to proceed. This is useful if the
        server is about to execute a blocking routine that is
        dependent upon the client thread during its setup routine."""
        self.server_ready.set()

    def _setUp(self):
        self.server_ready = threading.Event()
        self.client_ready = threading.Event()
        self.done = threading.Event()
        self.queue = Queue.Queue(1)

        # Do some munging to start the client test.
        methodname = self.id()
        i = methodname.rfind('.')
        methodname = methodname[i+1:]
        test_method = getattr(self, '_' + methodname)
        self.client_thread = thread.start_new_thread(
            self.clientRun, (test_method,))

        self.__setUp()
        if not self.server_ready.is_set():
            self.server_ready.set()
        self.client_ready.wait()

    def _tearDown(self):
        self.__tearDown()
        self.done.wait()

        if not self.queue.empty():
            msg = self.queue.get()
            self.fail(msg)

    def clientRun(self, test_func):
        self.server_ready.wait()
        self.clientSetUp()
        self.client_ready.set()
        if not callable(test_func):
            raise TypeError("test_func must be a callable function.")
        try:
            test_func()
        except Exception, strerror:
            self.queue.put(strerror)
        self.clientTearDown()

    def clientSetUp(self):
        raise NotImplementedError("clientSetUp must be implemented.")

    def clientTearDown(self):
        self.done.set()
        thread.exit()

class ThreadedTCPSocketTest(SocketTCPTest, ThreadableTest):

    def __init__(self, methodName='runTest'):
        SocketTCPTest.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        self.cli = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)

class ThreadedUDPSocketTest(SocketUDPTest, ThreadableTest):

    def __init__(self, methodName='runTest'):
        SocketUDPTest.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        self.cli = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)

class SocketConnectedTest(ThreadedTCPSocketTest):

    def __init__(self, methodName='runTest'):
        ThreadedTCPSocketTest.__init__(self, methodName=methodName)

    def setUp(self):
        ThreadedTCPSocketTest.setUp(self)
        # Indicate explicitly we're ready for the client thread to
        # proceed and then perform the blocking call to accept
        self.serverExplicitReady()
        conn, addr = self.serv.accept()
        self.cli_conn = conn

    def tearDown(self):
        self.cli_conn.close()
        self.cli_conn = None
        ThreadedTCPSocketTest.tearDown(self)

    def clientSetUp(self):
        ThreadedTCPSocketTest.clientSetUp(self)
        self.cli.connect((HOST, self.port))
        self.serv_conn = self.cli

    def clientTearDown(self):
        self.serv_conn.close()
        self.serv_conn = None
        ThreadedTCPSocketTest.clientTearDown(self)

class SocketPairTest(unittest.TestCase, ThreadableTest):

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def setUp(self):
        self.serv, self.cli = socket.socketpair()

    def tearDown(self):
        self.serv.close()
        self.serv = None

    def clientSetUp(self):
        pass

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)


#######################################################################
## Begin Tests

class GeneralModuleTests(unittest.TestCase):

    @unittest.skipUnless(_socket is not None, 'need _socket module')
    def test_csocket_repr(self):
        s = _socket.socket(_socket.AF_INET, _socket.SOCK_STREAM)
        try:
            expected = ('<socket object, fd=%s, family=%s, type=%s, protocol=%s>'
                        % (s.fileno(), s.family, s.type, s.proto))
            self.assertEqual(repr(s), expected)
        finally:
            s.close()
        expected = ('<socket object, fd=-1, family=%s, type=%s, protocol=%s>'
                    % (s.family, s.type, s.proto))
        self.assertEqual(repr(s), expected)

    def test_weakref(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        p = weakref.proxy(s)
        self.assertEqual(p.fileno(), s.fileno())
        s.close()
        s = None
        try:
            p.fileno()
        except ReferenceError:
            pass
        else:
            self.fail('Socket proxy still exists')

    def test_weakref__sock(self):
        s = socket.socket()._sock
        w = weakref.ref(s)
        self.assertIs(w(), s)
        del s
        test_support.gc_collect()
        self.assertIsNone(w())

    def testSocketError(self):
        # Testing socket module exceptions
        def raise_error(*args, **kwargs):
            raise socket.error
        def raise_herror(*args, **kwargs):
            raise socket.herror
        def raise_gaierror(*args, **kwargs):
            raise socket.gaierror
        self.assertRaises(socket.error, raise_error,
                              "Error raising socket exception.")
        self.assertRaises(socket.error, raise_herror,
                              "Error raising socket exception.")
        self.assertRaises(socket.error, raise_gaierror,
                              "Error raising socket exception.")

    def testSendtoErrors(self):
        # Testing that sendto doesn't mask failures. See #10169.
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.addCleanup(s.close)
        s.bind(('', 0))
        sockname = s.getsockname()
        # 2 args
        with self.assertRaises(UnicodeEncodeError):
            s.sendto(u'\u2620', sockname)
        with self.assertRaises(TypeError) as cm:
            s.sendto(5j, sockname)
        self.assertIn('not complex', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            s.sendto('foo', None)
        self.assertIn('not NoneType', str(cm.exception))
        # 3 args
        with self.assertRaises(UnicodeEncodeError):
            s.sendto(u'\u2620', 0, sockname)
        with self.assertRaises(TypeError) as cm:
            s.sendto(5j, 0, sockname)
        self.assertIn('not complex', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            s.sendto('foo', 0, None)
        self.assertIn('not NoneType', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            s.sendto('foo', 'bar', sockname)
        self.assertIn('an integer is required', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            s.sendto('foo', None, None)
        self.assertIn('an integer is required', str(cm.exception))
        # wrong number of args
        with self.assertRaises(TypeError) as cm:
            s.sendto('foo')
        self.assertIn('(1 given)', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            s.sendto('foo', 0, sockname, 4)
        self.assertIn('(4 given)', str(cm.exception))


    def testCrucialConstants(self):
        # Testing for mission critical constants
        socket.AF_INET
        socket.SOCK_STREAM
        socket.SOCK_DGRAM
        socket.SOCK_RAW
        socket.SOCK_RDM
        socket.SOCK_SEQPACKET
        socket.SOL_SOCKET
        socket.SO_REUSEADDR

    def testHostnameRes(self):
        # Testing hostname resolution mechanisms
        hostname = socket.gethostname()
        try:
            ip = socket.gethostbyname(hostname)
        except socket.error:
            # Probably name lookup wasn't set up right; skip this test
            self.skipTest('name lookup failure')
        self.assertTrue(ip.find('.') >= 0, "Error resolving host to ip.")
        try:
            hname, aliases, ipaddrs = socket.gethostbyaddr(ip)
        except socket.error:
            # Probably a similar problem as above; skip this test
            self.skipTest('address lookup failure')
        all_host_names = [hostname, hname] + aliases
        fqhn = socket.getfqdn(ip)
        if not fqhn in all_host_names:
            self.fail("Error testing host resolution mechanisms. (fqdn: %s, all: %s)" % (fqhn, repr(all_host_names)))

    @unittest.skipUnless(hasattr(sys, 'getrefcount'),
                         'test needs sys.getrefcount()')
    def testRefCountGetNameInfo(self):
        # Testing reference count for getnameinfo
        try:
            # On some versions, this loses a reference
            orig = sys.getrefcount(__name__)
            socket.getnameinfo(__name__,0)
        except TypeError:
            self.assertEqual(sys.getrefcount(__name__), orig,
                             "socket.getnameinfo loses a reference")

    def testInterpreterCrash(self):
        # Making sure getnameinfo doesn't crash the interpreter
        try:
            # On some versions, this crashes the interpreter.
            socket.getnameinfo(('x', 0, 0, 0), 0)
        except socket.error:
            pass

    def testNtoH(self):
        # This just checks that htons etc. are their own inverse,
        # when looking at the lower 16 or 32 bits.
        sizes = {socket.htonl: 32, socket.ntohl: 32,
                 socket.htons: 16, socket.ntohs: 16}
        for func, size in sizes.items():
            mask = (1L<<size) - 1
            for i in (0, 1, 0xffff, ~0xffff, 2, 0x01234567, 0x76543210):
                self.assertEqual(i & mask, func(func(i&mask)) & mask)

            swapped = func(mask)
            self.assertEqual(swapped & mask, mask)
            self.assertRaises(OverflowError, func, 1L<<34)

    def testNtoHErrors(self):
        good_values = [ 1, 2, 3, 1L, 2L, 3L ]
        bad_values = [ -1, -2, -3, -1L, -2L, -3L ]
        for k in good_values:
            socket.ntohl(k)
            socket.ntohs(k)
            socket.htonl(k)
            socket.htons(k)
        for k in bad_values:
            self.assertRaises(OverflowError, socket.ntohl, k)
            self.assertRaises(OverflowError, socket.ntohs, k)
            self.assertRaises(OverflowError, socket.htonl, k)
            self.assertRaises(OverflowError, socket.htons, k)

    def testGetServBy(self):
        eq = self.assertEqual
        # Find one service that exists, then check all the related interfaces.
        # I've ordered this by protocols that have both a tcp and udp
        # protocol, at least for modern Linuxes.
        if (sys.platform.startswith('linux') or
            sys.platform.startswith('freebsd') or
            sys.platform.startswith('netbsd') or
            sys.platform == 'darwin'):
            # avoid the 'echo' service on this platform, as there is an
            # assumption breaking non-standard port/protocol entry
            services = ('daytime', 'qotd', 'domain')
        else:
            services = ('echo', 'daytime', 'domain')
        for service in services:
            try:
                port = socket.getservbyname(service, 'tcp')
                break
            except socket.error:
                pass
        else:
            raise socket.error
        # Try same call with optional protocol omitted
        port2 = socket.getservbyname(service)
        eq(port, port2)
        # Try udp, but don't barf if it doesn't exist
        try:
            udpport = socket.getservbyname(service, 'udp')
        except socket.error:
            udpport = None
        else:
            eq(udpport, port)
        # Now make sure the lookup by port returns the same service name
        eq(socket.getservbyport(port2), service)
        eq(socket.getservbyport(port, 'tcp'), service)
        if udpport is not None:
            eq(socket.getservbyport(udpport, 'udp'), service)
        # Make sure getservbyport does not accept out of range ports.
        self.assertRaises(OverflowError, socket.getservbyport, -1)
        self.assertRaises(OverflowError, socket.getservbyport, 65536)

    def testDefaultTimeout(self):
        # Testing default timeout
        # The default timeout should initially be None
        self.assertEqual(socket.getdefaulttimeout(), None)
        s = socket.socket()
        self.assertEqual(s.gettimeout(), None)
        s.close()

        # Set the default timeout to 10, and see if it propagates
        socket.setdefaulttimeout(10)
        self.assertEqual(socket.getdefaulttimeout(), 10)
        s = socket.socket()
        self.assertEqual(s.gettimeout(), 10)
        s.close()

        # Reset the default timeout to None, and see if it propagates
        socket.setdefaulttimeout(None)
        self.assertEqual(socket.getdefaulttimeout(), None)
        s = socket.socket()
        self.assertEqual(s.gettimeout(), None)
        s.close()

        # Check that setting it to an invalid value raises ValueError
        self.assertRaises(ValueError, socket.setdefaulttimeout, -1)

        # Check that setting it to an invalid type raises TypeError
        self.assertRaises(TypeError, socket.setdefaulttimeout, "spam")

    @unittest.skipUnless(hasattr(socket, 'inet_aton'),
                         'test needs socket.inet_aton()')
    def testIPv4_inet_aton_fourbytes(self):
        # Test that issue1008086 and issue767150 are fixed.
        # It must return 4 bytes.
        self.assertEqual('\x00'*4, socket.inet_aton('0.0.0.0'))
        self.assertEqual('\xff'*4, socket.inet_aton('255.255.255.255'))

    @unittest.skipUnless(hasattr(socket, 'inet_pton'),
                         'test needs socket.inet_pton()')
    def testIPv4toString(self):
        from socket import inet_aton as f, inet_pton, AF_INET
        g = lambda a: inet_pton(AF_INET, a)

        self.assertEqual('\x00\x00\x00\x00', f('0.0.0.0'))
        self.assertEqual('\xff\x00\xff\x00', f('255.0.255.0'))
        self.assertEqual('\xaa\xaa\xaa\xaa', f('170.170.170.170'))
        self.assertEqual('\x01\x02\x03\x04', f('1.2.3.4'))
        self.assertEqual('\xff\xff\xff\xff', f('255.255.255.255'))

        self.assertEqual('\x00\x00\x00\x00', g('0.0.0.0'))
        self.assertEqual('\xff\x00\xff\x00', g('255.0.255.0'))
        self.assertEqual('\xaa\xaa\xaa\xaa', g('170.170.170.170'))
        self.assertEqual('\xff\xff\xff\xff', g('255.255.255.255'))

    @unittest.skipUnless(hasattr(socket, 'inet_pton'),
                         'test needs socket.inet_pton()')
    def testIPv6toString(self):
        try:
            from socket import inet_pton, AF_INET6, has_ipv6
            if not has_ipv6:
                self.skipTest('IPv6 not available')
        except ImportError:
            self.skipTest('could not import needed symbols from socket')
        f = lambda a: inet_pton(AF_INET6, a)

        self.assertEqual('\x00' * 16, f('::'))
        self.assertEqual('\x00' * 16, f('0::0'))
        self.assertEqual('\x00\x01' + '\x00' * 14, f('1::'))
        self.assertEqual(
            '\x45\xef\x76\xcb\x00\x1a\x56\xef\xaf\xeb\x0b\xac\x19\x24\xae\xae',
            f('45ef:76cb:1a:56ef:afeb:bac:1924:aeae')
        )

    @unittest.skipUnless(hasattr(socket, 'inet_ntop'),
                         'test needs socket.inet_ntop()')
    def testStringToIPv4(self):
        from socket import inet_ntoa as f, inet_ntop, AF_INET
        g = lambda a: inet_ntop(AF_INET, a)

        self.assertEqual('1.0.1.0', f('\x01\x00\x01\x00'))
        self.assertEqual('170.85.170.85', f('\xaa\x55\xaa\x55'))
        self.assertEqual('255.255.255.255', f('\xff\xff\xff\xff'))
        self.assertEqual('1.2.3.4', f('\x01\x02\x03\x04'))

        self.assertEqual('1.0.1.0', g('\x01\x00\x01\x00'))
        self.assertEqual('170.85.170.85', g('\xaa\x55\xaa\x55'))
        self.assertEqual('255.255.255.255', g('\xff\xff\xff\xff'))

    @unittest.skipUnless(hasattr(socket, 'inet_ntop'),
                         'test needs socket.inet_ntop()')
    def testStringToIPv6(self):
        try:
            from socket import inet_ntop, AF_INET6, has_ipv6
            if not has_ipv6:
                self.skipTest('IPv6 not available')
        except ImportError:
            self.skipTest('could not import needed symbols from socket')
        f = lambda a: inet_ntop(AF_INET6, a)

        self.assertEqual('::', f('\x00' * 16))
        self.assertEqual('::1', f('\x00' * 15 + '\x01'))
        self.assertEqual(
            'aef:b01:506:1001:ffff:9997:55:170',
            f('\x0a\xef\x0b\x01\x05\x06\x10\x01\xff\xff\x99\x97\x00\x55\x01\x70')
        )

    # XXX The following don't test module-level functionality...

    def _get_unused_port(self, bind_address='0.0.0.0'):
        """Use a temporary socket to elicit an unused ephemeral port.

        Args:
            bind_address: Hostname or IP address to search for a port on.

        Returns: A most likely to be unused port.
        """
        tempsock = socket.socket()
        tempsock.bind((bind_address, 0))
        host, port = tempsock.getsockname()
        tempsock.close()
        return port

    def testSockName(self):
        # Testing getsockname()
        port = self._get_unused_port()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(sock.close)
        sock.bind(("0.0.0.0", port))
        name = sock.getsockname()
        # XXX(nnorwitz): http://tinyurl.com/os5jz seems to indicate
        # it reasonable to get the host's addr in addition to 0.0.0.0.
        # At least for eCos.  This is required for the S/390 to pass.
        try:
            my_ip_addr = socket.gethostbyname(socket.gethostname())
        except socket.error:
            # Probably name lookup wasn't set up right; skip this test
            self.skipTest('name lookup failure')
        self.assertIn(name[0], ("0.0.0.0", my_ip_addr), '%s invalid' % name[0])
        self.assertEqual(name[1], port)

    def testGetSockOpt(self):
        # Testing getsockopt()
        # We know a socket should start without reuse==0
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(sock.close)
        reuse = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR)
        self.assertFalse(reuse != 0, "initial mode is reuse")

    def testSetSockOpt(self):
        # Testing setsockopt()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(sock.close)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        reuse = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR)
        self.assertFalse(reuse == 0, "failed to set reuse mode")

    def testSendAfterClose(self):
        # testing send() after close() with timeout
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(1)
        sock.close()
        self.assertRaises(socket.error, sock.send, "spam")

    def testNewAttributes(self):
        # testing .family, .type and .protocol
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.assertEqual(sock.family, socket.AF_INET)
        self.assertEqual(sock.type, socket.SOCK_STREAM)
        self.assertEqual(sock.proto, 0)
        sock.close()

    def test_getsockaddrarg(self):
        sock = socket.socket()
        self.addCleanup(sock.close)
        port = test_support.find_unused_port()
        big_port = port + 65536
        neg_port = port - 65536
        self.assertRaises(OverflowError, sock.bind, (HOST, big_port))
        self.assertRaises(OverflowError, sock.bind, (HOST, neg_port))
        # Since find_unused_port() is inherently subject to race conditions, we
        # call it a couple times if necessary.
        for i in itertools.count():
            port = test_support.find_unused_port()
            try:
                sock.bind((HOST, port))
            except OSError as e:
                if e.errno != errno.EADDRINUSE or i == 5:
                    raise
            else:
                break

    @unittest.skipUnless(os.name == "nt", "Windows specific")
    def test_sock_ioctl(self):
        self.assertTrue(hasattr(socket.socket, 'ioctl'))
        self.assertTrue(hasattr(socket, 'SIO_RCVALL'))
        self.assertTrue(hasattr(socket, 'RCVALL_ON'))
        self.assertTrue(hasattr(socket, 'RCVALL_OFF'))
        self.assertTrue(hasattr(socket, 'SIO_KEEPALIVE_VALS'))
        s = socket.socket()
        self.addCleanup(s.close)
        self.assertRaises(ValueError, s.ioctl, -1, None)
        s.ioctl(socket.SIO_KEEPALIVE_VALS, (1, 100, 100))

    def testGetaddrinfo(self):
        try:
            socket.getaddrinfo('localhost', 80)
        except socket.gaierror as err:
            if err.errno == socket.EAI_SERVICE:
                # see http://bugs.python.org/issue1282647
                self.skipTest("buggy libc version")
            raise
        # len of every sequence is supposed to be == 5
        for info in socket.getaddrinfo(HOST, None):
            self.assertEqual(len(info), 5)
        # host can be a domain name, a string representation of an
        # IPv4/v6 address or None
        socket.getaddrinfo('localhost', 80)
        socket.getaddrinfo('127.0.0.1', 80)
        socket.getaddrinfo(None, 80)
        if SUPPORTS_IPV6:
            socket.getaddrinfo('::1', 80)
        # port can be a string service name such as "http", a numeric
        # port number (int or long), or None
        socket.getaddrinfo(HOST, "http")
        socket.getaddrinfo(HOST, 80)
        socket.getaddrinfo(HOST, 80L)
        socket.getaddrinfo(HOST, None)
        # test family and socktype filters
        infos = socket.getaddrinfo(HOST, None, socket.AF_INET)
        for family, _, _, _, _ in infos:
            self.assertEqual(family, socket.AF_INET)
        infos = socket.getaddrinfo(HOST, None, 0, socket.SOCK_STREAM)
        for _, socktype, _, _, _ in infos:
            self.assertEqual(socktype, socket.SOCK_STREAM)
        # test proto and flags arguments
        socket.getaddrinfo(HOST, None, 0, 0, socket.SOL_TCP)
        socket.getaddrinfo(HOST, None, 0, 0, 0, socket.AI_PASSIVE)
        # a server willing to support both IPv4 and IPv6 will
        # usually do this
        socket.getaddrinfo(None, 0, socket.AF_UNSPEC, socket.SOCK_STREAM, 0,
                           socket.AI_PASSIVE)

        # Issue 17269: test workaround for OS X platform bug segfault
        if hasattr(socket, 'AI_NUMERICSERV'):
            try:
                # The arguments here are undefined and the call may succeed
                # or fail.  All we care here is that it doesn't segfault.
                socket.getaddrinfo("localhost", None, 0, 0, 0,
                                   socket.AI_NUMERICSERV)
            except socket.gaierror:
                pass

    def check_sendall_interrupted(self, with_timeout):
        # socketpair() is not strictly required, but it makes things easier.
        if not hasattr(signal, 'alarm') or not hasattr(socket, 'socketpair'):
            self.skipTest("signal.alarm and socket.socketpair required for this test")
        # Our signal handlers clobber the C errno by calling a math function
        # with an invalid domain value.
        def ok_handler(*args):
            self.assertRaises(ValueError, math.acosh, 0)
        def raising_handler(*args):
            self.assertRaises(ValueError, math.acosh, 0)
            1 // 0
        c, s = socket.socketpair()
        old_alarm = signal.signal(signal.SIGALRM, raising_handler)
        try:
            if with_timeout:
                # Just above the one second minimum for signal.alarm
                c.settimeout(1.5)
            with self.assertRaises(ZeroDivisionError):
                signal.alarm(1)
                c.sendall(b"x" * test_support.SOCK_MAX_SIZE)
            if with_timeout:
                signal.signal(signal.SIGALRM, ok_handler)
                signal.alarm(1)
                self.assertRaises(socket.timeout, c.sendall,
                                  b"x" * test_support.SOCK_MAX_SIZE)
        finally:
            signal.alarm(0)
            signal.signal(signal.SIGALRM, old_alarm)
            c.close()
            s.close()

    def test_sendall_interrupted(self):
        self.check_sendall_interrupted(False)

    def test_sendall_interrupted_with_timeout(self):
        self.check_sendall_interrupted(True)

    def test_listen_backlog(self):
        for backlog in 0, -1:
            srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            srv.bind((HOST, 0))
            srv.listen(backlog)
            srv.close()

    @test_support.cpython_only
    def test_listen_backlog_overflow(self):
        # Issue 15989
        import _testcapi
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.bind((HOST, 0))
        self.assertRaises(OverflowError, srv.listen, _testcapi.INT_MAX + 1)
        srv.close()

    @unittest.skipUnless(SUPPORTS_IPV6, 'IPv6 required for this test.')
    def test_flowinfo(self):
        self.assertRaises(OverflowError, socket.getnameinfo,
                          ('::1',0, 0xffffffff), 0)
        s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
        try:
            self.assertRaises(OverflowError, s.bind, ('::1', 0, -10))
        finally:
            s.close()


@unittest.skipUnless(thread, 'Threading required for this test.')
class BasicTCPTest(SocketConnectedTest):

    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def testRecv(self):
        # Testing large receive over TCP
        msg = self.cli_conn.recv(1024)
        self.assertEqual(msg, MSG)

    def _testRecv(self):
        self.serv_conn.send(MSG)

    def testOverFlowRecv(self):
        # Testing receive in chunks over TCP
        seg1 = self.cli_conn.recv(len(MSG) - 3)
        seg2 = self.cli_conn.recv(1024)
        msg = seg1 + seg2
        self.assertEqual(msg, MSG)

    def _testOverFlowRecv(self):
        self.serv_conn.send(MSG)

    def testRecvFrom(self):
        # Testing large recvfrom() over TCP
        msg, addr = self.cli_conn.recvfrom(1024)
        self.assertEqual(msg, MSG)

    def _testRecvFrom(self):
        self.serv_conn.send(MSG)

    def testOverFlowRecvFrom(self):
        # Testing recvfrom() in chunks over TCP
        seg1, addr = self.cli_conn.recvfrom(len(MSG)-3)
        seg2, addr = self.cli_conn.recvfrom(1024)
        msg = seg1 + seg2
        self.assertEqual(msg, MSG)

    def _testOverFlowRecvFrom(self):
        self.serv_conn.send(MSG)

    def testSendAll(self):
        # Testing sendall() with a 2048 byte string over TCP
        msg = ''
        while 1:
            read = self.cli_conn.recv(1024)
            if not read:
                break
            msg += read
        self.assertEqual(msg, 'f' * 2048)

    def _testSendAll(self):
        big_chunk = 'f' * 2048
        self.serv_conn.sendall(big_chunk)

    @unittest.skipUnless(hasattr(socket, 'fromfd'),
                         'socket.fromfd not available')
    def testFromFd(self):
        # Testing fromfd()
        fd = self.cli_conn.fileno()
        sock = socket.fromfd(fd, socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(sock.close)
        msg = sock.recv(1024)
        self.assertEqual(msg, MSG)

    def _testFromFd(self):
        self.serv_conn.send(MSG)

    def testDup(self):
        # Testing dup()
        sock = self.cli_conn.dup()
        self.addCleanup(sock.close)
        msg = sock.recv(1024)
        self.assertEqual(msg, MSG)

    def _testDup(self):
        self.serv_conn.send(MSG)

    def testShutdown(self):
        # Testing shutdown()
        msg = self.cli_conn.recv(1024)
        self.assertEqual(msg, MSG)
        # wait for _testShutdown to finish: on OS X, when the server
        # closes the connection the client also becomes disconnected,
        # and the client's shutdown call will fail. (Issue #4397.)
        self.done.wait()

    def _testShutdown(self):
        self.serv_conn.send(MSG)
        self.serv_conn.shutdown(2)

    testShutdown_overflow = test_support.cpython_only(testShutdown)

    @test_support.cpython_only
    def _testShutdown_overflow(self):
        import _testcapi
        self.serv_conn.send(MSG)
        # Issue 15989
        self.assertRaises(OverflowError, self.serv_conn.shutdown,
                          _testcapi.INT_MAX + 1)
        self.assertRaises(OverflowError, self.serv_conn.shutdown,
                          2 + (_testcapi.UINT_MAX + 1))
        self.serv_conn.shutdown(2)

@unittest.skipUnless(thread, 'Threading required for this test.')
class BasicUDPTest(ThreadedUDPSocketTest):

    def __init__(self, methodName='runTest'):
        ThreadedUDPSocketTest.__init__(self, methodName=methodName)

    def testSendtoAndRecv(self):
        # Testing sendto() and Recv() over UDP
        msg = self.serv.recv(len(MSG))
        self.assertEqual(msg, MSG)

    def _testSendtoAndRecv(self):
        self.cli.sendto(MSG, 0, (HOST, self.port))

    def testRecvFrom(self):
        # Testing recvfrom() over UDP
        msg, addr = self.serv.recvfrom(len(MSG))
        self.assertEqual(msg, MSG)

    def _testRecvFrom(self):
        self.cli.sendto(MSG, 0, (HOST, self.port))

    def testRecvFromNegative(self):
        # Negative lengths passed to recvfrom should give ValueError.
        self.assertRaises(ValueError, self.serv.recvfrom, -1)

    def _testRecvFromNegative(self):
        self.cli.sendto(MSG, 0, (HOST, self.port))

@unittest.skipUnless(thread, 'Threading required for this test.')
class TCPCloserTest(ThreadedTCPSocketTest):

    def testClose(self):
        conn, addr = self.serv.accept()
        conn.close()

        sd = self.cli
        read, write, err = select.select([sd], [], [], 1.0)
        self.assertEqual(read, [sd])
        self.assertEqual(sd.recv(1), '')

    def _testClose(self):
        self.cli.connect((HOST, self.port))
        time.sleep(1.0)

@unittest.skipUnless(hasattr(socket, 'socketpair'),
                     'test needs socket.socketpair()')
@unittest.skipUnless(thread, 'Threading required for this test.')
class BasicSocketPairTest(SocketPairTest):

    def __init__(self, methodName='runTest'):
        SocketPairTest.__init__(self, methodName=methodName)

    def testRecv(self):
        msg = self.serv.recv(1024)
        self.assertEqual(msg, MSG)

    def _testRecv(self):
        self.cli.send(MSG)

    def testSend(self):
        self.serv.send(MSG)

    def _testSend(self):
        msg = self.cli.recv(1024)
        self.assertEqual(msg, MSG)

@unittest.skipUnless(thread, 'Threading required for this test.')
class NonBlockingTCPTests(ThreadedTCPSocketTest):

    def __init__(self, methodName='runTest'):
        self.event = threading.Event()
        ThreadedTCPSocketTest.__init__(self, methodName=methodName)

    def testSetBlocking(self):
        # Testing whether set blocking works
        self.serv.setblocking(True)
        self.assertIsNone(self.serv.gettimeout())
        self.assertTrue(self.serv.getblocking())
        if fcntl:
            self.assertTrue(_is_fd_in_blocking_mode(self.serv))

        self.serv.setblocking(False)
        self.assertEqual(self.serv.gettimeout(), 0.0)
        self.assertFalse(self.serv.getblocking())
        if fcntl:
            self.assertFalse(_is_fd_in_blocking_mode(self.serv))

        self.serv.settimeout(None)
        self.assertTrue(self.serv.getblocking())
        if fcntl:
            self.assertTrue(_is_fd_in_blocking_mode(self.serv))

        self.serv.settimeout(0)
        self.assertFalse(self.serv.getblocking())
        self.assertEqual(self.serv.gettimeout(), 0)
        if fcntl:
            self.assertFalse(_is_fd_in_blocking_mode(self.serv))

        self.serv.settimeout(10)
        self.assertTrue(self.serv.getblocking())
        self.assertEqual(self.serv.gettimeout(), 10)
        if fcntl:
            # When a Python socket has a non-zero timeout, it's
            # switched internally to a non-blocking mode.
            # Later, sock.sendall(), sock.recv(), and other socket
            # operations use a `select()` call and handle EWOULDBLOCK/EGAIN
            # on all socket operations.  That's how timeouts are
            # enforced.
            self.assertFalse(_is_fd_in_blocking_mode(self.serv))

        self.serv.settimeout(0)
        self.assertFalse(self.serv.getblocking())
        if fcntl:
            self.assertFalse(_is_fd_in_blocking_mode(self.serv))

        start = time.time()
        try:
            self.serv.accept()
        except socket.error:
            pass
        end = time.time()
        self.assertTrue((end - start) < 1.0, "Error setting non-blocking mode.")

    def _testSetBlocking(self):
        pass

    @test_support.cpython_only
    def testSetBlocking_overflow(self):
        # Issue 15989
        import _testcapi
        if _testcapi.UINT_MAX >= _testcapi.ULONG_MAX:
            self.skipTest('needs UINT_MAX < ULONG_MAX')
        self.serv.setblocking(False)
        self.assertEqual(self.serv.gettimeout(), 0.0)
        self.serv.setblocking(_testcapi.UINT_MAX + 1)
        self.assertIsNone(self.serv.gettimeout())

    _testSetBlocking_overflow = test_support.cpython_only(_testSetBlocking)

    @unittest.skipUnless(hasattr(socket, 'SOCK_NONBLOCK'),
                         'test needs socket.SOCK_NONBLOCK')
    @support.requires_linux_version(2, 6, 28)
    def testInitNonBlocking(self):
        # reinit server socket
        self.serv.close()
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM |
                                                  socket.SOCK_NONBLOCK)
        self.assertFalse(self.serv.getblocking())
        self.assertEqual(self.serv.gettimeout(), 0)
        self.port = support.bind_port(self.serv)
        self.serv.listen()
        # actual testing
        start = time.time()
        try:
            self.serv.accept()
        except OSError:
            pass
        end = time.time()
        self.assertTrue((end - start) < 1.0, "Error creating with non-blocking mode.")

    def _testInitNonBlocking(self):
        pass

    def testInheritFlags(self):
        # Issue #7995: when calling accept() on a listening socket with a
        # timeout, the resulting socket should not be non-blocking.
        self.serv.settimeout(10)
        try:
            conn, addr = self.serv.accept()
            message = conn.recv(len(MSG))
        finally:
            conn.close()
            self.serv.settimeout(None)

    def _testInheritFlags(self):
        time.sleep(0.1)
        self.cli.connect((HOST, self.port))
        time.sleep(0.5)
        self.cli.send(MSG)

    def testAccept(self):
        # Testing non-blocking accept
        self.serv.setblocking(0)

        # connect() didn't start: non-blocking accept() fails
        with self.assertRaises(socket.error):
            conn, addr = self.serv.accept()

        self.event.set()

        read, write, err = select.select([self.serv], [], [], MAIN_TIMEOUT)
        if self.serv not in read:
            self.fail("Error trying to do accept after select.")

        # connect() completed: non-blocking accept() doesn't block
        conn, addr = self.serv.accept()
        self.addCleanup(conn.close)
        self.assertIsNone(conn.gettimeout())

    def _testAccept(self):
        # don't connect before event is set to check
        # that non-blocking accept() raises socket.error
        self.event.wait()

        self.cli.connect((HOST, self.port))

    def testConnect(self):
        # Testing non-blocking connect
        conn, addr = self.serv.accept()
        conn.close()

    def _testConnect(self):
        self.cli.settimeout(10)
        self.cli.connect((HOST, self.port))

    def testRecv(self):
        # Testing non-blocking recv
        conn, addr = self.serv.accept()
        self.addCleanup(conn.close)
        conn.setblocking(0)

        # the server didn't send data yet: non-blocking recv() fails
        with self.assertRaises(socket.error):
            msg = conn.recv(len(MSG))

        self.event.set()

        read, write, err = select.select([conn], [], [], MAIN_TIMEOUT)
        if conn not in read:
            self.fail("Error during select call to non-blocking socket.")

        # the server sent data yet: non-blocking recv() doesn't block
        msg = conn.recv(len(MSG))
        self.assertEqual(msg, MSG)

    def _testRecv(self):
        self.cli.connect((HOST, self.port))

        # don't send anything before event is set to check
        # that non-blocking recv() raises socket.error
        self.event.wait()

        # send data: recv() will no longer block
        self.cli.sendall(MSG)

@unittest.skipUnless(thread, 'Threading required for this test.')
class FileObjectClassTestCase(SocketConnectedTest):

    bufsize = -1 # Use default buffer size

    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def setUp(self):
        SocketConnectedTest.setUp(self)
        self.serv_file = self.cli_conn.makefile('rb', self.bufsize)

    def tearDown(self):
        self.serv_file.close()
        self.assertTrue(self.serv_file.closed)
        SocketConnectedTest.tearDown(self)
        self.serv_file = None

    def clientSetUp(self):
        SocketConnectedTest.clientSetUp(self)
        self.cli_file = self.serv_conn.makefile('wb')

    def clientTearDown(self):
        self.cli_file.close()
        self.assertTrue(self.cli_file.closed)
        self.cli_file = None
        SocketConnectedTest.clientTearDown(self)

    def testSmallRead(self):
        # Performing small file read test
        first_seg = self.serv_file.read(len(MSG)-3)
        second_seg = self.serv_file.read(3)
        msg = first_seg + second_seg
        self.assertEqual(msg, MSG)

    def _testSmallRead(self):
        self.cli_file.write(MSG)
        self.cli_file.flush()

    def testFullRead(self):
        # read until EOF
        msg = self.serv_file.read()
        self.assertEqual(msg, MSG)

    def _testFullRead(self):
        self.cli_file.write(MSG)
        self.cli_file.close()

    def testUnbufferedRead(self):
        # Performing unbuffered file read test
        buf = ''
        while 1:
            char = self.serv_file.read(1)
            if not char:
                break
            buf += char
        self.assertEqual(buf, MSG)

    def _testUnbufferedRead(self):
        self.cli_file.write(MSG)
        self.cli_file.flush()

    def testReadline(self):
        # Performing file readline test
        line = self.serv_file.readline()
        self.assertEqual(line, MSG)

    def _testReadline(self):
        self.cli_file.write(MSG)
        self.cli_file.flush()

    def testReadlineAfterRead(self):
        a_baloo_is = self.serv_file.read(len("A baloo is"))
        self.assertEqual("A baloo is", a_baloo_is)
        _a_bear = self.serv_file.read(len(" a bear"))
        self.assertEqual(" a bear", _a_bear)
        line = self.serv_file.readline()
        self.assertEqual("\n", line)
        line = self.serv_file.readline()
        self.assertEqual("A BALOO IS A BEAR.\n", line)
        line = self.serv_file.readline()
        self.assertEqual(MSG, line)

    def _testReadlineAfterRead(self):
        self.cli_file.write("A baloo is a bear\n")
        self.cli_file.write("A BALOO IS A BEAR.\n")
        self.cli_file.write(MSG)
        self.cli_file.flush()

    def testReadlineAfterReadNoNewline(self):
        end_of_ = self.serv_file.read(len("End Of "))
        self.assertEqual("End Of ", end_of_)
        line = self.serv_file.readline()
        self.assertEqual("Line", line)

    def _testReadlineAfterReadNoNewline(self):
        self.cli_file.write("End Of Line")

    def testClosedAttr(self):
        self.assertTrue(not self.serv_file.closed)

    def _testClosedAttr(self):
        self.assertTrue(not self.cli_file.closed)


class FileObjectInterruptedTestCase(unittest.TestCase):
    """Test that the file object correctly handles EINTR internally."""

    class MockSocket(object):
        def __init__(self, recv_funcs=()):
            # A generator that returns callables that we'll call for each
            # call to recv().
            self._recv_step = iter(recv_funcs)

        def recv(self, size):
            return self._recv_step.next()()

    @staticmethod
    def _raise_eintr():
        raise socket.error(errno.EINTR)

    def _test_readline(self, size=-1, **kwargs):
        mock_sock = self.MockSocket(recv_funcs=[
                lambda : "This is the first line\nAnd the sec",
                self._raise_eintr,
                lambda : "ond line is here\n",
                lambda : "",
            ])
        fo = socket._fileobject(mock_sock, **kwargs)
        self.assertEqual(fo.readline(size), "This is the first line\n")
        self.assertEqual(fo.readline(size), "And the second line is here\n")

    def _test_read(self, size=-1, **kwargs):
        mock_sock = self.MockSocket(recv_funcs=[
                lambda : "This is the first line\nAnd the sec",
                self._raise_eintr,
                lambda : "ond line is here\n",
                lambda : "",
            ])
        fo = socket._fileobject(mock_sock, **kwargs)
        self.assertEqual(fo.read(size), "This is the first line\n"
                          "And the second line is here\n")

    def test_default(self):
        self._test_readline()
        self._test_readline(size=100)
        self._test_read()
        self._test_read(size=100)

    def test_with_1k_buffer(self):
        self._test_readline(bufsize=1024)
        self._test_readline(size=100, bufsize=1024)
        self._test_read(bufsize=1024)
        self._test_read(size=100, bufsize=1024)

    def _test_readline_no_buffer(self, size=-1):
        mock_sock = self.MockSocket(recv_funcs=[
                lambda : "aa",
                lambda : "\n",
                lambda : "BB",
                self._raise_eintr,
                lambda : "bb",
                lambda : "",
            ])
        fo = socket._fileobject(mock_sock, bufsize=0)
        self.assertEqual(fo.readline(size), "aa\n")
        self.assertEqual(fo.readline(size), "BBbb")

    def test_no_buffer(self):
        self._test_readline_no_buffer()
        self._test_readline_no_buffer(size=4)
        self._test_read(bufsize=0)
        self._test_read(size=100, bufsize=0)


class UnbufferedFileObjectClassTestCase(FileObjectClassTestCase):

    """Repeat the tests from FileObjectClassTestCase with bufsize==0.

    In this case (and in this case only), it should be possible to
    create a file object, read a line from it, create another file
    object, read another line from it, without loss of data in the
    first file object's buffer.  Note that httplib relies on this
    when reading multiple requests from the same socket."""

    bufsize = 0 # Use unbuffered mode

    def testUnbufferedReadline(self):
        # Read a line, create a new file object, read another line with it
        line = self.serv_file.readline() # first line
        self.assertEqual(line, "A. " + MSG) # first line
        self.serv_file = self.cli_conn.makefile('rb', 0)
        line = self.serv_file.readline() # second line
        self.assertEqual(line, "B. " + MSG) # second line

    def _testUnbufferedReadline(self):
        self.cli_file.write("A. " + MSG)
        self.cli_file.write("B. " + MSG)
        self.cli_file.flush()

class LineBufferedFileObjectClassTestCase(FileObjectClassTestCase):

    bufsize = 1 # Default-buffered for reading; line-buffered for writing

    class SocketMemo(object):
        """A wrapper to keep track of sent data, needed to examine write behaviour"""
        def __init__(self, sock):
            self._sock = sock
            self.sent = []

        def send(self, data, flags=0):
            n = self._sock.send(data, flags)
            self.sent.append(data[:n])
            return n

        def sendall(self, data, flags=0):
            self._sock.sendall(data, flags)
            self.sent.append(data)

        def __getattr__(self, attr):
            return getattr(self._sock, attr)

        def getsent(self):
            return [e.tobytes() if isinstance(e, memoryview) else e for e in self.sent]

    def setUp(self):
        FileObjectClassTestCase.setUp(self)
        self.serv_file._sock = self.SocketMemo(self.serv_file._sock)

    def testLinebufferedWrite(self):
        # Write two lines, in small chunks
        msg = MSG.strip()
        print >> self.serv_file, msg,
        print >> self.serv_file, msg

        # second line:
        print >> self.serv_file, msg,
        print >> self.serv_file, msg,
        print >> self.serv_file, msg

        # third line
        print >> self.serv_file, ''

        self.serv_file.flush()

        msg1 = "%s %s\n"%(msg, msg)
        msg2 =  "%s %s %s\n"%(msg, msg, msg)
        msg3 =  "\n"
        self.assertEqual(self.serv_file._sock.getsent(), [msg1, msg2, msg3])

    def _testLinebufferedWrite(self):
        msg = MSG.strip()
        msg1 = "%s %s\n"%(msg, msg)
        msg2 =  "%s %s %s\n"%(msg, msg, msg)
        msg3 =  "\n"
        l1 = self.cli_file.readline()
        self.assertEqual(l1, msg1)
        l2 = self.cli_file.readline()
        self.assertEqual(l2, msg2)
        l3 = self.cli_file.readline()
        self.assertEqual(l3, msg3)


class SmallBufferedFileObjectClassTestCase(FileObjectClassTestCase):

    bufsize = 2 # Exercise the buffering code


class NetworkConnectionTest(object):
    """Prove network connection."""
    def clientSetUp(self):
        # We're inherited below by BasicTCPTest2, which also inherits
        # BasicTCPTest, which defines self.port referenced below.
        self.cli = socket.create_connection((HOST, self.port))
        self.serv_conn = self.cli

class BasicTCPTest2(NetworkConnectionTest, BasicTCPTest):
    """Tests that NetworkConnection does not break existing TCP functionality.
    """

class NetworkConnectionNoServer(unittest.TestCase):
    class MockSocket(socket.socket):
        def connect(self, *args):
            raise socket.timeout('timed out')

    @contextlib.contextmanager
    def mocked_socket_module(self):
        """Return a socket which times out on connect"""
        old_socket = socket.socket
        socket.socket = self.MockSocket
        try:
            yield
        finally:
            socket.socket = old_socket

    def test_connect(self):
        port = test_support.find_unused_port()
        cli = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(cli.close)
        with self.assertRaises(socket.error) as cm:
            cli.connect((HOST, port))
        self.assertEqual(cm.exception.errno, errno.ECONNREFUSED)

    def test_create_connection(self):
        # Issue #9792: errors raised by create_connection() should have
        # a proper errno attribute.
        port = test_support.find_unused_port()
        with self.assertRaises(socket.error) as cm:
            socket.create_connection((HOST, port))

        # Issue #16257: create_connection() calls getaddrinfo() against
        # 'localhost'.  This may result in an IPV6 addr being returned
        # as well as an IPV4 one:
        #   >>> socket.getaddrinfo('localhost', port, 0, SOCK_STREAM)
        #   >>> [(2,  2, 0, '', ('127.0.0.1', 41230)),
        #        (26, 2, 0, '', ('::1', 41230, 0, 0))]
        #
        # create_connection() enumerates through all the addresses returned
        # and if it doesn't successfully bind to any of them, it propagates
        # the last exception it encountered.
        #
        # On Solaris, ENETUNREACH is returned in this circumstance instead
        # of ECONNREFUSED.  So, if that errno exists, add it to our list of
        # expected errnos.
        expected_errnos = [ errno.ECONNREFUSED, ]
        if hasattr(errno, 'ENETUNREACH'):
            expected_errnos.append(errno.ENETUNREACH)
        if hasattr(errno, 'EADDRNOTAVAIL'):
            # bpo-31910: socket.create_connection() fails randomly
            # with EADDRNOTAVAIL on Travis CI
            expected_errnos.append(errno.EADDRNOTAVAIL)

        self.assertIn(cm.exception.errno, expected_errnos)

    def test_create_connection_timeout(self):
        # Issue #9792: create_connection() should not recast timeout errors
        # as generic socket errors.
        with self.mocked_socket_module():
            with self.assertRaises(socket.timeout):
                socket.create_connection((HOST, 1234))


@unittest.skipUnless(thread, 'Threading required for this test.')
class NetworkConnectionAttributesTest(SocketTCPTest, ThreadableTest):

    def __init__(self, methodName='runTest'):
        SocketTCPTest.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        self.source_port = test_support.find_unused_port()

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)

    def _justAccept(self):
        conn, addr = self.serv.accept()
        conn.close()

    testFamily = _justAccept
    def _testFamily(self):
        self.cli = socket.create_connection((HOST, self.port), timeout=30)
        self.addCleanup(self.cli.close)
        self.assertEqual(self.cli.family, 2)

    testSourceAddress = _justAccept
    def _testSourceAddress(self):
        self.cli = socket.create_connection((HOST, self.port), timeout=30,
                source_address=('', self.source_port))
        self.addCleanup(self.cli.close)
        self.assertEqual(self.cli.getsockname()[1], self.source_port)
        # The port number being used is sufficient to show that the bind()
        # call happened.

    testTimeoutDefault = _justAccept
    def _testTimeoutDefault(self):
        # passing no explicit timeout uses socket's global default
        self.assertTrue(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(42)
        try:
            self.cli = socket.create_connection((HOST, self.port))
            self.addCleanup(self.cli.close)
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(self.cli.gettimeout(), 42)

    testTimeoutNone = _justAccept
    def _testTimeoutNone(self):
        # None timeout means the same as sock.settimeout(None)
        self.assertTrue(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            self.cli = socket.create_connection((HOST, self.port), timeout=None)
            self.addCleanup(self.cli.close)
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(self.cli.gettimeout(), None)

    testTimeoutValueNamed = _justAccept
    def _testTimeoutValueNamed(self):
        self.cli = socket.create_connection((HOST, self.port), timeout=30)
        self.assertEqual(self.cli.gettimeout(), 30)

    testTimeoutValueNonamed = _justAccept
    def _testTimeoutValueNonamed(self):
        self.cli = socket.create_connection((HOST, self.port), 30)
        self.addCleanup(self.cli.close)
        self.assertEqual(self.cli.gettimeout(), 30)

@unittest.skipUnless(thread, 'Threading required for this test.')
class NetworkConnectionBehaviourTest(SocketTCPTest, ThreadableTest):

    def __init__(self, methodName='runTest'):
        SocketTCPTest.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        pass

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)

    def testInsideTimeout(self):
        conn, addr = self.serv.accept()
        self.addCleanup(conn.close)
        time.sleep(3)
        conn.send("done!")
    testOutsideTimeout = testInsideTimeout

    def _testInsideTimeout(self):
        self.cli = sock = socket.create_connection((HOST, self.port))
        data = sock.recv(5)
        self.assertEqual(data, "done!")

    def _testOutsideTimeout(self):
        self.cli = sock = socket.create_connection((HOST, self.port), timeout=1)
        self.assertRaises(socket.timeout, lambda: sock.recv(5))


class Urllib2FileobjectTest(unittest.TestCase):

    # urllib2.HTTPHandler has "borrowed" socket._fileobject, and requires that
    # it close the socket if the close c'tor argument is true

    def testClose(self):
        class MockSocket:
            closed = False
            def flush(self): pass
            def close(self): self.closed = True

        # must not close unless we request it: the original use of _fileobject
        # by module socket requires that the underlying socket not be closed until
        # the _socketobject that created the _fileobject is closed
        s = MockSocket()
        f = socket._fileobject(s)
        f.close()
        self.assertTrue(not s.closed)

        s = MockSocket()
        f = socket._fileobject(s, close=True)
        f.close()
        self.assertTrue(s.closed)

class TCPTimeoutTest(SocketTCPTest):

    def testTCPTimeout(self):
        def raise_timeout(*args, **kwargs):
            self.serv.settimeout(1.0)
            self.serv.accept()
        self.assertRaises(socket.timeout, raise_timeout,
                              "Error generating a timeout exception (TCP)")

    def testTimeoutZero(self):
        ok = False
        try:
            self.serv.settimeout(0.0)
            foo = self.serv.accept()
        except socket.timeout:
            self.fail("caught timeout instead of error (TCP)")
        except socket.error:
            ok = True
        except:
            self.fail("caught unexpected exception (TCP)")
        if not ok:
            self.fail("accept() returned success when we did not expect it")

    @unittest.skipUnless(hasattr(signal, 'alarm'),
                         'test needs signal.alarm()')
    def testInterruptedTimeout(self):
        # XXX I don't know how to do this test on MSWindows or any other
        # plaform that doesn't support signal.alarm() or os.kill(), though
        # the bug should have existed on all platforms.
        self.serv.settimeout(5.0)   # must be longer than alarm
        class Alarm(Exception):
            pass
        def alarm_handler(signal, frame):
            raise Alarm
        old_alarm = signal.signal(signal.SIGALRM, alarm_handler)
        try:
            try:
                signal.alarm(2)    # POSIX allows alarm to be up to 1 second early
                foo = self.serv.accept()
            except socket.timeout:
                self.fail("caught timeout instead of Alarm")
            except Alarm:
                pass
            except:
                self.fail("caught other exception instead of Alarm:"
                          " %s(%s):\n%s" %
                          (sys.exc_info()[:2] + (traceback.format_exc(),)))
            else:
                self.fail("nothing caught")
            finally:
                signal.alarm(0)         # shut off alarm
        except Alarm:
            self.fail("got Alarm in wrong place")
        finally:
            # no alarm can be pending.  Safe to restore old handler.
            signal.signal(signal.SIGALRM, old_alarm)

class UDPTimeoutTest(SocketUDPTest):

    def testUDPTimeout(self):
        def raise_timeout(*args, **kwargs):
            self.serv.settimeout(1.0)
            self.serv.recv(1024)
        self.assertRaises(socket.timeout, raise_timeout,
                              "Error generating a timeout exception (UDP)")

    def testTimeoutZero(self):
        ok = False
        try:
            self.serv.settimeout(0.0)
            foo = self.serv.recv(1024)
        except socket.timeout:
            self.fail("caught timeout instead of error (UDP)")
        except socket.error:
            ok = True
        except:
            self.fail("caught unexpected exception (UDP)")
        if not ok:
            self.fail("recv() returned success when we did not expect it")

class TestExceptions(unittest.TestCase):

    def testExceptionTree(self):
        self.assertTrue(issubclass(socket.error, Exception))
        self.assertTrue(issubclass(socket.herror, socket.error))
        self.assertTrue(issubclass(socket.gaierror, socket.error))
        self.assertTrue(issubclass(socket.timeout, socket.error))

@unittest.skipUnless(sys.platform == 'linux', 'Linux specific test')
class TestLinuxAbstractNamespace(unittest.TestCase):

    UNIX_PATH_MAX = 108

    def testLinuxAbstractNamespace(self):
        address = "\x00python-test-hello\x00\xff"
        s1 = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s1.bind(address)
        s1.listen(1)
        s2 = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s2.connect(s1.getsockname())
        s1.accept()
        self.assertEqual(s1.getsockname(), address)
        self.assertEqual(s2.getpeername(), address)

    def testMaxName(self):
        address = "\x00" + "h" * (self.UNIX_PATH_MAX - 1)
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.bind(address)
        self.assertEqual(s.getsockname(), address)

    def testNameOverflow(self):
        address = "\x00" + "h" * self.UNIX_PATH_MAX
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.assertRaises(socket.error, s.bind, address)


@unittest.skipUnless(thread, 'Threading required for this test.')
class BufferIOTest(SocketConnectedTest):
    """
    Test the buffer versions of socket.recv() and socket.send().
    """
    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def testRecvIntoArray(self):
        buf = array.array('c', ' '*1024)
        nbytes = self.cli_conn.recv_into(buf)
        self.assertEqual(nbytes, len(MSG))
        msg = buf.tostring()[:len(MSG)]
        self.assertEqual(msg, MSG)

    def _testRecvIntoArray(self):
        with test_support.check_py3k_warnings():
            buf = buffer(MSG)
        self.serv_conn.send(buf)

    def testRecvIntoBytearray(self):
        buf = bytearray(1024)
        nbytes = self.cli_conn.recv_into(buf)
        self.assertEqual(nbytes, len(MSG))
        msg = buf[:len(MSG)]
        self.assertEqual(msg, MSG)

    _testRecvIntoBytearray = _testRecvIntoArray

    def testRecvIntoMemoryview(self):
        buf = bytearray(1024)
        nbytes = self.cli_conn.recv_into(memoryview(buf))
        self.assertEqual(nbytes, len(MSG))
        msg = buf[:len(MSG)]
        self.assertEqual(msg, MSG)

    _testRecvIntoMemoryview = _testRecvIntoArray

    def testRecvFromIntoArray(self):
        buf = array.array('c', ' '*1024)
        nbytes, addr = self.cli_conn.recvfrom_into(buf)
        self.assertEqual(nbytes, len(MSG))
        msg = buf.tostring()[:len(MSG)]
        self.assertEqual(msg, MSG)

    def _testRecvFromIntoArray(self):
        with test_support.check_py3k_warnings():
            buf = buffer(MSG)
        self.serv_conn.send(buf)

    def testRecvFromIntoBytearray(self):
        buf = bytearray(1024)
        nbytes, addr = self.cli_conn.recvfrom_into(buf)
        self.assertEqual(nbytes, len(MSG))
        msg = buf[:len(MSG)]
        self.assertEqual(msg, MSG)

    _testRecvFromIntoBytearray = _testRecvFromIntoArray

    def testRecvFromIntoMemoryview(self):
        buf = bytearray(1024)
        nbytes, addr = self.cli_conn.recvfrom_into(memoryview(buf))
        self.assertEqual(nbytes, len(MSG))
        msg = buf[:len(MSG)]
        self.assertEqual(msg, MSG)

    _testRecvFromIntoMemoryview = _testRecvFromIntoArray

    def testRecvFromIntoSmallBuffer(self):
        # See issue #20246.
        buf = bytearray(8)
        self.assertRaises(ValueError, self.cli_conn.recvfrom_into, buf, 1024)

    def _testRecvFromIntoSmallBuffer(self):
        with test_support.check_py3k_warnings():
            buf = buffer(MSG)
        self.serv_conn.send(buf)

    def testRecvFromIntoEmptyBuffer(self):
        buf = bytearray()
        self.cli_conn.recvfrom_into(buf)
        self.cli_conn.recvfrom_into(buf, 0)

    _testRecvFromIntoEmptyBuffer = _testRecvFromIntoArray


TIPC_STYPE = 2000
TIPC_LOWER = 200
TIPC_UPPER = 210

def isTipcAvailable():
    """Check if the TIPC module is loaded

    The TIPC module is not loaded automatically on Ubuntu and probably
    other Linux distros.
    """
    if not hasattr(socket, "AF_TIPC"):
        return False
    try:
        f = open("/proc/modules")
    except IOError as e:
        # It's ok if the file does not exist, is a directory or if we
        # have not the permission to read it. In any other case it's a
        # real error, so raise it again.
        if e.errno in (errno.ENOENT, errno.EISDIR, errno.EACCES):
            return False
        else:
            raise
    with f:
        for line in f:
            if line.startswith("tipc "):
                return True
    return False

@unittest.skipUnless(isTipcAvailable(),
                     "TIPC module is not loaded, please 'sudo modprobe tipc'")
class TIPCTest(unittest.TestCase):
    def testRDM(self):
        srv = socket.socket(socket.AF_TIPC, socket.SOCK_RDM)
        cli = socket.socket(socket.AF_TIPC, socket.SOCK_RDM)

        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srvaddr = (socket.TIPC_ADDR_NAMESEQ, TIPC_STYPE,
                TIPC_LOWER, TIPC_UPPER)
        srv.bind(srvaddr)

        sendaddr = (socket.TIPC_ADDR_NAME, TIPC_STYPE,
                TIPC_LOWER + (TIPC_UPPER - TIPC_LOWER) / 2, 0)
        cli.sendto(MSG, sendaddr)

        msg, recvaddr = srv.recvfrom(1024)

        self.assertEqual(cli.getsockname(), recvaddr)
        self.assertEqual(msg, MSG)


@unittest.skipUnless(isTipcAvailable(),
                     "TIPC module is not loaded, please 'sudo modprobe tipc'")
class TIPCThreadableTest(unittest.TestCase, ThreadableTest):
    def __init__(self, methodName = 'runTest'):
        unittest.TestCase.__init__(self, methodName = methodName)
        ThreadableTest.__init__(self)

    def setUp(self):
        self.srv = socket.socket(socket.AF_TIPC, socket.SOCK_STREAM)
        self.srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srvaddr = (socket.TIPC_ADDR_NAMESEQ, TIPC_STYPE,
                TIPC_LOWER, TIPC_UPPER)
        self.srv.bind(srvaddr)
        self.srv.listen(5)
        self.serverExplicitReady()
        self.conn, self.connaddr = self.srv.accept()

    def clientSetUp(self):
        # There is a hittable race between serverExplicitReady() and the
        # accept() call; sleep a little while to avoid it, otherwise
        # we could get an exception
        time.sleep(0.1)
        self.cli = socket.socket(socket.AF_TIPC, socket.SOCK_STREAM)
        addr = (socket.TIPC_ADDR_NAME, TIPC_STYPE,
                TIPC_LOWER + (TIPC_UPPER - TIPC_LOWER) / 2, 0)
        self.cli.connect(addr)
        self.cliaddr = self.cli.getsockname()

    def testStream(self):
        msg = self.conn.recv(1024)
        self.assertEqual(msg, MSG)
        self.assertEqual(self.cliaddr, self.connaddr)

    def _testStream(self):
        self.cli.send(MSG)
        self.cli.close()


class ContextManagersTest(ThreadedTCPSocketTest):

    def _testSocketClass(self):
        # base test
        with socket.socket() as sock:
            self.assertFalse(sock._closed)
        self.assertTrue(sock._closed)
        # close inside with block
        with socket.socket() as sock:
            sock.close()
        self.assertTrue(sock._closed)
        # exception inside with block
        with socket.socket() as sock:
            self.assertRaises(OSError, sock.sendall, b'foo')
        self.assertTrue(sock._closed)

    def testCreateConnectionBase(self):
        conn, addr = self.serv.accept()
        self.addCleanup(conn.close)
        data = conn.recv(1024)
        conn.sendall(data)

    def _testCreateConnectionBase(self):
        address = self.serv.getsockname()
        with socket.create_connection(address) as sock:
            self.assertFalse(sock._closed)
            sock.sendall(b'foo')
            self.assertEqual(sock.recv(1024), b'foo')
        self.assertTrue(sock._closed)

    def testCreateConnectionClose(self):
        conn, addr = self.serv.accept()
        self.addCleanup(conn.close)
        data = conn.recv(1024)
        conn.sendall(data)

    def _testCreateConnectionClose(self):
        address = self.serv.getsockname()
        with socket.create_connection(address) as sock:
            sock.close()
        self.assertTrue(sock._closed)
        self.assertRaises(OSError, sock.sendall, b'foo')


class InheritanceTest(unittest.TestCase):
    @unittest.skipUnless(hasattr(socket, "SOCK_CLOEXEC"),
                         "SOCK_CLOEXEC not defined")
    @support.requires_linux_version(2, 6, 28)
    def test_SOCK_CLOEXEC(self):
        with socket.socket(socket.AF_INET,
                           socket.SOCK_STREAM | socket.SOCK_CLOEXEC) as s:
            self.assertEqual(s.type, socket.SOCK_STREAM)
            self.assertFalse(s.get_inheritable())

    def test_default_inheritable(self):
        sock = socket.socket()
        with sock:
            self.assertEqual(sock.get_inheritable(), False)

    def test_dup(self):
        sock = socket.socket()
        with sock:
            newsock = sock.dup()
            sock.close()
            with newsock:
                self.assertEqual(newsock.get_inheritable(), False)

    def test_set_inheritable(self):
        sock = socket.socket()
        with sock:
            sock.set_inheritable(True)
            self.assertEqual(sock.get_inheritable(), True)

            sock.set_inheritable(False)
            self.assertEqual(sock.get_inheritable(), False)

    @unittest.skipIf(fcntl is None, "need fcntl")
    def test_get_inheritable_cloexec(self):
        sock = socket.socket()
        with sock:
            fd = sock.fileno()
            self.assertEqual(sock.get_inheritable(), False)

            # clear FD_CLOEXEC flag
            flags = fcntl.fcntl(fd, fcntl.F_GETFD)
            flags &= ~fcntl.FD_CLOEXEC
            fcntl.fcntl(fd, fcntl.F_SETFD, flags)

            self.assertEqual(sock.get_inheritable(), True)

    @unittest.skipIf(fcntl is None, "need fcntl")
    def test_set_inheritable_cloexec(self):
        sock = socket.socket()
        with sock:
            fd = sock.fileno()
            self.assertEqual(fcntl.fcntl(fd, fcntl.F_GETFD) & fcntl.FD_CLOEXEC,
                             fcntl.FD_CLOEXEC)

            sock.set_inheritable(True)
            self.assertEqual(fcntl.fcntl(fd, fcntl.F_GETFD) & fcntl.FD_CLOEXEC,
                             0)


    def test_socketpair(self):
        s1, s2 = socket.socketpair()
        self.addCleanup(s1.close)
        self.addCleanup(s2.close)
        self.assertEqual(s1.get_inheritable(), False)
        self.assertEqual(s2.get_inheritable(), False)


@unittest.skipUnless(hasattr(socket, "SOCK_NONBLOCK"),
                     "SOCK_NONBLOCK not defined")
class NonblockConstantTest(unittest.TestCase):
    def checkNonblock(self, s, nonblock=True, timeout=0.0):
        if nonblock:
            self.assertEqual(s.type, socket.SOCK_STREAM)
            self.assertEqual(s.gettimeout(), timeout)
            self.assertTrue(
                fcntl.fcntl(s, fcntl.F_GETFL, os.O_NONBLOCK) & os.O_NONBLOCK)
            if timeout == 0:
                # timeout == 0: means that getblocking() must be False.
                self.assertFalse(s.getblocking())
            else:
                # If timeout > 0, the socket will be in a "blocking" mode
                # from the standpoint of the Python API.  For Python socket
                # object, "blocking" means that operations like 'sock.recv()'
                # will block.  Internally, file descriptors for
                # "blocking" Python sockets *with timeouts* are in a
                # *non-blocking* mode, and 'sock.recv()' uses 'select()'
                # and handles EWOULDBLOCK/EAGAIN to enforce the timeout.
                self.assertTrue(s.getblocking())
        else:
            self.assertEqual(s.type, socket.SOCK_STREAM)
            self.assertEqual(s.gettimeout(), None)
            self.assertFalse(
                fcntl.fcntl(s, fcntl.F_GETFL, os.O_NONBLOCK) & os.O_NONBLOCK)
            self.assertTrue(s.getblocking())

    @support.requires_linux_version(2, 6, 28)
    def test_SOCK_NONBLOCK(self):
        # a lot of it seems silly and redundant, but I wanted to test that
        # changing back and forth worked ok
        with socket.socket(socket.AF_INET,
                           socket.SOCK_STREAM | socket.SOCK_NONBLOCK) as s:
            self.checkNonblock(s)
            s.setblocking(1)
            self.checkNonblock(s, nonblock=False)
            s.setblocking(0)
            self.checkNonblock(s)
            s.settimeout(None)
            self.checkNonblock(s, nonblock=False)
            s.settimeout(2.0)
            self.checkNonblock(s, timeout=2.0)
            s.setblocking(1)
            self.checkNonblock(s, nonblock=False)
        # defaulttimeout
        t = socket.getdefaulttimeout()
        socket.setdefaulttimeout(0.0)
        with socket.socket() as s:
            self.checkNonblock(s)
        socket.setdefaulttimeout(None)
        with socket.socket() as s:
            self.checkNonblock(s, False)
        socket.setdefaulttimeout(2.0)
        with socket.socket() as s:
            self.checkNonblock(s, timeout=2.0)
        socket.setdefaulttimeout(None)
        with socket.socket() as s:
            self.checkNonblock(s, False)
        socket.setdefaulttimeout(t)


@unittest.skipUnless(os.name == "nt", "Windows specific")
@unittest.skipUnless(multiprocessing, "need multiprocessing")
class TestSocketSharing(SocketTCPTest):
    # This must be classmethod and not staticmethod or multiprocessing
    # won't be able to bootstrap it.
    @classmethod
    def remoteProcessServer(cls, q):
        # Recreate socket from shared data
        sdata = q.get()
        message = q.get()

        s = socket.fromshare(sdata)
        s2, c = s.accept()

        # Send the message
        s2.sendall(message)
        s2.close()
        s.close()

    def testShare(self):
        # Transfer the listening server socket to another process
        # and service it from there.

        # Create process:
        q = multiprocessing.Queue()
        p = multiprocessing.Process(target=self.remoteProcessServer, args=(q,))
        p.start()

        # Get the shared socket data
        data = self.serv.share(p.pid)

        # Pass the shared socket to the other process
        addr = self.serv.getsockname()
        self.serv.close()
        q.put(data)

        # The data that the server will send us
        message = b"slapmahfro"
        q.put(message)

        # Connect
        s = socket.create_connection(addr)
        #  listen for the data
        m = []
        while True:
            data = s.recv(100)
            if not data:
                break
            m.append(data)
        s.close()
        received = b"".join(m)
        self.assertEqual(received, message)
        p.join()

    def testShareLength(self):
        data = self.serv.share(os.getpid())
        self.assertRaises(ValueError, socket.fromshare, data[:-1])
        self.assertRaises(ValueError, socket.fromshare, data+b"foo")

    def compareSockets(self, org, other):
        # socket sharing is expected to work only for blocking socket
        # since the internal python timeout value isn't transferred.
        self.assertEqual(org.gettimeout(), None)
        self.assertEqual(org.gettimeout(), other.gettimeout())

        self.assertEqual(org.family, other.family)
        self.assertEqual(org.type, other.type)
        # If the user specified "0" for proto, then
        # internally windows will have picked the correct value.
        # Python introspection on the socket however will still return
        # 0.  For the shared socket, the python value is recreated
        # from the actual value, so it may not compare correctly.
        if org.proto != 0:
            self.assertEqual(org.proto, other.proto)

    def testShareLocal(self):
        data = self.serv.share(os.getpid())
        s = socket.fromshare(data)
        try:
            self.compareSockets(self.serv, s)
        finally:
            s.close()

    def testTypes(self):
        families = [socket.AF_INET, socket.AF_INET6]
        types = [socket.SOCK_STREAM, socket.SOCK_DGRAM]
        for f in families:
            for t in types:
                try:
                    source = socket.socket(f, t)
                except OSError:
                    continue # This combination is not supported
                try:
                    data = source.share(os.getpid())
                    shared = socket.fromshare(data)
                    try:
                        self.compareSockets(source, shared)
                    finally:
                        shared.close()
                finally:
                    source.close()


class SendfileUsingSendTest(ThreadedTCPSocketTest):
    """
    Test the send() implementation of socket.sendfile().
    """

    FILESIZE = (10 * 1024 * 1024)  # 10 MiB
    BUFSIZE = 8192
    FILEDATA = b""
    TIMEOUT = 2

    @classmethod
    def setUpClass(cls):
        def chunks(total, step):
            assert total >= step
            while total > step:
                yield step
                total -= step
            if total:
                yield total

        chunk = b"".join([random.choice(string.ascii_letters).encode()
                          for i in range(cls.BUFSIZE)])
        with open(support.TESTFN, 'wb') as f:
            for csize in chunks(cls.FILESIZE, cls.BUFSIZE):
                f.write(chunk)
        with open(support.TESTFN, 'rb') as f:
            cls.FILEDATA = f.read()
            assert len(cls.FILEDATA) == cls.FILESIZE

    @classmethod
    def tearDownClass(cls):
        support.unlink(support.TESTFN)

    def accept_conn(self):
        self.serv.settimeout(self.TIMEOUT)
        conn, addr = self.serv.accept()
        conn.settimeout(self.TIMEOUT)
        self.addCleanup(conn.close)
        return conn

    def recv_data(self, conn):
        received = []
        while True:
            chunk = conn.recv(self.BUFSIZE)
            if not chunk:
                break
            received.append(chunk)
        return b''.join(received)

    def meth_from_sock(self, sock):
        # Depending on the mixin class being run return either send()
        # or sendfile() method implementation.
        return getattr(sock, "_sendfile_use_send")

    # regular file

    def _testRegularFile(self):
        address = self.serv.getsockname()
        file = open(support.TESTFN, 'rb')
        with socket.create_connection(address) as sock, file as file:
            meth = self.meth_from_sock(sock)
            sent = meth(file)
            self.assertEqual(sent, self.FILESIZE)
            self.assertEqual(file.tell(), self.FILESIZE)

    def testRegularFile(self):
        conn = self.accept_conn()
        data = self.recv_data(conn)
        self.assertEqual(len(data), self.FILESIZE)
        self.assertEqual(data, self.FILEDATA)

    # non regular file

    def _testNonRegularFile(self):
        address = self.serv.getsockname()
        file = io.BytesIO(self.FILEDATA)
        with socket.create_connection(address) as sock, file as file:
            sent = sock.sendfile(file)
            self.assertEqual(sent, self.FILESIZE)
            self.assertEqual(file.tell(), self.FILESIZE)
            self.assertRaises(socket._GiveupOnSendfile,
                              sock._sendfile_use_sendfile, file)

    def testNonRegularFile(self):
        conn = self.accept_conn()
        data = self.recv_data(conn)
        self.assertEqual(len(data), self.FILESIZE)
        self.assertEqual(data, self.FILEDATA)

    # empty file

    def _testEmptyFileSend(self):
        address = self.serv.getsockname()
        filename = support.TESTFN + "2"
        with open(filename, 'wb'):
            self.addCleanup(support.unlink, filename)
        file = open(filename, 'rb')
        with socket.create_connection(address) as sock, file as file:
            meth = self.meth_from_sock(sock)
            sent = meth(file)
            self.assertEqual(sent, 0)
            self.assertEqual(file.tell(), 0)

    def testEmptyFileSend(self):
        conn = self.accept_conn()
        data = self.recv_data(conn)
        self.assertEqual(data, b"")

    # offset

    def _testOffset(self):
        address = self.serv.getsockname()
        file = open(support.TESTFN, 'rb')
        with socket.create_connection(address) as sock, file as file:
            meth = self.meth_from_sock(sock)
            sent = meth(file, offset=5000)
            self.assertEqual(sent, self.FILESIZE - 5000)
            self.assertEqual(file.tell(), self.FILESIZE)

    def testOffset(self):
        conn = self.accept_conn()
        data = self.recv_data(conn)
        self.assertEqual(len(data), self.FILESIZE - 5000)
        self.assertEqual(data, self.FILEDATA[5000:])

    # count

    def _testCount(self):
        address = self.serv.getsockname()
        file = open(support.TESTFN, 'rb')
        with socket.create_connection(address, timeout=2) as sock, file as file:
            count = 5000007
            meth = self.meth_from_sock(sock)
            sent = meth(file, count=count)
            self.assertEqual(sent, count)
            self.assertEqual(file.tell(), count)

    def testCount(self):
        count = 5000007
        conn = self.accept_conn()
        data = self.recv_data(conn)
        self.assertEqual(len(data), count)
        self.assertEqual(data, self.FILEDATA[:count])

    # count small

    def _testCountSmall(self):
        address = self.serv.getsockname()
        file = open(support.TESTFN, 'rb')
        with socket.create_connection(address, timeout=2) as sock, file as file:
            count = 1
            meth = self.meth_from_sock(sock)
            sent = meth(file, count=count)
            self.assertEqual(sent, count)
            self.assertEqual(file.tell(), count)

    def testCountSmall(self):
        count = 1
        conn = self.accept_conn()
        data = self.recv_data(conn)
        self.assertEqual(len(data), count)
        self.assertEqual(data, self.FILEDATA[:count])

    # count + offset

    def _testCountWithOffset(self):
        address = self.serv.getsockname()
        file = open(support.TESTFN, 'rb')
        with socket.create_connection(address, timeout=2) as sock, file as file:
            count = 100007
            meth = self.meth_from_sock(sock)
            sent = meth(file, offset=2007, count=count)
            self.assertEqual(sent, count)
            self.assertEqual(file.tell(), count + 2007)

    def testCountWithOffset(self):
        count = 100007
        conn = self.accept_conn()
        data = self.recv_data(conn)
        self.assertEqual(len(data), count)
        self.assertEqual(data, self.FILEDATA[2007:count+2007])

    # non blocking sockets are not supposed to work

    def _testNonBlocking(self):
        address = self.serv.getsockname()
        file = open(support.TESTFN, 'rb')
        with socket.create_connection(address) as sock, file as file:
            sock.setblocking(False)
            meth = self.meth_from_sock(sock)
            self.assertRaises(ValueError, meth, file)
            self.assertRaises(ValueError, sock.sendfile, file)

    def testNonBlocking(self):
        conn = self.accept_conn()
        if conn.recv(8192):
            self.fail('was not supposed to receive any data')

    # timeout (non-triggered)

    def _testWithTimeout(self):
        address = self.serv.getsockname()
        file = open(support.TESTFN, 'rb')
        with socket.create_connection(address, timeout=2) as sock, file as file:
            meth = self.meth_from_sock(sock)
            sent = meth(file)
            self.assertEqual(sent, self.FILESIZE)

    def testWithTimeout(self):
        conn = self.accept_conn()
        data = self.recv_data(conn)
        self.assertEqual(len(data), self.FILESIZE)
        self.assertEqual(data, self.FILEDATA)

    # timeout (triggered)

    def _testWithTimeoutTriggeredSend(self):
        address = self.serv.getsockname()
        file = open(support.TESTFN, 'rb')
        with socket.create_connection(address, timeout=0.01) as sock, \
                file as file:
            meth = self.meth_from_sock(sock)
            self.assertRaises(socket.timeout, meth, file)

    def testWithTimeoutTriggeredSend(self):
        conn = self.accept_conn()
        conn.recv(88192)

    # errors

    def _test_errors(self):
        pass

    def test_errors(self):
        with open(support.TESTFN, 'rb') as file:
            with socket.socket(type=socket.SOCK_DGRAM) as s:
                meth = self.meth_from_sock(s)
                self.assertRaisesRegex(
                    ValueError, "SOCK_STREAM", meth, file)
        with open(support.TESTFN, 'rt') as file:
            with socket.socket() as s:
                meth = self.meth_from_sock(s)
                self.assertRaisesRegex(
                    ValueError, "binary mode", meth, file)
        with open(support.TESTFN, 'rb') as file:
            with socket.socket() as s:
                meth = self.meth_from_sock(s)
                self.assertRaisesRegex(TypeError, "positive integer",
                                       meth, file, count='2')
                self.assertRaisesRegex(TypeError, "positive integer",
                                       meth, file, count=0.1)
                self.assertRaisesRegex(ValueError, "positive integer",
                                       meth, file, count=0)
                self.assertRaisesRegex(ValueError, "positive integer",
                                       meth, file, count=-1)


@unittest.skipUnless(hasattr(os, "sendfile"),
                     'os.sendfile() required for this test.')
class SendfileUsingSendfileTest(SendfileUsingSendTest):
    """
    Test the sendfile() implementation of socket.sendfile().
    """
    def meth_from_sock(self, sock):
        return getattr(sock, "_sendfile_use_sendfile")


@unittest.skipUnless(HAVE_SOCKET_ALG, 'AF_ALG required')
class LinuxKernelCryptoAPI(unittest.TestCase):
    # tests for AF_ALG
    def create_alg(self, typ, name):
        sock = socket.socket(socket.AF_ALG, socket.SOCK_SEQPACKET, 0)
        try:
            sock.bind((typ, name))
        except FileNotFoundError as e:
            # type / algorithm is not available
            sock.close()
            raise unittest.SkipTest(str(e), typ, name)
        else:
            return sock

    # bpo-31705: On kernel older than 4.5, sendto() failed with ENOKEY,
    # at least on ppc64le architecture
    @support.requires_linux_version(4, 5)
    def test_sha256(self):
        expected = bytes.fromhex("ba7816bf8f01cfea414140de5dae2223b00361a396"
                                 "177a9cb410ff61f20015ad")
        with self.create_alg('hash', 'sha256') as algo:
            op, _ = algo.accept()
            with op:
                op.sendall(b"abc")
                self.assertEqual(op.recv(512), expected)

            op, _ = algo.accept()
            with op:
                op.send(b'a', socket.MSG_MORE)
                op.send(b'b', socket.MSG_MORE)
                op.send(b'c', socket.MSG_MORE)
                op.send(b'')
                self.assertEqual(op.recv(512), expected)

    def test_hmac_sha1(self):
        expected = bytes.fromhex("effcdf6ae5eb2fa2d27416d5f184df9c259a7c79")
        with self.create_alg('hash', 'hmac(sha1)') as algo:
            algo.setsockopt(socket.SOL_ALG, socket.ALG_SET_KEY, b"Jefe")
            op, _ = algo.accept()
            with op:
                op.sendall(b"what do ya want for nothing?")
                self.assertEqual(op.recv(512), expected)

    # Although it should work with 3.19 and newer the test blocks on
    # Ubuntu 15.10 with Kernel 4.2.0-19.
    @support.requires_linux_version(4, 3)
    def test_aes_cbc(self):
        key = bytes.fromhex('06a9214036b8a15b512e03d534120006')
        iv = bytes.fromhex('3dafba429d9eb430b422da802c9fac41')
        msg = b"Single block msg"
        ciphertext = bytes.fromhex('e353779c1079aeb82708942dbe77181a')
        msglen = len(msg)
        with self.create_alg('skcipher', 'cbc(aes)') as algo:
            algo.setsockopt(socket.SOL_ALG, socket.ALG_SET_KEY, key)
            op, _ = algo.accept()
            with op:
                op.sendmsg_afalg(op=socket.ALG_OP_ENCRYPT, iv=iv,
                                 flags=socket.MSG_MORE)
                op.sendall(msg)
                self.assertEqual(op.recv(msglen), ciphertext)

            op, _ = algo.accept()
            with op:
                op.sendmsg_afalg([ciphertext],
                                 op=socket.ALG_OP_DECRYPT, iv=iv)
                self.assertEqual(op.recv(msglen), msg)

            # long message
            multiplier = 1024
            longmsg = [msg] * multiplier
            op, _ = algo.accept()
            with op:
                op.sendmsg_afalg(longmsg,
                                 op=socket.ALG_OP_ENCRYPT, iv=iv)
                enc = op.recv(msglen * multiplier)
            self.assertEqual(len(enc), msglen * multiplier)
            self.assertTrue(enc[:msglen], ciphertext)

            op, _ = algo.accept()
            with op:
                op.sendmsg_afalg([enc],
                                 op=socket.ALG_OP_DECRYPT, iv=iv)
                dec = op.recv(msglen * multiplier)
            self.assertEqual(len(dec), msglen * multiplier)
            self.assertEqual(dec, msg * multiplier)

    @support.requires_linux_version(4, 9)  # see issue29324
    def test_aead_aes_gcm(self):
        key = bytes.fromhex('c939cc13397c1d37de6ae0e1cb7c423c')
        iv = bytes.fromhex('b3d8cc017cbb89b39e0f67e2')
        plain = bytes.fromhex('c3b3c41f113a31b73d9a5cd432103069')
        assoc = bytes.fromhex('24825602bd12a984e0092d3e448eda5f')
        expected_ct = bytes.fromhex('93fe7d9e9bfd10348a5606e5cafa7354')
        expected_tag = bytes.fromhex('0032a1dc85f1c9786925a2e71d8272dd')

        taglen = len(expected_tag)
        assoclen = len(assoc)

        with self.create_alg('aead', 'gcm(aes)') as algo:
            algo.setsockopt(socket.SOL_ALG, socket.ALG_SET_KEY, key)
            algo.setsockopt(socket.SOL_ALG, socket.ALG_SET_AEAD_AUTHSIZE,
                            None, taglen)

            # send assoc, plain and tag buffer in separate steps
            op, _ = algo.accept()
            with op:
                op.sendmsg_afalg(op=socket.ALG_OP_ENCRYPT, iv=iv,
                                 assoclen=assoclen, flags=socket.MSG_MORE)
                op.sendall(assoc, socket.MSG_MORE)
                op.sendall(plain)
                res = op.recv(assoclen + len(plain) + taglen)
                self.assertEqual(expected_ct, res[assoclen:-taglen])
                self.assertEqual(expected_tag, res[-taglen:])

            # now with msg
            op, _ = algo.accept()
            with op:
                msg = assoc + plain
                op.sendmsg_afalg([msg], op=socket.ALG_OP_ENCRYPT, iv=iv,
                                 assoclen=assoclen)
                res = op.recv(assoclen + len(plain) + taglen)
                self.assertEqual(expected_ct, res[assoclen:-taglen])
                self.assertEqual(expected_tag, res[-taglen:])

            # create anc data manually
            pack_uint32 = struct.Struct('I').pack
            op, _ = algo.accept()
            with op:
                msg = assoc + plain
                op.sendmsg(
                    [msg],
                    ([socket.SOL_ALG, socket.ALG_SET_OP, pack_uint32(socket.ALG_OP_ENCRYPT)],
                     [socket.SOL_ALG, socket.ALG_SET_IV, pack_uint32(len(iv)) + iv],
                     [socket.SOL_ALG, socket.ALG_SET_AEAD_ASSOCLEN, pack_uint32(assoclen)],
                    )
                )
                res = op.recv(len(msg) + taglen)
                self.assertEqual(expected_ct, res[assoclen:-taglen])
                self.assertEqual(expected_tag, res[-taglen:])

            # decrypt and verify
            op, _ = algo.accept()
            with op:
                msg = assoc + expected_ct + expected_tag
                op.sendmsg_afalg([msg], op=socket.ALG_OP_DECRYPT, iv=iv,
                                 assoclen=assoclen)
                res = op.recv(len(msg) - taglen)
                self.assertEqual(plain, res[assoclen:])

    @support.requires_linux_version(4, 3)  # see test_aes_cbc
    def test_drbg_pr_sha256(self):
        # deterministic random bit generator, prediction resistance, sha256
        with self.create_alg('rng', 'drbg_pr_sha256') as algo:
            extra_seed = os.urandom(32)
            algo.setsockopt(socket.SOL_ALG, socket.ALG_SET_KEY, extra_seed)
            op, _ = algo.accept()
            with op:
                rn = op.recv(32)
                self.assertEqual(len(rn), 32)

    def test_sendmsg_afalg_args(self):
        sock = socket.socket(socket.AF_ALG, socket.SOCK_SEQPACKET, 0)
        with sock:
            with self.assertRaises(TypeError):
                sock.sendmsg_afalg()

            with self.assertRaises(TypeError):
                sock.sendmsg_afalg(op=None)

            with self.assertRaises(TypeError):
                sock.sendmsg_afalg(1)

            with self.assertRaises(TypeError):
                sock.sendmsg_afalg(op=socket.ALG_OP_ENCRYPT, assoclen=None)

            with self.assertRaises(TypeError):
                sock.sendmsg_afalg(op=socket.ALG_OP_ENCRYPT, assoclen=-1)


def test_main():
    tests = [GeneralModuleTests, BasicTCPTest, TCPCloserTest, TCPTimeoutTest,
             TestExceptions, BufferIOTest, BasicTCPTest2, BasicUDPTest,
             UDPTimeoutTest ]

    tests.extend([
        NonBlockingTCPTests,
        FileObjectClassTestCase,
        FileObjectInterruptedTestCase,
        UnbufferedFileObjectClassTestCase,
        LineBufferedFileObjectClassTestCase,
        SmallBufferedFileObjectClassTestCase,
        Urllib2FileobjectTest,
        NetworkConnectionNoServer,
        NetworkConnectionAttributesTest,
        NetworkConnectionBehaviourTest,
    ])
    tests.append(BasicSocketPairTest)
    tests.append(TestLinuxAbstractNamespace)
    tests.extend([TIPCTest, TIPCThreadableTest])

    thread_info = test_support.threading_setup()
    test_support.run_unittest(*tests)
    test_support.threading_cleanup(*thread_info)

if __name__ == "__main__":
    test_main()
