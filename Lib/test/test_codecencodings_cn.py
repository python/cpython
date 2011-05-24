#!/usr/bin/env python
#
# test_codecencodings_cn.py
#   Codec encoding tests for PRC encodings.
#

from test import test_support
from test import test_multibytecodec_support
import unittest

class Test_GB2312(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb2312'
    tstring = test_multibytecodec_support.load_teststring('gb2312')
    codectests = (
        # invalid bytes
        ("abc\x81\x81\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x81\x81\xc1\xc4", "replace", u"abc\ufffd\u804a"),
        ("abc\x81\x81\xc1\xc4\xc8", "replace", u"abc\ufffd\u804a\ufffd"),
        ("abc\x81\x81\xc1\xc4", "ignore",  u"abc\u804a"),
        ("\xc1\x64", "strict", None),
    )

class Test_GBK(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gbk'
    tstring = test_multibytecodec_support.load_teststring('gbk')
    codectests = (
        # invalid bytes
        ("abc\x80\x80\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x80\x80\xc1\xc4", "replace", u"abc\ufffd\u804a"),
        ("abc\x80\x80\xc1\xc4\xc8", "replace", u"abc\ufffd\u804a\ufffd"),
        ("abc\x80\x80\xc1\xc4", "ignore",  u"abc\u804a"),
        ("\x83\x34\x83\x31", "strict", None),
        (u"\u30fb", "strict", None),
    )

class Test_GB18030(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb18030'
    tstring = test_multibytecodec_support.load_teststring('gb18030')
    codectests = (
        # invalid bytes
        ("abc\x80\x80\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x80\x80\xc1\xc4", "replace", u"abc\ufffd\u804a"),
        ("abc\x80\x80\xc1\xc4\xc8", "replace", u"abc\ufffd\u804a\ufffd"),
        ("abc\x80\x80\xc1\xc4", "ignore",  u"abc\u804a"),
        ("abc\x84\x39\x84\x39\xc1\xc4", "replace", u"abc\ufffd\u804a"),
        (u"\u30fb", "strict", "\x819\xa79"),
    )
    has_iso10646 = True

class Test_HZ(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'hz'
    tstring = test_multibytecodec_support.load_teststring('hz')
    codectests = (
        # test '~\n' (3 lines)
        (b'This sentence is in ASCII.\n'
         b'The next sentence is in GB.~{<:Ky2;S{#,~}~\n'
         b'~{NpJ)l6HK!#~}Bye.\n',
         'strict',
         u'This sentence is in ASCII.\n'
         u'The next sentence is in GB.'
         u'\u5df1\u6240\u4e0d\u6b32\uff0c\u52ff\u65bd\u65bc\u4eba\u3002'
         u'Bye.\n'),
        # test '~\n' (4 lines)
        (b'This sentence is in ASCII.\n'
         b'The next sentence is in GB.~\n'
         b'~{<:Ky2;S{#,NpJ)l6HK!#~}~\n'
         b'Bye.\n',
         'strict',
         u'This sentence is in ASCII.\n'
         u'The next sentence is in GB.'
         u'\u5df1\u6240\u4e0d\u6b32\uff0c\u52ff\u65bd\u65bc\u4eba\u3002'
         u'Bye.\n'),
        # invalid bytes
        (b'ab~cd', 'replace', u'ab\uFFFDd'),
        (b'ab\xffcd', 'replace', u'ab\uFFFDcd'),
        (b'ab~{\x81\x81\x41\x44~}cd', 'replace', u'ab\uFFFD\uFFFD\u804Acd'),
    )

def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
