import unittest
from test import test_support

import string, mimetools
import io

msgtext1 = mimetools.Message(io.StringIO(
"""Content-Type: text/plain; charset=iso-8859-1; format=flowed
Content-Transfer-Encoding: 8bit

Foo!
"""))

sample = bytes(string.ascii_letters + "=" + string.digits + "\n", "ASCII")

class MimeToolsTest(unittest.TestCase):

    def decode_encode_test(self, enc):
        i = io.BytesIO(sample)
        o = io.BytesIO()
        mimetools.encode(i, o, enc)
        i = io.BytesIO(o.getvalue())
        o = io.BytesIO()
        mimetools.decode(i, o, enc)
        self.assertEqual(o.getvalue(), sample)

    # Separate tests for better diagnostics

    def test_7bit(self):
        self.decode_encode_test('7bit')

    def test_8bit(self):
        self.decode_encode_test('8bit')

    def test_base64(self):
        self.decode_encode_test('base64')

    def test_quoted_printable(self):
        self.decode_encode_test('quoted-printable')

    def test_uuencode(self):
        self.decode_encode_test('uuencode')

    def test_x_uuencode(self):
        self.decode_encode_test('x-uuencode')

    def test_uue(self):
        self.decode_encode_test('uue')

    def test_x_uue(self):
        self.decode_encode_test('x-uue')

    def test_boundary(self):
        s = set([""])
        for i in range(100):
            nb = mimetools.choose_boundary()
            self.assert_(nb not in s)
            s.add(nb)

    def test_message(self):
        msg = mimetools.Message(io.StringIO(msgtext1))
        self.assertEqual(msg.gettype(), "text/plain")
        self.assertEqual(msg.getmaintype(), "text")
        self.assertEqual(msg.getsubtype(), "plain")
        self.assertEqual(msg.getplist(), ["charset=iso-8859-1", "format=flowed"])
        self.assertEqual(msg.getparamnames(), ["charset", "format"])
        self.assertEqual(msg.getparam("charset"), "iso-8859-1")
        self.assertEqual(msg.getparam("format"), "flowed")
        self.assertEqual(msg.getparam("spam"), None)
        self.assertEqual(msg.getencoding(), "8bit")

def test_main():
    test_support.run_unittest(MimeToolsTest)

if __name__=="__main__":
    test_main()
