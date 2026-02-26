import unittest
import py_compile
import os

class TestParserUTF7Newline(unittest.TestCase):
    def test_utf7_r_after_coding_cookie(self):
        # This reproduced a SystemError in string_parser.c
        # where \r introduced by codec caused the lexer to
        # produce a broken token.
        filename = 'test_utf7_r.py'
        if os.path.exists(filename):
            os.remove(filename)
        self.addCleanup(os.remove, filename)

        # '+AA0-' is UTF-7 for '\r'.
        # The '-' is optional if followed by non-base64.
        with open(filename, 'wb') as f:
            f.write(b"#coding=u7+AA0''")

        try:
            py_compile.compile(filename, doraise=True)
        except SyntaxError:
            # We don't care if it's a syntax error (it shouldn't be,
            # but that's not the bug), we care that it doesn't
            # raise SystemError.
            pass
        except SystemError as e:
            self.fail(f"SystemError raised: {e}")

if __name__ == "__main__":
    unittest.main()
