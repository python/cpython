import unittest
from test import support
from test.support import os_helper
from test.support import socket_helper
from test.support import threading_helper

import errno
import io
import itertools
import socket
import select
import tempfile
import time
import traceback
import queue
import sys
import os
import platform
import array
import contextlib
from weakref import proxy
import signal
import math
import pickle
import struct
import random
import shutil
import string
import _thread as thread
import threading
try:
    import multiprocessing
except ImportError:
    multiprocessing = False
try:
    import fcntl
except ImportError:
    fcntl = None

HOST = socket_helper.HOST
# test unicode string and carriage return
MSG = 'Michael Gilfix was here\u1234\r\n'.encode('utf-8')

VSOCKPORT = 1234
AIX = platform.system() == "AIX"

try:
    import _socket
except ImportError:
    _socket = None

def get_cid():
    if fcntl is None:
        return None
    if not hasattr(socket, 'IOCTL_VM_SOCKETS_GET_LOCAL_CID'):
        return None
    try:
        with open("/dev/vsock", "rb") as f:
            r = fcntl.ioctl(f, socket.IOCTL_VM_SOCKETS_GET_LOCAL_CID, "    ")
    except OSError:
        return None
    else:
        return struct.unpack("I", r)[0]

def _have_socket_can():
    """Check whether CAN sockets are supported on this host."""
    try:
        s = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
    except (AttributeError, OSError):
        return False
    else:
        s.close()
    return True

def _have_socket_can_isotp():
    """Check whether CAN ISOTP sockets are supported on this host."""
    try:
        s = socket.socket(socket.PF_CAN, socket.SOCK_DGRAM, socket.CAN_ISOTP)
    except (AttributeError, OSError):
        return False
    else:
        s.close()
    return True

def _have_socket_can_j1939():
    """Check whether CAN J1939 sockets are supported on this host."""
    try:
        s = socket.socket(socket.PF_CAN, socket.SOCK_DGRAM, socket.CAN_J1939)
    except (AttributeError, OSError):
        return False
    else:
        s.close()
    return True

def _have_socket_rds():
    """Check whether RDS sockets are supported on this host."""
    try:
        s = socket.socket(socket.PF_RDS, socket.SOCK_SEQPACKET, 0)
    except (AttributeError, OSError):
        return False
    else:
        s.close()
    return True

def _have_socket_alg():
    """Check whether AF_ALG sockets are supported on this host."""
    try:
        s = socket.socket(socket.AF_ALG, socket.SOCK_SEQPACKET, 0)
    except (AttributeError, OSError):
        return False
    else:
        s.close()
    return True

def _have_socket_qipcrtr():
    """Check whether AF_QIPCRTR sockets are supported on this host."""
    try:
        s = socket.socket(socket.AF_QIPCRTR, socket.SOCK_DGRAM, 0)
    except (AttributeError, OSError):
        return False
    else:
        s.close()
    return True

def _have_socket_vsock():
    """Check whether AF_VSOCK sockets are supported on this host."""
    ret = get_cid() is not None
    return ret


def _have_socket_bluetooth():
    """Check whether AF_BLUETOOTH sockets are supported on this host."""
    try:
        # RFCOMM is supported by all platforms with bluetooth support. Windows
        # does not support omitting the protocol.
        s = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
    except (AttributeError, OSError):
        return False
    else:
        s.close()
    return True


@contextlib.contextmanager
def socket_setdefaulttimeout(timeout):
    old_timeout = socket.getdefaulttimeout()
    try:
        socket.setdefaulttimeout(timeout)
        yield
    finally:
        socket.setdefaulttimeout(old_timeout)


HAVE_SOCKET_CAN = _have_socket_can()

HAVE_SOCKET_CAN_ISOTP = _have_socket_can_isotp()

HAVE_SOCKET_CAN_J1939 = _have_socket_can_j1939()

HAVE_SOCKET_RDS = _have_socket_rds()

HAVE_SOCKET_ALG = _have_socket_alg()

HAVE_SOCKET_QIPCRTR = _have_socket_qipcrtr()

HAVE_SOCKET_VSOCK = _have_socket_vsock()

HAVE_SOCKET_UDPLITE = hasattr(socket, "IPPROTO_UDPLITE")

HAVE_SOCKET_BLUETOOTH = _have_socket_bluetooth()

# Size in bytes of the int type
SIZEOF_INT = array.array("i").itemsize

class SocketTCPTest(unittest.TestCase):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.port = socket_helper.bind_port(self.serv)
        self.serv.listen()

    def tearDown(self):
        self.serv.close()
        self.serv = None

class SocketUDPTest(unittest.TestCase):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.port = socket_helper.bind_port(self.serv)

    def tearDown(self):
        self.serv.close()
        self.serv = None

class SocketUDPLITETest(SocketUDPTest):

    def setUp(self):
        self.serv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDPLITE)
        self.port = socket_helper.bind_port(self.serv)

class ThreadSafeCleanupTestCase(unittest.TestCase):
    """Subclass of unittest.TestCase with thread-safe cleanup methods.

    This subclass protects the addCleanup() and doCleanups() methods
    with a recursive lock.
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._cleanup_lock = threading.RLock()

    def addCleanup(self, *args, **kwargs):
        with self._cleanup_lock:
            return super().addCleanup(*args, **kwargs)

    def doCleanups(self, *args, **kwargs):
        with self._cleanup_lock:
            return super().doCleanups(*args, **kwargs)

class SocketCANTest(unittest.TestCase):

    """To be able to run this test, a `vcan0` CAN interface can be created with
    the following commands:
    # modprobe vcan
    # ip link add dev vcan0 type vcan
    # ifconfig vcan0 up
    """
    interface = 'vcan0'
    bufsize = 128

    """The CAN frame structure is defined in <linux/can.h>:

    struct can_frame {
        canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
        __u8    can_dlc; /* data length code: 0 .. 8 */
        __u8    data[8] __attribute__((aligned(8)));
    };
    """
    can_frame_fmt = "=IB3x8s"
    can_frame_size = struct.calcsize(can_frame_fmt)

    """The Broadcast Management Command frame structure is defined
    in <linux/can/bcm.h>:

    struct bcm_msg_head {
        __u32 opcode;
        __u32 flags;
        __u32 count;
        struct timeval ival1, ival2;
        canid_t can_id;
        __u32 nframes;
        struct can_frame frames[0];
    }

    `bcm_msg_head` must be 8 bytes aligned because of the `frames` member (see
    `struct can_frame` definition). Must use native not standard types for packing.
    """
    bcm_cmd_msg_fmt = "@3I4l2I"
    bcm_cmd_msg_fmt += "x" * (struct.calcsize(bcm_cmd_msg_fmt) % 8)

    def setUp(self):
        self.s = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
        self.addCleanup(self.s.close)
        try:
            self.s.bind((self.interface,))
        except OSError:
            self.skipTest('network interface `%s` does not exist' %
                           self.interface)


class SocketRDSTest(unittest.TestCase):

    """To be able to run this test, the `rds` kernel module must be loaded:
    # modprobe rds
    """
    bufsize = 8192

    def setUp(self):
        self.serv = socket.socket(socket.PF_RDS, socket.SOCK_SEQPACKET, 0)
        self.addCleanup(self.serv.close)
        try:
            self.port = socket_helper.bind_port(self.serv)
        except OSError:
            self.skipTest('unable to bind RDS socket')


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
        self.wait_threads = threading_helper.wait_threads_exit()
        self.wait_threads.__enter__()

        self.server_ready = threading.Event()
        self.client_ready = threading.Event()
        self.done = threading.Event()
        self.queue = queue.Queue(1)
        self.server_crashed = False

        # Do some munging to start the client test.
        methodname = self.id()
        i = methodname.rfind('.')
        methodname = methodname[i+1:]
        test_method = getattr(self, '_' + methodname)
        self.client_thread = thread.start_new_thread(
            self.clientRun, (test_method,))

        try:
            self.__setUp()
        except:
            self.server_crashed = True
            raise
        finally:
            self.server_ready.set()
        self.client_ready.wait()

    def _tearDown(self):
        self.__tearDown()
        self.done.wait()
        self.wait_threads.__exit__(None, None, None)

        if self.queue.qsize():
            exc = self.queue.get()
            raise exc

    def clientRun(self, test_func):
        self.server_ready.wait()
        try:
            self.clientSetUp()
        except BaseException as e:
            self.queue.put(e)
            self.clientTearDown()
            return
        finally:
            self.client_ready.set()
        if self.server_crashed:
            self.clientTearDown()
            return
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

@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
class ThreadedUDPLITESocketTest(SocketUDPLITETest, ThreadableTest):

    def __init__(self, methodName='runTest'):
        SocketUDPLITETest.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        self.cli = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDPLITE)

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)

class ThreadedCANSocketTest(SocketCANTest, ThreadableTest):

    def __init__(self, methodName='runTest'):
        SocketCANTest.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        self.cli = socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
        try:
            self.cli.bind((self.interface,))
        except OSError:
            # skipTest should not be called here, and will be called in the
            # server instead
            pass

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)

class ThreadedRDSSocketTest(SocketRDSTest, ThreadableTest):

    def __init__(self, methodName='runTest'):
        SocketRDSTest.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        self.cli = socket.socket(socket.PF_RDS, socket.SOCK_SEQPACKET, 0)
        try:
            # RDS sockets must be bound explicitly to send or receive data
            self.cli.bind((HOST, 0))
            self.cli_addr = self.cli.getsockname()
        except OSError:
            # skipTest should not be called here, and will be called in the
            # server instead
            pass

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)

@unittest.skipIf(fcntl is None, "need fcntl")
@unittest.skipUnless(HAVE_SOCKET_VSOCK,
          'VSOCK sockets required for this test.')
@unittest.skipUnless(get_cid() != 2,
          "This test can only be run on a virtual guest.")
class ThreadedVSOCKSocketStreamTest(unittest.TestCase, ThreadableTest):

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def setUp(self):
        self.serv = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
        self.addCleanup(self.serv.close)
        self.serv.bind((socket.VMADDR_CID_ANY, VSOCKPORT))
        self.serv.listen()
        self.serverExplicitReady()
        self.conn, self.connaddr = self.serv.accept()
        self.addCleanup(self.conn.close)

    def clientSetUp(self):
        time.sleep(0.1)
        self.cli = socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM)
        self.addCleanup(self.cli.close)
        cid = get_cid()
        self.cli.connect((cid, VSOCKPORT))

    def testStream(self):
        msg = self.conn.recv(1024)
        self.assertEqual(msg, MSG)

    def _testStream(self):
        self.cli.send(MSG)
        self.cli.close()

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


# The following classes are used by the sendmsg()/recvmsg() tests.
# Combining, for instance, ConnectedStreamTestMixin and TCPTestBase
# gives a drop-in replacement for SocketConnectedTest, but different
# address families can be used, and the attributes serv_addr and
# cli_addr will be set to the addresses of the endpoints.

class SocketTestBase(unittest.TestCase):
    """A base class for socket tests.

    Subclasses must provide methods newSocket() to return a new socket
    and bindSock(sock) to bind it to an unused address.

    Creates a socket self.serv and sets self.serv_addr to its address.
    """

    def setUp(self):
        self.serv = self.newSocket()
        self.bindServer()

    def bindServer(self):
        """Bind server socket and set self.serv_addr to its address."""
        self.bindSock(self.serv)
        self.serv_addr = self.serv.getsockname()

    def tearDown(self):
        self.serv.close()
        self.serv = None


class SocketListeningTestMixin(SocketTestBase):
    """Mixin to listen on the server socket."""

    def setUp(self):
        super().setUp()
        self.serv.listen()


class ThreadedSocketTestMixin(ThreadSafeCleanupTestCase, SocketTestBase,
                              ThreadableTest):
    """Mixin to add client socket and allow client/server tests.

    Client socket is self.cli and its address is self.cli_addr.  See
    ThreadableTest for usage information.
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        self.cli = self.newClientSocket()
        self.bindClient()

    def newClientSocket(self):
        """Return a new socket for use as client."""
        return self.newSocket()

    def bindClient(self):
        """Bind client socket and set self.cli_addr to its address."""
        self.bindSock(self.cli)
        self.cli_addr = self.cli.getsockname()

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)


class ConnectedStreamTestMixin(SocketListeningTestMixin,
                               ThreadedSocketTestMixin):
    """Mixin to allow client/server stream tests with connected client.

    Server's socket representing connection to client is self.cli_conn
    and client's connection to server is self.serv_conn.  (Based on
    SocketConnectedTest.)
    """

    def setUp(self):
        super().setUp()
        # Indicate explicitly we're ready for the client thread to
        # proceed and then perform the blocking call to accept
        self.serverExplicitReady()
        conn, addr = self.serv.accept()
        self.cli_conn = conn

    def tearDown(self):
        self.cli_conn.close()
        self.cli_conn = None
        super().tearDown()

    def clientSetUp(self):
        super().clientSetUp()
        self.cli.connect(self.serv_addr)
        self.serv_conn = self.cli

    def clientTearDown(self):
        try:
            self.serv_conn.close()
            self.serv_conn = None
        except AttributeError:
            pass
        super().clientTearDown()


class UnixSocketTestBase(SocketTestBase):
    """Base class for Unix-domain socket tests."""

    # This class is used for file descriptor passing tests, so we
    # create the sockets in a private directory so that other users
    # can't send anything that might be problematic for a privileged
    # user running the tests.

    def setUp(self):
        self.dir_path = tempfile.mkdtemp()
        self.addCleanup(os.rmdir, self.dir_path)
        super().setUp()

    def bindSock(self, sock):
        path = tempfile.mktemp(dir=self.dir_path)
        socket_helper.bind_unix_socket(sock, path)
        self.addCleanup(os_helper.unlink, path)

class UnixStreamBase(UnixSocketTestBase):
    """Base class for Unix-domain SOCK_STREAM tests."""

    def newSocket(self):
        return socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)


class InetTestBase(SocketTestBase):
    """Base class for IPv4 socket tests."""

    host = HOST

    def setUp(self):
        super().setUp()
        self.port = self.serv_addr[1]

    def bindSock(self, sock):
        socket_helper.bind_port(sock, host=self.host)

class TCPTestBase(InetTestBase):
    """Base class for TCP-over-IPv4 tests."""

    def newSocket(self):
        return socket.socket(socket.AF_INET, socket.SOCK_STREAM)

class UDPTestBase(InetTestBase):
    """Base class for UDP-over-IPv4 tests."""

    def newSocket(self):
        return socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

class UDPLITETestBase(InetTestBase):
    """Base class for UDPLITE-over-IPv4 tests."""

    def newSocket(self):
        return socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDPLITE)

class SCTPStreamBase(InetTestBase):
    """Base class for SCTP tests in one-to-one (SOCK_STREAM) mode."""

    def newSocket(self):
        return socket.socket(socket.AF_INET, socket.SOCK_STREAM,
                             socket.IPPROTO_SCTP)


class Inet6TestBase(InetTestBase):
    """Base class for IPv6 socket tests."""

    host = socket_helper.HOSTv6

class UDP6TestBase(Inet6TestBase):
    """Base class for UDP-over-IPv6 tests."""

    def newSocket(self):
        return socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)

class UDPLITE6TestBase(Inet6TestBase):
    """Base class for UDPLITE-over-IPv6 tests."""

    def newSocket(self):
        return socket.socket(socket.AF_INET6, socket.SOCK_DGRAM, socket.IPPROTO_UDPLITE)


# Test-skipping decorators for use with ThreadableTest.

def skipWithClientIf(condition, reason):
    """Skip decorated test if condition is true, add client_skip decorator.

    If the decorated object is not a class, sets its attribute
    "client_skip" to a decorator which will return an empty function
    if the test is to be skipped, or the original function if it is
    not.  This can be used to avoid running the client part of a
    skipped test when using ThreadableTest.
    """
    def client_pass(*args, **kwargs):
        pass
    def skipdec(obj):
        retval = unittest.skip(reason)(obj)
        if not isinstance(obj, type):
            retval.client_skip = lambda f: client_pass
        return retval
    def noskipdec(obj):
        if not (isinstance(obj, type) or hasattr(obj, "client_skip")):
            obj.client_skip = lambda f: f
        return obj
    return skipdec if condition else noskipdec


def requireAttrs(obj, *attributes):
    """Skip decorated test if obj is missing any of the given attributes.

    Sets client_skip attribute as skipWithClientIf() does.
    """
    missing = [name for name in attributes if not hasattr(obj, name)]
    return skipWithClientIf(
        missing, "don't have " + ", ".join(name for name in missing))


def requireSocket(*args):
    """Skip decorated test if a socket cannot be created with given arguments.

    When an argument is given as a string, will use the value of that
    attribute of the socket module, or skip the test if it doesn't
    exist.  Sets client_skip attribute as skipWithClientIf() does.
    """
    err = None
    missing = [obj for obj in args if
               isinstance(obj, str) and not hasattr(socket, obj)]
    if missing:
        err = "don't have " + ", ".join(name for name in missing)
    else:
        callargs = [getattr(socket, obj) if isinstance(obj, str) else obj
                    for obj in args]
        try:
            s = socket.socket(*callargs)
        except OSError as e:
            # XXX: check errno?
            err = str(e)
        else:
            s.close()
    return skipWithClientIf(
        err is not None,
        "can't create socket({0}): {1}".format(
            ", ".join(str(o) for o in args), err))


#######################################################################
## Begin Tests

class GeneralModuleTests(unittest.TestCase):

    def test_SocketType_is_socketobject(self):
        import _socket
        self.assertTrue(socket.SocketType is _socket.socket)
        s = socket.socket()
        self.assertIsInstance(s, socket.SocketType)
        s.close()

    def test_repr(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        with s:
            self.assertIn('fd=%i' % s.fileno(), repr(s))
            self.assertIn('family=%s' % socket.AF_INET, repr(s))
            self.assertIn('type=%s' % socket.SOCK_STREAM, repr(s))
            self.assertIn('proto=0', repr(s))
            self.assertNotIn('raddr', repr(s))
            s.bind(('127.0.0.1', 0))
            self.assertIn('laddr', repr(s))
            self.assertIn(str(s.getsockname()), repr(s))
        self.assertIn('[closed]', repr(s))
        self.assertNotIn('laddr', repr(s))

    @unittest.skipUnless(_socket is not None, 'need _socket module')
    def test_csocket_repr(self):
        s = _socket.socket(_socket.AF_INET, _socket.SOCK_STREAM)
        try:
            expected = ('<socket object, fd=%s, family=%s, type=%s, proto=%s>'
                        % (s.fileno(), s.family, s.type, s.proto))
            self.assertEqual(repr(s), expected)
        finally:
            s.close()
        expected = ('<socket object, fd=-1, family=%s, type=%s, proto=%s>'
                    % (s.family, s.type, s.proto))
        self.assertEqual(repr(s), expected)

    def test_weakref(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            p = proxy(s)
            self.assertEqual(p.fileno(), s.fileno())
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
        with self.assertRaises(OSError, msg=msg % 'OSError'):
            raise OSError
        with self.assertRaises(OSError, msg=msg % 'socket.herror'):
            raise socket.herror
        with self.assertRaises(OSError, msg=msg % 'socket.gaierror'):
            raise socket.gaierror

    def testSendtoErrors(self):
        # Testing that sendto doesn't mask failures. See #10169.
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.addCleanup(s.close)
        s.bind(('', 0))
        sockname = s.getsockname()
        # 2 args
        with self.assertRaises(TypeError) as cm:
            s.sendto('\u2620', sockname)
        self.assertEqual(str(cm.exception),
                         "a bytes-like object is required, not 'str'")
        with self.assertRaises(TypeError) as cm:
            s.sendto(5j, sockname)
        self.assertEqual(str(cm.exception),
                         "a bytes-like object is required, not 'complex'")
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo', None)
        self.assertIn('not NoneType',str(cm.exception))
        # 3 args
        with self.assertRaises(TypeError) as cm:
            s.sendto('\u2620', 0, sockname)
        self.assertEqual(str(cm.exception),
                         "a bytes-like object is required, not 'str'")
        with self.assertRaises(TypeError) as cm:
            s.sendto(5j, 0, sockname)
        self.assertEqual(str(cm.exception),
                         "a bytes-like object is required, not 'complex'")
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo', 0, None)
        self.assertIn('not NoneType', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo', 'bar', sockname)
        with self.assertRaises(TypeError) as cm:
            s.sendto(b'foo', None, None)
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
        if socket.has_ipv6:
            socket.AF_INET6
        socket.SOCK_STREAM
        socket.SOCK_DGRAM
        socket.SOCK_RAW
        socket.SOCK_RDM
        socket.SOCK_SEQPACKET
        socket.SOL_SOCKET
        socket.SO_REUSEADDR

    def testCrucialIpProtoConstants(self):
        socket.IPPROTO_TCP
        socket.IPPROTO_UDP
        if socket.has_ipv6:
            socket.IPPROTO_IPV6

    @unittest.skipUnless(os.name == "nt", "Windows specific")
    def testWindowsSpecificConstants(self):
        socket.IPPROTO_ICLFXBM
        socket.IPPROTO_ST
        socket.IPPROTO_CBT
        socket.IPPROTO_IGP
        socket.IPPROTO_RDP
        socket.IPPROTO_PGM
        socket.IPPROTO_L2TP
        socket.IPPROTO_SCTP

    @unittest.skipUnless(sys.platform == 'darwin', 'macOS specific test')
    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test')
    def test3542SocketOptions(self):
        # Ref. issue #35569 and https://tools.ietf.org/html/rfc3542
        opts = {
            'IPV6_CHECKSUM',
            'IPV6_DONTFRAG',
            'IPV6_DSTOPTS',
            'IPV6_HOPLIMIT',
            'IPV6_HOPOPTS',
            'IPV6_NEXTHOP',
            'IPV6_PATHMTU',
            'IPV6_PKTINFO',
            'IPV6_RECVDSTOPTS',
            'IPV6_RECVHOPLIMIT',
            'IPV6_RECVHOPOPTS',
            'IPV6_RECVPATHMTU',
            'IPV6_RECVPKTINFO',
            'IPV6_RECVRTHDR',
            'IPV6_RECVTCLASS',
            'IPV6_RTHDR',
            'IPV6_RTHDRDSTOPTS',
            'IPV6_RTHDR_TYPE_0',
            'IPV6_TCLASS',
            'IPV6_USE_MIN_MTU',
        }
        for opt in opts:
            self.assertTrue(
                hasattr(socket, opt), f"Missing RFC3542 socket option '{opt}'"
            )

    def testHostnameRes(self):
        # Testing hostname resolution mechanisms
        hostname = socket.gethostname()
        try:
            ip = socket.gethostbyname(hostname)
        except OSError:
            # Probably name lookup wasn't set up right; skip this test
            self.skipTest('name lookup failure')
        self.assertTrue(ip.find('.') >= 0, "Error resolving host to ip.")
        try:
            hname, aliases, ipaddrs = socket.gethostbyaddr(ip)
        except OSError:
            # Probably a similar problem as above; skip this test
            self.skipTest('name lookup failure')
        all_host_names = [hostname, hname] + aliases
        fqhn = socket.getfqdn(ip)
        if not fqhn in all_host_names:
            self.fail("Error testing host resolution mechanisms. (fqdn: %s, all: %s)" % (fqhn, repr(all_host_names)))

    def test_host_resolution(self):
        for addr in [socket_helper.HOSTv4, '10.0.0.1', '255.255.255.255']:
            self.assertEqual(socket.gethostbyname(addr), addr)

        # we don't test socket_helper.HOSTv6 because there's a chance it doesn't have
        # a matching name entry (e.g. 'ip6-localhost')
        for host in [socket_helper.HOSTv4]:
            self.assertIn(host, socket.gethostbyaddr(host)[2])

    def test_host_resolution_bad_address(self):
        # These are all malformed IP addresses and expected not to resolve to
        # any result.  But some ISPs, e.g. AWS, may successfully resolve these
        # IPs.
        explanation = (
            "resolving an invalid IP address did not raise OSError; "
            "can be caused by a broken DNS server"
        )
        for addr in ['0.1.1.~1', '1+.1.1.1', '::1q', '::1::2',
                     '1:1:1:1:1:1:1:1:1']:
            with self.assertRaises(OSError, msg=addr):
                socket.gethostbyname(addr)
            with self.assertRaises(OSError, msg=explanation):
                socket.gethostbyaddr(addr)

    @unittest.skipUnless(hasattr(socket, 'sethostname'), "test needs socket.sethostname()")
    @unittest.skipUnless(hasattr(socket, 'gethostname'), "test needs socket.gethostname()")
    def test_sethostname(self):
        oldhn = socket.gethostname()
        try:
            socket.sethostname('new')
        except OSError as e:
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

    @unittest.skipUnless(hasattr(socket, 'if_nameindex'),
                         'socket.if_nameindex() not available.')
    def testInterfaceNameIndex(self):
        interfaces = socket.if_nameindex()
        for index, name in interfaces:
            self.assertIsInstance(index, int)
            self.assertIsInstance(name, str)
            # interface indices are non-zero integers
            self.assertGreater(index, 0)
            _index = socket.if_nametoindex(name)
            self.assertIsInstance(_index, int)
            self.assertEqual(index, _index)
            _name = socket.if_indextoname(index)
            self.assertIsInstance(_name, str)
            self.assertEqual(name, _name)

    @unittest.skipUnless(hasattr(socket, 'if_indextoname'),
                         'socket.if_indextoname() not available.')
    def testInvalidInterfaceIndexToName(self):
        self.assertRaises(OSError, socket.if_indextoname, 0)
        self.assertRaises(TypeError, socket.if_indextoname, '_DEADBEEF')

    @unittest.skipUnless(hasattr(socket, 'if_nametoindex'),
                         'socket.if_nametoindex() not available.')
    def testInvalidInterfaceNameToIndex(self):
        self.assertRaises(TypeError, socket.if_nametoindex, 0)
        self.assertRaises(OSError, socket.if_nametoindex, '_DEADBEEF')

    @unittest.skipUnless(hasattr(sys, 'getrefcount'),
                         'test needs sys.getrefcount()')
    def testRefCountGetNameInfo(self):
        # Testing reference count for getnameinfo
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
        except OSError:
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

    @support.cpython_only
    def testNtoHErrors(self):
        import _testcapi
        s_good_values = [0, 1, 2, 0xffff]
        l_good_values = s_good_values + [0xffffffff]
        l_bad_values = [-1, -2, 1<<32, 1<<1000]
        s_bad_values = (
            l_bad_values +
            [_testcapi.INT_MIN-1, _testcapi.INT_MAX+1] +
            [1 << 16, _testcapi.INT_MAX]
        )
        for k in s_good_values:
            socket.ntohs(k)
            socket.htons(k)
        for k in l_good_values:
            socket.ntohl(k)
            socket.htonl(k)
        for k in s_bad_values:
            self.assertRaises(OverflowError, socket.ntohs, k)
            self.assertRaises(OverflowError, socket.htons, k)
        for k in l_bad_values:
            self.assertRaises(OverflowError, socket.ntohl, k)
            self.assertRaises(OverflowError, socket.htonl, k)

    def testGetServBy(self):
        eq = self.assertEqual
        # Find one service that exists, then check all the related interfaces.
        # I've ordered this by protocols that have both a tcp and udp
        # protocol, at least for modern Linuxes.
        if (sys.platform.startswith(('freebsd', 'netbsd', 'gnukfreebsd'))
            or sys.platform in ('linux', 'darwin')):
            # avoid the 'echo' service on this platform, as there is an
            # assumption breaking non-standard port/protocol entry
            services = ('daytime', 'qotd', 'domain')
        else:
            services = ('echo', 'daytime', 'domain')
        for service in services:
            try:
                port = socket.getservbyname(service, 'tcp')
                break
            except OSError:
                pass
        else:
            raise OSError
        # Try same call with optional protocol omitted
        # Issue #26936: Android getservbyname() was broken before API 23.
        if (not hasattr(sys, 'getandroidapilevel') or
                sys.getandroidapilevel() >= 23):
            port2 = socket.getservbyname(service)
            eq(port, port2)
        # Try udp, but don't barf if it doesn't exist
        try:
            udpport = socket.getservbyname(service, 'udp')
        except OSError:
            udpport = None
        else:
            eq(udpport, port)
        # Now make sure the lookup by port returns the same service name
        # Issue #26936: Android getservbyport() is broken.
        if not support.is_android:
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
        with socket.socket() as s:
            self.assertEqual(s.gettimeout(), None)

        # Set the default timeout to 10, and see if it propagates
        with socket_setdefaulttimeout(10):
            self.assertEqual(socket.getdefaulttimeout(), 10)
            with socket.socket() as sock:
                self.assertEqual(sock.gettimeout(), 10)

            # Reset the default timeout to None, and see if it propagates
            socket.setdefaulttimeout(None)
            self.assertEqual(socket.getdefaulttimeout(), None)
            with socket.socket() as sock:
                self.assertEqual(sock.gettimeout(), None)

        # Check that setting it to an invalid value raises ValueError
        self.assertRaises(ValueError, socket.setdefaulttimeout, -1)

        # Check that setting it to an invalid type raises TypeError
        self.assertRaises(TypeError, socket.setdefaulttimeout, "spam")

    @unittest.skipUnless(hasattr(socket, 'inet_aton'),
                         'test needs socket.inet_aton()')
    def testIPv4_inet_aton_fourbytes(self):
        # Test that issue1008086 and issue767150 are fixed.
        # It must return 4 bytes.
        self.assertEqual(b'\x00'*4, socket.inet_aton('0.0.0.0'))
        self.assertEqual(b'\xff'*4, socket.inet_aton('255.255.255.255'))

    @unittest.skipUnless(hasattr(socket, 'inet_pton'),
                         'test needs socket.inet_pton()')
    def testIPv4toString(self):
        from socket import inet_aton as f, inet_pton, AF_INET
        g = lambda a: inet_pton(AF_INET, a)

        assertInvalid = lambda func,a: self.assertRaises(
            (OSError, ValueError), func, a
        )

        self.assertEqual(b'\x00\x00\x00\x00', f('0.0.0.0'))
        self.assertEqual(b'\xff\x00\xff\x00', f('255.0.255.0'))
        self.assertEqual(b'\xaa\xaa\xaa\xaa', f('170.170.170.170'))
        self.assertEqual(b'\x01\x02\x03\x04', f('1.2.3.4'))
        self.assertEqual(b'\xff\xff\xff\xff', f('255.255.255.255'))
        # bpo-29972: inet_pton() doesn't fail on AIX
        if not AIX:
            assertInvalid(f, '0.0.0.')
        assertInvalid(f, '300.0.0.0')
        assertInvalid(f, 'a.0.0.0')
        assertInvalid(f, '1.2.3.4.5')
        assertInvalid(f, '::1')

        self.assertEqual(b'\x00\x00\x00\x00', g('0.0.0.0'))
        self.assertEqual(b'\xff\x00\xff\x00', g('255.0.255.0'))
        self.assertEqual(b'\xaa\xaa\xaa\xaa', g('170.170.170.170'))
        self.assertEqual(b'\xff\xff\xff\xff', g('255.255.255.255'))
        assertInvalid(g, '0.0.0.')
        assertInvalid(g, '300.0.0.0')
        assertInvalid(g, 'a.0.0.0')
        assertInvalid(g, '1.2.3.4.5')
        assertInvalid(g, '::1')

    @unittest.skipUnless(hasattr(socket, 'inet_pton'),
                         'test needs socket.inet_pton()')
    def testIPv6toString(self):
        try:
            from socket import inet_pton, AF_INET6, has_ipv6
            if not has_ipv6:
                self.skipTest('IPv6 not available')
        except ImportError:
            self.skipTest('could not import needed symbols from socket')

        if sys.platform == "win32":
            try:
                inet_pton(AF_INET6, '::')
            except OSError as e:
                if e.winerror == 10022:
                    self.skipTest('IPv6 might not be supported')

        f = lambda a: inet_pton(AF_INET6, a)
        assertInvalid = lambda a: self.assertRaises(
            (OSError, ValueError), f, a
        )

        self.assertEqual(b'\x00' * 16, f('::'))
        self.assertEqual(b'\x00' * 16, f('0::0'))
        self.assertEqual(b'\x00\x01' + b'\x00' * 14, f('1::'))
        self.assertEqual(
            b'\x45\xef\x76\xcb\x00\x1a\x56\xef\xaf\xeb\x0b\xac\x19\x24\xae\xae',
            f('45ef:76cb:1a:56ef:afeb:bac:1924:aeae')
        )
        self.assertEqual(
            b'\xad\x42\x0a\xbc' + b'\x00' * 4 + b'\x01\x27\x00\x00\x02\x54\x00\x02',
            f('ad42:abc::127:0:254:2')
        )
        self.assertEqual(b'\x00\x12\x00\x0a' + b'\x00' * 12, f('12:a::'))
        assertInvalid('0x20::')
        assertInvalid(':::')
        assertInvalid('::0::')
        assertInvalid('1::abc::')
        assertInvalid('1::abc::def')
        assertInvalid('1:2:3:4:5:6')
        assertInvalid('1:2:3:4:5:6:')
        assertInvalid('1:2:3:4:5:6:7:8:0')
        # bpo-29972: inet_pton() doesn't fail on AIX
        if not AIX:
            assertInvalid('1:2:3:4:5:6:7:8:')

        self.assertEqual(b'\x00' * 12 + b'\xfe\x2a\x17\x40',
            f('::254.42.23.64')
        )
        self.assertEqual(
            b'\x00\x42' + b'\x00' * 8 + b'\xa2\x9b\xfe\x2a\x17\x40',
            f('42::a29b:254.42.23.64')
        )
        self.assertEqual(
            b'\x00\x42\xa8\xb9\x00\x00\x00\x02\xff\xff\xa2\x9b\xfe\x2a\x17\x40',
            f('42:a8b9:0:2:ffff:a29b:254.42.23.64')
        )
        assertInvalid('255.254.253.252')
        assertInvalid('1::260.2.3.0')
        assertInvalid('1::0.be.e.0')
        assertInvalid('1:2:3:4:5:6:7:1.2.3.4')
        assertInvalid('::1.2.3.4:0')
        assertInvalid('0.100.200.0:3:4:5:6:7:8')

    @unittest.skipUnless(hasattr(socket, 'inet_ntop'),
                         'test needs socket.inet_ntop()')
    def testStringToIPv4(self):
        from socket import inet_ntoa as f, inet_ntop, AF_INET
        g = lambda a: inet_ntop(AF_INET, a)
        assertInvalid = lambda func,a: self.assertRaises(
            (OSError, ValueError), func, a
        )

        self.assertEqual('1.0.1.0', f(b'\x01\x00\x01\x00'))
        self.assertEqual('170.85.170.85', f(b'\xaa\x55\xaa\x55'))
        self.assertEqual('255.255.255.255', f(b'\xff\xff\xff\xff'))
        self.assertEqual('1.2.3.4', f(b'\x01\x02\x03\x04'))
        assertInvalid(f, b'\x00' * 3)
        assertInvalid(f, b'\x00' * 5)
        assertInvalid(f, b'\x00' * 16)
        self.assertEqual('170.85.170.85', f(bytearray(b'\xaa\x55\xaa\x55')))

        self.assertEqual('1.0.1.0', g(b'\x01\x00\x01\x00'))
        self.assertEqual('170.85.170.85', g(b'\xaa\x55\xaa\x55'))
        self.assertEqual('255.255.255.255', g(b'\xff\xff\xff\xff'))
        assertInvalid(g, b'\x00' * 3)
        assertInvalid(g, b'\x00' * 5)
        assertInvalid(g, b'\x00' * 16)
        self.assertEqual('170.85.170.85', g(bytearray(b'\xaa\x55\xaa\x55')))

    @unittest.skipUnless(hasattr(socket, 'inet_ntop'),
                         'test needs socket.inet_ntop()')
    def testStringToIPv6(self):
        try:
            from socket import inet_ntop, AF_INET6, has_ipv6
            if not has_ipv6:
                self.skipTest('IPv6 not available')
        except ImportError:
            self.skipTest('could not import needed symbols from socket')

        if sys.platform == "win32":
            try:
                inet_ntop(AF_INET6, b'\x00' * 16)
            except OSError as e:
                if e.winerror == 10022:
                    self.skipTest('IPv6 might not be supported')

        f = lambda a: inet_ntop(AF_INET6, a)
        assertInvalid = lambda a: self.assertRaises(
            (OSError, ValueError), f, a
        )

        self.assertEqual('::', f(b'\x00' * 16))
        self.assertEqual('::1', f(b'\x00' * 15 + b'\x01'))
        self.assertEqual(
            'aef:b01:506:1001:ffff:9997:55:170',
            f(b'\x0a\xef\x0b\x01\x05\x06\x10\x01\xff\xff\x99\x97\x00\x55\x01\x70')
        )
        self.assertEqual('::1', f(bytearray(b'\x00' * 15 + b'\x01')))

        assertInvalid(b'\x12' * 15)
        assertInvalid(b'\x12' * 17)
        assertInvalid(b'\x12' * 4)

    # XXX The following don't test module-level functionality...

    def testSockName(self):
        # Testing getsockname()
        port = socket_helper.find_unused_port()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(sock.close)
        sock.bind(("0.0.0.0", port))
        name = sock.getsockname()
        # XXX(nnorwitz): http://tinyurl.com/os5jz seems to indicate
        # it reasonable to get the host's addr in addition to 0.0.0.0.
        # At least for eCos.  This is required for the S/390 to pass.
        try:
            my_ip_addr = socket.gethostbyname(socket.gethostname())
        except OSError:
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
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(1)
        self.assertRaises(OSError, sock.send, b"spam")

    def testCloseException(self):
        sock = socket.socket()
        sock.bind((socket._LOCALHOST, 0))
        socket.socket(fileno=sock.fileno()).close()
        try:
            sock.close()
        except OSError as err:
            # Winsock apparently raises ENOTSOCK
            self.assertIn(err.errno, (errno.EBADF, errno.ENOTSOCK))
        else:
            self.fail("close() should raise EBADF/ENOTSOCK")

    def testNewAttributes(self):
        # testing .family, .type and .protocol

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            self.assertEqual(sock.family, socket.AF_INET)
            if hasattr(socket, 'SOCK_CLOEXEC'):
                self.assertIn(sock.type,
                              (socket.SOCK_STREAM | socket.SOCK_CLOEXEC,
                               socket.SOCK_STREAM))
            else:
                self.assertEqual(sock.type, socket.SOCK_STREAM)
            self.assertEqual(sock.proto, 0)

    def test_getsockaddrarg(self):
        sock = socket.socket()
        self.addCleanup(sock.close)
        port = socket_helper.find_unused_port()
        big_port = port + 65536
        neg_port = port - 65536
        self.assertRaises(OverflowError, sock.bind, (HOST, big_port))
        self.assertRaises(OverflowError, sock.bind, (HOST, neg_port))
        # Since find_unused_port() is inherently subject to race conditions, we
        # call it a couple times if necessary.
        for i in itertools.count():
            port = socket_helper.find_unused_port()
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

    @unittest.skipUnless(os.name == "nt", "Windows specific")
    @unittest.skipUnless(hasattr(socket, 'SIO_LOOPBACK_FAST_PATH'),
                         'Loopback fast path support required for this test')
    def test_sio_loopback_fast_path(self):
        s = socket.socket()
        self.addCleanup(s.close)
        try:
            s.ioctl(socket.SIO_LOOPBACK_FAST_PATH, True)
        except OSError as exc:
            WSAEOPNOTSUPP = 10045
            if exc.winerror == WSAEOPNOTSUPP:
                self.skipTest("SIO_LOOPBACK_FAST_PATH is defined but "
                              "doesn't implemented in this Windows version")
            raise
        self.assertRaises(TypeError, s.ioctl, socket.SIO_LOOPBACK_FAST_PATH, None)

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
        if socket_helper.IPV6_ENABLED:
            socket.getaddrinfo('::1', 80)
        # port can be a string service name such as "http", a numeric
        # port number or None
        # Issue #26936: Android getaddrinfo() was broken before API level 23.
        if (not hasattr(sys, 'getandroidapilevel') or
                sys.getandroidapilevel() >= 23):
            socket.getaddrinfo(HOST, "http")
        socket.getaddrinfo(HOST, 80)
        socket.getaddrinfo(HOST, None)
        # test family and socktype filters
        infos = socket.getaddrinfo(HOST, 80, socket.AF_INET, socket.SOCK_STREAM)
        for family, type, _, _, _ in infos:
            self.assertEqual(family, socket.AF_INET)
            self.assertEqual(str(family), 'AF_INET')
            self.assertEqual(type, socket.SOCK_STREAM)
            self.assertEqual(str(type), 'SOCK_STREAM')
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

        # Issue 17269: test workaround for OS X platform bug segfault
        if hasattr(socket, 'AI_NUMERICSERV'):
            try:
                # The arguments here are undefined and the call may succeed
                # or fail.  All we care here is that it doesn't segfault.
                socket.getaddrinfo("localhost", None, 0, 0, 0,
                                   socket.AI_NUMERICSERV)
            except socket.gaierror:
                pass

    def test_getnameinfo(self):
        # only IP addresses are allowed
        self.assertRaises(OSError, socket.getnameinfo, ('mail.python.org',0), 0)

    @unittest.skipUnless(support.is_resource_enabled('network'),
                         'network is not enabled')
    def test_idna(self):
        # Check for internet access before running test
        # (issue #12804, issue #25138).
        with socket_helper.transient_internet('python.org'):
            socket.gethostbyname('python.org')

        # these should all be successful
        domain = 'испытание.pythontest.net'
        socket.gethostbyname(domain)
        socket.gethostbyname_ex(domain)
        socket.getaddrinfo(domain,0,socket.AF_UNSPEC,socket.SOCK_STREAM)
        # this may not work if the forward lookup chooses the IPv6 address, as that doesn't
        # have a reverse entry yet
        # socket.gethostbyaddr('испытание.python.org')

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
                c.sendall(b"x" * support.SOCK_MAX_SIZE)
            if with_timeout:
                signal.signal(signal.SIGALRM, ok_handler)
                signal.alarm(1)
                self.assertRaises(TimeoutError, c.sendall,
                                  b"x" * support.SOCK_MAX_SIZE)
        finally:
            signal.alarm(0)
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

    def test_unusable_closed_socketio(self):
        with socket.socket() as sock:
            fp = sock.makefile("rb", buffering=0)
            self.assertTrue(fp.readable())
            self.assertFalse(fp.writable())
            self.assertFalse(fp.seekable())
            fp.close()
            self.assertRaises(ValueError, fp.readable)
            self.assertRaises(ValueError, fp.writable)
            self.assertRaises(ValueError, fp.seekable)

    def test_socket_close(self):
        sock = socket.socket()
        try:
            sock.bind((HOST, 0))
            socket.close(sock.fileno())
            with self.assertRaises(OSError):
                sock.listen(1)
        finally:
            with self.assertRaises(OSError):
                # sock.close() fails with EBADF
                sock.close()
        with self.assertRaises(TypeError):
            socket.close(None)
        with self.assertRaises(OSError):
            socket.close(-1)

    def test_makefile_mode(self):
        for mode in 'r', 'rb', 'rw', 'w', 'wb':
            with self.subTest(mode=mode):
                with socket.socket() as sock:
                    with sock.makefile(mode) as fp:
                        self.assertEqual(fp.mode, mode)

    def test_makefile_invalid_mode(self):
        for mode in 'rt', 'x', '+', 'a':
            with self.subTest(mode=mode):
                with socket.socket() as sock:
                    with self.assertRaisesRegex(ValueError, 'invalid mode'):
                        sock.makefile(mode)

    def test_pickle(self):
        sock = socket.socket()
        with sock:
            for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
                self.assertRaises(TypeError, pickle.dumps, sock, protocol)
        for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
            family = pickle.loads(pickle.dumps(socket.AF_INET, protocol))
            self.assertEqual(family, socket.AF_INET)
            type = pickle.loads(pickle.dumps(socket.SOCK_STREAM, protocol))
            self.assertEqual(type, socket.SOCK_STREAM)

    def test_listen_backlog(self):
        for backlog in 0, -1:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
                srv.bind((HOST, 0))
                srv.listen(backlog)

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
            srv.bind((HOST, 0))
            srv.listen()

    @support.cpython_only
    def test_listen_backlog_overflow(self):
        # Issue 15989
        import _testcapi
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
            srv.bind((HOST, 0))
            self.assertRaises(OverflowError, srv.listen, _testcapi.INT_MAX + 1)

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
    def test_flowinfo(self):
        self.assertRaises(OverflowError, socket.getnameinfo,
                          (socket_helper.HOSTv6, 0, 0xffffffff), 0)
        with socket.socket(socket.AF_INET6, socket.SOCK_STREAM) as s:
            self.assertRaises(OverflowError, s.bind, (socket_helper.HOSTv6, 0, -10))

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
    def test_getaddrinfo_ipv6_basic(self):
        ((*_, sockaddr),) = socket.getaddrinfo(
            'ff02::1de:c0:face:8D',  # Note capital letter `D`.
            1234, socket.AF_INET6,
            socket.SOCK_DGRAM,
            socket.IPPROTO_UDP
        )
        self.assertEqual(sockaddr, ('ff02::1de:c0:face:8d', 1234, 0, 0))

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
    @unittest.skipIf(sys.platform == 'win32', 'does not work on Windows')
    @unittest.skipIf(AIX, 'Symbolic scope id does not work')
    @unittest.skipUnless(hasattr(socket, 'if_nameindex'), "test needs socket.if_nameindex()")
    def test_getaddrinfo_ipv6_scopeid_symbolic(self):
        # Just pick up any network interface (Linux, Mac OS X)
        (ifindex, test_interface) = socket.if_nameindex()[0]
        ((*_, sockaddr),) = socket.getaddrinfo(
            'ff02::1de:c0:face:8D%' + test_interface,
            1234, socket.AF_INET6,
            socket.SOCK_DGRAM,
            socket.IPPROTO_UDP
        )
        # Note missing interface name part in IPv6 address
        self.assertEqual(sockaddr, ('ff02::1de:c0:face:8d', 1234, 0, ifindex))

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
    @unittest.skipUnless(
        sys.platform == 'win32',
        'Numeric scope id does not work or undocumented')
    def test_getaddrinfo_ipv6_scopeid_numeric(self):
        # Also works on Linux and Mac OS X, but is not documented (?)
        # Windows, Linux and Max OS X allow nonexistent interface numbers here.
        ifindex = 42
        ((*_, sockaddr),) = socket.getaddrinfo(
            'ff02::1de:c0:face:8D%' + str(ifindex),
            1234, socket.AF_INET6,
            socket.SOCK_DGRAM,
            socket.IPPROTO_UDP
        )
        # Note missing interface name part in IPv6 address
        self.assertEqual(sockaddr, ('ff02::1de:c0:face:8d', 1234, 0, ifindex))

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
    @unittest.skipIf(sys.platform == 'win32', 'does not work on Windows')
    @unittest.skipIf(AIX, 'Symbolic scope id does not work')
    @unittest.skipUnless(hasattr(socket, 'if_nameindex'), "test needs socket.if_nameindex()")
    def test_getnameinfo_ipv6_scopeid_symbolic(self):
        # Just pick up any network interface.
        (ifindex, test_interface) = socket.if_nameindex()[0]
        sockaddr = ('ff02::1de:c0:face:8D', 1234, 0, ifindex)  # Note capital letter `D`.
        nameinfo = socket.getnameinfo(sockaddr, socket.NI_NUMERICHOST | socket.NI_NUMERICSERV)
        self.assertEqual(nameinfo, ('ff02::1de:c0:face:8d%' + test_interface, '1234'))

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
    @unittest.skipUnless( sys.platform == 'win32',
        'Numeric scope id does not work or undocumented')
    def test_getnameinfo_ipv6_scopeid_numeric(self):
        # Also works on Linux (undocumented), but does not work on Mac OS X
        # Windows and Linux allow nonexistent interface numbers here.
        ifindex = 42
        sockaddr = ('ff02::1de:c0:face:8D', 1234, 0, ifindex)  # Note capital letter `D`.
        nameinfo = socket.getnameinfo(sockaddr, socket.NI_NUMERICHOST | socket.NI_NUMERICSERV)
        self.assertEqual(nameinfo, ('ff02::1de:c0:face:8d%' + str(ifindex), '1234'))

    def test_str_for_enums(self):
        # Make sure that the AF_* and SOCK_* constants have enum-like string
        # reprs.
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            self.assertEqual(str(s.family), 'AF_INET')
            self.assertEqual(str(s.type), 'SOCK_STREAM')

    def test_socket_consistent_sock_type(self):
        SOCK_NONBLOCK = getattr(socket, 'SOCK_NONBLOCK', 0)
        SOCK_CLOEXEC = getattr(socket, 'SOCK_CLOEXEC', 0)
        sock_type = socket.SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC

        with socket.socket(socket.AF_INET, sock_type) as s:
            self.assertEqual(s.type, socket.SOCK_STREAM)
            s.settimeout(1)
            self.assertEqual(s.type, socket.SOCK_STREAM)
            s.settimeout(0)
            self.assertEqual(s.type, socket.SOCK_STREAM)
            s.setblocking(True)
            self.assertEqual(s.type, socket.SOCK_STREAM)
            s.setblocking(False)
            self.assertEqual(s.type, socket.SOCK_STREAM)

    def test_unknown_socket_family_repr(self):
        # Test that when created with a family that's not one of the known
        # AF_*/SOCK_* constants, socket.family just returns the number.
        #
        # To do this we fool socket.socket into believing it already has an
        # open fd because on this path it doesn't actually verify the family and
        # type and populates the socket object.
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        fd = sock.detach()
        unknown_family = max(socket.AddressFamily.__members__.values()) + 1

        unknown_type = max(
            kind
            for name, kind in socket.SocketKind.__members__.items()
            if name not in {'SOCK_NONBLOCK', 'SOCK_CLOEXEC'}
        ) + 1

        with socket.socket(
                family=unknown_family, type=unknown_type, proto=23,
                fileno=fd) as s:
            self.assertEqual(s.family, unknown_family)
            self.assertEqual(s.type, unknown_type)
            # some OS like macOS ignore proto
            self.assertIn(s.proto, {0, 23})

    @unittest.skipUnless(hasattr(os, 'sendfile'), 'test needs os.sendfile()')
    def test__sendfile_use_sendfile(self):
        class File:
            def __init__(self, fd):
                self.fd = fd

            def fileno(self):
                return self.fd
        with socket.socket() as sock:
            fd = os.open(os.curdir, os.O_RDONLY)
            os.close(fd)
            with self.assertRaises(socket._GiveupOnSendfile):
                sock._sendfile_use_sendfile(File(fd))
            with self.assertRaises(OverflowError):
                sock._sendfile_use_sendfile(File(2**1000))
            with self.assertRaises(TypeError):
                sock._sendfile_use_sendfile(File(None))

    def _test_socket_fileno(self, s, family, stype):
        self.assertEqual(s.family, family)
        self.assertEqual(s.type, stype)

        fd = s.fileno()
        s2 = socket.socket(fileno=fd)
        self.addCleanup(s2.close)
        # detach old fd to avoid double close
        s.detach()
        self.assertEqual(s2.family, family)
        self.assertEqual(s2.type, stype)
        self.assertEqual(s2.fileno(), fd)

    def test_socket_fileno(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(s.close)
        s.bind((socket_helper.HOST, 0))
        self._test_socket_fileno(s, socket.AF_INET, socket.SOCK_STREAM)

        if hasattr(socket, "SOCK_DGRAM"):
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.addCleanup(s.close)
            s.bind((socket_helper.HOST, 0))
            self._test_socket_fileno(s, socket.AF_INET, socket.SOCK_DGRAM)

        if socket_helper.IPV6_ENABLED:
            s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            self.addCleanup(s.close)
            s.bind((socket_helper.HOSTv6, 0, 0, 0))
            self._test_socket_fileno(s, socket.AF_INET6, socket.SOCK_STREAM)

        if hasattr(socket, "AF_UNIX"):
            tmpdir = tempfile.mkdtemp()
            self.addCleanup(shutil.rmtree, tmpdir)
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.addCleanup(s.close)
            try:
                s.bind(os.path.join(tmpdir, 'socket'))
            except PermissionError:
                pass
            else:
                self._test_socket_fileno(s, socket.AF_UNIX,
                                         socket.SOCK_STREAM)

    def test_socket_fileno_rejects_float(self):
        with self.assertRaises(TypeError):
            socket.socket(socket.AF_INET, socket.SOCK_STREAM, fileno=42.5)

    def test_socket_fileno_rejects_other_types(self):
        with self.assertRaises(TypeError):
            socket.socket(socket.AF_INET, socket.SOCK_STREAM, fileno="foo")

    def test_socket_fileno_rejects_invalid_socket(self):
        with self.assertRaisesRegex(ValueError, "negative file descriptor"):
            socket.socket(socket.AF_INET, socket.SOCK_STREAM, fileno=-1)

    @unittest.skipIf(os.name == "nt", "Windows disallows -1 only")
    def test_socket_fileno_rejects_negative(self):
        with self.assertRaisesRegex(ValueError, "negative file descriptor"):
            socket.socket(socket.AF_INET, socket.SOCK_STREAM, fileno=-42)

    def test_socket_fileno_requires_valid_fd(self):
        WSAENOTSOCK = 10038
        with self.assertRaises(OSError) as cm:
            socket.socket(fileno=os_helper.make_bad_fd())
        self.assertIn(cm.exception.errno, (errno.EBADF, WSAENOTSOCK))

        with self.assertRaises(OSError) as cm:
            socket.socket(
                socket.AF_INET,
                socket.SOCK_STREAM,
                fileno=os_helper.make_bad_fd())
        self.assertIn(cm.exception.errno, (errno.EBADF, WSAENOTSOCK))

    def test_socket_fileno_requires_socket_fd(self):
        with tempfile.NamedTemporaryFile() as afile:
            with self.assertRaises(OSError):
                socket.socket(fileno=afile.fileno())

            with self.assertRaises(OSError) as cm:
                socket.socket(
                    socket.AF_INET,
                    socket.SOCK_STREAM,
                    fileno=afile.fileno())
            self.assertEqual(cm.exception.errno, errno.ENOTSOCK)


@unittest.skipUnless(HAVE_SOCKET_CAN, 'SocketCan required for this test.')
class BasicCANTest(unittest.TestCase):

    def testCrucialConstants(self):
        socket.AF_CAN
        socket.PF_CAN
        socket.CAN_RAW

    @unittest.skipUnless(hasattr(socket, "CAN_BCM"),
                         'socket.CAN_BCM required for this test.')
    def testBCMConstants(self):
        socket.CAN_BCM

        # opcodes
        socket.CAN_BCM_TX_SETUP     # create (cyclic) transmission task
        socket.CAN_BCM_TX_DELETE    # remove (cyclic) transmission task
        socket.CAN_BCM_TX_READ      # read properties of (cyclic) transmission task
        socket.CAN_BCM_TX_SEND      # send one CAN frame
        socket.CAN_BCM_RX_SETUP     # create RX content filter subscription
        socket.CAN_BCM_RX_DELETE    # remove RX content filter subscription
        socket.CAN_BCM_RX_READ      # read properties of RX content filter subscription
        socket.CAN_BCM_TX_STATUS    # reply to TX_READ request
        socket.CAN_BCM_TX_EXPIRED   # notification on performed transmissions (count=0)
        socket.CAN_BCM_RX_STATUS    # reply to RX_READ request
        socket.CAN_BCM_RX_TIMEOUT   # cyclic message is absent
        socket.CAN_BCM_RX_CHANGED   # updated CAN frame (detected content change)

        # flags
        socket.CAN_BCM_SETTIMER
        socket.CAN_BCM_STARTTIMER
        socket.CAN_BCM_TX_COUNTEVT
        socket.CAN_BCM_TX_ANNOUNCE
        socket.CAN_BCM_TX_CP_CAN_ID
        socket.CAN_BCM_RX_FILTER_ID
        socket.CAN_BCM_RX_CHECK_DLC
        socket.CAN_BCM_RX_NO_AUTOTIMER
        socket.CAN_BCM_RX_ANNOUNCE_RESUME
        socket.CAN_BCM_TX_RESET_MULTI_IDX
        socket.CAN_BCM_RX_RTR_FRAME

    def testCreateSocket(self):
        with socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW) as s:
            pass

    @unittest.skipUnless(hasattr(socket, "CAN_BCM"),
                         'socket.CAN_BCM required for this test.')
    def testCreateBCMSocket(self):
        with socket.socket(socket.PF_CAN, socket.SOCK_DGRAM, socket.CAN_BCM) as s:
            pass

    def testBindAny(self):
        with socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW) as s:
            address = ('', )
            s.bind(address)
            self.assertEqual(s.getsockname(), address)

    def testTooLongInterfaceName(self):
        # most systems limit IFNAMSIZ to 16, take 1024 to be sure
        with socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW) as s:
            self.assertRaisesRegex(OSError, 'interface name too long',
                                   s.bind, ('x' * 1024,))

    @unittest.skipUnless(hasattr(socket, "CAN_RAW_LOOPBACK"),
                         'socket.CAN_RAW_LOOPBACK required for this test.')
    def testLoopback(self):
        with socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW) as s:
            for loopback in (0, 1):
                s.setsockopt(socket.SOL_CAN_RAW, socket.CAN_RAW_LOOPBACK,
                             loopback)
                self.assertEqual(loopback,
                    s.getsockopt(socket.SOL_CAN_RAW, socket.CAN_RAW_LOOPBACK))

    @unittest.skipUnless(hasattr(socket, "CAN_RAW_FILTER"),
                         'socket.CAN_RAW_FILTER required for this test.')
    def testFilter(self):
        can_id, can_mask = 0x200, 0x700
        can_filter = struct.pack("=II", can_id, can_mask)
        with socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW) as s:
            s.setsockopt(socket.SOL_CAN_RAW, socket.CAN_RAW_FILTER, can_filter)
            self.assertEqual(can_filter,
                    s.getsockopt(socket.SOL_CAN_RAW, socket.CAN_RAW_FILTER, 8))
            s.setsockopt(socket.SOL_CAN_RAW, socket.CAN_RAW_FILTER, bytearray(can_filter))


@unittest.skipUnless(HAVE_SOCKET_CAN, 'SocketCan required for this test.')
class CANTest(ThreadedCANSocketTest):

    def __init__(self, methodName='runTest'):
        ThreadedCANSocketTest.__init__(self, methodName=methodName)

    @classmethod
    def build_can_frame(cls, can_id, data):
        """Build a CAN frame."""
        can_dlc = len(data)
        data = data.ljust(8, b'\x00')
        return struct.pack(cls.can_frame_fmt, can_id, can_dlc, data)

    @classmethod
    def dissect_can_frame(cls, frame):
        """Dissect a CAN frame."""
        can_id, can_dlc, data = struct.unpack(cls.can_frame_fmt, frame)
        return (can_id, can_dlc, data[:can_dlc])

    def testSendFrame(self):
        cf, addr = self.s.recvfrom(self.bufsize)
        self.assertEqual(self.cf, cf)
        self.assertEqual(addr[0], self.interface)
        self.assertEqual(addr[1], socket.AF_CAN)

    def _testSendFrame(self):
        self.cf = self.build_can_frame(0x00, b'\x01\x02\x03\x04\x05')
        self.cli.send(self.cf)

    def testSendMaxFrame(self):
        cf, addr = self.s.recvfrom(self.bufsize)
        self.assertEqual(self.cf, cf)

    def _testSendMaxFrame(self):
        self.cf = self.build_can_frame(0x00, b'\x07' * 8)
        self.cli.send(self.cf)

    def testSendMultiFrames(self):
        cf, addr = self.s.recvfrom(self.bufsize)
        self.assertEqual(self.cf1, cf)

        cf, addr = self.s.recvfrom(self.bufsize)
        self.assertEqual(self.cf2, cf)

    def _testSendMultiFrames(self):
        self.cf1 = self.build_can_frame(0x07, b'\x44\x33\x22\x11')
        self.cli.send(self.cf1)

        self.cf2 = self.build_can_frame(0x12, b'\x99\x22\x33')
        self.cli.send(self.cf2)

    @unittest.skipUnless(hasattr(socket, "CAN_BCM"),
                         'socket.CAN_BCM required for this test.')
    def _testBCM(self):
        cf, addr = self.cli.recvfrom(self.bufsize)
        self.assertEqual(self.cf, cf)
        can_id, can_dlc, data = self.dissect_can_frame(cf)
        self.assertEqual(self.can_id, can_id)
        self.assertEqual(self.data, data)

    @unittest.skipUnless(hasattr(socket, "CAN_BCM"),
                         'socket.CAN_BCM required for this test.')
    def testBCM(self):
        bcm = socket.socket(socket.PF_CAN, socket.SOCK_DGRAM, socket.CAN_BCM)
        self.addCleanup(bcm.close)
        bcm.connect((self.interface,))
        self.can_id = 0x123
        self.data = bytes([0xc0, 0xff, 0xee])
        self.cf = self.build_can_frame(self.can_id, self.data)
        opcode = socket.CAN_BCM_TX_SEND
        flags = 0
        count = 0
        ival1_seconds = ival1_usec = ival2_seconds = ival2_usec = 0
        bcm_can_id = 0x0222
        nframes = 1
        assert len(self.cf) == 16
        header = struct.pack(self.bcm_cmd_msg_fmt,
                    opcode,
                    flags,
                    count,
                    ival1_seconds,
                    ival1_usec,
                    ival2_seconds,
                    ival2_usec,
                    bcm_can_id,
                    nframes,
                    )
        header_plus_frame = header + self.cf
        bytes_sent = bcm.send(header_plus_frame)
        self.assertEqual(bytes_sent, len(header_plus_frame))


@unittest.skipUnless(HAVE_SOCKET_CAN_ISOTP, 'CAN ISOTP required for this test.')
class ISOTPTest(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.interface = "vcan0"

    def testCrucialConstants(self):
        socket.AF_CAN
        socket.PF_CAN
        socket.CAN_ISOTP
        socket.SOCK_DGRAM

    def testCreateSocket(self):
        with socket.socket(socket.PF_CAN, socket.SOCK_RAW, socket.CAN_RAW) as s:
            pass

    @unittest.skipUnless(hasattr(socket, "CAN_ISOTP"),
                         'socket.CAN_ISOTP required for this test.')
    def testCreateISOTPSocket(self):
        with socket.socket(socket.PF_CAN, socket.SOCK_DGRAM, socket.CAN_ISOTP) as s:
            pass

    def testTooLongInterfaceName(self):
        # most systems limit IFNAMSIZ to 16, take 1024 to be sure
        with socket.socket(socket.PF_CAN, socket.SOCK_DGRAM, socket.CAN_ISOTP) as s:
            with self.assertRaisesRegex(OSError, 'interface name too long'):
                s.bind(('x' * 1024, 1, 2))

    def testBind(self):
        try:
            with socket.socket(socket.PF_CAN, socket.SOCK_DGRAM, socket.CAN_ISOTP) as s:
                addr = self.interface, 0x123, 0x456
                s.bind(addr)
                self.assertEqual(s.getsockname(), addr)
        except OSError as e:
            if e.errno == errno.ENODEV:
                self.skipTest('network interface `%s` does not exist' %
                           self.interface)
            else:
                raise


@unittest.skipUnless(HAVE_SOCKET_CAN_J1939, 'CAN J1939 required for this test.')
class J1939Test(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.interface = "vcan0"

    @unittest.skipUnless(hasattr(socket, "CAN_J1939"),
                         'socket.CAN_J1939 required for this test.')
    def testJ1939Constants(self):
        socket.CAN_J1939

        socket.J1939_MAX_UNICAST_ADDR
        socket.J1939_IDLE_ADDR
        socket.J1939_NO_ADDR
        socket.J1939_NO_NAME
        socket.J1939_PGN_REQUEST
        socket.J1939_PGN_ADDRESS_CLAIMED
        socket.J1939_PGN_ADDRESS_COMMANDED
        socket.J1939_PGN_PDU1_MAX
        socket.J1939_PGN_MAX
        socket.J1939_NO_PGN

        # J1939 socket options
        socket.SO_J1939_FILTER
        socket.SO_J1939_PROMISC
        socket.SO_J1939_SEND_PRIO
        socket.SO_J1939_ERRQUEUE

        socket.SCM_J1939_DEST_ADDR
        socket.SCM_J1939_DEST_NAME
        socket.SCM_J1939_PRIO
        socket.SCM_J1939_ERRQUEUE

        socket.J1939_NLA_PAD
        socket.J1939_NLA_BYTES_ACKED

        socket.J1939_EE_INFO_NONE
        socket.J1939_EE_INFO_TX_ABORT

        socket.J1939_FILTER_MAX

    @unittest.skipUnless(hasattr(socket, "CAN_J1939"),
                         'socket.CAN_J1939 required for this test.')
    def testCreateJ1939Socket(self):
        with socket.socket(socket.PF_CAN, socket.SOCK_DGRAM, socket.CAN_J1939) as s:
            pass

    def testBind(self):
        try:
            with socket.socket(socket.PF_CAN, socket.SOCK_DGRAM, socket.CAN_J1939) as s:
                addr = self.interface, socket.J1939_NO_NAME, socket.J1939_NO_PGN, socket.J1939_NO_ADDR
                s.bind(addr)
                self.assertEqual(s.getsockname(), addr)
        except OSError as e:
            if e.errno == errno.ENODEV:
                self.skipTest('network interface `%s` does not exist' %
                           self.interface)
            else:
                raise


@unittest.skipUnless(HAVE_SOCKET_RDS, 'RDS sockets required for this test.')
class BasicRDSTest(unittest.TestCase):

    def testCrucialConstants(self):
        socket.AF_RDS
        socket.PF_RDS

    def testCreateSocket(self):
        with socket.socket(socket.PF_RDS, socket.SOCK_SEQPACKET, 0) as s:
            pass

    def testSocketBufferSize(self):
        bufsize = 16384
        with socket.socket(socket.PF_RDS, socket.SOCK_SEQPACKET, 0) as s:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, bufsize)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, bufsize)


@unittest.skipUnless(HAVE_SOCKET_RDS, 'RDS sockets required for this test.')
class RDSTest(ThreadedRDSSocketTest):

    def __init__(self, methodName='runTest'):
        ThreadedRDSSocketTest.__init__(self, methodName=methodName)

    def setUp(self):
        super().setUp()
        self.evt = threading.Event()

    def testSendAndRecv(self):
        data, addr = self.serv.recvfrom(self.bufsize)
        self.assertEqual(self.data, data)
        self.assertEqual(self.cli_addr, addr)

    def _testSendAndRecv(self):
        self.data = b'spam'
        self.cli.sendto(self.data, 0, (HOST, self.port))

    def testPeek(self):
        data, addr = self.serv.recvfrom(self.bufsize, socket.MSG_PEEK)
        self.assertEqual(self.data, data)
        data, addr = self.serv.recvfrom(self.bufsize)
        self.assertEqual(self.data, data)

    def _testPeek(self):
        self.data = b'spam'
        self.cli.sendto(self.data, 0, (HOST, self.port))

    @requireAttrs(socket.socket, 'recvmsg')
    def testSendAndRecvMsg(self):
        data, ancdata, msg_flags, addr = self.serv.recvmsg(self.bufsize)
        self.assertEqual(self.data, data)

    @requireAttrs(socket.socket, 'sendmsg')
    def _testSendAndRecvMsg(self):
        self.data = b'hello ' * 10
        self.cli.sendmsg([self.data], (), 0, (HOST, self.port))

    def testSendAndRecvMulti(self):
        data, addr = self.serv.recvfrom(self.bufsize)
        self.assertEqual(self.data1, data)

        data, addr = self.serv.recvfrom(self.bufsize)
        self.assertEqual(self.data2, data)

    def _testSendAndRecvMulti(self):
        self.data1 = b'bacon'
        self.cli.sendto(self.data1, 0, (HOST, self.port))

        self.data2 = b'egg'
        self.cli.sendto(self.data2, 0, (HOST, self.port))

    def testSelect(self):
        r, w, x = select.select([self.serv], [], [], 3.0)
        self.assertIn(self.serv, r)
        data, addr = self.serv.recvfrom(self.bufsize)
        self.assertEqual(self.data, data)

    def _testSelect(self):
        self.data = b'select'
        self.cli.sendto(self.data, 0, (HOST, self.port))

@unittest.skipUnless(HAVE_SOCKET_QIPCRTR,
          'QIPCRTR sockets required for this test.')
class BasicQIPCRTRTest(unittest.TestCase):

    def testCrucialConstants(self):
        socket.AF_QIPCRTR

    def testCreateSocket(self):
        with socket.socket(socket.AF_QIPCRTR, socket.SOCK_DGRAM) as s:
            pass

    def testUnbound(self):
        with socket.socket(socket.AF_QIPCRTR, socket.SOCK_DGRAM) as s:
            self.assertEqual(s.getsockname()[1], 0)

    def testBindSock(self):
        with socket.socket(socket.AF_QIPCRTR, socket.SOCK_DGRAM) as s:
            socket_helper.bind_port(s, host=s.getsockname()[0])
            self.assertNotEqual(s.getsockname()[1], 0)

    def testInvalidBindSock(self):
        with socket.socket(socket.AF_QIPCRTR, socket.SOCK_DGRAM) as s:
            self.assertRaises(OSError, socket_helper.bind_port, s, host=-2)

    def testAutoBindSock(self):
        with socket.socket(socket.AF_QIPCRTR, socket.SOCK_DGRAM) as s:
            s.connect((123, 123))
            self.assertNotEqual(s.getsockname()[1], 0)

@unittest.skipIf(fcntl is None, "need fcntl")
@unittest.skipUnless(HAVE_SOCKET_VSOCK,
          'VSOCK sockets required for this test.')
class BasicVSOCKTest(unittest.TestCase):

    def testCrucialConstants(self):
        socket.AF_VSOCK

    def testVSOCKConstants(self):
        socket.SO_VM_SOCKETS_BUFFER_SIZE
        socket.SO_VM_SOCKETS_BUFFER_MIN_SIZE
        socket.SO_VM_SOCKETS_BUFFER_MAX_SIZE
        socket.VMADDR_CID_ANY
        socket.VMADDR_PORT_ANY
        socket.VMADDR_CID_HOST
        socket.VM_SOCKETS_INVALID_VERSION
        socket.IOCTL_VM_SOCKETS_GET_LOCAL_CID

    def testCreateSocket(self):
        with socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM) as s:
            pass

    def testSocketBufferSize(self):
        with socket.socket(socket.AF_VSOCK, socket.SOCK_STREAM) as s:
            orig_max = s.getsockopt(socket.AF_VSOCK,
                                    socket.SO_VM_SOCKETS_BUFFER_MAX_SIZE)
            orig = s.getsockopt(socket.AF_VSOCK,
                                socket.SO_VM_SOCKETS_BUFFER_SIZE)
            orig_min = s.getsockopt(socket.AF_VSOCK,
                                    socket.SO_VM_SOCKETS_BUFFER_MIN_SIZE)

            s.setsockopt(socket.AF_VSOCK,
                         socket.SO_VM_SOCKETS_BUFFER_MAX_SIZE, orig_max * 2)
            s.setsockopt(socket.AF_VSOCK,
                         socket.SO_VM_SOCKETS_BUFFER_SIZE, orig * 2)
            s.setsockopt(socket.AF_VSOCK,
                         socket.SO_VM_SOCKETS_BUFFER_MIN_SIZE, orig_min * 2)

            self.assertEqual(orig_max * 2,
                             s.getsockopt(socket.AF_VSOCK,
                             socket.SO_VM_SOCKETS_BUFFER_MAX_SIZE))
            self.assertEqual(orig * 2,
                             s.getsockopt(socket.AF_VSOCK,
                             socket.SO_VM_SOCKETS_BUFFER_SIZE))
            self.assertEqual(orig_min * 2,
                             s.getsockopt(socket.AF_VSOCK,
                             socket.SO_VM_SOCKETS_BUFFER_MIN_SIZE))


@unittest.skipUnless(HAVE_SOCKET_BLUETOOTH,
                     'Bluetooth sockets required for this test.')
class BasicBluetoothTest(unittest.TestCase):

    def testBluetoothConstants(self):
        socket.BDADDR_ANY
        socket.BDADDR_LOCAL
        socket.AF_BLUETOOTH
        socket.BTPROTO_RFCOMM

        if sys.platform != "win32":
            socket.BTPROTO_HCI
            socket.SOL_HCI
            socket.BTPROTO_L2CAP

            if not sys.platform.startswith("freebsd"):
                socket.BTPROTO_SCO

    def testCreateRfcommSocket(self):
        with socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM) as s:
            pass

    @unittest.skipIf(sys.platform == "win32", "windows does not support L2CAP sockets")
    def testCreateL2capSocket(self):
        with socket.socket(socket.AF_BLUETOOTH, socket.SOCK_SEQPACKET, socket.BTPROTO_L2CAP) as s:
            pass

    @unittest.skipIf(sys.platform == "win32", "windows does not support HCI sockets")
    def testCreateHciSocket(self):
        with socket.socket(socket.AF_BLUETOOTH, socket.SOCK_RAW, socket.BTPROTO_HCI) as s:
            pass

    @unittest.skipIf(sys.platform == "win32" or sys.platform.startswith("freebsd"),
                     "windows and freebsd do not support SCO sockets")
    def testCreateScoSocket(self):
        with socket.socket(socket.AF_BLUETOOTH, socket.SOCK_SEQPACKET, socket.BTPROTO_SCO) as s:
            pass


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

    testShutdown_overflow = support.cpython_only(testShutdown)

    @support.cpython_only
    def _testShutdown_overflow(self):
        import _testcapi
        self.serv_conn.send(MSG)
        # Issue 15989
        self.assertRaises(OverflowError, self.serv_conn.shutdown,
                          _testcapi.INT_MAX + 1)
        self.assertRaises(OverflowError, self.serv_conn.shutdown,
                          2 + (_testcapi.UINT_MAX + 1))
        self.serv_conn.shutdown(2)

    def testDetach(self):
        # Testing detach()
        fileno = self.cli_conn.fileno()
        f = self.cli_conn.detach()
        self.assertEqual(f, fileno)
        # cli_conn cannot be used anymore...
        self.assertTrue(self.cli_conn._closed)
        self.assertRaises(OSError, self.cli_conn.recv, 1024)
        self.cli_conn.close()
        # ...but we can create another socket using the (still open)
        # file descriptor
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, fileno=f)
        self.addCleanup(sock.close)
        msg = sock.recv(1024)
        self.assertEqual(msg, MSG)

    def _testDetach(self):
        self.serv_conn.send(MSG)


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


@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
class BasicUDPLITETest(ThreadedUDPLITESocketTest):

    def __init__(self, methodName='runTest'):
        ThreadedUDPLITESocketTest.__init__(self, methodName=methodName)

    def testSendtoAndRecv(self):
        # Testing sendto() and Recv() over UDPLITE
        msg = self.serv.recv(len(MSG))
        self.assertEqual(msg, MSG)

    def _testSendtoAndRecv(self):
        self.cli.sendto(MSG, 0, (HOST, self.port))

    def testRecvFrom(self):
        # Testing recvfrom() over UDPLITE
        msg, addr = self.serv.recvfrom(len(MSG))
        self.assertEqual(msg, MSG)

    def _testRecvFrom(self):
        self.cli.sendto(MSG, 0, (HOST, self.port))

    def testRecvFromNegative(self):
        # Negative lengths passed to recvfrom should give ValueError.
        self.assertRaises(ValueError, self.serv.recvfrom, -1)

    def _testRecvFromNegative(self):
        self.cli.sendto(MSG, 0, (HOST, self.port))

# Tests for the sendmsg()/recvmsg() interface.  Where possible, the
# same test code is used with different families and types of socket
# (e.g. stream, datagram), and tests using recvmsg() are repeated
# using recvmsg_into().
#
# The generic test classes such as SendmsgTests and
# RecvmsgGenericTests inherit from SendrecvmsgBase and expect to be
# supplied with sockets cli_sock and serv_sock representing the
# client's and the server's end of the connection respectively, and
# attributes cli_addr and serv_addr holding their (numeric where
# appropriate) addresses.
#
# The final concrete test classes combine these with subclasses of
# SocketTestBase which set up client and server sockets of a specific
# type, and with subclasses of SendrecvmsgBase such as
# SendrecvmsgDgramBase and SendrecvmsgConnectedBase which map these
# sockets to cli_sock and serv_sock and override the methods and
# attributes of SendrecvmsgBase to fill in destination addresses if
# needed when sending, check for specific flags in msg_flags, etc.
#
# RecvmsgIntoMixin provides a version of doRecvmsg() implemented using
# recvmsg_into().

# XXX: like the other datagram (UDP) tests in this module, the code
# here assumes that datagram delivery on the local machine will be
# reliable.

class SendrecvmsgBase(ThreadSafeCleanupTestCase):
    # Base class for sendmsg()/recvmsg() tests.

    # Time in seconds to wait before considering a test failed, or
    # None for no timeout.  Not all tests actually set a timeout.
    fail_timeout = support.LOOPBACK_TIMEOUT

    def setUp(self):
        self.misc_event = threading.Event()
        super().setUp()

    def sendToServer(self, msg):
        # Send msg to the server.
        return self.cli_sock.send(msg)

    # Tuple of alternative default arguments for sendmsg() when called
    # via sendmsgToServer() (e.g. to include a destination address).
    sendmsg_to_server_defaults = ()

    def sendmsgToServer(self, *args):
        # Call sendmsg() on self.cli_sock with the given arguments,
        # filling in any arguments which are not supplied with the
        # corresponding items of self.sendmsg_to_server_defaults, if
        # any.
        return self.cli_sock.sendmsg(
            *(args + self.sendmsg_to_server_defaults[len(args):]))

    def doRecvmsg(self, sock, bufsize, *args):
        # Call recvmsg() on sock with given arguments and return its
        # result.  Should be used for tests which can use either
        # recvmsg() or recvmsg_into() - RecvmsgIntoMixin overrides
        # this method with one which emulates it using recvmsg_into(),
        # thus allowing the same test to be used for both methods.
        result = sock.recvmsg(bufsize, *args)
        self.registerRecvmsgResult(result)
        return result

    def registerRecvmsgResult(self, result):
        # Called by doRecvmsg() with the return value of recvmsg() or
        # recvmsg_into().  Can be overridden to arrange cleanup based
        # on the returned ancillary data, for instance.
        pass

    def checkRecvmsgAddress(self, addr1, addr2):
        # Called to compare the received address with the address of
        # the peer.
        self.assertEqual(addr1, addr2)

    # Flags that are normally unset in msg_flags
    msg_flags_common_unset = 0
    for name in ("MSG_CTRUNC", "MSG_OOB"):
        msg_flags_common_unset |= getattr(socket, name, 0)

    # Flags that are normally set
    msg_flags_common_set = 0

    # Flags set when a complete record has been received (e.g. MSG_EOR
    # for SCTP)
    msg_flags_eor_indicator = 0

    # Flags set when a complete record has not been received
    # (e.g. MSG_TRUNC for datagram sockets)
    msg_flags_non_eor_indicator = 0

    def checkFlags(self, flags, eor=None, checkset=0, checkunset=0, ignore=0):
        # Method to check the value of msg_flags returned by recvmsg[_into]().
        #
        # Checks that all bits in msg_flags_common_set attribute are
        # set in "flags" and all bits in msg_flags_common_unset are
        # unset.
        #
        # The "eor" argument specifies whether the flags should
        # indicate that a full record (or datagram) has been received.
        # If "eor" is None, no checks are done; otherwise, checks
        # that:
        #
        #  * if "eor" is true, all bits in msg_flags_eor_indicator are
        #    set and all bits in msg_flags_non_eor_indicator are unset
        #
        #  * if "eor" is false, all bits in msg_flags_non_eor_indicator
        #    are set and all bits in msg_flags_eor_indicator are unset
        #
        # If "checkset" and/or "checkunset" are supplied, they require
        # the given bits to be set or unset respectively, overriding
        # what the attributes require for those bits.
        #
        # If any bits are set in "ignore", they will not be checked,
        # regardless of the other inputs.
        #
        # Will raise Exception if the inputs require a bit to be both
        # set and unset, and it is not ignored.

        defaultset = self.msg_flags_common_set
        defaultunset = self.msg_flags_common_unset

        if eor:
            defaultset |= self.msg_flags_eor_indicator
            defaultunset |= self.msg_flags_non_eor_indicator
        elif eor is not None:
            defaultset |= self.msg_flags_non_eor_indicator
            defaultunset |= self.msg_flags_eor_indicator

        # Function arguments override defaults
        defaultset &= ~checkunset
        defaultunset &= ~checkset

        # Merge arguments with remaining defaults, and check for conflicts
        checkset |= defaultset
        checkunset |= defaultunset
        inboth = checkset & checkunset & ~ignore
        if inboth:
            raise Exception("contradictory set, unset requirements for flags "
                            "{0:#x}".format(inboth))

        # Compare with given msg_flags value
        mask = (checkset | checkunset) & ~ignore
        self.assertEqual(flags & mask, checkset & mask)


class RecvmsgIntoMixin(SendrecvmsgBase):
    # Mixin to implement doRecvmsg() using recvmsg_into().

    def doRecvmsg(self, sock, bufsize, *args):
        buf = bytearray(bufsize)
        result = sock.recvmsg_into([buf], *args)
        self.registerRecvmsgResult(result)
        self.assertGreaterEqual(result[0], 0)
        self.assertLessEqual(result[0], bufsize)
        return (bytes(buf[:result[0]]),) + result[1:]


class SendrecvmsgDgramFlagsBase(SendrecvmsgBase):
    # Defines flags to be checked in msg_flags for datagram sockets.

    @property
    def msg_flags_non_eor_indicator(self):
        return super().msg_flags_non_eor_indicator | socket.MSG_TRUNC


class SendrecvmsgSCTPFlagsBase(SendrecvmsgBase):
    # Defines flags to be checked in msg_flags for SCTP sockets.

    @property
    def msg_flags_eor_indicator(self):
        return super().msg_flags_eor_indicator | socket.MSG_EOR


class SendrecvmsgConnectionlessBase(SendrecvmsgBase):
    # Base class for tests on connectionless-mode sockets.  Users must
    # supply sockets on attributes cli and serv to be mapped to
    # cli_sock and serv_sock respectively.

    @property
    def serv_sock(self):
        return self.serv

    @property
    def cli_sock(self):
        return self.cli

    @property
    def sendmsg_to_server_defaults(self):
        return ([], [], 0, self.serv_addr)

    def sendToServer(self, msg):
        return self.cli_sock.sendto(msg, self.serv_addr)


class SendrecvmsgConnectedBase(SendrecvmsgBase):
    # Base class for tests on connected sockets.  Users must supply
    # sockets on attributes serv_conn and cli_conn (representing the
    # connections *to* the server and the client), to be mapped to
    # cli_sock and serv_sock respectively.

    @property
    def serv_sock(self):
        return self.cli_conn

    @property
    def cli_sock(self):
        return self.serv_conn

    def checkRecvmsgAddress(self, addr1, addr2):
        # Address is currently "unspecified" for a connected socket,
        # so we don't examine it
        pass


class SendrecvmsgServerTimeoutBase(SendrecvmsgBase):
    # Base class to set a timeout on server's socket.

    def setUp(self):
        super().setUp()
        self.serv_sock.settimeout(self.fail_timeout)


class SendmsgTests(SendrecvmsgServerTimeoutBase):
    # Tests for sendmsg() which can use any socket type and do not
    # involve recvmsg() or recvmsg_into().

    def testSendmsg(self):
        # Send a simple message with sendmsg().
        self.assertEqual(self.serv_sock.recv(len(MSG)), MSG)

    def _testSendmsg(self):
        self.assertEqual(self.sendmsgToServer([MSG]), len(MSG))

    def testSendmsgDataGenerator(self):
        # Send from buffer obtained from a generator (not a sequence).
        self.assertEqual(self.serv_sock.recv(len(MSG)), MSG)

    def _testSendmsgDataGenerator(self):
        self.assertEqual(self.sendmsgToServer((o for o in [MSG])),
                         len(MSG))

    def testSendmsgAncillaryGenerator(self):
        # Gather (empty) ancillary data from a generator.
        self.assertEqual(self.serv_sock.recv(len(MSG)), MSG)

    def _testSendmsgAncillaryGenerator(self):
        self.assertEqual(self.sendmsgToServer([MSG], (o for o in [])),
                         len(MSG))

    def testSendmsgArray(self):
        # Send data from an array instead of the usual bytes object.
        self.assertEqual(self.serv_sock.recv(len(MSG)), MSG)

    def _testSendmsgArray(self):
        self.assertEqual(self.sendmsgToServer([array.array("B", MSG)]),
                         len(MSG))

    def testSendmsgGather(self):
        # Send message data from more than one buffer (gather write).
        self.assertEqual(self.serv_sock.recv(len(MSG)), MSG)

    def _testSendmsgGather(self):
        self.assertEqual(self.sendmsgToServer([MSG[:3], MSG[3:]]), len(MSG))

    def testSendmsgBadArgs(self):
        # Check that sendmsg() rejects invalid arguments.
        self.assertEqual(self.serv_sock.recv(1000), b"done")

    def _testSendmsgBadArgs(self):
        self.assertRaises(TypeError, self.cli_sock.sendmsg)
        self.assertRaises(TypeError, self.sendmsgToServer,
                          b"not in an iterable")
        self.assertRaises(TypeError, self.sendmsgToServer,
                          object())
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [object()])
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG, object()])
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], object())
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [], object())
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [], 0, object())
        self.sendToServer(b"done")

    def testSendmsgBadCmsg(self):
        # Check that invalid ancillary data items are rejected.
        self.assertEqual(self.serv_sock.recv(1000), b"done")

    def _testSendmsgBadCmsg(self):
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [object()])
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [(object(), 0, b"data")])
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [(0, object(), b"data")])
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [(0, 0, object())])
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [(0, 0)])
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [(0, 0, b"data", 42)])
        self.sendToServer(b"done")

    @requireAttrs(socket, "CMSG_SPACE")
    def testSendmsgBadMultiCmsg(self):
        # Check that invalid ancillary data items are rejected when
        # more than one item is present.
        self.assertEqual(self.serv_sock.recv(1000), b"done")

    @testSendmsgBadMultiCmsg.client_skip
    def _testSendmsgBadMultiCmsg(self):
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [0, 0, b""])
        self.assertRaises(TypeError, self.sendmsgToServer,
                          [MSG], [(0, 0, b""), object()])
        self.sendToServer(b"done")

    def testSendmsgExcessCmsgReject(self):
        # Check that sendmsg() rejects excess ancillary data items
        # when the number that can be sent is limited.
        self.assertEqual(self.serv_sock.recv(1000), b"done")

    def _testSendmsgExcessCmsgReject(self):
        if not hasattr(socket, "CMSG_SPACE"):
            # Can only send one item
            with self.assertRaises(OSError) as cm:
                self.sendmsgToServer([MSG], [(0, 0, b""), (0, 0, b"")])
            self.assertIsNone(cm.exception.errno)
        self.sendToServer(b"done")

    def testSendmsgAfterClose(self):
        # Check that sendmsg() fails on a closed socket.
        pass

    def _testSendmsgAfterClose(self):
        self.cli_sock.close()
        self.assertRaises(OSError, self.sendmsgToServer, [MSG])


class SendmsgStreamTests(SendmsgTests):
    # Tests for sendmsg() which require a stream socket and do not
    # involve recvmsg() or recvmsg_into().

    def testSendmsgExplicitNoneAddr(self):
        # Check that peer address can be specified as None.
        self.assertEqual(self.serv_sock.recv(len(MSG)), MSG)

    def _testSendmsgExplicitNoneAddr(self):
        self.assertEqual(self.sendmsgToServer([MSG], [], 0, None), len(MSG))

    def testSendmsgTimeout(self):
        # Check that timeout works with sendmsg().
        self.assertEqual(self.serv_sock.recv(512), b"a"*512)
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))

    def _testSendmsgTimeout(self):
        try:
            self.cli_sock.settimeout(0.03)
            try:
                while True:
                    self.sendmsgToServer([b"a"*512])
            except TimeoutError:
                pass
            except OSError as exc:
                if exc.errno != errno.ENOMEM:
                    raise
                # bpo-33937 the test randomly fails on Travis CI with
                # "OSError: [Errno 12] Cannot allocate memory"
            else:
                self.fail("TimeoutError not raised")
        finally:
            self.misc_event.set()

    # XXX: would be nice to have more tests for sendmsg flags argument.

    # Linux supports MSG_DONTWAIT when sending, but in general, it
    # only works when receiving.  Could add other platforms if they
    # support it too.
    @skipWithClientIf(sys.platform not in {"linux"},
                      "MSG_DONTWAIT not known to work on this platform when "
                      "sending")
    def testSendmsgDontWait(self):
        # Check that MSG_DONTWAIT in flags causes non-blocking behaviour.
        self.assertEqual(self.serv_sock.recv(512), b"a"*512)
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))

    @testSendmsgDontWait.client_skip
    def _testSendmsgDontWait(self):
        try:
            with self.assertRaises(OSError) as cm:
                while True:
                    self.sendmsgToServer([b"a"*512], [], socket.MSG_DONTWAIT)
            # bpo-33937: catch also ENOMEM, the test randomly fails on Travis CI
            # with "OSError: [Errno 12] Cannot allocate memory"
            self.assertIn(cm.exception.errno,
                          (errno.EAGAIN, errno.EWOULDBLOCK, errno.ENOMEM))
        finally:
            self.misc_event.set()


class SendmsgConnectionlessTests(SendmsgTests):
    # Tests for sendmsg() which require a connectionless-mode
    # (e.g. datagram) socket, and do not involve recvmsg() or
    # recvmsg_into().

    def testSendmsgNoDestAddr(self):
        # Check that sendmsg() fails when no destination address is
        # given for unconnected socket.
        pass

    def _testSendmsgNoDestAddr(self):
        self.assertRaises(OSError, self.cli_sock.sendmsg,
                          [MSG])
        self.assertRaises(OSError, self.cli_sock.sendmsg,
                          [MSG], [], 0, None)


class RecvmsgGenericTests(SendrecvmsgBase):
    # Tests for recvmsg() which can also be emulated using
    # recvmsg_into(), and can use any socket type.

    def testRecvmsg(self):
        # Receive a simple message with recvmsg[_into]().
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock, len(MSG))
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsg(self):
        self.sendToServer(MSG)

    def testRecvmsgExplicitDefaults(self):
        # Test recvmsg[_into]() with default arguments provided explicitly.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), 0, 0)
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsgExplicitDefaults(self):
        self.sendToServer(MSG)

    def testRecvmsgShorter(self):
        # Receive a message smaller than buffer.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG) + 42)
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsgShorter(self):
        self.sendToServer(MSG)

    def testRecvmsgTrunc(self):
        # Receive part of message, check for truncation indicators.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG) - 3)
        self.assertEqual(msg, MSG[:-3])
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=False)

    def _testRecvmsgTrunc(self):
        self.sendToServer(MSG)

    def testRecvmsgShortAncillaryBuf(self):
        # Test ancillary data buffer too small to hold any ancillary data.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), 1)
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsgShortAncillaryBuf(self):
        self.sendToServer(MSG)

    def testRecvmsgLongAncillaryBuf(self):
        # Test large ancillary data buffer.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), 10240)
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsgLongAncillaryBuf(self):
        self.sendToServer(MSG)

    def testRecvmsgAfterClose(self):
        # Check that recvmsg[_into]() fails on a closed socket.
        self.serv_sock.close()
        self.assertRaises(OSError, self.doRecvmsg, self.serv_sock, 1024)

    def _testRecvmsgAfterClose(self):
        pass

    def testRecvmsgTimeout(self):
        # Check that timeout works.
        try:
            self.serv_sock.settimeout(0.03)
            self.assertRaises(TimeoutError,
                              self.doRecvmsg, self.serv_sock, len(MSG))
        finally:
            self.misc_event.set()

    def _testRecvmsgTimeout(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))

    @requireAttrs(socket, "MSG_PEEK")
    def testRecvmsgPeek(self):
        # Check that MSG_PEEK in flags enables examination of pending
        # data without consuming it.

        # Receive part of data with MSG_PEEK.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG) - 3, 0,
                                                   socket.MSG_PEEK)
        self.assertEqual(msg, MSG[:-3])
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        # Ignoring MSG_TRUNC here (so this test is the same for stream
        # and datagram sockets).  Some wording in POSIX seems to
        # suggest that it needn't be set when peeking, but that may
        # just be a slip.
        self.checkFlags(flags, eor=False,
                        ignore=getattr(socket, "MSG_TRUNC", 0))

        # Receive all data with MSG_PEEK.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), 0,
                                                   socket.MSG_PEEK)
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

        # Check that the same data can still be received normally.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock, len(MSG))
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    @testRecvmsgPeek.client_skip
    def _testRecvmsgPeek(self):
        self.sendToServer(MSG)

    @requireAttrs(socket.socket, "sendmsg")
    def testRecvmsgFromSendmsg(self):
        # Test receiving with recvmsg[_into]() when message is sent
        # using sendmsg().
        self.serv_sock.settimeout(self.fail_timeout)
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock, len(MSG))
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    @testRecvmsgFromSendmsg.client_skip
    def _testRecvmsgFromSendmsg(self):
        self.assertEqual(self.sendmsgToServer([MSG[:3], MSG[3:]]), len(MSG))


class RecvmsgGenericStreamTests(RecvmsgGenericTests):
    # Tests which require a stream socket and can use either recvmsg()
    # or recvmsg_into().

    def testRecvmsgEOF(self):
        # Receive end-of-stream indicator (b"", peer socket closed).
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock, 1024)
        self.assertEqual(msg, b"")
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=None) # Might not have end-of-record marker

    def _testRecvmsgEOF(self):
        self.cli_sock.close()

    def testRecvmsgOverflow(self):
        # Receive a message in more than one chunk.
        seg1, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                    len(MSG) - 3)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=False)

        seg2, ancdata, flags, addr = self.doRecvmsg(self.serv_sock, 1024)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

        msg = seg1 + seg2
        self.assertEqual(msg, MSG)

    def _testRecvmsgOverflow(self):
        self.sendToServer(MSG)


class RecvmsgTests(RecvmsgGenericTests):
    # Tests for recvmsg() which can use any socket type.

    def testRecvmsgBadArgs(self):
        # Check that recvmsg() rejects invalid arguments.
        self.assertRaises(TypeError, self.serv_sock.recvmsg)
        self.assertRaises(ValueError, self.serv_sock.recvmsg,
                          -1, 0, 0)
        self.assertRaises(ValueError, self.serv_sock.recvmsg,
                          len(MSG), -1, 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg,
                          [bytearray(10)], 0, 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg,
                          object(), 0, 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg,
                          len(MSG), object(), 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg,
                          len(MSG), 0, object())

        msg, ancdata, flags, addr = self.serv_sock.recvmsg(len(MSG), 0, 0)
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsgBadArgs(self):
        self.sendToServer(MSG)


class RecvmsgIntoTests(RecvmsgIntoMixin, RecvmsgGenericTests):
    # Tests for recvmsg_into() which can use any socket type.

    def testRecvmsgIntoBadArgs(self):
        # Check that recvmsg_into() rejects invalid arguments.
        buf = bytearray(len(MSG))
        self.assertRaises(TypeError, self.serv_sock.recvmsg_into)
        self.assertRaises(TypeError, self.serv_sock.recvmsg_into,
                          len(MSG), 0, 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg_into,
                          buf, 0, 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg_into,
                          [object()], 0, 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg_into,
                          [b"I'm not writable"], 0, 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg_into,
                          [buf, object()], 0, 0)
        self.assertRaises(ValueError, self.serv_sock.recvmsg_into,
                          [buf], -1, 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg_into,
                          [buf], object(), 0)
        self.assertRaises(TypeError, self.serv_sock.recvmsg_into,
                          [buf], 0, object())

        nbytes, ancdata, flags, addr = self.serv_sock.recvmsg_into([buf], 0, 0)
        self.assertEqual(nbytes, len(MSG))
        self.assertEqual(buf, bytearray(MSG))
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsgIntoBadArgs(self):
        self.sendToServer(MSG)

    def testRecvmsgIntoGenerator(self):
        # Receive into buffer obtained from a generator (not a sequence).
        buf = bytearray(len(MSG))
        nbytes, ancdata, flags, addr = self.serv_sock.recvmsg_into(
            (o for o in [buf]))
        self.assertEqual(nbytes, len(MSG))
        self.assertEqual(buf, bytearray(MSG))
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsgIntoGenerator(self):
        self.sendToServer(MSG)

    def testRecvmsgIntoArray(self):
        # Receive into an array rather than the usual bytearray.
        buf = array.array("B", [0] * len(MSG))
        nbytes, ancdata, flags, addr = self.serv_sock.recvmsg_into([buf])
        self.assertEqual(nbytes, len(MSG))
        self.assertEqual(buf.tobytes(), MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsgIntoArray(self):
        self.sendToServer(MSG)

    def testRecvmsgIntoScatter(self):
        # Receive into multiple buffers (scatter write).
        b1 = bytearray(b"----")
        b2 = bytearray(b"0123456789")
        b3 = bytearray(b"--------------")
        nbytes, ancdata, flags, addr = self.serv_sock.recvmsg_into(
            [b1, memoryview(b2)[2:9], b3])
        self.assertEqual(nbytes, len(b"Mary had a little lamb"))
        self.assertEqual(b1, bytearray(b"Mary"))
        self.assertEqual(b2, bytearray(b"01 had a 9"))
        self.assertEqual(b3, bytearray(b"little lamb---"))
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True)

    def _testRecvmsgIntoScatter(self):
        self.sendToServer(b"Mary had a little lamb")


class CmsgMacroTests(unittest.TestCase):
    # Test the functions CMSG_LEN() and CMSG_SPACE().  Tests
    # assumptions used by sendmsg() and recvmsg[_into](), which share
    # code with these functions.

    # Match the definition in socketmodule.c
    try:
        import _testcapi
    except ImportError:
        socklen_t_limit = 0x7fffffff
    else:
        socklen_t_limit = min(0x7fffffff, _testcapi.INT_MAX)

    @requireAttrs(socket, "CMSG_LEN")
    def testCMSG_LEN(self):
        # Test CMSG_LEN() with various valid and invalid values,
        # checking the assumptions used by recvmsg() and sendmsg().
        toobig = self.socklen_t_limit - socket.CMSG_LEN(0) + 1
        values = list(range(257)) + list(range(toobig - 257, toobig))

        # struct cmsghdr has at least three members, two of which are ints
        self.assertGreater(socket.CMSG_LEN(0), array.array("i").itemsize * 2)
        for n in values:
            ret = socket.CMSG_LEN(n)
            # This is how recvmsg() calculates the data size
            self.assertEqual(ret - socket.CMSG_LEN(0), n)
            self.assertLessEqual(ret, self.socklen_t_limit)

        self.assertRaises(OverflowError, socket.CMSG_LEN, -1)
        # sendmsg() shares code with these functions, and requires
        # that it reject values over the limit.
        self.assertRaises(OverflowError, socket.CMSG_LEN, toobig)
        self.assertRaises(OverflowError, socket.CMSG_LEN, sys.maxsize)

    @requireAttrs(socket, "CMSG_SPACE")
    def testCMSG_SPACE(self):
        # Test CMSG_SPACE() with various valid and invalid values,
        # checking the assumptions used by sendmsg().
        toobig = self.socklen_t_limit - socket.CMSG_SPACE(1) + 1
        values = list(range(257)) + list(range(toobig - 257, toobig))

        last = socket.CMSG_SPACE(0)
        # struct cmsghdr has at least three members, two of which are ints
        self.assertGreater(last, array.array("i").itemsize * 2)
        for n in values:
            ret = socket.CMSG_SPACE(n)
            self.assertGreaterEqual(ret, last)
            self.assertGreaterEqual(ret, socket.CMSG_LEN(n))
            self.assertGreaterEqual(ret, n + socket.CMSG_LEN(0))
            self.assertLessEqual(ret, self.socklen_t_limit)
            last = ret

        self.assertRaises(OverflowError, socket.CMSG_SPACE, -1)
        # sendmsg() shares code with these functions, and requires
        # that it reject values over the limit.
        self.assertRaises(OverflowError, socket.CMSG_SPACE, toobig)
        self.assertRaises(OverflowError, socket.CMSG_SPACE, sys.maxsize)


class SCMRightsTest(SendrecvmsgServerTimeoutBase):
    # Tests for file descriptor passing on Unix-domain sockets.

    # Invalid file descriptor value that's unlikely to evaluate to a
    # real FD even if one of its bytes is replaced with a different
    # value (which shouldn't actually happen).
    badfd = -0x5555

    def newFDs(self, n):
        # Return a list of n file descriptors for newly-created files
        # containing their list indices as ASCII numbers.
        fds = []
        for i in range(n):
            fd, path = tempfile.mkstemp()
            self.addCleanup(os.unlink, path)
            self.addCleanup(os.close, fd)
            os.write(fd, str(i).encode())
            fds.append(fd)
        return fds

    def checkFDs(self, fds):
        # Check that the file descriptors in the given list contain
        # their correct list indices as ASCII numbers.
        for n, fd in enumerate(fds):
            os.lseek(fd, 0, os.SEEK_SET)
            self.assertEqual(os.read(fd, 1024), str(n).encode())

    def registerRecvmsgResult(self, result):
        self.addCleanup(self.closeRecvmsgFDs, result)

    def closeRecvmsgFDs(self, recvmsg_result):
        # Close all file descriptors specified in the ancillary data
        # of the given return value from recvmsg() or recvmsg_into().
        for cmsg_level, cmsg_type, cmsg_data in recvmsg_result[1]:
            if (cmsg_level == socket.SOL_SOCKET and
                    cmsg_type == socket.SCM_RIGHTS):
                fds = array.array("i")
                fds.frombytes(cmsg_data[:
                        len(cmsg_data) - (len(cmsg_data) % fds.itemsize)])
                for fd in fds:
                    os.close(fd)

    def createAndSendFDs(self, n):
        # Send n new file descriptors created by newFDs() to the
        # server, with the constant MSG as the non-ancillary data.
        self.assertEqual(
            self.sendmsgToServer([MSG],
                                 [(socket.SOL_SOCKET,
                                   socket.SCM_RIGHTS,
                                   array.array("i", self.newFDs(n)))]),
            len(MSG))

    def checkRecvmsgFDs(self, numfds, result, maxcmsgs=1, ignoreflags=0):
        # Check that constant MSG was received with numfds file
        # descriptors in a maximum of maxcmsgs control messages (which
        # must contain only complete integers).  By default, check
        # that MSG_CTRUNC is unset, but ignore any flags in
        # ignoreflags.
        msg, ancdata, flags, addr = result
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.checkFlags(flags, eor=True, checkunset=socket.MSG_CTRUNC,
                        ignore=ignoreflags)

        self.assertIsInstance(ancdata, list)
        self.assertLessEqual(len(ancdata), maxcmsgs)
        fds = array.array("i")
        for item in ancdata:
            self.assertIsInstance(item, tuple)
            cmsg_level, cmsg_type, cmsg_data = item
            self.assertEqual(cmsg_level, socket.SOL_SOCKET)
            self.assertEqual(cmsg_type, socket.SCM_RIGHTS)
            self.assertIsInstance(cmsg_data, bytes)
            self.assertEqual(len(cmsg_data) % SIZEOF_INT, 0)
            fds.frombytes(cmsg_data)

        self.assertEqual(len(fds), numfds)
        self.checkFDs(fds)

    def testFDPassSimple(self):
        # Pass a single FD (array read from bytes object).
        self.checkRecvmsgFDs(1, self.doRecvmsg(self.serv_sock,
                                               len(MSG), 10240))

    def _testFDPassSimple(self):
        self.assertEqual(
            self.sendmsgToServer(
                [MSG],
                [(socket.SOL_SOCKET,
                  socket.SCM_RIGHTS,
                  array.array("i", self.newFDs(1)).tobytes())]),
            len(MSG))

    def testMultipleFDPass(self):
        # Pass multiple FDs in a single array.
        self.checkRecvmsgFDs(4, self.doRecvmsg(self.serv_sock,
                                               len(MSG), 10240))

    def _testMultipleFDPass(self):
        self.createAndSendFDs(4)

    @requireAttrs(socket, "CMSG_SPACE")
    def testFDPassCMSG_SPACE(self):
        # Test using CMSG_SPACE() to calculate ancillary buffer size.
        self.checkRecvmsgFDs(
            4, self.doRecvmsg(self.serv_sock, len(MSG),
                              socket.CMSG_SPACE(4 * SIZEOF_INT)))

    @testFDPassCMSG_SPACE.client_skip
    def _testFDPassCMSG_SPACE(self):
        self.createAndSendFDs(4)

    def testFDPassCMSG_LEN(self):
        # Test using CMSG_LEN() to calculate ancillary buffer size.
        self.checkRecvmsgFDs(1,
                             self.doRecvmsg(self.serv_sock, len(MSG),
                                            socket.CMSG_LEN(4 * SIZEOF_INT)),
                             # RFC 3542 says implementations may set
                             # MSG_CTRUNC if there isn't enough space
                             # for trailing padding.
                             ignoreflags=socket.MSG_CTRUNC)

    def _testFDPassCMSG_LEN(self):
        self.createAndSendFDs(1)

    @unittest.skipIf(sys.platform == "darwin", "skipping, see issue #12958")
    @unittest.skipIf(AIX, "skipping, see issue #22397")
    @requireAttrs(socket, "CMSG_SPACE")
    def testFDPassSeparate(self):
        # Pass two FDs in two separate arrays.  Arrays may be combined
        # into a single control message by the OS.
        self.checkRecvmsgFDs(2,
                             self.doRecvmsg(self.serv_sock, len(MSG), 10240),
                             maxcmsgs=2)

    @testFDPassSeparate.client_skip
    @unittest.skipIf(sys.platform == "darwin", "skipping, see issue #12958")
    @unittest.skipIf(AIX, "skipping, see issue #22397")
    def _testFDPassSeparate(self):
        fd0, fd1 = self.newFDs(2)
        self.assertEqual(
            self.sendmsgToServer([MSG], [(socket.SOL_SOCKET,
                                          socket.SCM_RIGHTS,
                                          array.array("i", [fd0])),
                                         (socket.SOL_SOCKET,
                                          socket.SCM_RIGHTS,
                                          array.array("i", [fd1]))]),
            len(MSG))

    @unittest.skipIf(sys.platform == "darwin", "skipping, see issue #12958")
    @unittest.skipIf(AIX, "skipping, see issue #22397")
    @requireAttrs(socket, "CMSG_SPACE")
    def testFDPassSeparateMinSpace(self):
        # Pass two FDs in two separate arrays, receiving them into the
        # minimum space for two arrays.
        num_fds = 2
        self.checkRecvmsgFDs(num_fds,
                             self.doRecvmsg(self.serv_sock, len(MSG),
                                            socket.CMSG_SPACE(SIZEOF_INT) +
                                            socket.CMSG_LEN(SIZEOF_INT * num_fds)),
                             maxcmsgs=2, ignoreflags=socket.MSG_CTRUNC)

    @testFDPassSeparateMinSpace.client_skip
    @unittest.skipIf(sys.platform == "darwin", "skipping, see issue #12958")
    @unittest.skipIf(AIX, "skipping, see issue #22397")
    def _testFDPassSeparateMinSpace(self):
        fd0, fd1 = self.newFDs(2)
        self.assertEqual(
            self.sendmsgToServer([MSG], [(socket.SOL_SOCKET,
                                          socket.SCM_RIGHTS,
                                          array.array("i", [fd0])),
                                         (socket.SOL_SOCKET,
                                          socket.SCM_RIGHTS,
                                          array.array("i", [fd1]))]),
            len(MSG))

    def sendAncillaryIfPossible(self, msg, ancdata):
        # Try to send msg and ancdata to server, but if the system
        # call fails, just send msg with no ancillary data.
        try:
            nbytes = self.sendmsgToServer([msg], ancdata)
        except OSError as e:
            # Check that it was the system call that failed
            self.assertIsInstance(e.errno, int)
            nbytes = self.sendmsgToServer([msg])
        self.assertEqual(nbytes, len(msg))

    @unittest.skipIf(sys.platform == "darwin", "see issue #24725")
    def testFDPassEmpty(self):
        # Try to pass an empty FD array.  Can receive either no array
        # or an empty array.
        self.checkRecvmsgFDs(0, self.doRecvmsg(self.serv_sock,
                                               len(MSG), 10240),
                             ignoreflags=socket.MSG_CTRUNC)

    def _testFDPassEmpty(self):
        self.sendAncillaryIfPossible(MSG, [(socket.SOL_SOCKET,
                                            socket.SCM_RIGHTS,
                                            b"")])

    def testFDPassPartialInt(self):
        # Try to pass a truncated FD array.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), 10240)
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.checkFlags(flags, eor=True, ignore=socket.MSG_CTRUNC)
        self.assertLessEqual(len(ancdata), 1)
        for cmsg_level, cmsg_type, cmsg_data in ancdata:
            self.assertEqual(cmsg_level, socket.SOL_SOCKET)
            self.assertEqual(cmsg_type, socket.SCM_RIGHTS)
            self.assertLess(len(cmsg_data), SIZEOF_INT)

    def _testFDPassPartialInt(self):
        self.sendAncillaryIfPossible(
            MSG,
            [(socket.SOL_SOCKET,
              socket.SCM_RIGHTS,
              array.array("i", [self.badfd]).tobytes()[:-1])])

    @requireAttrs(socket, "CMSG_SPACE")
    def testFDPassPartialIntInMiddle(self):
        # Try to pass two FD arrays, the first of which is truncated.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), 10240)
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.checkFlags(flags, eor=True, ignore=socket.MSG_CTRUNC)
        self.assertLessEqual(len(ancdata), 2)
        fds = array.array("i")
        # Arrays may have been combined in a single control message
        for cmsg_level, cmsg_type, cmsg_data in ancdata:
            self.assertEqual(cmsg_level, socket.SOL_SOCKET)
            self.assertEqual(cmsg_type, socket.SCM_RIGHTS)
            fds.frombytes(cmsg_data[:
                    len(cmsg_data) - (len(cmsg_data) % fds.itemsize)])
        self.assertLessEqual(len(fds), 2)
        self.checkFDs(fds)

    @testFDPassPartialIntInMiddle.client_skip
    def _testFDPassPartialIntInMiddle(self):
        fd0, fd1 = self.newFDs(2)
        self.sendAncillaryIfPossible(
            MSG,
            [(socket.SOL_SOCKET,
              socket.SCM_RIGHTS,
              array.array("i", [fd0, self.badfd]).tobytes()[:-1]),
             (socket.SOL_SOCKET,
              socket.SCM_RIGHTS,
              array.array("i", [fd1]))])

    def checkTruncatedHeader(self, result, ignoreflags=0):
        # Check that no ancillary data items are returned when data is
        # truncated inside the cmsghdr structure.
        msg, ancdata, flags, addr = result
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True, checkset=socket.MSG_CTRUNC,
                        ignore=ignoreflags)

    def testCmsgTruncNoBufSize(self):
        # Check that no ancillary data is received when no buffer size
        # is specified.
        self.checkTruncatedHeader(self.doRecvmsg(self.serv_sock, len(MSG)),
                                  # BSD seems to set MSG_CTRUNC only
                                  # if an item has been partially
                                  # received.
                                  ignoreflags=socket.MSG_CTRUNC)

    def _testCmsgTruncNoBufSize(self):
        self.createAndSendFDs(1)

    def testCmsgTrunc0(self):
        # Check that no ancillary data is received when buffer size is 0.
        self.checkTruncatedHeader(self.doRecvmsg(self.serv_sock, len(MSG), 0),
                                  ignoreflags=socket.MSG_CTRUNC)

    def _testCmsgTrunc0(self):
        self.createAndSendFDs(1)

    # Check that no ancillary data is returned for various non-zero
    # (but still too small) buffer sizes.

    def testCmsgTrunc1(self):
        self.checkTruncatedHeader(self.doRecvmsg(self.serv_sock, len(MSG), 1))

    def _testCmsgTrunc1(self):
        self.createAndSendFDs(1)

    def testCmsgTrunc2Int(self):
        # The cmsghdr structure has at least three members, two of
        # which are ints, so we still shouldn't see any ancillary
        # data.
        self.checkTruncatedHeader(self.doRecvmsg(self.serv_sock, len(MSG),
                                                 SIZEOF_INT * 2))

    def _testCmsgTrunc2Int(self):
        self.createAndSendFDs(1)

    def testCmsgTruncLen0Minus1(self):
        self.checkTruncatedHeader(self.doRecvmsg(self.serv_sock, len(MSG),
                                                 socket.CMSG_LEN(0) - 1))

    def _testCmsgTruncLen0Minus1(self):
        self.createAndSendFDs(1)

    # The following tests try to truncate the control message in the
    # middle of the FD array.

    def checkTruncatedArray(self, ancbuf, maxdata, mindata=0):
        # Check that file descriptor data is truncated to between
        # mindata and maxdata bytes when received with buffer size
        # ancbuf, and that any complete file descriptor numbers are
        # valid.
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), ancbuf)
        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.checkFlags(flags, eor=True, checkset=socket.MSG_CTRUNC)

        if mindata == 0 and ancdata == []:
            return
        self.assertEqual(len(ancdata), 1)
        cmsg_level, cmsg_type, cmsg_data = ancdata[0]
        self.assertEqual(cmsg_level, socket.SOL_SOCKET)
        self.assertEqual(cmsg_type, socket.SCM_RIGHTS)
        self.assertGreaterEqual(len(cmsg_data), mindata)
        self.assertLessEqual(len(cmsg_data), maxdata)
        fds = array.array("i")
        fds.frombytes(cmsg_data[:
                len(cmsg_data) - (len(cmsg_data) % fds.itemsize)])
        self.checkFDs(fds)

    def testCmsgTruncLen0(self):
        self.checkTruncatedArray(ancbuf=socket.CMSG_LEN(0), maxdata=0)

    def _testCmsgTruncLen0(self):
        self.createAndSendFDs(1)

    def testCmsgTruncLen0Plus1(self):
        self.checkTruncatedArray(ancbuf=socket.CMSG_LEN(0) + 1, maxdata=1)

    def _testCmsgTruncLen0Plus1(self):
        self.createAndSendFDs(2)

    def testCmsgTruncLen1(self):
        self.checkTruncatedArray(ancbuf=socket.CMSG_LEN(SIZEOF_INT),
                                 maxdata=SIZEOF_INT)

    def _testCmsgTruncLen1(self):
        self.createAndSendFDs(2)

    def testCmsgTruncLen2Minus1(self):
        self.checkTruncatedArray(ancbuf=socket.CMSG_LEN(2 * SIZEOF_INT) - 1,
                                 maxdata=(2 * SIZEOF_INT) - 1)

    def _testCmsgTruncLen2Minus1(self):
        self.createAndSendFDs(2)


class RFC3542AncillaryTest(SendrecvmsgServerTimeoutBase):
    # Test sendmsg() and recvmsg[_into]() using the ancillary data
    # features of the RFC 3542 Advanced Sockets API for IPv6.
    # Currently we can only handle certain data items (e.g. traffic
    # class, hop limit, MTU discovery and fragmentation settings)
    # without resorting to unportable means such as the struct module,
    # but the tests here are aimed at testing the ancillary data
    # handling in sendmsg() and recvmsg() rather than the IPv6 API
    # itself.

    # Test value to use when setting hop limit of packet
    hop_limit = 2

    # Test value to use when setting traffic class of packet.
    # -1 means "use kernel default".
    traffic_class = -1

    def ancillaryMapping(self, ancdata):
        # Given ancillary data list ancdata, return a mapping from
        # pairs (cmsg_level, cmsg_type) to corresponding cmsg_data.
        # Check that no (level, type) pair appears more than once.
        d = {}
        for cmsg_level, cmsg_type, cmsg_data in ancdata:
            self.assertNotIn((cmsg_level, cmsg_type), d)
            d[(cmsg_level, cmsg_type)] = cmsg_data
        return d

    def checkHopLimit(self, ancbufsize, maxhop=255, ignoreflags=0):
        # Receive hop limit into ancbufsize bytes of ancillary data
        # space.  Check that data is MSG, ancillary data is not
        # truncated (but ignore any flags in ignoreflags), and hop
        # limit is between 0 and maxhop inclusive.
        self.serv_sock.setsockopt(socket.IPPROTO_IPV6,
                                  socket.IPV6_RECVHOPLIMIT, 1)
        self.misc_event.set()
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), ancbufsize)

        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.checkFlags(flags, eor=True, checkunset=socket.MSG_CTRUNC,
                        ignore=ignoreflags)

        self.assertEqual(len(ancdata), 1)
        self.assertIsInstance(ancdata[0], tuple)
        cmsg_level, cmsg_type, cmsg_data = ancdata[0]
        self.assertEqual(cmsg_level, socket.IPPROTO_IPV6)
        self.assertEqual(cmsg_type, socket.IPV6_HOPLIMIT)
        self.assertIsInstance(cmsg_data, bytes)
        self.assertEqual(len(cmsg_data), SIZEOF_INT)
        a = array.array("i")
        a.frombytes(cmsg_data)
        self.assertGreaterEqual(a[0], 0)
        self.assertLessEqual(a[0], maxhop)

    @requireAttrs(socket, "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT")
    def testRecvHopLimit(self):
        # Test receiving the packet hop limit as ancillary data.
        self.checkHopLimit(ancbufsize=10240)

    @testRecvHopLimit.client_skip
    def _testRecvHopLimit(self):
        # Need to wait until server has asked to receive ancillary
        # data, as implementations are not required to buffer it
        # otherwise.
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "CMSG_SPACE", "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT")
    def testRecvHopLimitCMSG_SPACE(self):
        # Test receiving hop limit, using CMSG_SPACE to calculate buffer size.
        self.checkHopLimit(ancbufsize=socket.CMSG_SPACE(SIZEOF_INT))

    @testRecvHopLimitCMSG_SPACE.client_skip
    def _testRecvHopLimitCMSG_SPACE(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    # Could test receiving into buffer sized using CMSG_LEN, but RFC
    # 3542 says portable applications must provide space for trailing
    # padding.  Implementations may set MSG_CTRUNC if there isn't
    # enough space for the padding.

    @requireAttrs(socket.socket, "sendmsg")
    @requireAttrs(socket, "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT")
    def testSetHopLimit(self):
        # Test setting hop limit on outgoing packet and receiving it
        # at the other end.
        self.checkHopLimit(ancbufsize=10240, maxhop=self.hop_limit)

    @testSetHopLimit.client_skip
    def _testSetHopLimit(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.assertEqual(
            self.sendmsgToServer([MSG],
                                 [(socket.IPPROTO_IPV6, socket.IPV6_HOPLIMIT,
                                   array.array("i", [self.hop_limit]))]),
            len(MSG))

    def checkTrafficClassAndHopLimit(self, ancbufsize, maxhop=255,
                                     ignoreflags=0):
        # Receive traffic class and hop limit into ancbufsize bytes of
        # ancillary data space.  Check that data is MSG, ancillary
        # data is not truncated (but ignore any flags in ignoreflags),
        # and traffic class and hop limit are in range (hop limit no
        # more than maxhop).
        self.serv_sock.setsockopt(socket.IPPROTO_IPV6,
                                  socket.IPV6_RECVHOPLIMIT, 1)
        self.serv_sock.setsockopt(socket.IPPROTO_IPV6,
                                  socket.IPV6_RECVTCLASS, 1)
        self.misc_event.set()
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), ancbufsize)

        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.checkFlags(flags, eor=True, checkunset=socket.MSG_CTRUNC,
                        ignore=ignoreflags)
        self.assertEqual(len(ancdata), 2)
        ancmap = self.ancillaryMapping(ancdata)

        tcdata = ancmap[(socket.IPPROTO_IPV6, socket.IPV6_TCLASS)]
        self.assertEqual(len(tcdata), SIZEOF_INT)
        a = array.array("i")
        a.frombytes(tcdata)
        self.assertGreaterEqual(a[0], 0)
        self.assertLessEqual(a[0], 255)

        hldata = ancmap[(socket.IPPROTO_IPV6, socket.IPV6_HOPLIMIT)]
        self.assertEqual(len(hldata), SIZEOF_INT)
        a = array.array("i")
        a.frombytes(hldata)
        self.assertGreaterEqual(a[0], 0)
        self.assertLessEqual(a[0], maxhop)

    @requireAttrs(socket, "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT",
                  "IPV6_RECVTCLASS", "IPV6_TCLASS")
    def testRecvTrafficClassAndHopLimit(self):
        # Test receiving traffic class and hop limit as ancillary data.
        self.checkTrafficClassAndHopLimit(ancbufsize=10240)

    @testRecvTrafficClassAndHopLimit.client_skip
    def _testRecvTrafficClassAndHopLimit(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "CMSG_SPACE", "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT",
                  "IPV6_RECVTCLASS", "IPV6_TCLASS")
    def testRecvTrafficClassAndHopLimitCMSG_SPACE(self):
        # Test receiving traffic class and hop limit, using
        # CMSG_SPACE() to calculate buffer size.
        self.checkTrafficClassAndHopLimit(
            ancbufsize=socket.CMSG_SPACE(SIZEOF_INT) * 2)

    @testRecvTrafficClassAndHopLimitCMSG_SPACE.client_skip
    def _testRecvTrafficClassAndHopLimitCMSG_SPACE(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket.socket, "sendmsg")
    @requireAttrs(socket, "CMSG_SPACE", "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT",
                  "IPV6_RECVTCLASS", "IPV6_TCLASS")
    def testSetTrafficClassAndHopLimit(self):
        # Test setting traffic class and hop limit on outgoing packet,
        # and receiving them at the other end.
        self.checkTrafficClassAndHopLimit(ancbufsize=10240,
                                          maxhop=self.hop_limit)

    @testSetTrafficClassAndHopLimit.client_skip
    def _testSetTrafficClassAndHopLimit(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.assertEqual(
            self.sendmsgToServer([MSG],
                                 [(socket.IPPROTO_IPV6, socket.IPV6_TCLASS,
                                   array.array("i", [self.traffic_class])),
                                  (socket.IPPROTO_IPV6, socket.IPV6_HOPLIMIT,
                                   array.array("i", [self.hop_limit]))]),
            len(MSG))

    @requireAttrs(socket.socket, "sendmsg")
    @requireAttrs(socket, "CMSG_SPACE", "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT",
                  "IPV6_RECVTCLASS", "IPV6_TCLASS")
    def testOddCmsgSize(self):
        # Try to send ancillary data with first item one byte too
        # long.  Fall back to sending with correct size if this fails,
        # and check that second item was handled correctly.
        self.checkTrafficClassAndHopLimit(ancbufsize=10240,
                                          maxhop=self.hop_limit)

    @testOddCmsgSize.client_skip
    def _testOddCmsgSize(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        try:
            nbytes = self.sendmsgToServer(
                [MSG],
                [(socket.IPPROTO_IPV6, socket.IPV6_TCLASS,
                  array.array("i", [self.traffic_class]).tobytes() + b"\x00"),
                 (socket.IPPROTO_IPV6, socket.IPV6_HOPLIMIT,
                  array.array("i", [self.hop_limit]))])
        except OSError as e:
            self.assertIsInstance(e.errno, int)
            nbytes = self.sendmsgToServer(
                [MSG],
                [(socket.IPPROTO_IPV6, socket.IPV6_TCLASS,
                  array.array("i", [self.traffic_class])),
                 (socket.IPPROTO_IPV6, socket.IPV6_HOPLIMIT,
                  array.array("i", [self.hop_limit]))])
            self.assertEqual(nbytes, len(MSG))

    # Tests for proper handling of truncated ancillary data

    def checkHopLimitTruncatedHeader(self, ancbufsize, ignoreflags=0):
        # Receive hop limit into ancbufsize bytes of ancillary data
        # space, which should be too small to contain the ancillary
        # data header (if ancbufsize is None, pass no second argument
        # to recvmsg()).  Check that data is MSG, MSG_CTRUNC is set
        # (unless included in ignoreflags), and no ancillary data is
        # returned.
        self.serv_sock.setsockopt(socket.IPPROTO_IPV6,
                                  socket.IPV6_RECVHOPLIMIT, 1)
        self.misc_event.set()
        args = () if ancbufsize is None else (ancbufsize,)
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), *args)

        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.assertEqual(ancdata, [])
        self.checkFlags(flags, eor=True, checkset=socket.MSG_CTRUNC,
                        ignore=ignoreflags)

    @requireAttrs(socket, "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT")
    def testCmsgTruncNoBufSize(self):
        # Check that no ancillary data is received when no ancillary
        # buffer size is provided.
        self.checkHopLimitTruncatedHeader(ancbufsize=None,
                                          # BSD seems to set
                                          # MSG_CTRUNC only if an item
                                          # has been partially
                                          # received.
                                          ignoreflags=socket.MSG_CTRUNC)

    @testCmsgTruncNoBufSize.client_skip
    def _testCmsgTruncNoBufSize(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT")
    def testSingleCmsgTrunc0(self):
        # Check that no ancillary data is received when ancillary
        # buffer size is zero.
        self.checkHopLimitTruncatedHeader(ancbufsize=0,
                                          ignoreflags=socket.MSG_CTRUNC)

    @testSingleCmsgTrunc0.client_skip
    def _testSingleCmsgTrunc0(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    # Check that no ancillary data is returned for various non-zero
    # (but still too small) buffer sizes.

    @requireAttrs(socket, "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT")
    def testSingleCmsgTrunc1(self):
        self.checkHopLimitTruncatedHeader(ancbufsize=1)

    @testSingleCmsgTrunc1.client_skip
    def _testSingleCmsgTrunc1(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT")
    def testSingleCmsgTrunc2Int(self):
        self.checkHopLimitTruncatedHeader(ancbufsize=2 * SIZEOF_INT)

    @testSingleCmsgTrunc2Int.client_skip
    def _testSingleCmsgTrunc2Int(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT")
    def testSingleCmsgTruncLen0Minus1(self):
        self.checkHopLimitTruncatedHeader(ancbufsize=socket.CMSG_LEN(0) - 1)

    @testSingleCmsgTruncLen0Minus1.client_skip
    def _testSingleCmsgTruncLen0Minus1(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT")
    def testSingleCmsgTruncInData(self):
        # Test truncation of a control message inside its associated
        # data.  The message may be returned with its data truncated,
        # or not returned at all.
        self.serv_sock.setsockopt(socket.IPPROTO_IPV6,
                                  socket.IPV6_RECVHOPLIMIT, 1)
        self.misc_event.set()
        msg, ancdata, flags, addr = self.doRecvmsg(
            self.serv_sock, len(MSG), socket.CMSG_LEN(SIZEOF_INT) - 1)

        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.checkFlags(flags, eor=True, checkset=socket.MSG_CTRUNC)

        self.assertLessEqual(len(ancdata), 1)
        if ancdata:
            cmsg_level, cmsg_type, cmsg_data = ancdata[0]
            self.assertEqual(cmsg_level, socket.IPPROTO_IPV6)
            self.assertEqual(cmsg_type, socket.IPV6_HOPLIMIT)
            self.assertLess(len(cmsg_data), SIZEOF_INT)

    @testSingleCmsgTruncInData.client_skip
    def _testSingleCmsgTruncInData(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    def checkTruncatedSecondHeader(self, ancbufsize, ignoreflags=0):
        # Receive traffic class and hop limit into ancbufsize bytes of
        # ancillary data space, which should be large enough to
        # contain the first item, but too small to contain the header
        # of the second.  Check that data is MSG, MSG_CTRUNC is set
        # (unless included in ignoreflags), and only one ancillary
        # data item is returned.
        self.serv_sock.setsockopt(socket.IPPROTO_IPV6,
                                  socket.IPV6_RECVHOPLIMIT, 1)
        self.serv_sock.setsockopt(socket.IPPROTO_IPV6,
                                  socket.IPV6_RECVTCLASS, 1)
        self.misc_event.set()
        msg, ancdata, flags, addr = self.doRecvmsg(self.serv_sock,
                                                   len(MSG), ancbufsize)

        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.checkFlags(flags, eor=True, checkset=socket.MSG_CTRUNC,
                        ignore=ignoreflags)

        self.assertEqual(len(ancdata), 1)
        cmsg_level, cmsg_type, cmsg_data = ancdata[0]
        self.assertEqual(cmsg_level, socket.IPPROTO_IPV6)
        self.assertIn(cmsg_type, {socket.IPV6_TCLASS, socket.IPV6_HOPLIMIT})
        self.assertEqual(len(cmsg_data), SIZEOF_INT)
        a = array.array("i")
        a.frombytes(cmsg_data)
        self.assertGreaterEqual(a[0], 0)
        self.assertLessEqual(a[0], 255)

    # Try the above test with various buffer sizes.

    @requireAttrs(socket, "CMSG_SPACE", "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT",
                  "IPV6_RECVTCLASS", "IPV6_TCLASS")
    def testSecondCmsgTrunc0(self):
        self.checkTruncatedSecondHeader(socket.CMSG_SPACE(SIZEOF_INT),
                                        ignoreflags=socket.MSG_CTRUNC)

    @testSecondCmsgTrunc0.client_skip
    def _testSecondCmsgTrunc0(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "CMSG_SPACE", "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT",
                  "IPV6_RECVTCLASS", "IPV6_TCLASS")
    def testSecondCmsgTrunc1(self):
        self.checkTruncatedSecondHeader(socket.CMSG_SPACE(SIZEOF_INT) + 1)

    @testSecondCmsgTrunc1.client_skip
    def _testSecondCmsgTrunc1(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "CMSG_SPACE", "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT",
                  "IPV6_RECVTCLASS", "IPV6_TCLASS")
    def testSecondCmsgTrunc2Int(self):
        self.checkTruncatedSecondHeader(socket.CMSG_SPACE(SIZEOF_INT) +
                                        2 * SIZEOF_INT)

    @testSecondCmsgTrunc2Int.client_skip
    def _testSecondCmsgTrunc2Int(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "CMSG_SPACE", "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT",
                  "IPV6_RECVTCLASS", "IPV6_TCLASS")
    def testSecondCmsgTruncLen0Minus1(self):
        self.checkTruncatedSecondHeader(socket.CMSG_SPACE(SIZEOF_INT) +
                                        socket.CMSG_LEN(0) - 1)

    @testSecondCmsgTruncLen0Minus1.client_skip
    def _testSecondCmsgTruncLen0Minus1(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)

    @requireAttrs(socket, "CMSG_SPACE", "IPV6_RECVHOPLIMIT", "IPV6_HOPLIMIT",
                  "IPV6_RECVTCLASS", "IPV6_TCLASS")
    def testSecondCmsgTruncInData(self):
        # Test truncation of the second of two control messages inside
        # its associated data.
        self.serv_sock.setsockopt(socket.IPPROTO_IPV6,
                                  socket.IPV6_RECVHOPLIMIT, 1)
        self.serv_sock.setsockopt(socket.IPPROTO_IPV6,
                                  socket.IPV6_RECVTCLASS, 1)
        self.misc_event.set()
        msg, ancdata, flags, addr = self.doRecvmsg(
            self.serv_sock, len(MSG),
            socket.CMSG_SPACE(SIZEOF_INT) + socket.CMSG_LEN(SIZEOF_INT) - 1)

        self.assertEqual(msg, MSG)
        self.checkRecvmsgAddress(addr, self.cli_addr)
        self.checkFlags(flags, eor=True, checkset=socket.MSG_CTRUNC)

        cmsg_types = {socket.IPV6_TCLASS, socket.IPV6_HOPLIMIT}

        cmsg_level, cmsg_type, cmsg_data = ancdata.pop(0)
        self.assertEqual(cmsg_level, socket.IPPROTO_IPV6)
        cmsg_types.remove(cmsg_type)
        self.assertEqual(len(cmsg_data), SIZEOF_INT)
        a = array.array("i")
        a.frombytes(cmsg_data)
        self.assertGreaterEqual(a[0], 0)
        self.assertLessEqual(a[0], 255)

        if ancdata:
            cmsg_level, cmsg_type, cmsg_data = ancdata.pop(0)
            self.assertEqual(cmsg_level, socket.IPPROTO_IPV6)
            cmsg_types.remove(cmsg_type)
            self.assertLess(len(cmsg_data), SIZEOF_INT)

        self.assertEqual(ancdata, [])

    @testSecondCmsgTruncInData.client_skip
    def _testSecondCmsgTruncInData(self):
        self.assertTrue(self.misc_event.wait(timeout=self.fail_timeout))
        self.sendToServer(MSG)


# Derive concrete test classes for different socket types.

class SendrecvmsgUDPTestBase(SendrecvmsgDgramFlagsBase,
                             SendrecvmsgConnectionlessBase,
                             ThreadedSocketTestMixin, UDPTestBase):
    pass

@requireAttrs(socket.socket, "sendmsg")
class SendmsgUDPTest(SendmsgConnectionlessTests, SendrecvmsgUDPTestBase):
    pass

@requireAttrs(socket.socket, "recvmsg")
class RecvmsgUDPTest(RecvmsgTests, SendrecvmsgUDPTestBase):
    pass

@requireAttrs(socket.socket, "recvmsg_into")
class RecvmsgIntoUDPTest(RecvmsgIntoTests, SendrecvmsgUDPTestBase):
    pass


class SendrecvmsgUDP6TestBase(SendrecvmsgDgramFlagsBase,
                              SendrecvmsgConnectionlessBase,
                              ThreadedSocketTestMixin, UDP6TestBase):

    def checkRecvmsgAddress(self, addr1, addr2):
        # Called to compare the received address with the address of
        # the peer, ignoring scope ID
        self.assertEqual(addr1[:-1], addr2[:-1])

@requireAttrs(socket.socket, "sendmsg")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@requireSocket("AF_INET6", "SOCK_DGRAM")
class SendmsgUDP6Test(SendmsgConnectionlessTests, SendrecvmsgUDP6TestBase):
    pass

@requireAttrs(socket.socket, "recvmsg")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@requireSocket("AF_INET6", "SOCK_DGRAM")
class RecvmsgUDP6Test(RecvmsgTests, SendrecvmsgUDP6TestBase):
    pass

@requireAttrs(socket.socket, "recvmsg_into")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@requireSocket("AF_INET6", "SOCK_DGRAM")
class RecvmsgIntoUDP6Test(RecvmsgIntoTests, SendrecvmsgUDP6TestBase):
    pass

@requireAttrs(socket.socket, "recvmsg")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@requireAttrs(socket, "IPPROTO_IPV6")
@requireSocket("AF_INET6", "SOCK_DGRAM")
class RecvmsgRFC3542AncillaryUDP6Test(RFC3542AncillaryTest,
                                      SendrecvmsgUDP6TestBase):
    pass

@requireAttrs(socket.socket, "recvmsg_into")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@requireAttrs(socket, "IPPROTO_IPV6")
@requireSocket("AF_INET6", "SOCK_DGRAM")
class RecvmsgIntoRFC3542AncillaryUDP6Test(RecvmsgIntoMixin,
                                          RFC3542AncillaryTest,
                                          SendrecvmsgUDP6TestBase):
    pass


@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
class SendrecvmsgUDPLITETestBase(SendrecvmsgDgramFlagsBase,
                             SendrecvmsgConnectionlessBase,
                             ThreadedSocketTestMixin, UDPLITETestBase):
    pass

@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
@requireAttrs(socket.socket, "sendmsg")
class SendmsgUDPLITETest(SendmsgConnectionlessTests, SendrecvmsgUDPLITETestBase):
    pass

@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
@requireAttrs(socket.socket, "recvmsg")
class RecvmsgUDPLITETest(RecvmsgTests, SendrecvmsgUDPLITETestBase):
    pass

@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
@requireAttrs(socket.socket, "recvmsg_into")
class RecvmsgIntoUDPLITETest(RecvmsgIntoTests, SendrecvmsgUDPLITETestBase):
    pass


@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
class SendrecvmsgUDPLITE6TestBase(SendrecvmsgDgramFlagsBase,
                              SendrecvmsgConnectionlessBase,
                              ThreadedSocketTestMixin, UDPLITE6TestBase):

    def checkRecvmsgAddress(self, addr1, addr2):
        # Called to compare the received address with the address of
        # the peer, ignoring scope ID
        self.assertEqual(addr1[:-1], addr2[:-1])

@requireAttrs(socket.socket, "sendmsg")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
@requireSocket("AF_INET6", "SOCK_DGRAM")
class SendmsgUDPLITE6Test(SendmsgConnectionlessTests, SendrecvmsgUDPLITE6TestBase):
    pass

@requireAttrs(socket.socket, "recvmsg")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
@requireSocket("AF_INET6", "SOCK_DGRAM")
class RecvmsgUDPLITE6Test(RecvmsgTests, SendrecvmsgUDPLITE6TestBase):
    pass

@requireAttrs(socket.socket, "recvmsg_into")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
@requireSocket("AF_INET6", "SOCK_DGRAM")
class RecvmsgIntoUDPLITE6Test(RecvmsgIntoTests, SendrecvmsgUDPLITE6TestBase):
    pass

@requireAttrs(socket.socket, "recvmsg")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
@requireAttrs(socket, "IPPROTO_IPV6")
@requireSocket("AF_INET6", "SOCK_DGRAM")
class RecvmsgRFC3542AncillaryUDPLITE6Test(RFC3542AncillaryTest,
                                      SendrecvmsgUDPLITE6TestBase):
    pass

@requireAttrs(socket.socket, "recvmsg_into")
@unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test.')
@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
@requireAttrs(socket, "IPPROTO_IPV6")
@requireSocket("AF_INET6", "SOCK_DGRAM")
class RecvmsgIntoRFC3542AncillaryUDPLITE6Test(RecvmsgIntoMixin,
                                          RFC3542AncillaryTest,
                                          SendrecvmsgUDPLITE6TestBase):
    pass


class SendrecvmsgTCPTestBase(SendrecvmsgConnectedBase,
                             ConnectedStreamTestMixin, TCPTestBase):
    pass

@requireAttrs(socket.socket, "sendmsg")
class SendmsgTCPTest(SendmsgStreamTests, SendrecvmsgTCPTestBase):
    pass

@requireAttrs(socket.socket, "recvmsg")
class RecvmsgTCPTest(RecvmsgTests, RecvmsgGenericStreamTests,
                     SendrecvmsgTCPTestBase):
    pass

@requireAttrs(socket.socket, "recvmsg_into")
class RecvmsgIntoTCPTest(RecvmsgIntoTests, RecvmsgGenericStreamTests,
                         SendrecvmsgTCPTestBase):
    pass


class SendrecvmsgSCTPStreamTestBase(SendrecvmsgSCTPFlagsBase,
                                    SendrecvmsgConnectedBase,
                                    ConnectedStreamTestMixin, SCTPStreamBase):
    pass

@requireAttrs(socket.socket, "sendmsg")
@unittest.skipIf(AIX, "IPPROTO_SCTP: [Errno 62] Protocol not supported on AIX")
@requireSocket("AF_INET", "SOCK_STREAM", "IPPROTO_SCTP")
class SendmsgSCTPStreamTest(SendmsgStreamTests, SendrecvmsgSCTPStreamTestBase):
    pass

@requireAttrs(socket.socket, "recvmsg")
@unittest.skipIf(AIX, "IPPROTO_SCTP: [Errno 62] Protocol not supported on AIX")
@requireSocket("AF_INET", "SOCK_STREAM", "IPPROTO_SCTP")
class RecvmsgSCTPStreamTest(RecvmsgTests, RecvmsgGenericStreamTests,
                            SendrecvmsgSCTPStreamTestBase):

    def testRecvmsgEOF(self):
        try:
            super(RecvmsgSCTPStreamTest, self).testRecvmsgEOF()
        except OSError as e:
            if e.errno != errno.ENOTCONN:
                raise
            self.skipTest("sporadic ENOTCONN (kernel issue?) - see issue #13876")

@requireAttrs(socket.socket, "recvmsg_into")
@unittest.skipIf(AIX, "IPPROTO_SCTP: [Errno 62] Protocol not supported on AIX")
@requireSocket("AF_INET", "SOCK_STREAM", "IPPROTO_SCTP")
class RecvmsgIntoSCTPStreamTest(RecvmsgIntoTests, RecvmsgGenericStreamTests,
                                SendrecvmsgSCTPStreamTestBase):

    def testRecvmsgEOF(self):
        try:
            super(RecvmsgIntoSCTPStreamTest, self).testRecvmsgEOF()
        except OSError as e:
            if e.errno != errno.ENOTCONN:
                raise
            self.skipTest("sporadic ENOTCONN (kernel issue?) - see issue #13876")


class SendrecvmsgUnixStreamTestBase(SendrecvmsgConnectedBase,
                                    ConnectedStreamTestMixin, UnixStreamBase):
    pass

@requireAttrs(socket.socket, "sendmsg")
@requireAttrs(socket, "AF_UNIX")
class SendmsgUnixStreamTest(SendmsgStreamTests, SendrecvmsgUnixStreamTestBase):
    pass

@requireAttrs(socket.socket, "recvmsg")
@requireAttrs(socket, "AF_UNIX")
class RecvmsgUnixStreamTest(RecvmsgTests, RecvmsgGenericStreamTests,
                            SendrecvmsgUnixStreamTestBase):
    pass

@requireAttrs(socket.socket, "recvmsg_into")
@requireAttrs(socket, "AF_UNIX")
class RecvmsgIntoUnixStreamTest(RecvmsgIntoTests, RecvmsgGenericStreamTests,
                                SendrecvmsgUnixStreamTestBase):
    pass

@requireAttrs(socket.socket, "sendmsg", "recvmsg")
@requireAttrs(socket, "AF_UNIX", "SOL_SOCKET", "SCM_RIGHTS")
class RecvmsgSCMRightsStreamTest(SCMRightsTest, SendrecvmsgUnixStreamTestBase):
    pass

@requireAttrs(socket.socket, "sendmsg", "recvmsg_into")
@requireAttrs(socket, "AF_UNIX", "SOL_SOCKET", "SCM_RIGHTS")
class RecvmsgIntoSCMRightsStreamTest(RecvmsgIntoMixin, SCMRightsTest,
                                     SendrecvmsgUnixStreamTestBase):
    pass


# Test interrupting the interruptible send/receive methods with a
# signal when a timeout is set.  These tests avoid having multiple
# threads alive during the test so that the OS cannot deliver the
# signal to the wrong one.

class InterruptedTimeoutBase(unittest.TestCase):
    # Base class for interrupted send/receive tests.  Installs an
    # empty handler for SIGALRM and removes it on teardown, along with
    # any scheduled alarms.

    def setUp(self):
        super().setUp()
        orig_alrm_handler = signal.signal(signal.SIGALRM,
                                          lambda signum, frame: 1 / 0)
        self.addCleanup(signal.signal, signal.SIGALRM, orig_alrm_handler)

    # Timeout for socket operations
    timeout = support.LOOPBACK_TIMEOUT

    # Provide setAlarm() method to schedule delivery of SIGALRM after
    # given number of seconds, or cancel it if zero, and an
    # appropriate time value to use.  Use setitimer() if available.
    if hasattr(signal, "setitimer"):
        alarm_time = 0.05

        def setAlarm(self, seconds):
            signal.setitimer(signal.ITIMER_REAL, seconds)
    else:
        # Old systems may deliver the alarm up to one second early
        alarm_time = 2

        def setAlarm(self, seconds):
            signal.alarm(seconds)


# Require siginterrupt() in order to ensure that system calls are
# interrupted by default.
@requireAttrs(signal, "siginterrupt")
@unittest.skipUnless(hasattr(signal, "alarm") or hasattr(signal, "setitimer"),
                     "Don't have signal.alarm or signal.setitimer")
class InterruptedRecvTimeoutTest(InterruptedTimeoutBase, UDPTestBase):
    # Test interrupting the recv*() methods with signals when a
    # timeout is set.

    def setUp(self):
        super().setUp()
        self.serv.settimeout(self.timeout)

    def checkInterruptedRecv(self, func, *args, **kwargs):
        # Check that func(*args, **kwargs) raises
        # errno of EINTR when interrupted by a signal.
        try:
            self.setAlarm(self.alarm_time)
            with self.assertRaises(ZeroDivisionError) as cm:
                func(*args, **kwargs)
        finally:
            self.setAlarm(0)

    def testInterruptedRecvTimeout(self):
        self.checkInterruptedRecv(self.serv.recv, 1024)

    def testInterruptedRecvIntoTimeout(self):
        self.checkInterruptedRecv(self.serv.recv_into, bytearray(1024))

    def testInterruptedRecvfromTimeout(self):
        self.checkInterruptedRecv(self.serv.recvfrom, 1024)

    def testInterruptedRecvfromIntoTimeout(self):
        self.checkInterruptedRecv(self.serv.recvfrom_into, bytearray(1024))

    @requireAttrs(socket.socket, "recvmsg")
    def testInterruptedRecvmsgTimeout(self):
        self.checkInterruptedRecv(self.serv.recvmsg, 1024)

    @requireAttrs(socket.socket, "recvmsg_into")
    def testInterruptedRecvmsgIntoTimeout(self):
        self.checkInterruptedRecv(self.serv.recvmsg_into, [bytearray(1024)])


# Require siginterrupt() in order to ensure that system calls are
# interrupted by default.
@requireAttrs(signal, "siginterrupt")
@unittest.skipUnless(hasattr(signal, "alarm") or hasattr(signal, "setitimer"),
                     "Don't have signal.alarm or signal.setitimer")
class InterruptedSendTimeoutTest(InterruptedTimeoutBase,
                                 ThreadSafeCleanupTestCase,
                                 SocketListeningTestMixin, TCPTestBase):
    # Test interrupting the interruptible send*() methods with signals
    # when a timeout is set.

    def setUp(self):
        super().setUp()
        self.serv_conn = self.newSocket()
        self.addCleanup(self.serv_conn.close)
        # Use a thread to complete the connection, but wait for it to
        # terminate before running the test, so that there is only one
        # thread to accept the signal.
        cli_thread = threading.Thread(target=self.doConnect)
        cli_thread.start()
        self.cli_conn, addr = self.serv.accept()
        self.addCleanup(self.cli_conn.close)
        cli_thread.join()
        self.serv_conn.settimeout(self.timeout)

    def doConnect(self):
        self.serv_conn.connect(self.serv_addr)

    def checkInterruptedSend(self, func, *args, **kwargs):
        # Check that func(*args, **kwargs), run in a loop, raises
        # OSError with an errno of EINTR when interrupted by a
        # signal.
        try:
            with self.assertRaises(ZeroDivisionError) as cm:
                while True:
                    self.setAlarm(self.alarm_time)
                    func(*args, **kwargs)
        finally:
            self.setAlarm(0)

    # Issue #12958: The following tests have problems on OS X prior to 10.7
    @support.requires_mac_ver(10, 7)
    def testInterruptedSendTimeout(self):
        self.checkInterruptedSend(self.serv_conn.send, b"a"*512)

    @support.requires_mac_ver(10, 7)
    def testInterruptedSendtoTimeout(self):
        # Passing an actual address here as Python's wrapper for
        # sendto() doesn't allow passing a zero-length one; POSIX
        # requires that the address is ignored since the socket is
        # connection-mode, however.
        self.checkInterruptedSend(self.serv_conn.sendto, b"a"*512,
                                  self.serv_addr)

    @support.requires_mac_ver(10, 7)
    @requireAttrs(socket.socket, "sendmsg")
    def testInterruptedSendmsgTimeout(self):
        self.checkInterruptedSend(self.serv_conn.sendmsg, [b"a"*512])


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


class NonBlockingTCPTests(ThreadedTCPSocketTest):

    def __init__(self, methodName='runTest'):
        self.event = threading.Event()
        ThreadedTCPSocketTest.__init__(self, methodName=methodName)

    def assert_sock_timeout(self, sock, timeout):
        self.assertEqual(self.serv.gettimeout(), timeout)

        blocking = (timeout != 0.0)
        self.assertEqual(sock.getblocking(), blocking)

        if fcntl is not None:
            # When a Python socket has a non-zero timeout, it's switched
            # internally to a non-blocking mode. Later, sock.sendall(),
            # sock.recv(), and other socket operations use a select() call and
            # handle EWOULDBLOCK/EGAIN on all socket operations. That's how
            # timeouts are enforced.
            fd_blocking = (timeout is None)

            flag = fcntl.fcntl(sock, fcntl.F_GETFL, os.O_NONBLOCK)
            self.assertEqual(not bool(flag & os.O_NONBLOCK), fd_blocking)

    def testSetBlocking(self):
        # Test setblocking() and settimeout() methods
        self.serv.setblocking(True)
        self.assert_sock_timeout(self.serv, None)

        self.serv.setblocking(False)
        self.assert_sock_timeout(self.serv, 0.0)

        self.serv.settimeout(None)
        self.assert_sock_timeout(self.serv, None)

        self.serv.settimeout(0)
        self.assert_sock_timeout(self.serv, 0)

        self.serv.settimeout(10)
        self.assert_sock_timeout(self.serv, 10)

        self.serv.settimeout(0)
        self.assert_sock_timeout(self.serv, 0)

    def _testSetBlocking(self):
        pass

    @support.cpython_only
    def testSetBlocking_overflow(self):
        # Issue 15989
        import _testcapi
        if _testcapi.UINT_MAX >= _testcapi.ULONG_MAX:
            self.skipTest('needs UINT_MAX < ULONG_MAX')

        self.serv.setblocking(False)
        self.assertEqual(self.serv.gettimeout(), 0.0)

        self.serv.setblocking(_testcapi.UINT_MAX + 1)
        self.assertIsNone(self.serv.gettimeout())

    _testSetBlocking_overflow = support.cpython_only(_testSetBlocking)

    @unittest.skipUnless(hasattr(socket, 'SOCK_NONBLOCK'),
                         'test needs socket.SOCK_NONBLOCK')
    @support.requires_linux_version(2, 6, 28)
    def testInitNonBlocking(self):
        # create a socket with SOCK_NONBLOCK
        self.serv.close()
        self.serv = socket.socket(socket.AF_INET,
                                  socket.SOCK_STREAM | socket.SOCK_NONBLOCK)
        self.assert_sock_timeout(self.serv, 0)

    def _testInitNonBlocking(self):
        pass

    def testInheritFlagsBlocking(self):
        # bpo-7995: accept() on a listening socket with a timeout and the
        # default timeout is None, the resulting socket must be blocking.
        with socket_setdefaulttimeout(None):
            self.serv.settimeout(10)
            conn, addr = self.serv.accept()
            self.addCleanup(conn.close)
            self.assertIsNone(conn.gettimeout())

    def _testInheritFlagsBlocking(self):
        self.cli.connect((HOST, self.port))

    def testInheritFlagsTimeout(self):
        # bpo-7995: accept() on a listening socket with a timeout and the
        # default timeout is None, the resulting socket must inherit
        # the default timeout.
        default_timeout = 20.0
        with socket_setdefaulttimeout(default_timeout):
            self.serv.settimeout(10)
            conn, addr = self.serv.accept()
            self.addCleanup(conn.close)
            self.assertEqual(conn.gettimeout(), default_timeout)

    def _testInheritFlagsTimeout(self):
        self.cli.connect((HOST, self.port))

    def testAccept(self):
        # Testing non-blocking accept
        self.serv.setblocking(False)

        # connect() didn't start: non-blocking accept() fails
        start_time = time.monotonic()
        with self.assertRaises(BlockingIOError):
            conn, addr = self.serv.accept()
        dt = time.monotonic() - start_time
        self.assertLess(dt, 1.0)

        self.event.set()

        read, write, err = select.select([self.serv], [], [], support.LONG_TIMEOUT)
        if self.serv not in read:
            self.fail("Error trying to do accept after select.")

        # connect() completed: non-blocking accept() doesn't block
        conn, addr = self.serv.accept()
        self.addCleanup(conn.close)
        self.assertIsNone(conn.gettimeout())

    def _testAccept(self):
        # don't connect before event is set to check
        # that non-blocking accept() raises BlockingIOError
        self.event.wait()

        self.cli.connect((HOST, self.port))

    def testRecv(self):
        # Testing non-blocking recv
        conn, addr = self.serv.accept()
        self.addCleanup(conn.close)
        conn.setblocking(False)

        # the server didn't send data yet: non-blocking recv() fails
        with self.assertRaises(BlockingIOError):
            msg = conn.recv(len(MSG))

        self.event.set()

        read, write, err = select.select([conn], [], [], support.LONG_TIMEOUT)
        if conn not in read:
            self.fail("Error during select call to non-blocking socket.")

        # the server sent data yet: non-blocking recv() doesn't block
        msg = conn.recv(len(MSG))
        self.assertEqual(msg, MSG)

    def _testRecv(self):
        self.cli.connect((HOST, self.port))

        # don't send anything before event is set to check
        # that non-blocking recv() raises BlockingIOError
        self.event.wait()

        # send data: recv() will no longer block
        self.cli.sendall(MSG)


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
        self.assertRaises(TimeoutError, self.read_file.read, 1)
        # Second read is disallowed
        with self.assertRaises(OSError) as ctx:
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
        self.assertRaises(OSError, self.cli_conn.getsockname)

    def _testRealClose(self):
        pass


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
        self.assertRaises(OSError, self.cli_conn.recv, 1024)

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
        # Avoid closing the socket before the server test has finished,
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
        BIG = b"x" * support.SOCK_MAX_SIZE
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
            raise TimeoutError('timed out')

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
        port = socket_helper.find_unused_port()
        cli = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(cli.close)
        with self.assertRaises(OSError) as cm:
            cli.connect((HOST, port))
        self.assertEqual(cm.exception.errno, errno.ECONNREFUSED)

    def test_create_connection(self):
        # Issue #9792: errors raised by create_connection() should have
        # a proper errno attribute.
        port = socket_helper.find_unused_port()
        with self.assertRaises(OSError) as cm:
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
        expected_errnos = socket_helper.get_socket_conn_refused_errs()
        self.assertIn(cm.exception.errno, expected_errnos)

    def test_create_connection_timeout(self):
        # Issue #9792: create_connection() should not recast timeout errors
        # as generic socket errors.
        with self.mocked_socket_module():
            try:
                socket.create_connection((HOST, 1234))
            except TimeoutError:
                pass
            except OSError as exc:
                if socket_helper.IPV6_ENABLED or exc.errno != errno.EAFNOSUPPORT:
                    raise
            else:
                self.fail('TimeoutError not raised')


class NetworkConnectionAttributesTest(SocketTCPTest, ThreadableTest):

    def __init__(self, methodName='runTest'):
        SocketTCPTest.__init__(self, methodName=methodName)
        ThreadableTest.__init__(self)

    def clientSetUp(self):
        self.source_port = socket_helper.find_unused_port()

    def clientTearDown(self):
        self.cli.close()
        self.cli = None
        ThreadableTest.clientTearDown(self)

    def _justAccept(self):
        conn, addr = self.serv.accept()
        conn.close()

    testFamily = _justAccept
    def _testFamily(self):
        self.cli = socket.create_connection((HOST, self.port),
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(self.cli.close)
        self.assertEqual(self.cli.family, 2)

    testSourceAddress = _justAccept
    def _testSourceAddress(self):
        self.cli = socket.create_connection((HOST, self.port),
                            timeout=support.LOOPBACK_TIMEOUT,
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
        self.assertRaises(TimeoutError, lambda: sock.recv(5))


class TCPTimeoutTest(SocketTCPTest):

    def testTCPTimeout(self):
        def raise_timeout(*args, **kwargs):
            self.serv.settimeout(1.0)
            self.serv.accept()
        self.assertRaises(TimeoutError, raise_timeout,
                              "Error generating a timeout exception (TCP)")

    def testTimeoutZero(self):
        ok = False
        try:
            self.serv.settimeout(0.0)
            foo = self.serv.accept()
        except TimeoutError:
            self.fail("caught timeout instead of error (TCP)")
        except OSError:
            ok = True
        except:
            self.fail("caught unexpected exception (TCP)")
        if not ok:
            self.fail("accept() returned success when we did not expect it")

    @unittest.skipUnless(hasattr(signal, 'alarm'),
                         'test needs signal.alarm()')
    def testInterruptedTimeout(self):
        # XXX I don't know how to do this test on MSWindows or any other
        # platform that doesn't support signal.alarm() or os.kill(), though
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
            except TimeoutError:
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
        self.assertRaises(TimeoutError, raise_timeout,
                              "Error generating a timeout exception (UDP)")

    def testTimeoutZero(self):
        ok = False
        try:
            self.serv.settimeout(0.0)
            foo = self.serv.recv(1024)
        except TimeoutError:
            self.fail("caught timeout instead of error (UDP)")
        except OSError:
            ok = True
        except:
            self.fail("caught unexpected exception (UDP)")
        if not ok:
            self.fail("recv() returned success when we did not expect it")

@unittest.skipUnless(HAVE_SOCKET_UDPLITE,
          'UDPLITE sockets required for this test.')
class UDPLITETimeoutTest(SocketUDPLITETest):

    def testUDPLITETimeout(self):
        def raise_timeout(*args, **kwargs):
            self.serv.settimeout(1.0)
            self.serv.recv(1024)
        self.assertRaises(TimeoutError, raise_timeout,
                              "Error generating a timeout exception (UDPLITE)")

    def testTimeoutZero(self):
        ok = False
        try:
            self.serv.settimeout(0.0)
            foo = self.serv.recv(1024)
        except TimeoutError:
            self.fail("caught timeout instead of error (UDPLITE)")
        except OSError:
            ok = True
        except:
            self.fail("caught unexpected exception (UDPLITE)")
        if not ok:
            self.fail("recv() returned success when we did not expect it")

class TestExceptions(unittest.TestCase):

    def testExceptionTree(self):
        self.assertTrue(issubclass(OSError, Exception))
        self.assertTrue(issubclass(socket.herror, OSError))
        self.assertTrue(issubclass(socket.gaierror, OSError))
        self.assertTrue(issubclass(socket.timeout, OSError))
        self.assertIs(socket.error, OSError)
        self.assertIs(socket.timeout, TimeoutError)

    def test_setblocking_invalidfd(self):
        # Regression test for issue #28471

        sock0 = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        sock = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM, 0, sock0.fileno())
        sock0.close()
        self.addCleanup(sock.detach)

        with self.assertRaises(OSError):
            sock.setblocking(False)


@unittest.skipUnless(sys.platform == 'linux', 'Linux specific test')
class TestLinuxAbstractNamespace(unittest.TestCase):

    UNIX_PATH_MAX = 108

    def testLinuxAbstractNamespace(self):
        address = b"\x00python-test-hello\x00\xff"
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s1:
            s1.bind(address)
            s1.listen()
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
            self.assertRaises(OSError, s.bind, address)

    def testStrName(self):
        # Check that an abstract name can be passed as a string.
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            s.bind("\x00python\x00test\x00")
            self.assertEqual(s.getsockname(), b"\x00python\x00test\x00")
        finally:
            s.close()

    def testBytearrayName(self):
        # Check that an abstract name can be passed as a bytearray.
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
            s.bind(bytearray(b"\x00python\x00test\x00"))
            self.assertEqual(s.getsockname(), b"\x00python\x00test\x00")

@unittest.skipUnless(hasattr(socket, 'AF_UNIX'), 'test needs socket.AF_UNIX')
class TestUnixDomain(unittest.TestCase):

    def setUp(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

    def tearDown(self):
        self.sock.close()

    def encoded(self, path):
        # Return the given path encoded in the file system encoding,
        # or skip the test if this is not possible.
        try:
            return os.fsencode(path)
        except UnicodeEncodeError:
            self.skipTest(
                "Pathname {0!a} cannot be represented in file "
                "system encoding {1!r}".format(
                    path, sys.getfilesystemencoding()))

    def bind(self, sock, path):
        # Bind the socket
        try:
            socket_helper.bind_unix_socket(sock, path)
        except OSError as e:
            if str(e) == "AF_UNIX path too long":
                self.skipTest(
                    "Pathname {0!a} is too long to serve as an AF_UNIX path"
                    .format(path))
            else:
                raise

    def testUnbound(self):
        # Issue #30205 (note getsockname() can return None on OS X)
        self.assertIn(self.sock.getsockname(), ('', None))

    def testStrAddr(self):
        # Test binding to and retrieving a normal string pathname.
        path = os.path.abspath(os_helper.TESTFN)
        self.bind(self.sock, path)
        self.addCleanup(os_helper.unlink, path)
        self.assertEqual(self.sock.getsockname(), path)

    def testBytesAddr(self):
        # Test binding to a bytes pathname.
        path = os.path.abspath(os_helper.TESTFN)
        self.bind(self.sock, self.encoded(path))
        self.addCleanup(os_helper.unlink, path)
        self.assertEqual(self.sock.getsockname(), path)

    def testSurrogateescapeBind(self):
        # Test binding to a valid non-ASCII pathname, with the
        # non-ASCII bytes supplied using surrogateescape encoding.
        path = os.path.abspath(os_helper.TESTFN_UNICODE)
        b = self.encoded(path)
        self.bind(self.sock, b.decode("ascii", "surrogateescape"))
        self.addCleanup(os_helper.unlink, path)
        self.assertEqual(self.sock.getsockname(), path)

    def testUnencodableAddr(self):
        # Test binding to a pathname that cannot be encoded in the
        # file system encoding.
        if os_helper.TESTFN_UNENCODABLE is None:
            self.skipTest("No unencodable filename available")
        path = os.path.abspath(os_helper.TESTFN_UNENCODABLE)
        self.bind(self.sock, path)
        self.addCleanup(os_helper.unlink, path)
        self.assertEqual(self.sock.getsockname(), path)


class BufferIOTest(SocketConnectedTest):
    """
    Test the buffer versions of socket.recv() and socket.send().
    """
    def __init__(self, methodName='runTest'):
        SocketConnectedTest.__init__(self, methodName=methodName)

    def testRecvIntoArray(self):
        buf = array.array("B", [0] * len(MSG))
        nbytes = self.cli_conn.recv_into(buf)
        self.assertEqual(nbytes, len(MSG))
        buf = buf.tobytes()
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
        buf = array.array("B", [0] * len(MSG))
        nbytes, addr = self.cli_conn.recvfrom_into(buf)
        self.assertEqual(nbytes, len(MSG))
        buf = buf.tobytes()
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

    def testRecvFromIntoSmallBuffer(self):
        # See issue #20246.
        buf = bytearray(8)
        self.assertRaises(ValueError, self.cli_conn.recvfrom_into, buf, 1024)

    def _testRecvFromIntoSmallBuffer(self):
        self.serv_conn.send(MSG)

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
    except (FileNotFoundError, IsADirectoryError, PermissionError):
        # It's ok if the file does not exist, is a directory or if we
        # have not the permission to read it.
        return False
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
        self.addCleanup(srv.close)
        self.addCleanup(cli.close)

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


@unittest.skipUnless(isTipcAvailable(),
                     "TIPC module is not loaded, please 'sudo modprobe tipc'")
class TIPCThreadableTest(unittest.TestCase, ThreadableTest):
    def __init__(self, methodName = 'runTest'):
        unittest.TestCase.__init__(self, methodName = methodName)
        ThreadableTest.__init__(self)

    def setUp(self):
        self.srv = socket.socket(socket.AF_TIPC, socket.SOCK_STREAM)
        self.addCleanup(self.srv.close)
        self.srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srvaddr = (socket.TIPC_ADDR_NAMESEQ, TIPC_STYPE,
                TIPC_LOWER, TIPC_UPPER)
        self.srv.bind(srvaddr)
        self.srv.listen()
        self.serverExplicitReady()
        self.conn, self.connaddr = self.srv.accept()
        self.addCleanup(self.conn.close)

    def clientSetUp(self):
        # There is a hittable race between serverExplicitReady() and the
        # accept() call; sleep a little while to avoid it, otherwise
        # we could get an exception
        time.sleep(0.1)
        self.cli = socket.socket(socket.AF_TIPC, socket.SOCK_STREAM)
        self.addCleanup(self.cli.close)
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
            s.setblocking(True)
            self.checkNonblock(s, nonblock=False)
            s.setblocking(False)
            self.checkNonblock(s)
            s.settimeout(None)
            self.checkNonblock(s, nonblock=False)
            s.settimeout(2.0)
            self.checkNonblock(s, timeout=2.0)
            s.setblocking(True)
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
    TIMEOUT = support.LOOPBACK_TIMEOUT

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
        with open(os_helper.TESTFN, 'wb') as f:
            for csize in chunks(cls.FILESIZE, cls.BUFSIZE):
                f.write(chunk)
        with open(os_helper.TESTFN, 'rb') as f:
            cls.FILEDATA = f.read()
            assert len(cls.FILEDATA) == cls.FILESIZE

    @classmethod
    def tearDownClass(cls):
        os_helper.unlink(os_helper.TESTFN)

    def accept_conn(self):
        self.serv.settimeout(support.LONG_TIMEOUT)
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
        file = open(os_helper.TESTFN, 'rb')
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
        filename = os_helper.TESTFN + "2"
        with open(filename, 'wb'):
            self.addCleanup(os_helper.unlink, filename)
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
        file = open(os_helper.TESTFN, 'rb')
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
        file = open(os_helper.TESTFN, 'rb')
        sock = socket.create_connection(address,
                                        timeout=support.LOOPBACK_TIMEOUT)
        with sock, file:
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
        file = open(os_helper.TESTFN, 'rb')
        sock = socket.create_connection(address,
                                        timeout=support.LOOPBACK_TIMEOUT)
        with sock, file:
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
        file = open(os_helper.TESTFN, 'rb')
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
        file = open(os_helper.TESTFN, 'rb')
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
        file = open(os_helper.TESTFN, 'rb')
        sock = socket.create_connection(address,
                                        timeout=support.LOOPBACK_TIMEOUT)
        with sock, file:
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
        with open(os_helper.TESTFN, 'rb') as file:
            with socket.create_connection(address) as sock:
                sock.settimeout(0.01)
                meth = self.meth_from_sock(sock)
                self.assertRaises(TimeoutError, meth, file)

    def testWithTimeoutTriggeredSend(self):
        conn = self.accept_conn()
        conn.recv(88192)

    # errors

    def _test_errors(self):
        pass

    def test_errors(self):
        with open(os_helper.TESTFN, 'rb') as file:
            with socket.socket(type=socket.SOCK_DGRAM) as s:
                meth = self.meth_from_sock(s)
                self.assertRaisesRegex(
                    ValueError, "SOCK_STREAM", meth, file)
        with open(os_helper.TESTFN, 'rt') as file:
            with socket.socket() as s:
                meth = self.meth_from_sock(s)
                self.assertRaisesRegex(
                    ValueError, "binary mode", meth, file)
        with open(os_helper.TESTFN, 'rb') as file:
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
            self.assertEqual(enc[:msglen], ciphertext)

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

    def test_length_restriction(self):
        # bpo-35050, off-by-one error in length check
        sock = socket.socket(socket.AF_ALG, socket.SOCK_SEQPACKET, 0)
        self.addCleanup(sock.close)

        # salg_type[14]
        with self.assertRaises(FileNotFoundError):
            sock.bind(("t" * 13, "name"))
        with self.assertRaisesRegex(ValueError, "type too long"):
            sock.bind(("t" * 14, "name"))

        # salg_name[64]
        with self.assertRaises(FileNotFoundError):
            sock.bind(("type", "n" * 63))
        with self.assertRaisesRegex(ValueError, "name too long"):
            sock.bind(("type", "n" * 64))


@unittest.skipUnless(sys.platform.startswith("win"), "requires Windows")
class TestMSWindowsTCPFlags(unittest.TestCase):
    knownTCPFlags = {
                       # available since long time ago
                       'TCP_MAXSEG',
                       'TCP_NODELAY',
                       # available starting with Windows 10 1607
                       'TCP_FASTOPEN',
                       # available starting with Windows 10 1703
                       'TCP_KEEPCNT',
                       # available starting with Windows 10 1709
                       'TCP_KEEPIDLE',
                       'TCP_KEEPINTVL'
                       }

    def test_new_tcp_flags(self):
        provided = [s for s in dir(socket) if s.startswith('TCP')]
        unknown = [s for s in provided if s not in self.knownTCPFlags]

        self.assertEqual([], unknown,
            "New TCP flags were discovered. See bpo-32394 for more information")


class CreateServerTest(unittest.TestCase):

    def test_address(self):
        port = socket_helper.find_unused_port()
        with socket.create_server(("127.0.0.1", port)) as sock:
            self.assertEqual(sock.getsockname()[0], "127.0.0.1")
            self.assertEqual(sock.getsockname()[1], port)
        if socket_helper.IPV6_ENABLED:
            with socket.create_server(("::1", port),
                                      family=socket.AF_INET6) as sock:
                self.assertEqual(sock.getsockname()[0], "::1")
                self.assertEqual(sock.getsockname()[1], port)

    def test_family_and_type(self):
        with socket.create_server(("127.0.0.1", 0)) as sock:
            self.assertEqual(sock.family, socket.AF_INET)
            self.assertEqual(sock.type, socket.SOCK_STREAM)
        if socket_helper.IPV6_ENABLED:
            with socket.create_server(("::1", 0), family=socket.AF_INET6) as s:
                self.assertEqual(s.family, socket.AF_INET6)
                self.assertEqual(sock.type, socket.SOCK_STREAM)

    def test_reuse_port(self):
        if not hasattr(socket, "SO_REUSEPORT"):
            with self.assertRaises(ValueError):
                socket.create_server(("localhost", 0), reuse_port=True)
        else:
            with socket.create_server(("localhost", 0)) as sock:
                opt = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT)
                self.assertEqual(opt, 0)
            with socket.create_server(("localhost", 0), reuse_port=True) as sock:
                opt = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT)
                self.assertNotEqual(opt, 0)

    @unittest.skipIf(not hasattr(_socket, 'IPPROTO_IPV6') or
                     not hasattr(_socket, 'IPV6_V6ONLY'),
                     "IPV6_V6ONLY option not supported")
    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test')
    def test_ipv6_only_default(self):
        with socket.create_server(("::1", 0), family=socket.AF_INET6) as sock:
            assert sock.getsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY)

    @unittest.skipIf(not socket.has_dualstack_ipv6(),
                     "dualstack_ipv6 not supported")
    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test')
    def test_dualstack_ipv6_family(self):
        with socket.create_server(("::1", 0), family=socket.AF_INET6,
                                  dualstack_ipv6=True) as sock:
            self.assertEqual(sock.family, socket.AF_INET6)


class CreateServerFunctionalTest(unittest.TestCase):
    timeout = support.LOOPBACK_TIMEOUT

    def setUp(self):
        self.thread = None

    def tearDown(self):
        if self.thread is not None:
            self.thread.join(self.timeout)

    def echo_server(self, sock):
        def run(sock):
            with sock:
                conn, _ = sock.accept()
                with conn:
                    event.wait(self.timeout)
                    msg = conn.recv(1024)
                    if not msg:
                        return
                    conn.sendall(msg)

        event = threading.Event()
        sock.settimeout(self.timeout)
        self.thread = threading.Thread(target=run, args=(sock, ))
        self.thread.start()
        event.set()

    def echo_client(self, addr, family):
        with socket.socket(family=family) as sock:
            sock.settimeout(self.timeout)
            sock.connect(addr)
            sock.sendall(b'foo')
            self.assertEqual(sock.recv(1024), b'foo')

    def test_tcp4(self):
        port = socket_helper.find_unused_port()
        with socket.create_server(("", port)) as sock:
            self.echo_server(sock)
            self.echo_client(("127.0.0.1", port), socket.AF_INET)

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test')
    def test_tcp6(self):
        port = socket_helper.find_unused_port()
        with socket.create_server(("", port),
                                  family=socket.AF_INET6) as sock:
            self.echo_server(sock)
            self.echo_client(("::1", port), socket.AF_INET6)

    # --- dual stack tests

    @unittest.skipIf(not socket.has_dualstack_ipv6(),
                     "dualstack_ipv6 not supported")
    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test')
    def test_dual_stack_client_v4(self):
        port = socket_helper.find_unused_port()
        with socket.create_server(("", port), family=socket.AF_INET6,
                                  dualstack_ipv6=True) as sock:
            self.echo_server(sock)
            self.echo_client(("127.0.0.1", port), socket.AF_INET)

    @unittest.skipIf(not socket.has_dualstack_ipv6(),
                     "dualstack_ipv6 not supported")
    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'IPv6 required for this test')
    def test_dual_stack_client_v6(self):
        port = socket_helper.find_unused_port()
        with socket.create_server(("", port), family=socket.AF_INET6,
                                  dualstack_ipv6=True) as sock:
            self.echo_server(sock)
            self.echo_client(("::1", port), socket.AF_INET6)

@requireAttrs(socket, "send_fds")
@requireAttrs(socket, "recv_fds")
@requireAttrs(socket, "AF_UNIX")
class SendRecvFdsTests(unittest.TestCase):
    def testSendAndRecvFds(self):
        def close_pipes(pipes):
            for fd1, fd2 in pipes:
                os.close(fd1)
                os.close(fd2)

        def close_fds(fds):
            for fd in fds:
                os.close(fd)

        # send 10 file descriptors
        pipes = [os.pipe() for _ in range(10)]
        self.addCleanup(close_pipes, pipes)
        fds = [rfd for rfd, wfd in pipes]

        # use a UNIX socket pair to exchange file descriptors locally
        sock1, sock2 = socket.socketpair(socket.AF_UNIX, socket.SOCK_STREAM)
        with sock1, sock2:
            socket.send_fds(sock1, [MSG], fds)
            # request more data and file descriptors than expected
            msg, fds2, flags, addr = socket.recv_fds(sock2, len(MSG) * 2, len(fds) * 2)
            self.addCleanup(close_fds, fds2)

        self.assertEqual(msg, MSG)
        self.assertEqual(len(fds2), len(fds))
        self.assertEqual(flags, 0)
        # don't test addr

        # test that file descriptors are connected
        for index, fds in enumerate(pipes):
            rfd, wfd = fds
            os.write(wfd, str(index).encode())

        for index, rfd in enumerate(fds2):
            data = os.read(rfd, 100)
            self.assertEqual(data,  str(index).encode())


def test_main():
    tests = [GeneralModuleTests, BasicTCPTest, TCPCloserTest, TCPTimeoutTest,
             TestExceptions, BufferIOTest, BasicTCPTest2, BasicUDPTest,
             UDPTimeoutTest, CreateServerTest, CreateServerFunctionalTest,
             SendRecvFdsTests]

    tests.extend([
        NonBlockingTCPTests,
        FileObjectClassTestCase,
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
        InheritanceTest,
        NonblockConstantTest
    ])
    tests.append(BasicSocketPairTest)
    tests.append(TestUnixDomain)
    tests.append(TestLinuxAbstractNamespace)
    tests.extend([TIPCTest, TIPCThreadableTest])
    tests.extend([BasicCANTest, CANTest])
    tests.extend([BasicRDSTest, RDSTest])
    tests.append(LinuxKernelCryptoAPI)
    tests.append(BasicQIPCRTRTest)
    tests.extend([
        BasicVSOCKTest,
        ThreadedVSOCKSocketStreamTest,
    ])
    tests.append(BasicBluetoothTest)
    tests.extend([
        CmsgMacroTests,
        SendmsgUDPTest,
        RecvmsgUDPTest,
        RecvmsgIntoUDPTest,
        SendmsgUDP6Test,
        RecvmsgUDP6Test,
        RecvmsgRFC3542AncillaryUDP6Test,
        RecvmsgIntoRFC3542AncillaryUDP6Test,
        RecvmsgIntoUDP6Test,
        SendmsgUDPLITETest,
        RecvmsgUDPLITETest,
        RecvmsgIntoUDPLITETest,
        SendmsgUDPLITE6Test,
        RecvmsgUDPLITE6Test,
        RecvmsgRFC3542AncillaryUDPLITE6Test,
        RecvmsgIntoRFC3542AncillaryUDPLITE6Test,
        RecvmsgIntoUDPLITE6Test,
        SendmsgTCPTest,
        RecvmsgTCPTest,
        RecvmsgIntoTCPTest,
        SendmsgSCTPStreamTest,
        RecvmsgSCTPStreamTest,
        RecvmsgIntoSCTPStreamTest,
        SendmsgUnixStreamTest,
        RecvmsgUnixStreamTest,
        RecvmsgIntoUnixStreamTest,
        RecvmsgSCMRightsStreamTest,
        RecvmsgIntoSCMRightsStreamTest,
        # These are slow when setitimer() is not available
        InterruptedRecvTimeoutTest,
        InterruptedSendTimeoutTest,
        TestSocketSharing,
        SendfileUsingSendTest,
        SendfileUsingSendfileTest,
    ])
    tests.append(TestMSWindowsTCPFlags)

    thread_info = threading_helper.threading_setup()
    support.run_unittest(*tests)
    threading_helper.threading_cleanup(*thread_info)


if __name__ == "__main__":
    test_main()
