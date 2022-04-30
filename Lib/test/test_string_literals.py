r"""Test correct treatment of various string literals by the parser.

There are four types of string literals:

    'abc'             -- normal str
    r'abc'            -- raw str
    b'xyz'            -- normal bytes
    br'xyz' | rb'xyz' -- raw bytes

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
backslashes.  It doesn't concern itself with issues like single
vs. double quotes or singly- vs. triply-quoted strings: that's dealt
with elsewhere (I assume).
"""

import os
import sys
import shutil
import tempfile
import unittest
import warnings


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
h = '\U0001d120'
assert ord(h) == 0x1d120
i = r'\U0001d120'
assert list(map(ord, i)) == [92, 85, 48, 48, 48, 49, 100, 49, 50, 48]
"""


def byte(i):
    return bytes([i])


class TestLiterals(unittest.TestCase):

    def setUp(self):
        self.save_path = sys.path[:]
        self.tmpdir = tempfile.mkdtemp()
        sys.path.insert(0, self.tmpdir)

    def tearDown(self):
        sys.path[:] = self.save_path
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
        self.assertEqual(eval(r""" '\U0001d120' """), chr(0x1d120))
        self.assertEqual(eval(""" '\U0001d120' """), chr(0x1d120))

    def test_eval_str_incomplete(self):
        self.assertRaises(SyntaxError, eval, r""" '\x' """)
        self.assertRaises(SyntaxError, eval, r""" '\x0' """)
        self.assertRaises(SyntaxError, eval, r""" '\u' """)
        self.assertRaises(SyntaxError, eval, r""" '\u0' """)
        self.assertRaises(SyntaxError, eval, r""" '\u00' """)
        self.assertRaises(SyntaxError, eval, r""" '\u000' """)
        self.assertRaises(SyntaxError, eval, r""" '\U' """)
        self.assertRaises(SyntaxError, eval, r""" '\U0' """)
        self.assertRaises(SyntaxError, eval, r""" '\U00' """)
        self.assertRaises(SyntaxError, eval, r""" '\U000' """)
        self.assertRaises(SyntaxError, eval, r""" '\U0000' """)
        self.assertRaises(SyntaxError, eval, r""" '\U00000' """)
        self.assertRaises(SyntaxError, eval, r""" '\U000000' """)
        self.assertRaises(SyntaxError, eval, r""" '\U0000000' """)

    def test_eval_str_invalid_escape(self):
        for b in range(1, 128):
            if b in b"""\n\r"'01234567NU\\abfnrtuvx""":
                continue
            with self.assertWarns(DeprecationWarning):
                self.assertEqual(eval(r"'\%c'" % b), '\\' + chr(b))

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('always', category=DeprecationWarning)
            eval("'''\n\\z'''")
        self.assertEqual(len(w), 1)
        self.assertEqual(str(w[0].message), r"invalid escape sequence '\z'")
        self.assertEqual(w[0].filename, '<string>')
        self.assertEqual(w[0].lineno, 1)

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('error', category=DeprecationWarning)
            with self.assertRaises(SyntaxError) as cm:
                eval("'''\n\\z'''")
            exc = cm.exception
        self.assertEqual(w, [])
        self.assertEqual(exc.msg, r"invalid escape sequence '\z'")
        self.assertEqual(exc.filename, '<string>')
        self.assertEqual(exc.lineno, 1)
        self.assertEqual(exc.offset, 1)

    def test_eval_str_invalid_octal_escape(self):
        for i in range(0o400, 0o1000):
            with self.assertWarns(DeprecationWarning):
                self.assertEqual(eval(r"'\%o'" % i), chr(i))

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('always', category=DeprecationWarning)
            eval("'''\n\\407'''")
        self.assertEqual(len(w), 1)
        self.assertEqual(str(w[0].message),
                         r"invalid octal escape sequence '\407'")
        self.assertEqual(w[0].filename, '<string>')
        self.assertEqual(w[0].lineno, 1)

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('error', category=DeprecationWarning)
            with self.assertRaises(SyntaxError) as cm:
                eval("'''\n\\407'''")
            exc = cm.exception
        self.assertEqual(w, [])
        self.assertEqual(exc.msg, r"invalid octal escape sequence '\407'")
        self.assertEqual(exc.filename, '<string>')
        self.assertEqual(exc.lineno, 1)
        self.assertEqual(exc.offset, 1)

    def test_eval_str_raw(self):
        self.assertEqual(eval(""" r'x' """), 'x')
        self.assertEqual(eval(r""" r'\x01' """), '\\' + 'x01')
        self.assertEqual(eval(""" r'\x01' """), chr(1))
        self.assertEqual(eval(r""" r'\x81' """), '\\' + 'x81')
        self.assertEqual(eval(""" r'\x81' """), chr(0x81))
        self.assertEqual(eval(r""" r'\u1881' """), '\\' + 'u1881')
        self.assertEqual(eval(""" r'\u1881' """), chr(0x1881))
        self.assertEqual(eval(r""" r'\U0001d120' """), '\\' + 'U0001d120')
        self.assertEqual(eval(""" r'\U0001d120' """), chr(0x1d120))

    def test_eval_bytes_normal(self):
        self.assertEqual(eval(""" b'x' """), b'x')
        self.assertEqual(eval(r""" b'\x01' """), byte(1))
        self.assertEqual(eval(""" b'\x01' """), byte(1))
        self.assertEqual(eval(r""" b'\x81' """), byte(0x81))
        self.assertRaises(SyntaxError, eval, """ b'\x81' """)
        self.assertEqual(eval(r""" br'\u1881' """), b'\\' + b'u1881')
        self.assertRaises(SyntaxError, eval, """ b'\u1881' """)
        self.assertEqual(eval(r""" br'\U0001d120' """), b'\\' + b'U0001d120')
        self.assertRaises(SyntaxError, eval, """ b'\U0001d120' """)

    def test_eval_bytes_incomplete(self):
        self.assertRaises(SyntaxError, eval, r""" b'\x' """)
        self.assertRaises(SyntaxError, eval, r""" b'\x0' """)

    def test_eval_bytes_invalid_escape(self):
        for b in range(1, 128):
            if b in b"""\n\r"'01234567\\abfnrtvx""":
                continue
            with self.assertWarns(DeprecationWarning):
                self.assertEqual(eval(r"b'\%c'" % b), b'\\' + bytes([b]))

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('always', category=DeprecationWarning)
            eval("b'''\n\\z'''")
        self.assertEqual(len(w), 1)
        self.assertEqual(str(w[0].message), r"invalid escape sequence '\z'")
        self.assertEqual(w[0].filename, '<string>')
        self.assertEqual(w[0].lineno, 1)

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('error', category=DeprecationWarning)
            with self.assertRaises(SyntaxError) as cm:
                eval("b'''\n\\z'''")
            exc = cm.exception
        self.assertEqual(w, [])
        self.assertEqual(exc.msg, r"invalid escape sequence '\z'")
        self.assertEqual(exc.filename, '<string>')
        self.assertEqual(exc.lineno, 1)

    def test_eval_bytes_invalid_octal_escape(self):
        for i in range(0o400, 0o1000):
            with self.assertWarns(DeprecationWarning):
                self.assertEqual(eval(r"b'\%o'" % i), bytes([i & 0o377]))

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('always', category=DeprecationWarning)
            eval("b'''\n\\407'''")
        self.assertEqual(len(w), 1)
        self.assertEqual(str(w[0].message),
                         r"invalid octal escape sequence '\407'")
        self.assertEqual(w[0].filename, '<string>')
        self.assertEqual(w[0].lineno, 1)

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter('error', category=DeprecationWarning)
            with self.assertRaises(SyntaxError) as cm:
                eval("b'''\n\\407'''")
            exc = cm.exception
        self.assertEqual(w, [])
        self.assertEqual(exc.msg, r"invalid octal escape sequence '\407'")
        self.assertEqual(exc.filename, '<string>')
        self.assertEqual(exc.lineno, 1)

    def test_eval_bytes_raw(self):
        self.assertEqual(eval(""" br'x' """), b'x')
        self.assertEqual(eval(""" rb'x' """), b'x')
        self.assertEqual(eval(r""" br'\x01' """), b'\\' + b'x01')
        self.assertEqual(eval(r""" rb'\x01' """), b'\\' + b'x01')
        self.assertEqual(eval(""" br'\x01' """), byte(1))
        self.assertEqual(eval(""" rb'\x01' """), byte(1))
        self.assertEqual(eval(r""" br'\x81' """), b"\\" + b"x81")
        self.assertEqual(eval(r""" rb'\x81' """), b"\\" + b"x81")
        self.assertRaises(SyntaxError, eval, """ br'\x81' """)
        self.assertRaises(SyntaxError, eval, """ rb'\x81' """)
        self.assertEqual(eval(r""" br'\u1881' """), b"\\" + b"u1881")
        self.assertEqual(eval(r""" rb'\u1881' """), b"\\" + b"u1881")
        self.assertRaises(SyntaxError, eval, """ br'\u1881' """)
        self.assertRaises(SyntaxError, eval, """ rb'\u1881' """)
        self.assertEqual(eval(r""" br'\U0001d120' """), b"\\" + b"U0001d120")
        self.assertEqual(eval(r""" rb'\U0001d120' """), b"\\" + b"U0001d120")
        self.assertRaises(SyntaxError, eval, """ br'\U0001d120' """)
        self.assertRaises(SyntaxError, eval, """ rb'\U0001d120' """)
        self.assertRaises(SyntaxError, eval, """ bb'' """)
        self.assertRaises(SyntaxError, eval, """ rr'' """)
        self.assertRaises(SyntaxError, eval, """ brr'' """)
        self.assertRaises(SyntaxError, eval, """ bbr'' """)
        self.assertRaises(SyntaxError, eval, """ rrb'' """)
        self.assertRaises(SyntaxError, eval, """ rbb'' """)

    def test_eval_str_u(self):
        self.assertEqual(eval(""" u'x' """), 'x')
        self.assertEqual(eval(""" U'\u00e4' """), 'ä')
        self.assertEqual(eval(""" u'\N{LATIN SMALL LETTER A WITH DIAERESIS}' """), 'ä')
        self.assertRaises(SyntaxError, eval, """ ur'' """)
        self.assertRaises(SyntaxError, eval, """ ru'' """)
        self.assertRaises(SyntaxError, eval, """ bu'' """)
        self.assertRaises(SyntaxError, eval, """ ub'' """)

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
        self.check_encoding("utf-8")

    def test_file_iso_8859_1(self):
        self.check_encoding("iso-8859-1")

    def test_file_latin_1(self):
        self.check_encoding("latin-1")

    def test_file_latin9(self):
        self.check_encoding("latin9")


if __name__ == "__main__":
    unittest.main()
