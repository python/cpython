from test.test_support import verify, vereq, TESTFN
import mmap
import os, re

PAGESIZE = mmap.PAGESIZE

def test_both():
    "Test mmap module on Unix systems and Windows"

    # Create a file to be mmap'ed.
    if os.path.exists(TESTFN):
        os.unlink(TESTFN)
    f = open(TESTFN, 'w+')

    try:    # unlink TESTFN no matter what
        # Write 2 pages worth of data to the file
        f.write('\0'* PAGESIZE)
        f.write('foo')
        f.write('\0'* (PAGESIZE-3) )
        f.flush()
        m = mmap.mmap(f.fileno(), 2 * PAGESIZE)
        f.close()

        # Simple sanity checks

        print type(m)  # SF bug 128713:  segfaulted on Linux
        print '  Position of foo:', m.find('foo') / float(PAGESIZE), 'pages'
        vereq(m.find('foo'), PAGESIZE)

        print '  Length of file:', len(m) / float(PAGESIZE), 'pages'
        vereq(len(m), 2*PAGESIZE)

        print '  Contents of byte 0:', repr(m[0])
        vereq(m[0], '\0')
        print '  Contents of first 3 bytes:', repr(m[0:3])
        vereq(m[0:3], '\0\0\0')

        # Modify the file's content
        print "\n  Modifying file's content..."
        m[0] = '3'
        m[PAGESIZE +3: PAGESIZE +3+3] = 'bar'

        # Check that the modification worked
        print '  Contents of byte 0:', repr(m[0])
        vereq(m[0], '3')
        print '  Contents of first 3 bytes:', repr(m[0:3])
        vereq(m[0:3], '3\0\0')
        print '  Contents of second page:',  repr(m[PAGESIZE-1 : PAGESIZE + 7])
        vereq(m[PAGESIZE-1 : PAGESIZE + 7], '\0foobar\0')

        m.flush()

        # Test doing a regular expression match in an mmap'ed file
        match = re.search('[A-Za-z]+', m)
        if match is None:
            print '  ERROR: regex match on mmap failed!'
        else:
            start, end = match.span(0)
            length = end - start

            print '  Regex match on mmap (page start, length of match):',
            print start / float(PAGESIZE), length

            vereq(start, PAGESIZE)
            vereq(end, PAGESIZE + 6)

        # test seeking around (try to overflow the seek implementation)
        m.seek(0,0)
        print '  Seek to zeroth byte'
        vereq(m.tell(), 0)
        m.seek(42,1)
        print '  Seek to 42nd byte'
        vereq(m.tell(), 42)
        m.seek(0,2)
        print '  Seek to last byte'
        vereq(m.tell(), len(m))

        print '  Try to seek to negative position...'
        try:
            m.seek(-1)
        except ValueError:
            pass
        else:
            verify(0, 'expected a ValueError but did not get it')

        print '  Try to seek beyond end of mmap...'
        try:
            m.seek(1,2)
        except ValueError:
            pass
        else:
            verify(0, 'expected a ValueError but did not get it')

        print '  Try to seek to negative position...'
        try:
            m.seek(-len(m)-1,2)
        except ValueError:
            pass
        else:
            verify(0, 'expected a ValueError but did not get it')

        # Try resizing map
        print '  Attempting resize()'
        try:
            m.resize(512)
        except SystemError:
            # resize() not supported
            # No messages are printed, since the output of this test suite
            # would then be different across platforms.
            pass
        else:
            # resize() is supported
            verify(len(m) == 512,
                    "len(m) is %d, but expecting 512" % (len(m),) )
            # Check that we can no longer seek beyond the new size.
            try:
                m.seek(513,0)
            except ValueError:
                pass
            else:
                verify(0, 'Could seek beyond the new size')

            # Check that the underlying file is truncated too
            # (bug #728515)
            f = open(TESTFN)
            f.seek(0, 2)
            verify(f.tell() == 512, 'Underlying file not truncated')
            f.close()
            verify(m.size() == 512, 'New size not reflected in file')

        m.close()

    finally:
        try:
            f.close()
        except OSError:
            pass
        try:
            os.unlink(TESTFN)
        except OSError:
            pass

    # Test for "access" keyword parameter
    try:
        mapsize = 10
        print "  Creating", mapsize, "byte test data file."
        open(TESTFN, "wb").write("a"*mapsize)
        print "  Opening mmap with access=ACCESS_READ"
        f = open(TESTFN, "rb")
        m = mmap.mmap(f.fileno(), mapsize, access=mmap.ACCESS_READ)
        verify(m[:] == 'a'*mapsize, "Readonly memory map data incorrect.")

        print "  Ensuring that readonly mmap can't be slice assigned."
        try:
            m[:] = 'b'*mapsize
        except TypeError:
            pass
        else:
            verify(0, "Able to write to readonly memory map")

        print "  Ensuring that readonly mmap can't be item assigned."
        try:
            m[0] = 'b'
        except TypeError:
            pass
        else:
            verify(0, "Able to write to readonly memory map")

        print "  Ensuring that readonly mmap can't be write() to."
        try:
            m.seek(0,0)
            m.write('abc')
        except TypeError:
            pass
        else:
            verify(0, "Able to write to readonly memory map")

        print "  Ensuring that readonly mmap can't be write_byte() to."
        try:
            m.seek(0,0)
            m.write_byte('d')
        except TypeError:
            pass
        else:
            verify(0, "Able to write to readonly memory map")

        print "  Ensuring that readonly mmap can't be resized."
        try:
            m.resize(2*mapsize)
        except SystemError:   # resize is not universally supported
            pass
        except TypeError:
            pass
        else:
            verify(0, "Able to resize readonly memory map")
        del m, f
        verify(open(TESTFN, "rb").read() == 'a'*mapsize,
               "Readonly memory map data file was modified")

        print "  Opening mmap with size too big"
        import sys
        f = open(TESTFN, "r+b")
        try:
            m = mmap.mmap(f.fileno(), mapsize+1)
        except ValueError:
            # we do not expect a ValueError on Windows
            # CAUTION:  This also changes the size of the file on disk, and
            # later tests assume that the length hasn't changed.  We need to
            # repair that.
            if sys.platform.startswith('win'):
                verify(0, "Opening mmap with size+1 should work on Windows.")
        else:
            # we expect a ValueError on Unix, but not on Windows
            if not sys.platform.startswith('win'):
                verify(0, "Opening mmap with size+1 should raise ValueError.")
            m.close()
        f.close()
        if sys.platform.startswith('win'):
            # Repair damage from the resizing test.
            f = open(TESTFN, 'r+b')
            f.truncate(mapsize)
            f.close()

        print "  Opening mmap with access=ACCESS_WRITE"
        f = open(TESTFN, "r+b")
        m = mmap.mmap(f.fileno(), mapsize, access=mmap.ACCESS_WRITE)
        print "  Modifying write-through memory map."
        m[:] = 'c'*mapsize
        verify(m[:] == 'c'*mapsize,
               "Write-through memory map memory not updated properly.")
        m.flush()
        m.close()
        f.close()
        f = open(TESTFN, 'rb')
        stuff = f.read()
        f.close()
        verify(stuff == 'c'*mapsize,
               "Write-through memory map data file not updated properly.")

        print "  Opening mmap with access=ACCESS_COPY"
        f = open(TESTFN, "r+b")
        m = mmap.mmap(f.fileno(), mapsize, access=mmap.ACCESS_COPY)
        print "  Modifying copy-on-write memory map."
        m[:] = 'd'*mapsize
        verify(m[:] == 'd' * mapsize,
               "Copy-on-write memory map data not written correctly.")
        m.flush()
        verify(open(TESTFN, "rb").read() == 'c'*mapsize,
               "Copy-on-write test data file should not be modified.")
        try:
            print "  Ensuring copy-on-write maps cannot be resized."
            m.resize(2*mapsize)
        except TypeError:
            pass
        else:
            verify(0, "Copy-on-write mmap resize did not raise exception.")
        del m, f
        try:
            print "  Ensuring invalid access parameter raises exception."
            f = open(TESTFN, "r+b")
            m = mmap.mmap(f.fileno(), mapsize, access=4)
        except ValueError:
            pass
        else:
            verify(0, "Invalid access code should have raised exception.")

        if os.name == "posix":
            # Try incompatible flags, prot and access parameters.
            f = open(TESTFN, "r+b")
            try:
                m = mmap.mmap(f.fileno(), mapsize, flags=mmap.MAP_PRIVATE,
                              prot=mmap.PROT_READ, access=mmap.ACCESS_WRITE)
            except ValueError:
                pass
            else:
                verify(0, "Incompatible parameters should raise ValueError.")
            f.close()
    finally:
        try:
            os.unlink(TESTFN)
        except OSError:
            pass

    print '  Try opening a bad file descriptor...'
    try:
        mmap.mmap(-2, 4096)
    except mmap.error:
        pass
    else:
        verify(0, 'expected a mmap.error but did not get it')

    # Do a tougher .find() test.  SF bug 515943 pointed out that, in 2.2,
    # searching for data with embedded \0 bytes didn't work.
    f = open(TESTFN, 'w+')

    try:    # unlink TESTFN no matter what
        data = 'aabaac\x00deef\x00\x00aa\x00'
        n = len(data)
        f.write(data)
        f.flush()
        m = mmap.mmap(f.fileno(), n)
        f.close()

        for start in range(n+1):
            for finish in range(start, n+1):
                slice = data[start : finish]
                vereq(m.find(slice), data.find(slice))
                vereq(m.find(slice + 'x'), -1)
        m.close()

    finally:
        os.unlink(TESTFN)

    # make sure a double close doesn't crash on Solaris (Bug# 665913)
    f = open(TESTFN, 'w+')

    try:    # unlink TESTFN no matter what
        f.write(2**16 * 'a') # Arbitrary character
        f.close()

        f = open(TESTFN)
        mf = mmap.mmap(f.fileno(), 2**16, access=mmap.ACCESS_READ)
        mf.close()
        mf.close()
        f.close()

    finally:
        os.unlink(TESTFN)

    # test mapping of entire file by passing 0 for map length
    if hasattr(os, "stat"):
        print "  Ensuring that passing 0 as map length sets map size to current file size."
        f = open(TESTFN, "w+")

        try:
            f.write(2**16 * 'm') # Arbitrary character
            f.close()

            f = open(TESTFN, "rb+")
            mf = mmap.mmap(f.fileno(), 0)
            verify(len(mf) == 2**16, "Map size should equal file size.")
            vereq(mf.read(2**16), 2**16 * "m")
            mf.close()
            f.close()

        finally:
            os.unlink(TESTFN)

    # test mapping of entire file by passing 0 for map length
    if hasattr(os, "stat"):
        print "  Ensuring that passing 0 as map length sets map size to current file size."
        f = open(TESTFN, "w+")
        try:
            f.write(2**16 * 'm') # Arbitrary character
            f.close()

            f = open(TESTFN, "rb+")
            mf = mmap.mmap(f.fileno(), 0)
            verify(len(mf) == 2**16, "Map size should equal file size.")
            vereq(mf.read(2**16), 2**16 * "m")
            mf.close()
            f.close()

        finally:
            os.unlink(TESTFN)

    # make move works everywhere (64-bit format problem earlier)
    f = open(TESTFN, 'w+')

    try:    # unlink TESTFN no matter what
        f.write("ABCDEabcde") # Arbitrary character
        f.flush()

        mf = mmap.mmap(f.fileno(), 10)
        mf.move(5, 0, 5)
        verify(mf[:] == "ABCDEABCDE", "Map move should have duplicated front 5")
        mf.close()
        f.close()

    finally:
        os.unlink(TESTFN)

def test_anon():
    print "  anonymous mmap.mmap(-1, PAGESIZE)..."
    m = mmap.mmap(-1, PAGESIZE)
    for x in xrange(PAGESIZE):
        verify(m[x] == '\0', "anonymously mmap'ed contents should be zero")

    for x in xrange(PAGESIZE):
        m[x] = ch = chr(x & 255)
        vereq(m[x], ch)

test_both()
test_anon()
print ' Test passed'
