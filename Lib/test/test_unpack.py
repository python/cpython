from test_support import *

t = (1, 2, 3)
l = [4, 5, 6]

class Seq:
    def __getitem__(self, i):
        if i >= 0 and i < 3: return i
        raise IndexError

a = -1
b = -1
c = -1

# unpack tuple
if verbose:
    print 'unpack tuple'
a, b, c = t
if a <> 1 or b <> 2 or c <> 3:
    raise TestFailed

# unpack list
if verbose:
    print 'unpack list'
a, b, c = l
if a <> 4 or b <> 5 or c <> 6:
    raise TestFailed

# unpack implied tuple
if verbose:
    print 'unpack implied tuple'
a, b, c = 7, 8, 9
if a <> 7 or b <> 8 or c <> 9:
    raise TestFailed

# unpack string... fun!
if verbose:
    print 'unpack string'
a, b, c = 'one'
if a <> 'o' or b <> 'n' or c <> 'e':
    raise TestFailed

# unpack generic sequence
if verbose:
    print 'unpack sequence'
a, b, c = Seq()
if a <> 0 or b <> 1 or c <> 2:
    raise TestFailed

# single element unpacking, with extra syntax
if verbose:
    print 'unpack single tuple/list'
st = (99,)
sl = [100]
a, = st
if a <> 99:
    raise TestFailed
b, = sl
if b <> 100:
    raise TestFailed

# now for some failures

# unpacking non-sequence
if verbose:
    print 'unpack non-sequence'
try:
    a, b, c = 7
    raise TestFailed
except TypeError:
    pass


# unpacking tuple of wrong size
if verbose:
    print 'unpack tuple wrong size'
try:
    a, b = t
    raise TestFailed
except ValueError:
    pass

# unpacking list of wrong size
if verbose:
    print 'unpack list wrong size'
try:
    a, b = l
    raise TestFailed
except ValueError:
    pass


# unpacking sequence too short
if verbose:
    print 'unpack sequence too short'
try:
    a, b, c, d = Seq()
    raise TestFailed
except ValueError:
    pass


# unpacking sequence too long
if verbose:
    print 'unpack sequence too long'
try:
    a, b = Seq()
    raise TestFailed
except ValueError:
    pass


# unpacking a sequence where the test for too long raises a different
# kind of error
class BozoError(Exception):
    pass

class BadSeq:
    def __getitem__(self, i):
        if i >= 0 and i < 3:
            return i
        elif i == 3:
            raise BozoError
        else:
            raise IndexError


# trigger code while not expecting an IndexError
if verbose:
    print 'unpack sequence too long, wrong error'
try:
    a, b, c, d, e = BadSeq()
    raise TestFailed
except BozoError:
    pass

# trigger code while expecting an IndexError
if verbose:
    print 'unpack sequence too short, wrong error'
try:
    a, b, c = BadSeq()
    raise TestFailed
except BozoError:
    pass
