import io
import os
import unittest
from test import support
from test.support import import_helper, os_helper, warnings_helper


_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
_io = import_helper.import_module('_io')
NULL = None
STDOUT_FD = 1

with open(__file__, 'rb') as fp:
    FIRST_LINE = next(fp).decode()
FIRST_LINE_NORM = FIRST_LINE.rstrip() + '\n'


class CAPIFileTest(unittest.TestCase):
    def test_pyfile_fromfd(self):
        # Test PyFile_FromFd() which is a thin wrapper to _io.open()
        pyfile_fromfd = _testlimitedcapi.pyfile_fromfd
        filename = __file__
        with open(filename, "rb") as fp:
            fd = fp.fileno()

            # FileIO
            fp.seek(0)
            obj = pyfile_fromfd(fd, filename, "rb", 0, NULL, NULL, NULL, 0)
            try:
                self.assertIsInstance(obj, _io.FileIO)
                self.assertEqual(obj.readline(), FIRST_LINE.encode())
            finally:
                obj.close()

            # BufferedReader
            fp.seek(0)
            obj = pyfile_fromfd(fd, filename, "rb", 1024, NULL, NULL, NULL, 0)
            try:
                self.assertIsInstance(obj, _io.BufferedReader)
                self.assertEqual(obj.readline(), FIRST_LINE.encode())
            finally:
                obj.close()

            # TextIOWrapper
            fp.seek(0)
            obj = pyfile_fromfd(fd, filename, "r", 1,
                                "utf-8", "replace", NULL, 0)
            try:
                self.assertIsInstance(obj, _io.TextIOWrapper)
                self.assertEqual(obj.encoding, "utf-8")
                self.assertEqual(obj.errors, "replace")
                self.assertEqual(obj.readline(), FIRST_LINE_NORM)
            finally:
                obj.close()

    def test_pyfile_getline(self):
        # Test PyFile_GetLine(file, n): call file.readline()
        # and strip "\n" suffix if n < 0.
        pyfile_getline = _testlimitedcapi.pyfile_getline

        # Test Unicode
        with open(__file__, "r") as fp:
            fp.seek(0)
            self.assertEqual(pyfile_getline(fp, -1),
                             FIRST_LINE_NORM.rstrip('\n'))
            fp.seek(0)
            self.assertEqual(pyfile_getline(fp, 0),
                             FIRST_LINE_NORM)
            fp.seek(0)
            self.assertEqual(pyfile_getline(fp, 6),
                             FIRST_LINE_NORM[:6])

        # Test bytes
        with open(__file__, "rb") as fp:
            fp.seek(0)
            self.assertEqual(pyfile_getline(fp, -1),
                             FIRST_LINE.rstrip('\n').encode())
            fp.seek(0)
            self.assertEqual(pyfile_getline(fp, 0),
                             FIRST_LINE.encode())
            fp.seek(0)
            self.assertEqual(pyfile_getline(fp, 6),
                             FIRST_LINE.encode()[:6])

    def test_pyfile_writestring(self):
        # Test PyFile_WriteString(str, file): call file.write(str)
        writestr = _testlimitedcapi.pyfile_writestring

        with io.StringIO() as fp:
            self.assertEqual(writestr("a\xe9\u20ac\U0010FFFF".encode(), fp), 0)
            with self.assertRaises(UnicodeDecodeError):
                writestr(b"\xff", fp)
            with self.assertRaises(UnicodeDecodeError):
                writestr("\udc80".encode("utf-8", "surrogatepass"), fp)

            text = fp.getvalue()
            self.assertEqual(text, "a\xe9\u20ac\U0010FFFF")

        with self.assertRaises(SystemError):
            writestr(b"abc", NULL)

    def test_pyfile_writeobject(self):
        # Test PyFile_WriteObject(obj, file, flags):
        # - Call file.write(str(obj)) if flags equals Py_PRINT_RAW.
        # - Call file.write(repr(obj)) otherwise.
        writeobject = _testlimitedcapi.pyfile_writeobject
        Py_PRINT_RAW = 1

        with io.StringIO() as fp:
            # Test flags=Py_PRINT_RAW
            self.assertEqual(writeobject("raw", fp, Py_PRINT_RAW), 0)
            writeobject(NULL, fp, Py_PRINT_RAW)

            # Test flags=0
            self.assertEqual(writeobject("repr", fp, 0), 0)
            writeobject(NULL, fp, 0)

            text = fp.getvalue()
            self.assertEqual(text, "raw<NULL>'repr'<NULL>")

        # invalid file type
        for invalid_file in (123, "abc", object()):
            with self.subTest(file=invalid_file):
                with self.assertRaises(AttributeError):
                    writeobject("abc", invalid_file, Py_PRINT_RAW)

        with self.assertRaises(TypeError):
            writeobject("abc", NULL, 0)

    def test_pyobject_asfiledescriptor(self):
        # Test PyObject_AsFileDescriptor(obj):
        # - Return obj if obj is an integer.
        # - Return obj.fileno() otherwise.
        # File descriptor must be >= 0.
        asfd = _testlimitedcapi.pyobject_asfiledescriptor

        self.assertEqual(asfd(123), 123)
        self.assertEqual(asfd(0), 0)

        with open(__file__, "rb") as fp:
            self.assertEqual(asfd(fp), fp.fileno())

        # bool emits RuntimeWarning
        msg = r"bool is used as a file descriptor"
        with warnings_helper.check_warnings((msg, RuntimeWarning)):
            self.assertEqual(asfd(True), 1)

        class FakeFile:
            def __init__(self, fd):
                self.fd = fd
            def fileno(self):
                return self.fd

        # file descriptor must be positive
        with self.assertRaises(ValueError):
            asfd(-1)
        with self.assertRaises(ValueError):
            asfd(FakeFile(-1))

        # fileno() result must be an integer
        with self.assertRaises(TypeError):
            asfd(FakeFile("text"))

        # unsupported types
        for obj in ("string", ["list"], object()):
            with self.subTest(obj=obj):
                with self.assertRaises(TypeError):
                    asfd(obj)

        # CRASHES asfd(NULL)

    def test_pyfile_newstdprinter(self):
        # Test PyFile_NewStdPrinter()
        pyfile_newstdprinter = _testcapi.pyfile_newstdprinter

        file = pyfile_newstdprinter(STDOUT_FD)
        self.assertEqual(file.closed, False)
        self.assertIsNone(file.encoding)
        self.assertEqual(file.mode, "w")

        self.assertEqual(file.fileno(), STDOUT_FD)
        self.assertEqual(file.isatty(), os.isatty(STDOUT_FD))

        # flush() is a no-op
        self.assertIsNone(file.flush())

        # close() is a no-op
        self.assertIsNone(file.close())
        self.assertEqual(file.closed, False)

        support.check_disallow_instantiation(self, type(file))

    def test_pyfile_newstdprinter_write(self):
        # Test the write() method of PyFile_NewStdPrinter()
        pyfile_newstdprinter = _testcapi.pyfile_newstdprinter

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        try:
            old_stdout = os.dup(STDOUT_FD)
        except OSError as exc:
            # os.dup(STDOUT_FD) is not supported on WASI
            self.skipTest(f"os.dup() failed with {exc!r}")

        try:
            with open(filename, "wb") as fp:
                # PyFile_NewStdPrinter() only accepts fileno(stdout)
                # or fileno(stderr) file descriptor.
                fd = fp.fileno()
                os.dup2(fd, STDOUT_FD)

                file = pyfile_newstdprinter(STDOUT_FD)
                self.assertEqual(file.write("text"), 4)
                # The surrogate character is encoded with
                # the "surrogateescape" error handler
                self.assertEqual(file.write("[\udc80]"), 8)
        finally:
            os.dup2(old_stdout, STDOUT_FD)
            os.close(old_stdout)

        with open(filename, "r") as fp:
            self.assertEqual(fp.read(), "text[\\udc80]")

    def test_py_fopen(self):
        # Test Py_fopen() and Py_fclose()
        py_fopen = _testcapi.py_fopen

        with open(__file__, "rb") as fp:
            source = fp.read()

        for filename in (__file__, os.fsencode(__file__)):
            with self.subTest(filename=filename):
                data = py_fopen(filename, "rb")
                self.assertEqual(data, source[:256])

                data = py_fopen(os_helper.FakePath(filename), "rb")
                self.assertEqual(data, source[:256])

        filenames = [
            os_helper.TESTFN,
            os.fsencode(os_helper.TESTFN),
        ]
        if os_helper.TESTFN_UNDECODABLE is not None:
            filenames.append(os_helper.TESTFN_UNDECODABLE)
            filenames.append(os.fsdecode(os_helper.TESTFN_UNDECODABLE))
        if os_helper.TESTFN_UNENCODABLE is not None:
            filenames.append(os_helper.TESTFN_UNENCODABLE)
        for filename in filenames:
            with self.subTest(filename=filename):
                try:
                    with open(filename, "wb") as fp:
                        fp.write(source)
                except OSError:
                    # TESTFN_UNDECODABLE cannot be used to create a file
                    # on macOS/WASI.
                    filename = None
                    continue
                try:
                    data = py_fopen(filename, "rb")
                    self.assertEqual(data, source[:256])
                finally:
                    os_helper.unlink(filename)

        # embedded null character/byte in the filename
        with self.assertRaises(ValueError):
            py_fopen("a\x00b", "rb")
        with self.assertRaises(ValueError):
            py_fopen(b"a\x00b", "rb")

        # non-ASCII mode failing with "Invalid argument"
        with self.assertRaises(OSError):
            py_fopen(__file__, b"\xc2\x80")
        with self.assertRaises(OSError):
            # \x98 is invalid in cp1250, cp1251, cp1257
            # \x9d is invalid in cp1252-cp1255, cp1258
            py_fopen(__file__, b"\xc2\x98\xc2\x9d")
        # UnicodeDecodeError can come from the audit hook code
        with self.assertRaises((UnicodeDecodeError, OSError)):
            py_fopen(__file__, b"\x98\x9d")

        # invalid filename type
        for invalid_type in (123, object()):
            with self.subTest(filename=invalid_type):
                with self.assertRaises(TypeError):
                    py_fopen(invalid_type, "rb")

        if support.MS_WINDOWS:
            with self.assertRaises(OSError):
                # On Windows, the file mode is limited to 10 characters
                py_fopen(__file__, "rt+, ccs=UTF-8")

        # CRASHES py_fopen(NULL, 'rb')
        # CRASHES py_fopen(__file__, NULL)

    # TODO: Test Py_UniversalNewlineFgets()

    # PyFile_SetOpenCodeHook() and PyFile_OpenCode() are tested by
    # test_embed.test_open_code_hook()


if __name__ == "__main__":
    unittest.main()
