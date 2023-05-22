"""
Tests for uu module.
Nick Mathewson
"""

import unittest
from test import support

import os
import stat
import sys
import uu
import io

plaintext = b"The symbols on top of your keyboard are !@#$%^&*()_+|~\n"

encodedtext = b"""\
M5&AE('-Y;6)O;',@;VX@=&]P(&]F('EO=7(@:V5Y8F]A<F0@87)E("% (R0E
*7B8J*"E?*WQ^"@  """

# Stolen from io.py
class FakeIO(io.TextIOWrapper):
    """Text I/O implementation using an in-memory buffer.

    Can be a used as a drop-in replacement for sys.stdin and sys.stdout.
    """

    # XXX This is really slow, but fully functional

    def __init__(self, initial_value="", encoding="utf-8",
                 errors="strict", newline="\n"):
        super(FakeIO, self).__init__(io.BytesIO(),
                                     encoding=encoding,
                                     errors=errors,
                                     newline=newline)
        self._encoding = encoding
        self._errors = errors
        if initial_value:
            if not isinstance(initial_value, str):
                initial_value = str(initial_value)
            self.write(initial_value)
            self.seek(0)

    def getvalue(self):
        self.flush()
        return self.buffer.getvalue().decode(self._encoding, self._errors)


def encodedtextwrapped(mode, filename, backtick=False):
    if backtick:
        res = (bytes("begin %03o %s\n" % (mode, filename), "ascii") +
               encodedtext.replace(b' ', b'`') + b"\n`\nend\n")
    else:
        res = (bytes("begin %03o %s\n" % (mode, filename), "ascii") +
               encodedtext + b"\n \nend\n")
    return res

class UUTest(unittest.TestCase):

    def test_encode(self):
        inp = io.BytesIO(plaintext)
        out = io.BytesIO()
        uu.encode(inp, out, "t1")
        self.assertEqual(out.getvalue(), encodedtextwrapped(0o666, "t1"))
        inp = io.BytesIO(plaintext)
        out = io.BytesIO()
        uu.encode(inp, out, "t1", 0o644)
        self.assertEqual(out.getvalue(), encodedtextwrapped(0o644, "t1"))
        inp = io.BytesIO(plaintext)
        out = io.BytesIO()
        uu.encode(inp, out, "t1", backtick=True)
        self.assertEqual(out.getvalue(), encodedtextwrapped(0o666, "t1", True))
        with self.assertRaises(TypeError):
            uu.encode(inp, out, "t1", 0o644, True)

    def test_decode(self):
        for backtick in True, False:
            inp = io.BytesIO(encodedtextwrapped(0o666, "t1", backtick=backtick))
            out = io.BytesIO()
            uu.decode(inp, out)
            self.assertEqual(out.getvalue(), plaintext)
            inp = io.BytesIO(
                b"UUencoded files may contain many lines,\n" +
                b"even some that have 'begin' in them.\n" +
                encodedtextwrapped(0o666, "t1", backtick=backtick)
            )
            out = io.BytesIO()
            uu.decode(inp, out)
            self.assertEqual(out.getvalue(), plaintext)

    def test_truncatedinput(self):
        inp = io.BytesIO(b"begin 644 t1\n" + encodedtext)
        out = io.BytesIO()
        try:
            uu.decode(inp, out)
            self.fail("No exception raised")
        except uu.Error as e:
            self.assertEqual(str(e), "Truncated input file")

    def test_missingbegin(self):
        inp = io.BytesIO(b"")
        out = io.BytesIO()
        try:
            uu.decode(inp, out)
            self.fail("No exception raised")
        except uu.Error as e:
            self.assertEqual(str(e), "No valid begin line found in input file")

    def test_garbage_padding(self):
        # Issue #22406
        encodedtext1 = (
            b"begin 644 file\n"
            # length 1; bits 001100 111111 111111 111111
            b"\x21\x2C\x5F\x5F\x5F\n"
            b"\x20\n"
            b"end\n"
        )
        encodedtext2 = (
            b"begin 644 file\n"
            # length 1; bits 001100 111111 111111 111111
            b"\x21\x2C\x5F\x5F\x5F\n"
            b"\x60\n"
            b"end\n"
        )
        plaintext = b"\x33"  # 00110011

        for encodedtext in encodedtext1, encodedtext2:
            with self.subTest("uu.decode()"):
                inp = io.BytesIO(encodedtext)
                out = io.BytesIO()
                uu.decode(inp, out, quiet=True)
                self.assertEqual(out.getvalue(), plaintext)

            with self.subTest("uu_codec"):
                import codecs
                decoded = codecs.decode(encodedtext, "uu_codec")
                self.assertEqual(decoded, plaintext)

    def test_newlines_escaped(self):
        # Test newlines are escaped with uu.encode
        inp = io.BytesIO(plaintext)
        out = io.BytesIO()
        filename = "test.txt\n\roverflow.txt"
        safefilename = b"test.txt\\n\\roverflow.txt"
        uu.encode(inp, out, filename)
        self.assertIn(safefilename, out.getvalue())

    def test_no_directory_traversal(self):
        relative_bad = b"""\
begin 644 ../../../../../../../../tmp/test1
$86)C"@``
`
end
"""
        with self.assertRaisesRegex(uu.Error, 'directory'):
            uu.decode(io.BytesIO(relative_bad))
        if os.altsep:
            relative_bad_bs = relative_bad.replace(b'/', b'\\')
            with self.assertRaisesRegex(uu.Error, 'directory'):
                uu.decode(io.BytesIO(relative_bad_bs))

        absolute_bad = b"""\
begin 644 /tmp/test2
$86)C"@``
`
end
"""
        with self.assertRaisesRegex(uu.Error, 'directory'):
            uu.decode(io.BytesIO(absolute_bad))
        if os.altsep:
            absolute_bad_bs = absolute_bad.replace(b'/', b'\\')
            with self.assertRaisesRegex(uu.Error, 'directory'):
                uu.decode(io.BytesIO(absolute_bad_bs))


class UUStdIOTest(unittest.TestCase):

    def setUp(self):
        self.stdin = sys.stdin
        self.stdout = sys.stdout

    def tearDown(self):
        sys.stdin = self.stdin
        sys.stdout = self.stdout

    def test_encode(self):
        sys.stdin = FakeIO(plaintext.decode("ascii"))
        sys.stdout = FakeIO()
        uu.encode("-", "-", "t1", 0o666)
        self.assertEqual(sys.stdout.getvalue(),
                         encodedtextwrapped(0o666, "t1").decode("ascii"))

    def test_decode(self):
        sys.stdin = FakeIO(encodedtextwrapped(0o666, "t1").decode("ascii"))
        sys.stdout = FakeIO()
        uu.decode("-", "-")
        stdout = sys.stdout
        sys.stdout = self.stdout
        sys.stdin = self.stdin
        self.assertEqual(stdout.getvalue(), plaintext.decode("ascii"))

class UUFileTest(unittest.TestCase):

    def setUp(self):
        self.tmpin  = support.TESTFN + "i"
        self.tmpout = support.TESTFN + "o"
        self.addCleanup(support.unlink, self.tmpin)
        self.addCleanup(support.unlink, self.tmpout)

    def test_encode(self):
        with open(self.tmpin, 'wb') as fin:
            fin.write(plaintext)

        with open(self.tmpin, 'rb') as fin:
            with open(self.tmpout, 'wb') as fout:
                uu.encode(fin, fout, self.tmpin, mode=0o644)

        with open(self.tmpout, 'rb') as fout:
            s = fout.read()
        self.assertEqual(s, encodedtextwrapped(0o644, self.tmpin))

        # in_file and out_file as filenames
        uu.encode(self.tmpin, self.tmpout, self.tmpin, mode=0o644)
        with open(self.tmpout, 'rb') as fout:
            s = fout.read()
        self.assertEqual(s, encodedtextwrapped(0o644, self.tmpin))

    def test_decode(self):
        with open(self.tmpin, 'wb') as f:
            f.write(encodedtextwrapped(0o644, self.tmpout))

        with open(self.tmpin, 'rb') as f:
            uu.decode(f)

        with open(self.tmpout, 'rb') as f:
            s = f.read()
        self.assertEqual(s, plaintext)
        # XXX is there an xp way to verify the mode?

    def test_decode_filename(self):
        with open(self.tmpin, 'wb') as f:
            f.write(encodedtextwrapped(0o644, self.tmpout))

        uu.decode(self.tmpin)

        with open(self.tmpout, 'rb') as f:
            s = f.read()
        self.assertEqual(s, plaintext)

    def test_decodetwice(self):
        # Verify that decode() will refuse to overwrite an existing file
        with open(self.tmpin, 'wb') as f:
            f.write(encodedtextwrapped(0o644, self.tmpout))
        with open(self.tmpin, 'rb') as f:
            uu.decode(f)

        with open(self.tmpin, 'rb') as f:
            self.assertRaises(uu.Error, uu.decode, f)

    def test_decode_mode(self):
        # Verify that decode() will set the given mode for the out_file
        expected_mode = 0o444
        with open(self.tmpin, 'wb') as f:
            f.write(encodedtextwrapped(expected_mode, self.tmpout))

        # make file writable again, so it can be removed (Windows only)
        self.addCleanup(os.chmod, self.tmpout, expected_mode | stat.S_IWRITE)

        with open(self.tmpin, 'rb') as f:
            uu.decode(f)

        self.assertEqual(
            stat.S_IMODE(os.stat(self.tmpout).st_mode),
            expected_mode
        )


if __name__=="__main__":
    unittest.main()
