from test import test_support
import unittest

import sys, os, io, subprocess
import quopri



ENCSAMPLE = b"""\
Here's a bunch of special=20

=A1=A2=A3=A4=A5=A6=A7=A8=A9
=AA=AB=AC=AD=AE=AF=B0=B1=B2=B3
=B4=B5=B6=B7=B8=B9=BA=BB=BC=BD=BE
=BF=C0=C1=C2=C3=C4=C5=C6
=C7=C8=C9=CA=CB=CC=CD=CE=CF
=D0=D1=D2=D3=D4=D5=D6=D7
=D8=D9=DA=DB=DC=DD=DE=DF
=E0=E1=E2=E3=E4=E5=E6=E7
=E8=E9=EA=EB=EC=ED=EE=EF
=F0=F1=F2=F3=F4=F5=F6=F7
=F8=F9=FA=FB=FC=FD=FE=FF

characters... have fun!
"""

# First line ends with a space
DECSAMPLE = b"Here's a bunch of special \n" + \
b"""\

\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9
\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3
\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe
\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6
\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf
\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7
\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf
\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7
\xe8\xe9\xea\xeb\xec\xed\xee\xef
\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7
\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff

characters... have fun!
"""


def withpythonimplementation(testfunc):
    def newtest(self):
        # Test default implementation
        testfunc(self)
        # Test Python implementation
        if quopri.b2a_qp is not None or quopri.a2b_qp is not None:
            oldencode = quopri.b2a_qp
            olddecode = quopri.a2b_qp
            try:
                quopri.b2a_qp = None
                quopri.a2b_qp = None
                testfunc(self)
            finally:
                quopri.b2a_qp = oldencode
                quopri.a2b_qp = olddecode
    newtest.__name__ = testfunc.__name__
    return newtest

class QuopriTestCase(unittest.TestCase):
    # Each entry is a tuple of (plaintext, encoded string).  These strings are
    # used in the "quotetabs=0" tests.
    STRINGS = (
        # Some normal strings
        (b'hello', b'hello'),
        (b'''hello
        there
        world''', b'''hello
        there
        world'''),
        (b'''hello
        there
        world
''', b'''hello
        there
        world
'''),
        (b'\201\202\203', b'=81=82=83'),
        # Add some trailing MUST QUOTE strings
        (b'hello ', b'hello=20'),
        (b'hello\t', b'hello=09'),
        # Some long lines.  First, a single line of 108 characters
        (b'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\xd8\xd9\xda\xdb\xdc\xdd\xde\xdfxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
         b'''xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx=D8=D9=DA=DB=DC=DD=DE=DFx=
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'''),
        # A line of exactly 76 characters, no soft line break should be needed
        (b'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy',
        b'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy'),
        # A line of 77 characters, forcing a soft line break at position 75,
        # and a second line of exactly 2 characters (because the soft line
        # break `=' sign counts against the line length limit).
        (b'zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz',
         b'''zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz=
zz'''),
        # A line of 151 characters, forcing a soft line break at position 75,
        # with a second line of exactly 76 characters and no trailing =
        (b'zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz',
         b'''zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz=
zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz'''),
        # A string containing a hard line break, but which the first line is
        # 151 characters and the second line is exactly 76 characters.  This
        # should leave us with three lines, the first which has a soft line
        # break, and which the second and third do not.
        (b'''yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz''',
         b'''yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy=
yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz'''),
        # Now some really complex stuff ;)
        (DECSAMPLE, ENCSAMPLE),
        )

    # These are used in the "quotetabs=1" tests.
    ESTRINGS = (
        (b'hello world', b'hello=20world'),
        (b'hello\tworld', b'hello=09world'),
        )

    # These are used in the "header=1" tests.
    HSTRINGS = (
        (b'hello world', b'hello_world'),
        (b'hello_world', b'hello=5Fworld'),
        )

    @withpythonimplementation
    def test_encodestring(self):
        for p, e in self.STRINGS:
            self.assertEqual(quopri.encodestring(p), e)

    @withpythonimplementation
    def test_decodestring(self):
        for p, e in self.STRINGS:
            self.assertEqual(quopri.decodestring(e), p)

    @withpythonimplementation
    def test_idempotent_string(self):
        for p, e in self.STRINGS:
            self.assertEqual(quopri.decodestring(quopri.encodestring(e)), e)

    @withpythonimplementation
    def test_encode(self):
        for p, e in self.STRINGS:
            infp = io.BytesIO(p)
            outfp = io.BytesIO()
            quopri.encode(infp, outfp, quotetabs=False)
            self.assertEqual(outfp.getvalue(), e)

    @withpythonimplementation
    def test_decode(self):
        for p, e in self.STRINGS:
            infp = io.BytesIO(e)
            outfp = io.BytesIO()
            quopri.decode(infp, outfp)
            self.assertEqual(outfp.getvalue(), p)

    @withpythonimplementation
    def test_embedded_ws(self):
        for p, e in self.ESTRINGS:
            self.assertEqual(quopri.encodestring(p, quotetabs=True), e)
            self.assertEqual(quopri.decodestring(e), p)

    @withpythonimplementation
    def test_encode_header(self):
        for p, e in self.HSTRINGS:
            self.assertEqual(quopri.encodestring(p, header=True), e)

    @withpythonimplementation
    def test_decode_header(self):
        for p, e in self.HSTRINGS:
            self.assertEqual(quopri.decodestring(e, header=True), p)

    def test_scriptencode(self):
        (p, e) = self.STRINGS[-1]
        process = subprocess.Popen([sys.executable, "-mquopri"],
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        cout, cerr = process.communicate(p)
        # On Windows, Python will output the result to stdout using
        # CRLF, as the mode of stdout is text mode. To compare this
        # with the expected result, we need to do a line-by-line comparison.
        cout = cout.decode('latin-1').splitlines()
        e = e.decode('latin-1').splitlines()
        assert len(cout)==len(e)
        for i in range(len(cout)):
            self.assertEqual(cout[i], e[i])
        self.assertEqual(cout, e)

    def test_scriptdecode(self):
        (p, e) = self.STRINGS[-1]
        process = subprocess.Popen([sys.executable, "-mquopri", "-d"],
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        cout, cerr = process.communicate(e)
        cout = cout.decode('latin-1')
        p = p.decode('latin-1')
        self.assertEqual(cout.splitlines(), p.splitlines())

def test_main():
    test_support.run_unittest(QuopriTestCase)


if __name__ == "__main__":
    test_main()
