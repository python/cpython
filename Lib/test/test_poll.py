# Test case for the os.poll() function

import sys, os, select, random
from test.test_support import verify, verbose, TestSkipped, TESTFN

try:
    select.poll
except AttributeError:
    raise TestSkipped, "select.poll not defined -- skipping test_poll"


def find_ready_matching(ready, flag):
    match = []
    for fd, mode in ready:
        if mode & flag:
            match.append(fd)
    return match

def test_poll1():
    """Basic functional test of poll object

    Create a bunch of pipe and test that poll works with them.
    """
    print 'Running poll test 1'
    p = select.poll()

    NUM_PIPES = 12
    MSG = " This is a test."
    MSG_LEN = len(MSG)
    readers = []
    writers = []
    r2w = {}
    w2r = {}

    for i in range(NUM_PIPES):
        rd, wr = os.pipe()
        p.register(rd, select.POLLIN)
        p.register(wr, select.POLLOUT)
        readers.append(rd)
        writers.append(wr)
        r2w[rd] = wr
        w2r[wr] = rd

    while writers:
        ready = p.poll()
        ready_writers = find_ready_matching(ready, select.POLLOUT)
        if not ready_writers:
            raise RuntimeError, "no pipes ready for writing"
        wr = random.choice(ready_writers)
        os.write(wr, MSG)

        ready = p.poll()
        ready_readers = find_ready_matching(ready, select.POLLIN)
        if not ready_readers:
            raise RuntimeError, "no pipes ready for reading"
        rd = random.choice(ready_readers)
        buf = os.read(rd, MSG_LEN)
        verify(len(buf) == MSG_LEN)
        print buf
        os.close(r2w[rd]) ; os.close( rd )
        p.unregister( r2w[rd] )
        p.unregister( rd )
        writers.remove(r2w[rd])

    poll_unit_tests()
    print 'Poll test 1 complete'

def poll_unit_tests():
    # returns NVAL for invalid file descriptor
    FD = 42
    try:
        os.close(FD)
    except OSError:
        pass
    p = select.poll()
    p.register(FD)
    r = p.poll()
    verify(r[0] == (FD, select.POLLNVAL))

    f = open(TESTFN, 'w')
    fd = f.fileno()
    p = select.poll()
    p.register(f)
    r = p.poll()
    verify(r[0][0] == fd)
    f.close()
    r = p.poll()
    verify(r[0] == (fd, select.POLLNVAL))
    os.unlink(TESTFN)

    # type error for invalid arguments
    p = select.poll()
    try:
        p.register(p)
    except TypeError:
        pass
    else:
        print "Bogus register call did not raise TypeError"
    try:
        p.unregister(p)
    except TypeError:
        pass
    else:
        print "Bogus unregister call did not raise TypeError"

    # can't unregister non-existent object
    p = select.poll()
    try:
        p.unregister(3)
    except KeyError:
        pass
    else:
        print "Bogus unregister call did not raise KeyError"

    # Test error cases
    pollster = select.poll()
    class Nope:
        pass

    class Almost:
        def fileno(self):
            return 'fileno'

    try:
        pollster.register( Nope(), 0 )
    except TypeError: pass
    else: print 'expected TypeError exception, not raised'

    try:
        pollster.register( Almost(), 0 )
    except TypeError: pass
    else: print 'expected TypeError exception, not raised'


# Another test case for poll().  This is copied from the test case for
# select(), modified to use poll() instead.

def test_poll2():
    print 'Running poll test 2'
    cmd = 'for i in 0 1 2 3 4 5 6 7 8 9; do echo testing...; sleep 1; done'
    p = os.popen(cmd, 'r')
    pollster = select.poll()
    pollster.register( p, select.POLLIN )
    for tout in (0, 1000, 2000, 4000, 8000, 16000) + (-1,)*10:
        if verbose:
            print 'timeout =', tout
        fdlist = pollster.poll(tout)
        if (fdlist == []):
            continue
        fd, flags = fdlist[0]
        if flags & select.POLLHUP:
            line = p.readline()
            if line != "":
                print 'error: pipe seems to be closed, but still returns data'
            continue

        elif flags & select.POLLIN:
            line = p.readline()
            if verbose:
                print repr(line)
            if not line:
                if verbose:
                    print 'EOF'
                break
            continue
        else:
            print 'Unexpected return value from select.poll:', fdlist
    p.close()
    print 'Poll test 2 complete'

test_poll1()
test_poll2()
