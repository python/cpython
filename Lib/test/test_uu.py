"""
Tests for uu module.
Nick Mathewson
"""

import unittest
from test import support

import sys, os
import uu
import io

plaintext = b"The smooth-scaled python crept over the sleeping dog\n"

encodedtext = b"""\
M5&AE('-M;V]T:\"US8V%L960@<'ET:&]N(&-R97!T(&]V97(@=&AE('-L965P
(:6YG(&1O9PH """

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


def encodedtextwrapped(mode, filename):
    return (bytes("begin %03o %s\n" % (mode, filename), "ascii") +
            encodedtext + b"\n \nend\n")

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

    def test_decode(self):
        inp = io.BytesIO(encodedtextwrapped(0o666, "t1"))
        out = io.BytesIO()
        uu.decode(inp, out)
        self.assertEqual(out.getvalue(), plaintext)
        inp = io.BytesIO(
            b"UUencoded files may contain many lines,\n" +
            b"even some that have 'begin' in them.\n" +
            encodedtextwrapped(0o666, "t1")
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
        encodedtext = (
            b"begin 644 file\n"
            # length 1; bits 001100 111111 111111 111111
            b"\x21\x2C\x5F\x5F\x5F\n"
            b"\x20\n"
            b"end\n"
        )
        plaintext = b"\x33"  # 00110011

        with self.subTest("uu.decode()"):
            inp = io.BytesIO(encodedtext)
            out = io.BytesIO()
            uu.decode(inp, out, quiet=True)
            self.assertEqual(out.getvalue(), plaintext)

        with self.subTest("uu_codec"):
            import codecs
            decoded = codecs.decode(encodedtext, "uu_codec")
            self.assertEqual(decoded, plaintext)

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

    def _kill(self, f):
        # close and remove file
        if f is None:
            return
        try:
            f.close()
        except (SystemExit, KeyboardInterrupt):
            raise
        except:
            pass
        try:
            os.unlink(f.name)
        except (SystemExit, KeyboardInterrupt):
            raise
        except:
            pass

    def setUp(self):
        self.tmpin  = support.TESTFN + "i"
        self.tmpout = support.TESTFN + "o"

    def tearDown(self):
        del self.tmpin
        del self.tmpout

    def test_encode(self):
        fin = fout = None
        try:
            support.unlink(self.tmpin)
            fin = open(self.tmpin, 'wb')
            fin.write(plaintext)
            fin.close()

            fin = open(self.tmpin, 'rb')
            fout = open(self.tmpout, 'wb')
            uu.encode(fin, fout, self.tmpin, mode=0o644)
            fin.close()
            fout.close()

            fout = open(self.tmpout, 'rb')
            s = fout.read()
            fout.close()
            self.assertEqual(s, encodedtextwrapped(0o644, self.tmpin))

            # in_file and out_file as filenames
            uu.encode(self.tmpin, self.tmpout, self.tmpin, mode=0o644)
            fout = open(self.tmpout, 'rb')
            s = fout.read()
            fout.close()
            self.assertEqual(s, encodedtextwrapped(0o644, self.tmpin))

        finally:
            self._kill(fin)
            self._kill(fout)

    def test_decode(self):
        f = None
        try:
            support.unlink(self.tmpin)
            f = open(self.tmpin, 'wb')
            f.write(encodedtextwrapped(0o644, self.tmpout))
            f.close()

            f = open(self.tmpin, 'rb')
            uu.decode(f)
            f.close()

            f = open(self.tmpout, 'rb')
            s = f.read()
            f.close()
            self.assertEqual(s, plaintext)
            # XXX is there an xp way to verify the mode?
        finally:
            self._kill(f)

    def test_decode_filename(self):
        f = None
        try:
            support.unlink(self.tmpin)
            f = open(self.tmpin, 'wb')
            f.write(encodedtextwrapped(0o644, self.tmpout))
            f.close()

            uu.decode(self.tmpin)

            f = open(self.tmpout, 'rb')
            s = f.read()
            f.close()
            self.assertEqual(s, plaintext)
        finally:
            self._kill(f)

    def test_decodetwice(self):
        # Verify that decode() will refuse to overwrite an existing file
        f = None
        try:
            f = io.BytesIO(encodedtextwrapped(0o644, self.tmpout))

            f = open(self.tmpin, 'rb')
            uu.decode(f)
            f.close()

            f = open(self.tmpin, 'rb')
            self.assertRaises(uu.Error, uu.decode, f)
            f.close()
        finally:
            self._kill(f)

def test_main():
    support.run_unittest(UUTest,
                              UUStdIOTest,
                              UUFileTest,
                              )

if __name__=="__main__":
    test_main()
