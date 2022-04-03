"""test script for a few new invalid token catches"""

import sys
from test import support
from test.support import os_helper
from test.support import script_helper
import unittest

class EOFTestCase(unittest.TestCase):
    def test_EOF_single_quote(self):
        expect = "unterminated string literal (detected at line 1) (<string>, line 1)"
        for quote in ("'", "\""):
            try:
                eval(f"""{quote}this is a test\
                """)
            except SyntaxError as msg:
                self.assertEqual(str(msg), expect)
                self.assertEqual(msg.offset, 1)
            else:
                raise support.TestFailed

    def test_EOFS(self):
        expect = ("unterminated triple-quoted string literal (detected at line 1) (<string>, line 1)")
        try:
            eval("""'''this is a test""")
        except SyntaxError as msg:
            self.assertEqual(str(msg), expect)
            self.assertEqual(msg.offset, 1)
        else:
            raise support.TestFailed

    def test_EOFS_with_file(self):
        expect = ("(<string>, line 1)")
        with os_helper.temp_dir() as temp_dir:
            file_name = script_helper.make_script(temp_dir, 'foo', """'''this is \na \ntest""")
            rc, out, err = script_helper.assert_python_failure(file_name)
        self.assertIn(b'unterminated triple-quoted string literal (detected at line 3)', err)

    def test_eof_with_line_continuation(self):
        expect = "unexpected EOF while parsing (<string>, line 1)"
        try:
            compile('"\\xhh" \\',  '<string>', 'exec', dont_inherit=True)
        except SyntaxError as msg:
            self.assertEqual(str(msg), expect)
        else:
            raise support.TestFailed

    def test_line_continuation_EOF(self):
        """A continuation at the end of input must be an error; bpo2180."""
        expect = 'unexpected EOF while parsing (<string>, line 1)'
        with self.assertRaises(SyntaxError) as excinfo:
            exec('x = 5\\')
        self.assertEqual(str(excinfo.exception), expect)
        with self.assertRaises(SyntaxError) as excinfo:
            exec('\\')
        self.assertEqual(str(excinfo.exception), expect)

    @unittest.skipIf(not sys.executable, "sys.executable required")
    def test_line_continuation_EOF_from_file_bpo2180(self):
        """Ensure tok_nextc() does not add too many ending newlines."""
        with os_helper.temp_dir() as temp_dir:
            file_name = script_helper.make_script(temp_dir, 'foo', '\\')
            rc, out, err = script_helper.assert_python_failure(file_name)
            self.assertIn(b'unexpected EOF while parsing', err)
            self.assertIn(b'line 1', err)
            self.assertIn(b'\\', err)

            file_name = script_helper.make_script(temp_dir, 'foo', 'y = 6\\')
            rc, out, err = script_helper.assert_python_failure(file_name)
            self.assertIn(b'unexpected EOF while parsing', err)
            self.assertIn(b'line 1', err)
            self.assertIn(b'y = 6\\', err)

if __name__ == "__main__":
    unittest.main()
