#!/usr/bin/env python

import unittest
from test import test_support

import errno
import socket
import select
import thread, threading
import time
import traceback
import Queue
import sys
import os
import array
from weakref import proxy
import signal

HOST = test_support.HOST
MSG = 'Michael Gilfix was here\n'

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
        self.client_ready.set()
        self.clientSetUp()
        with test_support._check_py3k_warnings():
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
        def raise_error(*args, **kwargs):
            raise socket.error
        def raise_herror(*args, **kwargs):
            raise socket.herror
        def raise_gaierror(*args, **kwargs):
            raise socket.gaierror
        self.failUnlessRaises(socket.error, raise_error,
                              "Error raising socket exception.")
        self.failUnlessRaises(socket.error, raise_herror,
                              "Error raising socket exception.")
        self.failUnlessRaises(socket.error, raise_gaierror,
                              "Error raising socket exception.")

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
        self.assert_(ip.find('.') >= 0, "Error resolving host to ip.")
        try:
            hname, aliases, ipaddrs = socket.gethostbyaddr(ip)
        except socket.error:
            # Probably a similar problem as above; skip this test
            return
        all_host_names = [hostname, hname] + aliases
        fqhn = socket.getfqdn(ip)
        if not fqhn in all_host_names:
            self.fail("Error testing host resolution mechanisms. (fqdn: %s, all: %s)" % (fqhn, repr(all_host_names)))

    def testRefCountGetNameInfo(self):
        # Testing reference count for getnameinfo
        if hasattr(sys, "getrefcount"):
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
        if sys.platform in ('linux2', 'freebsd4', 'freebsd5', 'freebsd6',
                            'freebsd7', 'freebsd8', 'darwin'):
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
        self.assertEquals('\x00'*4, socket.inet_aton('0.0.0.0'))
        self.assertEquals('\xff'*4, socket.inet_aton('255.255.255.255'))

    def testIPv4toString(self):
        if not hasattr(socket, 'inet_pton'):
            return # No inet_pton() on this platform
        from socket import inet_aton as f, inet_pton, AF_INET
        g = lambda a: inet_pton(AF_INET, a)

        self.assertEquals('\x00\x00\x00\x00', f('0.0.0.0'))
        self.assertEquals('\xff\x00\xff\x00', f('255.0.255.0'))
        self.assertEquals('\xaa\xaa\xaa\xaa', f('170.170.170.170'))
        self.assertEquals('\x01\x02\x03\x04', f('1.2.3.4'))
        self.assertEquals('\xff\xff\xff\xff', f('255.255.255.255'))

        self.assertEquals('\x00\x00\x00\x00', g('0.0.0.0'))
        self.assertEquals('\xff\x00\xff\x00', g('255.0.255.0'))
        self.assertEquals('\xaa\xaa\xaa\xaa', g('170.170.170.170'))
        self.assertEquals('\xff\xff\xff\xff', g('255.255.255.255'))

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

        self.assertEquals('\x00' * 16, f('::'))
        self.assertEquals('\x00' * 16, f('0::0'))
        self.assertEquals('\x00\x01' + '\x00' * 14, f('1::'))
        self.assertEquals(
            '\x45\xef\x76\xcb\x00\x1a\x56\xef\xaf\xeb\x0b\xac\x19\x24\xae\xae',
            f('45ef:76cb:1a:56ef:afeb:bac:1924:aeae')
        )

    def testStringToIPv4(self):
        if not hasattr(socket, 'inet_ntop'):
            return # No inet_ntop() on this platform
        from socket import inet_ntoa as f, inet_ntop, AF_INET
        g = lambda a: inet_ntop(AF_INET, a)

        self.assertEquals('1.0.1.0', f('\x01\x00\x01\x00'))
        self.assertEquals('170.85.170.85', f('\xaa\x55\xaa\x55'))
        self.assertEquals('255.255.255.255', f('\xff\xff\xff\xff'))
        self.assertEquals('1.2.3.4', f('\x01\x02\x03\x04'))

        self.assertEquals('1.0.1.0', g('\x01\x00\x01\x00'))
        self.assertEquals('170.85.170.85', g('\xaa\x55\xaa\x55'))
        self.assertEquals('255.255.255.255', g('\xff\xff\xff\xff'))

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

        self.assertEquals('::', f('\x00' * 16))
        self.assertEquals('::1', f('\x00' * 15 + '\x01'))
        self.assertEquals(
            'aef:b01:506:1001:ffff:9997:55:170',
            f('\x0a\xef\x0b\x01\x05\x06\x10\x01\xff\xff\x99\x97\x00\x55\x01\x70')
        )

    # XXX The following don't test module-level functionality...

    def testSockName(self):
        # Testing getsockname().  Use a temporary socket to elicit an unused
        # ephemeral port that we can use later in the test.
        tempsock = socket.socket()
        tempsock.bind(("0.0.0.0", 0))
        (host, port) = tempsock.getsockname()
        tempsock.close()
        del tempsock

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind(("0.0.0.0", port))
        name = sock.getsockname()
        # XXX(nnorwitz): http://tinyurl.com/os5jz seems to indicate
        # it reasonable to get the host's addr in addition to 0.0.0.0.
        # At least for eCos.  This is required for the S/390 to pass.
        my_ip_addr = socket.gethostbyname(socket.gethostname())
        self.assert_(name[0] in ("0.0.0.0", my_ip_addr), '%s invalid' % name[0])
        self.assertEqual(name[1], port)

    def testGetSockOpt(self):
        # Testing getsockopt()
        # We know a socket should start without reuse==0
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        reuse = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR)
        self.failIf(reuse != 0, "initial mode is reuse")

    def testSetSockOpt(self):
        # Testing setsockopt()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        reuse = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR)
        self.failIf(reuse == 0, "failed to set reuse mode")

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

    def test_sock_ioctl(self):
        if os.name != "nt":
            return
        self.assert_(hasattr(socket.socket, 'ioctl'))
        self.assert_(hasattr(socket, 'SIO_RCVALL'))
        self.assert_(hasattr(socket, 'RCVALL_ON'))
        self.assert_(hasattr(socket, 'RCVALL_OFF'))


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

    def testFromFd(self):
        # Testing fromfd()
        if not hasattr(socket, "fromfd"):
            return # On Windows, this doesn't exist
        fd = self.cli_conn.fileno()
        sock = socket.fromfd(fd, socket.AF_INET, socket.SOCK_STREAM)
        msg = sock.recv(1024)
        self.assertEqual(msg, MSG)

    def _testFromFd(self):
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
        self.assert_((end - start) < 1.0, "Error setting non-blocking mode.")

    def _testSetBlocking(self):
        pass

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
        else:
            self.fail("Error trying to do accept after select.")

    def _testAccept(self):
        time.sleep(0.1)
        self.cli.connect((HOST, self.port))

    def testConnect(self):
        # Testing non-blocking connect
        conn, addr = self.serv.accept()

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
            self.assertEqual(msg, MSG)
        else:
            self.fail("Error during select call to non-blocking socket.")

    def _testRecv(self):
        self.cli.connect((HOST, self.port))
        time.sleep(0.1)
        self.cli.send(MSG)

class FileObjectClassTestCase(SocketConnectedTest):

    bufsize = -1 # Use default buffer size

    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def setUp(self):
        SocketConnectedTest.setUp(self)
        self.serv_file = self.cli_conn.makefile('rb', self.bufsize)

    def tearDown(self):
        self.serv_file.close()
        self.assert_(self.serv_file.closed)
        self.serv_file = None
        SocketConnectedTest.tearDown(self)

    def clientSetUp(self):
        SocketConnectedTest.clientSetUp(self)
        self.cli_file = self.serv_conn.makefile('wb')

    def clientTearDown(self):
        self.cli_file.close()
        self.assert_(self.cli_file.closed)
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
        self.assert_(not self.serv_file.closed)

    def _testClosedAttr(self):
        self.assert_(not self.cli_file.closed)


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
        self.assertEquals(fo.readline(size), "This is the first line\n")
        self.assertEquals(fo.readline(size), "And the second line is here\n")

    def _test_read(self, size=-1, **kwargs):
        mock_sock = self.MockSocket(recv_funcs=[
                lambda : "This is the first line\nAnd the sec",
                self._raise_eintr,
                lambda : "ond line is here\n",
                lambda : "",
            ])
        fo = socket._fileobject(mock_sock, **kwargs)
        self.assertEquals(fo.read(size), "This is the first line\n"
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
        self.assertEquals(fo.readline(size), "aa\n")
        self.assertEquals(fo.readline(size), "BBbb")

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
    def testWithoutServer(self):
        port = test_support.find_unused_port()
        self.failUnlessRaises(
            socket.error,
            lambda: socket.create_connection((HOST, port))
        )

class NetworkConnectionAttributesTest(SocketTCPTest, ThreadableTest):

    def __init__(self, methodName='runTest'):
        SocketTCPTest.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        pass

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)

    def _justAccept(self):
        conn, addr = self.serv.accept()

    testFamily = _justAccept
    def _testFamily(self):
        self.cli = socket.create_connection((HOST, self.port), timeout=30)
        self.assertEqual(self.cli.family, 2)

    testTimeoutDefault = _justAccept
    def _testTimeoutDefault(self):
        # passing no explicit timeout uses socket's global default
        self.assert_(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(42)
        try:
            self.cli = socket.create_connection((HOST, self.port))
        finally:
            socket.setdefaulttimeout(None)
        self.assertEquals(self.cli.gettimeout(), 42)

    testTimeoutNone = _justAccept
    def _testTimeoutNone(self):
        # None timeout means the same as sock.settimeout(None)
        self.assert_(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            self.cli = socket.create_connection((HOST, self.port), timeout=None)
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
        self.assertEqual(self.cli.gettimeout(), 30)

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
        time.sleep(3)
        conn.send("done!")
    testOutsideTimeout = testInsideTimeout

    def _testInsideTimeout(self):
        self.cli = sock = socket.create_connection((HOST, self.port))
        data = sock.recv(5)
        self.assertEqual(data, "done!")

    def _testOutsideTimeout(self):
        self.cli = sock = socket.create_connection((HOST, self.port), timeout=1)
        self.failUnlessRaises(socket.timeout, lambda: sock.recv(5))

class TestIssue9543(SocketTCPTest, NetworkConnectionTest):
    """
    This test exercises the code in the _fileobject.flush() method when the
    whole write buffer hasn't been flushed.
    See http://bugs.python.org/issue9543
    """
    # XXX: this is just a sanity check, proper tests for flush() should still
    #      be added
    def setUp(self):
        SocketTCPTest.setUp(self)
        NetworkConnectionTest.clientSetUp(self)

    def test_issue9543(self):
        self.cli.close()
        file_a = self.serv.makefile('w')
        file_a.write('x')
        # flush() will try to send data to self.cli and raise an error because
        # it's closed
        self.assertRaises(socket.error, file_a.flush)
        # close() will raise an error too, because it calls flush() before
        # closing the file
        self.assertRaises(socket.error, file_a.close)
        self.assertTrue(file_a.closed)


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
        self.assert_(not s.closed)

        s = MockSocket()
        f = socket._fileobject(s, close=True)
        f.close()
        self.assert_(s.closed)

class TCPTimeoutTest(SocketTCPTest):

    def testTCPTimeout(self):
        def raise_timeout(*args, **kwargs):
            self.serv.settimeout(1.0)
            self.serv.accept()
        self.failUnlessRaises(socket.timeout, raise_timeout,
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
        self.failUnlessRaises(socket.timeout, raise_timeout,
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
        self.assert_(issubclass(socket.error, Exception))
        self.assert_(issubclass(socket.herror, socket.error))
        self.assert_(issubclass(socket.gaierror, socket.error))
        self.assert_(issubclass(socket.timeout, socket.error))

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


class BufferIOTest(SocketConnectedTest):
    """
    Test the buffer versions of socket.recv() and socket.send().
    """
    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def testRecvInto(self):
        buf = array.array('c', ' '*1024)
        nbytes = self.cli_conn.recv_into(buf)
        self.assertEqual(nbytes, len(MSG))
        msg = buf.tostring()[:len(MSG)]
        self.assertEqual(msg, MSG)

    def _testRecvInto(self):
        with test_support._check_py3k_warnings():
            buf = buffer(MSG)
        self.serv_conn.send(buf)

    def testRecvFromInto(self):
        buf = array.array('c', ' '*1024)
        nbytes, addr = self.cli_conn.recvfrom_into(buf)
        self.assertEqual(nbytes, len(MSG))
        msg = buf.tostring()[:len(MSG)]
        self.assertEqual(msg, MSG)

    def _testRecvFromInto(self):
        with test_support._check_py3k_warnings():
            buf = buffer(MSG)
        self.serv_conn.send(buf)


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
    if test_support.verbose:
        print "TIPC module is not loaded, please 'sudo modprobe tipc'"
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
                TIPC_LOWER + (TIPC_UPPER - TIPC_LOWER) / 2, 0)
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


def test_main():
    tests = [GeneralModuleTests, BasicTCPTest, TCPCloserTest, TCPTimeoutTest,
             TestExceptions, BufferIOTest, BasicTCPTest2]
    if sys.platform != 'mac':
        tests.extend([ BasicUDPTest, UDPTimeoutTest ])

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
        TestIssue9543,
    ])
    if hasattr(socket, "socketpair"):
        tests.append(BasicSocketPairTest)
    if sys.platform == 'linux2':
        tests.append(TestLinuxAbstractNamespace)
    if isTipcAvailable():
        tests.append(TIPCTest)
        tests.append(TIPCThreadableTest)

    thread_info = test_support.threading_setup()
    test_support.run_unittest(*tests)
    test_support.threading_cleanup(*thread_info)

if __name__ == "__main__":
    test_main()
