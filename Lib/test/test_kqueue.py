"""
Tests for kqueue wrapper.
"""
import socket
import errno
import time
import select
import sys
import unittest

from test import test_support
if not hasattr(select, "kqueue"):
    raise unittest.SkipTest("test works only on BSD")

class TestKQueue(unittest.TestCase):
    def test_create_queue(self):
        kq = select.kqueue()
        self.assertTrue(kq.fileno() > 0, kq.fileno())
        self.assertTrue(not kq.closed)
        kq.close()
        self.assertTrue(kq.closed)
        self.assertRaises(ValueError, kq.fileno)

    def test_create_event(self):
        fd = sys.stderr.fileno()
        ev = select.kevent(fd)
        other = select.kevent(1000)
        self.assertEqual(ev.ident, fd)
        self.assertEqual(ev.filter, select.KQ_FILTER_READ)
        self.assertEqual(ev.flags, select.KQ_EV_ADD)
        self.assertEqual(ev.fflags, 0)
        self.assertEqual(ev.data, 0)
        self.assertEqual(ev.udata, 0)
        self.assertEqual(ev, ev)
        self.assertNotEqual(ev, other)
        self.assertEqual(cmp(ev, other), -1)
        self.assertTrue(ev < other)
        self.assertTrue(other >= ev)
        self.assertRaises(TypeError, cmp, ev, None)
        self.assertRaises(TypeError, cmp, ev, 1)
        self.assertRaises(TypeError, cmp, ev, "ev")

        ev = select.kevent(fd, select.KQ_FILTER_WRITE)
        self.assertEqual(ev.ident, fd)
        self.assertEqual(ev.filter, select.KQ_FILTER_WRITE)
        self.assertEqual(ev.flags, select.KQ_EV_ADD)
        self.assertEqual(ev.fflags, 0)
        self.assertEqual(ev.data, 0)
        self.assertEqual(ev.udata, 0)
        self.assertEqual(ev, ev)
        self.assertNotEqual(ev, other)

        ev = select.kevent(fd, select.KQ_FILTER_WRITE, select.KQ_EV_ONESHOT)
        self.assertEqual(ev.ident, fd)
        self.assertEqual(ev.filter, select.KQ_FILTER_WRITE)
        self.assertEqual(ev.flags, select.KQ_EV_ONESHOT)
        self.assertEqual(ev.fflags, 0)
        self.assertEqual(ev.data, 0)
        self.assertEqual(ev.udata, 0)
        self.assertEqual(ev, ev)
        self.assertNotEqual(ev, other)

        ev = select.kevent(1, 2, 3, 4, 5, 6)
        self.assertEqual(ev.ident, 1)
        self.assertEqual(ev.filter, 2)
        self.assertEqual(ev.flags, 3)
        self.assertEqual(ev.fflags, 4)
        self.assertEqual(ev.data, 5)
        self.assertEqual(ev.udata, 6)
        self.assertEqual(ev, ev)
        self.assertNotEqual(ev, other)

        bignum = 0x7fff
        ev = select.kevent(bignum, 1, 2, 3, bignum - 1, bignum)
        self.assertEqual(ev.ident, bignum)
        self.assertEqual(ev.filter, 1)
        self.assertEqual(ev.flags, 2)
        self.assertEqual(ev.fflags, 3)
        self.assertEqual(ev.data, bignum - 1)
        self.assertEqual(ev.udata, bignum)
        self.assertEqual(ev, ev)
        self.assertNotEqual(ev, other)

        # Issue 11973
        bignum = 0xffff
        ev = select.kevent(0, 1, bignum)
        self.assertEqual(ev.ident, 0)
        self.assertEqual(ev.filter, 1)
        self.assertEqual(ev.flags, bignum)
        self.assertEqual(ev.fflags, 0)
        self.assertEqual(ev.data, 0)
        self.assertEqual(ev.udata, 0)
        self.assertEqual(ev, ev)
        self.assertNotEqual(ev, other)

        # Issue 11973
        bignum = 0xffffffff
        ev = select.kevent(0, 1, 2, bignum)
        self.assertEqual(ev.ident, 0)
        self.assertEqual(ev.filter, 1)
        self.assertEqual(ev.flags, 2)
        self.assertEqual(ev.fflags, bignum)
        self.assertEqual(ev.data, 0)
        self.assertEqual(ev.udata, 0)
        self.assertEqual(ev, ev)
        self.assertNotEqual(ev, other)


    def test_queue_event(self):
        serverSocket = socket.socket()
        serverSocket.bind(('127.0.0.1', 0))
        serverSocket.listen(1)
        client = socket.socket()
        client.setblocking(False)
        try:
            client.connect(('127.0.0.1', serverSocket.getsockname()[1]))
        except socket.error, e:
            self.assertEqual(e.args[0], errno.EINPROGRESS)
        else:
            #raise AssertionError("Connect should have raised EINPROGRESS")
            pass # FreeBSD doesn't raise an exception here
        server, addr = serverSocket.accept()

        kq = select.kqueue()
        kq2 = select.kqueue.fromfd(kq.fileno())

        ev = select.kevent(server.fileno(),
                           select.KQ_FILTER_WRITE,
                           select.KQ_EV_ADD | select.KQ_EV_ENABLE)
        kq.control([ev], 0)
        ev = select.kevent(server.fileno(),
                           select.KQ_FILTER_READ,
                           select.KQ_EV_ADD | select.KQ_EV_ENABLE)
        kq.control([ev], 0)
        ev = select.kevent(client.fileno(),
                           select.KQ_FILTER_WRITE,
                           select.KQ_EV_ADD | select.KQ_EV_ENABLE)
        kq2.control([ev], 0)
        ev = select.kevent(client.fileno(),
                           select.KQ_FILTER_READ,
                           select.KQ_EV_ADD | select.KQ_EV_ENABLE)
        kq2.control([ev], 0)

        events = kq.control(None, 4, 1)
        events = set((e.ident, e.filter) for e in events)
        self.assertEqual(events, set([
            (client.fileno(), select.KQ_FILTER_WRITE),
            (server.fileno(), select.KQ_FILTER_WRITE)]))

        client.send("Hello!")
        server.send("world!!!")

        # We may need to call it several times
        for i in range(10):
            events = kq.control(None, 4, 1)
            if len(events) == 4:
                break
            time.sleep(1.0)
        else:
            self.fail('timeout waiting for event notifications')

        events = set((e.ident, e.filter) for e in events)
        self.assertEqual(events, set([
            (client.fileno(), select.KQ_FILTER_WRITE),
            (client.fileno(), select.KQ_FILTER_READ),
            (server.fileno(), select.KQ_FILTER_WRITE),
            (server.fileno(), select.KQ_FILTER_READ)]))

        # Remove completely client, and server read part
        ev = select.kevent(client.fileno(),
                           select.KQ_FILTER_WRITE,
                           select.KQ_EV_DELETE)
        kq.control([ev], 0)
        ev = select.kevent(client.fileno(),
                           select.KQ_FILTER_READ,
                           select.KQ_EV_DELETE)
        kq.control([ev], 0)
        ev = select.kevent(server.fileno(),
                           select.KQ_FILTER_READ,
                           select.KQ_EV_DELETE)
        kq.control([ev], 0, 0)

        events = kq.control([], 4, 0.99)
        events = set((e.ident, e.filter) for e in events)
        self.assertEqual(events, set([
            (server.fileno(), select.KQ_FILTER_WRITE)]))

        client.close()
        server.close()
        serverSocket.close()

    def testPair(self):
        kq = select.kqueue()
        a, b = socket.socketpair()

        a.send(b'foo')
        event1 = select.kevent(a, select.KQ_FILTER_READ, select.KQ_EV_ADD | select.KQ_EV_ENABLE)
        event2 = select.kevent(b, select.KQ_FILTER_READ, select.KQ_EV_ADD | select.KQ_EV_ENABLE)
        r = kq.control([event1, event2], 1, 1)
        self.assertTrue(r)
        self.assertFalse(r[0].flags & select.KQ_EV_ERROR)
        self.assertEqual(b.recv(r[0].data), b'foo')

        a.close()
        b.close()
        kq.close()

def test_main():
    test_support.run_unittest(TestKQueue)

if __name__ == "__main__":
    test_main()
