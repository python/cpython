# Test case for the select.devpoll() function

# Initial tests are copied as is from "test_poll.py"

import os
import random
import select
import unittest
from test.support import cpython_only

if not hasattr(select, 'devpoll') :
    raise unittest.SkipTest('test works only on Solaris OS family')


def find_ready_matching(ready, flag):
    match = []
    for fd, mode in ready:
        if mode & flag:
            match.append(fd)
    return match

class DevPollTests(unittest.TestCase):

    def test_devpoll1(self):
        # Basic functional test of poll object
        # Create a bunch of pipe and test that poll works with them.

        p = select.devpoll()

        NUM_PIPES = 12
        MSG = b" This is a test."
        MSG_LEN = len(MSG)
        readers = []
        writers = []
        r2w = {}
        w2r = {}

        for i in range(NUM_PIPES):
            rd, wr = os.pipe()
            p.register(rd)
            p.modify(rd, select.POLLIN)
            p.register(wr, select.POLLOUT)
            readers.append(rd)
            writers.append(wr)
            r2w[rd] = wr
            w2r[wr] = rd

        bufs = []

        while writers:
            ready = p.poll()
            ready_writers = find_ready_matching(ready, select.POLLOUT)
            if not ready_writers:
                self.fail("no pipes ready for writing")
            wr = random.choice(ready_writers)
            os.write(wr, MSG)

            ready = p.poll()
            ready_readers = find_ready_matching(ready, select.POLLIN)
            if not ready_readers:
                self.fail("no pipes ready for reading")
            self.assertEqual([w2r[wr]], ready_readers)
            rd = ready_readers[0]
            buf = os.read(rd, MSG_LEN)
            self.assertEqual(len(buf), MSG_LEN)
            bufs.append(buf)
            os.close(r2w[rd]) ; os.close(rd)
            p.unregister(r2w[rd])
            p.unregister(rd)
            writers.remove(r2w[rd])

        self.assertEqual(bufs, [MSG] * NUM_PIPES)

    def test_timeout_overflow(self):
        pollster = select.devpoll()
        w, r = os.pipe()
        pollster.register(w)

        pollster.poll(-1)
        self.assertRaises(OverflowError, pollster.poll, -2)
        self.assertRaises(OverflowError, pollster.poll, -1 << 31)
        self.assertRaises(OverflowError, pollster.poll, -1 << 64)

        pollster.poll(0)
        pollster.poll(1)
        pollster.poll(1 << 30)
        self.assertRaises(OverflowError, pollster.poll, 1 << 31)
        self.assertRaises(OverflowError, pollster.poll, 1 << 63)
        self.assertRaises(OverflowError, pollster.poll, 1 << 64)

    def test_close(self):
        open_file = open(__file__, "rb")
        self.addCleanup(open_file.close)
        fd = open_file.fileno()
        devpoll = select.devpoll()

        # test fileno() method and closed attribute
        self.assertIsInstance(devpoll.fileno(), int)
        self.assertFalse(devpoll.closed)

        # test close()
        devpoll.close()
        self.assertTrue(devpoll.closed)
        self.assertRaises(ValueError, devpoll.fileno)

        # close() can be called more than once
        devpoll.close()

        # operations must fail with ValueError("I/O operation on closed ...")
        self.assertRaises(ValueError, devpoll.modify, fd, select.POLLIN)
        self.assertRaises(ValueError, devpoll.poll)
        self.assertRaises(ValueError, devpoll.register, fd, select.POLLIN)
        self.assertRaises(ValueError, devpoll.unregister, fd)

    def test_fd_non_inheritable(self):
        devpoll = select.devpoll()
        self.addCleanup(devpoll.close)
        self.assertEqual(os.get_inheritable(devpoll.fileno()), False)

    def test_events_mask_overflow(self):
        pollster = select.devpoll()
        w, r = os.pipe()
        pollster.register(w)
        # Issue #17919
        self.assertRaises(ValueError, pollster.register, 0, -1)
        self.assertRaises(OverflowError, pollster.register, 0, 1 << 64)
        self.assertRaises(ValueError, pollster.modify, 1, -1)
        self.assertRaises(OverflowError, pollster.modify, 1, 1 << 64)

    @cpython_only
    def test_events_mask_overflow_c_limits(self):
        from _testcapi import USHRT_MAX
        pollster = select.devpoll()
        w, r = os.pipe()
        pollster.register(w)
        # Issue #17919
        self.assertRaises(OverflowError, pollster.register, 0, USHRT_MAX + 1)
        self.assertRaises(OverflowError, pollster.modify, 1, USHRT_MAX + 1)


if __name__ == '__main__':
    unittest.main()
