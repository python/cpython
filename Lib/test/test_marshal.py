from test_support import TestFailed
import marshal
import sys

# XXX Much more needed here.

# Test the full range of Python ints.
n = sys.maxint
while n:
    for expected in (-n, n):
        s = marshal.dumps(expected)
        got = marshal.loads(s)
        if expected != got:
            raise TestFailed("for int %d, marshal string is %r, loaded "
                             "back as %d" % (expected, s, got))
    n = n >> 1

# Simulate int marshaling on a 64-bit box.  This is most interesting if
# we're running the test on a 32-bit box, of course.

def to_little_endian_string(value, nbytes):
    bytes = []
    for i in range(nbytes):
        bytes.append(chr(value & 0xff))
        value >>= 8
    return ''.join(bytes)

maxint64 = (1L << 63) - 1
minint64 = -maxint64-1

for base in maxint64, minint64, -maxint64, -(minint64 >> 1):
    while base:
        s = 'I' + to_little_endian_string(base, 8)
        got = marshal.loads(s)
        if base != got:
            raise TestFailed("for int %d, simulated marshal string is %r, "
                             "loaded back as %d" % (base, s, got))
        if base == -1:  # a fixed-point for shifting right 1
            base = 0
        else:
            base >>= 1

# Simple-minded check for SF 588452: Debug build crashes
marshal.dumps([128] * 1000)
