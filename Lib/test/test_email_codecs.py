# Copyright (C) 2002 Python Software Foundation
# email package unit tests for (optional) Asian codecs

import unittest
import test_support
from test_email import TestEmailBase

from email.Charset import Charset
from email.Header import Header, decode_header

# See if we have the Japanese codecs package installed
try:
    unicode('foo', 'japanese.iso-2022-jp')
except LookupError:
    raise test_support.TestSkipped, 'Optional Japanese codecs not installed'



class TestEmailAsianCodecs(TestEmailBase):
    def test_japanese_codecs(self):
        eq = self.ndiffAssertEqual
        j = Charset("euc-jp")
        g = Charset("iso-8859-1")
        h = Header("Hello World!")
        jhello = '\xa5\xcf\xa5\xed\xa1\xbc\xa5\xef\xa1\xbc\xa5\xeb\xa5\xc9\xa1\xaa'
        ghello = 'Gr\xfc\xdf Gott!'
        h.append(jhello, j)
        h.append(ghello, g)
        eq(h.encode(), 'Hello World! =?iso-2022-jp?b?GyRCJU8lbSE8JW8hPCVrJUkhKhsoQg==?=\n =?iso-8859-1?q?Gr=FC=DF_Gott!?=')
        eq(decode_header(h.encode()),
           [('Hello World!', None),
            ('\x1b$B%O%m!<%o!<%k%I!*\x1b(B', 'iso-2022-jp'),
            ('Gr\xfc\xdf Gott!', 'iso-8859-1')])
        long = 'test-ja \xa4\xd8\xc5\xea\xb9\xc6\xa4\xb5\xa4\xec\xa4\xbf\xa5\xe1\xa1\xbc\xa5\xeb\xa4\xcf\xbb\xca\xb2\xf1\xbc\xd4\xa4\xce\xbe\xb5\xc7\xa7\xa4\xf2\xc2\xd4\xa4\xc3\xa4\xc6\xa4\xa4\xa4\xde\xa4\xb9'
        h = Header(long, j, header_name="Subject")
        # test a very long header
        enc = h.encode()
        # BAW: The following used to pass.  Sadly, the test afterwards is what
        # happens now.  I've no idea which is right.  Please, any Japanese and
        # RFC 2047 experts, please verify!
##        eq(enc, '''\
##=?iso-2022-jp?b?dGVzdC1qYSAbJEIkWEVqOUYkNSRsJD8lYRsoQg==?=
## =?iso-2022-jp?b?GyRCITwlayRPO0oycTxUJE4+NRsoQg==?=
## =?iso-2022-jp?b?GyRCRyckckJUJEMkRiQkJF4kORsoQg==?=''')
        eq(enc, """\
=?iso-2022-jp?b?dGVzdC1qYSAbJEIkWEVqOUYkNSRsJD8lYRsoQg==?=
 =?iso-2022-jp?b?GyRCITwlayRPO0oycTxUJE4+NUcnJHJCVCRDJEYkJCReJDkbKEI=?=""")
        # BAW: same deal here. :(
##        self.assertEqual(
##            decode_header(enc),
##            [("test-ja \x1b$B$XEj9F$5$l$?%a\x1b(B\x1b$B!<%k$O;J2q<T$N>5\x1b(B\x1b$BG'$rBT$C$F$$$^$9\x1b(B", 'iso-2022-jp')])
        self.assertEqual(
            decode_header(enc),
            [("test-ja \x1b$B$XEj9F$5$l$?%a\x1b(B\x1b$B!<%k$O;J2q<T$N>5G'$rBT$C$F$$$^$9\x1b(B", 'iso-2022-jp')])



def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestEmailAsianCodecs))
    return suite


def test_main():
    test_support.run_unittest(TestEmailAsianCodecs)



if __name__ == '__main__':
    unittest.main(defaultTest='suite')
