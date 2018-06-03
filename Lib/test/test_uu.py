"""
Tests for uu module.
Nick Mathewson
"""

import unittest
from test import test_support as support

import cStringIO
import sys
import uu

plaintext = "The smooth-scaled python crept over the sleeping dog\n"

encodedtext = """\
M5&AE('-M;V]T:\"US8V%L960@<'ET:&]N(&-R97!T(&]V97(@=&AE('-L965P
(:6YG(&1O9PH """

encodedtextwrapped = "begin %03o %s\n" + encodedtext.replace("%", "%%") + "\n \nend\n"

class UUTest(unittest.TestCase):

    def test_encode(self):
        inp = cStringIO.StringIO(plaintext)
        out = cStringIO.StringIO()
        uu.encode(inp, out, "t1")
        self.assertEqual(out.getvalue(), encodedtextwrapped % (0666, "t1"))
        inp = cStringIO.StringIO(plaintext)
        out = cStringIO.StringIO()
        uu.encode(inp, out, "t1", 0644)
        self.assertEqual(out.getvalue(), encodedtextwrapped % (0644, "t1"))

    def test_decode(self):
        inp = cStringIO.StringIO(encodedtextwrapped % (0666, "t1"))
        out = cStringIO.StringIO()
        uu.decode(inp, out)
        self.assertEqual(out.getvalue(), plaintext)
        inp = cStringIO.StringIO(
            "UUencoded files may contain many lines,\n" +
            "even some that have 'begin' in them.\n" +
            encodedtextwrapped % (0666, "t1")
        )
        out = cStringIO.StringIO()
        uu.decode(inp, out)
        self.assertEqual(out.getvalue(), plaintext)

    def test_truncatedinput(self):
        inp = cStringIO.StringIO("begin 644 t1\n" + encodedtext)
        out = cStringIO.StringIO()
        try:
            uu.decode(inp, out)
            self.fail("No exception raised")
        except uu.Error, e:
            self.assertEqual(str(e), "Truncated input file")

    def test_missingbegin(self):
        inp = cStringIO.StringIO("")
        out = cStringIO.StringIO()
        try:
            uu.decode(inp, out)
            self.fail("No exception raised")
        except uu.Error, e:
            self.assertEqual(str(e), "No valid begin line found in input file")

    def test_garbage_padding(self):
        # Issue #22406
        encodedtext = (
            "begin 644 file\n"
            # length 1; bits 001100 111111 111111 111111
            "\x21\x2C\x5F\x5F\x5F\n"
            "\x20\n"
            "end\n"
        )
        plaintext = "\x33"  # 00110011

        inp = cStringIO.StringIO(encodedtext)
        out = cStringIO.StringIO()
        uu.decode(inp, out, quiet=True)
        self.assertEqual(out.getvalue(), plaintext)

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
        sys.stdin = cStringIO.StringIO(plaintext)
        sys.stdout = cStringIO.StringIO()
        uu.encode("-", "-", "t1", 0666)
        self.assertEqual(
            sys.stdout.getvalue(),
            encodedtextwrapped % (0666, "t1")
        )

    def test_decode(self):
        sys.stdin = cStringIO.StringIO(encodedtextwrapped % (0666, "t1"))
        sys.stdout = cStringIO.StringIO()
        uu.decode("-", "-")
        self.assertEqual(sys.stdout.getvalue(), plaintext)

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
            with open(self.tmpout, 'w') as fout:
                uu.encode(fin, fout, self.tmpin, mode=0o644)

        with open(self.tmpout, 'r') as fout:
            s = fout.read()
        self.assertEqual(s, encodedtextwrapped % (0o644, self.tmpin))

        # in_file and out_file as filenames
        uu.encode(self.tmpin, self.tmpout, self.tmpin, mode=0o644)
        with open(self.tmpout, 'r') as fout:
            s = fout.read()
        self.assertEqual(s, encodedtextwrapped % (0o644, self.tmpin))

    def test_decode(self):
        with open(self.tmpin, 'w') as f:
            f.write(encodedtextwrapped % (0o644, self.tmpout))

        with open(self.tmpin, 'r') as f:
            uu.decode(f)

        with open(self.tmpout, 'r') as f:
            s = f.read()
        self.assertEqual(s, plaintext)
        # XXX is there an xp way to verify the mode?

    def test_decode_filename(self):
        with open(self.tmpin, 'w') as f:
            f.write(encodedtextwrapped % (0o644, self.tmpout))

        uu.decode(self.tmpin)

        with open(self.tmpout, 'r') as f:
            s = f.read()
        self.assertEqual(s, plaintext)

    def test_decodetwice(self):
        # Verify that decode() will refuse to overwrite an existing file
        with open(self.tmpin, 'wb') as f:
            f.write(encodedtextwrapped % (0o644, self.tmpout))
        with open(self.tmpin, 'r') as f:
            uu.decode(f)

        with open(self.tmpin, 'r') as f:
            self.assertRaises(uu.Error, uu.decode, f)

def test_main():
    support.run_unittest(UUTest, UUStdIOTest, UUFileTest)

if __name__=="__main__":
    test_main()
