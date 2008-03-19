"""Test program for the fcntl C module.

OS/2+EMX doesn't support the file locking operations.

"""
import struct
import fcntl
import os
import struct
import sys
import unittest
from test.test_support import verbose, TESTFN, unlink, run_unittest

# TODO - Write tests for ioctl(), flock() and lockf().

try:
    import termios
except ImportError:
    termios = None

def get_lockdata():
    if sys.platform.startswith('atheos'):
        start_len = "qq"
    else:
        try:
            os.O_LARGEFILE
        except AttributeError:
            start_len = "ll"
        else:
            start_len = "qq"

    if sys.platform in ('netbsd1', 'netbsd2', 'netbsd3',
                        'Darwin1.2', 'darwin',
                        'freebsd2', 'freebsd3', 'freebsd4', 'freebsd5',
                        'freebsd6', 'freebsd7', 'freebsd8',
                        'bsdos2', 'bsdos3', 'bsdos4',
                        'openbsd', 'openbsd2', 'openbsd3', 'openbsd4'):
        if struct.calcsize('l') == 8:
            off_t = 'l'
            pid_t = 'i'
        else:
            off_t = 'lxxxx'
            pid_t = 'l'
        lockdata = struct.pack(off_t + off_t + pid_t + 'hh', 0, 0, 0,
                               fcntl.F_WRLCK, 0)
    elif sys.platform in ['aix3', 'aix4', 'hp-uxB', 'unixware7']:
        lockdata = struct.pack('hhlllii', fcntl.F_WRLCK, 0, 0, 0, 0, 0, 0)
    elif sys.platform in ['os2emx']:
        lockdata = None
    else:
        lockdata = struct.pack('hh'+start_len+'hh', fcntl.F_WRLCK, 0, 0, 0, 0, 0)
    if lockdata:
        if verbose:
            print 'struct.pack: ', repr(lockdata)
    return lockdata

lockdata = get_lockdata()


class TestFcntl(unittest.TestCase):

    def setUp(self):
        self.f = None

    def tearDown(self):
        if not self.f.closed:
            self.f.close()
        unlink(TESTFN)

    def test_fcntl_fileno(self):
        # the example from the library docs
        self.f = open(TESTFN, 'w')
        rv = fcntl.fcntl(self.f.fileno(), fcntl.F_SETFL, os.O_NONBLOCK)
        if verbose:
            print 'Status from fcntl with O_NONBLOCK: ', rv
        if sys.platform not in ['os2emx']:
            rv = fcntl.fcntl(self.f.fileno(), fcntl.F_SETLKW, lockdata)
            if verbose:
                print 'String from fcntl with F_SETLKW: ', repr(rv)
        self.f.close()

    def test_fcntl_file_descriptor(self):
        # again, but pass the file rather than numeric descriptor
        self.f = open(TESTFN, 'w')
        rv = fcntl.fcntl(self.f, fcntl.F_SETFL, os.O_NONBLOCK)
        if sys.platform not in ['os2emx']:
            rv = fcntl.fcntl(self.f, fcntl.F_SETLKW, lockdata)
        self.f.close()


class TestIoctl(unittest.TestCase):
    if termios:
        def test_ioctl_signed_unsigned_code_param(self):
            if termios.TIOCSWINSZ < 0:
                set_winsz_opcode_maybe_neg = termios.TIOCSWINSZ
                set_winsz_opcode_pos = termios.TIOCSWINSZ & 0xffffffffL
            else:
                set_winsz_opcode_pos = termios.TIOCSWINSZ
                set_winsz_opcode_maybe_neg, = struct.unpack("i",
                        struct.pack("I", termios.TIOCSWINSZ))

            # We're just testing that these calls do not raise exceptions.
            saved_winsz = fcntl.ioctl(0, termios.TIOCGWINSZ, "\0"*8)
            our_winsz = struct.pack("HHHH",80,25,0,0)
            # test both with a positive and potentially negative ioctl code
            new_winsz = fcntl.ioctl(0, set_winsz_opcode_pos, our_winsz)
            new_winsz = fcntl.ioctl(0, set_winsz_opcode_maybe_neg, our_winsz)
            fcntl.ioctl(0, set_winsz_opcode_maybe_neg, saved_winsz)


def test_main():
    run_unittest(TestFcntl)
    run_unittest(TestIoctl)

if __name__ == '__main__':
    test_main()
