import asyncore
import unittest
import select
import os
import socket
import sys
import time
import warnings
import errno

from test import support
from test.support import TESTFN, run_unittest, unlink
from io import BytesIO
from io import StringIO

try:
    import threading
except ImportError:
    threading = None

HOST = support.HOST

class dummysocket:
    def __init__(self):
        self.closed = False

    def close(self):
        self.closed = True

    def fileno(self):
        return 42

class dummychannel:
    def __init__(self):
        self.socket = dummysocket()

    def close(self):
        self.socket.close()

class exitingdummy:
    def __init__(self):
        pass

    def handle_read_event(self):
        raise asyncore.ExitNow()

    handle_write_event = handle_read_event
    handle_close = handle_read_event
    handle_expt_event = handle_read_event

class crashingdummy:
    def __init__(self):
        self.error_handled = False

    def handle_read_event(self):
        raise Exception()

    handle_write_event = handle_read_event
    handle_close = handle_read_event
    handle_expt_event = handle_read_event

    def handle_error(self):
        self.error_handled = True

# used when testing senders; just collects what it gets until newline is sent
def capture_server(evt, buf, serv):
    try:
        serv.listen(5)
        conn, addr = serv.accept()
    except socket.timeout:
        pass
    else:
        n = 200
        while n > 0:
            r, w, e = select.select([conn], [], [])
            if r:
                data = conn.recv(10)
                # keep everything except for the newline terminator
                buf.write(data.replace(b'\n', b''))
                if b'\n' in data:
                    break
            n -= 1
            time.sleep(0.01)

        conn.close()
    finally:
        serv.close()
        evt.set()


class HelperFunctionTests(unittest.TestCase):
    def test_readwriteexc(self):
        # Check exception handling behavior of read, write and _exception

        # check that ExitNow exceptions in the object handler method
        # bubbles all the way up through asyncore read/write/_exception calls
        tr1 = exitingdummy()
        self.assertRaises(asyncore.ExitNow, asyncore.read, tr1)
        self.assertRaises(asyncore.ExitNow, asyncore.write, tr1)
        self.assertRaises(asyncore.ExitNow, asyncore._exception, tr1)

        # check that an exception other than ExitNow in the object handler
        # method causes the handle_error method to get called
        tr2 = crashingdummy()
        asyncore.read(tr2)
        self.assertEqual(tr2.error_handled, True)

        tr2 = crashingdummy()
        asyncore.write(tr2)
        self.assertEqual(tr2.error_handled, True)

        tr2 = crashingdummy()
        asyncore._exception(tr2)
        self.assertEqual(tr2.error_handled, True)

    # asyncore.readwrite uses constants in the select module that
    # are not present in Windows systems (see this thread:
    # http://mail.python.org/pipermail/python-list/2001-October/109973.html)
    # These constants should be present as long as poll is available

    @unittest.skipUnless(hasattr(select, 'poll'), 'select.poll required')
    def test_readwrite(self):
        # Check that correct methods are called by readwrite()

        attributes = ('read', 'expt', 'write', 'closed', 'error_handled')

        expected = (
            (select.POLLIN, 'read'),
            (select.POLLPRI, 'expt'),
            (select.POLLOUT, 'write'),
            (select.POLLERR, 'closed'),
            (select.POLLHUP, 'closed'),
            (select.POLLNVAL, 'closed'),
            )

        class testobj:
            def __init__(self):
                self.read = False
                self.write = False
                self.closed = False
                self.expt = False
                self.error_handled = False

            def handle_read_event(self):
                self.read = True

            def handle_write_event(self):
                self.write = True

            def handle_close(self):
                self.closed = True

            def handle_expt_event(self):
                self.expt = True

            def handle_error(self):
                self.error_handled = True

        for flag, expectedattr in expected:
            tobj = testobj()
            self.assertEqual(getattr(tobj, expectedattr), False)
            asyncore.readwrite(tobj, flag)

            # Only the attribute modified by the routine we expect to be
            # called should be True.
            for attr in attributes:
                self.assertEqual(getattr(tobj, attr), attr==expectedattr)

            # check that ExitNow exceptions in the object handler method
            # bubbles all the way up through asyncore readwrite call
            tr1 = exitingdummy()
            self.assertRaises(asyncore.ExitNow, asyncore.readwrite, tr1, flag)

            # check that an exception other than ExitNow in the object handler
            # method causes the handle_error method to get called
            tr2 = crashingdummy()
            self.assertEqual(tr2.error_handled, False)
            asyncore.readwrite(tr2, flag)
            self.assertEqual(tr2.error_handled, True)

    def test_closeall(self):
        self.closeall_check(False)

    def test_closeall_default(self):
        self.closeall_check(True)

    def closeall_check(self, usedefault):
        # Check that close_all() closes everything in a given map

        l = []
        testmap = {}
        for i in range(10):
            c = dummychannel()
            l.append(c)
            self.assertEqual(c.socket.closed, False)
            testmap[i] = c

        if usedefault:
            socketmap = asyncore.socket_map
            try:
                asyncore.socket_map = testmap
                asyncore.close_all()
            finally:
                testmap, asyncore.socket_map = asyncore.socket_map, socketmap
        else:
            asyncore.close_all(testmap)

        self.assertEqual(len(testmap), 0)

        for c in l:
            self.assertEqual(c.socket.closed, True)

    def test_compact_traceback(self):
        try:
            raise Exception("I don't like spam!")
        except:
            real_t, real_v, real_tb = sys.exc_info()
            r = asyncore.compact_traceback()
        else:
            self.fail("Expected exception")

        (f, function, line), t, v, info = r
        self.assertEqual(os.path.split(f)[-1], 'test_asyncore.py')
        self.assertEqual(function, 'test_compact_traceback')
        self.assertEqual(t, real_t)
        self.assertEqual(v, real_v)
        self.assertEqual(info, '[%s|%s|%s]' % (f, function, line))


class DispatcherTests(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        asyncore.close_all()

    def test_basic(self):
        d = asyncore.dispatcher()
        self.assertEqual(d.readable(), True)
        self.assertEqual(d.writable(), True)

    def test_repr(self):
        d = asyncore.dispatcher()
        self.assertEqual(repr(d), '<asyncore.dispatcher at %#x>' % id(d))

    def test_log(self):
        d = asyncore.dispatcher()

        # capture output of dispatcher.log() (to stderr)
        fp = StringIO()
        stderr = sys.stderr
        l1 = "Lovely spam! Wonderful spam!"
        l2 = "I don't like spam!"
        try:
            sys.stderr = fp
            d.log(l1)
            d.log(l2)
        finally:
            sys.stderr = stderr

        lines = fp.getvalue().splitlines()
        self.assertEqual(lines, ['log: %s' % l1, 'log: %s' % l2])

    def test_log_info(self):
        d = asyncore.dispatcher()

        # capture output of dispatcher.log_info() (to stdout via print)
        fp = StringIO()
        stdout = sys.stdout
        l1 = "Have you got anything without spam?"
        l2 = "Why can't she have egg bacon spam and sausage?"
        l3 = "THAT'S got spam in it!"
        try:
            sys.stdout = fp
            d.log_info(l1, 'EGGS')
            d.log_info(l2)
            d.log_info(l3, 'SPAM')
        finally:
            sys.stdout = stdout

        lines = fp.getvalue().splitlines()
        expected = ['EGGS: %s' % l1, 'info: %s' % l2, 'SPAM: %s' % l3]

        self.assertEqual(lines, expected)

    def test_unhandled(self):
        d = asyncore.dispatcher()
        d.ignore_log_types = ()

        # capture output of dispatcher.log_info() (to stdout via print)
        fp = StringIO()
        stdout = sys.stdout
        try:
            sys.stdout = fp
            d.handle_expt()
            d.handle_read()
            d.handle_write()
            d.handle_connect()
        finally:
            sys.stdout = stdout

        lines = fp.getvalue().splitlines()
        expected = ['warning: unhandled incoming priority event',
                    'warning: unhandled read event',
                    'warning: unhandled write event',
                    'warning: unhandled connect event']
        self.assertEqual(lines, expected)

    def test_issue_8594(self):
        # XXX - this test is supposed to be removed in next major Python
        # version
        d = asyncore.dispatcher(socket.socket())
        # make sure the error message no longer refers to the socket
        # object but the dispatcher instance instead
        self.assertRaisesRegexp(AttributeError, 'dispatcher instance',
                                getattr, d, 'foo')
        # cheap inheritance with the underlying socket is supposed
        # to still work but a DeprecationWarning is expected
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            family = d.family
            self.assertEqual(family, socket.AF_INET)
            self.assertEqual(len(w), 1)
            self.assertTrue(issubclass(w[0].category, DeprecationWarning))

    def test_strerror(self):
        # refers to bug #8573
        err = asyncore._strerror(errno.EPERM)
        if hasattr(os, 'strerror'):
            self.assertEqual(err, os.strerror(errno.EPERM))
        err = asyncore._strerror(-1)
        self.assertIn("unknown error", err.lower())


class dispatcherwithsend_noread(asyncore.dispatcher_with_send):
    def readable(self):
        return False

    def handle_connect(self):
        pass

class DispatcherWithSendTests(unittest.TestCase):
    usepoll = False

    def setUp(self):
        pass

    def tearDown(self):
        asyncore.close_all()

    @unittest.skipUnless(threading, 'Threading required for this test.')
    @support.reap_threads
    def test_send(self):
        evt = threading.Event()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(3)
        port = support.bind_port(sock)

        cap = BytesIO()
        args = (evt, cap, sock)
        t = threading.Thread(target=capture_server, args=args)
        t.start()
        try:
            # wait a little longer for the server to initialize (it sometimes
            # refuses connections on slow machines without this wait)
            time.sleep(0.2)

            data = b"Suppose there isn't a 16-ton weight?"
            d = dispatcherwithsend_noread()
            d.create_socket(socket.AF_INET, socket.SOCK_STREAM)
            d.connect((HOST, port))

            # give time for socket to connect
            time.sleep(0.1)

            d.send(data)
            d.send(data)
            d.send(b'\n')

            n = 1000
            while d.out_buffer and n > 0:
                asyncore.poll()
                n -= 1

            evt.wait()

            self.assertEqual(cap.getvalue(), data*2)
        finally:
            t.join()


class DispatcherWithSendTests_UsePoll(DispatcherWithSendTests):
    usepoll = True

@unittest.skipUnless(hasattr(asyncore, 'file_wrapper'),
                     'asyncore.file_wrapper required')
class FileWrapperTest(unittest.TestCase):
    def setUp(self):
        self.d = b"It's not dead, it's sleeping!"
        with open(TESTFN, 'wb') as file:
            file.write(self.d)

    def tearDown(self):
        unlink(TESTFN)

    def test_recv(self):
        fd = os.open(TESTFN, os.O_RDONLY)
        w = asyncore.file_wrapper(fd)
        os.close(fd)

        self.assertNotEqual(w.fd, fd)
        self.assertNotEqual(w.fileno(), fd)
        self.assertEqual(w.recv(13), b"It's not dead")
        self.assertEqual(w.read(6), b", it's")
        w.close()
        self.assertRaises(OSError, w.read, 1)

    def test_send(self):
        d1 = b"Come again?"
        d2 = b"I want to buy some cheese."
        fd = os.open(TESTFN, os.O_WRONLY | os.O_APPEND)
        w = asyncore.file_wrapper(fd)
        os.close(fd)

        w.write(d1)
        w.send(d2)
        w.close()
        with open(TESTFN, 'rb') as file:
            self.assertEqual(file.read(), self.d + d1 + d2)

    @unittest.skipUnless(hasattr(asyncore, 'file_dispatcher'),
                         'asyncore.file_dispatcher required')
    def test_dispatcher(self):
        fd = os.open(TESTFN, os.O_RDONLY)
        data = []
        class FileDispatcher(asyncore.file_dispatcher):
            def handle_read(self):
                data.append(self.recv(29))
        s = FileDispatcher(fd)
        os.close(fd)
        asyncore.loop(timeout=0.01, use_poll=True, count=2)
        self.assertEqual(b"".join(data), self.d)


class BaseTestHandler(asyncore.dispatcher):

    def __init__(self, sock=None):
        asyncore.dispatcher.__init__(self, sock)
        self.flag = False

    def handle_accept(self):
        raise Exception("handle_accept not supposed to be called")

    def handle_accepted(self):
        raise Exception("handle_accepted not supposed to be called")

    def handle_connect(self):
        raise Exception("handle_connect not supposed to be called")

    def handle_expt(self):
        raise Exception("handle_expt not supposed to be called")

    def handle_close(self):
        raise Exception("handle_close not supposed to be called")

    def handle_error(self):
        raise


class TCPServer(asyncore.dispatcher):
    """A server which listens on an address and dispatches the
    connection to a handler.
    """

    def __init__(self, handler=BaseTestHandler, host=HOST, port=0):
        asyncore.dispatcher.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.set_reuse_addr()
        self.bind((host, port))
        self.listen(5)
        self.handler = handler

    @property
    def address(self):
        return self.socket.getsockname()[:2]

    def handle_accepted(self, sock, addr):
        self.handler(sock)

    def handle_error(self):
        raise


class BaseClient(BaseTestHandler):

    def __init__(self, address):
        BaseTestHandler.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect(address)

    def handle_connect(self):
        pass


class BaseTestAPI(unittest.TestCase):

    def tearDown(self):
        asyncore.close_all()

    def loop_waiting_for_flag(self, instance, timeout=5):
        timeout = float(timeout) / 100
        count = 100
        while asyncore.socket_map and count > 0:
            asyncore.loop(timeout=0.01, count=1, use_poll=self.use_poll)
            if instance.flag:
                return
            count -= 1
            time.sleep(timeout)
        self.fail("flag not set")

    def test_handle_connect(self):
        # make sure handle_connect is called on connect()

        class TestClient(BaseClient):
            def handle_connect(self):
                self.flag = True

        server = TCPServer()
        client = TestClient(server.address)
        self.loop_waiting_for_flag(client)

    def test_handle_accept(self):
        # make sure handle_accept() is called when a client connects

        class TestListener(BaseTestHandler):

            def __init__(self):
                BaseTestHandler.__init__(self)
                self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
                self.bind((HOST, 0))
                self.listen(5)
                self.address = self.socket.getsockname()[:2]

            def handle_accept(self):
                self.flag = True

        server = TestListener()
        client = BaseClient(server.address)
        self.loop_waiting_for_flag(server)

    def test_handle_accepted(self):
        # make sure handle_accepted() is called when a client connects

        class TestListener(BaseTestHandler):

            def __init__(self):
                BaseTestHandler.__init__(self)
                self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
                self.bind((HOST, 0))
                self.listen(5)
                self.address = self.socket.getsockname()[:2]

            def handle_accept(self):
                asyncore.dispatcher.handle_accept(self)

            def handle_accepted(self, sock, addr):
                self.flag = True

        server = TestListener()
        client = BaseClient(server.address)
        self.loop_waiting_for_flag(server)


    def test_handle_read(self):
        # make sure handle_read is called on data received

        class TestClient(BaseClient):
            def handle_read(self):
                self.flag = True

        class TestHandler(BaseTestHandler):
            def __init__(self, conn):
                BaseTestHandler.__init__(self, conn)
                self.send(b'x' * 1024)

        server = TCPServer(TestHandler)
        client = TestClient(server.address)
        self.loop_waiting_for_flag(client)

    def test_handle_write(self):
        # make sure handle_write is called

        class TestClient(BaseClient):
            def handle_write(self):
                self.flag = True

        server = TCPServer()
        client = TestClient(server.address)
        self.loop_waiting_for_flag(client)

    def test_handle_close(self):
        # make sure handle_close is called when the other end closes
        # the connection

        class TestClient(BaseClient):

            def handle_read(self):
                # in order to make handle_close be called we are supposed
                # to make at least one recv() call
                self.recv(1024)

            def handle_close(self):
                self.flag = True
                self.close()

        class TestHandler(BaseTestHandler):
            def __init__(self, conn):
                BaseTestHandler.__init__(self, conn)
                self.close()

        server = TCPServer(TestHandler)
        client = TestClient(server.address)
        self.loop_waiting_for_flag(client)

    @unittest.skipIf(sys.platform.startswith("sunos"),
                     "OOB support is broken on Solaris")
    def test_handle_expt(self):
        # Make sure handle_expt is called on OOB data received.
        # Note: this might fail on some platforms as OOB data is
        # tenuously supported and rarely used.

        class TestClient(BaseClient):
            def handle_expt(self):
                self.flag = True

        class TestHandler(BaseTestHandler):
            def __init__(self, conn):
                BaseTestHandler.__init__(self, conn)
                self.socket.send(bytes(chr(244), 'latin-1'), socket.MSG_OOB)

        server = TCPServer(TestHandler)
        client = TestClient(server.address)
        self.loop_waiting_for_flag(client)

    def test_handle_error(self):

        class TestClient(BaseClient):
            def handle_write(self):
                1.0 / 0
            def handle_error(self):
                self.flag = True
                try:
                    raise
                except ZeroDivisionError:
                    pass
                else:
                    raise Exception("exception not raised")

        server = TCPServer()
        client = TestClient(server.address)
        self.loop_waiting_for_flag(client)

    def test_connection_attributes(self):
        server = TCPServer()
        client = BaseClient(server.address)

        # we start disconnected
        self.assertFalse(server.connected)
        self.assertTrue(server.accepting)
        # this can't be taken for granted across all platforms
        #self.assertFalse(client.connected)
        self.assertFalse(client.accepting)

        # execute some loops so that client connects to server
        asyncore.loop(timeout=0.01, use_poll=self.use_poll, count=100)
        self.assertFalse(server.connected)
        self.assertTrue(server.accepting)
        self.assertTrue(client.connected)
        self.assertFalse(client.accepting)

        # disconnect the client
        client.close()
        self.assertFalse(server.connected)
        self.assertTrue(server.accepting)
        self.assertFalse(client.connected)
        self.assertFalse(client.accepting)

        # stop serving
        server.close()
        self.assertFalse(server.connected)
        self.assertFalse(server.accepting)

    def test_create_socket(self):
        s = asyncore.dispatcher()
        s.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.assertEqual(s.socket.family, socket.AF_INET)
        SOCK_NONBLOCK = getattr(socket, 'SOCK_NONBLOCK', 0)
        self.assertEqual(s.socket.type, socket.SOCK_STREAM | SOCK_NONBLOCK)

    def test_bind(self):
        s1 = asyncore.dispatcher()
        s1.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        s1.bind((HOST, 0))
        s1.listen(5)
        port = s1.socket.getsockname()[1]

        s2 = asyncore.dispatcher()
        s2.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        # EADDRINUSE indicates the socket was correctly bound
        self.assertRaises(socket.error, s2.bind, (HOST, port))

    def test_set_reuse_addr(self):
        sock = socket.socket()
        try:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        except socket.error:
            unittest.skip("SO_REUSEADDR not supported on this platform")
        else:
            # if SO_REUSEADDR succeeded for sock we expect asyncore
            # to do the same
            s = asyncore.dispatcher(socket.socket())
            self.assertFalse(s.socket.getsockopt(socket.SOL_SOCKET,
                                                 socket.SO_REUSEADDR))
            s.create_socket(socket.AF_INET, socket.SOCK_STREAM)
            s.set_reuse_addr()
            self.assertTrue(s.socket.getsockopt(socket.SOL_SOCKET,
                                                 socket.SO_REUSEADDR))
        finally:
            sock.close()


class TestAPI_UseSelect(BaseTestAPI):
    use_poll = False

@unittest.skipUnless(hasattr(select, 'poll'), 'select.poll required')
class TestAPI_UsePoll(BaseTestAPI):
    use_poll = True


def test_main():
    tests = [HelperFunctionTests, DispatcherTests, DispatcherWithSendTests,
             DispatcherWithSendTests_UsePoll, TestAPI_UseSelect,
             TestAPI_UsePoll, FileWrapperTest]
    run_unittest(*tests)

if __name__ == "__main__":
    test_main()
