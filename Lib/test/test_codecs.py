from test import test_support
import unittest
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

class EscapeDecodeTest(unittest.TestCase):
    def test_empty_escape_decode(self):
        self.assertEquals(codecs.escape_decode(""), ("", 0))

class RecodingTest(unittest.TestCase):
    def test_recoding(self):
        f = StringIO.StringIO()
        f2 = codecs.EncodedFile(f, "unicode_internal", "utf-8")
        f2.write(u"a")
        f2.close()
        # Python used to crash on this at exit because of a refcount
        # bug in _codecsmodule.c

# From RFC 3492
punycode_testcases = [
    # A Arabic (Egyptian):
    (u"\u0644\u064A\u0647\u0645\u0627\u0628\u062A\u0643\u0644"
     u"\u0645\u0648\u0634\u0639\u0631\u0628\u064A\u061F",
     "egbpdaj6bu4bxfgehfvwxn"),
    # B Chinese (simplified):
    (u"\u4ED6\u4EEC\u4E3A\u4EC0\u4E48\u4E0D\u8BF4\u4E2D\u6587",
     "ihqwcrb4cv8a8dqg056pqjye"),
    # C Chinese (traditional):
    (u"\u4ED6\u5011\u7232\u4EC0\u9EBD\u4E0D\u8AAA\u4E2D\u6587",
     "ihqwctvzc91f659drss3x8bo0yb"),
    # D Czech: Pro<ccaron>prost<ecaron>nemluv<iacute><ccaron>esky
    (u"\u0050\u0072\u006F\u010D\u0070\u0072\u006F\u0073\u0074"
     u"\u011B\u006E\u0065\u006D\u006C\u0075\u0076\u00ED\u010D"
     u"\u0065\u0073\u006B\u0079",
     "Proprostnemluvesky-uyb24dma41a"),
    # E Hebrew:
    (u"\u05DC\u05DE\u05D4\u05D4\u05DD\u05E4\u05E9\u05D5\u05D8"
     u"\u05DC\u05D0\u05DE\u05D3\u05D1\u05E8\u05D9\u05DD\u05E2"
     u"\u05D1\u05E8\u05D9\u05EA",
     "4dbcagdahymbxekheh6e0a7fei0b"),
    # F Hindi (Devanagari):
    (u"\u092F\u0939\u0932\u094B\u0917\u0939\u093F\u0928\u094D"
    u"\u0926\u0940\u0915\u094D\u092F\u094B\u0902\u0928\u0939"
    u"\u0940\u0902\u092C\u094B\u0932\u0938\u0915\u0924\u0947"
    u"\u0939\u0948\u0902",
    "i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd"),

    #(G) Japanese (kanji and hiragana):
    (u"\u306A\u305C\u307F\u3093\u306A\u65E5\u672C\u8A9E\u3092"
    u"\u8A71\u3057\u3066\u304F\u308C\u306A\u3044\u306E\u304B",
     "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa"),

    # (H) Korean (Hangul syllables):
    (u"\uC138\uACC4\uC758\uBAA8\uB4E0\uC0AC\uB78C\uB4E4\uC774"
     u"\uD55C\uAD6D\uC5B4\uB97C\uC774\uD574\uD55C\uB2E4\uBA74"
     u"\uC5BC\uB9C8\uB098\uC88B\uC744\uAE4C",
     "989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5j"
     "psd879ccm6fea98c"),

    # (I) Russian (Cyrillic):
    (u"\u043F\u043E\u0447\u0435\u043C\u0443\u0436\u0435\u043E"
     u"\u043D\u0438\u043D\u0435\u0433\u043E\u0432\u043E\u0440"
     u"\u044F\u0442\u043F\u043E\u0440\u0443\u0441\u0441\u043A"
     u"\u0438",
     "b1abfaaepdrnnbgefbaDotcwatmq2g4l"),

    # (J) Spanish: Porqu<eacute>nopuedensimplementehablarenEspa<ntilde>ol
    (u"\u0050\u006F\u0072\u0071\u0075\u00E9\u006E\u006F\u0070"
     u"\u0075\u0065\u0064\u0065\u006E\u0073\u0069\u006D\u0070"
     u"\u006C\u0065\u006D\u0065\u006E\u0074\u0065\u0068\u0061"
     u"\u0062\u006C\u0061\u0072\u0065\u006E\u0045\u0073\u0070"
     u"\u0061\u00F1\u006F\u006C",
     "PorqunopuedensimplementehablarenEspaol-fmd56a"),

    # (K) Vietnamese:
    #  T<adotbelow>isaoh<odotbelow>kh<ocirc>ngth<ecirchookabove>ch\
    #   <ihookabove>n<oacute>iti<ecircacute>ngVi<ecircdotbelow>t
    (u"\u0054\u1EA1\u0069\u0073\u0061\u006F\u0068\u1ECD\u006B"
     u"\u0068\u00F4\u006E\u0067\u0074\u0068\u1EC3\u0063\u0068"
     u"\u1EC9\u006E\u00F3\u0069\u0074\u0069\u1EBF\u006E\u0067"
     u"\u0056\u0069\u1EC7\u0074",
     "TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g"),


    #(L) 3<nen>B<gumi><kinpachi><sensei>
    (u"\u0033\u5E74\u0042\u7D44\u91D1\u516B\u5148\u751F",
     "3B-ww4c5e180e575a65lsy2b"),

    # (M) <amuro><namie>-with-SUPER-MONKEYS
    (u"\u5B89\u5BA4\u5948\u7F8E\u6075\u002D\u0077\u0069\u0074"
     u"\u0068\u002D\u0053\u0055\u0050\u0045\u0052\u002D\u004D"
     u"\u004F\u004E\u004B\u0045\u0059\u0053",
     "-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n"),

    # (N) Hello-Another-Way-<sorezore><no><basho>
    (u"\u0048\u0065\u006C\u006C\u006F\u002D\u0041\u006E\u006F"
     u"\u0074\u0068\u0065\u0072\u002D\u0057\u0061\u0079\u002D"
     u"\u305D\u308C\u305E\u308C\u306E\u5834\u6240",
     "Hello-Another-Way--fc4qua05auwb3674vfr0b"),

    # (O) <hitotsu><yane><no><shita>2
    (u"\u3072\u3068\u3064\u5C4B\u6839\u306E\u4E0B\u0032",
     "2-u9tlzr9756bt3uc0v"),

    # (P) Maji<de>Koi<suru>5<byou><mae>
    (u"\u004D\u0061\u006A\u0069\u3067\u004B\u006F\u0069\u3059"
     u"\u308B\u0035\u79D2\u524D",
     "MajiKoi5-783gue6qz075azm5e"),

     # (Q) <pafii>de<runba>
    (u"\u30D1\u30D5\u30A3\u30FC\u0064\u0065\u30EB\u30F3\u30D0",
     "de-jg4avhby1noc0d"),

    # (R) <sono><supiido><de>
    (u"\u305D\u306E\u30B9\u30D4\u30FC\u30C9\u3067",
     "d9juau41awczczp"),

    # (S) -> $1.00 <-
    (u"\u002D\u003E\u0020\u0024\u0031\u002E\u0030\u0030\u0020"
     u"\u003C\u002D",
     "-> $1.00 <--")
    ]

for i in punycode_testcases:
    if len(i)!=2:
        print repr(i)

class PunycodeTest(unittest.TestCase):
    def test_encode(self):
        for uni, puny in punycode_testcases:
            # Need to convert both strings to lower case, since
            # some of the extended encodings use upper case, but our
            # code produces only lower case. Converting just puny to
            # lower is also insufficient, since some of the input characters
            # are upper case.
            self.assertEquals(uni.encode("punycode").lower(), puny.lower())

    def test_decode(self):
        for uni, puny in punycode_testcases:
            self.assertEquals(uni, puny.decode("punycode"))

# From http://www.gnu.org/software/libidn/draft-josefsson-idn-test-vectors.html
nameprep_tests = [
    # 3.1 Map to nothing.
    ('foo\xc2\xad\xcd\x8f\xe1\xa0\x86\xe1\xa0\x8bbar'
     '\xe2\x80\x8b\xe2\x81\xa0baz\xef\xb8\x80\xef\xb8\x88\xef'
     '\xb8\x8f\xef\xbb\xbf',
     'foobarbaz'),
    # 3.2 Case folding ASCII U+0043 U+0041 U+0046 U+0045.
    ('CAFE',
     'cafe'),
    # 3.3 Case folding 8bit U+00DF (german sharp s).
    # The original test case is bogus; it says \xc3\xdf
    ('\xc3\x9f',
     'ss'),
    # 3.4 Case folding U+0130 (turkish capital I with dot).
    ('\xc4\xb0',
     'i\xcc\x87'),
    # 3.5 Case folding multibyte U+0143 U+037A.
    ('\xc5\x83\xcd\xba',
     '\xc5\x84 \xce\xb9'),
    # 3.6 Case folding U+2121 U+33C6 U+1D7BB.
    # XXX: skip this as it fails in UCS-2 mode
    #('\xe2\x84\xa1\xe3\x8f\x86\xf0\x9d\x9e\xbb',
    # 'telc\xe2\x88\x95kg\xcf\x83'),
    (None, None),
    # 3.7 Normalization of U+006a U+030c U+00A0 U+00AA.
    ('j\xcc\x8c\xc2\xa0\xc2\xaa',
     '\xc7\xb0 a'),
    # 3.8 Case folding U+1FB7 and normalization.
    ('\xe1\xbe\xb7',
     '\xe1\xbe\xb6\xce\xb9'),
    # 3.9 Self-reverting case folding U+01F0 and normalization.
    # The original test case is bogus, it says `\xc7\xf0'
    ('\xc7\xb0',
     '\xc7\xb0'),
    # 3.10 Self-reverting case folding U+0390 and normalization.
    ('\xce\x90',
     '\xce\x90'),
    # 3.11 Self-reverting case folding U+03B0 and normalization.
    ('\xce\xb0',
     '\xce\xb0'),
    # 3.12 Self-reverting case folding U+1E96 and normalization.
    ('\xe1\xba\x96',
     '\xe1\xba\x96'),
    # 3.13 Self-reverting case folding U+1F56 and normalization.
    ('\xe1\xbd\x96',
     '\xe1\xbd\x96'),
    # 3.14 ASCII space character U+0020.
    (' ',
     ' '),
    # 3.15 Non-ASCII 8bit space character U+00A0.
    ('\xc2\xa0',
     ' '),
    # 3.16 Non-ASCII multibyte space character U+1680.
    ('\xe1\x9a\x80',
     None),
    # 3.17 Non-ASCII multibyte space character U+2000.
    ('\xe2\x80\x80',
     ' '),
    # 3.18 Zero Width Space U+200b.
    ('\xe2\x80\x8b',
     ''),
    # 3.19 Non-ASCII multibyte space character U+3000.
    ('\xe3\x80\x80',
     ' '),
    # 3.20 ASCII control characters U+0010 U+007F.
    ('\x10\x7f',
     '\x10\x7f'),
    # 3.21 Non-ASCII 8bit control character U+0085.
    ('\xc2\x85',
     None),
    # 3.22 Non-ASCII multibyte control character U+180E.
    ('\xe1\xa0\x8e',
     None),
    # 3.23 Zero Width No-Break Space U+FEFF.
    ('\xef\xbb\xbf',
     ''),
    # 3.24 Non-ASCII control character U+1D175.
    ('\xf0\x9d\x85\xb5',
     None),
    # 3.25 Plane 0 private use character U+F123.
    ('\xef\x84\xa3',
     None),
    # 3.26 Plane 15 private use character U+F1234.
    ('\xf3\xb1\x88\xb4',
     None),
    # 3.27 Plane 16 private use character U+10F234.
    ('\xf4\x8f\x88\xb4',
     None),
    # 3.28 Non-character code point U+8FFFE.
    ('\xf2\x8f\xbf\xbe',
     None),
    # 3.29 Non-character code point U+10FFFF.
    ('\xf4\x8f\xbf\xbf',
     None),
    # 3.30 Surrogate code U+DF42.
    ('\xed\xbd\x82',
     None),
    # 3.31 Non-plain text character U+FFFD.
    ('\xef\xbf\xbd',
     None),
    # 3.32 Ideographic description character U+2FF5.
    ('\xe2\xbf\xb5',
     None),
    # 3.33 Display property character U+0341.
    ('\xcd\x81',
     '\xcc\x81'),
    # 3.34 Left-to-right mark U+200E.
    ('\xe2\x80\x8e',
     None),
    # 3.35 Deprecated U+202A.
    ('\xe2\x80\xaa',
     None),
    # 3.36 Language tagging character U+E0001.
    ('\xf3\xa0\x80\x81',
     None),
    # 3.37 Language tagging character U+E0042.
    ('\xf3\xa0\x81\x82',
     None),
    # 3.38 Bidi: RandALCat character U+05BE and LCat characters.
    ('foo\xd6\xbebar',
     None),
    # 3.39 Bidi: RandALCat character U+FD50 and LCat characters.
    ('foo\xef\xb5\x90bar',
     None),
    # 3.40 Bidi: RandALCat character U+FB38 and LCat characters.
    ('foo\xef\xb9\xb6bar',
     'foo \xd9\x8ebar'),
    # 3.41 Bidi: RandALCat without trailing RandALCat U+0627 U+0031.
    ('\xd8\xa71',
     None),
    # 3.42 Bidi: RandALCat character U+0627 U+0031 U+0628.
    ('\xd8\xa71\xd8\xa8',
     '\xd8\xa71\xd8\xa8'),
    # 3.43 Unassigned code point U+E0002.
    # Skip this test as we allow unassigned
    #('\xf3\xa0\x80\x82',
    # None),
    (None, None),
    # 3.44 Larger test (shrinking).
    # Original test case reads \xc3\xdf
    ('X\xc2\xad\xc3\x9f\xc4\xb0\xe2\x84\xa1j\xcc\x8c\xc2\xa0\xc2'
     '\xaa\xce\xb0\xe2\x80\x80',
     'xssi\xcc\x87tel\xc7\xb0 a\xce\xb0 '),
    # 3.45 Larger test (expanding).
    # Original test case reads \xc3\x9f
    ('X\xc3\x9f\xe3\x8c\x96\xc4\xb0\xe2\x84\xa1\xe2\x92\x9f\xe3\x8c'
     '\x80',
     'xss\xe3\x82\xad\xe3\x83\xad\xe3\x83\xa1\xe3\x83\xbc\xe3'
     '\x83\x88\xe3\x83\xabi\xcc\x87tel\x28d\x29\xe3\x82'
     '\xa2\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x88')
    ]


class NameprepTest(unittest.TestCase):
    def test_nameprep(self):
        from encodings.idna import nameprep
        for pos, (orig, prepped) in enumerate(nameprep_tests):
            if orig is None:
                # Skipped
                continue
            # The Unicode strings are given in UTF-8
            orig = unicode(orig, "utf-8")
            if prepped is None:
                # Input contains prohibited characters
                self.assertRaises(UnicodeError, nameprep, orig)
            else:
                prepped = unicode(prepped, "utf-8")
                try:
                    self.assertEquals(nameprep(orig), prepped)
                except Exception,e:
                    raise test_support.TestFailed("Test 3.%d: %s" % (pos+1, str(e)))

def test_main():
    test_support.run_unittest(
        UTF16Test,
        EscapeDecodeTest,
        RecodingTest,
        PunycodeTest,
        NameprepTest
    )


if __name__ == "__main__":
    test_main()
