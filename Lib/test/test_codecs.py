import test_support,unittest
import codecs
import StringIO

class UTF16Test(unittest.TestCase):

    spamle = '\xff\xfes\x00p\x00a\x00m\x00s\x00p\x00a\x00m\x00'
    spambe = '\xfe\xff\x00s\x00p\x00a\x00m\x00s\x00p\x00a\x00m'

    def test_only_one_bom(self):
        _,_,reader,writer = codecs.lookup("utf-16")
        # encode some stream
        s = StringIO.StringIO()
        f = writer(s)
        f.write(u"spam")
        f.write(u"spam")
        d = s.getvalue()
        # check whether there is exactly one BOM in it
        self.assert_(d == self.spamle or d == self.spambe)
        # try to read it back
        s = StringIO.StringIO(d)
        f = reader(s)
        self.assertEquals(f.read(), u"spamspam")


def test_main():
    test_support.run_unittest(UTF16Test)


if __name__ == "__main__":
    test_main()
