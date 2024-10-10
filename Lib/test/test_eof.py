"""test script for a few new invalid token catches"""

import sys
from codecs import BOM_UTF8
from test import support
from test.support import os_helper
from test.support import script_helper
from test.support import warnings_helper
import unittest

class EOFTestCase(unittest.TestCase):
    def test_EOF_single_quote(self):
        expect = "unterminated string literal (detected at line 1) (<string>, line 1)"
        for quote in ("'", "\""):
            with self.assertRaises(SyntaxError) as cm:
                eval(f"""{quote}this is a test\
                """)
            self.assertEqual(str(cm.exception), expect)
            self.assertEqual(cm.exception.offset, 1)

    def test_EOFS(self):
        expect = ("unterminated triple-quoted string literal (detected at line 3) (<string>, line 1)")
        with self.assertRaises(SyntaxError) as cm:
            eval("""ä = '''thîs is \na \ntest""")
        self.assertEqual(str(cm.exception), expect)
        self.assertEqual(cm.exception.text, "ä = '''thîs is ")
        self.assertEqual(cm.exception.offset, 5)

        with self.assertRaises(SyntaxError) as cm:
            eval("""ä = '''thîs is \na \ntest""".encode())
        self.assertEqual(str(cm.exception), expect)
        self.assertEqual(cm.exception.text, "ä = '''thîs is ")
        self.assertEqual(cm.exception.offset, 5)

        with self.assertRaises(SyntaxError) as cm:
            eval(BOM_UTF8 + """ä = '''thîs is \na \ntest""".encode())
        self.assertEqual(str(cm.exception), expect)
        self.assertEqual(cm.exception.text, "ä = '''thîs is ")
        self.assertEqual(cm.exception.offset, 5)

        with self.assertRaises(SyntaxError) as cm:
            eval("""# coding: latin1\nä = '''thîs is \na \ntest""".encode('latin1'))
        self.assertEqual(str(cm.exception), "unterminated triple-quoted string literal (detected at line 4) (<string>, line 2)")
        self.assertEqual(cm.exception.text, "ä = '''thîs is ")
        self.assertEqual(cm.exception.offset, 5)

    def test_EOFS_with_file(self):
        expect = ("(<string>, line 1)")
        with os_helper.temp_dir() as temp_dir:
            file_name = script_helper.make_script(temp_dir, 'foo',
                                                  """ä = '''thîs is \na \ntest""")
            rc, out, err = script_helper.assert_python_failure('-X', 'utf8', file_name)
            err = err.decode().splitlines()
            self.assertEqual(err[-3:], [
                "    ä = '''thîs is ",
                '        ^',
                'SyntaxError: unterminated triple-quoted string literal (detected at line 3)'])

            file_name = script_helper.make_script(temp_dir, 'foo',
                                                  """ä = '''thîs is \na \ntest""".encode())
            rc, out, err = script_helper.assert_python_failure('-X', 'utf8', file_name)
            err = err.decode().splitlines()
            self.assertEqual(err[-3:], [
                "    ä = '''thîs is ",
                '        ^',
                'SyntaxError: unterminated triple-quoted string literal (detected at line 3)'])

            file_name = script_helper.make_script(temp_dir, 'foo',
                                                  BOM_UTF8 + """ä = '''thîs is \na \ntest""".encode())
            rc, out, err = script_helper.assert_python_failure('-X', 'utf8', file_name)
            err = err.decode().splitlines()
            self.assertEqual(err[-3:], [
                "    ä = '''thîs is ",
                '        ^',
                'SyntaxError: unterminated triple-quoted string literal (detected at line 3)'])

            file_name = script_helper.make_script(temp_dir, 'foo',
                                                  """# coding: latin1\nä = '''thîs is \na \ntest""".encode('latin1'))
            rc, out, err = script_helper.assert_python_failure('-X', 'utf8', file_name)
            err = err.decode().splitlines()
            self.assertEqual(err[-3:], [
                "    ä = '''thîs is ",
                '        ^',
                'SyntaxError: unterminated triple-quoted string literal (detected at line 4)'])

    @warnings_helper.ignore_warnings(category=SyntaxWarning)
    def test_eof_with_line_continuation(self):
        expect = "unexpected EOF while parsing (<string>, line 1)"
        with self.assertRaises(SyntaxError) as cm:
            compile('"\\Xhh" \\', '<string>', 'exec')
        self.assertEqual(str(cm.exception), expect)

    def test_line_continuation_EOF(self):
        """A continuation at the end of input must be an error; bpo2180."""
        expect = 'unexpected EOF while parsing (<string>, line 1)'
        with self.assertRaises(SyntaxError) as cm:
            exec('ä = 5\\')
        self.assertEqual(str(cm.exception), expect)
        self.assertEqual(cm.exception.text, 'ä = 5\\\n')
        self.assertEqual(cm.exception.offset, 7)

        with self.assertRaises(SyntaxError) as cm:
            exec('ä = 5\\'.encode())
        self.assertEqual(str(cm.exception), expect)
        self.assertEqual(cm.exception.text, 'ä = 5\\\n')
        self.assertEqual(cm.exception.offset, 7)

        with self.assertRaises(SyntaxError) as cm:
            exec('# coding:latin1\nä = 5\\'.encode('latin1'))
        self.assertEqual(str(cm.exception),
                         'unexpected EOF while parsing (<string>, line 2)')
        self.assertEqual(cm.exception.text, 'ä = 5\\\n')
        self.assertEqual(cm.exception.offset, 7)

        with self.assertRaises(SyntaxError) as cm:
            exec(BOM_UTF8 + 'ä = 5\\'.encode())
        self.assertEqual(str(cm.exception), expect)
        self.assertEqual(cm.exception.text, 'ä = 5\\\n')
        self.assertEqual(cm.exception.offset, 7)

        with self.assertRaises(SyntaxError) as cm:
            exec('\\')
        self.assertEqual(str(cm.exception), expect)

    @unittest.skipIf(not sys.executable, "sys.executable required")
    def test_line_continuation_EOF_from_file_bpo2180(self):
        """Ensure tok_nextc() does not add too many ending newlines."""
        with os_helper.temp_dir() as temp_dir:
            file_name = script_helper.make_script(temp_dir, 'foo', '\\')
            rc, out, err = script_helper.assert_python_failure('-X', 'utf8', file_name)
            err = err.decode().splitlines()
            self.assertEqual(err[-2:], [
                '    \\',
                'SyntaxError: unexpected EOF while parsing'])
            self.assertEqual(err[-3][-8:], ', line 1', err)

            file_name = script_helper.make_script(temp_dir, 'foo', 'ä = 6\\')
            rc, out, err = script_helper.assert_python_failure('-X', 'utf8', file_name)
            err = err.decode().splitlines()
            self.assertEqual(err[-3:], [
                '    ä = 6\\',
                '          ^',
                'SyntaxError: unexpected EOF while parsing'])
            self.assertEqual(err[-4][-8:], ', line 1', err)

            file_name = script_helper.make_script(temp_dir, 'foo',
                                                  '# coding:latin1\n'
                                                  'ä = 7\\'.encode('latin1'))
            rc, out, err = script_helper.assert_python_failure('-X', 'utf8', file_name)
            err = err.decode().splitlines()
            self.assertEqual(err[-3:], [
                '    ä = 7\\',
                '          ^',
                'SyntaxError: unexpected EOF while parsing'])
            self.assertEqual(err[-4][-8:], ', line 2', err)

            file_name = script_helper.make_script(temp_dir, 'foo',
                                                  BOM_UTF8 + 'ä = 8\\'.encode())
            rc, out, err = script_helper.assert_python_failure('-X', 'utf8', file_name)
            err = err.decode().splitlines()
            self.assertEqual(err[-3:], [
                '    ä = 8\\',
                '          ^',
                'SyntaxError: unexpected EOF while parsing'])
            self.assertEqual(err[-4][-8:], ', line 1', err)


if __name__ == "__main__":
    unittest.main()
