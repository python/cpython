"""
Tests for uu module.
Nick Mathewson
"""

import unittest
from test import test_support

import sys, os
import uu
from io import BytesIO
import io

plaintext = b"The smooth-scaled python crept over the sleeping dog\n"

encodedtext = b"""\
M5&AE('-M;V]T:\"US8V%L960@<'ET:&]N(&-R97!T(&]V97(@=&AE('-L965P
(:6YG(&1O9PH """

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
            self.fail("No exception thrown")
        except uu.Error as e:
            self.assertEqual(str(e), "Truncated input file")

    def test_missingbegin(self):
        inp = io.BytesIO(b"")
        out = io.BytesIO()
        try:
            uu.decode(inp, out)
            self.fail("No exception thrown")
        except uu.Error as e:
            self.assertEqual(str(e), "No valid begin line found in input file")

class UUStdIOTest(unittest.TestCase):

    def setUp(self):
        self.stdin = sys.stdin
        self.stdout = sys.stdout

    def tearDown(self):
        sys.stdin = self.stdin
        sys.stdout = self.stdout

    def test_encode(self):
        sys.stdin = io.StringIO(plaintext.decode("ascii"))
        sys.stdout = io.StringIO()
        uu.encode("-", "-", "t1", 0o666)
        self.assertEqual(sys.stdout.getvalue(),
                         encodedtextwrapped(0o666, "t1").decode("ascii"))

    def test_decode(self):
        sys.stdin = io.StringIO(encodedtextwrapped(0o666, "t1").decode("ascii"))
        sys.stdout = io.StringIO()
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
        self.tmpin  = test_support.TESTFN + "i"
        self.tmpout = test_support.TESTFN + "o"

    def tearDown(self):
        del self.tmpin
        del self.tmpout

    def test_encode(self):
        fin = fout = None
        try:
            test_support.unlink(self.tmpin)
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
            test_support.unlink(self.tmpin)
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
    test_support.run_unittest(UUTest,
                              UUStdIOTest,
                              UUFileTest,
                              )

if __name__=="__main__":
    test_main()
