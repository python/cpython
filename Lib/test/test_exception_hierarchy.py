import builtins
import os
import select
import socket
import unittest
import errno
from errno import EEXIST


class SubOSError(OSError):
    pass

class SubOSErrorWithInit(OSError):
    def __init__(self, message, bar):
        self.bar = bar
        super().__init__(message)

class SubOSErrorWithNew(OSError):
    def __new__(cls, message, baz):
        self = super().__new__(cls, message)
        self.baz = baz
        return self

class SubOSErrorCombinedInitFirst(SubOSErrorWithInit, SubOSErrorWithNew):
    pass

class SubOSErrorCombinedNewFirst(SubOSErrorWithNew, SubOSErrorWithInit):
    pass

class SubOSErrorWithStandaloneInit(OSError):
    def __init__(self):
        pass


class HierarchyTest(unittest.TestCase):

    def test_builtin_errors(self):
        self.assertEqual(OSError.__name__, 'OSError')
        self.assertIs(IOError, OSError)
        self.assertIs(EnvironmentError, OSError)

    def test_socket_errors(self):
        self.assertIs(socket.error, OSError)
        self.assertIs(socket.gaierror.__base__, OSError)
        self.assertIs(socket.herror.__base__, OSError)
        self.assertIs(socket.timeout, TimeoutError)

    def test_select_error(self):
        self.assertIs(select.error, OSError)

    # mmap.error is tested in test_mmap

    _pep_map = """
        +-- BlockingIOError        EAGAIN, EALREADY, EWOULDBLOCK, EINPROGRESS
        +-- ChildProcessError                                          ECHILD
        +-- ConnectionError
            +-- BrokenPipeError                              EPIPE, ESHUTDOWN
            +-- ConnectionAbortedError                           ECONNABORTED
            +-- ConnectionRefusedError                           ECONNREFUSED
            +-- ConnectionResetError                               ECONNRESET
        +-- FileExistsError                                            EEXIST
        +-- FileNotFoundError                                          ENOENT
        +-- InterruptedError                                            EINTR
        +-- IsADirectoryError                                          EISDIR
        +-- NotADirectoryError                                        ENOTDIR
        +-- PermissionError                        EACCES, EPERM, ENOTCAPABLE
        +-- ProcessLookupError                                          ESRCH
        +-- TimeoutError                                            ETIMEDOUT
    """
    def _make_map(s):
        _map = {}
        for line in s.splitlines():
            line = line.strip('+- ')
            if not line:
                continue
            excname, _, errnames = line.partition(' ')
            for errname in filter(None, errnames.strip().split(', ')):
                if errname == "ENOTCAPABLE" and not hasattr(errno, errname):
                    continue
                _map[getattr(errno, errname)] = getattr(builtins, excname)
        return _map
    _map = _make_map(_pep_map)

    def test_errno_mapping(self):
        # The OSError constructor maps errnos to subclasses
        # A sample test for the basic functionality
        e = OSError(EEXIST, "Bad file descriptor")
        self.assertIs(type(e), FileExistsError)
        # Exhaustive testing
        for errcode, exc in self._map.items():
            e = OSError(errcode, "Some message")
            self.assertIs(type(e), exc)
        othercodes = set(errno.errorcode) - set(self._map)
        for errcode in othercodes:
            e = OSError(errcode, "Some message")
            self.assertIs(type(e), OSError, repr(e))

    def _defaults(self):
        # The first errno listed for a class is the one it defaults to.
        defaults = {}
        for errcode, exc in self._map.items():
            defaults.setdefault(exc, errcode)
        return defaults

    def test_default_errno(self):
        for exc, errcode in self._defaults().items():
            with self.subTest(exc=exc.__name__):
                self.assertEqual(exc.default_errno, errcode)
                e = exc()
                self.assertEqual(e.errno, errcode)
                self.assertEqual(e.strerror, os.strerror(errcode))
                self.assertEqual(e.args, (errcode, os.strerror(errcode)))

    def test_try_except(self):
        filename = "some_hopefully_non_existing_file"

        # This checks that try .. except checks the concrete exception
        # (FileNotFoundError) and not the base type specified when
        # PyErr_SetFromErrnoWithFilenameObject was called.
        # (it is therefore deliberate that it doesn't use assertRaises)
        try:
            open(filename)
        except FileNotFoundError:
            pass
        else:
            self.fail("should have raised a FileNotFoundError")

        # Another test for PyErr_SetExcFromWindowsErrWithFilenameObject()
        self.assertFalse(os.path.exists(filename))
        try:
            os.unlink(filename)
        except FileNotFoundError:
            pass
        else:
            self.fail("should have raised a FileNotFoundError")


class AttributesTest(unittest.TestCase):

    def test_windows_error(self):
        if os.name == "nt":
            self.assertIn('winerror', dir(OSError))
        else:
            self.assertNotIn('winerror', dir(OSError))

    def test_posix_error(self):
        e = OSError(EEXIST, "File already exists", "foo.txt")
        self.assertEqual(e.errno, EEXIST)
        self.assertEqual(e.args[0], EEXIST)
        self.assertEqual(e.strerror, "File already exists")
        self.assertEqual(e.filename, "foo.txt")
        if os.name == "nt":
            self.assertEqual(e.winerror, None)

    def test_strerror_derived_from_errno(self):
        e = FileNotFoundError()
        self.assertEqual(e.errno, errno.ENOENT)
        self.assertEqual(e.strerror, os.strerror(errno.ENOENT))
        # an explicit strerror wins over the one derived from errno
        e = FileNotFoundError('not found')
        self.assertEqual(e.errno, errno.ENOENT)
        self.assertEqual(e.strerror, 'not found')
        self.assertEqual(e.args, (errno.ENOENT, 'not found'))
        # ... including an explicit None
        e = FileNotFoundError(errno.ENOENT, None)
        self.assertIsNone(e.strerror)

    @unittest.skipUnless(os.name == "nt", "Windows-specific test")
    def test_strerror_derived_from_winerror(self):
        # the message of the Windows error code is more specific than the one
        # of the errno it is translated to
        import ctypes
        e = OSError(filename="foo.txt", winerror=183)
        self.assertEqual(e.errno, EEXIST)
        expected = ctypes.WinError(183).strerror.rstrip(" .")
        self.assertEqual(e.strerror, expected)
        # a code which the system has no message for
        e = OSError(winerror=99999)
        self.assertEqual(e.errno, errno.EINVAL)
        self.assertEqual(e.strerror, "Windows Error 0x%x" % 99999)

    def test_strerror_not_derived_from_bogus_errno(self):
        for code in 2**31, -2**31-1, 2**1000, -2**1000, 'x':
            with self.subTest(default_errno=code):
                cls = type('E', (OSError,), {'default_errno': code})
                e = cls()
                self.assertEqual(e.errno, code)
                self.assertIsNone(e.strerror)

    @unittest.skipUnless(os.name == "nt", "Windows-specific test")
    def test_strerror_not_derived_from_bogus_winerror(self):
        # a winerror out of the C long range is rejected
        for code in 2**31, -2**31-1, 2**1000, -2**1000:
            with self.subTest(winerror=code):
                self.assertRaises(OverflowError, OSError, winerror=code)
        # a winerror which is not an integer is not translated at all
        e = OSError(winerror='x')
        self.assertIsNone(e.errno)
        self.assertIsNone(e.strerror)
        self.assertEqual(e.winerror, 'x')

    def test_keyword_arguments(self):
        e = FileNotFoundError(filename='foo.txt')
        self.assertEqual(e.errno, errno.ENOENT)
        self.assertEqual(e.strerror, os.strerror(errno.ENOENT))
        self.assertEqual(e.filename, 'foo.txt')
        self.assertIsNone(e.filename2)

        e = OSError('cannot open', filename='foo.txt', filename2='bar.txt')
        self.assertIsNone(e.errno)
        self.assertEqual(e.strerror, 'cannot open')
        self.assertEqual(e.filename, 'foo.txt')
        self.assertEqual(e.filename2, 'bar.txt')

        e = OSError(EEXIST, 'exists', filename='foo.txt')
        self.assertEqual(e.errno, EEXIST)
        self.assertEqual(e.filename, 'foo.txt')

    def test_keyword_argument_errors(self):
        # errno and strerror are positional-only
        self.assertRaises(TypeError, OSError, errno=EEXIST)
        self.assertRaises(TypeError, OSError, strerror='exists')
        # and cannot be given twice
        self.assertRaises(TypeError, OSError,
                          EEXIST, 'exists', 'foo.txt', filename='bar.txt')
        # more than five arguments
        self.assertRaises(TypeError, OSError, 1, 2, 3, 4, 5, 6)

    @unittest.skipUnless(os.name == "nt", "Windows-specific test")
    def test_errno_translation(self):
        # ERROR_ALREADY_EXISTS (183) -> EEXIST
        e = OSError(0, "File already exists", "foo.txt", 183)
        self.assertEqual(e.winerror, 183)
        self.assertEqual(e.errno, EEXIST)
        self.assertEqual(e.args[0], EEXIST)
        self.assertEqual(e.strerror, "File already exists")
        self.assertEqual(e.filename, "foo.txt")
        # winerror can also be given by keyword
        e = OSError("File already exists", filename="foo.txt", winerror=183)
        self.assertEqual(e.winerror, 183)
        self.assertEqual(e.errno, EEXIST)
        self.assertEqual(e.args, (EEXIST, "File already exists", "foo.txt", 183))
        self.assertEqual(e.strerror, "File already exists")
        self.assertEqual(e.filename, "foo.txt")

    def test_blockingioerror(self):
        args = ("a", "b", "c", "d", "e")
        for n in range(6):
            e = BlockingIOError(*args[:n])
            with self.assertRaises(AttributeError):
                e.characters_written
            with self.assertRaises(AttributeError):
                del e.characters_written
        e = BlockingIOError("a", "b", 3)
        self.assertEqual(e.characters_written, 3)
        e.characters_written = 5
        self.assertEqual(e.characters_written, 5)
        del e.characters_written
        with self.assertRaises(AttributeError):
            e.characters_written

        # characters_written can also be given by keyword
        e = BlockingIOError("would block", characters_written=3)
        self.assertEqual(e.strerror, "would block")
        self.assertEqual(e.characters_written, 3)
        # including when the class is chosen by errno
        e = OSError(errno.EAGAIN, "would block", characters_written=3)
        self.assertIs(type(e), BlockingIOError)
        self.assertEqual(e.characters_written, 3)
        # but only for BlockingIOError
        for cls in OSError, FileNotFoundError:
            with self.subTest(cls=cls.__name__):
                self.assertRaises(TypeError, cls, characters_written=3)
        # and not together with a file name, which it is an alternative to
        self.assertRaises(TypeError, BlockingIOError,
                          filename="foo.txt", characters_written=3)
        self.assertRaises(TypeError, BlockingIOError,
                          errno.EAGAIN, "would block", 3, characters_written=3)
        # and it is keyword-only
        self.assertRaises(TypeError, BlockingIOError, 1, 2, 3, 4, 5, 6)


class ExplicitSubclassingTest(unittest.TestCase):

    def test_errno_mapping(self):
        # When constructing an OSError subclass, errno mapping isn't done
        e = SubOSError(EEXIST, "Bad file descriptor")
        self.assertIs(type(e), SubOSError)

    def test_init_overridden(self):
        e = SubOSErrorWithInit("some message", "baz")
        self.assertEqual(e.bar, "baz")
        self.assertEqual(e.args, ("some message",))

    def test_init_kwdargs(self):
        e = SubOSErrorWithInit("some message", bar="baz")
        self.assertEqual(e.bar, "baz")
        self.assertEqual(e.args, ("some message",))

    def test_new_overridden(self):
        e = SubOSErrorWithNew("some message", "baz")
        self.assertEqual(e.baz, "baz")
        self.assertEqual(e.args, ("some message",))

    def test_new_kwdargs(self):
        e = SubOSErrorWithNew("some message", baz="baz")
        self.assertEqual(e.baz, "baz")
        self.assertEqual(e.args, ("some message",))

    def test_init_new_overridden(self):
        e = SubOSErrorCombinedInitFirst("some message", "baz")
        self.assertEqual(e.bar, "baz")
        self.assertEqual(e.baz, "baz")
        self.assertEqual(e.args, ("some message",))
        e = SubOSErrorCombinedNewFirst("some message", "baz")
        self.assertEqual(e.bar, "baz")
        self.assertEqual(e.baz, "baz")
        self.assertEqual(e.args, ("some message",))

    def test_init_standalone(self):
        # __init__ doesn't propagate to OSError.__init__ (see issue #15229)
        e = SubOSErrorWithStandaloneInit()
        self.assertEqual(e.args, ())
        self.assertEqual(str(e), '')


if __name__ == "__main__":
    unittest.main()
