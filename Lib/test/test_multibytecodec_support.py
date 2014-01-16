# test_multibytecodec_support.py
#   Common Unittest Routines for CJK codecs
#

import codecs
import os
import re
import sys
import unittest
from httplib import HTTPException
from test import test_support
from StringIO import StringIO

class TestBase:
    encoding        = ''   # codec name
    codec           = None # codec tuple (with 4 elements)
    tstring         = ''   # string to test StreamReader

    codectests      = None # must set. codec test tuple
    roundtriptest   = 1    # set if roundtrip is possible with unicode
    has_iso10646    = 0    # set if this encoding contains whole iso10646 map
    xmlcharnametest = None # string to test xmlcharrefreplace
    unmappedunicode = u'\udeee' # a unicode codepoint that is not mapped.

    def setUp(self):
        if self.codec is None:
            self.codec = codecs.lookup(self.encoding)
        self.encode = self.codec.encode
        self.decode = self.codec.decode
        self.reader = self.codec.streamreader
        self.writer = self.codec.streamwriter
        self.incrementalencoder = self.codec.incrementalencoder
        self.incrementaldecoder = self.codec.incrementaldecoder

    def test_chunkcoding(self):
        for native, utf8 in zip(*[StringIO(f).readlines()
                                  for f in self.tstring]):
            u = self.decode(native)[0]
            self.assertEqual(u, utf8.decode('utf-8'))
            if self.roundtriptest:
                self.assertEqual(native, self.encode(u)[0])

    def test_errorhandle(self):
        for source, scheme, expected in self.codectests:
            if isinstance(source, bytes):
                func = self.decode
            else:
                func = self.encode
            if expected:
                result = func(source, scheme)[0]
                if func is self.decode:
                    self.assertTrue(type(result) is unicode, type(result))
                    self.assertEqual(result, expected,
                                     '%r.decode(%r, %r)=%r != %r'
                                     % (source, self.encoding, scheme, result,
                                        expected))
                else:
                    self.assertTrue(type(result) is bytes, type(result))
                    self.assertEqual(result, expected,
                                     '%r.encode(%r, %r)=%r != %r'
                                     % (source, self.encoding, scheme, result,
                                        expected))
            else:
                self.assertRaises(UnicodeError, func, source, scheme)

    def test_xmlcharrefreplace(self):
        if self.has_iso10646:
            self.skipTest('encoding contains full ISO 10646 map')

        s = u"\u0b13\u0b23\u0b60 nd eggs"
        self.assertEqual(
            self.encode(s, "xmlcharrefreplace")[0],
            "&#2835;&#2851;&#2912; nd eggs"
        )

    def test_customreplace_encode(self):
        if self.has_iso10646:
            self.skipTest('encoding contains full ISO 10646 map')

        from htmlentitydefs import codepoint2name

        def xmlcharnamereplace(exc):
            if not isinstance(exc, UnicodeEncodeError):
                raise TypeError("don't know how to handle %r" % exc)
            l = []
            for c in exc.object[exc.start:exc.end]:
                if ord(c) in codepoint2name:
                    l.append(u"&%s;" % codepoint2name[ord(c)])
                else:
                    l.append(u"&#%d;" % ord(c))
            return (u"".join(l), exc.end)

        codecs.register_error("test.xmlcharnamereplace", xmlcharnamereplace)

        if self.xmlcharnametest:
            sin, sout = self.xmlcharnametest
        else:
            sin = u"\xab\u211c\xbb = \u2329\u1234\u232a"
            sout = "&laquo;&real;&raquo; = &lang;&#4660;&rang;"
        self.assertEqual(self.encode(sin,
                                    "test.xmlcharnamereplace")[0], sout)

    def test_callback_wrong_objects(self):
        def myreplace(exc):
            return (ret, exc.end)
        codecs.register_error("test.cjktest", myreplace)

        for ret in ([1, 2, 3], [], None, object(), 'string', ''):
            self.assertRaises(TypeError, self.encode, self.unmappedunicode,
                              'test.cjktest')

    def test_callback_long_index(self):
        def myreplace(exc):
            return (u'x', long(exc.end))
        codecs.register_error("test.cjktest", myreplace)
        self.assertEqual(self.encode(u'abcd' + self.unmappedunicode + u'efgh',
                                     'test.cjktest'), ('abcdxefgh', 9))

        def myreplace(exc):
            return (u'x', sys.maxint + 1)
        codecs.register_error("test.cjktest", myreplace)
        self.assertRaises(IndexError, self.encode, self.unmappedunicode,
                          'test.cjktest')

    def test_callback_None_index(self):
        def myreplace(exc):
            return (u'x', None)
        codecs.register_error("test.cjktest", myreplace)
        self.assertRaises(TypeError, self.encode, self.unmappedunicode,
                          'test.cjktest')

    def test_callback_backward_index(self):
        def myreplace(exc):
            if myreplace.limit > 0:
                myreplace.limit -= 1
                return (u'REPLACED', 0)
            else:
                return (u'TERMINAL', exc.end)
        myreplace.limit = 3
        codecs.register_error("test.cjktest", myreplace)
        self.assertEqual(self.encode(u'abcd' + self.unmappedunicode + u'efgh',
                                     'test.cjktest'),
                ('abcdREPLACEDabcdREPLACEDabcdREPLACEDabcdTERMINALefgh', 9))

    def test_callback_forward_index(self):
        def myreplace(exc):
            return (u'REPLACED', exc.end + 2)
        codecs.register_error("test.cjktest", myreplace)
        self.assertEqual(self.encode(u'abcd' + self.unmappedunicode + u'efgh',
                                     'test.cjktest'), ('abcdREPLACEDgh', 9))

    def test_callback_index_outofbound(self):
        def myreplace(exc):
            return (u'TERM', 100)
        codecs.register_error("test.cjktest", myreplace)
        self.assertRaises(IndexError, self.encode, self.unmappedunicode,
                          'test.cjktest')

    def test_incrementalencoder(self):
        UTF8Reader = codecs.getreader('utf-8')
        for sizehint in [None] + range(1, 33) + \
                        [64, 128, 256, 512, 1024]:
            istream = UTF8Reader(StringIO(self.tstring[1]))
            ostream = StringIO()
            encoder = self.incrementalencoder()
            while 1:
                if sizehint is not None:
                    data = istream.read(sizehint)
                else:
                    data = istream.read()

                if not data:
                    break
                e = encoder.encode(data)
                ostream.write(e)

            self.assertEqual(ostream.getvalue(), self.tstring[0])

    def test_incrementaldecoder(self):
        UTF8Writer = codecs.getwriter('utf-8')
        for sizehint in [None, -1] + range(1, 33) + \
                        [64, 128, 256, 512, 1024]:
            istream = StringIO(self.tstring[0])
            ostream = UTF8Writer(StringIO())
            decoder = self.incrementaldecoder()
            while 1:
                data = istream.read(sizehint)
                if not data:
                    break
                else:
                    u = decoder.decode(data)
                    ostream.write(u)

            self.assertEqual(ostream.getvalue(), self.tstring[1])

    def test_incrementalencoder_error_callback(self):
        inv = self.unmappedunicode

        e = self.incrementalencoder()
        self.assertRaises(UnicodeEncodeError, e.encode, inv, True)

        e.errors = 'ignore'
        self.assertEqual(e.encode(inv, True), '')

        e.reset()
        def tempreplace(exc):
            return (u'called', exc.end)
        codecs.register_error('test.incremental_error_callback', tempreplace)
        e.errors = 'test.incremental_error_callback'
        self.assertEqual(e.encode(inv, True), 'called')

        # again
        e.errors = 'ignore'
        self.assertEqual(e.encode(inv, True), '')

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
        readfuncs = ('read', 'readline', 'readlines')
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

class TestBase_Mapping(unittest.TestCase):
    pass_enctest = []
    pass_dectest = []
    supmaps = []
    codectests = []

    def __init__(self, *args, **kw):
        unittest.TestCase.__init__(self, *args, **kw)
        try:
            self.open_mapping_file().close() # test it to report the error early
        except (IOError, HTTPException):
            self.skipTest("Could not retrieve "+self.mapfileurl)

    def open_mapping_file(self):
        return test_support.open_urlresource(self.mapfileurl)

    def test_mapping_file(self):
        if self.mapfileurl.endswith('.xml'):
            self._test_mapping_file_ucm()
        else:
            self._test_mapping_file_plain()

    def _test_mapping_file_plain(self):
        _unichr = lambda c: eval("u'\\U%08x'" % int(c, 16))
        unichrs = lambda s: u''.join(_unichr(c) for c in s.split('+'))
        urt_wa = {}

        with self.open_mapping_file() as f:
            for line in f:
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
                if unich == u'\ufffd' or unich in urt_wa:
                    continue
                urt_wa[unich] = csetch

                self._testpoint(csetch, unich)

    def _test_mapping_file_ucm(self):
        with self.open_mapping_file() as f:
            ucmdata = f.read()
        uc = re.findall('<a u="([A-F0-9]{4})" b="([0-9A-F ]+)"/>', ucmdata)
        for uni, coded in uc:
            unich = unichr(int(uni, 16))
            codech = ''.join(chr(int(c, 16)) for c in coded.split())
            self._testpoint(codech, unich)

    def test_mapping_supplemental(self):
        for mapping in self.supmaps:
            self._testpoint(*mapping)

    def _testpoint(self, csetch, unich):
        if (csetch, unich) not in self.pass_enctest:
            try:
                self.assertEqual(unich.encode(self.encoding), csetch)
            except UnicodeError, exc:
                self.fail('Encoding failed while testing %s -> %s: %s' % (
                            repr(unich), repr(csetch), exc.reason))
        if (csetch, unich) not in self.pass_dectest:
            try:
                self.assertEqual(csetch.decode(self.encoding), unich)
            except UnicodeError, exc:
                self.fail('Decoding failed while testing %s -> %s: %s' % (
                            repr(csetch), repr(unich), exc.reason))

    def test_errorhandle(self):
        for source, scheme, expected in self.codectests:
            if isinstance(source, bytes):
                func = source.decode
            else:
                func = source.encode
            if expected:
                if isinstance(source, bytes):
                    result = func(self.encoding, scheme)
                    self.assertTrue(type(result) is unicode, type(result))
                    self.assertEqual(result, expected,
                                     '%r.decode(%r, %r)=%r != %r'
                                     % (source, self.encoding, scheme, result,
                                        expected))
                else:
                    result = func(self.encoding, scheme)
                    self.assertTrue(type(result) is bytes, type(result))
                    self.assertEqual(result, expected,
                                     '%r.encode(%r, %r)=%r != %r'
                                     % (source, self.encoding, scheme, result,
                                        expected))
            else:
                self.assertRaises(UnicodeError, func, self.encoding, scheme)

def load_teststring(name):
    dir = os.path.join(os.path.dirname(__file__), 'cjkencodings')
    with open(os.path.join(dir, name + '.txt'), 'rb') as f:
        encoded = f.read()
    with open(os.path.join(dir, name + '-utf8.txt'), 'rb') as f:
        utf8 = f.read()
    return encoded, utf8
