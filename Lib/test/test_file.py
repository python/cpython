import sys
import os
from array import array
from weakref import proxy

from test.test_support import verify, TESTFN, TestFailed, findfile
from UserList import UserList

# verify weak references
f = file(TESTFN, 'w')
p = proxy(f)
p.write('teststring')
verify(f.tell(), p.tell())
f.close()
f = None
try:
    p.tell()
except ReferenceError:
    pass
else:
    raise TestFailed('file proxy still exists when the file is gone')

# verify expected attributes exist
f = file(TESTFN, 'w')
softspace = f.softspace
f.name     # merely shouldn't blow up
f.mode     # ditto
f.closed   # ditto

# verify softspace is writable
f.softspace = softspace    # merely shouldn't blow up

# verify the others aren't
for attr in 'name', 'mode', 'closed':
    try:
        setattr(f, attr, 'oops')
    except (AttributeError, TypeError):
        pass
    else:
        raise TestFailed('expected exception setting file attr %r' % attr)
f.close()

# check invalid mode strings
for mode in ("", "aU", "wU+"):
    try:
        f = file(TESTFN, mode)
    except ValueError:
        pass
    else:
        f.close()
        raise TestFailed('%r is an invalid file mode' % mode)

# verify writelines with instance sequence
l = UserList(['1', '2'])
f = open(TESTFN, 'wb')
f.writelines(l)
f.close()
f = open(TESTFN, 'rb')
buf = f.read()
f.close()
verify(buf == '12')

# verify readinto
a = array('c', 'x'*10)
f = open(TESTFN, 'rb')
n = f.readinto(a)
f.close()
verify(buf == a.tostring()[:n])

# verify writelines with integers
f = open(TESTFN, 'wb')
try:
    f.writelines([1, 2, 3])
except TypeError:
    pass
else:
    print "writelines accepted sequence of integers"
f.close()

# verify writelines with integers in UserList
f = open(TESTFN, 'wb')
l = UserList([1,2,3])
try:
    f.writelines(l)
except TypeError:
    pass
else:
    print "writelines accepted sequence of integers"
f.close()

# verify writelines with non-string object
class NonString: pass

f = open(TESTFN, 'wb')
try:
    f.writelines([NonString(), NonString()])
except TypeError:
    pass
else:
    print "writelines accepted sequence of non-string objects"
f.close()

# This causes the interpreter to exit on OSF1 v5.1.
if sys.platform != 'osf1V5':
    try:
        sys.stdin.seek(-1)
    except IOError:
        pass
    else:
        print "should not be able to seek on sys.stdin"
else:
    print >>sys.__stdout__, (
        '  Skipping sys.stdin.seek(-1), it may crash the interpreter.'
        ' Test manually.')

try:
    sys.stdin.truncate()
except IOError:
    pass
else:
    print "should not be able to truncate on sys.stdin"

# verify repr works
f = open(TESTFN)
if not repr(f).startswith("<open file '" + TESTFN):
    print "repr(file) failed"
f.close()

# verify repr works for unicode too
f = open(unicode(TESTFN))
if not repr(f).startswith("<open file u'" + TESTFN):
    print "repr(file with unicode name) failed"
f.close()

# verify that we get a sensible error message for bad mode argument
bad_mode = "qwerty"
try:
    open(TESTFN, bad_mode)
except IOError, msg:
    if msg[0] != 0:
        s = str(msg)
        if s.find(TESTFN) != -1 or s.find(bad_mode) == -1:
            print "bad error message for invalid mode: %s" % s
    # if msg[0] == 0, we're probably on Windows where there may be
    # no obvious way to discover why open() failed.
else:
    print "no error for invalid mode: %s" % bad_mode

f = open(TESTFN)
if f.name != TESTFN:
    raise TestFailed, 'file.name should be "%s"' % TESTFN
if f.isatty():
    raise TestFailed, 'file.isatty() should be false'

if f.closed:
    raise TestFailed, 'file.closed should be false'

try:
    f.readinto("")
except TypeError:
    pass
else:
    raise TestFailed, 'file.readinto("") should raise a TypeError'

f.close()
if not f.closed:
    raise TestFailed, 'file.closed should be true'

# make sure that explicitly setting the buffer size doesn't cause
# misbehaviour especially with repeated close() calls
for s in (-1, 0, 1, 512):
    try:
        f = open(TESTFN, 'w', s)
        f.write(str(s))
        f.close()
        f.close()
        f = open(TESTFN, 'r', s)
        d = int(f.read())
        f.close()
        f.close()
    except IOError, msg:
        raise TestFailed, 'error setting buffer size %d: %s' % (s, str(msg))
    if d != s:
        raise TestFailed, 'readback failure using buffer size %d'

methods = ['fileno', 'flush', 'isatty', 'next', 'read', 'readinto',
           'readline', 'readlines', 'seek', 'tell', 'truncate', 'write',
           'xreadlines', '__iter__']
if sys.platform.startswith('atheos'):
    methods.remove('truncate')

for methodname in methods:
    method = getattr(f, methodname)
    try:
        method()
    except ValueError:
        pass
    else:
        raise TestFailed, 'file.%s() on a closed file should raise a ValueError' % methodname

try:
    f.writelines([])
except ValueError:
    pass
else:
    raise TestFailed, 'file.writelines([]) on a closed file should raise a ValueError'

os.unlink(TESTFN)

def bug801631():
    # SF bug <http://www.python.org/sf/801631>
    # "file.truncate fault on windows"
    f = file(TESTFN, 'wb')
    f.write('12345678901')   # 11 bytes
    f.close()

    f = file(TESTFN,'rb+')
    data = f.read(5)
    if data != '12345':
        raise TestFailed("Read on file opened for update failed %r" % data)
    if f.tell() != 5:
        raise TestFailed("File pos after read wrong %d" % f.tell())

    f.truncate()
    if f.tell() != 5:
        raise TestFailed("File pos after ftruncate wrong %d" % f.tell())

    f.close()
    size = os.path.getsize(TESTFN)
    if size != 5:
        raise TestFailed("File size after ftruncate wrong %d" % size)

try:
    bug801631()
finally:
    os.unlink(TESTFN)

# Test the complex interaction when mixing file-iteration and the various
# read* methods. Ostensibly, the mixture could just be tested to work
# when it should work according to the Python language, instead of fail
# when it should fail according to the current CPython implementation.
# People don't always program Python the way they should, though, and the
# implemenation might change in subtle ways, so we explicitly test for
# errors, too; the test will just have to be updated when the
# implementation changes.
dataoffset = 16384
filler = "ham\n"
assert not dataoffset % len(filler), \
    "dataoffset must be multiple of len(filler)"
nchunks = dataoffset // len(filler)
testlines = [
    "spam, spam and eggs\n",
    "eggs, spam, ham and spam\n",
    "saussages, spam, spam and eggs\n",
    "spam, ham, spam and eggs\n",
    "spam, spam, spam, spam, spam, ham, spam\n",
    "wonderful spaaaaaam.\n"
]
methods = [("readline", ()), ("read", ()), ("readlines", ()),
           ("readinto", (array("c", " "*100),))]

try:
    # Prepare the testfile
    bag = open(TESTFN, "w")
    bag.write(filler * nchunks)
    bag.writelines(testlines)
    bag.close()
    # Test for appropriate errors mixing read* and iteration
    for methodname, args in methods:
        f = open(TESTFN)
        if f.next() != filler:
            raise TestFailed, "Broken testfile"
        meth = getattr(f, methodname)
        try:
            meth(*args)
        except ValueError:
            pass
        else:
            raise TestFailed("%s%r after next() didn't raise ValueError" %
                             (methodname, args))
        f.close()

    # Test to see if harmless (by accident) mixing of read* and iteration
    # still works. This depends on the size of the internal iteration
    # buffer (currently 8192,) but we can test it in a flexible manner.
    # Each line in the bag o' ham is 4 bytes ("h", "a", "m", "\n"), so
    # 4096 lines of that should get us exactly on the buffer boundary for
    # any power-of-2 buffersize between 4 and 16384 (inclusive).
    f = open(TESTFN)
    for i in range(nchunks):
        f.next()
    testline = testlines.pop(0)
    try:
        line = f.readline()
    except ValueError:
        raise TestFailed("readline() after next() with supposedly empty "
                         "iteration-buffer failed anyway")
    if line != testline:
        raise TestFailed("readline() after next() with empty buffer "
                         "failed. Got %r, expected %r" % (line, testline))
    testline = testlines.pop(0)
    buf = array("c", "\x00" * len(testline))
    try:
        f.readinto(buf)
    except ValueError:
        raise TestFailed("readinto() after next() with supposedly empty "
                         "iteration-buffer failed anyway")
    line = buf.tostring()
    if line != testline:
        raise TestFailed("readinto() after next() with empty buffer "
                         "failed. Got %r, expected %r" % (line, testline))

    testline = testlines.pop(0)
    try:
        line = f.read(len(testline))
    except ValueError:
        raise TestFailed("read() after next() with supposedly empty "
                         "iteration-buffer failed anyway")
    if line != testline:
        raise TestFailed("read() after next() with empty buffer "
                         "failed. Got %r, expected %r" % (line, testline))
    try:
        lines = f.readlines()
    except ValueError:
        raise TestFailed("readlines() after next() with supposedly empty "
                         "iteration-buffer failed anyway")
    if lines != testlines:
        raise TestFailed("readlines() after next() with empty buffer "
                         "failed. Got %r, expected %r" % (line, testline))
    # Reading after iteration hit EOF shouldn't hurt either
    f = open(TESTFN)
    try:
        for line in f:
            pass
        try:
            f.readline()
            f.readinto(buf)
            f.read()
            f.readlines()
        except ValueError:
            raise TestFailed("read* failed after next() consumed file")
    finally:
        f.close()
finally:
    os.unlink(TESTFN)
