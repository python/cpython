import builtins
import locale
import os
import select
import socket
import unittest
import errno
from errno import EEXIST
from test.support import cpython_only, is_android, os_helper


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

    @cpython_only
    @unittest.skipIf(os.name == "nt", "POSIX-specific test")
    def test_strerror_cached(self):
        message = os.strerror(errno.ENOENT)
        self.assertIs(os.strerror(errno.ENOENT), message)
        with self.assertRaises(FileNotFoundError) as cm:
            open("some_hopefully_non_existing_file")
        self.assertIs(cm.exception.strerror, message)
        # Android ignores the locale (_Py_FORCE_UTF8_LOCALE).
        if is_android:
            return
        # The locale part of the cache key. musl (and WASI) has no C.UTF-8
        # LC_MESSAGES: it's "C".
        def key():
            return (locale.nl_langinfo(locale.CODESET),
                    locale.setlocale(locale.LC_MESSAGES))
        for category in (locale.LC_CTYPE, locale.LC_MESSAGES):
            old = locale.setlocale(category)
            self.addCleanup(locale.setlocale, category, old)
            locale.setlocale(category, "C")
            message = os.strerror(errno.ENOENT)
            old_key = key()
            for name in ("C.UTF-8", "C.utf8", "en_US.UTF-8"):
                try:
                    locale.setlocale(category, name)
                except locale.Error:
                    continue
                if key() != old_key:
                    self.assertIsNot(os.strerror(errno.ENOENT), message)
                    break
        message = os.strerror(errno.ENOENT)
        with os_helper.EnvironmentVarGuard() as env:
            env["LANGUAGE"] = "de"
            self.assertIsNot(os.strerror(errno.ENOENT), message)

    @unittest.skipUnless(os.name == "nt", "Windows-specific test")
    def test_errno_translation(self):
        # ERROR_ALREADY_EXISTS (183) -> EEXIST
        e = OSError(0, "File already exists", "foo.txt", 183)
        self.assertEqual(e.winerror, 183)
        self.assertEqual(e.errno, EEXIST)
        self.assertEqual(e.args[0], EEXIST)
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
