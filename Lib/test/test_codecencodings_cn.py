#!/usr/bin/env python
#
# test_codecencodings_cn.py
#   Codec encoding tests for PRC encodings.
#

from test import support
from test import test_multibytecodec_support
import unittest

class Test_GB2312(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb2312'
    tstring = test_multibytecodec_support.load_teststring('gb2312')
    codectests = (
        # invalid bytes
        (b"abc\x81\x81\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x81\x81\xc1\xc4", "replace", "abc\ufffd\u804a"),
        (b"abc\x81\x81\xc1\xc4\xc8", "replace", "abc\ufffd\u804a\ufffd"),
        (b"abc\x81\x81\xc1\xc4", "ignore",  "abc\u804a"),
        (b"\xc1\x64", "strict", None),
    )

class Test_GBK(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gbk'
    tstring = test_multibytecodec_support.load_teststring('gbk')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\u804a"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\u804a\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u804a"),
        (b"\x83\x34\x83\x31", "strict", None),
        ("\u30fb", "strict", None),
    )

class Test_GB18030(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb18030'
    tstring = test_multibytecodec_support.load_teststring('gb18030')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\u804a"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\u804a\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u804a"),
        (b"abc\x84\x39\x84\x39\xc1\xc4", "replace", "abc\ufffd\u804a"),
        ("\u30fb", "strict", b"\x819\xa79"),
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
         'This sentence is in ASCII.\n'
         'The next sentence is in GB.'
         '\u5df1\u6240\u4e0d\u6b32\uff0c\u52ff\u65bd\u65bc\u4eba\u3002'
         'Bye.\n'),
        # test '~\n' (4 lines)
        (b'This sentence is in ASCII.\n'
         b'The next sentence is in GB.~\n'
         b'~{<:Ky2;S{#,NpJ)l6HK!#~}~\n'
         b'Bye.\n',
         'strict',
         'This sentence is in ASCII.\n'
         'The next sentence is in GB.'
         '\u5df1\u6240\u4e0d\u6b32\uff0c\u52ff\u65bd\u65bc\u4eba\u3002'
         'Bye.\n'),
        # invalid bytes
        (b'ab~cd', 'replace', 'ab\uFFFDd'),
        (b'ab\xffcd', 'replace', 'ab\uFFFDcd'),
        (b'ab~{\x81\x81\x41\x44~}cd', 'replace', 'ab\uFFFD\uFFFD\u804Acd'),
    )

def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
