import test_support
import unittest

from cStringIO import StringIO
from quopri import *



ENCSAMPLE = """\
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
DECSAMPLE = """\
Here's a bunch of special 

¡¢£¤¥¦§¨©
ª«¬­®¯°±²³
´µ¶·¸¹º»¼½¾
¿ÀÁÂÃÄÅÆ
ÇÈÉÊËÌÍÎÏ
ÐÑÒÓÔÕÖ×
ØÙÚÛÜÝÞß
àáâãäåæç
èéêëìíîï
ðñòóôõö÷
øùúûüýþÿ

characters... have fun!
"""



class QuopriTestCase(unittest.TestCase):
    # Each entry is a tuple of (plaintext, encoded string).  These strings are
    # used in the "quotetabs=0" tests.
    STRINGS = (
        # Some normal strings
        ('hello', 'hello'),
        ('''hello
        there
        world''', '''hello
        there
        world'''),
        ('''hello
        there
        world
''', '''hello
        there
        world
'''),
        ('\201\202\203', '=81=82=83'),
        # Add some trailing MUST QUOTE strings
        ('hello ', 'hello=20'),
        ('hello\t', 'hello=09'),
        # Some long lines.  First, a single line of 108 characters
        ('xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxØÙÚÛÜÝÞßxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx',
         '''xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx=D8=D9=DA=DB=DC=DD=DE=DFx=
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'''),
        # A line of exactly 76 characters, no soft line break should be needed
        ('yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy',
        'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy'),
        # A line of 77 characters, forcing a soft line break at position 75,
        # and a second line of exactly 2 characters (because the soft line
        # break `=' sign counts against the line length limit).
        ('zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz',
         '''zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz=
zz'''),
        # A line of 151 characters, forcing a soft line break at position 75,
        # with a second line of exactly 76 characters and no trailing =
        ('zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz',
         '''zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz=
zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz'''),
        # A string containing a hard line break, but which the first line is
        # 151 characters and the second line is exactly 76 characters.  This
        # should leave us with three lines, the first which has a soft line
        # break, and which the second and third do not.
        ('''yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz''',
         '''yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy=
yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy
zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz'''),
        # Now some really complex stuff ;)
        (DECSAMPLE, ENCSAMPLE),
        )

    # These are used in the "quotetabs=1" tests.
    ESTRINGS = (
        ('hello world', 'hello=20world'),
        ('hello\tworld', 'hello=09world'),
        )
        
    def test_encodestring(self):
        for p, e in self.STRINGS:
            self.assert_(encodestring(p) == e)
        
    def test_decodestring(self):
        for p, e in self.STRINGS:
            self.assert_(decodestring(e) == p)
        
    def test_idempotent_string(self):
        for p, e in self.STRINGS:
            self.assert_(decodestring(encodestring(e)) == e)

    def test_encode(self):
        for p, e in self.STRINGS:
            infp = StringIO(p)
            outfp = StringIO()
            encode(infp, outfp, quotetabs=0)
            self.assert_(outfp.getvalue() == e)

    def test_decode(self):
        for p, e in self.STRINGS:
            infp = StringIO(e)
            outfp = StringIO()
            decode(infp, outfp)
            self.assert_(outfp.getvalue() == p)

    def test_embedded_ws(self):
        for p, e in self.ESTRINGS:
            self.assert_(encodestring(p, quotetabs=1) == e)
            self.assert_(decodestring(e) == p)


test_support.run_unittest(QuopriTestCase)
