# Test case for the os.poll() function

import os, select, random, unittest
import _testcapi
from test.support import TESTFN, run_unittest

try:
    select.poll
except AttributeError:
    raise unittest.SkipTest("select.poll not defined -- skipping test_poll")


def find_ready_matching(ready, flag):
    match = []
    for fd, mode in ready:
        if mode & flag:
            match.append(fd)
    return match

class PollTests(unittest.TestCase):

    def test_poll1(self):
        # Basic functional test of poll object
        # Create a bunch of pipe and test that poll works with them.

        p = select.poll()

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
                raise RuntimeError("no pipes ready for writing")
            wr = random.choice(ready_writers)
            os.write(wr, MSG)

            ready = p.poll()
            ready_readers = find_ready_matching(ready, select.POLLIN)
            if not ready_readers:
                raise RuntimeError("no pipes ready for reading")
            rd = random.choice(ready_readers)
            buf = os.read(rd, MSG_LEN)
            self.assertEqual(len(buf), MSG_LEN)
            bufs.append(buf)
            os.close(r2w[rd]) ; os.close( rd )
            p.unregister( r2w[rd] )
            p.unregister( rd )
            writers.remove(r2w[rd])

        self.assertEqual(bufs, [MSG] * NUM_PIPES)

    def poll_unit_tests(self):
        # returns NVAL for invalid file descriptor
        FD = 42
        try:
            os.close(FD)
        except OSError:
            pass
        p = select.poll()
        p.register(FD)
        r = p.poll()
        self.assertEqual(r[0], (FD, select.POLLNVAL))

        f = open(TESTFN, 'w')
        fd = f.fileno()
        p = select.poll()
        p.register(f)
        r = p.poll()
        self.assertEqual(r[0][0], fd)
        f.close()
        r = p.poll()
        self.assertEqual(r[0], (fd, select.POLLNVAL))
        os.unlink(TESTFN)

        # type error for invalid arguments
        p = select.poll()
        self.assertRaises(TypeError, p.register, p)
        self.assertRaises(TypeError, p.unregister, p)

        # can't unregister non-existent object
        p = select.poll()
        self.assertRaises(KeyError, p.unregister, 3)

        # Test error cases
        pollster = select.poll()
        class Nope:
            pass

        class Almost:
            def fileno(self):
                return 'fileno'

        self.assertRaises(TypeError, pollster.register, Nope(), 0)
        self.assertRaises(TypeError, pollster.register, Almost(), 0)

    # Another test case for poll().  This is copied from the test case for
    # select(), modified to use poll() instead.

    def test_poll2(self):
        cmd = 'for i in 0 1 2 3 4 5 6 7 8 9; do echo testing...; sleep 1; done'
        p = os.popen(cmd, 'r')
        pollster = select.poll()
        pollster.register( p, select.POLLIN )
        for tout in (0, 1000, 2000, 4000, 8000, 16000) + (-1,)*10:
            fdlist = pollster.poll(tout)
            if (fdlist == []):
                continue
            fd, flags = fdlist[0]
            if flags & select.POLLHUP:
                line = p.readline()
                if line != "":
                    self.fail('error: pipe seems to be closed, but still returns data')
                continue

            elif flags & select.POLLIN:
                line = p.readline()
                if not line:
                    break
                continue
            else:
                self.fail('Unexpected return value from select.poll: %s' % fdlist)
        p.close()

    def test_poll3(self):
        # test int overflow
        pollster = select.poll()
        pollster.register(1)

        self.assertRaises(OverflowError, pollster.poll, 1 << 64)

        x = 2 + 3
        if x != 5:
            self.fail('Overflow must have occurred')

        pollster = select.poll()
        # Issue 15989
        self.assertRaises(OverflowError, pollster.register, 0,
                          _testcapi.SHRT_MAX + 1)
        self.assertRaises(OverflowError, pollster.register, 0,
                          _testcapi.USHRT_MAX + 1)
        self.assertRaises(OverflowError, pollster.poll, _testcapi.INT_MAX + 1)
        self.assertRaises(OverflowError, pollster.poll, _testcapi.UINT_MAX + 1)

def test_main():
    run_unittest(PollTests)

if __name__ == '__main__':
    test_main()
