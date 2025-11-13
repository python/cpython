import array
import codecs
import locale
import os
import pickle
import sys
import threading
import time
import unittest
import warnings
import weakref
from collections import UserList
from test import support
from test.support import os_helper, threading_helper
from test.support.script_helper import assert_python_ok
from .utils import CTestCase, PyTestCase

import io  # C implementation of io
import _pyio as pyio # Python implementation of io


def _default_chunk_size():
    """Get the default TextIOWrapper chunk size"""
    with open(__file__, "r", encoding="latin-1") as f:
        return f._CHUNK_SIZE


class BadIndex:
    def __index__(self):
        1/0


# To fully exercise seek/tell, the StatefulIncrementalDecoder has these
# properties:
#   - A single output character can correspond to many bytes of input.
#   - The number of input bytes to complete the character can be
#     undetermined until the last input byte is received.
#   - The number of input bytes can vary depending on previous input.
#   - A single input byte can correspond to many characters of output.
#   - The number of output characters can be undetermined until the
#     last input byte is received.
#   - The number of output characters can vary depending on previous input.

class StatefulIncrementalDecoder(codecs.IncrementalDecoder):
    """
    For testing seek/tell behavior with a stateful, buffering decoder.

    Input is a sequence of words.  Words may be fixed-length (length set
    by input) or variable-length (period-terminated).  In variable-length
    mode, extra periods are ignored.  Possible words are:
      - 'i' followed by a number sets the input length, I (maximum 99).
        When I is set to 0, words are space-terminated.
      - 'o' followed by a number sets the output length, O (maximum 99).
      - Any other word is converted into a word followed by a period on
        the output.  The output word consists of the input word truncated
        or padded out with hyphens to make its length equal to O.  If O
        is 0, the word is output verbatim without truncating or padding.
    I and O are initially set to 1.  When I changes, any buffered input is
    re-scanned according to the new I.  EOF also terminates the last word.
    """

    def __init__(self, errors='strict'):
        codecs.IncrementalDecoder.__init__(self, errors)
        self.reset()

    def __repr__(self):
        return '<SID %x>' % id(self)

    def reset(self):
        self.i = 1
        self.o = 1
        self.buffer = bytearray()

    def getstate(self):
        i, o = self.i ^ 1, self.o ^ 1 # so that flags = 0 after reset()
        return bytes(self.buffer), i*100 + o

    def setstate(self, state):
        buffer, io = state
        self.buffer = bytearray(buffer)
        i, o = divmod(io, 100)
        self.i, self.o = i ^ 1, o ^ 1

    def decode(self, input, final=False):
        output = ''
        for b in input:
            if self.i == 0: # variable-length, terminated with period
                if b == ord('.'):
                    if self.buffer:
                        output += self.process_word()
                else:
                    self.buffer.append(b)
            else: # fixed-length, terminate after self.i bytes
                self.buffer.append(b)
                if len(self.buffer) == self.i:
                    output += self.process_word()
        if final and self.buffer: # EOF terminates the last word
            output += self.process_word()
        return output

    def process_word(self):
        output = ''
        if self.buffer[0] == ord('i'):
            self.i = min(99, int(self.buffer[1:] or 0)) # set input length
        elif self.buffer[0] == ord('o'):
            self.o = min(99, int(self.buffer[1:] or 0)) # set output length
        else:
            output = self.buffer.decode('ascii')
            if len(output) < self.o:
                output += '-'*self.o # pad out with hyphens
            if self.o:
                output = output[:self.o] # truncate to output length
            output += '.'
        self.buffer = bytearray()
        return output

    codecEnabled = False


# bpo-41919: This method is separated from StatefulIncrementalDecoder to avoid a resource leak
# when registering codecs and cleanup functions.
def lookupTestDecoder(name):
    if StatefulIncrementalDecoder.codecEnabled and name == 'test_decoder':
        latin1 = codecs.lookup('latin-1')
        return codecs.CodecInfo(
            name='test_decoder', encode=latin1.encode, decode=None,
            incrementalencoder=None,
            streamreader=None, streamwriter=None,
            incrementaldecoder=StatefulIncrementalDecoder)


class StatefulIncrementalDecoderTest(unittest.TestCase):
    """
    Make sure the StatefulIncrementalDecoder actually works.
    """

    test_cases = [
        # I=1, O=1 (fixed-length input == fixed-length output)
        (b'abcd', False, 'a.b.c.d.'),
        # I=0, O=0 (variable-length input, variable-length output)
        (b'oiabcd', True, 'abcd.'),
        # I=0, O=0 (should ignore extra periods)
        (b'oi...abcd...', True, 'abcd.'),
        # I=0, O=6 (variable-length input, fixed-length output)
        (b'i.o6.x.xyz.toolongtofit.', False, 'x-----.xyz---.toolon.'),
        # I=2, O=6 (fixed-length input < fixed-length output)
        (b'i.i2.o6xyz', True, 'xy----.z-----.'),
        # I=6, O=3 (fixed-length input > fixed-length output)
        (b'i.o3.i6.abcdefghijklmnop', True, 'abc.ghi.mno.'),
        # I=0, then 3; O=29, then 15 (with longer output)
        (b'i.o29.a.b.cde.o15.abcdefghijabcdefghij.i3.a.b.c.d.ei00k.l.m', True,
         'a----------------------------.' +
         'b----------------------------.' +
         'cde--------------------------.' +
         'abcdefghijabcde.' +
         'a.b------------.' +
         '.c.------------.' +
         'd.e------------.' +
         'k--------------.' +
         'l--------------.' +
         'm--------------.')
    ]

    def test_decoder(self):
        # Try a few one-shot test cases.
        for input, eof, output in self.test_cases:
            d = StatefulIncrementalDecoder()
            self.assertEqual(d.decode(input, eof), output)

        # Also test an unfinished decode, followed by forcing EOF.
        d = StatefulIncrementalDecoder()
        self.assertEqual(d.decode(b'oiabcd'), '')
        self.assertEqual(d.decode(b'', 1), 'abcd.')

class TextIOWrapperTest:

    def setUp(self):
        self.testdata = b"AAA\r\nBBB\rCCC\r\nDDD\nEEE\r\n"
        self.normalized = b"AAA\nBBB\nCCC\nDDD\nEEE\n".decode("ascii")
        os_helper.unlink(os_helper.TESTFN)
        codecs.register(lookupTestDecoder)
        self.addCleanup(codecs.unregister, lookupTestDecoder)

    def tearDown(self):
        os_helper.unlink(os_helper.TESTFN)

    def test_constructor(self):
        r = self.BytesIO(b"\xc3\xa9\n\n")
        b = self.BufferedReader(r, 1000)
        t = self.TextIOWrapper(b, encoding="utf-8")
        t.__init__(b, encoding="latin-1", newline="\r\n")
        self.assertEqual(t.encoding, "latin-1")
        self.assertEqual(t.line_buffering, False)
        t.__init__(b, encoding="utf-8", line_buffering=True)
        self.assertEqual(t.encoding, "utf-8")
        self.assertEqual(t.line_buffering, True)
        self.assertEqual("\xe9\n", t.readline())
        invalid_type = TypeError if self.is_C else ValueError
        with self.assertRaises(invalid_type):
            t.__init__(b, encoding=42)
        with self.assertRaises(UnicodeEncodeError):
            t.__init__(b, encoding='\udcfe')
        with self.assertRaises(ValueError):
            t.__init__(b, encoding='utf-8\0')
        with self.assertRaises(invalid_type):
            t.__init__(b, encoding="utf-8", errors=42)
        if support.Py_DEBUG or sys.flags.dev_mode or self.is_C:
            with self.assertRaises(UnicodeEncodeError):
                t.__init__(b, encoding="utf-8", errors='\udcfe')
        if support.Py_DEBUG or sys.flags.dev_mode or self.is_C:
            with self.assertRaises(ValueError):
                t.__init__(b, encoding="utf-8", errors='replace\0')
        with self.assertRaises(TypeError):
            t.__init__(b, encoding="utf-8", newline=42)
        with self.assertRaises(ValueError):
            t.__init__(b, encoding="utf-8", newline='\udcfe')
        with self.assertRaises(ValueError):
            t.__init__(b, encoding="utf-8", newline='\n\0')
        with self.assertRaises(ValueError):
            t.__init__(b, encoding="utf-8", newline='xyzzy')

    def test_uninitialized(self):
        t = self.TextIOWrapper.__new__(self.TextIOWrapper)
        del t
        t = self.TextIOWrapper.__new__(self.TextIOWrapper)
        self.assertRaises(Exception, repr, t)
        self.assertRaisesRegex((ValueError, AttributeError),
                               'uninitialized|has no attribute',
                               t.read, 0)
        t.__init__(self.MockRawIO(), encoding="utf-8")
        self.assertEqual(t.read(0), '')

    def test_non_text_encoding_codecs_are_rejected(self):
        # Ensure the constructor complains if passed a codec that isn't
        # marked as a text encoding
        # http://bugs.python.org/issue20404
        r = self.BytesIO()
        b = self.BufferedWriter(r)
        with self.assertRaisesRegex(LookupError, "is not a text encoding"):
            self.TextIOWrapper(b, encoding="hex")

    def test_detach(self):
        r = self.BytesIO()
        b = self.BufferedWriter(r)
        t = self.TextIOWrapper(b, encoding="ascii")
        self.assertIs(t.detach(), b)

        t = self.TextIOWrapper(b, encoding="ascii")
        t.write("howdy")
        self.assertFalse(r.getvalue())
        t.detach()
        self.assertEqual(r.getvalue(), b"howdy")
        self.assertRaises(ValueError, t.detach)

        # Operations independent of the detached stream should still work
        repr(t)
        self.assertEqual(t.encoding, "ascii")
        self.assertEqual(t.errors, "strict")
        self.assertFalse(t.line_buffering)
        self.assertFalse(t.write_through)

    def test_repr(self):
        raw = self.BytesIO("hello".encode("utf-8"))
        b = self.BufferedReader(raw)
        t = self.TextIOWrapper(b, encoding="utf-8")
        modname = self.TextIOWrapper.__module__
        self.assertRegex(repr(t),
                         r"<(%s\.)?TextIOWrapper encoding='utf-8'>" % modname)
        raw.name = "dummy"
        self.assertRegex(repr(t),
                         r"<(%s\.)?TextIOWrapper name='dummy' encoding='utf-8'>" % modname)
        t.mode = "r"
        self.assertRegex(repr(t),
                         r"<(%s\.)?TextIOWrapper name='dummy' mode='r' encoding='utf-8'>" % modname)
        raw.name = b"dummy"
        self.assertRegex(repr(t),
                         r"<(%s\.)?TextIOWrapper name=b'dummy' mode='r' encoding='utf-8'>" % modname)

        t.buffer.detach()
        repr(t)  # Should not raise an exception

    def test_recursive_repr(self):
        # Issue #25455
        raw = self.BytesIO()
        t = self.TextIOWrapper(raw, encoding="utf-8")
        with support.swap_attr(raw, 'name', t), support.infinite_recursion(25):
            with self.assertRaises(RuntimeError):
                repr(t)  # Should not crash

    def test_subclass_repr(self):
        class TestSubclass(self.TextIOWrapper):
            pass

        f = TestSubclass(self.StringIO())
        self.assertIn(TestSubclass.__name__, repr(f))

    def test_line_buffering(self):
        r = self.BytesIO()
        b = self.BufferedWriter(r, 1000)
        t = self.TextIOWrapper(b, encoding="utf-8", newline="\n", line_buffering=True)
        t.write("X")
        self.assertEqual(r.getvalue(), b"")  # No flush happened
        t.write("Y\nZ")
        self.assertEqual(r.getvalue(), b"XY\nZ")  # All got flushed
        t.write("A\rB")
        self.assertEqual(r.getvalue(), b"XY\nZA\rB")

    def test_reconfigure_line_buffering(self):
        r = self.BytesIO()
        b = self.BufferedWriter(r, 1000)
        t = self.TextIOWrapper(b, encoding="utf-8", newline="\n", line_buffering=False)
        t.write("AB\nC")
        self.assertEqual(r.getvalue(), b"")

        t.reconfigure(line_buffering=True)   # implicit flush
        self.assertEqual(r.getvalue(), b"AB\nC")
        t.write("DEF\nG")
        self.assertEqual(r.getvalue(), b"AB\nCDEF\nG")
        t.write("H")
        self.assertEqual(r.getvalue(), b"AB\nCDEF\nG")
        t.reconfigure(line_buffering=False)   # implicit flush
        self.assertEqual(r.getvalue(), b"AB\nCDEF\nGH")
        t.write("IJ")
        self.assertEqual(r.getvalue(), b"AB\nCDEF\nGH")

        # Keeping default value
        t.reconfigure()
        t.reconfigure(line_buffering=None)
        self.assertEqual(t.line_buffering, False)
        t.reconfigure(line_buffering=True)
        t.reconfigure()
        t.reconfigure(line_buffering=None)
        self.assertEqual(t.line_buffering, True)

    @unittest.skipIf(sys.flags.utf8_mode, "utf-8 mode is enabled")
    def test_default_encoding(self):
        with os_helper.EnvironmentVarGuard() as env:
            # try to get a user preferred encoding different than the current
            # locale encoding to check that TextIOWrapper() uses the current
            # locale encoding and not the user preferred encoding
            env.unset('LC_ALL', 'LANG', 'LC_CTYPE')

            current_locale_encoding = locale.getencoding()
            b = self.BytesIO()
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", EncodingWarning)
                t = self.TextIOWrapper(b)
            self.assertEqual(t.encoding, current_locale_encoding)

    def test_encoding(self):
        # Check the encoding attribute is always set, and valid
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="utf-8")
        self.assertEqual(t.encoding, "utf-8")
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", EncodingWarning)
            t = self.TextIOWrapper(b)
        self.assertIsNotNone(t.encoding)
        codecs.lookup(t.encoding)

    def test_encoding_errors_reading(self):
        # (1) default
        b = self.BytesIO(b"abc\n\xff\n")
        t = self.TextIOWrapper(b, encoding="ascii")
        self.assertRaises(UnicodeError, t.read)
        # (2) explicit strict
        b = self.BytesIO(b"abc\n\xff\n")
        t = self.TextIOWrapper(b, encoding="ascii", errors="strict")
        self.assertRaises(UnicodeError, t.read)
        # (3) ignore
        b = self.BytesIO(b"abc\n\xff\n")
        t = self.TextIOWrapper(b, encoding="ascii", errors="ignore")
        self.assertEqual(t.read(), "abc\n\n")
        # (4) replace
        b = self.BytesIO(b"abc\n\xff\n")
        t = self.TextIOWrapper(b, encoding="ascii", errors="replace")
        self.assertEqual(t.read(), "abc\n\ufffd\n")

    def test_encoding_errors_writing(self):
        # (1) default
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="ascii")
        self.assertRaises(UnicodeError, t.write, "\xff")
        # (2) explicit strict
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="ascii", errors="strict")
        self.assertRaises(UnicodeError, t.write, "\xff")
        # (3) ignore
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="ascii", errors="ignore",
                             newline="\n")
        t.write("abc\xffdef\n")
        t.flush()
        self.assertEqual(b.getvalue(), b"abcdef\n")
        # (4) replace
        b = self.BytesIO()
        t = self.TextIOWrapper(b, encoding="ascii", errors="replace",
                             newline="\n")
        t.write("abc\xffdef\n")
        t.flush()
        self.assertEqual(b.getvalue(), b"abc?def\n")

    def test_newlines(self):
        input_lines = [ "unix\n", "windows\r\n", "os9\r", "last\n", "nonl" ]

        tests = [
            [ None, [ 'unix\n', 'windows\n', 'os9\n', 'last\n', 'nonl' ] ],
            [ '', input_lines ],
            [ '\n', [ "unix\n", "windows\r\n", "os9\rlast\n", "nonl" ] ],
            [ '\r\n', [ "unix\nwindows\r\n", "os9\rlast\nnonl" ] ],
            [ '\r', [ "unix\nwindows\r", "\nos9\r", "last\nnonl" ] ],
        ]
        encodings = (
            'utf-8', 'latin-1',
            'utf-16', 'utf-16-le', 'utf-16-be',
            'utf-32', 'utf-32-le', 'utf-32-be',
        )

        # Try a range of buffer sizes to test the case where \r is the last
        # character in TextIOWrapper._pending_line.
        for encoding in encodings:
            # XXX: str.encode() should return bytes
            data = bytes(''.join(input_lines).encode(encoding))
            for do_reads in (False, True):
                for bufsize in range(1, 10):
                    for newline, exp_lines in tests:
                        bufio = self.BufferedReader(self.BytesIO(data), bufsize)
                        textio = self.TextIOWrapper(bufio, newline=newline,
                                                  encoding=encoding)
                        if do_reads:
                            got_lines = []
                            while True:
                                c2 = textio.read(2)
                                if c2 == '':
                                    break
                                self.assertEqual(len(c2), 2)
                                got_lines.append(c2 + textio.readline())
                        else:
                            got_lines = list(textio)

                        for got_line, exp_line in zip(got_lines, exp_lines):
                            self.assertEqual(got_line, exp_line)
                        self.assertEqual(len(got_lines), len(exp_lines))

    def test_newlines_input(self):
        testdata = b"AAA\nBB\x00B\nCCC\rDDD\rEEE\r\nFFF\r\nGGG"
        normalized = testdata.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
        for newline, expected in [
            (None, normalized.decode("ascii").splitlines(keepends=True)),
            ("", testdata.decode("ascii").splitlines(keepends=True)),
            ("\n", ["AAA\n", "BB\x00B\n", "CCC\rDDD\rEEE\r\n", "FFF\r\n", "GGG"]),
            ("\r\n", ["AAA\nBB\x00B\nCCC\rDDD\rEEE\r\n", "FFF\r\n", "GGG"]),
            ("\r",  ["AAA\nBB\x00B\nCCC\r", "DDD\r", "EEE\r", "\nFFF\r", "\nGGG"]),
            ]:
            buf = self.BytesIO(testdata)
            txt = self.TextIOWrapper(buf, encoding="ascii", newline=newline)
            self.assertEqual(txt.readlines(), expected)
            txt.seek(0)
            self.assertEqual(txt.read(), "".join(expected))

    def test_newlines_output(self):
        testdict = {
            "": b"AAA\nBBB\nCCC\nX\rY\r\nZ",
            "\n": b"AAA\nBBB\nCCC\nX\rY\r\nZ",
            "\r": b"AAA\rBBB\rCCC\rX\rY\r\rZ",
            "\r\n": b"AAA\r\nBBB\r\nCCC\r\nX\rY\r\r\nZ",
            }
        tests = [(None, testdict[os.linesep])] + sorted(testdict.items())
        for newline, expected in tests:
            buf = self.BytesIO()
            txt = self.TextIOWrapper(buf, encoding="ascii", newline=newline)
            txt.write("AAA\nB")
            txt.write("BB\nCCC\n")
            txt.write("X\rY\r\nZ")
            txt.flush()
            self.assertEqual(buf.closed, False)
            self.assertEqual(buf.getvalue(), expected)

    def test_destructor(self):
        l = []
        base = self.BytesIO
        class MyBytesIO(base):
            def close(self):
                l.append(self.getvalue())
                base.close(self)
        b = MyBytesIO()
        t = self.TextIOWrapper(b, encoding="ascii")
        t.write("abc")
        del t
        support.gc_collect()
        self.assertEqual([b"abc"], l)

    def test_override_destructor(self):
        record = []
        class MyTextIO(self.TextIOWrapper):
            def __del__(self):
                record.append(1)
                try:
                    f = super().__del__
                except AttributeError:
                    pass
                else:
                    f()
            def close(self):
                record.append(2)
                super().close()
            def flush(self):
                record.append(3)
                super().flush()
        b = self.BytesIO()
        t = MyTextIO(b, encoding="ascii")
        del t
        support.gc_collect()
        self.assertEqual(record, [1, 2, 3])

    def test_error_through_destructor(self):
        # Test that the exception state is not modified by a destructor,
        # even if close() fails.
        rawio = self.CloseFailureIO()
        with support.catch_unraisable_exception() as cm:
            with self.assertRaises(AttributeError):
                self.TextIOWrapper(rawio, encoding="utf-8").xyzzy

            self.assertEqual(cm.unraisable.exc_type, OSError)

    # Systematic tests of the text I/O API

    def test_basic_io(self):
        for chunksize in (1, 2, 3, 4, 5, 15, 16, 17, 31, 32, 33, 63, 64, 65):
            for enc in "ascii", "latin-1", "utf-8" :# , "utf-16-be", "utf-16-le":
                f = self.open(os_helper.TESTFN, "w+", encoding=enc)
                f._CHUNK_SIZE = chunksize
                self.assertEqual(f.write("abc"), 3)
                f.close()
                f = self.open(os_helper.TESTFN, "r+", encoding=enc)
                f._CHUNK_SIZE = chunksize
                self.assertEqual(f.tell(), 0)
                self.assertEqual(f.read(), "abc")
                cookie = f.tell()
                self.assertEqual(f.seek(0), 0)
                self.assertEqual(f.read(None), "abc")
                f.seek(0)
                self.assertEqual(f.read(2), "ab")
                self.assertEqual(f.read(1), "c")
                self.assertEqual(f.read(1), "")
                self.assertEqual(f.read(), "")
                self.assertEqual(f.tell(), cookie)
                self.assertEqual(f.seek(0), 0)
                self.assertEqual(f.seek(0, 2), cookie)
                self.assertEqual(f.write("def"), 3)
                self.assertEqual(f.seek(cookie), cookie)
                self.assertEqual(f.read(), "def")
                if enc.startswith("utf"):
                    self.multi_line_test(f, enc)
                f.close()

    def multi_line_test(self, f, enc):
        f.seek(0)
        f.truncate()
        sample = "s\xff\u0fff\uffff"
        wlines = []
        for size in (0, 1, 2, 3, 4, 5, 30, 31, 32, 33, 62, 63, 64, 65, 1000):
            chars = []
            for i in range(size):
                chars.append(sample[i % len(sample)])
            line = "".join(chars) + "\n"
            wlines.append((f.tell(), line))
            f.write(line)
        f.seek(0)
        rlines = []
        while True:
            pos = f.tell()
            line = f.readline()
            if not line:
                break
            rlines.append((pos, line))
        self.assertEqual(rlines, wlines)

    def test_telling(self):
        f = self.open(os_helper.TESTFN, "w+", encoding="utf-8")
        p0 = f.tell()
        f.write("\xff\n")
        p1 = f.tell()
        f.write("\xff\n")
        p2 = f.tell()
        f.seek(0)
        self.assertEqual(f.tell(), p0)
        self.assertEqual(f.readline(), "\xff\n")
        self.assertEqual(f.tell(), p1)
        self.assertEqual(f.readline(), "\xff\n")
        self.assertEqual(f.tell(), p2)
        f.seek(0)
        for line in f:
            self.assertEqual(line, "\xff\n")
            self.assertRaises(OSError, f.tell)
        self.assertEqual(f.tell(), p2)
        f.close()

    def test_seeking(self):
        chunk_size = _default_chunk_size()
        prefix_size = chunk_size - 2
        u_prefix = "a" * prefix_size
        prefix = bytes(u_prefix.encode("utf-8"))
        self.assertEqual(len(u_prefix), len(prefix))
        u_suffix = "\u8888\n"
        suffix = bytes(u_suffix.encode("utf-8"))
        line = prefix + suffix
        with self.open(os_helper.TESTFN, "wb") as f:
            f.write(line*2)
        with self.open(os_helper.TESTFN, "r", encoding="utf-8") as f:
            s = f.read(prefix_size)
            self.assertEqual(s, str(prefix, "ascii"))
            self.assertEqual(f.tell(), prefix_size)
            self.assertEqual(f.readline(), u_suffix)

    def test_seeking_too(self):
        # Regression test for a specific bug
        data = b'\xe0\xbf\xbf\n'
        with self.open(os_helper.TESTFN, "wb") as f:
            f.write(data)
        with self.open(os_helper.TESTFN, "r", encoding="utf-8") as f:
            f._CHUNK_SIZE  # Just test that it exists
            f._CHUNK_SIZE = 2
            f.readline()
            f.tell()

    def test_seek_and_tell(self):
        #Test seek/tell using the StatefulIncrementalDecoder.
        # Make test faster by doing smaller seeks
        CHUNK_SIZE = 128

        def test_seek_and_tell_with_data(data, min_pos=0):
            """Tell/seek to various points within a data stream and ensure
            that the decoded data returned by read() is consistent."""
            f = self.open(os_helper.TESTFN, 'wb')
            f.write(data)
            f.close()
            f = self.open(os_helper.TESTFN, encoding='test_decoder')
            f._CHUNK_SIZE = CHUNK_SIZE
            decoded = f.read()
            f.close()

            for i in range(min_pos, len(decoded) + 1): # seek positions
                for j in [1, 5, len(decoded) - i]: # read lengths
                    f = self.open(os_helper.TESTFN, encoding='test_decoder')
                    self.assertEqual(f.read(i), decoded[:i])
                    cookie = f.tell()
                    self.assertEqual(f.read(j), decoded[i:i + j])
                    f.seek(cookie)
                    self.assertEqual(f.read(), decoded[i:])
                    f.close()

        # Enable the test decoder.
        StatefulIncrementalDecoder.codecEnabled = 1

        # Run the tests.
        try:
            # Try each test case.
            for input, _, _ in StatefulIncrementalDecoderTest.test_cases:
                test_seek_and_tell_with_data(input)

            # Position each test case so that it crosses a chunk boundary.
            for input, _, _ in StatefulIncrementalDecoderTest.test_cases:
                offset = CHUNK_SIZE - len(input)//2
                prefix = b'.'*offset
                # Don't bother seeking into the prefix (takes too long).
                min_pos = offset*2
                test_seek_and_tell_with_data(prefix + input, min_pos)

        # Ensure our test decoder won't interfere with subsequent tests.
        finally:
            StatefulIncrementalDecoder.codecEnabled = 0

    def test_multibyte_seek_and_tell(self):
        f = self.open(os_helper.TESTFN, "w", encoding="euc_jp")
        f.write("AB\n\u3046\u3048\n")
        f.close()

        f = self.open(os_helper.TESTFN, "r", encoding="euc_jp")
        self.assertEqual(f.readline(), "AB\n")
        p0 = f.tell()
        self.assertEqual(f.readline(), "\u3046\u3048\n")
        p1 = f.tell()
        f.seek(p0)
        self.assertEqual(f.readline(), "\u3046\u3048\n")
        self.assertEqual(f.tell(), p1)
        f.close()

    def test_tell_after_readline_with_cr(self):
        # Test for gh-141314: TextIOWrapper.tell() assertion failure
        # when dealing with standalone carriage returns
        data = b'line1\r'
        with self.open(os_helper.TESTFN, "wb") as f:
            f.write(data)

        with self.open(os_helper.TESTFN, "r") as f:
            # Read line that ends with \r
            line = f.readline()
            self.assertEqual(line, "line1\n")
            # This should not cause an assertion failure
            pos = f.tell()
            # Verify we can seek back to this position
            f.seek(pos)
            remaining = f.read()
            self.assertEqual(remaining, "")


    def test_seek_with_encoder_state(self):
        f = self.open(os_helper.TESTFN, "w", encoding="euc_jis_2004")
        f.write("\u00e6\u0300")
        p0 = f.tell()
        f.write("\u00e6")
        f.seek(p0)
        f.write("\u0300")
        f.close()

        f = self.open(os_helper.TESTFN, "r", encoding="euc_jis_2004")
        self.assertEqual(f.readline(), "\u00e6\u0300\u0300")
        f.close()

    def test_encoded_writes(self):
        data = "1234567890"
        tests = ("utf-16",
                 "utf-16-le",
                 "utf-16-be",
                 "utf-32",
                 "utf-32-le",
                 "utf-32-be")
        for encoding in tests:
            buf = self.BytesIO()
            f = self.TextIOWrapper(buf, encoding=encoding)
            # Check if the BOM is written only once (see issue1753).
            f.write(data)
            f.write(data)
            f.seek(0)
            self.assertEqual(f.read(), data * 2)
            f.seek(0)
            self.assertEqual(f.read(), data * 2)
            self.assertEqual(buf.getvalue(), (data * 2).encode(encoding))

    def test_unreadable(self):
        class UnReadable(self.BytesIO):
            def readable(self):
                return False
        txt = self.TextIOWrapper(UnReadable(), encoding="utf-8")
        self.assertRaises(OSError, txt.read)

    def test_read_one_by_one(self):
        txt = self.TextIOWrapper(self.BytesIO(b"AA\r\nBB"), encoding="utf-8")
        reads = ""
        while True:
            c = txt.read(1)
            if not c:
                break
            reads += c
        self.assertEqual(reads, "AA\nBB")

    def test_readlines(self):
        txt = self.TextIOWrapper(self.BytesIO(b"AA\nBB\nCC"), encoding="utf-8")
        self.assertEqual(txt.readlines(), ["AA\n", "BB\n", "CC"])
        txt.seek(0)
        self.assertEqual(txt.readlines(None), ["AA\n", "BB\n", "CC"])
        txt.seek(0)
        self.assertEqual(txt.readlines(5), ["AA\n", "BB\n"])

    # read in amounts equal to TextIOWrapper._CHUNK_SIZE which is 128.
    def test_read_by_chunk(self):
        # make sure "\r\n" straddles 128 char boundary.
        txt = self.TextIOWrapper(self.BytesIO(b"A" * 127 + b"\r\nB"), encoding="utf-8")
        reads = ""
        while True:
            c = txt.read(128)
            if not c:
                break
            reads += c
        self.assertEqual(reads, "A"*127+"\nB")

    def test_writelines(self):
        l = ['ab', 'cd', 'ef']
        buf = self.BytesIO()
        txt = self.TextIOWrapper(buf, encoding="utf-8")
        txt.writelines(l)
        txt.flush()
        self.assertEqual(buf.getvalue(), b'abcdef')

    def test_writelines_userlist(self):
        l = UserList(['ab', 'cd', 'ef'])
        buf = self.BytesIO()
        txt = self.TextIOWrapper(buf, encoding="utf-8")
        txt.writelines(l)
        txt.flush()
        self.assertEqual(buf.getvalue(), b'abcdef')

    def test_writelines_error(self):
        txt = self.TextIOWrapper(self.BytesIO(), encoding="utf-8")
        self.assertRaises(TypeError, txt.writelines, [1, 2, 3])
        self.assertRaises(TypeError, txt.writelines, None)
        self.assertRaises(TypeError, txt.writelines, b'abc')

    def test_issue1395_1(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")

        # read one char at a time
        reads = ""
        while True:
            c = txt.read(1)
            if not c:
                break
            reads += c
        self.assertEqual(reads, self.normalized)

    def test_issue1395_2(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = ""
        while True:
            c = txt.read(4)
            if not c:
                break
            reads += c
        self.assertEqual(reads, self.normalized)

    def test_issue1395_3(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read(4)
        reads += txt.readline()
        reads += txt.readline()
        reads += txt.readline()
        self.assertEqual(reads, self.normalized)

    def test_issue1395_4(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read()
        self.assertEqual(reads, self.normalized)

    def test_issue1395_5(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        pos = txt.tell()
        txt.seek(0)
        txt.seek(pos)
        self.assertEqual(txt.read(4), "BBB\n")

    def test_issue2282(self):
        buffer = self.BytesIO(self.testdata)
        txt = self.TextIOWrapper(buffer, encoding="ascii")

        self.assertEqual(buffer.seekable(), txt.seekable())

    def test_append_bom(self):
        # The BOM is not written again when appending to a non-empty file
        filename = os_helper.TESTFN
        for charset in ('utf-8-sig', 'utf-16', 'utf-32'):
            with self.open(filename, 'w', encoding=charset) as f:
                f.write('aaa')
                pos = f.tell()
            with self.open(filename, 'rb') as f:
                self.assertEqual(f.read(), 'aaa'.encode(charset))

            with self.open(filename, 'a', encoding=charset) as f:
                f.write('xxx')
            with self.open(filename, 'rb') as f:
                self.assertEqual(f.read(), 'aaaxxx'.encode(charset))

    def test_seek_bom(self):
        # Same test, but when seeking manually
        filename = os_helper.TESTFN
        for charset in ('utf-8-sig', 'utf-16', 'utf-32'):
            with self.open(filename, 'w', encoding=charset) as f:
                f.write('aaa')
                pos = f.tell()
            with self.open(filename, 'r+', encoding=charset) as f:
                f.seek(pos)
                f.write('zzz')
                f.seek(0)
                f.write('bbb')
            with self.open(filename, 'rb') as f:
                self.assertEqual(f.read(), 'bbbzzz'.encode(charset))

    def test_seek_append_bom(self):
        # Same test, but first seek to the start and then to the end
        filename = os_helper.TESTFN
        for charset in ('utf-8-sig', 'utf-16', 'utf-32'):
            with self.open(filename, 'w', encoding=charset) as f:
                f.write('aaa')
            with self.open(filename, 'a', encoding=charset) as f:
                f.seek(0)
                f.seek(0, self.SEEK_END)
                f.write('xxx')
            with self.open(filename, 'rb') as f:
                self.assertEqual(f.read(), 'aaaxxx'.encode(charset))

    def test_errors_property(self):
        with self.open(os_helper.TESTFN, "w", encoding="utf-8") as f:
            self.assertEqual(f.errors, "strict")
        with self.open(os_helper.TESTFN, "w", encoding="utf-8", errors="replace") as f:
            self.assertEqual(f.errors, "replace")

    @support.no_tracing
    @threading_helper.requires_working_threading()
    def test_threads_write(self):
        # Issue6750: concurrent writes could duplicate data
        event = threading.Event()
        with self.open(os_helper.TESTFN, "w", encoding="utf-8", buffering=1) as f:
            def run(n):
                text = "Thread%03d\n" % n
                event.wait()
                f.write(text)
            threads = [threading.Thread(target=run, args=(x,))
                       for x in range(20)]
            with threading_helper.start_threads(threads, event.set):
                time.sleep(0.02)
        with self.open(os_helper.TESTFN, encoding="utf-8") as f:
            content = f.read()
            for n in range(20):
                self.assertEqual(content.count("Thread%03d\n" % n), 1)

    def test_flush_error_on_close(self):
        # Test that text file is closed despite failed flush
        # and that flush() is called before file closed.
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        closed = []
        def bad_flush():
            closed[:] = [txt.closed, txt.buffer.closed]
            raise OSError()
        txt.flush = bad_flush
        self.assertRaises(OSError, txt.close) # exception not swallowed
        self.assertTrue(txt.closed)
        self.assertTrue(txt.buffer.closed)
        self.assertTrue(closed)      # flush() called
        self.assertFalse(closed[0])  # flush() called before file closed
        self.assertFalse(closed[1])
        txt.flush = lambda: None  # break reference loop

    def test_close_error_on_close(self):
        buffer = self.BytesIO(self.testdata)
        def bad_flush():
            raise OSError('flush')
        def bad_close():
            raise OSError('close')
        buffer.close = bad_close
        txt = self.TextIOWrapper(buffer, encoding="ascii")
        txt.flush = bad_flush
        with self.assertRaises(OSError) as err: # exception not swallowed
            txt.close()
        self.assertEqual(err.exception.args, ('close',))
        self.assertIsInstance(err.exception.__context__, OSError)
        self.assertEqual(err.exception.__context__.args, ('flush',))
        self.assertFalse(txt.closed)

        # Silence destructor error
        buffer.close = lambda: None
        txt.flush = lambda: None

    def test_nonnormalized_close_error_on_close(self):
        # Issue #21677
        buffer = self.BytesIO(self.testdata)
        def bad_flush():
            raise non_existing_flush
        def bad_close():
            raise non_existing_close
        buffer.close = bad_close
        txt = self.TextIOWrapper(buffer, encoding="ascii")
        txt.flush = bad_flush
        with self.assertRaises(NameError) as err: # exception not swallowed
            txt.close()
        self.assertIn('non_existing_close', str(err.exception))
        self.assertIsInstance(err.exception.__context__, NameError)
        self.assertIn('non_existing_flush', str(err.exception.__context__))
        self.assertFalse(txt.closed)

        # Silence destructor error
        buffer.close = lambda: None
        txt.flush = lambda: None

    def test_multi_close(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        txt.close()
        txt.close()
        txt.close()
        self.assertRaises(ValueError, txt.flush)

    def test_unseekable(self):
        txt = self.TextIOWrapper(self.MockUnseekableIO(self.testdata), encoding="utf-8")
        self.assertRaises(self.UnsupportedOperation, txt.tell)
        self.assertRaises(self.UnsupportedOperation, txt.seek, 0)

    def test_readonly_attributes(self):
        txt = self.TextIOWrapper(self.BytesIO(self.testdata), encoding="ascii")
        buf = self.BytesIO(self.testdata)
        with self.assertRaises(AttributeError):
            txt.buffer = buf

    def test_rawio(self):
        # Issue #12591: TextIOWrapper must work with raw I/O objects, so
        # that subprocess.Popen() can have the required unbuffered
        # semantics with universal_newlines=True.
        raw = self.MockRawIO([b'abc', b'def', b'ghi\njkl\nopq\n'])
        txt = self.TextIOWrapper(raw, encoding='ascii', newline='\n')
        # Reads
        self.assertEqual(txt.read(4), 'abcd')
        self.assertEqual(txt.readline(), 'efghi\n')
        self.assertEqual(list(txt), ['jkl\n', 'opq\n'])

    def test_rawio_write_through(self):
        # Issue #12591: with write_through=True, writes don't need a flush
        raw = self.MockRawIO([b'abc', b'def', b'ghi\njkl\nopq\n'])
        txt = self.TextIOWrapper(raw, encoding='ascii', newline='\n',
                                 write_through=True)
        txt.write('1')
        txt.write('23\n4')
        txt.write('5')
        self.assertEqual(b''.join(raw._write_stack), b'123\n45')

    def test_bufio_write_through(self):
        # Issue #21396: write_through=True doesn't force a flush()
        # on the underlying binary buffered object.
        flush_called, write_called = [], []
        class BufferedWriter(self.BufferedWriter):
            def flush(self, *args, **kwargs):
                flush_called.append(True)
                return super().flush(*args, **kwargs)
            def write(self, *args, **kwargs):
                write_called.append(True)
                return super().write(*args, **kwargs)

        rawio = self.BytesIO()
        data = b"a"
        bufio = BufferedWriter(rawio, len(data)*2)
        textio = self.TextIOWrapper(bufio, encoding='ascii',
                                    write_through=True)
        # write to the buffered io but don't overflow the buffer
        text = data.decode('ascii')
        textio.write(text)

        # buffer.flush is not called with write_through=True
        self.assertFalse(flush_called)
        # buffer.write *is* called with write_through=True
        self.assertTrue(write_called)
        self.assertEqual(rawio.getvalue(), b"") # no flush

        write_called = [] # reset
        textio.write(text * 10) # total content is larger than bufio buffer
        self.assertTrue(write_called)
        self.assertEqual(rawio.getvalue(), data * 11) # all flushed

    def test_reconfigure_write_through(self):
        raw = self.MockRawIO([])
        t = self.TextIOWrapper(raw, encoding='ascii', newline='\n')
        t.write('1')
        t.reconfigure(write_through=True)  # implied flush
        self.assertEqual(t.write_through, True)
        self.assertEqual(b''.join(raw._write_stack), b'1')
        t.write('23')
        self.assertEqual(b''.join(raw._write_stack), b'123')
        t.reconfigure(write_through=False)
        self.assertEqual(t.write_through, False)
        t.write('45')
        t.flush()
        self.assertEqual(b''.join(raw._write_stack), b'12345')
        # Keeping default value
        t.reconfigure()
        t.reconfigure(write_through=None)
        self.assertEqual(t.write_through, False)
        t.reconfigure(write_through=True)
        t.reconfigure()
        t.reconfigure(write_through=None)
        self.assertEqual(t.write_through, True)

    def test_read_nonbytes(self):
        # Issue #17106
        # Crash when underlying read() returns non-bytes
        t = self.TextIOWrapper(self.StringIO('a'), encoding="utf-8")
        self.assertRaises(TypeError, t.read, 1)
        t = self.TextIOWrapper(self.StringIO('a'), encoding="utf-8")
        self.assertRaises(TypeError, t.readline)
        t = self.TextIOWrapper(self.StringIO('a'), encoding="utf-8")
        self.assertRaises(TypeError, t.read)

    def test_illegal_encoder(self):
        # Issue 31271: Calling write() while the return value of encoder's
        # encode() is invalid shouldn't cause an assertion failure.
        rot13 = codecs.lookup("rot13")
        with support.swap_attr(rot13, '_is_text_encoding', True):
            t = self.TextIOWrapper(self.BytesIO(b'foo'), encoding="rot13")
        self.assertRaises(TypeError, t.write, 'bar')

    def test_illegal_decoder(self):
        # Issue #17106
        # Bypass the early encoding check added in issue 20404
        def _make_illegal_wrapper():
            quopri = codecs.lookup("quopri")
            quopri._is_text_encoding = True
            try:
                t = self.TextIOWrapper(self.BytesIO(b'aaaaaa'),
                                       newline='\n', encoding="quopri")
            finally:
                quopri._is_text_encoding = False
            return t
        # Crash when decoder returns non-string
        t = _make_illegal_wrapper()
        self.assertRaises(TypeError, t.read, 1)
        t = _make_illegal_wrapper()
        self.assertRaises(TypeError, t.readline)
        t = _make_illegal_wrapper()
        self.assertRaises(TypeError, t.read)

        # Issue 31243: calling read() while the return value of decoder's
        # getstate() is invalid should neither crash the interpreter nor
        # raise a SystemError.
        def _make_very_illegal_wrapper(getstate_ret_val):
            class BadDecoder:
                def getstate(self):
                    return getstate_ret_val
            def _get_bad_decoder(dummy):
                return BadDecoder()
            quopri = codecs.lookup("quopri")
            with support.swap_attr(quopri, 'incrementaldecoder',
                                   _get_bad_decoder):
                return _make_illegal_wrapper()
        t = _make_very_illegal_wrapper(42)
        self.assertRaises(TypeError, t.read, 42)
        t = _make_very_illegal_wrapper(())
        self.assertRaises(TypeError, t.read, 42)
        t = _make_very_illegal_wrapper((1, 2))
        self.assertRaises(TypeError, t.read, 42)

    def _check_create_at_shutdown(self, **kwargs):
        # Issue #20037: creating a TextIOWrapper at shutdown
        # shouldn't crash the interpreter.
        iomod = self.io.__name__
        code = """if 1:
            import codecs
            import {iomod} as io

            # Avoid looking up codecs at shutdown
            codecs.lookup('utf-8')

            class C:
                def __del__(self):
                    io.TextIOWrapper(io.BytesIO(), **{kwargs})
                    print("ok")
            c = C()
            """.format(iomod=iomod, kwargs=kwargs)
        return assert_python_ok("-c", code)

    def test_create_at_shutdown_without_encoding(self):
        rc, out, err = self._check_create_at_shutdown()
        if err:
            # Can error out with a RuntimeError if the module state
            # isn't found.
            self.assertIn(self.shutdown_error, err.decode())
        else:
            self.assertEqual("ok", out.decode().strip())

    def test_create_at_shutdown_with_encoding(self):
        rc, out, err = self._check_create_at_shutdown(encoding='utf-8',
                                                      errors='strict')
        self.assertFalse(err)
        self.assertEqual("ok", out.decode().strip())

    def test_read_byteslike(self):
        r = MemviewBytesIO(b'Just some random string\n')
        t = self.TextIOWrapper(r, 'utf-8')

        # TextIOwrapper will not read the full string, because
        # we truncate it to a multiple of the native int size
        # so that we can construct a more complex memoryview.
        bytes_val =  _to_memoryview(r.getvalue()).tobytes()

        self.assertEqual(t.read(200), bytes_val.decode('utf-8'))

    def test_issue22849(self):
        class F(object):
            def readable(self): return True
            def writable(self): return True
            def seekable(self): return True

        for i in range(10):
            try:
                self.TextIOWrapper(F(), encoding='utf-8')
            except Exception:
                pass

        F.tell = lambda x: 0
        t = self.TextIOWrapper(F(), encoding='utf-8')

    def test_reconfigure_locale(self):
        wrapper = self.TextIOWrapper(self.BytesIO(b"test"))
        wrapper.reconfigure(encoding="locale")

    def test_reconfigure_encoding_read(self):
        # latin1 -> utf8
        # (latin1 can decode utf-8 encoded string)
        data = 'abc\xe9\n'.encode('latin1') + 'd\xe9f\n'.encode('utf8')
        raw = self.BytesIO(data)
        txt = self.TextIOWrapper(raw, encoding='latin1', newline='\n')
        self.assertEqual(txt.readline(), 'abc\xe9\n')
        with self.assertRaises(self.UnsupportedOperation):
            txt.reconfigure(encoding='utf-8')
        with self.assertRaises(self.UnsupportedOperation):
            txt.reconfigure(newline=None)

    def test_reconfigure_write_fromascii(self):
        # ascii has a specific encodefunc in the C implementation,
        # but utf-8-sig has not. Make sure that we get rid of the
        # cached encodefunc when we switch encoders.
        raw = self.BytesIO()
        txt = self.TextIOWrapper(raw, encoding='ascii', newline='\n')
        txt.write('foo\n')
        txt.reconfigure(encoding='utf-8-sig')
        txt.write('\xe9\n')
        txt.flush()
        self.assertEqual(raw.getvalue(), b'foo\n\xc3\xa9\n')

    def test_reconfigure_write(self):
        # latin -> utf8
        raw = self.BytesIO()
        txt = self.TextIOWrapper(raw, encoding='latin1', newline='\n')
        txt.write('abc\xe9\n')
        txt.reconfigure(encoding='utf-8')
        self.assertEqual(raw.getvalue(), b'abc\xe9\n')
        txt.write('d\xe9f\n')
        txt.flush()
        self.assertEqual(raw.getvalue(), b'abc\xe9\nd\xc3\xa9f\n')

        # ascii -> utf-8-sig: ensure that no BOM is written in the middle of
        # the file
        raw = self.BytesIO()
        txt = self.TextIOWrapper(raw, encoding='ascii', newline='\n')
        txt.write('abc\n')
        txt.reconfigure(encoding='utf-8-sig')
        txt.write('d\xe9f\n')
        txt.flush()
        self.assertEqual(raw.getvalue(), b'abc\nd\xc3\xa9f\n')

    def test_reconfigure_write_non_seekable(self):
        raw = self.BytesIO()
        raw.seekable = lambda: False
        raw.seek = None
        txt = self.TextIOWrapper(raw, encoding='ascii', newline='\n')
        txt.write('abc\n')
        txt.reconfigure(encoding='utf-8-sig')
        txt.write('d\xe9f\n')
        txt.flush()

        # If the raw stream is not seekable, there'll be a BOM
        self.assertEqual(raw.getvalue(),  b'abc\n\xef\xbb\xbfd\xc3\xa9f\n')

    def test_reconfigure_defaults(self):
        txt = self.TextIOWrapper(self.BytesIO(), 'ascii', 'replace', '\n')
        txt.reconfigure(encoding=None)
        self.assertEqual(txt.encoding, 'ascii')
        self.assertEqual(txt.errors, 'replace')
        txt.write('LF\n')

        txt.reconfigure(newline='\r\n')
        self.assertEqual(txt.encoding, 'ascii')
        self.assertEqual(txt.errors, 'replace')

        txt.reconfigure(errors='ignore')
        self.assertEqual(txt.encoding, 'ascii')
        self.assertEqual(txt.errors, 'ignore')
        txt.write('CRLF\n')

        txt.reconfigure(encoding='utf-8', newline=None)
        self.assertEqual(txt.errors, 'strict')
        txt.seek(0)
        self.assertEqual(txt.read(), 'LF\nCRLF\n')

        self.assertEqual(txt.detach().getvalue(), b'LF\nCRLF\r\n')

    def test_reconfigure_errors(self):
        txt = self.TextIOWrapper(self.BytesIO(), 'ascii', 'replace', '\r')
        with self.assertRaises(TypeError):  # there was a crash
            txt.reconfigure(encoding=42)
        if self.is_C:
            with self.assertRaises(UnicodeEncodeError):
                txt.reconfigure(encoding='\udcfe')
            with self.assertRaises(LookupError):
                txt.reconfigure(encoding='locale\0')
        # TODO: txt.reconfigure(encoding='utf-8\0')
        # TODO: txt.reconfigure(encoding='nonexisting')
        with self.assertRaises(TypeError):
            txt.reconfigure(errors=42)
        if self.is_C:
            with self.assertRaises(UnicodeEncodeError):
                txt.reconfigure(errors='\udcfe')
        # TODO: txt.reconfigure(errors='ignore\0')
        # TODO: txt.reconfigure(errors='nonexisting')
        with self.assertRaises(TypeError):
            txt.reconfigure(newline=42)
        with self.assertRaises(ValueError):
            txt.reconfigure(newline='\udcfe')
        with self.assertRaises(ValueError):
            txt.reconfigure(newline='xyz')
        if not self.is_C:
            # TODO: Should fail in C too.
            with self.assertRaises(ValueError):
                txt.reconfigure(newline='\n\0')
        if self.is_C:
            # TODO: Use __bool__(), not __index__().
            with self.assertRaises(ZeroDivisionError):
                txt.reconfigure(line_buffering=BadIndex())
            with self.assertRaises(OverflowError):
                txt.reconfigure(line_buffering=2**1000)
            with self.assertRaises(ZeroDivisionError):
                txt.reconfigure(write_through=BadIndex())
            with self.assertRaises(OverflowError):
                txt.reconfigure(write_through=2**1000)
            with self.assertRaises(ZeroDivisionError):  # there was a crash
                txt.reconfigure(line_buffering=BadIndex(),
                                write_through=BadIndex())
        self.assertEqual(txt.encoding, 'ascii')
        self.assertEqual(txt.errors, 'replace')
        self.assertIs(txt.line_buffering, False)
        self.assertIs(txt.write_through, False)

        txt.reconfigure(encoding='latin1', errors='ignore', newline='\r\n',
                        line_buffering=True, write_through=True)
        self.assertEqual(txt.encoding, 'latin1')
        self.assertEqual(txt.errors, 'ignore')
        self.assertIs(txt.line_buffering, True)
        self.assertIs(txt.write_through, True)

    def test_reconfigure_newline(self):
        raw = self.BytesIO(b'CR\rEOF')
        txt = self.TextIOWrapper(raw, 'ascii', newline='\n')
        txt.reconfigure(newline=None)
        self.assertEqual(txt.readline(), 'CR\n')
        raw = self.BytesIO(b'CR\rEOF')
        txt = self.TextIOWrapper(raw, 'ascii', newline='\n')
        txt.reconfigure(newline='')
        self.assertEqual(txt.readline(), 'CR\r')
        raw = self.BytesIO(b'CR\rLF\nEOF')
        txt = self.TextIOWrapper(raw, 'ascii', newline='\r')
        txt.reconfigure(newline='\n')
        self.assertEqual(txt.readline(), 'CR\rLF\n')
        raw = self.BytesIO(b'LF\nCR\rEOF')
        txt = self.TextIOWrapper(raw, 'ascii', newline='\n')
        txt.reconfigure(newline='\r')
        self.assertEqual(txt.readline(), 'LF\nCR\r')
        raw = self.BytesIO(b'CR\rCRLF\r\nEOF')
        txt = self.TextIOWrapper(raw, 'ascii', newline='\r')
        txt.reconfigure(newline='\r\n')
        self.assertEqual(txt.readline(), 'CR\rCRLF\r\n')

        txt = self.TextIOWrapper(self.BytesIO(), 'ascii', newline='\r')
        txt.reconfigure(newline=None)
        txt.write('linesep\n')
        txt.reconfigure(newline='')
        txt.write('LF\n')
        txt.reconfigure(newline='\n')
        txt.write('LF\n')
        txt.reconfigure(newline='\r')
        txt.write('CR\n')
        txt.reconfigure(newline='\r\n')
        txt.write('CRLF\n')
        expected = 'linesep' + os.linesep + 'LF\nLF\nCR\rCRLF\r\n'
        self.assertEqual(txt.detach().getvalue().decode('ascii'), expected)

    def test_issue25862(self):
        # Assertion failures occurred in tell() after read() and write().
        t = self.TextIOWrapper(self.BytesIO(b'test'), encoding='ascii')
        t.read(1)
        t.read()
        t.tell()
        t = self.TextIOWrapper(self.BytesIO(b'test'), encoding='ascii')
        t.read(1)
        t.write('x')
        t.tell()

    def test_issue35928(self):
        p = self.BufferedRWPair(self.BytesIO(b'foo\nbar\n'), self.BytesIO())
        f = self.TextIOWrapper(p)
        res = f.readline()
        self.assertEqual(res, 'foo\n')
        f.write(res)
        self.assertEqual(res + f.readline(), 'foo\nbar\n')

    def test_pickling_subclass(self):
        global MyTextIO
        class MyTextIO(self.TextIOWrapper):
            def __init__(self, raw, tag):
                super().__init__(raw)
                self.tag = tag
            def __getstate__(self):
                return self.tag, self.buffer.getvalue()
            def __setstate__(slf, state):
                tag, value = state
                slf.__init__(self.BytesIO(value), tag)

        raw = self.BytesIO(b'data')
        txt = MyTextIO(raw, 'ham')
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(protocol=proto):
                pickled = pickle.dumps(txt, proto)
                newtxt = pickle.loads(pickled)
                self.assertEqual(newtxt.buffer.getvalue(), b'data')
                self.assertEqual(newtxt.tag, 'ham')
        del MyTextIO

    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_read_non_blocking(self):
        import os
        r, w = os.pipe()
        try:
            os.set_blocking(r, False)
            with self.io.open(r, 'rt') as textfile:
                r = None
                # Nothing has been written so a non-blocking read raises a BlockingIOError exception.
                with self.assertRaises(BlockingIOError):
                    textfile.read()
        finally:
            if r is not None:
                os.close(r)
            os.close(w)


class MemviewBytesIO(io.BytesIO):
    '''A BytesIO object whose read method returns memoryviews
       rather than bytes'''

    def read1(self, len_):
        return _to_memoryview(super().read1(len_))

    def read(self, len_):
        return _to_memoryview(super().read(len_))

def _to_memoryview(buf):
    '''Convert bytes-object *buf* to a non-trivial memoryview'''

    arr = array.array('i')
    idx = len(buf) - len(buf) % arr.itemsize
    arr.frombytes(buf[:idx])
    return memoryview(arr)


class CTextIOWrapperTest(TextIOWrapperTest, CTestCase):
    shutdown_error = "LookupError: unknown encoding: ascii"

    def test_initialization(self):
        r = self.BytesIO(b"\xc3\xa9\n\n")
        b = self.BufferedReader(r, 1000)
        t = self.TextIOWrapper(b, encoding="utf-8")
        self.assertRaises(ValueError, t.__init__, b, encoding="utf-8", newline='xyzzy')
        self.assertRaises(ValueError, t.read)

        t = self.TextIOWrapper.__new__(self.TextIOWrapper)
        self.assertRaises(Exception, repr, t)

    def test_garbage_collection(self):
        # C TextIOWrapper objects are collected, and collecting them flushes
        # all data to disk.
        # The Python version has __del__, so it ends in gc.garbage instead.
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", ResourceWarning)
            rawio = self.FileIO(os_helper.TESTFN, "wb")
            b = self.BufferedWriter(rawio)
            t = self.TextIOWrapper(b, encoding="ascii")
            t.write("456def")
            t.x = t
            wr = weakref.ref(t)
            del t
            support.gc_collect()
        self.assertIsNone(wr(), wr)
        with self.open(os_helper.TESTFN, "rb") as f:
            self.assertEqual(f.read(), b"456def")

    def test_rwpair_cleared_before_textio(self):
        # Issue 13070: TextIOWrapper's finalization would crash when called
        # after the reference to the underlying BufferedRWPair's writer got
        # cleared by the GC.
        for i in range(1000):
            b1 = self.BufferedRWPair(self.MockRawIO(), self.MockRawIO())
            t1 = self.TextIOWrapper(b1, encoding="ascii")
            b2 = self.BufferedRWPair(self.MockRawIO(), self.MockRawIO())
            t2 = self.TextIOWrapper(b2, encoding="ascii")
            # circular references
            t1.buddy = t2
            t2.buddy = t1
        support.gc_collect()

    def test_del__CHUNK_SIZE_SystemError(self):
        t = self.TextIOWrapper(self.BytesIO(), encoding='ascii')
        with self.assertRaises(AttributeError):
            del t._CHUNK_SIZE

    def test_internal_buffer_size(self):
        # bpo-43260: TextIOWrapper's internal buffer should not store
        # data larger than chunk size.
        chunk_size = 8192  # default chunk size, updated later

        class MockIO(self.MockRawIO):
            def write(self, data):
                if len(data) > chunk_size:
                    raise RuntimeError
                return super().write(data)

        buf = MockIO()
        t = self.TextIOWrapper(buf, encoding="ascii")
        chunk_size = t._CHUNK_SIZE
        t.write("abc")
        t.write("def")
        # default chunk size is 8192 bytes so t don't write data to buf.
        self.assertEqual([], buf._write_stack)

        with self.assertRaises(RuntimeError):
            t.write("x"*(chunk_size+1))

        self.assertEqual([b"abcdef"], buf._write_stack)
        t.write("ghi")
        t.write("x"*chunk_size)
        self.assertEqual([b"abcdef", b"ghi", b"x"*chunk_size], buf._write_stack)

    def test_issue119506(self):
        chunk_size = 8192

        class MockIO(self.MockRawIO):
            written = False
            def write(self, data):
                if not self.written:
                    self.written = True
                    t.write("middle")
                return super().write(data)

        buf = MockIO()
        t = self.TextIOWrapper(buf)
        t.write("abc")
        t.write("def")
        # writing data which size >= chunk_size cause flushing buffer before write.
        t.write("g" * chunk_size)
        t.flush()

        self.assertEqual([b"abcdef", b"middle", b"g"*chunk_size],
                         buf._write_stack)


class PyTextIOWrapperTest(TextIOWrapperTest, PyTestCase):
    shutdown_error = "LookupError: unknown encoding: ascii"


class IncrementalNewlineDecoderTest:

    def check_newline_decoding_utf8(self, decoder):
        # UTF-8 specific tests for a newline decoder
        def _check_decode(b, s, **kwargs):
            # We exercise getstate() / setstate() as well as decode()
            state = decoder.getstate()
            self.assertEqual(decoder.decode(b, **kwargs), s)
            decoder.setstate(state)
            self.assertEqual(decoder.decode(b, **kwargs), s)

        _check_decode(b'\xe8\xa2\x88', "\u8888")

        _check_decode(b'\xe8', "")
        _check_decode(b'\xa2', "")
        _check_decode(b'\x88', "\u8888")

        _check_decode(b'\xe8', "")
        _check_decode(b'\xa2', "")
        _check_decode(b'\x88', "\u8888")

        _check_decode(b'\xe8', "")
        self.assertRaises(UnicodeDecodeError, decoder.decode, b'', final=True)

        decoder.reset()
        _check_decode(b'\n', "\n")
        _check_decode(b'\r', "")
        _check_decode(b'', "\n", final=True)
        _check_decode(b'\r', "\n", final=True)

        _check_decode(b'\r', "")
        _check_decode(b'a', "\na")

        _check_decode(b'\r\r\n', "\n\n")
        _check_decode(b'\r', "")
        _check_decode(b'\r', "\n")
        _check_decode(b'\na', "\na")

        _check_decode(b'\xe8\xa2\x88\r\n', "\u8888\n")
        _check_decode(b'\xe8\xa2\x88', "\u8888")
        _check_decode(b'\n', "\n")
        _check_decode(b'\xe8\xa2\x88\r', "\u8888")
        _check_decode(b'\n', "\n")

    def check_newline_decoding(self, decoder, encoding):
        result = []
        if encoding is not None:
            encoder = codecs.getincrementalencoder(encoding)()
            def _decode_bytewise(s):
                # Decode one byte at a time
                for b in encoder.encode(s):
                    result.append(decoder.decode(bytes([b])))
        else:
            encoder = None
            def _decode_bytewise(s):
                # Decode one char at a time
                for c in s:
                    result.append(decoder.decode(c))
        self.assertEqual(decoder.newlines, None)
        _decode_bytewise("abc\n\r")
        self.assertEqual(decoder.newlines, '\n')
        _decode_bytewise("\nabc")
        self.assertEqual(decoder.newlines, ('\n', '\r\n'))
        _decode_bytewise("abc\r")
        self.assertEqual(decoder.newlines, ('\n', '\r\n'))
        _decode_bytewise("abc")
        self.assertEqual(decoder.newlines, ('\r', '\n', '\r\n'))
        _decode_bytewise("abc\r")
        self.assertEqual("".join(result), "abc\n\nabcabc\nabcabc")
        decoder.reset()
        input = "abc"
        if encoder is not None:
            encoder.reset()
            input = encoder.encode(input)
        self.assertEqual(decoder.decode(input), "abc")
        self.assertEqual(decoder.newlines, None)

    def test_newline_decoder(self):
        encodings = (
            # None meaning the IncrementalNewlineDecoder takes unicode input
            # rather than bytes input
            None, 'utf-8', 'latin-1',
            'utf-16', 'utf-16-le', 'utf-16-be',
            'utf-32', 'utf-32-le', 'utf-32-be',
        )
        for enc in encodings:
            decoder = enc and codecs.getincrementaldecoder(enc)()
            decoder = self.IncrementalNewlineDecoder(decoder, translate=True)
            self.check_newline_decoding(decoder, enc)
        decoder = codecs.getincrementaldecoder("utf-8")()
        decoder = self.IncrementalNewlineDecoder(decoder, translate=True)
        self.check_newline_decoding_utf8(decoder)
        self.assertRaises(TypeError, decoder.setstate, 42)

    def test_newline_bytes(self):
        # Issue 5433: Excessive optimization in IncrementalNewlineDecoder
        def _check(dec):
            self.assertEqual(dec.newlines, None)
            self.assertEqual(dec.decode("\u0D00"), "\u0D00")
            self.assertEqual(dec.newlines, None)
            self.assertEqual(dec.decode("\u0A00"), "\u0A00")
            self.assertEqual(dec.newlines, None)
        dec = self.IncrementalNewlineDecoder(None, translate=False)
        _check(dec)
        dec = self.IncrementalNewlineDecoder(None, translate=True)
        _check(dec)

    def test_translate(self):
        # issue 35062
        for translate in (-2, -1, 1, 2):
            decoder = codecs.getincrementaldecoder("utf-8")()
            decoder = self.IncrementalNewlineDecoder(decoder, translate)
            self.check_newline_decoding_utf8(decoder)
        decoder = codecs.getincrementaldecoder("utf-8")()
        decoder = self.IncrementalNewlineDecoder(decoder, translate=0)
        self.assertEqual(decoder.decode(b"\r\r\n"), "\r\r\n")

class CIncrementalNewlineDecoderTest(IncrementalNewlineDecoderTest, unittest.TestCase):
    IncrementalNewlineDecoder = io.IncrementalNewlineDecoder

    @support.cpython_only
    def test_uninitialized(self):
        uninitialized = self.IncrementalNewlineDecoder.__new__(
            self.IncrementalNewlineDecoder)
        self.assertRaises(ValueError, uninitialized.decode, b'bar')
        self.assertRaises(ValueError, uninitialized.getstate)
        self.assertRaises(ValueError, uninitialized.setstate, (b'foo', 0))
        self.assertRaises(ValueError, uninitialized.reset)


class PyIncrementalNewlineDecoderTest(IncrementalNewlineDecoderTest, unittest.TestCase):
    IncrementalNewlineDecoder = pyio.IncrementalNewlineDecoder
