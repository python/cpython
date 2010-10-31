"""
Tests for uu module.
Nick Mathewson
"""

import unittest
from test import test_support

import sys, os, uu, cStringIO
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
            self.fail("No exception thrown")
        except uu.Error, e:
            self.assertEqual(str(e), "Truncated input file")

    def test_missingbegin(self):
        inp = cStringIO.StringIO("")
        out = cStringIO.StringIO()
        try:
            uu.decode(inp, out)
            self.fail("No exception thrown")
        except uu.Error, e:
            self.assertEqual(str(e), "No valid begin line found in input file")

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

    def _kill(self, f):
        # close and remove file
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
            fout = open(self.tmpout, 'w')
            uu.encode(fin, fout, self.tmpin, mode=0644)
            fin.close()
            fout.close()

            fout = open(self.tmpout, 'r')
            s = fout.read()
            fout.close()
            self.assertEqual(s, encodedtextwrapped % (0644, self.tmpin))

            # in_file and out_file as filenames
            uu.encode(self.tmpin, self.tmpout, self.tmpin, mode=0644)
            fout = open(self.tmpout, 'r')
            s = fout.read()
            fout.close()
            self.assertEqual(s, encodedtextwrapped % (0644, self.tmpin))

        finally:
            self._kill(fin)
            self._kill(fout)

    def test_decode(self):
        f = None
        try:
            test_support.unlink(self.tmpin)
            f = open(self.tmpin, 'w')
            f.write(encodedtextwrapped % (0644, self.tmpout))
            f.close()

            f = open(self.tmpin, 'r')
            uu.decode(f)
            f.close()

            f = open(self.tmpout, 'r')
            s = f.read()
            f.close()
            self.assertEqual(s, plaintext)
            # XXX is there an xp way to verify the mode?
        finally:
            self._kill(f)

    def test_decode_filename(self):
        f = None
        try:
            test_support.unlink(self.tmpin)
            f = open(self.tmpin, 'w')
            f.write(encodedtextwrapped % (0644, self.tmpout))
            f.close()

            uu.decode(self.tmpin)

            f = open(self.tmpout, 'r')
            s = f.read()
            f.close()
            self.assertEqual(s, plaintext)
        finally:
            self._kill(f)

    def test_decodetwice(self):
        # Verify that decode() will refuse to overwrite an existing file
        f = None
        try:
            f = cStringIO.StringIO(encodedtextwrapped % (0644, self.tmpout))

            f = open(self.tmpin, 'r')
            uu.decode(f)
            f.close()

            f = open(self.tmpin, 'r')
            self.assertRaises(uu.Error, uu.decode, f)
            f.close()
        finally:
            self._kill(f)

def test_main():
    test_support.run_unittest(UUTest, UUStdIOTest, UUFileTest)

if __name__=="__main__":
    test_main()
