import unittest
import io
import os

from test.support import import_helper, os_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')

NULL = None


class TestPyFileCAPI(unittest.TestCase):
    def tearDown(self):
        try:
            os.unlink(os_helper.TESTFN)
        except FileNotFoundError:
            pass

    def test_file_from_fd(self):
        # PyFile_FromFd()
        with open(os_helper.TESTFN, "w") as f:
            # raise ValueError(*file_mode, 0, "utf-8", "strict", "\n", 0,)
            file_obj = _testcapi.file_from_fd(
                f.fileno(), os_helper.TESTFN, "w",
                1, "utf-8", "strict", "\n", 0,
            )

        # We don't apply heavy testing here, because inside it directly calls
        # `_io.open` which is fully tested in `test_io`.
        self.assertIsInstance(file_obj, io.TextIOWrapper)

    def test_file_get_line(self):
        # PyFile_GetLine
        get_line = _testcapi.file_get_line

        # Create file with unicode content:
        first_line = "text with юникод 统一码"
        with open(os_helper.TESTFN, "w") as f:
            f.writelines([first_line])

        with open(os_helper.TESTFN) as f:
            self.assertEqual(get_line(f, 0), first_line)
        with open(os_helper.TESTFN) as f:
            self.assertEqual(get_line(f, 4), 'text')
        with open(os_helper.TESTFN) as f:
            self.assertEqual(get_line(f, -1), first_line)

        # Create file with bytes content:
        first_line = "text with юникод 统一码".encode("utf8")
        with open(os_helper.TESTFN, mode="wb") as f:
            f.writelines([first_line])

        with open(os_helper.TESTFN, 'rb') as f:
            self.assertEqual(get_line(f, 0), first_line)
        with open(os_helper.TESTFN, 'rb') as f:
            self.assertEqual(get_line(f, 4), b'text')
        with open(os_helper.TESTFN, 'rb') as f:
            self.assertEqual(get_line(f, -1), first_line)

        # Create empty file:
        with open(os_helper.TESTFN, mode="w") as f:
            pass

        with open(os_helper.TESTFN) as f:
            self.assertEqual(get_line(f, 0), '')
        with open(os_helper.TESTFN) as f:
            self.assertEqual(get_line(f, 4), '')
        with open(os_helper.TESTFN) as f:
            self.assertRaises(EOFError, get_line, f, -1)

        # Not `bytes` or `str` is returned:
        class _BadIO(io.IOBase):
            def readline(self, size = 0):
                return object()

        self.assertRaises(TypeError, get_line, _BadIO(), 0)
        self.assertRaises(AttributeError, get_line, object(), 0)
        # CRASHES: get_line(NULL)

    def test_file_write_object(self):
        # PyFile_WriteObject
        write = _testcapi.file_write_object

        def write_and_return(obj, flags=0):
            dst = io.StringIO()
            write(obj, dst, flags)
            return dst.getvalue()

        self.assertEqual(write_and_return(1), '1')
        self.assertEqual(write_and_return('1'), "'1'")
        self.assertEqual(write_and_return(False), 'False')
        self.assertEqual(write_and_return(b'123'), "b'123'")

        class Custom:
            def __str__(self):
                return '<str>'
            def __repr__(self):
                return '<repr>'

        self.assertEqual(write_and_return(Custom()), '<repr>')
        self.assertEqual(
            write_and_return(Custom(), flags=_testcapi.Py_PRINT_RAW),
            '<str>',
        )

        self.assertRaises(TypeError, write, object(), None)
        # CRASHES: write(NULL, io.StringIO(), flags)
        # CRASHES: write(NULL, NULL, flags)

    def test_file_write_string(self):
        # PyFile_WriteString
        write = _testcapi.file_write_string

        def write_and_return(string):
            dst = io.StringIO()
            write(string, dst)
            return dst.getvalue()

        self.assertEqual(write_and_return("abc"), "abc")
        self.assertEqual(
            write_and_return("text with юникод 统一码"),
            "text with юникод 统一码",
        )
        self.assertRaises(TypeError, write, object(), io.StringIO())
        self.assertRaises(AttributeError, write, 'abc', object())
        # CRASHES: write('', NULL)

    def test_object_as_file_descriptor(self):
        # PyObject_AsFileDescriptor()
        as_fd = _testcapi.object_as_file_descriptor
        with open(os_helper.TESTFN, mode="w") as f:
            self.assertEqual(as_fd(f), f.fileno())
        with open(os_helper.TESTFN, mode="w") as f:
            self.assertEqual(as_fd(f.fileno()), f.fileno())

        class _BadIO(io.IOBase):
            def fileno(self):
                return object()  # not int
        self.assertRaises(TypeError, as_fd, _BadIO())
        self.assertRaises(TypeError, as_fd, object())
        # CRASHES as_fd(NULL)
