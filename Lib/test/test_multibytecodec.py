#!/usr/bin/env python
#
# test_multibytecodec.py
#   Unit test for multibytecodec itself
#
# $CJKCodecs: test_multibytecodec.py,v 1.8 2004/06/19 06:09:55 perky Exp $

from test import test_support
from test import test_multibytecodec_support
import unittest, StringIO, codecs

class Test_StreamWriter(unittest.TestCase):
    if len(u'\U00012345') == 2: # UCS2
        def test_gb18030(self):
            s= StringIO.StringIO()
            c = codecs.lookup('gb18030')[3](s)
            c.write(u'123')
            self.assertEqual(s.getvalue(), '123')
            c.write(u'\U00012345')
            self.assertEqual(s.getvalue(), '123\x907\x959')
            c.write(u'\U00012345'[0])
            self.assertEqual(s.getvalue(), '123\x907\x959')
            c.write(u'\U00012345'[1] + u'\U00012345' + u'\uac00\u00ac')
            self.assertEqual(s.getvalue(),
                    '123\x907\x959\x907\x959\x907\x959\x827\xcf5\x810\x851')
            c.write(u'\U00012345'[0])
            self.assertEqual(s.getvalue(),
                    '123\x907\x959\x907\x959\x907\x959\x827\xcf5\x810\x851')
            self.assertRaises(UnicodeError, c.reset)
            self.assertEqual(s.getvalue(),
                    '123\x907\x959\x907\x959\x907\x959\x827\xcf5\x810\x851')

        # standard utf-8 codecs has broken StreamReader
        if test_multibytecodec_support.__cjkcodecs__:
            def test_utf_8(self):
                s= StringIO.StringIO()
                c = codecs.lookup('utf-8')[3](s)
                c.write(u'123')
                self.assertEqual(s.getvalue(), '123')
                c.write(u'\U00012345')
                self.assertEqual(s.getvalue(), '123\xf0\x92\x8d\x85')
                c.write(u'\U00012345'[0])
                self.assertEqual(s.getvalue(), '123\xf0\x92\x8d\x85')
                c.write(u'\U00012345'[1] + u'\U00012345' + u'\uac00\u00ac')
                self.assertEqual(s.getvalue(),
                    '123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    '\xea\xb0\x80\xc2\xac')
                c.write(u'\U00012345'[0])
                self.assertEqual(s.getvalue(),
                    '123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    '\xea\xb0\x80\xc2\xac')
                c.reset()
                self.assertEqual(s.getvalue(),
                    '123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    '\xea\xb0\x80\xc2\xac\xed\xa0\x88')
                c.write(u'\U00012345'[1])
                self.assertEqual(s.getvalue(),
                    '123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    '\xea\xb0\x80\xc2\xac\xed\xa0\x88\xed\xbd\x85')

    else: # UCS4
        pass

    def test_nullcoding(self):
        self.assertEqual(''.decode('gb18030'), u'')
        self.assertEqual(unicode('', 'gb18030'), u'')
        self.assertEqual(u''.encode('gb18030'), '')

    def test_str_decode(self):
        self.assertEqual('abcd'.encode('gb18030'), 'abcd')

    def test_streamwriter_strwrite(self):
        s = StringIO.StringIO()
        wr = codecs.getwriter('gb18030')(s)
        wr.write('abcd')
        self.assertEqual(s.getvalue(), 'abcd')

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(Test_StreamWriter))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
