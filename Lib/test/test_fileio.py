# Adapted from test_file.py by Daniel Stutzbach

import sys
import os
import io
import errno
import unittest
from array import array
from weakref import proxy
from functools import wraps

from test.support import (
    cpython_only, swap_attr, gc_collect, is_wasi,
    infinite_recursion, strace_helper
)
from test.support.os_helper import (
    TESTFN, TESTFN_ASCII, TESTFN_UNICODE, make_bad_fd,
    )
from test.support.warnings_helper import check_warnings
from test.support.import_helper import import_module
from collections import UserList

import _io  # C implementation of io
import _pyio # Python implementation of io


_strace_flags=["--trace=%file,%desc"]


class AutoFileTests:
    # file tests for which a test file is automatically set up

    def setUp(self):
        self.f = self.FileIO(TESTFN, 'w')

    def tearDown(self):
        if self.f:
            self.f.close()
        os.remove(TESTFN)

    def testWeakRefs(self):
        # verify weak references
        p = proxy(self.f)
        p.write(bytes(range(10)))
        self.assertEqual(self.f.tell(), p.tell())
        self.f.close()
        self.f = None
        gc_collect()  # For PyPy or other GCs.
        self.assertRaises(ReferenceError, getattr, p, 'tell')

    def testSeekTell(self):
        self.f.write(bytes(range(20)))
        self.assertEqual(self.f.tell(), 20)
        self.f.seek(0)
        self.assertEqual(self.f.tell(), 0)
        self.f.seek(10)
        self.assertEqual(self.f.tell(), 10)
        self.f.seek(5, 1)
        self.assertEqual(self.f.tell(), 15)
        self.f.seek(-5, 1)
        self.assertEqual(self.f.tell(), 10)
        self.f.seek(-5, 2)
        self.assertEqual(self.f.tell(), 15)

    def testAttributes(self):
        # verify expected attributes exist
        f = self.f

        self.assertEqual(f.mode, "wb")
        self.assertEqual(f.closed, False)

        # verify the attributes are readonly
        for attr in 'mode', 'closed':
            self.assertRaises((AttributeError, TypeError),
                              setattr, f, attr, 'oops')

    @unittest.skipIf(is_wasi, "WASI does not expose st_blksize.")
    def testBlksize(self):
        # test private _blksize attribute
        blksize = io.DEFAULT_BUFFER_SIZE
        # try to get preferred blksize from stat.st_blksize, if available
        if hasattr(os, 'fstat'):
            fst = os.fstat(self.f.fileno())
            blksize = getattr(fst, 'st_blksize', blksize)
        self.assertEqual(self.f._blksize, blksize)

    # verify readinto
    def testReadintoByteArray(self):
        self.f.write(bytes([1, 2, 0, 255]))
        self.f.close()

        ba = bytearray(b'abcdefgh')
        with self.FileIO(TESTFN, 'r') as f:
            n = f.readinto(ba)
        self.assertEqual(ba, b'\x01\x02\x00\xffefgh')
        self.assertEqual(n, 4)

    def _testReadintoMemoryview(self):
        self.f.write(bytes([1, 2, 0, 255]))
        self.f.close()

        m = memoryview(bytearray(b'abcdefgh'))
        with self.FileIO(TESTFN, 'r') as f:
            n = f.readinto(m)
        self.assertEqual(m, b'\x01\x02\x00\xffefgh')
        self.assertEqual(n, 4)

        m = memoryview(bytearray(b'abcdefgh')).cast('H', shape=[2, 2])
        with self.FileIO(TESTFN, 'r') as f:
            n = f.readinto(m)
        self.assertEqual(bytes(m), b'\x01\x02\x00\xffefgh')
        self.assertEqual(n, 4)

    def _testReadintoArray(self):
        self.f.write(bytes([1, 2, 0, 255]))
        self.f.close()

        a = array('B', b'abcdefgh')
        with self.FileIO(TESTFN, 'r') as f:
            n = f.readinto(a)
        self.assertEqual(a, array('B', [1, 2, 0, 255, 101, 102, 103, 104]))
        self.assertEqual(n, 4)

        a = array('b', b'abcdefgh')
        with self.FileIO(TESTFN, 'r') as f:
            n = f.readinto(a)
        self.assertEqual(a, array('b', [1, 2, 0, -1, 101, 102, 103, 104]))
        self.assertEqual(n, 4)

        a = array('I', b'abcdefgh')
        with self.FileIO(TESTFN, 'r') as f:
            n = f.readinto(a)
        self.assertEqual(a, array('I', b'\x01\x02\x00\xffefgh'))
        self.assertEqual(n, 4)

    def testWritelinesList(self):
        l = [b'123', b'456']
        self.f.writelines(l)
        self.f.close()
        self.f = self.FileIO(TESTFN, 'rb')
        buf = self.f.read()
        self.assertEqual(buf, b'123456')

    def testWritelinesUserList(self):
        l = UserList([b'123', b'456'])
        self.f.writelines(l)
        self.f.close()
        self.f = self.FileIO(TESTFN, 'rb')
        buf = self.f.read()
        self.assertEqual(buf, b'123456')

    def testWritelinesError(self):
        self.assertRaises(TypeError, self.f.writelines, [1, 2, 3])
        self.assertRaises(TypeError, self.f.writelines, None)
        self.assertRaises(TypeError, self.f.writelines, "abc")

    def test_none_args(self):
        self.f.write(b"hi\nbye\nabc")
        self.f.close()
        self.f = self.FileIO(TESTFN, 'r')
        self.assertEqual(self.f.read(None), b"hi\nbye\nabc")
        self.f.seek(0)
        self.assertEqual(self.f.readline(None), b"hi\n")
        self.assertEqual(self.f.readlines(None), [b"bye\n", b"abc"])

    def test_reject(self):
        self.assertRaises(TypeError, self.f.write, "Hello!")

    def testRepr(self):
        self.assertEqual(repr(self.f),
                         "<%s.FileIO name=%r mode=%r closefd=True>" %
                         (self.modulename, self.f.name, self.f.mode))
        del self.f.name
        self.assertEqual(repr(self.f),
                         "<%s.FileIO fd=%r mode=%r closefd=True>" %
                         (self.modulename, self.f.fileno(), self.f.mode))
        self.f.close()
        self.assertEqual(repr(self.f),
                         "<%s.FileIO [closed]>" % (self.modulename,))

    def test_subclass_repr(self):
        class TestSubclass(self.FileIO):
            pass

        f = TestSubclass(TESTFN)
        with f:
            self.assertIn(TestSubclass.__name__, repr(f))

        self.assertIn(TestSubclass.__name__, repr(f))

    def testReprNoCloseFD(self):
        fd = os.open(TESTFN, os.O_RDONLY)
        try:
            with self.FileIO(fd, 'r', closefd=False) as f:
                self.assertEqual(repr(f),
                                 "<%s.FileIO name=%r mode=%r closefd=False>" %
                                 (self.modulename, f.name, f.mode))
        finally:
            os.close(fd)

    @infinite_recursion(25)
    def testRecursiveRepr(self):
        # Issue #25455
        with swap_attr(self.f, 'name', self.f):
            with self.assertRaises(RuntimeError):
                repr(self.f)  # Should not crash

    def testErrors(self):
        f = self.f
        self.assertFalse(f.isatty())
        self.assertFalse(f.closed)
        #self.assertEqual(f.name, TESTFN)
        self.assertRaises(ValueError, f.read, 10) # Open for reading
        f.close()
        self.assertTrue(f.closed)
        f = self.FileIO(TESTFN, 'r')
        self.assertRaises(TypeError, f.readinto, "")
        self.assertFalse(f.closed)
        f.close()
        self.assertTrue(f.closed)

    def testMethods(self):
        methods = ['fileno', 'isatty', 'seekable', 'readable', 'writable',
                   'read', 'readall', 'readline', 'readlines',
                   'tell', 'truncate', 'flush']

        self.f.close()
        self.assertTrue(self.f.closed)

        for methodname in methods:
            method = getattr(self.f, methodname)
            # should raise on closed file
            self.assertRaises(ValueError, method)

        self.assertRaises(TypeError, self.f.readinto)
        self.assertRaises(ValueError, self.f.readinto, bytearray(1))
        self.assertRaises(TypeError, self.f.seek)
        self.assertRaises(ValueError, self.f.seek, 0)
        self.assertRaises(TypeError, self.f.write)
        self.assertRaises(ValueError, self.f.write, b'')
        self.assertRaises(TypeError, self.f.writelines)
        self.assertRaises(ValueError, self.f.writelines, b'')

    def testOpendir(self):
        # Issue 3703: opening a directory should fill the errno
        # Windows always returns "[Errno 13]: Permission denied
        # Unix uses fstat and returns "[Errno 21]: Is a directory"
        try:
            self.FileIO('.', 'r')
        except OSError as e:
            self.assertNotEqual(e.errno, 0)
            self.assertEqual(e.filename, ".")
        else:
            self.fail("Should have raised OSError")

    @unittest.skipIf(os.name == 'nt', "test only works on a POSIX-like system")
    def testOpenDirFD(self):
        fd = os.open('.', os.O_RDONLY)
        with self.assertRaises(OSError) as cm:
            self.FileIO(fd, 'r')
        os.close(fd)
        self.assertEqual(cm.exception.errno, errno.EISDIR)

    #A set of functions testing that we get expected behaviour if someone has
    #manually closed the internal file descriptor.  First, a decorator:
    def ClosedFD(func):
        @wraps(func)
        def wrapper(self):
            #forcibly close the fd before invoking the problem function
            f = self.f
            os.close(f.fileno())
            try:
                func(self, f)
            finally:
                try:
                    self.f.close()
                except OSError:
                    pass
        return wrapper

    def ClosedFDRaises(func):
        @wraps(func)
        def wrapper(self):
            #forcibly close the fd before invoking the problem function
            f = self.f
            os.close(f.fileno())
            try:
                func(self, f)
            except OSError as e:
                self.assertEqual(e.errno, errno.EBADF)
            else:
                self.fail("Should have raised OSError")
            finally:
                try:
                    self.f.close()
                except OSError:
                    pass
        return wrapper

    @ClosedFDRaises
    def testErrnoOnClose(self, f):
        f.close()

    @ClosedFDRaises
    def testErrnoOnClosedWrite(self, f):
        f.write(b'a')

    @ClosedFDRaises
    def testErrnoOnClosedSeek(self, f):
        f.seek(0)

    @ClosedFDRaises
    def testErrnoOnClosedTell(self, f):
        f.tell()

    @ClosedFDRaises
    def testErrnoOnClosedTruncate(self, f):
        f.truncate(0)

    @ClosedFD
    def testErrnoOnClosedSeekable(self, f):
        f.seekable()

    @ClosedFD
    def testErrnoOnClosedReadable(self, f):
        f.readable()

    @ClosedFD
    def testErrnoOnClosedWritable(self, f):
        f.writable()

    @ClosedFD
    def testErrnoOnClosedFileno(self, f):
        f.fileno()

    @ClosedFD
    def testErrnoOnClosedIsatty(self, f):
        self.assertEqual(f.isatty(), False)

    def ReopenForRead(self):
        try:
            self.f.close()
        except OSError:
            pass
        self.f = self.FileIO(TESTFN, 'r')
        os.close(self.f.fileno())
        return self.f

    @ClosedFDRaises
    def testErrnoOnClosedRead(self, f):
        f = self.ReopenForRead()
        f.read(1)

    @ClosedFDRaises
    def testErrnoOnClosedReadall(self, f):
        f = self.ReopenForRead()
        f.readall()

    @ClosedFDRaises
    def testErrnoOnClosedReadinto(self, f):
        f = self.ReopenForRead()
        a = array('b', b'x'*10)
        f.readinto(a)

    @strace_helper.requires_strace()
    def test_syscalls_read(self):
        """Check set of system calls during common I/O patterns

        It's expected as bits of the I/O implementation change, this will need
        to change. The goal is to catch changes that unintentionally add
        additional systemcalls (ex. additional calls have been looked at in
        bpo-21679 and gh-120754).
        """
        self.f.write(b"Hello, World!")
        self.f.close()


        def check_readall(name, code, prelude="", cleanup="",
                          extra_checks=None):
            with self.subTest(name=name):
                syscalls = strace_helper.get_events(code, _strace_flags,
                                                      prelude=prelude,
                                                      cleanup=cleanup)

                # Some system calls (ex. mmap) can be used for both File I/O and
                # memory allocation. Filter out the ones used for memory
                # allocation.
                syscalls = strace_helper.filter_memory(syscalls)

                # The first call should be an open that returns a
                # file descriptor (fd). Afer that calls may vary. Once the file
                # is opened, check calls refer to it by fd as the filename
                # could be removed from the filesystem, renamed, etc. See:
                # Time-of-check time-of-use (TOCTOU) software bug class.
                #
                # There are a number of related but distinct open system calls
                # so not checking precise name here.
                self.assertGreater(
                    len(syscalls),
                    1,
                    f"Should have had at least an open call|calls={syscalls}")
                fd_str = syscalls[0].returncode

                # All other calls should contain the fd in their argument set.
                for ev in syscalls[1:]:
                    self.assertIn(
                        fd_str,
                        ev.args,
                        f"Looking for file descriptor in arguments|ev={ev}"
                    )

                # There are a number of related syscalls used to implement
                # behaviors in a libc (ex. fstat, newfstatat, statx, open, openat).
                # Allow any that use the same substring.
                def count_similarname(name):
                    return len([ev for ev in syscalls if name in ev.syscall])

                checks = [
                    # Should open and close the file exactly once
                    ("open", 1),
                    ("close", 1),
                    # There should no longer be an isatty call (All files being
                    # tested are block devices / not character devices).
                    ('ioctl', 0),
                    # Should only have one fstat (bpo-21679, gh-120754)
                    # note: It's important this uses a fd rather than filename,
                    # That is validated by the `fd` check above.
                    # note: fstat, newfstatat, and statx have all been observed
                    # here in the underlying C library implementations.
                    ("stat", 1)
                ]

                if extra_checks:
                    checks += extra_checks

                for call, count in checks:
                    self.assertEqual(
                        count_similarname(call),
                        count,
                        msg=f"call={call}|count={count}|syscalls={syscalls}"
                    )

        # "open, read, close" file using different common patterns.
        check_readall(
            "open builtin with default options",
            f"""
            f = open('{TESTFN}')
            f.read()
            f.close()
            """
        )

        check_readall(
            "open in binary mode",
            f"""
            f = open('{TESTFN}', 'rb')
            f.read()
            f.close()
            """
        )

        check_readall(
            "open in text mode",
            f"""
            f = open('{TESTFN}', 'rt')
            f.read()
            f.close()
            """,
            # GH-122111: read_text uses BufferedIO which requires looking up
            # position in file. `read_bytes` disables that buffering and avoids
            # these calls which is tested the `pathlib read_bytes` case.
            extra_checks=[("seek", 1)]
        )

        check_readall(
            "pathlib read_bytes",
            "p.read_bytes()",
            prelude=f"""from pathlib import Path; p = Path("{TESTFN}")""",
            # GH-122111: Buffering is disabled so these calls are avoided.
            extra_checks=[("seek", 0)]
        )

        check_readall(
            "pathlib read_text",
            "p.read_text()",
            prelude=f"""from pathlib import Path; p = Path("{TESTFN}")"""
        )

        # Focus on just `read()`.
        calls = strace_helper.get_syscalls(
            prelude=f"f = open('{TESTFN}')",
            code="f.read()",
            cleanup="f.close()",
            strace_flags=_strace_flags
        )
        # One to read all the bytes
        # One to read the EOF and get a size 0 return.
        self.assertEqual(calls.count("read"), 2)



class CAutoFileTests(AutoFileTests, unittest.TestCase):
    FileIO = _io.FileIO
    modulename = '_io'

class PyAutoFileTests(AutoFileTests, unittest.TestCase):
    FileIO = _pyio.FileIO
    modulename = '_pyio'


class OtherFileTests:

    def testAbles(self):
        try:
            f = self.FileIO(TESTFN, "w")
            self.assertEqual(f.readable(), False)
            self.assertEqual(f.writable(), True)
            self.assertEqual(f.seekable(), True)
            f.close()

            f = self.FileIO(TESTFN, "r")
            self.assertEqual(f.readable(), True)
            self.assertEqual(f.writable(), False)
            self.assertEqual(f.seekable(), True)
            f.close()

            f = self.FileIO(TESTFN, "a+")
            self.assertEqual(f.readable(), True)
            self.assertEqual(f.writable(), True)
            self.assertEqual(f.seekable(), True)
            self.assertEqual(f.isatty(), False)
            f.close()

            if sys.platform != "win32":
                try:
                    f = self.FileIO("/dev/tty", "a")
                except OSError:
                    # When run in a cron job there just aren't any
                    # ttys, so skip the test.  This also handles other
                    # OS'es that don't support /dev/tty.
                    pass
                else:
                    self.assertEqual(f.readable(), False)
                    self.assertEqual(f.writable(), True)
                    if sys.platform != "darwin" and \
                       'bsd' not in sys.platform and \
                       not sys.platform.startswith(('sunos', 'aix')):
                        # Somehow /dev/tty appears seekable on some BSDs
                        self.assertEqual(f.seekable(), False)
                    self.assertEqual(f.isatty(), True)
                    f.close()
        finally:
            os.unlink(TESTFN)

    def testInvalidModeStrings(self):
        # check invalid mode strings
        for mode in ("", "aU", "wU+", "rw", "rt"):
            try:
                f = self.FileIO(TESTFN, mode)
            except ValueError:
                pass
            else:
                f.close()
                self.fail('%r is an invalid file mode' % mode)

    def testModeStrings(self):
        # test that the mode attribute is correct for various mode strings
        # given as init args
        try:
            for modes in [('w', 'wb'), ('wb', 'wb'), ('wb+', 'rb+'),
                          ('w+b', 'rb+'), ('a', 'ab'), ('ab', 'ab'),
                          ('ab+', 'ab+'), ('a+b', 'ab+'), ('r', 'rb'),
                          ('rb', 'rb'), ('rb+', 'rb+'), ('r+b', 'rb+')]:
                # read modes are last so that TESTFN will exist first
                with self.FileIO(TESTFN, modes[0]) as f:
                    self.assertEqual(f.mode, modes[1])
        finally:
            if os.path.exists(TESTFN):
                os.unlink(TESTFN)

    def testUnicodeOpen(self):
        # verify repr works for unicode too
        f = self.FileIO(str(TESTFN), "w")
        f.close()
        os.unlink(TESTFN)

    def testBytesOpen(self):
        # Opening a bytes filename
        fn = TESTFN_ASCII.encode("ascii")
        f = self.FileIO(fn, "w")
        try:
            f.write(b"abc")
            f.close()
            with self.open(TESTFN_ASCII, "rb") as f:
                self.assertEqual(f.read(), b"abc")
        finally:
            os.unlink(TESTFN_ASCII)

    @unittest.skipIf(sys.getfilesystemencoding() != 'utf-8',
                     "test only works for utf-8 filesystems")
    def testUtf8BytesOpen(self):
        # Opening a UTF-8 bytes filename
        try:
            fn = TESTFN_UNICODE.encode("utf-8")
        except UnicodeEncodeError:
            self.skipTest('could not encode %r to utf-8' % TESTFN_UNICODE)
        f = self.FileIO(fn, "w")
        try:
            f.write(b"abc")
            f.close()
            with self.open(TESTFN_UNICODE, "rb") as f:
                self.assertEqual(f.read(), b"abc")
        finally:
            os.unlink(TESTFN_UNICODE)

    def testConstructorHandlesNULChars(self):
        fn_with_NUL = 'foo\0bar'
        self.assertRaises(ValueError, self.FileIO, fn_with_NUL, 'w')
        self.assertRaises(ValueError, self.FileIO, bytes(fn_with_NUL, 'ascii'), 'w')

    def testInvalidFd(self):
        self.assertRaises(ValueError, self.FileIO, -10)
        self.assertRaises(OSError, self.FileIO, make_bad_fd())
        if sys.platform == 'win32':
            import msvcrt
            self.assertRaises(OSError, msvcrt.get_osfhandle, make_bad_fd())

    def testBooleanFd(self):
        for fd in False, True:
            with self.assertWarnsRegex(RuntimeWarning,
                    'bool is used as a file descriptor') as cm:
                f = self.FileIO(fd, closefd=False)
            f.close()
            self.assertEqual(cm.filename, __file__)

    def testBadModeArgument(self):
        # verify that we get a sensible error message for bad mode argument
        bad_mode = "qwerty"
        try:
            f = self.FileIO(TESTFN, bad_mode)
        except ValueError as msg:
            if msg.args[0] != 0:
                s = str(msg)
                if TESTFN in s or bad_mode not in s:
                    self.fail("bad error message for invalid mode: %s" % s)
            # if msg.args[0] == 0, we're probably on Windows where there may be
            # no obvious way to discover why open() failed.
        else:
            f.close()
            self.fail("no error for invalid mode: %s" % bad_mode)

    def testTruncate(self):
        f = self.FileIO(TESTFN, 'w')
        f.write(bytes(bytearray(range(10))))
        self.assertEqual(f.tell(), 10)
        f.truncate(5)
        self.assertEqual(f.tell(), 10)
        self.assertEqual(f.seek(0, io.SEEK_END), 5)
        f.truncate(15)
        self.assertEqual(f.tell(), 5)
        self.assertEqual(f.seek(0, io.SEEK_END), 15)
        f.close()

    def testTruncateOnWindows(self):
        def bug801631():
            # SF bug <https://bugs.python.org/issue801631>
            # "file.truncate fault on windows"
            f = self.FileIO(TESTFN, 'w')
            f.write(bytes(range(11)))
            f.close()

            f = self.FileIO(TESTFN,'r+')
            data = f.read(5)
            if data != bytes(range(5)):
                self.fail("Read on file opened for update failed %r" % data)
            if f.tell() != 5:
                self.fail("File pos after read wrong %d" % f.tell())

            f.truncate()
            if f.tell() != 5:
                self.fail("File pos after ftruncate wrong %d" % f.tell())

            f.close()
            size = os.path.getsize(TESTFN)
            if size != 5:
                self.fail("File size after ftruncate wrong %d" % size)

        try:
            bug801631()
        finally:
            os.unlink(TESTFN)

    def testAppend(self):
        try:
            f = self.FileIO(TESTFN, 'wb')
            f.write(b'spam')
            f.close()
            f = self.FileIO(TESTFN, 'ab')
            f.write(b'eggs')
            f.close()
            f = self.FileIO(TESTFN, 'rb')
            d = f.read()
            f.close()
            self.assertEqual(d, b'spameggs')
        finally:
            try:
                os.unlink(TESTFN)
            except:
                pass

    def testInvalidInit(self):
        self.assertRaises(TypeError, self.FileIO, "1", 0, 0)

    def testWarnings(self):
        with check_warnings(quiet=True) as w:
            self.assertEqual(w.warnings, [])
            self.assertRaises(TypeError, self.FileIO, [])
            self.assertEqual(w.warnings, [])
            self.assertRaises(ValueError, self.FileIO, "/some/invalid/name", "rt")
            self.assertEqual(w.warnings, [])

    def testUnclosedFDOnException(self):
        class MyException(Exception): pass
        class MyFileIO(self.FileIO):
            def __setattr__(self, name, value):
                if name == "name":
                    raise MyException("blocked setting name")
                return super(MyFileIO, self).__setattr__(name, value)
        fd = os.open(__file__, os.O_RDONLY)
        self.assertRaises(MyException, MyFileIO, fd)
        os.close(fd)  # should not raise OSError(EBADF)


class COtherFileTests(OtherFileTests, unittest.TestCase):
    FileIO = _io.FileIO
    modulename = '_io'
    open = _io.open

    @cpython_only
    def testInvalidFd_overflow(self):
        # Issue 15989
        _testcapi = import_module("_testcapi")
        self.assertRaises(TypeError, self.FileIO, _testcapi.INT_MAX + 1)
        self.assertRaises(TypeError, self.FileIO, _testcapi.INT_MIN - 1)

    def test_open_code(self):
        # Check that the default behaviour of open_code matches
        # open("rb")
        with self.FileIO(__file__, "rb") as f:
            expected = f.read()
        with _io.open_code(__file__) as f:
            actual = f.read()
        self.assertEqual(expected, actual)


class PyOtherFileTests(OtherFileTests, unittest.TestCase):
    FileIO = _pyio.FileIO
    modulename = '_pyio'
    open = _pyio.open

    def test_open_code(self):
        # Check that the default behaviour of open_code matches
        # open("rb")
        with self.FileIO(__file__, "rb") as f:
            expected = f.read()
        with check_warnings(quiet=True) as w:
            # Always test _open_code_with_warning
            with _pyio._open_code_with_warning(__file__) as f:
                actual = f.read()
            self.assertEqual(expected, actual)
            self.assertNotEqual(w.warnings, [])


def tearDownModule():
    # Historically, these tests have been sloppy about removing TESTFN.
    # So get rid of it no matter what.
    if os.path.exists(TESTFN):
        os.unlink(TESTFN)


if __name__ == '__main__':
    unittest.main()
