#
# test_codecencodings_cn.py
#   Codec encoding tests for PRC encodings.
#

from test import multibytecodec_support
import unittest

class Test_GB2312(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb2312'
    tstring = multibytecodec_support.load_teststring('gb2312')
    codectests = (
        # invalid bytes
        (b"abc\x81\x81\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x81\x81\xc1\xc4", "replace", "abc\ufffd\ufffd\u804a"),
        (b"abc\x81\x81\xc1\xc4\xc8", "replace", "abc\ufffd\ufffd\u804a\ufffd"),
        (b"abc\x81\x81\xc1\xc4", "ignore",  "abc\u804a"),
        (b"\xc1\x64", "strict", None),
    )

class Test_GBK(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gbk'
    tstring = multibytecodec_support.load_teststring('gbk')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\ufffd\u804a"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\ufffd\u804a\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u804a"),
        (b"\x83\x34\x83\x31", "strict", None),
        ("\u30fb", "strict", None),
    )

class Test_GB18030(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb18030'
    tstring = multibytecodec_support.load_teststring('gb18030')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\ufffd\u804a"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\ufffd\u804a\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u804a"),
        (b"abc\x84\x39\x84\x39\xc1\xc4", "replace", "abc\ufffd9\ufffd9\u804a"),
        ("\u30fb", "strict", b"\x819\xa79"),
        (b"abc\x84\x32\x80\x80def", "replace", 'abc\ufffd2\ufffd\ufffddef'),
        (b"abc\x81\x30\x81\x30def", "strict", 'abc\x80def'),
        (b"abc\x86\x30\x81\x30def", "replace", 'abc\ufffd0\ufffd0def'),
        # issue29990
        (b"\xff\x30\x81\x30", "strict", None),
        (b"\x81\x30\xff\x30", "strict", None),
        (b"abc\x81\x39\xff\x39\xc1\xc4", "replace", "abc\ufffd\x39\ufffd\x39\u804a"),
        (b"abc\xab\x36\xff\x30def", "replace", 'abc\ufffd\x36\ufffd\x30def'),
        (b"abc\xbf\x38\xff\x32\xc1\xc4", "ignore",  "abc\x38\x32\u804a"),
    )
    has_iso10646 = True

class Test_HZ(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'hz'
    tstring = multibytecodec_support.load_teststring('hz')
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
        (b'ab~cd', 'replace', 'ab\uFFFDcd'),
        (b'ab\xffcd', 'replace', 'ab\uFFFDcd'),
        (b'ab~{\x81\x81\x41\x44~}cd', 'replace', 'ab\uFFFD\uFFFD\u804Acd'),
        (b'ab~{\x41\x44~}cd', 'replace', 'ab\u804Acd'),
        (b"ab~{\x79\x79\x41\x44~}cd", "replace", "ab\ufffd\ufffd\u804acd"),
        # issue 30003
        ('ab~cd', 'strict',  b'ab~~cd'),  # escape ~
        (b'~{Dc~~:C~}', 'strict', None),  # ~~ only in ASCII mode
        (b'~{Dc~\n:C~}', 'strict', None), # ~\n only in ASCII mode
    )

if __name__ == "__main__":
    unittest.main()
