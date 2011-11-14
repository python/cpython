# Test case for the select.devpoll() function

# Initial tests are copied as is from "test_poll.py"

import os, select, random, unittest, sys
from test.support import TESTFN, run_unittest

try:
    select.devpoll
except AttributeError:
    raise unittest.SkipTest("select.devpoll not defined -- skipping test_devpoll")


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

def test_main():
    run_unittest(DevPollTests)

if __name__ == '__main__':
    test_main()
