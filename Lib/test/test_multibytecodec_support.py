#!/usr/bin/env python
#
# test_multibytecodec_support.py
#   Common Unittest Routines for CJK codecs
#
# $CJKCodecs: test_multibytecodec_support.py,v 1.6 2004/06/19 06:09:55 perky Exp $

import sys, codecs, os.path
import unittest
from test import test_support
from StringIO import StringIO

__cjkcodecs__ = 0 # define this as 0 for python

class TestBase:
    encoding        = ''   # codec name
    codec           = None # codec tuple (with 4 elements)
    tstring         = ''   # string to test StreamReader

    codectests      = None # must set. codec test tuple
    roundtriptest   = 1    # set if roundtrip is possible with unicode
    has_iso10646    = 0    # set if this encoding contains whole iso10646 map
    xmlcharnametest = None # string to test xmlcharrefreplace

    def setUp(self):
        if self.codec is None:
            self.codec = codecs.lookup(self.encoding)
        self.encode, self.decode, self.reader, self.writer = self.codec

    def test_chunkcoding(self):
        for native, utf8 in zip(*[StringIO(f).readlines()
                                  for f in self.tstring]):
            u = self.decode(native)[0]
            self.assertEqual(u, utf8.decode('utf-8'))
            if self.roundtriptest:
                self.assertEqual(native, self.encode(u)[0])

    def test_errorhandle(self):
        for source, scheme, expected in self.codectests:
            if type(source) == type(''):
                func = self.decode
            else:
                func = self.encode
            if expected:
                result = func(source, scheme)[0]
                self.assertEqual(result, expected)
            else:
                self.assertRaises(UnicodeError, func, source, scheme)

    if sys.hexversion >= 0x02030000:
        def test_xmlcharrefreplace(self):
            if self.has_iso10646:
                return

            s = u"\u0b13\u0b23\u0b60 nd eggs"
            self.assertEqual(
                self.encode(s, "xmlcharrefreplace")[0],
                "&#2835;&#2851;&#2912; nd eggs"
            )

        def test_customreplace(self):
            if self.has_iso10646:
                return

            import htmlentitydefs

            names = {}
            for (key, value) in htmlentitydefs.entitydefs.items():
                if len(value)==1:
                    names[value.decode('latin-1')] = self.decode(key)[0]
                else:
                    names[unichr(int(value[2:-1]))] = self.decode(key)[0]

            def xmlcharnamereplace(exc):
                if not isinstance(exc, UnicodeEncodeError):
                    raise TypeError("don't know how to handle %r" % exc)
                l = []
                for c in exc.object[exc.start:exc.end]:
                    try:
                        l.append(u"&%s;" % names[c])
                    except KeyError:
                        l.append(u"&#%d;" % ord(c))
                return (u"".join(l), exc.end)

            codecs.register_error(
                "test.xmlcharnamereplace", xmlcharnamereplace)

            if self.xmlcharnametest:
                sin, sout = self.xmlcharnametest
            else:
                sin = u"\xab\u211c\xbb = \u2329\u1234\u232a"
                sout = "&laquo;&real;&raquo; = &lang;&#4660;&rang;"
            self.assertEqual(self.encode(sin,
                                        "test.xmlcharnamereplace")[0], sout)

    def test_streamreader(self):
        UTF8Writer = codecs.getwriter('utf-8')
        for name in ["read", "readline", "readlines"]:
            for sizehint in [None, -1] + range(1, 33) + \
                            [64, 128, 256, 512, 1024]:
                istream = self.reader(StringIO(self.tstring[0]))
                ostream = UTF8Writer(StringIO())
                func = getattr(istream, name)
                while 1:
                    data = func(sizehint)
                    if not data:
                        break
                    if name == "readlines":
                        ostream.writelines(data)
                    else:
                        ostream.write(data)

                self.assertEqual(ostream.getvalue(), self.tstring[1])

    def test_streamwriter(self):
        if __cjkcodecs__:
            readfuncs = ('read', 'readline', 'readlines')
        else:
            # standard utf8 codec has broken readline and readlines.
            readfuncs = ('read',)
        UTF8Reader = codecs.getreader('utf-8')
        for name in readfuncs:
            for sizehint in [None] + range(1, 33) + \
                            [64, 128, 256, 512, 1024]:
                istream = UTF8Reader(StringIO(self.tstring[1]))
                ostream = self.writer(StringIO())
                func = getattr(istream, name)
                while 1:
                    if sizehint is not None:
                        data = func(sizehint)
                    else:
                        data = func()

                    if not data:
                        break
                    if name == "readlines":
                        ostream.writelines(data)
                    else:
                        ostream.write(data)

                self.assertEqual(ostream.getvalue(), self.tstring[0])

if len(u'\U00012345') == 2: # ucs2 build
    _unichr = unichr
    def unichr(v):
        if v >= 0x10000:
            return _unichr(0xd800 + ((v - 0x10000) >> 10)) + \
                   _unichr(0xdc00 + ((v - 0x10000) & 0x3ff))
        else:
            return _unichr(v)
    _ord = ord
    def ord(c):
        if len(c) == 2:
            return 0x10000 + ((_ord(c[0]) - 0xd800) << 10) + \
                          (ord(c[1]) - 0xdc00)
        else:
            return _ord(c)

class TestBase_Mapping(unittest.TestCase):
    pass_enctest = []
    pass_dectest = []
    supmaps = []

    def __init__(self, *args, **kw):
        unittest.TestCase.__init__(self, *args, **kw)
        if not os.path.exists(self.mapfilename):
            raise test_support.TestSkipped('%s not found, download from %s' %
                    (self.mapfilename, self.mapfileurl))

    def test_mapping_file(self):
        unichrs = lambda s: u''.join(map(unichr, map(eval, s.split('+'))))
        urt_wa = {}

        for line in open(self.mapfilename):
            if not line:
                break
            data = line.split('#')[0].strip().split()
            if len(data) != 2:
                continue

            csetval = eval(data[0])
            if csetval <= 0x7F:
                csetch = chr(csetval & 0xff)
            elif csetval >= 0x1000000:
                csetch = chr(csetval >> 24) + chr((csetval >> 16) & 0xff) + \
                         chr((csetval >> 8) & 0xff) + chr(csetval & 0xff)
            elif csetval >= 0x10000:
                csetch = chr(csetval >> 16) + \
                         chr((csetval >> 8) & 0xff) + chr(csetval & 0xff)
            elif csetval >= 0x100:
                csetch = chr(csetval >> 8) + chr(csetval & 0xff)
            else:
                continue

            unich = unichrs(data[1])
            if ord(unich) == 0xfffd or urt_wa.has_key(unich):
                continue
            urt_wa[unich] = csetch

            self._testpoint(csetch, unich)

    def test_mapping_supplemental(self):
        for mapping in self.supmaps:
            self._testpoint(*mapping)

    def _testpoint(self, csetch, unich):
        if (csetch, unich) not in self.pass_enctest:
            self.assertEqual(unich.encode(self.encoding), csetch)
        if (csetch, unich) not in self.pass_dectest:
            self.assertEqual(unicode(csetch, self.encoding), unich)

def load_teststring(encoding):
    if __cjkcodecs__:
        etxt = open(os.path.join('sampletexts', encoding) + '.txt').read()
        utxt = open(os.path.join('sampletexts', encoding) + '.utf8').read()
        return (etxt, utxt)
    else:
        from test import cjkencodings_test
        return cjkencodings_test.teststring[encoding]

def register_skip_expected(*cases):
    for case in cases: # len(cases) must be 1 at least.
        for path in [os.path.curdir, os.path.pardir]:
            fn = os.path.join(path, case.mapfilename)
            if os.path.exists(fn):
                case.mapfilename = fn
                break
        else:
            sys.modules[case.__module__].skip_expected = True
            break
    else:
        sys.modules[case.__module__].skip_expected = False
