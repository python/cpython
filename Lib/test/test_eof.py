"""test script for a few new invalid token catches"""

import subprocess
import sys
import tempfile
from test import support
import unittest

class EOFTestCase(unittest.TestCase):
    def test_EOFC(self):
        expect = "EOL while scanning string literal (<string>, line 1)"
        try:
            eval("""'this is a test\
            """)
        except SyntaxError as msg:
            self.assertEqual(str(msg), expect)
        else:
            raise support.TestFailed

    def test_EOFS(self):
        expect = ("EOF while scanning triple-quoted string literal "
                  "(<string>, line 1)")
        try:
            eval("""'''this is a test""")
        except SyntaxError as msg:
            self.assertEqual(str(msg), expect)
        else:
            raise support.TestFailed

    def test_line_continuation_EOF(self):
        """A contination at the end of input must be an error; bpo2180."""
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
        with tempfile.NamedTemporaryFile() as temp_f:
            temp_f.write(b'\\')
            temp_f.flush()
            proc = subprocess.run([sys.executable, temp_f.name],
                                  capture_output=True)
            self.assertIn(b'unexpected EOF while parsing', proc.stderr)
            self.assertGreater(proc.returncode, 0)
            temp_f.seek(0)
            temp_f.write(b'y = 6\\')
            temp_f.flush()
            proc = subprocess.run([sys.executable, temp_f.name],
                                  capture_output=True)
            self.assertIn(b'unexpected EOF while parsing', proc.stderr)
            self.assertGreater(proc.returncode, 0)

if __name__ == "__main__":
    unittest.main()
