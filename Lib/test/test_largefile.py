#!python

#----------------------------------------------------------------------
# test largefile support on system where this makes sense
#
#----------------------------------------------------------------------

import test_support
import os, struct, stat, sys

try:
    import signal
    # The default handler for SIGXFSZ is to abort the process.
    # By ignoring it, system calls exceeding the file size resource
    # limit will raise IOError instead of crashing the interpreter.
    oldhandler = signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
except (ImportError, AttributeError):
    pass


# create >2GB file (2GB = 2147483648 bytes)
size = 2500000000L
name = test_support.TESTFN


# On Windows this test comsumes large resources; It takes a long time to build
# the >2GB file and takes >2GB of disk space therefore the resource must be
# enabled to run this test.  If not, nothing after this line stanza will be
# executed.
if sys.platform[:3] == 'win':
    test_support.requires(
        'largefile',
        'test requires %s bytes and a long time to run' % str(size))
else:
    # Only run if the current filesystem supports large files.
    # (Skip this test on Windows, since we now always support large files.)
    f = open(test_support.TESTFN, 'wb')
    try:
        # 2**31 == 2147483648
        f.seek(2147483649L)
        # Seeking is not enough of a test: you must write and flush, too!
        f.write("x")
        f.flush()
    except (IOError, OverflowError):
        f.close()
        os.unlink(test_support.TESTFN)
        raise test_support.TestSkipped, \
              "filesystem does not have largefile support"
    else:
        f.close()


def expect(got_this, expect_this):
    if test_support.verbose:
        print '%r =?= %r ...' % (got_this, expect_this),
    if got_this != expect_this:
        if test_support.verbose:
            print 'no'
        raise test_support.TestFailed, 'got %r, but expected %r' %\
              (got_this, expect_this)
    else:
        if test_support.verbose:
            print 'yes'


# test that each file function works as expected for a large (i.e. >2GB, do
# we have to check >4GB) files

if test_support.verbose:
    print 'create large file via seek (may be sparse file) ...'
f = open(name, 'wb')
f.write('z')
f.seek(0)
f.seek(size)
f.write('a')
f.flush()
if test_support.verbose:
    print 'check file size with os.fstat'
expect(os.fstat(f.fileno())[stat.ST_SIZE], size+1)
f.close()
if test_support.verbose:
    print 'check file size with os.stat'
expect(os.stat(name)[stat.ST_SIZE], size+1)

if test_support.verbose:
    print 'play around with seek() and read() with the built largefile'
f = open(name, 'rb')
expect(f.tell(), 0)
expect(f.read(1), 'z')
expect(f.tell(), 1)
f.seek(0)
expect(f.tell(), 0)
f.seek(0, 0)
expect(f.tell(), 0)
f.seek(42)
expect(f.tell(), 42)
f.seek(42, 0)
expect(f.tell(), 42)
f.seek(42, 1)
expect(f.tell(), 84)
f.seek(0, 1)
expect(f.tell(), 84)
f.seek(0, 2) # seek from the end
expect(f.tell(), size + 1 + 0)
f.seek(-10, 2)
expect(f.tell(), size + 1 - 10)
f.seek(-size-1, 2)
expect(f.tell(), 0)
f.seek(size)
expect(f.tell(), size)
expect(f.read(1), 'a') # the 'a' that was written at the end of the file above
f.seek(-size-1, 1)
expect(f.read(1), 'z')
expect(f.tell(), 1)
f.close()

if test_support.verbose:
    print 'play around with os.lseek() with the built largefile'
f = open(name, 'rb')
expect(os.lseek(f.fileno(), 0, 0), 0)
expect(os.lseek(f.fileno(), 42, 0), 42)
expect(os.lseek(f.fileno(), 42, 1), 84)
expect(os.lseek(f.fileno(), 0, 1), 84)
expect(os.lseek(f.fileno(), 0, 2), size+1+0)
expect(os.lseek(f.fileno(), -10, 2), size+1-10)
expect(os.lseek(f.fileno(), -size-1, 2), 0)
expect(os.lseek(f.fileno(), size, 0), size)
expect(f.read(1), 'a') # the 'a' that was written at the end of the file above
f.close()

if hasattr(f, 'truncate'):
    if test_support.verbose:
        print 'try truncate'
    f = open(name, 'r+b')
    f.seek(0, 2)
    expect(f.tell(), size+1)
    # Cut it back via seek + truncate with no argument.
    newsize = size - 10
    f.seek(newsize)
    f.truncate()
    expect(f.tell(), newsize)
    # Ensure that truncate(bigger than true size) doesn't grow the file.
    f.truncate(size)
    expect(f.tell(), newsize)
    # Ensure that truncate(smaller than true size) shrinks the file.
    newsize -= 1
    f.seek(0)
    f.truncate(newsize)
    expect(f.tell(), newsize)
    # cut it waaaaay back
    f.truncate(1)
    f.seek(0)
    expect(len(f.read()), 1)
    f.close()

os.unlink(name)
