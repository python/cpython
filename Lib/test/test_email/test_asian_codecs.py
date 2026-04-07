# Copyright (C) 2002 Python Software Foundation
# Contact: email-sig@python.org
# email package unit tests for (optional) Asian codecs

import unittest

from test.test_email import TestEmailBase
from email.charset import Charset
from email.header import Header, decode_header
from email.message import Message

# We're compatible with Python 2.3, but it doesn't have the built-in Asian
# codecs, so we have to skip all these tests.
try:
    str(b'foo', 'euc-jp')
except LookupError:
    raise unittest.SkipTest



class TestEmailAsianCodecs(TestEmailBase):
    def test_japanese_codecs(self):
        eq = self.ndiffAssertEqual
        jcode = "euc-jp"
        gcode = "iso-8859-1"
        j = Charset(jcode)
        g = Charset(gcode)
        h = Header("Hello World!")
        jhello = str(b'\xa5\xcf\xa5\xed\xa1\xbc\xa5\xef\xa1\xbc'
                     b'\xa5\xeb\xa5\xc9\xa1\xaa', jcode)
        ghello = str(b'Gr\xfc\xdf Gott!', gcode)
        h.append(jhello, j)
        h.append(ghello, g)
        # BAW: This used to -- and maybe should -- fold the two iso-8859-1
        # chunks into a single encoded word.  However it doesn't violate the
        # standard to have them as two encoded chunks and maybe it's
        # reasonable <wink> for each .append() call to result in a separate
        # encoded word.
        eq(h.encode(), """\
Hello World! =?iso-2022-jp?b?GyRCJU8lbSE8JW8hPCVrJUkhKhsoQg==?=
 =?iso-8859-1?q?Gr=FC=DF_Gott!?=""")
        eq(decode_header(h.encode()),
           [(b'Hello World! ', None),
            (b'\x1b$B%O%m!<%o!<%k%I!*\x1b(B', 'iso-2022-jp'),
            (b'Gr\xfc\xdf Gott!', gcode)])
        subject_bytes = (b'test-ja \xa4\xd8\xc5\xea\xb9\xc6\xa4\xb5'
            b'\xa4\xec\xa4\xbf\xa5\xe1\xa1\xbc\xa5\xeb\xa4\xcf\xbb\xca\xb2'
            b'\xf1\xbc\xd4\xa4\xce\xbe\xb5\xc7\xa7\xa4\xf2\xc2\xd4\xa4\xc3'
            b'\xa4\xc6\xa4\xa4\xa4\xde\xa4\xb9')
        subject = str(subject_bytes, jcode)
        h = Header(subject, j, header_name="Subject")
        # test a very long header
        enc = h.encode()
        # TK: splitting point may differ by codec design and/or Header encoding
        eq(enc , """\
=?iso-2022-jp?b?dGVzdC1qYSAbJEIkWEVqOUYkNSRsJD8lYSE8JWskTztKGyhC?=
 =?iso-2022-jp?b?GyRCMnE8VCROPjVHJyRyQlQkQyRGJCQkXiQ5GyhC?=""")
        # TK: full decode comparison
        eq(str(h).encode(jcode), subject_bytes)

    def test_payload_encoding_utf8(self):
        jhello = str(b'\xa5\xcf\xa5\xed\xa1\xbc\xa5\xef\xa1\xbc'
                     b'\xa5\xeb\xa5\xc9\xa1\xaa', 'euc-jp')
        msg = Message()
        msg.set_payload(jhello, 'utf-8')
        ustr = msg.get_payload(decode=True).decode(msg.get_content_charset())
        self.assertEqual(jhello, ustr)

    def test_payload_encoding(self):
        jcode  = 'euc-jp'
        jhello = str(b'\xa5\xcf\xa5\xed\xa1\xbc\xa5\xef\xa1\xbc'
                     b'\xa5\xeb\xa5\xc9\xa1\xaa', jcode)
        msg = Message()
        msg.set_payload(jhello, jcode)
        ustr = msg.get_payload(decode=True).decode(msg.get_content_charset())
        self.assertEqual(jhello, ustr)



if __name__ == '__main__':
    unittest.main()
