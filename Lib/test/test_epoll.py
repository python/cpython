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
import os
import socket
import errno
import time
import select
import tempfile
import unittest

from test import support
if not hasattr(select, "epoll"):
    raise unittest.SkipTest("test works only on Linux 2.6")

try:
    select.epoll()
except IOError as e:
    if e.errno == errno.ENOSYS:
        raise unittest.SkipTest("kernel doesn't support epoll()")

class TestEPoll(unittest.TestCase):

    def setUp(self):
        self.serverSocket = socket.socket()
        self.serverSocket.bind(('127.0.0.1', 0))
        self.serverSocket.listen(1)
        self.connections = [self.serverSocket]


    def tearDown(self):
        for skt in self.connections:
            skt.close()

    def _connected_pair(self):
        client = socket.socket()
        client.setblocking(False)
        try:
            client.connect(('127.0.0.1', self.serverSocket.getsockname()[1]))
        except socket.error as e:
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

    def test_badcreate(self):
        self.assertRaises(TypeError, select.epoll, 1, 2, 3)
        self.assertRaises(TypeError, select.epoll, 'foo')
        self.assertRaises(TypeError, select.epoll, None)
        self.assertRaises(TypeError, select.epoll, ())
        self.assertRaises(TypeError, select.epoll, ['foo'])
        self.assertRaises(TypeError, select.epoll, {})

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
            # IOError: [Errno 9] Bad file descriptor
            self.assertRaises(IOError, ep.register, 10000,
                select.EPOLLIN | select.EPOLLOUT)
            # registering twice also raises an exception
            ep.register(server, select.EPOLLIN | select.EPOLLOUT)
            self.assertRaises(IOError, ep.register, server,
                select.EPOLLIN | select.EPOLLOUT)
        finally:
            ep.close()

    def test_fromfd(self):
        server, client = self._connected_pair()

        ep = select.epoll(2)
        ep2 = select.epoll.fromfd(ep.fileno())

        ep2.register(server.fileno(), select.EPOLLIN | select.EPOLLOUT)
        ep2.register(client.fileno(), select.EPOLLIN | select.EPOLLOUT)

        events = ep.poll(1, 4)
        events2 = ep2.poll(0.9, 4)
        self.assertEqual(len(events), 2)
        self.assertEqual(len(events2), 2)

        ep.close()
        try:
            ep2.poll(1, 4)
        except IOError as e:
            self.assertEqual(e.args[0], errno.EBADF, e)
        else:
            self.fail("epoll on closed fd didn't raise EBADF")

    def test_control_and_wait(self):
        client, server = self._connected_pair()

        ep = select.epoll(16)
        ep.register(server.fileno(),
                   select.EPOLLIN | select.EPOLLOUT | select.EPOLLET)
        ep.register(client.fileno(),
                   select.EPOLLIN | select.EPOLLOUT | select.EPOLLET)

        now = time.time()
        events = ep.poll(1, 4)
        then = time.time()
        self.assertFalse(then - now > 0.1, then - now)

        events.sort()
        expected = [(client.fileno(), select.EPOLLOUT),
                    (server.fileno(), select.EPOLLOUT)]
        expected.sort()

        self.assertEqual(events, expected)
        self.assertFalse(then - now > 0.01, then - now)

        now = time.time()
        events = ep.poll(timeout=2.1, maxevents=4)
        then = time.time()
        self.assertFalse(events)

        client.send(b"Hello!")
        server.send(b"world!!!")

        now = time.time()
        events = ep.poll(1, 4)
        then = time.time()
        self.assertFalse(then - now > 0.01)

        events.sort()
        expected = [(client.fileno(), select.EPOLLIN | select.EPOLLOUT),
                    (server.fileno(), select.EPOLLIN | select.EPOLLOUT)]
        expected.sort()

        self.assertEqual(events, expected)

        ep.unregister(client.fileno())
        ep.modify(server.fileno(), select.EPOLLOUT)
        now = time.time()
        events = ep.poll(1, 4)
        then = time.time()
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

        now = time.time()
        events = ep.poll(1, 4)
        then = time.time()
        self.assertFalse(then - now > 0.01)

        server.close()
        ep.unregister(fd)

def test_main():
    support.run_unittest(TestEPoll)

if __name__ == "__main__":
    test_main()
