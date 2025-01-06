import io
import unittest

from test.support import import_helper, os_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')

NULL = None


class _TempFileMixin:
    def tearDown(self):
        os_helper.unlink(os_helper.TESTFN)


class TestPyFile_FromFd(_TempFileMixin, unittest.TestCase):
    # We don't apply heavy testing here, because inside it directly calls
    # `_io.open` which is fully tested in `test_io`.

    def test_file_from_fd(self):
        from_fd = _testcapi.file_from_fd
        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            file_obj = from_fd(f.fileno(), os_helper.TESTFN, "w",
                               1, "utf-8", "strict", "\n", 0)
            self.assertIsInstance(file_obj, io.TextIOWrapper)
            self.assertEqual(file_obj.name, f.fileno())

    def test_name_null(self):
        from_fd = _testcapi.file_from_fd
        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            file_obj = from_fd(f.fileno(), NULL, "w",
                               1, "utf-8", "strict", "\n", 0)
            self.assertIsInstance(file_obj, io.TextIOWrapper)
            self.assertEqual(file_obj.name, f.fileno())

    def test_name_invalid_utf(self):  # TODO: use bytes
        from_fd = _testcapi.file_from_fd
        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            file_obj = from_fd(f.fileno(), "abc\xe9", "w",
                               1, "utf-8", "strict", "\n", 0)
        self.assertIsInstance(file_obj, io.TextIOWrapper)

    def test_mode_as_null(self):
        from_fd = _testcapi.file_from_fd
        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            with self.assertRaisesRegex(
                TypeError,
                r"open\(\) argument 'mode' must be str, not None",
            ):
                from_fd(f.fileno(), "abc\xe9", NULL,
                        1, "utf-8", "strict", "\n", 0)

    def test_string_args_as_null(self):
        from_fd = _testcapi.file_from_fd

        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            file_obj = from_fd(f.fileno(), os_helper.TESTFN, "w",
                               1, NULL, "strict", "\n", 0)
        self.assertIsInstance(file_obj, io.TextIOWrapper)
        self.assertEqual(file_obj.encoding, "UTF-8")

        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            file_obj = from_fd(f.fileno(), os_helper.TESTFN, "w",
                               1, "utf-8", NULL, "\n", 0)
        self.assertIsInstance(file_obj, io.TextIOWrapper)
        self.assertEqual(file_obj.errors, "strict")

        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            file_obj = from_fd(f.fileno(), os_helper.TESTFN, "w",
                               1, "utf-8", "strict", NULL, 0)
        self.assertIsInstance(file_obj, io.TextIOWrapper)
        self.assertIsNone(file_obj.newlines)

    def test_string_args_as_invalid_utf(self):
        from_fd = _testcapi.file_from_fd
        invalid_utf = "\xc3\x28"  # TODO: use bytes

        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            with self.assertRaises((ValueError, LookupError)):
                from_fd(f.fileno(), os_helper.TESTFN, "w",
                        1, invalid_utf, "strict", "\n", 0)

        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            with self.assertRaises((ValueError, LookupError)):
                from_fd(f.fileno(), os_helper.TESTFN, "w",
                        1, "utf-8", invalid_utf, "\n", 0)

        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            with self.assertRaises((ValueError, LookupError)):
                from_fd(f.fileno(), os_helper.TESTFN, "w",
                        1, "utf-8", "strict", invalid_utf, 0)


class TestPyFile_GetLine(_TempFileMixin, unittest.TestCase):
    get_line = _testcapi.file_get_line

    def assertGetLineNegativeIndex(self, first_line, index, eof):
        assert index < 0
        if first_line.endswith("\n"):
            first_line = first_line.rstrip("\n")

        with open(os_helper.TESTFN, encoding="utf-8") as f:
            if eof:
                self.assertRaises(EOFError, self.get_line, f, index)
            else:
                self.assertEqual(self.get_line(f, index), first_line)

    def assertGetLine(self, first_line, eof=False):
        with open(os_helper.TESTFN, encoding="utf-8") as f:
            self.assertEqual(self.get_line(f, 0), first_line)
        with open(os_helper.TESTFN, encoding="utf-8") as f:
            self.assertEqual(self.get_line(f, 4), first_line[:4])
        with open(os_helper.TESTFN, encoding="utf-8") as f:
            self.assertEqual(self.get_line(f, len(first_line) + 1), first_line)
        with open(os_helper.TESTFN, encoding="utf-8") as f:
            self.assertEqual(self.get_line(f, _testcapi.INT_MAX), first_line)
        self.assertGetLineNegativeIndex(first_line, -1, eof=eof)
        self.assertGetLineNegativeIndex(first_line, _testcapi.INT_MIN, eof=eof)

    def test_file_empty_line(self):
        first_line = ""
        with open(os_helper.TESTFN, "w", encoding="utf-8"):
            pass
        self.assertGetLine(first_line, eof=True)

    def test_file_single_unicode_line(self):
        for line_end in ("", "\n"):
            with self.subTest(line_end=line_end):
                first_line = "text with юникод 统一码" + line_end
                with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
                    f.writelines([first_line])
                self.assertGetLine(first_line)

    def test_file_single_unicode_line_invalid_utf(self):
        first_line = "\xc3\x28\n"  # TODO: use bytes
        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            f.writelines([first_line])
        self.assertGetLine(first_line)

    def test_file_single_unicode_line_encoding_mismatch(self):
        first_line = "text with юникод 统一码\n"
        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            f.writelines([first_line])
        with open(os_helper.TESTFN, encoding="ascii") as f:
            self.assertRaises(UnicodeDecodeError, self.get_line, f, 0)

    def test_file_multiple_unicode_lines(self):
        first_line = "text with юникод 统一码\n"
        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            f.write(first_line)
            f.write("\n".join(["other", "line", ""]))
        self.assertGetLine(first_line)

    def test_file_get_multiple_lines(self):
        first_line = "text with юникод 统一码\n"
        second_line = "second line\n"
        with open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            f.writelines([first_line, second_line])
        with open(os_helper.TESTFN, encoding="utf-8") as f:
            self.assertEqual(self.get_line(f, 0), first_line)
            self.assertEqual(self.get_line(f, 0), second_line)

    def test_file_get_line_from_file_like(self):
        first_line = "text with юникод 统一码\n"
        second_line = "second line\n"
        contents = io.StringIO(f"{first_line}{second_line}")
        self.assertEqual(self.get_line(contents, 0), first_line)
        self.assertEqual(self.get_line(contents, 0), second_line)

    def test_file_single_bytes_line(self):
        # Write bytes directly:
        first_line = "text ॐ".encode("utf8")
        with open(os_helper.TESTFN, mode="wb") as f:
            f.writelines([first_line])

        with open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(self.get_line(f, 0), first_line)
        with open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(self.get_line(f, 4), b"text")
        with open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(self.get_line(f, -1), first_line)

    def test_empty_file(self):
        with open(os_helper.TESTFN, mode="w", encoding="utf-8"):
            pass
        self.assertGetLine("", eof=True)

    def test_wrong_args(self):
        class _BadIO(io.IOBase):
            def readline(self, size = 0):
                return object()

        self.assertRaises(TypeError, self.get_line, _BadIO(), 0)
        self.assertRaises(AttributeError, self.get_line, object(), 0)
        # CRASHES: self.get_line(NULL)


class TestPyFile_WriteObject(_TempFileMixin, unittest.TestCase):
    write = _testcapi.file_write_object

    def write_and_return(self, obj, flags=0):
        dst = io.StringIO()
        self.assertEqual(self.write(obj, dst, flags), 0)
        return dst.getvalue()

    def test_file_write_object(self):
        self.assertEqual(self.write_and_return(1), "1")
        self.assertEqual(
            self.write_and_return("text with юникод 统一码"),
            "'text with юникод 统一码'",
        )
        self.assertEqual(
            self.write_and_return("text with юникод 统一码", flags=_testcapi.Py_PRINT_RAW),
            "text with юникод 统一码",
        )
        self.assertEqual(self.write_and_return(False), "False")

    def test_file_write_custom_obj(self):
        class Custom:
            def __str__(self):
                return "<str>"
            def __repr__(self):
                return "<repr>"

        self.assertEqual(self.write_and_return(Custom()), "<repr>")
        self.assertEqual(
            self.write_and_return(Custom(), flags=_testcapi.Py_PRINT_RAW),
            "<str>",
        )

    def test_file_write_custom_obj_raises(self):
        class ReprRaises:
            def __repr__(self):
                raise ValueError("repr raised")

        with self.assertRaisesRegex(ValueError, "repr raised"):
            self.write_and_return(ReprRaises())
        with self.assertRaisesRegex(ValueError, "repr raised"):
            self.write_and_return(ReprRaises(), flags=_testcapi.Py_PRINT_RAW)

        class StrRaises:
            def __str__(self):
                raise ValueError("str raised")

        with self.assertRaisesRegex(ValueError, "str raised"):
            self.write_and_return(StrRaises(), flags=_testcapi.Py_PRINT_RAW)

    def test_file_write_null(self):
        self.assertEqual(self.write_and_return(NULL), "<NULL>")

    def test_file_write_to_real_file(self):
        obj = "text with юникод 统一码"
        with open(os_helper.TESTFN, mode="w", encoding="utf-8") as f:
            self.write(obj, f, _testcapi.Py_PRINT_RAW)
        with open(os_helper.TESTFN, encoding="utf-8") as f:
            self.assertEqual(f.read(), obj)

    def test_file_write_to_ascii_file(self):
        obj = "text with юникод 统一码"
        with open(os_helper.TESTFN, mode="w", encoding="ascii") as f:
            self.assertRaises(
                UnicodeEncodeError,
                self.write, obj, f, 0,
            )
            self.assertRaises(
                UnicodeEncodeError,
                self.write, obj, f, _testcapi.Py_PRINT_RAW,
            )

    def test_file_write_invalid(self):
        wr = self.write
        self.assertRaises(TypeError, wr, object(), io.BytesIO(), 0)
        self.assertRaises(AttributeError, wr, object(), object(), 0)
        self.assertRaises(TypeError, wr, object(), NULL, 0)
        self.assertRaises(AttributeError, wr, NULL, object(), 0)
        self.assertRaises(TypeError, wr, NULL, NULL, 0)

    def test_file_write_invalid_print_raw(self):
        wr = self.write
        raw = _testcapi.Py_PRINT_RAW
        self.assertRaises(TypeError, wr, object(), io.BytesIO(), raw)
        self.assertRaises(AttributeError, wr, object(), object(), raw)
        self.assertRaises(TypeError, wr, object(), NULL, raw)
        self.assertRaises(AttributeError, wr, NULL, object(), raw)
        self.assertRaises(TypeError, wr, NULL, NULL, raw)


class TestPyFile_WriteString(unittest.TestCase):
    write = _testcapi.file_write_string

    def write_and_return(self, string):
        dst = io.StringIO()
        self.assertEqual(self.write(string, dst), 0)
        return dst.getvalue()

    def test_file_write_string(self):
        self.assertEqual(self.write_and_return("abc"), "abc")
        self.assertEqual(
            self.write_and_return("text with юникод 统一码"),
            "text with юникод 统一码",
        )
        # TODO: use real invalid utf8 via bytes
        self.assertEqual(self.write_and_return("\xc3\x28"), "\xc3\x28")

    def test_invalid_write(self):
        self.assertRaises(AttributeError, self.write, "abc", object())
        self.assertRaises(SystemError, self.write, "", NULL)
        self.assertRaises(SystemError, self.write, NULL, NULL)
        # CRASHES: self.write(NULL, object())


class TestPyObject_AsFileDescriptor(_TempFileMixin, unittest.TestCase):
    as_fd = _testcapi.object_as_file_descriptor

    def test_object_as_file_descriptor(self):
        with open(os_helper.TESTFN, mode="w") as f:
            self.assertEqual(self.as_fd(f), f.fileno())
            self.assertEqual(self.as_fd(f.fileno()), f.fileno())

    def test_incorrect_descriptors(self):
        class _BadIO(io.IOBase):
            def fileno(self):
                return object()  # not int
        self.assertRaises(TypeError, self.as_fd, _BadIO())
        self.assertRaises(ValueError, self.as_fd, -1)
        self.assertRaises(TypeError, self.as_fd, 1.5)
        self.assertRaises(TypeError, self.as_fd, object())
        # CRASHES self.as_fd(NULL)


if __name__ == "__main__":
    unittest.main()
