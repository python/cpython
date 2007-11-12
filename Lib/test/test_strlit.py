r"""Test correct treatment of various string literals by the parser.

There are four types of string literals:

    'abc'   -- normal str
    r'abc'  -- raw str
    b'xyz'  -- normal bytes
    br'xyz' -- raw bytes

The difference between normal and raw strings is of course that in a
raw string, \ escapes (while still used to determine the end of the
literal) are not interpreted, so that r'\x00' contains four
characters: a backslash, an x, and two zeros; while '\x00' contains a
single character (code point zero).

The tricky thing is what should happen when non-ASCII bytes are used
inside literals.  For bytes literals, this is considered illegal.  But
for str literals, those bytes are supposed to be decoded using the
encoding declared for the file (UTF-8 by default).

We have to test this with various file encodings.  We also test it with
exec()/eval(), which uses a different code path.

This file is really about correct treatment of encodings and
backslashes.  It doens't concern itself with issues like single
vs. double quotes or singly- vs. triply-quoted strings: that's dealt
with elsewhere (I assume).
"""

import os
import sys
import shutil
import tempfile
import unittest


TEMPLATE = r"""# coding: %s
a = 'x'
assert ord(a) == 120
b = '\x01'
assert ord(b) == 1
c = r'\x01'
assert list(map(ord, c)) == [92, 120, 48, 49]
d = '\x81'
assert ord(d) == 0x81
e = r'\x81'
assert list(map(ord, e)) == [92, 120, 56, 49]
f = '\u1881'
assert ord(f) == 0x1881
g = r'\u1881'
assert list(map(ord, g)) == [92, 117, 49, 56, 56, 49]
"""


def byte(i):
    return bytes([i])


class TestLiterals(unittest.TestCase):

    def setUp(self):
        self.save_path = sys.path[:]
        self.tmpdir = tempfile.mkdtemp()
        sys.path.insert(0, self.tmpdir)

    def tearDown(self):
        sys.path = self.save_path
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def test_template(self):
        # Check that the template doesn't contain any non-printables
        # except for \n.
        for c in TEMPLATE:
            assert c == '\n' or ' ' <= c <= '~', repr(c)

    def test_eval_str_normal(self):
        self.assertEqual(eval(""" 'x' """), 'x')
        self.assertEqual(eval(r""" '\x01' """), chr(1))
        self.assertEqual(eval(""" '\x01' """), chr(1))
        self.assertEqual(eval(r""" '\x81' """), chr(0x81))
        self.assertEqual(eval(""" '\x81' """), chr(0x81))
        self.assertEqual(eval(r""" '\u1881' """), chr(0x1881))
        self.assertEqual(eval(""" '\u1881' """), chr(0x1881))

    def test_eval_str_raw(self):
        self.assertEqual(eval(""" r'x' """), 'x')
        self.assertEqual(eval(r""" r'\x01' """), '\\' + 'x01')
        self.assertEqual(eval(""" r'\x01' """), chr(1))
        self.assertEqual(eval(r""" r'\x81' """), '\\' + 'x81')
        self.assertEqual(eval(""" r'\x81' """), chr(0x81))
        self.assertEqual(eval(r""" r'\u1881' """), '\\' + 'u1881')
        self.assertEqual(eval(""" r'\u1881' """), chr(0x1881))

    def test_eval_bytes_normal(self):
        self.assertEqual(eval(""" b'x' """), b'x')
        self.assertEqual(eval(r""" b'\x01' """), byte(1))
        self.assertEqual(eval(""" b'\x01' """), byte(1))
        self.assertEqual(eval(r""" b'\x81' """), byte(0x81))
        self.assertRaises(SyntaxError, eval, """ b'\x81' """)
        self.assertEqual(eval(r""" b'\u1881' """), b'\\' + b'u1881')
        self.assertRaises(SyntaxError, eval, """ b'\u1881' """)

    def test_eval_bytes_raw(self):
        self.assertEqual(eval(""" br'x' """), b'x')
        self.assertEqual(eval(r""" br'\x01' """), b'\\' + b'x01')
        self.assertEqual(eval(""" br'\x01' """), byte(1))
        self.assertEqual(eval(r""" br'\x81' """), b"\\" + b"x81")
        self.assertRaises(SyntaxError, eval, """ br'\x81' """)
        self.assertEqual(eval(r""" br'\u1881' """), b"\\" + b"u1881")
        self.assertRaises(SyntaxError, eval, """ br'\u1881' """)

    def check_encoding(self, encoding, extra=""):
        modname = "xx_" + encoding.replace("-", "_")
        fn = os.path.join(self.tmpdir, modname + ".py")
        f = open(fn, "w", encoding=encoding)
        try:
            f.write(TEMPLATE % encoding)
            f.write(extra)
        finally:
            f.close()
        __import__(modname)
        del sys.modules[modname]

    def test_file_utf_8(self):
        extra = "z = '\u1234'; assert ord(z) == 0x1234\n"
        self.check_encoding("utf-8", extra)

    def test_file_utf_8_error(self):
        extra = "b'\x80'\n"
        self.assertRaises(SyntaxError, self.check_encoding, "utf-8", extra)

    def test_file_utf8(self):
        self.check_encoding("utf8")

    def test_file_iso_8859_1(self):
        self.check_encoding("iso-8859-1")

    def test_file_latin_1(self):
        self.check_encoding("latin-1")

    def test_file_latin9(self):
        self.check_encoding("latin9")


if __name__ == "__main__":
    # Hack so that error messages containing non-ASCII can be printed
    sys.stdout._encoding = sys.stderr._encoding = "utf-8"
    unittest.main()
