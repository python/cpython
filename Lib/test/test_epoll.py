# Copyright (c) 2001-2006 Twisted Matrix Laboratories.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""
Tests for epoll wrapper.
"""
import errno
import os
import select
import socket
import time
import unittest
from test import support

if not hasattr(select, "epoll"):
    raise unittest.SkipTest("test works only on Linux 2.6")

try:
    select.epoll()
except OSError as e:
    if e.errno == errno.ENOSYS:
        raise unittest.SkipTest("kernel doesn't support epoll()")
    raise

class TestEPoll(unittest.TestCase):

    def setUp(self):
        self.serverSocket = socket.create_server(('127.0.0.1', 0))
        self.connections = [self.serverSocket]

    def tearDown(self):
        for skt in self.connections:
            skt.close()

    def _connected_pair(self):
        client = socket.socket()
        client.setblocking(False)
        try:
            client.connect(('127.0.0.1', self.serverSocket.getsockname()[1]))
        except OSError as e:
            self.assertEqual(e.args[0], errno.EINPROGRESS)
        else:
            raise AssertionError("Connect should have raised EINPROGRESS")
        server, addr = self.serverSocket.accept()

        self.connections.extend((client, server))
        return client, server

    def test_create(self):
        try:
            ep = select.epoll(16)
        except OSError as e:
            raise AssertionError(str(e))
        self.assertTrue(ep.fileno() > 0, ep.fileno())
        self.assertTrue(not ep.closed)
        ep.close()
        self.assertTrue(ep.closed)
        self.assertRaises(ValueError, ep.fileno)

        if hasattr(select, "EPOLL_CLOEXEC"):
            select.epoll(-1, select.EPOLL_CLOEXEC).close()
            select.epoll(flags=select.EPOLL_CLOEXEC).close()
            select.epoll(flags=0).close()

    def test_badcreate(self):
        self.assertRaises(TypeError, select.epoll, 1, 2, 3)
        self.assertRaises(TypeError, select.epoll, 'foo')
        self.assertRaises(TypeError, select.epoll, None)
        self.assertRaises(TypeError, select.epoll, ())
        self.assertRaises(TypeError, select.epoll, ['foo'])
        self.assertRaises(TypeError, select.epoll, {})

        self.assertRaises(ValueError, select.epoll, 0)
        self.assertRaises(ValueError, select.epoll, -2)
        self.assertRaises(ValueError, select.epoll, sizehint=-2)

        if hasattr(select, "EPOLL_CLOEXEC"):
            self.assertRaises(OSError, select.epoll, flags=12356)

    def test_context_manager(self):
        with select.epoll(16) as ep:
            self.assertGreater(ep.fileno(), 0)
            self.assertFalse(ep.closed)
        self.assertTrue(ep.closed)
        self.assertRaises(ValueError, ep.fileno)

    def test_add(self):
        server, client = self._connected_pair()

        ep = select.epoll(2)
        try:
            ep.register(server.fileno(), select.EPOLLIN | select.EPOLLOUT)
            ep.register(client.fileno(), select.EPOLLIN | select.EPOLLOUT)
        finally:
            ep.close()

        # adding by object w/ fileno works, too.
        ep = select.epoll(2)
        try:
            ep.register(server, select.EPOLLIN | select.EPOLLOUT)
            ep.register(client, select.EPOLLIN | select.EPOLLOUT)
        finally:
            ep.close()

        ep = select.epoll(2)
        try:
            # TypeError: argument must be an int, or have a fileno() method.
            self.assertRaises(TypeError, ep.register, object(),
                              select.EPOLLIN | select.EPOLLOUT)
            self.assertRaises(TypeError, ep.register, None,
                              select.EPOLLIN | select.EPOLLOUT)
            # ValueError: file descriptor cannot be a negative integer (-1)
            self.assertRaises(ValueError, ep.register, -1,
                              select.EPOLLIN | select.EPOLLOUT)
            # OSError: [Errno 9] Bad file descriptor
            self.assertRaises(OSError, ep.register, 10000,
                              select.EPOLLIN | select.EPOLLOUT)
            # registering twice also raises an exception
            ep.register(server, select.EPOLLIN | select.EPOLLOUT)
            self.assertRaises(OSError, ep.register, server,
                              select.EPOLLIN | select.EPOLLOUT)
        finally:
            ep.close()

    def test_fromfd(self):
        server, client = self._connected_pair()

        with select.epoll(2) as ep:
            ep2 = select.epoll.fromfd(ep.fileno())

            ep2.register(server.fileno(), select.EPOLLIN | select.EPOLLOUT)
            ep2.register(client.fileno(), select.EPOLLIN | select.EPOLLOUT)

            events = ep.poll(1, 4)
            events2 = ep2.poll(0.9, 4)
            self.assertEqual(len(events), 2)
            self.assertEqual(len(events2), 2)

        try:
            ep2.poll(1, 4)
        except OSError as e:
            self.assertEqual(e.args[0], errno.EBADF, e)
        else:
            self.fail("epoll on closed fd didn't raise EBADF")

    def test_control_and_wait(self):
        # create the epoll object
        client, server = self._connected_pair()
        ep = select.epoll(16)
        ep.register(server.fileno(),
                    select.EPOLLIN | select.EPOLLOUT | select.EPOLLET)
        ep.register(client.fileno(),
                    select.EPOLLIN | select.EPOLLOUT | select.EPOLLET)

        # EPOLLOUT
        now = time.monotonic()
        events = ep.poll(1, 4)
        then = time.monotonic()
        self.assertFalse(then - now > 0.1, then - now)

        expected = [(client.fileno(), select.EPOLLOUT),
                    (server.fileno(), select.EPOLLOUT)]
        self.assertEqual(sorted(events), sorted(expected))

        # no event
        events = ep.poll(timeout=0.1, maxevents=4)
        self.assertFalse(events)

        # send: EPOLLIN and EPOLLOUT
        client.sendall(b"Hello!")
        server.sendall(b"world!!!")

        # we might receive events one at a time, necessitating multiple calls to
        # poll
        events = []
        for _ in support.busy_retry(support.SHORT_TIMEOUT):
            now = time.monotonic()
            events += ep.poll(1.0, 4)
            then = time.monotonic()
            self.assertFalse(then - now > 0.01)
            if len(events) >= 2:
                break

        expected = [(client.fileno(), select.EPOLLIN | select.EPOLLOUT),
                    (server.fileno(), select.EPOLLIN | select.EPOLLOUT)]
        self.assertEqual(sorted(events), sorted(expected))

        # unregister, modify
        ep.unregister(client.fileno())
        ep.modify(server.fileno(), select.EPOLLOUT)
        now = time.monotonic()
        events = ep.poll(1, 4)
        then = time.monotonic()
        self.assertFalse(then - now > 0.01)

        expected = [(server.fileno(), select.EPOLLOUT)]
        self.assertEqual(events, expected)

    def test_errors(self):
        self.assertRaises(ValueError, select.epoll, -2)
        self.assertRaises(ValueError, select.epoll().register, -1,
                          select.EPOLLIN)

    def test_unregister_closed(self):
        server, client = self._connected_pair()
        fd = server.fileno()
        ep = select.epoll(16)
        ep.register(server)

        now = time.monotonic()
        events = ep.poll(1, 4)
        then = time.monotonic()
        self.assertFalse(then - now > 0.01)

        server.close()

        with self.assertRaises(OSError) as cm:
            ep.unregister(fd)
        self.assertEqual(cm.exception.errno, errno.EBADF)

    def test_close(self):
        open_file = open(__file__, "rb")
        self.addCleanup(open_file.close)
        fd = open_file.fileno()
        epoll = select.epoll()

        # test fileno() method and closed attribute
        self.assertIsInstance(epoll.fileno(), int)
        self.assertFalse(epoll.closed)

        # test close()
        epoll.close()
        self.assertTrue(epoll.closed)
        self.assertRaises(ValueError, epoll.fileno)

        # close() can be called more than once
        epoll.close()

        # operations must fail with ValueError("I/O operation on closed ...")
        self.assertRaises(ValueError, epoll.modify, fd, select.EPOLLIN)
        self.assertRaises(ValueError, epoll.poll, 1.0)
        self.assertRaises(ValueError, epoll.register, fd, select.EPOLLIN)
        self.assertRaises(ValueError, epoll.unregister, fd)

    def test_fd_non_inheritable(self):
        epoll = select.epoll()
        self.addCleanup(epoll.close)
        self.assertEqual(os.get_inheritable(epoll.fileno()), False)


if __name__ == "__main__":
    unittest.main()
