import unittest
from test import test_support

import string, StringIO, mimetools, sets

msgtext1 = mimetools.Message(StringIO.StringIO(
"""Content-Type: text/plain; charset=iso-8859-1; format=flowed
Content-Transfer-Encoding: 8bit

Foo!
"""))

class MimeToolsTest(unittest.TestCase):

    def test_decodeencode(self):
        start = string.ascii_letters + "=" + string.digits + "\n"
        for enc in ['7bit','8bit','base64','quoted-printable',
                    'uuencode', 'x-uuencode', 'uue', 'x-uue']:
            i = StringIO.StringIO(start)
            o = StringIO.StringIO()
            mimetools.encode(i, o, enc)
            i = StringIO.StringIO(o.getvalue())
            o = StringIO.StringIO()
            mimetools.decode(i, o, enc)
            self.assertEqual(o.getvalue(), start)

    def test_boundary(self):
        s = sets.Set([""])
        for i in xrange(100):
            nb = mimetools.choose_boundary()
            self.assert_(nb not in s)
            s.add(nb)

    def test_message(self):
        msg = mimetools.Message(StringIO.StringIO(msgtext1))
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
