#!/usr/bin/env python

import unittest
import test_support

import socket
import select
import time
import thread, threading
import Queue

PORT = 50007
HOST = 'localhost'
MSG = 'Michael Gilfix was here\n'

class SocketTCPTest(unittest.TestCase):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.serv.bind((HOST, PORT))
        self.serv.listen(1)

    def tearDown(self):
        self.serv.close()
        self.serv = None

class SocketUDPTest(unittest.TestCase):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.serv.bind((HOST, PORT))

    def tearDown(self):
        self.serv.close()
        self.serv = None

class ThreadableTest:

    def __init__(self):
        # Swap the true setup function
        self.__setUp = self.setUp
        self.__tearDown = self.tearDown
        self.setUp = self._setUp
        self.tearDown = self._tearDown

    def _setUp(self):
        self.ready = threading.Event()
        self.done = threading.Event()
        self.queue = Queue.Queue(1)

        # Do some munging to start the client test.
        test_method = getattr(self, ''.join(('_', self._TestCase__testMethodName)))
        self.client_thread = thread.start_new_thread(self.clientRun, (test_method, ))

        self.__setUp()
        self.ready.wait()

    def _tearDown(self):
        self.__tearDown()
        self.done.wait()

        if not self.queue.empty():
            msg = self.queue.get()
            self.fail(msg)

    def clientRun(self, test_func):
        self.ready.set()
        self.clientSetUp()
        if not callable(test_func):
            raise TypeError, "test_func must be a callable function"
        try:
            test_func()
        except Exception, strerror:
            self.queue.put(strerror)
        self.clientTearDown()

    def clientSetUp(self):
        raise NotImplementedError, "clientSetUp must be implemented."

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
        conn, addr = self.serv.accept()
        self.cli_conn = conn

    def tearDown(self):
        self.cli_conn.close()
        self.cli_conn = None
        ThreadedTCPSocketTest.tearDown(self)

    def clientSetUp(self):
        ThreadedTCPSocketTest.clientSetUp(self)
        self.cli.connect((HOST, PORT))
        self.serv_conn = self.cli

    def clientTearDown(self):
        self.serv_conn.close()
        self.serv_conn = None
        ThreadedTCPSocketTest.clientTearDown(self)

#######################################################################
## Begin Tests

class GeneralModuleTests(unittest.TestCase):

    def testSocketError(self):
        """Testing that socket module exceptions."""
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
        """Testing for mission critical constants."""
        socket.AF_INET
        socket.SOCK_STREAM
        socket.SOCK_DGRAM
        socket.SOCK_RAW
        socket.SOCK_RDM
        socket.SOCK_SEQPACKET
        socket.SOL_SOCKET
        socket.SO_REUSEADDR

    def testNonCrucialConstants(self):
        """Testing for existance of non-crucial constants."""
        for const in (
                 "AF_UNIX",

                 "SO_DEBUG", "SO_ACCEPTCONN", "SO_REUSEADDR", "SO_KEEPALIVE",
                 "SO_DONTROUTE", "SO_BROADCAST", "SO_USELOOPBACK", "SO_LINGER",
                 "SO_OOBINLINE", "SO_REUSEPORT", "SO_SNDBUF", "SO_RCVBUF",
                 "SO_SNDLOWAT", "SO_RCVLOWAT", "SO_SNDTIMEO", "SO_RCVTIMEO",
                 "SO_ERROR", "SO_TYPE", "SOMAXCONN",

                 "MSG_OOB", "MSG_PEEK", "MSG_DONTROUTE", "MSG_EOR",
                 "MSG_TRUNC", "MSG_CTRUNC", "MSG_WAITALL", "MSG_BTAG",
                 "MSG_ETAG",

                 "SOL_SOCKET",

                 "IPPROTO_IP", "IPPROTO_ICMP", "IPPROTO_IGMP",
                 "IPPROTO_GGP", "IPPROTO_TCP", "IPPROTO_EGP",
                 "IPPROTO_PUP", "IPPROTO_UDP", "IPPROTO_IDP",
                 "IPPROTO_HELLO", "IPPROTO_ND", "IPPROTO_TP",
                 "IPPROTO_XTP", "IPPROTO_EON", "IPPROTO_BIP",
                 "IPPROTO_RAW", "IPPROTO_MAX",

                 "IPPORT_RESERVED", "IPPORT_USERRESERVED",

                 "INADDR_ANY", "INADDR_BROADCAST", "INADDR_LOOPBACK",
                 "INADDR_UNSPEC_GROUP", "INADDR_ALLHOSTS_GROUP",
                 "INADDR_MAX_LOCAL_GROUP", "INADDR_NONE",

                 "IP_OPTIONS", "IP_HDRINCL", "IP_TOS", "IP_TTL",
                 "IP_RECVOPTS", "IP_RECVRETOPTS", "IP_RECVDSTADDR",
                 "IP_RETOPTS", "IP_MULTICAST_IF", "IP_MULTICAST_TTL",
                 "IP_MULTICAST_LOOP", "IP_ADD_MEMBERSHIP",
                 "IP_DROP_MEMBERSHIP",
                  ):
            try:
                getattr(socket, const)
            except AttributeError:
                pass

    def testHostnameRes(self):
        """Testing hostname resolution mechanisms."""
        hostname = socket.gethostname()
        ip = socket.gethostbyname(hostname)
        self.assert_(ip.find('.') >= 0, "Error resolving host to ip.")
        hname, aliases, ipaddrs = socket.gethostbyaddr(ip)
        all_host_names = [hname] + aliases
        fqhn = socket.getfqdn()
        if not fqhn in all_host_names:
            self.fail("Error testing host resolution mechanisms.")

    def testJavaRef(self):
        """Testing reference count for getnameinfo."""
        import sys
        if not sys.platform.startswith('java'):
            try:
                # On some versions, this loses a reference
                orig = sys.getrefcount(__name__)
                socket.getnameinfo(__name__,0)
            except SystemError:
                if sys.getrefcount(__name__) <> orig:
                    self.fail("socket.getnameinfo loses a reference")

    def testInterpreterCrash(self):
        """Making sure getnameinfo doesn't crash the interpreter."""
        try:
            # On some versions, this crashes the interpreter.
            socket.getnameinfo(('x', 0, 0, 0), 0)
        except socket.error:
            pass

    def testGetServByName(self):
        """Testing getservbyname."""
        if hasattr(socket, 'getservbyname'):
            socket.getservbyname('telnet', 'tcp')
            try:
                socket.getservbyname('telnet', 'udp')
            except socket.error:
                pass

    def testSockName(self):
        """Testing getsockname()."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        name = sock.getsockname()

    def testGetSockOpt(self):
        """Testing getsockopt()."""
        # We know a socket should start without reuse==0
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        reuse = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR)
        self.assert_(reuse == 0, "Error performing getsockopt.")

    def testSetSockOpt(self):
        """Testing setsockopt()."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        reuse = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR)
        self.assert_(reuse == 1, "Error performing setsockopt.")

class BasicTCPTest(SocketConnectedTest):

    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def testRecv(self):
        """Testing large receive over TCP."""
        msg = self.cli_conn.recv(1024)
        self.assertEqual(msg, MSG, "Error performing recv.")

    def _testRecv(self):
        self.serv_conn.send(MSG)

    def testOverFlowRecv(self):
        """Testing receive in chunks over TCP."""
        seg1 = self.cli_conn.recv(len(MSG) - 3)
        seg2 = self.cli_conn.recv(1024)
        msg = ''.join ((seg1, seg2))
        self.assertEqual(msg, MSG, "Error performing recv in chunks.")

    def _testOverFlowRecv(self):
        self.serv_conn.send(MSG)

    def testRecvFrom(self):
        """Testing large recvfrom() over TCP."""
        msg, addr = self.cli_conn.recvfrom(1024)
        hostname, port = addr
        self.assertEqual (hostname, socket.gethostbyname('localhost'),
                          "Wrong address from recvfrom.")
        self.assertEqual(msg, MSG, "Error performing recvfrom.")

    def _testRecvFrom(self):
        self.serv_conn.send(MSG)

    def testOverFlowRecvFrom(self):
        """Testing recvfrom() in chunks over TCP."""
        seg1, addr = self.cli_conn.recvfrom(len(MSG)-3)
        seg2, addr = self.cli_conn.recvfrom(1024)
        msg = ''.join((seg1, seg2))
        hostname, port = addr
        self.assertEqual(hostname, socket.gethostbyname('localhost'),
                         "Wrong address from recvfrom.")
        self.assertEqual(msg, MSG, "Error performing recvfrom in chunks.")

    def _testOverFlowRecvFrom(self):
        self.serv_conn.send(MSG)

    def testSendAll(self):
        """Testing sendall() with a 2048 byte string over TCP."""
        while 1:
            read = self.cli_conn.recv(1024)
            if not read:
                break
            self.assert_(len(read) == 1024, "Error performing sendall.")
            read = filter(lambda x: x == 'f', read)
            self.assert_(len(read) == 1024, "Error performing sendall.")

    def _testSendAll(self):
        big_chunk = ''.join([ 'f' ] * 2048)
        self.serv_conn.sendall(big_chunk)

    def testFromFd(self):
        """Testing fromfd()."""
        fd = self.cli_conn.fileno()
        sock = socket.fromfd(fd, socket.AF_INET, socket.SOCK_STREAM)
        msg = sock.recv(1024)
        self.assertEqual(msg, MSG, "Error creating socket using fromfd.")

    def _testFromFd(self):
        self.serv_conn.send(MSG)

    def testShutdown(self):
        """Testing shutdown()."""
        msg = self.cli_conn.recv(1024)
        self.assertEqual(msg, MSG, "Error testing shutdown.")

    def _testShutdown(self):
        self.serv_conn.send(MSG)
        self.serv_conn.shutdown(2)

class BasicUDPTest(ThreadedUDPSocketTest):

    def __init__(self, methodName='runTest'):
        ThreadedUDPSocketTest.__init__(self, methodName=methodName)

    def testSendtoAndRecv(self):
        """Testing sendto() and Recv() over UDP."""
        msg = self.serv.recv(len(MSG))
        self.assertEqual(msg, MSG, "Error performing sendto")

    def _testSendtoAndRecv(self):
        self.cli.sendto(MSG, 0, (HOST, PORT))

    def testRecvfrom(self):
        """Testing recfrom() over UDP."""
        msg, addr = self.serv.recvfrom(len(MSG))
        hostname, port = addr
        self.assertEqual(hostname, socket.gethostbyname('localhost'),
                         "Wrong address from recvfrom.")
        self.assertEqual(msg, MSG, "Error performing recvfrom in chunks.")

    def _testRecvfrom(self):
        self.cli.sendto(MSG, 0, (HOST, PORT))

class NonBlockingTCPTests(ThreadedTCPSocketTest):

    def __init__(self, methodName='runTest'):
        ThreadedTCPSocketTest.__init__(self, methodName=methodName)

    def testSetBlocking(self):
        """Testing whether set blocking works."""
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
        """Testing non-blocking accept."""
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
        time.sleep(1)
        self.cli.connect((HOST, PORT))

    def testConnect(self):
        """Testing non-blocking connect."""
        time.sleep(1)
        conn, addr = self.serv.accept()

    def _testConnect(self):
        self.cli.setblocking(0)
        try:
            self.cli.connect((HOST, PORT))
        except socket.error:
            pass
        else:
            self.fail("Error trying to do non-blocking connect.")
        read, write, err = select.select([self.cli], [], [])
        if self.cli in read:
            self.cli.connect((HOST, PORT))
        else:
            self.fail("Error trying to do connect after select.")

    def testRecv(self):
        """Testing non-blocking recv."""
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
            self.assertEqual(msg, MSG, "Error performing non-blocking recv.")
        else:
            self.fail("Error during select call to non-blocking socket.")

    def _testRecv(self):
        self.cli.connect((HOST, PORT))
        time.sleep(1)
        self.cli.send(MSG)

class FileObjectClassTestCase(SocketConnectedTest):

    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def setUp(self):
        SocketConnectedTest.setUp(self)
        self.serv_file = socket._fileobject(self.cli_conn, 'rb', 8192)

    def tearDown(self):
        self.serv_file.close()
        self.serv_file = None
        SocketConnectedTest.tearDown(self)

    def clientSetUp(self):
        SocketConnectedTest.clientSetUp(self)
        self.cli_file = socket._fileobject(self.serv_conn, 'rb', 8192)

    def clientTearDown(self):
        self.cli_file.close()
        self.cli_file = None
        SocketConnectedTest.clientTearDown(self)

    def testSmallRead(self):
        """Performing small file read test."""
        first_seg = self.serv_file.read(len(MSG)-3)
        second_seg = self.serv_file.read(3)
        msg = ''.join((first_seg, second_seg))
        self.assertEqual(msg, MSG, "Error performing small read.")

    def _testSmallRead(self):
        self.cli_file.write(MSG)
        self.cli_file.flush()

    def testUnbufferedRead(self):
        """Performing unbuffered file read test."""
        buf = ''
        while 1:
            char = self.serv_file.read(1)
            self.failIf(not char, "Error performing unbuffered read.")
            buf += char
            if buf == MSG:
                break

    def _testUnbufferedRead(self):
        self.cli_file.write(MSG)
        self.cli_file.flush()

    def testReadline(self):
        """Performing file readline test."""
        line = self.serv_file.readline()
        self.assertEqual(line, MSG, "Error performing readline.")

    def _testReadline(self):
        self.cli_file.write(MSG)
        self.cli_file.flush()

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(GeneralModuleTests))
    suite.addTest(unittest.makeSuite(BasicTCPTest))
    suite.addTest(unittest.makeSuite(BasicUDPTest))
    suite.addTest(unittest.makeSuite(NonBlockingTCPTests))
    suite.addTest(unittest.makeSuite(FileObjectClassTestCase))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
