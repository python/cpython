#!/usr/bin/env python3

import unittest
from test import support

import errno
import io
import socket
import select
import time
import traceback
import queue
import sys
import os
import array
import platform
import contextlib
from weakref import proxy
import signal
import math
import pickle
try:
    import fcntl
except ImportError:
    fcntl = False

def linux_version():
    try:
        # platform.release() is something like '2.6.33.7-desktop-2mnb'
        version_string = platform.release().split('-')[0]
        return tuple(map(int, version_string.split('.')))
    except ValueError:
        return 0, 0, 0

HOST = support.HOST
MSG = 'Michael Gilfix was here\u1234\r\n'.encode('utf-8') ## test unicode string and carriage return

try:
    import _thread as thread
    import threading
except ImportError:
    thread = None
    threading = None

class SocketTCPTest(unittest.TestCase):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.port = support.bind_port(self.serv)
        self.serv.listen(1)

    def tearDown(self):
        self.serv.close()
        self.serv = None

class SocketUDPTest(unittest.TestCase):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.port = support.bind_port(self.serv)

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
    tests in pairs, where the test name is preceeded with a
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
        self.queue = queue.Queue(1)

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

        if self.queue.qsize():
            exc = self.queue.get()
            raise exc

    def clientRun(self, test_func):
        self.server_ready.wait()
        self.client_ready.set()
        self.clientSetUp()
        if not hasattr(test_func, '__call__'):
            raise TypeError("test_func must be a callable function")
        try:
            test_func()
        except BaseException as e:
            self.queue.put(e)
        finally:
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
    """Socket tests for client-server connection.

    self.cli_conn is a client socket connected to the server.  The
    setUp() method guarantees that it is connected to the server.
    """

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

    def test_repr(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(s.close)
        self.assertTrue(repr(s).startswith("<socket.socket object"))

    def test_weakref(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        p = proxy(s)
        self.assertEqual(p.fileno(), s.fileno())
        s.close()
        s = None
        try:
            p.fileno()
        except ReferenceError:
            pass
        else:
            self.fail('Socket proxy still exists')

    def testSocketError(self):
        # Testing socket module exceptions
        msg = "Error raising socket exception (%s)."
        with self.assertRaises(socket.error, msg=msg % 'socket.error'):
            raise socket.error
        with self.assertRaises(socket.error, msg=msg % 'socket.herror'):
            raise socket.herror
        with self.assertRaises(socket.error, msg=msg % 'socket.gaierror'):
            raise socket.gaierror

    def testSendtoErrors(self):
        # Testing that sendto doens't masks failures. See #10169.
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.addCleanup(s.close)
        s.bind(('', 0))
        sockname = s.getsockname()
        # 2 args
        with self.assertRaises(TypeError) as cm:
            s.sendto('\u2620', sockname)
        self.assertEqual(str(cm.exception),
                         "'str' does not support the buffer interface")
        with self.assertRaises(TypeError) as cm:
            s.sendto(5j, sockname)
        self.assertEqual(str(cm.exception),
                         "'complex' does not support the buffer interface")
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo', None)
        self.assertIn('not NoneType',str(cm.exception))
        # 3 args
        with self.assertRaises(TypeError) as cm:
            s.sendto('\u2620', 0, sockname)
        self.assertEqual(str(cm.exception),
                         "'str' does not support the buffer interface")
        with self.assertRaises(TypeError) as cm:
            s.sendto(5j, 0, sockname)
        self.assertEqual(str(cm.exception),
                         "'complex' does not support the buffer interface")
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo', 0, None)
        self.assertIn('not NoneType', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo', 'bar', sockname)
        self.assertIn('an integer is required', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo', None, None)
        self.assertIn('an integer is required', str(cm.exception))
        # wrong number of args
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo')
        self.assertIn('(1 given)', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo', 0, sockname, 4)
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
            return
        self.assertTrue(ip.find('.') >= 0, "Error resolving host to ip.")
        try:
            hname, aliases, ipaddrs = socket.gethostbyaddr(ip)
        except socket.error:
            # Probably a similar problem as above; skip this test
            return
        all_host_names = [hostname, hname] + aliases
        fqhn = socket.getfqdn(ip)
        if not fqhn in all_host_names:
            self.fail("Error testing host resolution mechanisms. (fqdn: %s, all: %s)" % (fqhn, repr(all_host_names)))

    @unittest.skipUnless(hasattr(socket, 'sethostname'), "test needs socket.sethostname()")
    @unittest.skipUnless(hasattr(socket, 'gethostname'), "test needs socket.gethostname()")
    def test_sethostname(self):
        oldhn = socket.gethostname()
        try:
            socket.sethostname('new')
        except socket.error as e:
            if e.errno == errno.EPERM:
                self.skipTest("test should be run as root")
            else:
                raise
        try:
            # running test as root!
            self.assertEqual(socket.gethostname(), 'new')
            # Should work with bytes objects too
            socket.sethostname(b'bar')
            self.assertEqual(socket.gethostname(), 'bar')
        finally:
            socket.sethostname(oldhn)

    def testRefCountGetNameInfo(self):
        # Testing reference count for getnameinfo
        if hasattr(sys, "getrefcount"):
            try:
                # On some versions, this loses a reference
                orig = sys.getrefcount(__name__)
                socket.getnameinfo(__name__,0)
            except TypeError:
                if sys.getrefcount(__name__) != orig:
                    self.fail("socket.getnameinfo loses a reference")

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
            mask = (1<<size) - 1
            for i in (0, 1, 0xffff, ~0xffff, 2, 0x01234567, 0x76543210):
                self.assertEqual(i & mask, func(func(i&mask)) & mask)

            swapped = func(mask)
            self.assertEqual(swapped & mask, mask)
            self.assertRaises(OverflowError, func, 1<<34)

    def testNtoHErrors(self):
        good_values = [ 1, 2, 3, 1, 2, 3 ]
        bad_values = [ -1, -2, -3, -1, -2, -3 ]
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
        # Try udp, but don't barf it it doesn't exist
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

    def testIPv4_inet_aton_fourbytes(self):
        if not hasattr(socket, 'inet_aton'):
            return  # No inet_aton, nothing to check
        # Test that issue1008086 and issue767150 are fixed.
        # It must return 4 bytes.
        self.assertEqual(b'\x00'*4, socket.inet_aton('0.0.0.0'))
        self.assertEqual(b'\xff'*4, socket.inet_aton('255.255.255.255'))

    def testIPv4toString(self):
        if not hasattr(socket, 'inet_pton'):
            return # No inet_pton() on this platform
        from socket import inet_aton as f, inet_pton, AF_INET
        g = lambda a: inet_pton(AF_INET, a)

        self.assertEqual(b'\x00\x00\x00\x00', f('0.0.0.0'))
        self.assertEqual(b'\xff\x00\xff\x00', f('255.0.255.0'))
        self.assertEqual(b'\xaa\xaa\xaa\xaa', f('170.170.170.170'))
        self.assertEqual(b'\x01\x02\x03\x04', f('1.2.3.4'))
        self.assertEqual(b'\xff\xff\xff\xff', f('255.255.255.255'))

        self.assertEqual(b'\x00\x00\x00\x00', g('0.0.0.0'))
        self.assertEqual(b'\xff\x00\xff\x00', g('255.0.255.0'))
        self.assertEqual(b'\xaa\xaa\xaa\xaa', g('170.170.170.170'))
        self.assertEqual(b'\xff\xff\xff\xff', g('255.255.255.255'))

    def testIPv6toString(self):
        if not hasattr(socket, 'inet_pton'):
            return # No inet_pton() on this platform
        try:
            from socket import inet_pton, AF_INET6, has_ipv6
            if not has_ipv6:
                return
        except ImportError:
            return
        f = lambda a: inet_pton(AF_INET6, a)

        self.assertEqual(b'\x00' * 16, f('::'))
        self.assertEqual(b'\x00' * 16, f('0::0'))
        self.assertEqual(b'\x00\x01' + b'\x00' * 14, f('1::'))
        self.assertEqual(
            b'\x45\xef\x76\xcb\x00\x1a\x56\xef\xaf\xeb\x0b\xac\x19\x24\xae\xae',
            f('45ef:76cb:1a:56ef:afeb:bac:1924:aeae')
        )

    def testStringToIPv4(self):
        if not hasattr(socket, 'inet_ntop'):
            return # No inet_ntop() on this platform
        from socket import inet_ntoa as f, inet_ntop, AF_INET
        g = lambda a: inet_ntop(AF_INET, a)

        self.assertEqual('1.0.1.0', f(b'\x01\x00\x01\x00'))
        self.assertEqual('170.85.170.85', f(b'\xaa\x55\xaa\x55'))
        self.assertEqual('255.255.255.255', f(b'\xff\xff\xff\xff'))
        self.assertEqual('1.2.3.4', f(b'\x01\x02\x03\x04'))

        self.assertEqual('1.0.1.0', g(b'\x01\x00\x01\x00'))
        self.assertEqual('170.85.170.85', g(b'\xaa\x55\xaa\x55'))
        self.assertEqual('255.255.255.255', g(b'\xff\xff\xff\xff'))

    def testStringToIPv6(self):
        if not hasattr(socket, 'inet_ntop'):
            return # No inet_ntop() on this platform
        try:
            from socket import inet_ntop, AF_INET6, has_ipv6
            if not has_ipv6:
                return
        except ImportError:
            return
        f = lambda a: inet_ntop(AF_INET6, a)

        self.assertEqual('::', f(b'\x00' * 16))
        self.assertEqual('::1', f(b'\x00' * 15 + b'\x01'))
        self.assertEqual(
            'aef:b01:506:1001:ffff:9997:55:170',
            f(b'\x0a\xef\x0b\x01\x05\x06\x10\x01\xff\xff\x99\x97\x00\x55\x01\x70')
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
            return
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
        self.assertRaises(socket.error, sock.send, b"spam")

    def testNewAttributes(self):
        # testing .family, .type and .protocol
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.assertEqual(sock.family, socket.AF_INET)
        self.assertEqual(sock.type, socket.SOCK_STREAM)
        self.assertEqual(sock.proto, 0)
        sock.close()

    def test_getsockaddrarg(self):
        host = '0.0.0.0'
        port = self._get_unused_port(bind_address=host)
        big_port = port + 65536
        neg_port = port - 65536
        sock = socket.socket()
        try:
            self.assertRaises(OverflowError, sock.bind, (host, big_port))
            self.assertRaises(OverflowError, sock.bind, (host, neg_port))
            sock.bind((host, port))
        finally:
            sock.close()

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
        if support.IPV6_ENABLED:
            socket.getaddrinfo('::1', 80)
        # port can be a string service name such as "http", a numeric
        # port number or None
        socket.getaddrinfo(HOST, "http")
        socket.getaddrinfo(HOST, 80)
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
        # test keyword arguments
        a = socket.getaddrinfo(HOST, None)
        b = socket.getaddrinfo(host=HOST, port=None)
        self.assertEqual(a, b)
        a = socket.getaddrinfo(HOST, None, socket.AF_INET)
        b = socket.getaddrinfo(HOST, None, family=socket.AF_INET)
        self.assertEqual(a, b)
        a = socket.getaddrinfo(HOST, None, 0, socket.SOCK_STREAM)
        b = socket.getaddrinfo(HOST, None, type=socket.SOCK_STREAM)
        self.assertEqual(a, b)
        a = socket.getaddrinfo(HOST, None, 0, 0, socket.SOL_TCP)
        b = socket.getaddrinfo(HOST, None, proto=socket.SOL_TCP)
        self.assertEqual(a, b)
        a = socket.getaddrinfo(HOST, None, 0, 0, 0, socket.AI_PASSIVE)
        b = socket.getaddrinfo(HOST, None, flags=socket.AI_PASSIVE)
        self.assertEqual(a, b)
        a = socket.getaddrinfo(None, 0, socket.AF_UNSPEC, socket.SOCK_STREAM, 0,
                               socket.AI_PASSIVE)
        b = socket.getaddrinfo(host=None, port=0, family=socket.AF_UNSPEC,
                               type=socket.SOCK_STREAM, proto=0,
                               flags=socket.AI_PASSIVE)
        self.assertEqual(a, b)
        # Issue #6697.
        self.assertRaises(UnicodeEncodeError, socket.getaddrinfo, 'localhost', '\uD800')

    def test_getnameinfo(self):
        # only IP addresses are allowed
        self.assertRaises(socket.error, socket.getnameinfo, ('mail.python.org',0), 0)

    @unittest.skipUnless(support.is_resource_enabled('network'),
                         'network is not enabled')
    def test_idna(self):
        support.requires('network')
        # these should all be successful
        socket.gethostbyname('испытание.python.org')
        socket.gethostbyname_ex('испытание.python.org')
        socket.getaddrinfo('испытание.python.org',0,socket.AF_UNSPEC,socket.SOCK_STREAM)
        # this may not work if the forward lookup choses the IPv6 address, as that doesn't
        # have a reverse entry yet
        # socket.gethostbyaddr('испытание.python.org')

    def check_sendall_interrupted(self, with_timeout):
        # socketpair() is not stricly required, but it makes things easier.
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
                c.sendall(b"x" * (1024**2))
            if with_timeout:
                signal.signal(signal.SIGALRM, ok_handler)
                signal.alarm(1)
                self.assertRaises(socket.timeout, c.sendall, b"x" * (1024**2))
        finally:
            signal.signal(signal.SIGALRM, old_alarm)
            c.close()
            s.close()

    def test_sendall_interrupted(self):
        self.check_sendall_interrupted(False)

    def test_sendall_interrupted_with_timeout(self):
        self.check_sendall_interrupted(True)

    def test_dealloc_warn(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        r = repr(sock)
        with self.assertWarns(ResourceWarning) as cm:
            sock = None
            support.gc_collect()
        self.assertIn(r, str(cm.warning.args[0]))
        # An open socket file object gets dereferenced after the socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        f = sock.makefile('rb')
        r = repr(sock)
        sock = None
        support.gc_collect()
        with self.assertWarns(ResourceWarning):
            f = None
            support.gc_collect()

    def test_name_closed_socketio(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            fp = sock.makefile("rb")
            fp.close()
            self.assertEqual(repr(fp), "<_io.BufferedReader name=-1>")

    def test_pickle(self):
        sock = socket.socket()
        with sock:
            for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
                self.assertRaises(TypeError, pickle.dumps, sock, protocol)

    def test_listen_backlog0(self):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.bind((HOST, 0))
        # backlog = 0
        srv.listen(0)
        srv.close()


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
        msg = b''
        while 1:
            read = self.cli_conn.recv(1024)
            if not read:
                break
            msg += read
        self.assertEqual(msg, b'f' * 2048)

    def _testSendAll(self):
        big_chunk = b'f' * 2048
        self.serv_conn.sendall(big_chunk)

    def testFromFd(self):
        # Testing fromfd()
        fd = self.cli_conn.fileno()
        sock = socket.fromfd(fd, socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(sock.close)
        self.assertIsInstance(sock, socket.socket)
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

    def testDetach(self):
        # Testing detach()
        fileno = self.cli_conn.fileno()
        f = self.cli_conn.detach()
        self.assertEqual(f, fileno)
        # cli_conn cannot be used anymore...
        self.assertRaises(socket.error, self.cli_conn.recv, 1024)
        self.cli_conn.close()
        # ...but we can create another socket using the (still open)
        # file descriptor
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, fileno=f)
        self.addCleanup(sock.close)
        msg = sock.recv(1024)
        self.assertEqual(msg, MSG)

    def _testDetach(self):
        self.serv_conn.send(MSG)

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
        self.assertEqual(sd.recv(1), b'')

        # Calling close() many times should be safe.
        conn.close()
        conn.close()

    def _testClose(self):
        self.cli.connect((HOST, self.port))
        time.sleep(1.0)

@unittest.skipUnless(thread, 'Threading required for this test.')
class BasicSocketPairTest(SocketPairTest):

    def __init__(self, methodName='runTest'):
        SocketPairTest.__init__(self, methodName=methodName)

    def _check_defaults(self, sock):
        self.assertIsInstance(sock, socket.socket)
        if hasattr(socket, 'AF_UNIX'):
            self.assertEqual(sock.family, socket.AF_UNIX)
        else:
            self.assertEqual(sock.family, socket.AF_INET)
        self.assertEqual(sock.type, socket.SOCK_STREAM)
        self.assertEqual(sock.proto, 0)

    def _testDefaults(self):
        self._check_defaults(self.cli)

    def testDefaults(self):
        self._check_defaults(self.serv)

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
        ThreadedTCPSocketTest.__init__(self, methodName=methodName)

    def testSetBlocking(self):
        # Testing whether set blocking works
        self.serv.setblocking(0)
        start = time.time()
        try:
            self.serv.accept()
        except socket.error:
            pass
        end = time.time()
        self.assertTrue((end - start) < 1.0, "Error setting non-blocking mode.")

    def _testSetBlocking(self):
        pass

    if hasattr(socket, "SOCK_NONBLOCK"):
        def testInitNonBlocking(self):
            v = linux_version()
            if v < (2, 6, 28):
                self.skipTest("Linux kernel 2.6.28 or higher required, not %s"
                              % ".".join(map(str, v)))
            # reinit server socket
            self.serv.close()
            self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM |
                                                      socket.SOCK_NONBLOCK)
            self.port = support.bind_port(self.serv)
            self.serv.listen(1)
            # actual testing
            start = time.time()
            try:
                self.serv.accept()
            except socket.error:
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
        try:
            conn, addr = self.serv.accept()
        except socket.error:
            pass
        else:
            self.fail("Error trying to do non-blocking accept.")
        read, write, err = select.select([self.serv], [], [])
        if self.serv in read:
            conn, addr = self.serv.accept()
            conn.close()
        else:
            self.fail("Error trying to do accept after select.")

    def _testAccept(self):
        time.sleep(0.1)
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
        conn.setblocking(0)
        try:
            msg = conn.recv(len(MSG))
        except socket.error:
            pass
        else:
            self.fail("Error trying to do non-blocking recv.")
        read, write, err = select.select([conn], [], [])
        if conn in read:
            msg = conn.recv(len(MSG))
            conn.close()
            self.assertEqual(msg, MSG)
        else:
            self.fail("Error during select call to non-blocking socket.")

    def _testRecv(self):
        self.cli.connect((HOST, self.port))
        time.sleep(0.1)
        self.cli.send(MSG)

@unittest.skipUnless(thread, 'Threading required for this test.')
class FileObjectClassTestCase(SocketConnectedTest):
    """Unit tests for the object returned by socket.makefile()

    self.read_file is the io object returned by makefile() on
    the client connection.  You can read from this file to
    get output from the server.

    self.write_file is the io object returned by makefile() on the
    server connection.  You can write to this file to send output
    to the client.
    """

    bufsize = -1 # Use default buffer size
    encoding = 'utf-8'
    errors = 'strict'
    newline = None

    read_mode = 'rb'
    read_msg = MSG
    write_mode = 'wb'
    write_msg = MSG

    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def setUp(self):
        self.evt1, self.evt2, self.serv_finished, self.cli_finished = [
            threading.Event() for i in range(4)]
        SocketConnectedTest.setUp(self)
        self.read_file = self.cli_conn.makefile(
            self.read_mode, self.bufsize,
            encoding = self.encoding,
            errors = self.errors,
            newline = self.newline)

    def tearDown(self):
        self.serv_finished.set()
        self.read_file.close()
        self.assertTrue(self.read_file.closed)
        self.read_file = None
        SocketConnectedTest.tearDown(self)

    def clientSetUp(self):
        SocketConnectedTest.clientSetUp(self)
        self.write_file = self.serv_conn.makefile(
            self.write_mode, self.bufsize,
            encoding = self.encoding,
            errors = self.errors,
            newline = self.newline)

    def clientTearDown(self):
        self.cli_finished.set()
        self.write_file.close()
        self.assertTrue(self.write_file.closed)
        self.write_file = None
        SocketConnectedTest.clientTearDown(self)

    def testReadAfterTimeout(self):
        # Issue #7322: A file object must disallow further reads
        # after a timeout has occurred.
        self.cli_conn.settimeout(1)
        self.read_file.read(3)
        # First read raises a timeout
        self.assertRaises(socket.timeout, self.read_file.read, 1)
        # Second read is disallowed
        with self.assertRaises(IOError) as ctx:
            self.read_file.read(1)
        self.assertIn("cannot read from timed out object", str(ctx.exception))

    def _testReadAfterTimeout(self):
        self.write_file.write(self.write_msg[0:3])
        self.write_file.flush()
        self.serv_finished.wait()

    def testSmallRead(self):
        # Performing small file read test
        first_seg = self.read_file.read(len(self.read_msg)-3)
        second_seg = self.read_file.read(3)
        msg = first_seg + second_seg
        self.assertEqual(msg, self.read_msg)

    def _testSmallRead(self):
        self.write_file.write(self.write_msg)
        self.write_file.flush()

    def testFullRead(self):
        # read until EOF
        msg = self.read_file.read()
        self.assertEqual(msg, self.read_msg)

    def _testFullRead(self):
        self.write_file.write(self.write_msg)
        self.write_file.close()

    def testUnbufferedRead(self):
        # Performing unbuffered file read test
        buf = type(self.read_msg)()
        while 1:
            char = self.read_file.read(1)
            if not char:
                break
            buf += char
        self.assertEqual(buf, self.read_msg)

    def _testUnbufferedRead(self):
        self.write_file.write(self.write_msg)
        self.write_file.flush()

    def testReadline(self):
        # Performing file readline test
        line = self.read_file.readline()
        self.assertEqual(line, self.read_msg)

    def _testReadline(self):
        self.write_file.write(self.write_msg)
        self.write_file.flush()

    def testCloseAfterMakefile(self):
        # The file returned by makefile should keep the socket open.
        self.cli_conn.close()
        # read until EOF
        msg = self.read_file.read()
        self.assertEqual(msg, self.read_msg)

    def _testCloseAfterMakefile(self):
        self.write_file.write(self.write_msg)
        self.write_file.flush()

    def testMakefileAfterMakefileClose(self):
        self.read_file.close()
        msg = self.cli_conn.recv(len(MSG))
        if isinstance(self.read_msg, str):
            msg = msg.decode()
        self.assertEqual(msg, self.read_msg)

    def _testMakefileAfterMakefileClose(self):
        self.write_file.write(self.write_msg)
        self.write_file.flush()

    def testClosedAttr(self):
        self.assertTrue(not self.read_file.closed)

    def _testClosedAttr(self):
        self.assertTrue(not self.write_file.closed)

    def testAttributes(self):
        self.assertEqual(self.read_file.mode, self.read_mode)
        self.assertEqual(self.read_file.name, self.cli_conn.fileno())

    def _testAttributes(self):
        self.assertEqual(self.write_file.mode, self.write_mode)
        self.assertEqual(self.write_file.name, self.serv_conn.fileno())

    def testRealClose(self):
        self.read_file.close()
        self.assertRaises(ValueError, self.read_file.fileno)
        self.cli_conn.close()
        self.assertRaises(socket.error, self.cli_conn.getsockname)

    def _testRealClose(self):
        pass


class FileObjectInterruptedTestCase(unittest.TestCase):
    """Test that the file object correctly handles EINTR internally."""

    class MockSocket(object):
        def __init__(self, recv_funcs=()):
            # A generator that returns callables that we'll call for each
            # call to recv().
            self._recv_step = iter(recv_funcs)

        def recv_into(self, buffer):
            data = next(self._recv_step)()
            assert len(buffer) >= len(data)
            buffer[:len(data)] = data
            return len(data)

        def _decref_socketios(self):
            pass

        def _textiowrap_for_test(self, buffering=-1):
            raw = socket.SocketIO(self, "r")
            if buffering < 0:
                buffering = io.DEFAULT_BUFFER_SIZE
            if buffering == 0:
                return raw
            buffer = io.BufferedReader(raw, buffering)
            text = io.TextIOWrapper(buffer, None, None)
            text.mode = "rb"
            return text

    @staticmethod
    def _raise_eintr():
        raise socket.error(errno.EINTR)

    def _textiowrap_mock_socket(self, mock, buffering=-1):
        raw = socket.SocketIO(mock, "r")
        if buffering < 0:
            buffering = io.DEFAULT_BUFFER_SIZE
        if buffering == 0:
            return raw
        buffer = io.BufferedReader(raw, buffering)
        text = io.TextIOWrapper(buffer, None, None)
        text.mode = "rb"
        return text

    def _test_readline(self, size=-1, buffering=-1):
        mock_sock = self.MockSocket(recv_funcs=[
                lambda : b"This is the first line\nAnd the sec",
                self._raise_eintr,
                lambda : b"ond line is here\n",
                lambda : b"",
                lambda : b"",  # XXX(gps): io library does an extra EOF read
            ])
        fo = mock_sock._textiowrap_for_test(buffering=buffering)
        self.assertEqual(fo.readline(size), "This is the first line\n")
        self.assertEqual(fo.readline(size), "And the second line is here\n")

    def _test_read(self, size=-1, buffering=-1):
        mock_sock = self.MockSocket(recv_funcs=[
                lambda : b"This is the first line\nAnd the sec",
                self._raise_eintr,
                lambda : b"ond line is here\n",
                lambda : b"",
                lambda : b"",  # XXX(gps): io library does an extra EOF read
            ])
        expecting = (b"This is the first line\n"
                     b"And the second line is here\n")
        fo = mock_sock._textiowrap_for_test(buffering=buffering)
        if buffering == 0:
            data = b''
        else:
            data = ''
            expecting = expecting.decode('utf-8')
        while len(data) != len(expecting):
            part = fo.read(size)
            if not part:
                break
            data += part
        self.assertEqual(data, expecting)

    def test_default(self):
        self._test_readline()
        self._test_readline(size=100)
        self._test_read()
        self._test_read(size=100)

    def test_with_1k_buffer(self):
        self._test_readline(buffering=1024)
        self._test_readline(size=100, buffering=1024)
        self._test_read(buffering=1024)
        self._test_read(size=100, buffering=1024)

    def _test_readline_no_buffer(self, size=-1):
        mock_sock = self.MockSocket(recv_funcs=[
                lambda : b"a",
                lambda : b"\n",
                lambda : b"B",
                self._raise_eintr,
                lambda : b"b",
                lambda : b"",
            ])
        fo = mock_sock._textiowrap_for_test(buffering=0)
        self.assertEqual(fo.readline(size), b"a\n")
        self.assertEqual(fo.readline(size), b"Bb")

    def test_no_buffer(self):
        self._test_readline_no_buffer()
        self._test_readline_no_buffer(size=4)
        self._test_read(buffering=0)
        self._test_read(size=100, buffering=0)


class UnbufferedFileObjectClassTestCase(FileObjectClassTestCase):

    """Repeat the tests from FileObjectClassTestCase with bufsize==0.

    In this case (and in this case only), it should be possible to
    create a file object, read a line from it, create another file
    object, read another line from it, without loss of data in the
    first file object's buffer.  Note that http.client relies on this
    when reading multiple requests from the same socket."""

    bufsize = 0 # Use unbuffered mode

    def testUnbufferedReadline(self):
        # Read a line, create a new file object, read another line with it
        line = self.read_file.readline() # first line
        self.assertEqual(line, b"A. " + self.write_msg) # first line
        self.read_file = self.cli_conn.makefile('rb', 0)
        line = self.read_file.readline() # second line
        self.assertEqual(line, b"B. " + self.write_msg) # second line

    def _testUnbufferedReadline(self):
        self.write_file.write(b"A. " + self.write_msg)
        self.write_file.write(b"B. " + self.write_msg)
        self.write_file.flush()

    def testMakefileClose(self):
        # The file returned by makefile should keep the socket open...
        self.cli_conn.close()
        msg = self.cli_conn.recv(1024)
        self.assertEqual(msg, self.read_msg)
        # ...until the file is itself closed
        self.read_file.close()
        self.assertRaises(socket.error, self.cli_conn.recv, 1024)

    def _testMakefileClose(self):
        self.write_file.write(self.write_msg)
        self.write_file.flush()

    def testMakefileCloseSocketDestroy(self):
        refcount_before = sys.getrefcount(self.cli_conn)
        self.read_file.close()
        refcount_after = sys.getrefcount(self.cli_conn)
        self.assertEqual(refcount_before - 1, refcount_after)

    def _testMakefileCloseSocketDestroy(self):
        pass

    # Non-blocking ops
    # NOTE: to set `read_file` as non-blocking, we must call
    # `cli_conn.setblocking` and vice-versa (see setUp / clientSetUp).

    def testSmallReadNonBlocking(self):
        self.cli_conn.setblocking(False)
        self.assertEqual(self.read_file.readinto(bytearray(10)), None)
        self.assertEqual(self.read_file.read(len(self.read_msg) - 3), None)
        self.evt1.set()
        self.evt2.wait(1.0)
        first_seg = self.read_file.read(len(self.read_msg) - 3)
        if first_seg is None:
            # Data not arrived (can happen under Windows), wait a bit
            time.sleep(0.5)
            first_seg = self.read_file.read(len(self.read_msg) - 3)
        buf = bytearray(10)
        n = self.read_file.readinto(buf)
        self.assertEqual(n, 3)
        msg = first_seg + buf[:n]
        self.assertEqual(msg, self.read_msg)
        self.assertEqual(self.read_file.readinto(bytearray(16)), None)
        self.assertEqual(self.read_file.read(1), None)

    def _testSmallReadNonBlocking(self):
        self.evt1.wait(1.0)
        self.write_file.write(self.write_msg)
        self.write_file.flush()
        self.evt2.set()
        # Avoid cloding the socket before the server test has finished,
        # otherwise system recv() will return 0 instead of EWOULDBLOCK.
        self.serv_finished.wait(5.0)

    def testWriteNonBlocking(self):
        self.cli_finished.wait(5.0)
        # The client thread can't skip directly - the SkipTest exception
        # would appear as a failure.
        if self.serv_skipped:
            self.skipTest(self.serv_skipped)

    def _testWriteNonBlocking(self):
        self.serv_skipped = None
        self.serv_conn.setblocking(False)
        # Try to saturate the socket buffer pipe with repeated large writes.
        BIG = b"x" * (1024 ** 2)
        LIMIT = 10
        # The first write() succeeds since a chunk of data can be buffered
        n = self.write_file.write(BIG)
        self.assertGreater(n, 0)
        for i in range(LIMIT):
            n = self.write_file.write(BIG)
            if n is None:
                # Succeeded
                break
            self.assertGreater(n, 0)
        else:
            # Let us know that this test didn't manage to establish
            # the expected conditions. This is not a failure in itself but,
            # if it happens repeatedly, the test should be fixed.
            self.serv_skipped = "failed to saturate the socket buffer"


class LineBufferedFileObjectClassTestCase(FileObjectClassTestCase):

    bufsize = 1 # Default-buffered for reading; line-buffered for writing


class SmallBufferedFileObjectClassTestCase(FileObjectClassTestCase):

    bufsize = 2 # Exercise the buffering code


class UnicodeReadFileObjectClassTestCase(FileObjectClassTestCase):
    """Tests for socket.makefile() in text mode (rather than binary)"""

    read_mode = 'r'
    read_msg = MSG.decode('utf-8')
    write_mode = 'wb'
    write_msg = MSG
    newline = ''


class UnicodeWriteFileObjectClassTestCase(FileObjectClassTestCase):
    """Tests for socket.makefile() in text mode (rather than binary)"""

    read_mode = 'rb'
    read_msg = MSG
    write_mode = 'w'
    write_msg = MSG.decode('utf-8')
    newline = ''


class UnicodeReadWriteFileObjectClassTestCase(FileObjectClassTestCase):
    """Tests for socket.makefile() in text mode (rather than binary)"""

    read_mode = 'r'
    read_msg = MSG.decode('utf-8')
    write_mode = 'w'
    write_msg = MSG.decode('utf-8')
    newline = ''


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
        port = support.find_unused_port()
        cli = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(cli.close)
        with self.assertRaises(socket.error) as cm:
            cli.connect((HOST, port))
        self.assertEqual(cm.exception.errno, errno.ECONNREFUSED)

    def test_create_connection(self):
        # Issue #9792: errors raised by create_connection() should have
        # a proper errno attribute.
        port = support.find_unused_port()
        with self.assertRaises(socket.error) as cm:
            socket.create_connection((HOST, port))
        self.assertEqual(cm.exception.errno, errno.ECONNREFUSED)

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
        self.source_port = support.find_unused_port()

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
        conn.send(b"done!")
    testOutsideTimeout = testInsideTimeout

    def _testInsideTimeout(self):
        self.cli = sock = socket.create_connection((HOST, self.port))
        data = sock.recv(5)
        self.assertEqual(data, b"done!")

    def _testOutsideTimeout(self):
        self.cli = sock = socket.create_connection((HOST, self.port), timeout=1)
        self.assertRaises(socket.timeout, lambda: sock.recv(5))


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

    def testInterruptedTimeout(self):
        # XXX I don't know how to do this test on MSWindows or any other
        # plaform that doesn't support signal.alarm() or os.kill(), though
        # the bug should have existed on all platforms.
        if not hasattr(signal, "alarm"):
            return                  # can only test on *nix
        self.serv.settimeout(5.0)   # must be longer than alarm
        class Alarm(Exception):
            pass
        def alarm_handler(signal, frame):
            raise Alarm
        old_alarm = signal.signal(signal.SIGALRM, alarm_handler)
        try:
            signal.alarm(2)    # POSIX allows alarm to be up to 1 second early
            try:
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

class UDPTimeoutTest(SocketTCPTest):

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

class TestLinuxAbstractNamespace(unittest.TestCase):

    UNIX_PATH_MAX = 108

    def testLinuxAbstractNamespace(self):
        address = b"\x00python-test-hello\x00\xff"
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s1:
            s1.bind(address)
            s1.listen(1)
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s2:
                s2.connect(s1.getsockname())
                with s1.accept()[0] as s3:
                    self.assertEqual(s1.getsockname(), address)
                    self.assertEqual(s2.getpeername(), address)

    def testMaxName(self):
        address = b"\x00" + b"h" * (self.UNIX_PATH_MAX - 1)
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            s.bind(address)
            self.assertEqual(s.getsockname(), address)

    def testNameOverflow(self):
        address = "\x00" + "h" * self.UNIX_PATH_MAX
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            self.assertRaises(socket.error, s.bind, address)


@unittest.skipUnless(thread, 'Threading required for this test.')
class BufferIOTest(SocketConnectedTest):
    """
    Test the buffer versions of socket.recv() and socket.send().
    """
    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def testRecvIntoArray(self):
        buf = bytearray(1024)
        nbytes = self.cli_conn.recv_into(buf)
        self.assertEqual(nbytes, len(MSG))
        msg = buf[:len(MSG)]
        self.assertEqual(msg, MSG)

    def _testRecvIntoArray(self):
        buf = bytes(MSG)
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
        buf = bytearray(1024)
        nbytes, addr = self.cli_conn.recvfrom_into(buf)
        self.assertEqual(nbytes, len(MSG))
        msg = buf[:len(MSG)]
        self.assertEqual(msg, MSG)

    def _testRecvFromIntoArray(self):
        buf = bytes(MSG)
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
    if not os.path.isfile("/proc/modules"):
        return False
    with open("/proc/modules") as f:
        for line in f:
            if line.startswith("tipc "):
                return True
    if support.verbose:
        print("TIPC module is not loaded, please 'sudo modprobe tipc'")
    return False

class TIPCTest (unittest.TestCase):
    def testRDM(self):
        srv = socket.socket(socket.AF_TIPC, socket.SOCK_RDM)
        cli = socket.socket(socket.AF_TIPC, socket.SOCK_RDM)

        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srvaddr = (socket.TIPC_ADDR_NAMESEQ, TIPC_STYPE,
                TIPC_LOWER, TIPC_UPPER)
        srv.bind(srvaddr)

        sendaddr = (socket.TIPC_ADDR_NAME, TIPC_STYPE,
                TIPC_LOWER + int((TIPC_UPPER - TIPC_LOWER) / 2), 0)
        cli.sendto(MSG, sendaddr)

        msg, recvaddr = srv.recvfrom(1024)

        self.assertEqual(cli.getsockname(), recvaddr)
        self.assertEqual(msg, MSG)


class TIPCThreadableTest (unittest.TestCase, ThreadableTest):
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
        # The is a hittable race between serverExplicitReady() and the
        # accept() call; sleep a little while to avoid it, otherwise
        # we could get an exception
        time.sleep(0.1)
        self.cli = socket.socket(socket.AF_TIPC, socket.SOCK_STREAM)
        addr = (socket.TIPC_ADDR_NAME, TIPC_STYPE,
                TIPC_LOWER + int((TIPC_UPPER - TIPC_LOWER) / 2), 0)
        self.cli.connect(addr)
        self.cliaddr = self.cli.getsockname()

    def testStream(self):
        msg = self.conn.recv(1024)
        self.assertEqual(msg, MSG)
        self.assertEqual(self.cliaddr, self.connaddr)

    def _testStream(self):
        self.cli.send(MSG)
        self.cli.close()


@unittest.skipUnless(thread, 'Threading required for this test.')
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
            self.assertRaises(socket.error, sock.sendall, b'foo')
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
        self.assertRaises(socket.error, sock.sendall, b'foo')


@unittest.skipUnless(hasattr(socket, "SOCK_CLOEXEC"),
                     "SOCK_CLOEXEC not defined")
@unittest.skipUnless(fcntl, "module fcntl not available")
class CloexecConstantTest(unittest.TestCase):
    def test_SOCK_CLOEXEC(self):
        v = linux_version()
        if v < (2, 6, 28):
            self.skipTest("Linux kernel 2.6.28 or higher required, not %s"
                          % ".".join(map(str, v)))
        with socket.socket(socket.AF_INET,
                           socket.SOCK_STREAM | socket.SOCK_CLOEXEC) as s:
            self.assertTrue(s.type & socket.SOCK_CLOEXEC)
            self.assertTrue(fcntl.fcntl(s, fcntl.F_GETFD) & fcntl.FD_CLOEXEC)


@unittest.skipUnless(hasattr(socket, "SOCK_NONBLOCK"),
                     "SOCK_NONBLOCK not defined")
class NonblockConstantTest(unittest.TestCase):
    def checkNonblock(self, s, nonblock=True, timeout=0.0):
        if nonblock:
            self.assertTrue(s.type & socket.SOCK_NONBLOCK)
            self.assertEqual(s.gettimeout(), timeout)
        else:
            self.assertFalse(s.type & socket.SOCK_NONBLOCK)
            self.assertEqual(s.gettimeout(), None)

    def test_SOCK_NONBLOCK(self):
        v = linux_version()
        if v < (2, 6, 28):
            self.skipTest("Linux kernel 2.6.28 or higher required, not %s"
                          % ".".join(map(str, v)))
        # a lot of it seems silly and redundant, but I wanted to test that
        # changing back and forth worked ok
        with socket.socket(socket.AF_INET,
                           socket.SOCK_STREAM | socket.SOCK_NONBLOCK) as s:
            self.checkNonblock(s)
            s.setblocking(1)
            self.checkNonblock(s, False)
            s.setblocking(0)
            self.checkNonblock(s)
            s.settimeout(None)
            self.checkNonblock(s, False)
            s.settimeout(2.0)
            self.checkNonblock(s, timeout=2.0)
            s.setblocking(1)
            self.checkNonblock(s, False)
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


def test_main():
    tests = [GeneralModuleTests, BasicTCPTest, TCPCloserTest, TCPTimeoutTest,
             TestExceptions, BufferIOTest, BasicTCPTest2, BasicUDPTest, UDPTimeoutTest ]

    tests.extend([
        NonBlockingTCPTests,
        FileObjectClassTestCase,
        FileObjectInterruptedTestCase,
        UnbufferedFileObjectClassTestCase,
        LineBufferedFileObjectClassTestCase,
        SmallBufferedFileObjectClassTestCase,
        UnicodeReadFileObjectClassTestCase,
        UnicodeWriteFileObjectClassTestCase,
        UnicodeReadWriteFileObjectClassTestCase,
        NetworkConnectionNoServer,
        NetworkConnectionAttributesTest,
        NetworkConnectionBehaviourTest,
        ContextManagersTest,
        CloexecConstantTest,
        NonblockConstantTest
    ])
    if hasattr(socket, "socketpair"):
        tests.append(BasicSocketPairTest)
    if sys.platform == 'linux2':
        tests.append(TestLinuxAbstractNamespace)
    if isTipcAvailable():
        tests.append(TIPCTest)
        tests.append(TIPCThreadableTest)

    thread_info = support.threading_setup()
    support.run_unittest(*tests)
    support.threading_cleanup(*thread_info)

if __name__ == "__main__":
    test_main()
