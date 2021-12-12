from test.support import (requires, _2G, _4G, gc_collect, cpython_only)
from test.support.import_helper import import_module
from test.support.os_helper import TESTFN, unlink
import unittest
import os
import re
import itertools
import random
import socket
import string
import sys
import weakref

# Skip test if we can't import mmap.
mmap = import_module('mmap')

PAGESIZE = mmap.PAGESIZE

tagname_prefix = f'python_{os.getpid()}_test_mmap'
def random_tagname(length=10):
    suffix = ''.join(random.choices(string.ascii_uppercase, k=length))
    return f'{tagname_prefix}_{suffix}'

class MmapTests(unittest.TestCase):

    def setUp(self):
        if os.path.exists(TESTFN):
            os.unlink(TESTFN)

    def tearDown(self):
        try:
            os.unlink(TESTFN)
        except OSError:
            pass

    def test_basic(self):
        # Test mmap module on Unix systems and Windows

        # Create a file to be mmap'ed.
        f = open(TESTFN, 'bw+')
        try:
            # Write 2 pages worth of data to the file
            f.write(b'\0'* PAGESIZE)
            f.write(b'foo')
            f.write(b'\0'* (PAGESIZE-3) )
            f.flush()
            m = mmap.mmap(f.fileno(), 2 * PAGESIZE)
        finally:
            f.close()

        # Simple sanity checks

        tp = str(type(m))  # SF bug 128713:  segfaulted on Linux
        self.assertEqual(m.find(b'foo'), PAGESIZE)

        self.assertEqual(len(m), 2*PAGESIZE)

        self.assertEqual(m[0], 0)
        self.assertEqual(m[0:3], b'\0\0\0')

        # Shouldn't crash on boundary (Issue #5292)
        self.assertRaises(IndexError, m.__getitem__, len(m))
        self.assertRaises(IndexError, m.__setitem__, len(m), b'\0')

        # Modify the file's content
        m[0] = b'3'[0]
        m[PAGESIZE +3: PAGESIZE +3+3] = b'bar'

        # Check that the modification worked
        self.assertEqual(m[0], b'3'[0])
        self.assertEqual(m[0:3], b'3\0\0')
        self.assertEqual(m[PAGESIZE-1 : PAGESIZE + 7], b'\0foobar\0')

        m.flush()

        # Test doing a regular expression match in an mmap'ed file
        match = re.search(b'[A-Za-z]+', m)
        if match is None:
            self.fail('regex match on mmap failed!')
        else:
            start, end = match.span(0)
            length = end - start

            self.assertEqual(start, PAGESIZE)
            self.assertEqual(end, PAGESIZE + 6)

        # test seeking around (try to overflow the seek implementation)
        m.seek(0,0)
        self.assertEqual(m.tell(), 0)
        m.seek(42,1)
        self.assertEqual(m.tell(), 42)
        m.seek(0,2)
        self.assertEqual(m.tell(), len(m))

        # Try to seek to negative position...
        self.assertRaises(ValueError, m.seek, -1)

        # Try to seek beyond end of mmap...
        self.assertRaises(ValueError, m.seek, 1, 2)

        # Try to seek to negative position...
        self.assertRaises(ValueError, m.seek, -len(m)-1, 2)

        # Try resizing map
        try:
            m.resize(512)
        except SystemError:
            # resize() not supported
            # No messages are printed, since the output of this test suite
            # would then be different across platforms.
            pass
        else:
            # resize() is supported
            self.assertEqual(len(m), 512)
            # Check that we can no longer seek beyond the new size.
            self.assertRaises(ValueError, m.seek, 513, 0)

            # Check that the underlying file is truncated too
            # (bug #728515)
            f = open(TESTFN, 'rb')
            try:
                f.seek(0, 2)
                self.assertEqual(f.tell(), 512)
            finally:
                f.close()
            self.assertEqual(m.size(), 512)

        m.close()

    def test_access_parameter(self):
        # Test for "access" keyword parameter
        mapsize = 10
        with open(TESTFN, "wb") as fp:
            fp.write(b"a"*mapsize)
        with open(TESTFN, "rb") as f:
            m = mmap.mmap(f.fileno(), mapsize, access=mmap.ACCESS_READ)
            self.assertEqual(m[:], b'a'*mapsize, "Readonly memory map data incorrect.")

            # Ensuring that readonly mmap can't be slice assigned
            try:
                m[:] = b'b'*mapsize
            except TypeError:
                pass
            else:
                self.fail("Able to write to readonly memory map")

            # Ensuring that readonly mmap can't be item assigned
            try:
                m[0] = b'b'
            except TypeError:
                pass
            else:
                self.fail("Able to write to readonly memory map")

            # Ensuring that readonly mmap can't be write() to
            try:
                m.seek(0,0)
                m.write(b'abc')
            except TypeError:
                pass
            else:
                self.fail("Able to write to readonly memory map")

            # Ensuring that readonly mmap can't be write_byte() to
            try:
                m.seek(0,0)
                m.write_byte(b'd')
            except TypeError:
                pass
            else:
                self.fail("Able to write to readonly memory map")

            # Ensuring that readonly mmap can't be resized
            try:
                m.resize(2*mapsize)
            except SystemError:   # resize is not universally supported
                pass
            except TypeError:
                pass
            else:
                self.fail("Able to resize readonly memory map")
            with open(TESTFN, "rb") as fp:
                self.assertEqual(fp.read(), b'a'*mapsize,
                                 "Readonly memory map data file was modified")

        # Opening mmap with size too big
        with open(TESTFN, "r+b") as f:
            try:
                m = mmap.mmap(f.fileno(), mapsize+1)
            except ValueError:
                # we do not expect a ValueError on Windows
                # CAUTION:  This also changes the size of the file on disk, and
                # later tests assume that the length hasn't changed.  We need to
                # repair that.
                if sys.platform.startswith('win'):
                    self.fail("Opening mmap with size+1 should work on Windows.")
            else:
                # we expect a ValueError on Unix, but not on Windows
                if not sys.platform.startswith('win'):
                    self.fail("Opening mmap with size+1 should raise ValueError.")
                m.close()
            if sys.platform.startswith('win'):
                # Repair damage from the resizing test.
                with open(TESTFN, 'r+b') as f:
                    f.truncate(mapsize)

        # Opening mmap with access=ACCESS_WRITE
        with open(TESTFN, "r+b") as f:
            m = mmap.mmap(f.fileno(), mapsize, access=mmap.ACCESS_WRITE)
            # Modifying write-through memory map
            m[:] = b'c'*mapsize
            self.assertEqual(m[:], b'c'*mapsize,
                   "Write-through memory map memory not updated properly.")
            m.flush()
            m.close()
        with open(TESTFN, 'rb') as f:
            stuff = f.read()
        self.assertEqual(stuff, b'c'*mapsize,
               "Write-through memory map data file not updated properly.")

        # Opening mmap with access=ACCESS_COPY
        with open(TESTFN, "r+b") as f:
            m = mmap.mmap(f.fileno(), mapsize, access=mmap.ACCESS_COPY)
            # Modifying copy-on-write memory map
            m[:] = b'd'*mapsize
            self.assertEqual(m[:], b'd' * mapsize,
                             "Copy-on-write memory map data not written correctly.")
            m.flush()
            with open(TESTFN, "rb") as fp:
                self.assertEqual(fp.read(), b'c'*mapsize,
                                 "Copy-on-write test data file should not be modified.")
            # Ensuring copy-on-write maps cannot be resized
            self.assertRaises(TypeError, m.resize, 2*mapsize)
            m.close()

        # Ensuring invalid access parameter raises exception
        with open(TESTFN, "r+b") as f:
            self.assertRaises(ValueError, mmap.mmap, f.fileno(), mapsize, access=4)

        if os.name == "posix":
            # Try incompatible flags, prot and access parameters.
            with open(TESTFN, "r+b") as f:
                self.assertRaises(ValueError, mmap.mmap, f.fileno(), mapsize,
                                  flags=mmap.MAP_PRIVATE,
                                  prot=mmap.PROT_READ, access=mmap.ACCESS_WRITE)

            # Try writing with PROT_EXEC and without PROT_WRITE
            prot = mmap.PROT_READ | getattr(mmap, 'PROT_EXEC', 0)
            with open(TESTFN, "r+b") as f:
                m = mmap.mmap(f.fileno(), mapsize, prot=prot)
                self.assertRaises(TypeError, m.write, b"abcdef")
                self.assertRaises(TypeError, m.write_byte, 0)
                m.close()

    def test_bad_file_desc(self):
        # Try opening a bad file descriptor...
        self.assertRaises(OSError, mmap.mmap, -2, 4096)

    def test_tougher_find(self):
        # Do a tougher .find() test.  SF bug 515943 pointed out that, in 2.2,
        # searching for data with embedded \0 bytes didn't work.
        with open(TESTFN, 'wb+') as f:

            data = b'aabaac\x00deef\x00\x00aa\x00'
            n = len(data)
            f.write(data)
            f.flush()
            m = mmap.mmap(f.fileno(), n)

        for start in range(n+1):
            for finish in range(start, n+1):
                slice = data[start : finish]
                self.assertEqual(m.find(slice), data.find(slice))
                self.assertEqual(m.find(slice + b'x'), -1)
        m.close()

    def test_find_end(self):
        # test the new 'end' parameter works as expected
        with open(TESTFN, 'wb+') as f:
            data = b'one two ones'
            n = len(data)
            f.write(data)
            f.flush()
            m = mmap.mmap(f.fileno(), n)

        self.assertEqual(m.find(b'one'), 0)
        self.assertEqual(m.find(b'ones'), 8)
        self.assertEqual(m.find(b'one', 0, -1), 0)
        self.assertEqual(m.find(b'one', 1), 8)
        self.assertEqual(m.find(b'one', 1, -1), 8)
        self.assertEqual(m.find(b'one', 1, -2), -1)
        self.assertEqual(m.find(bytearray(b'one')), 0)


    def test_rfind(self):
        # test the new 'end' parameter works as expected
        with open(TESTFN, 'wb+') as f:
            data = b'one two ones'
            n = len(data)
            f.write(data)
            f.flush()
            m = mmap.mmap(f.fileno(), n)

        self.assertEqual(m.rfind(b'one'), 8)
        self.assertEqual(m.rfind(b'one '), 0)
        self.assertEqual(m.rfind(b'one', 0, -1), 8)
        self.assertEqual(m.rfind(b'one', 0, -2), 0)
        self.assertEqual(m.rfind(b'one', 1, -1), 8)
        self.assertEqual(m.rfind(b'one', 1, -2), -1)
        self.assertEqual(m.rfind(bytearray(b'one')), 8)


    def test_double_close(self):
        # make sure a double close doesn't crash on Solaris (Bug# 665913)
        with open(TESTFN, 'wb+') as f:
            f.write(2**16 * b'a') # Arbitrary character

        with open(TESTFN, 'rb') as f:
            mf = mmap.mmap(f.fileno(), 2**16, access=mmap.ACCESS_READ)
            mf.close()
            mf.close()

    def test_entire_file(self):
        # test mapping of entire file by passing 0 for map length
        with open(TESTFN, "wb+") as f:
            f.write(2**16 * b'm') # Arbitrary character

        with open(TESTFN, "rb+") as f, \
             mmap.mmap(f.fileno(), 0) as mf:
            self.assertEqual(len(mf), 2**16, "Map size should equal file size.")
            self.assertEqual(mf.read(2**16), 2**16 * b"m")

    def test_length_0_offset(self):
        # Issue #10916: test mapping of remainder of file by passing 0 for
        # map length with an offset doesn't cause a segfault.
        # NOTE: allocation granularity is currently 65536 under Win64,
        # and therefore the minimum offset alignment.
        with open(TESTFN, "wb") as f:
            f.write((65536 * 2) * b'm') # Arbitrary character

        with open(TESTFN, "rb") as f:
            with mmap.mmap(f.fileno(), 0, offset=65536, access=mmap.ACCESS_READ) as mf:
                self.assertRaises(IndexError, mf.__getitem__, 80000)

    def test_length_0_large_offset(self):
        # Issue #10959: test mapping of a file by passing 0 for
        # map length with a large offset doesn't cause a segfault.
        with open(TESTFN, "wb") as f:
            f.write(115699 * b'm') # Arbitrary character

        with open(TESTFN, "w+b") as f:
            self.assertRaises(ValueError, mmap.mmap, f.fileno(), 0,
                              offset=2147418112)

    def test_move(self):
        # make move works everywhere (64-bit format problem earlier)
        with open(TESTFN, 'wb+') as f:

            f.write(b"ABCDEabcde") # Arbitrary character
            f.flush()

            mf = mmap.mmap(f.fileno(), 10)
            mf.move(5, 0, 5)
            self.assertEqual(mf[:], b"ABCDEABCDE", "Map move should have duplicated front 5")
            mf.close()

        # more excessive test
        data = b"0123456789"
        for dest in range(len(data)):
            for src in range(len(data)):
                for count in range(len(data) - max(dest, src)):
                    expected = data[:dest] + data[src:src+count] + data[dest+count:]
                    m = mmap.mmap(-1, len(data))
                    m[:] = data
                    m.move(dest, src, count)
                    self.assertEqual(m[:], expected)
                    m.close()

        # segfault test (Issue 5387)
        m = mmap.mmap(-1, 100)
        offsets = [-100, -1, 0, 1, 100]
        for source, dest, size in itertools.product(offsets, offsets, offsets):
            try:
                m.move(source, dest, size)
            except ValueError:
                pass

        offsets = [(-1, -1, -1), (-1, -1, 0), (-1, 0, -1), (0, -1, -1),
                   (-1, 0, 0), (0, -1, 0), (0, 0, -1)]
        for source, dest, size in offsets:
            self.assertRaises(ValueError, m.move, source, dest, size)

        m.close()

        m = mmap.mmap(-1, 1) # single byte
        self.assertRaises(ValueError, m.move, 0, 0, 2)
        self.assertRaises(ValueError, m.move, 1, 0, 1)
        self.assertRaises(ValueError, m.move, 0, 1, 1)
        m.move(0, 0, 1)
        m.move(0, 0, 0)


    def test_anonymous(self):
        # anonymous mmap.mmap(-1, PAGE)
        m = mmap.mmap(-1, PAGESIZE)
        for x in range(PAGESIZE):
            self.assertEqual(m[x], 0,
                             "anonymously mmap'ed contents should be zero")

        for x in range(PAGESIZE):
            b = x & 0xff
            m[x] = b
            self.assertEqual(m[x], b)

    def test_read_all(self):
        m = mmap.mmap(-1, 16)
        self.addCleanup(m.close)

        # With no parameters, or None or a negative argument, reads all
        m.write(bytes(range(16)))
        m.seek(0)
        self.assertEqual(m.read(), bytes(range(16)))
        m.seek(8)
        self.assertEqual(m.read(), bytes(range(8, 16)))
        m.seek(16)
        self.assertEqual(m.read(), b'')
        m.seek(3)
        self.assertEqual(m.read(None), bytes(range(3, 16)))
        m.seek(4)
        self.assertEqual(m.read(-1), bytes(range(4, 16)))
        m.seek(5)
        self.assertEqual(m.read(-2), bytes(range(5, 16)))
        m.seek(9)
        self.assertEqual(m.read(-42), bytes(range(9, 16)))

    def test_read_invalid_arg(self):
        m = mmap.mmap(-1, 16)
        self.addCleanup(m.close)

        self.assertRaises(TypeError, m.read, 'foo')
        self.assertRaises(TypeError, m.read, 5.5)
        self.assertRaises(TypeError, m.read, [1, 2, 3])

    def test_extended_getslice(self):
        # Test extended slicing by comparing with list slicing.
        s = bytes(reversed(range(256)))
        m = mmap.mmap(-1, len(s))
        m[:] = s
        self.assertEqual(m[:], s)
        indices = (0, None, 1, 3, 19, 300, sys.maxsize, -1, -2, -31, -300)
        for start in indices:
            for stop in indices:
                # Skip step 0 (invalid)
                for step in indices[1:]:
                    self.assertEqual(m[start:stop:step],
                                     s[start:stop:step])

    def test_extended_set_del_slice(self):
        # Test extended slicing by comparing with list slicing.
        s = bytes(reversed(range(256)))
        m = mmap.mmap(-1, len(s))
        indices = (0, None, 1, 3, 19, 300, sys.maxsize, -1, -2, -31, -300)
        for start in indices:
            for stop in indices:
                # Skip invalid step 0
                for step in indices[1:]:
                    m[:] = s
                    self.assertEqual(m[:], s)
                    L = list(s)
                    # Make sure we have a slice of exactly the right length,
                    # but with different data.
                    data = L[start:stop:step]
                    data = bytes(reversed(data))
                    L[start:stop:step] = data
                    m[start:stop:step] = data
                    self.assertEqual(m[:], bytes(L))

    def make_mmap_file (self, f, halfsize):
        # Write 2 pages worth of data to the file
        f.write (b'\0' * halfsize)
        f.write (b'foo')
        f.write (b'\0' * (halfsize - 3))
        f.flush ()
        return mmap.mmap (f.fileno(), 0)

    def test_empty_file (self):
        f = open (TESTFN, 'w+b')
        f.close()
        with open(TESTFN, "rb") as f :
            self.assertRaisesRegex(ValueError,
                                   "cannot mmap an empty file",
                                   mmap.mmap, f.fileno(), 0,
                                   access=mmap.ACCESS_READ)

    def test_offset (self):
        f = open (TESTFN, 'w+b')

        try: # unlink TESTFN no matter what
            halfsize = mmap.ALLOCATIONGRANULARITY
            m = self.make_mmap_file (f, halfsize)
            m.close ()
            f.close ()

            mapsize = halfsize * 2
            # Try invalid offset
            f = open(TESTFN, "r+b")
            for offset in [-2, -1, None]:
                try:
                    m = mmap.mmap(f.fileno(), mapsize, offset=offset)
                    self.assertEqual(0, 1)
                except (ValueError, TypeError, OverflowError):
                    pass
                else:
                    self.assertEqual(0, 0)
            f.close()

            # Try valid offset, hopefully 8192 works on all OSes
            f = open(TESTFN, "r+b")
            m = mmap.mmap(f.fileno(), mapsize - halfsize, offset=halfsize)
            self.assertEqual(m[0:3], b'foo')
            f.close()

            # Try resizing map
            try:
                m.resize(512)
            except SystemError:
                pass
            else:
                # resize() is supported
                self.assertEqual(len(m), 512)
                # Check that we can no longer seek beyond the new size.
                self.assertRaises(ValueError, m.seek, 513, 0)
                # Check that the content is not changed
                self.assertEqual(m[0:3], b'foo')

                # Check that the underlying file is truncated too
                f = open(TESTFN, 'rb')
                f.seek(0, 2)
                self.assertEqual(f.tell(), halfsize + 512)
                f.close()
                self.assertEqual(m.size(), halfsize + 512)

            m.close()

        finally:
            f.close()
            try:
                os.unlink(TESTFN)
            except OSError:
                pass

    def test_subclass(self):
        class anon_mmap(mmap.mmap):
            def __new__(klass, *args, **kwargs):
                return mmap.mmap.__new__(klass, -1, *args, **kwargs)
        anon_mmap(PAGESIZE)

    @unittest.skipUnless(hasattr(mmap, 'PROT_READ'), "needs mmap.PROT_READ")
    def test_prot_readonly(self):
        mapsize = 10
        with open(TESTFN, "wb") as fp:
            fp.write(b"a"*mapsize)
        with open(TESTFN, "rb") as f:
            m = mmap.mmap(f.fileno(), mapsize, prot=mmap.PROT_READ)
            self.assertRaises(TypeError, m.write, "foo")

    def test_error(self):
        self.assertIs(mmap.error, OSError)

    def test_io_methods(self):
        data = b"0123456789"
        with open(TESTFN, "wb") as fp:
            fp.write(b"x"*len(data))
        with open(TESTFN, "r+b") as f:
            m = mmap.mmap(f.fileno(), len(data))
        # Test write_byte()
        for i in range(len(data)):
            self.assertEqual(m.tell(), i)
            m.write_byte(data[i])
            self.assertEqual(m.tell(), i+1)
        self.assertRaises(ValueError, m.write_byte, b"x"[0])
        self.assertEqual(m[:], data)
        # Test read_byte()
        m.seek(0)
        for i in range(len(data)):
            self.assertEqual(m.tell(), i)
            self.assertEqual(m.read_byte(), data[i])
            self.assertEqual(m.tell(), i+1)
        self.assertRaises(ValueError, m.read_byte)
        # Test read()
        m.seek(3)
        self.assertEqual(m.read(3), b"345")
        self.assertEqual(m.tell(), 6)
        # Test write()
        m.seek(3)
        m.write(b"bar")
        self.assertEqual(m.tell(), 6)
        self.assertEqual(m[:], b"012bar6789")
        m.write(bytearray(b"baz"))
        self.assertEqual(m.tell(), 9)
        self.assertEqual(m[:], b"012barbaz9")
        self.assertRaises(ValueError, m.write, b"ba")

    def test_non_ascii_byte(self):
        for b in (129, 200, 255): # > 128
            m = mmap.mmap(-1, 1)
            m.write_byte(b)
            self.assertEqual(m[0], b)
            m.seek(0)
            self.assertEqual(m.read_byte(), b)
            m.close()

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_tagname(self):
        data1 = b"0123456789"
        data2 = b"abcdefghij"
        assert len(data1) == len(data2)
        tagname1 = random_tagname()
        tagname2 = random_tagname()

        # Test same tag
        m1 = mmap.mmap(-1, len(data1), tagname=tagname1)
        m1[:] = data1
        m2 = mmap.mmap(-1, len(data2), tagname=tagname1)
        m2[:] = data2
        self.assertEqual(m1[:], data2)
        self.assertEqual(m2[:], data2)
        m2.close()
        m1.close()

        # Test different tag
        m1 = mmap.mmap(-1, len(data1), tagname=tagname1)
        m1[:] = data1
        m2 = mmap.mmap(-1, len(data2), tagname=tagname2)
        m2[:] = data2
        self.assertEqual(m1[:], data1)
        self.assertEqual(m2[:], data2)
        m2.close()
        m1.close()

    @cpython_only
    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_sizeof(self):
        m1 = mmap.mmap(-1, 100)
        tagname = random_tagname()
        m2 = mmap.mmap(-1, 100, tagname=tagname)
        self.assertEqual(sys.getsizeof(m2),
                         sys.getsizeof(m1) + len(tagname) + 1)

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_crasher_on_windows(self):
        # Should not crash (Issue 1733986)
        tagname = random_tagname()
        m = mmap.mmap(-1, 1000, tagname=tagname)
        try:
            mmap.mmap(-1, 5000, tagname=tagname)[:] # same tagname, but larger size
        except:
            pass
        m.close()

        # Should not crash (Issue 5385)
        with open(TESTFN, "wb") as fp:
            fp.write(b"x"*10)
        f = open(TESTFN, "r+b")
        m = mmap.mmap(f.fileno(), 0)
        f.close()
        try:
            m.resize(0) # will raise OSError
        except:
            pass
        try:
            m[:]
        except:
            pass
        m.close()

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_invalid_descriptor(self):
        # socket file descriptors are valid, but out of range
        # for _get_osfhandle, causing a crash when validating the
        # parameters to _get_osfhandle.
        s = socket.socket()
        try:
            with self.assertRaises(OSError):
                m = mmap.mmap(s.fileno(), 10)
        finally:
            s.close()

    def test_context_manager(self):
        with mmap.mmap(-1, 10) as m:
            self.assertFalse(m.closed)
        self.assertTrue(m.closed)

    def test_context_manager_exception(self):
        # Test that the OSError gets passed through
        with self.assertRaises(Exception) as exc:
            with mmap.mmap(-1, 10) as m:
                raise OSError
        self.assertIsInstance(exc.exception, OSError,
                              "wrong exception raised in context manager")
        self.assertTrue(m.closed, "context manager failed")

    def test_weakref(self):
        # Check mmap objects are weakrefable
        mm = mmap.mmap(-1, 16)
        wr = weakref.ref(mm)
        self.assertIs(wr(), mm)
        del mm
        gc_collect()
        self.assertIs(wr(), None)

    def test_write_returning_the_number_of_bytes_written(self):
        mm = mmap.mmap(-1, 16)
        self.assertEqual(mm.write(b""), 0)
        self.assertEqual(mm.write(b"x"), 1)
        self.assertEqual(mm.write(b"yz"), 2)
        self.assertEqual(mm.write(b"python"), 6)

    def test_resize_past_pos(self):
        m = mmap.mmap(-1, 8192)
        self.addCleanup(m.close)
        m.read(5000)
        try:
            m.resize(4096)
        except SystemError:
            self.skipTest("resizing not supported")
        self.assertEqual(m.read(14), b'')
        self.assertRaises(ValueError, m.read_byte)
        self.assertRaises(ValueError, m.write_byte, 42)
        self.assertRaises(ValueError, m.write, b'abc')

    def test_concat_repeat_exception(self):
        m = mmap.mmap(-1, 16)
        with self.assertRaises(TypeError):
            m + m
        with self.assertRaises(TypeError):
            m * 2

    def test_flush_return_value(self):
        # mm.flush() should return None on success, raise an
        # exception on error under all platforms.
        mm = mmap.mmap(-1, 16)
        self.addCleanup(mm.close)
        mm.write(b'python')
        result = mm.flush()
        self.assertIsNone(result)
        if sys.platform.startswith('linux'):
            # 'offset' must be a multiple of mmap.PAGESIZE on Linux.
            # See bpo-34754 for details.
            self.assertRaises(OSError, mm.flush, 1, len(b'python'))

    def test_repr(self):
        open_mmap_repr_pat = re.compile(
            r"<mmap.mmap closed=False, "
            r"access=(?P<access>\S+), "
            r"length=(?P<length>\d+), "
            r"pos=(?P<pos>\d+), "
            r"offset=(?P<offset>\d+)>")
        closed_mmap_repr_pat = re.compile(r"<mmap.mmap closed=True>")
        mapsizes = (50, 100, 1_000, 1_000_000, 10_000_000)
        offsets = tuple((mapsize // 2 // mmap.ALLOCATIONGRANULARITY)
                        * mmap.ALLOCATIONGRANULARITY for mapsize in mapsizes)
        for offset, mapsize in zip(offsets, mapsizes):
            data = b'a' * mapsize
            length = mapsize - offset
            accesses = ('ACCESS_DEFAULT', 'ACCESS_READ',
                        'ACCESS_COPY', 'ACCESS_WRITE')
            positions = (0, length//10, length//5, length//4)
            with open(TESTFN, "wb+") as fp:
                fp.write(data)
                fp.flush()
                for access, pos in itertools.product(accesses, positions):
                    accint = getattr(mmap, access)
                    with mmap.mmap(fp.fileno(),
                                   length,
                                   access=accint,
                                   offset=offset) as mm:
                        mm.seek(pos)
                        match = open_mmap_repr_pat.match(repr(mm))
                        self.assertIsNotNone(match)
                        self.assertEqual(match.group('access'), access)
                        self.assertEqual(match.group('length'), str(length))
                        self.assertEqual(match.group('pos'), str(pos))
                        self.assertEqual(match.group('offset'), str(offset))
                    match = closed_mmap_repr_pat.match(repr(mm))
                    self.assertIsNotNone(match)

    @unittest.skipUnless(hasattr(mmap.mmap, 'madvise'), 'needs madvise')
    def test_madvise(self):
        size = 2 * PAGESIZE
        m = mmap.mmap(-1, size)

        with self.assertRaisesRegex(ValueError, "madvise start out of bounds"):
            m.madvise(mmap.MADV_NORMAL, size)
        with self.assertRaisesRegex(ValueError, "madvise start out of bounds"):
            m.madvise(mmap.MADV_NORMAL, -1)
        with self.assertRaisesRegex(ValueError, "madvise length invalid"):
            m.madvise(mmap.MADV_NORMAL, 0, -1)
        with self.assertRaisesRegex(OverflowError, "madvise length too large"):
            m.madvise(mmap.MADV_NORMAL, PAGESIZE, sys.maxsize)
        self.assertEqual(m.madvise(mmap.MADV_NORMAL), None)
        self.assertEqual(m.madvise(mmap.MADV_NORMAL, PAGESIZE), None)
        self.assertEqual(m.madvise(mmap.MADV_NORMAL, PAGESIZE, size), None)
        self.assertEqual(m.madvise(mmap.MADV_NORMAL, 0, 2), None)
        self.assertEqual(m.madvise(mmap.MADV_NORMAL, 0, size), None)

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_resize_up_when_mapped_to_pagefile(self):
        """If the mmap is backed by the pagefile ensure a resize up can happen
        and that the original data is still in place
        """
        start_size = PAGESIZE
        new_size = 2 * start_size
        data = bytes(random.getrandbits(8) for _ in range(start_size))

        m = mmap.mmap(-1, start_size)
        m[:] = data
        m.resize(new_size)
        self.assertEqual(len(m), new_size)
        self.assertEqual(m[:start_size], data[:start_size])

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_resize_down_when_mapped_to_pagefile(self):
        """If the mmap is backed by the pagefile ensure a resize down up can happen
        and that a truncated form of the original data is still in place
        """
        start_size = PAGESIZE
        new_size = start_size // 2
        data = bytes(random.getrandbits(8) for _ in range(start_size))

        m = mmap.mmap(-1, start_size)
        m[:] = data
        m.resize(new_size)
        self.assertEqual(len(m), new_size)
        self.assertEqual(m[:new_size], data[:new_size])

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_resize_fails_if_mapping_held_elsewhere(self):
        """If more than one mapping is held against a named file on Windows, neither
        mapping can be resized
        """
        start_size = 2 * PAGESIZE
        reduced_size = PAGESIZE

        f = open(TESTFN, 'wb+')
        f.truncate(start_size)
        try:
            m1 = mmap.mmap(f.fileno(), start_size)
            m2 = mmap.mmap(f.fileno(), start_size)
            with self.assertRaises(OSError):
                m1.resize(reduced_size)
            with self.assertRaises(OSError):
                m2.resize(reduced_size)
            m2.close()
            m1.resize(reduced_size)
            self.assertEqual(m1.size(), reduced_size)
            self.assertEqual(os.stat(f.fileno()).st_size, reduced_size)
        finally:
            f.close()

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_resize_succeeds_with_error_for_second_named_mapping(self):
        """If a more than one mapping exists of the same name, none of them can
        be resized: they'll raise an Exception and leave the original mapping intact
        """
        start_size = 2 * PAGESIZE
        reduced_size = PAGESIZE
        tagname =  random_tagname()
        data_length = 8
        data = bytes(random.getrandbits(8) for _ in range(data_length))

        m1 = mmap.mmap(-1, start_size, tagname=tagname)
        m2 = mmap.mmap(-1, start_size, tagname=tagname)
        m1[:data_length] = data
        self.assertEqual(m2[:data_length], data)
        with self.assertRaises(OSError):
            m1.resize(reduced_size)
        self.assertEqual(m1.size(), start_size)
        self.assertEqual(m1[:data_length], data)
        self.assertEqual(m2[:data_length], data)

class LargeMmapTests(unittest.TestCase):

    def setUp(self):
        unlink(TESTFN)

    def tearDown(self):
        unlink(TESTFN)

    def _make_test_file(self, num_zeroes, tail):
        if sys.platform[:3] == 'win' or sys.platform == 'darwin':
            requires('largefile',
                'test requires %s bytes and a long time to run' % str(0x180000000))
        f = open(TESTFN, 'w+b')
        try:
            f.seek(num_zeroes)
            f.write(tail)
            f.flush()
        except (OSError, OverflowError, ValueError):
            try:
                f.close()
            except (OSError, OverflowError):
                pass
            raise unittest.SkipTest("filesystem does not have largefile support")
        return f

    def test_large_offset(self):
        with self._make_test_file(0x14FFFFFFF, b" ") as f:
            with mmap.mmap(f.fileno(), 0, offset=0x140000000, access=mmap.ACCESS_READ) as m:
                self.assertEqual(m[0xFFFFFFF], 32)

    def test_large_filesize(self):
        with self._make_test_file(0x17FFFFFFF, b" ") as f:
            if sys.maxsize < 0x180000000:
                # On 32 bit platforms the file is larger than sys.maxsize so
                # mapping the whole file should fail -- Issue #16743
                with self.assertRaises(OverflowError):
                    mmap.mmap(f.fileno(), 0x180000000, access=mmap.ACCESS_READ)
                with self.assertRaises(ValueError):
                    mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
            with mmap.mmap(f.fileno(), 0x10000, access=mmap.ACCESS_READ) as m:
                self.assertEqual(m.size(), 0x180000000)

    # Issue 11277: mmap() with large (~4 GiB) sparse files crashes on OS X.

    def _test_around_boundary(self, boundary):
        tail = b'  DEARdear  '
        start = boundary - len(tail) // 2
        end = start + len(tail)
        with self._make_test_file(start, tail) as f:
            with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as m:
                self.assertEqual(m[start:end], tail)

    @unittest.skipUnless(sys.maxsize > _4G, "test cannot run on 32-bit systems")
    def test_around_2GB(self):
        self._test_around_boundary(_2G)

    @unittest.skipUnless(sys.maxsize > _4G, "test cannot run on 32-bit systems")
    def test_around_4GB(self):
        self._test_around_boundary(_4G)


if __name__ == '__main__':
    unittest.main()
