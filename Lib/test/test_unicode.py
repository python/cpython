""" Test script for the Unicode implementation.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
import _string
import codecs
import struct
import sys
import unittest
import warnings
from test import support, string_tests

# Error handling (bad decoder return)
def search_function(encoding):
    def decode1(input, errors="strict"):
        return 42 # not a tuple
    def encode1(input, errors="strict"):
        return 42 # not a tuple
    def encode2(input, errors="strict"):
        return (42, 42) # no unicode
    def decode2(input, errors="strict"):
        return (42, 42) # no unicode
    if encoding=="test.unicode1":
        return (encode1, decode1, None, None)
    elif encoding=="test.unicode2":
        return (encode2, decode2, None, None)
    else:
        return None
codecs.register(search_function)

class UnicodeTest(string_tests.CommonTest,
        string_tests.MixinStrUnicodeUserStringTest,
        string_tests.MixinStrUnicodeTest,
        unittest.TestCase):

    type2test = str

    def checkequalnofix(self, result, object, methodname, *args):
        method = getattr(object, methodname)
        realresult = method(*args)
        self.assertEqual(realresult, result)
        self.assertTrue(type(realresult) is type(result))

        # if the original is returned make sure that
        # this doesn't happen with subclasses
        if realresult is object:
            class usub(str):
                def __repr__(self):
                    return 'usub(%r)' % str.__repr__(self)
            object = usub(object)
            method = getattr(object, methodname)
            realresult = method(*args)
            self.assertEqual(realresult, result)
            self.assertTrue(object is not realresult)

    def test_literals(self):
        self.assertEqual('\xff', '\u00ff')
        self.assertEqual('\uffff', '\U0000ffff')
        self.assertRaises(SyntaxError, eval, '\'\\Ufffffffe\'')
        self.assertRaises(SyntaxError, eval, '\'\\Uffffffff\'')
        self.assertRaises(SyntaxError, eval, '\'\\U%08x\'' % 0x110000)
        # raw strings should not have unicode escapes
        self.assertNotEqual(r"\u0020", " ")

    def test_ascii(self):
        if not sys.platform.startswith('java'):
            # Test basic sanity of repr()
            self.assertEqual(ascii('abc'), "'abc'")
            self.assertEqual(ascii('ab\\c'), "'ab\\\\c'")
            self.assertEqual(ascii('ab\\'), "'ab\\\\'")
            self.assertEqual(ascii('\\c'), "'\\\\c'")
            self.assertEqual(ascii('\\'), "'\\\\'")
            self.assertEqual(ascii('\n'), "'\\n'")
            self.assertEqual(ascii('\r'), "'\\r'")
            self.assertEqual(ascii('\t'), "'\\t'")
            self.assertEqual(ascii('\b'), "'\\x08'")
            self.assertEqual(ascii("'\""), """'\\'"'""")
            self.assertEqual(ascii("'\""), """'\\'"'""")
            self.assertEqual(ascii("'"), '''"'"''')
            self.assertEqual(ascii('"'), """'"'""")
            latin1repr = (
                "'\\x00\\x01\\x02\\x03\\x04\\x05\\x06\\x07\\x08\\t\\n\\x0b\\x0c\\r"
                "\\x0e\\x0f\\x10\\x11\\x12\\x13\\x14\\x15\\x16\\x17\\x18\\x19\\x1a"
                "\\x1b\\x1c\\x1d\\x1e\\x1f !\"#$%&\\'()*+,-./0123456789:;<=>?@ABCDEFGHI"
                "JKLMNOPQRSTUVWXYZ[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\\x7f"
                "\\x80\\x81\\x82\\x83\\x84\\x85\\x86\\x87\\x88\\x89\\x8a\\x8b\\x8c\\x8d"
                "\\x8e\\x8f\\x90\\x91\\x92\\x93\\x94\\x95\\x96\\x97\\x98\\x99\\x9a\\x9b"
                "\\x9c\\x9d\\x9e\\x9f\\xa0\\xa1\\xa2\\xa3\\xa4\\xa5\\xa6\\xa7\\xa8\\xa9"
                "\\xaa\\xab\\xac\\xad\\xae\\xaf\\xb0\\xb1\\xb2\\xb3\\xb4\\xb5\\xb6\\xb7"
                "\\xb8\\xb9\\xba\\xbb\\xbc\\xbd\\xbe\\xbf\\xc0\\xc1\\xc2\\xc3\\xc4\\xc5"
                "\\xc6\\xc7\\xc8\\xc9\\xca\\xcb\\xcc\\xcd\\xce\\xcf\\xd0\\xd1\\xd2\\xd3"
                "\\xd4\\xd5\\xd6\\xd7\\xd8\\xd9\\xda\\xdb\\xdc\\xdd\\xde\\xdf\\xe0\\xe1"
                "\\xe2\\xe3\\xe4\\xe5\\xe6\\xe7\\xe8\\xe9\\xea\\xeb\\xec\\xed\\xee\\xef"
                "\\xf0\\xf1\\xf2\\xf3\\xf4\\xf5\\xf6\\xf7\\xf8\\xf9\\xfa\\xfb\\xfc\\xfd"
                "\\xfe\\xff'")
            testrepr = ascii(''.join(map(chr, range(256))))
            self.assertEqual(testrepr, latin1repr)
            # Test ascii works on wide unicode escapes without overflow.
            self.assertEqual(ascii("\U00010000" * 39 + "\uffff" * 4096),
                             ascii("\U00010000" * 39 + "\uffff" * 4096))

            class WrongRepr:
                def __repr__(self):
                    return b'byte-repr'
            self.assertRaises(TypeError, ascii, WrongRepr())

    def test_repr(self):
        if not sys.platform.startswith('java'):
            # Test basic sanity of repr()
            self.assertEqual(repr('abc'), "'abc'")
            self.assertEqual(repr('ab\\c'), "'ab\\\\c'")
            self.assertEqual(repr('ab\\'), "'ab\\\\'")
            self.assertEqual(repr('\\c'), "'\\\\c'")
            self.assertEqual(repr('\\'), "'\\\\'")
            self.assertEqual(repr('\n'), "'\\n'")
            self.assertEqual(repr('\r'), "'\\r'")
            self.assertEqual(repr('\t'), "'\\t'")
            self.assertEqual(repr('\b'), "'\\x08'")
            self.assertEqual(repr("'\""), """'\\'"'""")
            self.assertEqual(repr("'\""), """'\\'"'""")
            self.assertEqual(repr("'"), '''"'"''')
            self.assertEqual(repr('"'), """'"'""")
            latin1repr = (
                "'\\x00\\x01\\x02\\x03\\x04\\x05\\x06\\x07\\x08\\t\\n\\x0b\\x0c\\r"
                "\\x0e\\x0f\\x10\\x11\\x12\\x13\\x14\\x15\\x16\\x17\\x18\\x19\\x1a"
                "\\x1b\\x1c\\x1d\\x1e\\x1f !\"#$%&\\'()*+,-./0123456789:;<=>?@ABCDEFGHI"
                "JKLMNOPQRSTUVWXYZ[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\\x7f"
                "\\x80\\x81\\x82\\x83\\x84\\x85\\x86\\x87\\x88\\x89\\x8a\\x8b\\x8c\\x8d"
                "\\x8e\\x8f\\x90\\x91\\x92\\x93\\x94\\x95\\x96\\x97\\x98\\x99\\x9a\\x9b"
                "\\x9c\\x9d\\x9e\\x9f\\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9"
                "\xaa\xab\xac\\xad\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7"
                "\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5"
                "\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3"
                "\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1"
                "\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
                "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd"
                "\xfe\xff'")
            testrepr = repr(''.join(map(chr, range(256))))
            self.assertEqual(testrepr, latin1repr)
            # Test repr works on wide unicode escapes without overflow.
            self.assertEqual(repr("\U00010000" * 39 + "\uffff" * 4096),
                             repr("\U00010000" * 39 + "\uffff" * 4096))

            class WrongRepr:
                def __repr__(self):
                    return b'byte-repr'
            self.assertRaises(TypeError, repr, WrongRepr())

    def test_iterators(self):
        # Make sure unicode objects have an __iter__ method
        it = "\u1111\u2222\u3333".__iter__()
        self.assertEqual(next(it), "\u1111")
        self.assertEqual(next(it), "\u2222")
        self.assertEqual(next(it), "\u3333")
        self.assertRaises(StopIteration, next, it)

    def test_count(self):
        string_tests.CommonTest.test_count(self)
        # check mixed argument types
        self.checkequalnofix(3,  'aaa', 'count', 'a')
        self.checkequalnofix(0,  'aaa', 'count', 'b')
        self.checkequalnofix(3, 'aaa', 'count',  'a')
        self.checkequalnofix(0, 'aaa', 'count',  'b')
        self.checkequalnofix(0, 'aaa', 'count',  'b')
        self.checkequalnofix(1, 'aaa', 'count',  'a', -1)
        self.checkequalnofix(3, 'aaa', 'count',  'a', -10)
        self.checkequalnofix(2, 'aaa', 'count',  'a', 0, -1)
        self.checkequalnofix(0, 'aaa', 'count',  'a', 0, -10)

    def test_find(self):
        string_tests.CommonTest.test_find(self)
        # test implementation details of the memchr fast path
        self.checkequal(100, 'a' * 100 + '\u0102', 'find', '\u0102')
        self.checkequal(-1, 'a' * 100 + '\u0102', 'find', '\u0201')
        self.checkequal(-1, 'a' * 100 + '\u0102', 'find', '\u0120')
        self.checkequal(-1, 'a' * 100 + '\u0102', 'find', '\u0220')
        self.checkequal(100, 'a' * 100 + '\U00100304', 'find', '\U00100304')
        self.checkequal(-1, 'a' * 100 + '\U00100304', 'find', '\U00100204')
        self.checkequal(-1, 'a' * 100 + '\U00100304', 'find', '\U00102004')
        # check mixed argument types
        self.checkequalnofix(0,  'abcdefghiabc', 'find', 'abc')
        self.checkequalnofix(9,  'abcdefghiabc', 'find', 'abc', 1)
        self.checkequalnofix(-1, 'abcdefghiabc', 'find', 'def', 4)

        self.assertRaises(TypeError, 'hello'.find)
        self.assertRaises(TypeError, 'hello'.find, 42)

    def test_rfind(self):
        string_tests.CommonTest.test_rfind(self)
        # test implementation details of the memrchr fast path
        self.checkequal(0, '\u0102' + 'a' * 100 , 'rfind', '\u0102')
        self.checkequal(-1, '\u0102' + 'a' * 100 , 'rfind', '\u0201')
        self.checkequal(-1, '\u0102' + 'a' * 100 , 'rfind', '\u0120')
        self.checkequal(-1, '\u0102' + 'a' * 100 , 'rfind', '\u0220')
        self.checkequal(0, '\U00100304' + 'a' * 100, 'rfind', '\U00100304')
        self.checkequal(-1, '\U00100304' + 'a' * 100, 'rfind', '\U00100204')
        self.checkequal(-1, '\U00100304' + 'a' * 100, 'rfind', '\U00102004')
        # check mixed argument types
        self.checkequalnofix(9,   'abcdefghiabc', 'rfind', 'abc')
        self.checkequalnofix(12,  'abcdefghiabc', 'rfind', '')
        self.checkequalnofix(12, 'abcdefghiabc', 'rfind',  '')

    def test_index(self):
        string_tests.CommonTest.test_index(self)
        self.checkequalnofix(0, 'abcdefghiabc', 'index',  '')
        self.checkequalnofix(3, 'abcdefghiabc', 'index',  'def')
        self.checkequalnofix(0, 'abcdefghiabc', 'index',  'abc')
        self.checkequalnofix(9, 'abcdefghiabc', 'index',  'abc', 1)
        self.assertRaises(ValueError, 'abcdefghiabc'.index, 'hib')
        self.assertRaises(ValueError, 'abcdefghiab'.index,  'abc', 1)
        self.assertRaises(ValueError, 'abcdefghi'.index,  'ghi', 8)
        self.assertRaises(ValueError, 'abcdefghi'.index,  'ghi', -1)

    def test_rindex(self):
        string_tests.CommonTest.test_rindex(self)
        self.checkequalnofix(12, 'abcdefghiabc', 'rindex',  '')
        self.checkequalnofix(3,  'abcdefghiabc', 'rindex',  'def')
        self.checkequalnofix(9,  'abcdefghiabc', 'rindex',  'abc')
        self.checkequalnofix(0,  'abcdefghiabc', 'rindex',  'abc', 0, -1)

        self.assertRaises(ValueError, 'abcdefghiabc'.rindex,  'hib')
        self.assertRaises(ValueError, 'defghiabc'.rindex,  'def', 1)
        self.assertRaises(ValueError, 'defghiabc'.rindex,  'abc', 0, -1)
        self.assertRaises(ValueError, 'abcdefghi'.rindex,  'ghi', 0, 8)
        self.assertRaises(ValueError, 'abcdefghi'.rindex,  'ghi', 0, -1)

    def test_maketrans_translate(self):
        # these work with plain translate()
        self.checkequalnofix('bbbc', 'abababc', 'translate',
                             {ord('a'): None})
        self.checkequalnofix('iiic', 'abababc', 'translate',
                             {ord('a'): None, ord('b'): ord('i')})
        self.checkequalnofix('iiix', 'abababc', 'translate',
                             {ord('a'): None, ord('b'): ord('i'), ord('c'): 'x'})
        self.checkequalnofix('c', 'abababc', 'translate',
                             {ord('a'): None, ord('b'): ''})
        self.checkequalnofix('xyyx', 'xzx', 'translate',
                             {ord('z'): 'yy'})
        # this needs maketrans()
        self.checkequalnofix('abababc', 'abababc', 'translate',
                             {'b': '<i>'})
        tbl = self.type2test.maketrans({'a': None, 'b': '<i>'})
        self.checkequalnofix('<i><i><i>c', 'abababc', 'translate', tbl)
        # test alternative way of calling maketrans()
        tbl = self.type2test.maketrans('abc', 'xyz', 'd')
        self.checkequalnofix('xyzzy', 'abdcdcbdddd', 'translate', tbl)

        self.assertRaises(TypeError, self.type2test.maketrans)
        self.assertRaises(ValueError, self.type2test.maketrans, 'abc', 'defg')
        self.assertRaises(TypeError, self.type2test.maketrans, 2, 'def')
        self.assertRaises(TypeError, self.type2test.maketrans, 'abc', 2)
        self.assertRaises(TypeError, self.type2test.maketrans, 'abc', 'def', 2)
        self.assertRaises(ValueError, self.type2test.maketrans, {'xy': 2})
        self.assertRaises(TypeError, self.type2test.maketrans, {(1,): 2})

        self.assertRaises(TypeError, 'hello'.translate)
        self.assertRaises(TypeError, 'abababc'.translate, 'abc', 'xyz')

    def test_split(self):
        string_tests.CommonTest.test_split(self)

        # Mixed arguments
        self.checkequalnofix(['a', 'b', 'c', 'd'], 'a//b//c//d', 'split', '//')
        self.checkequalnofix(['a', 'b', 'c', 'd'], 'a//b//c//d', 'split', '//')
        self.checkequalnofix(['endcase ', ''], 'endcase test', 'split', 'test')

    def test_join(self):
        string_tests.MixinStrUnicodeUserStringTest.test_join(self)

        class MyWrapper:
            def __init__(self, sval): self.sval = sval
            def __str__(self): return self.sval

        # mixed arguments
        self.checkequalnofix('a b c d', ' ', 'join', ['a', 'b', 'c', 'd'])
        self.checkequalnofix('abcd', '', 'join', ('a', 'b', 'c', 'd'))
        self.checkequalnofix('w x y z', ' ', 'join', string_tests.Sequence('wxyz'))
        self.checkequalnofix('a b c d', ' ', 'join', ['a', 'b', 'c', 'd'])
        self.checkequalnofix('a b c d', ' ', 'join', ['a', 'b', 'c', 'd'])
        self.checkequalnofix('abcd', '', 'join', ('a', 'b', 'c', 'd'))
        self.checkequalnofix('w x y z', ' ', 'join', string_tests.Sequence('wxyz'))
        self.checkraises(TypeError, ' ', 'join', ['1', '2', MyWrapper('foo')])
        self.checkraises(TypeError, ' ', 'join', ['1', '2', '3', bytes()])
        self.checkraises(TypeError, ' ', 'join', [1, 2, 3])
        self.checkraises(TypeError, ' ', 'join', ['1', '2', 3])

    def test_replace(self):
        string_tests.CommonTest.test_replace(self)

        # method call forwarded from str implementation because of unicode argument
        self.checkequalnofix('one@two!three!', 'one!two!three!', 'replace', '!', '@', 1)
        self.assertRaises(TypeError, 'replace'.replace, "r", 42)

    @support.cpython_only
    def test_replace_id(self):
        pattern = 'abc'
        text = 'abc def'
        self.assertIs(text.replace(pattern, pattern), text)

    def test_bytes_comparison(self):
        with support.check_warnings():
            warnings.simplefilter('ignore', BytesWarning)
            self.assertEqual('abc' == b'abc', False)
            self.assertEqual('abc' != b'abc', True)
            self.assertEqual('abc' == bytearray(b'abc'), False)
            self.assertEqual('abc' != bytearray(b'abc'), True)

    def test_comparison(self):
        # Comparisons:
        self.assertEqual('abc', 'abc')
        self.assertTrue('abcd' > 'abc')
        self.assertTrue('abc' < 'abcd')

        if 0:
            # Move these tests to a Unicode collation module test...
            # Testing UTF-16 code point order comparisons...

            # No surrogates, no fixup required.
            self.assertTrue('\u0061' < '\u20ac')
            # Non surrogate below surrogate value, no fixup required
            self.assertTrue('\u0061' < '\ud800\udc02')

            # Non surrogate above surrogate value, fixup required
            def test_lecmp(s, s2):
                self.assertTrue(s < s2)

            def test_fixup(s):
                s2 = '\ud800\udc01'
                test_lecmp(s, s2)
                s2 = '\ud900\udc01'
                test_lecmp(s, s2)
                s2 = '\uda00\udc01'
                test_lecmp(s, s2)
                s2 = '\udb00\udc01'
                test_lecmp(s, s2)
                s2 = '\ud800\udd01'
                test_lecmp(s, s2)
                s2 = '\ud900\udd01'
                test_lecmp(s, s2)
                s2 = '\uda00\udd01'
                test_lecmp(s, s2)
                s2 = '\udb00\udd01'
                test_lecmp(s, s2)
                s2 = '\ud800\ude01'
                test_lecmp(s, s2)
                s2 = '\ud900\ude01'
                test_lecmp(s, s2)
                s2 = '\uda00\ude01'
                test_lecmp(s, s2)
                s2 = '\udb00\ude01'
                test_lecmp(s, s2)
                s2 = '\ud800\udfff'
                test_lecmp(s, s2)
                s2 = '\ud900\udfff'
                test_lecmp(s, s2)
                s2 = '\uda00\udfff'
                test_lecmp(s, s2)
                s2 = '\udb00\udfff'
                test_lecmp(s, s2)

                test_fixup('\ue000')
                test_fixup('\uff61')

        # Surrogates on both sides, no fixup required
        self.assertTrue('\ud800\udc02' < '\ud84d\udc56')

    def test_islower(self):
        string_tests.MixinStrUnicodeUserStringTest.test_islower(self)
        self.checkequalnofix(False, '\u1FFc', 'islower')
        self.assertFalse('\u2167'.islower())
        self.assertTrue('\u2177'.islower())
        # non-BMP, uppercase
        self.assertFalse('\U00010401'.islower())
        self.assertFalse('\U00010427'.islower())
        # non-BMP, lowercase
        self.assertTrue('\U00010429'.islower())
        self.assertTrue('\U0001044E'.islower())
        # non-BMP, non-cased
        self.assertFalse('\U0001F40D'.islower())
        self.assertFalse('\U0001F46F'.islower())

    def test_isupper(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isupper(self)
        if not sys.platform.startswith('java'):
            self.checkequalnofix(False, '\u1FFc', 'isupper')
        self.assertTrue('\u2167'.isupper())
        self.assertFalse('\u2177'.isupper())
        # non-BMP, uppercase
        self.assertTrue('\U00010401'.isupper())
        self.assertTrue('\U00010427'.isupper())
        # non-BMP, lowercase
        self.assertFalse('\U00010429'.isupper())
        self.assertFalse('\U0001044E'.isupper())
        # non-BMP, non-cased
        self.assertFalse('\U0001F40D'.isupper())
        self.assertFalse('\U0001F46F'.isupper())

    def test_istitle(self):
        string_tests.MixinStrUnicodeUserStringTest.test_istitle(self)
        self.checkequalnofix(True, '\u1FFc', 'istitle')
        self.checkequalnofix(True, 'Greek \u1FFcitlecases ...', 'istitle')

        # non-BMP, uppercase + lowercase
        self.assertTrue('\U00010401\U00010429'.istitle())
        self.assertTrue('\U00010427\U0001044E'.istitle())
        # apparently there are no titlecased (Lt) non-BMP chars in Unicode 6
        for ch in ['\U00010429', '\U0001044E', '\U0001F40D', '\U0001F46F']:
            self.assertFalse(ch.istitle(), '{!a} is not title'.format(ch))

    def test_isspace(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isspace(self)
        self.checkequalnofix(True, '\u2000', 'isspace')
        self.checkequalnofix(True, '\u200a', 'isspace')
        self.checkequalnofix(False, '\u2014', 'isspace')
        # apparently there are no non-BMP spaces chars in Unicode 6
        for ch in ['\U00010401', '\U00010427', '\U00010429', '\U0001044E',
                   '\U0001F40D', '\U0001F46F']:
            self.assertFalse(ch.isspace(), '{!a} is not space.'.format(ch))

    def test_isalnum(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isalnum(self)
        for ch in ['\U00010401', '\U00010427', '\U00010429', '\U0001044E',
                   '\U0001D7F6', '\U00011066', '\U000104A0', '\U0001F107']:
            self.assertTrue(ch.isalnum(), '{!a} is alnum.'.format(ch))

    def test_isalpha(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isalpha(self)
        self.checkequalnofix(True, '\u1FFc', 'isalpha')
        # non-BMP, cased
        self.assertTrue('\U00010401'.isalpha())
        self.assertTrue('\U00010427'.isalpha())
        self.assertTrue('\U00010429'.isalpha())
        self.assertTrue('\U0001044E'.isalpha())
        # non-BMP, non-cased
        self.assertFalse('\U0001F40D'.isalpha())
        self.assertFalse('\U0001F46F'.isalpha())

    def test_isdecimal(self):
        self.checkequalnofix(False, '', 'isdecimal')
        self.checkequalnofix(False, 'a', 'isdecimal')
        self.checkequalnofix(True, '0', 'isdecimal')
        self.checkequalnofix(False, '\u2460', 'isdecimal') # CIRCLED DIGIT ONE
        self.checkequalnofix(False, '\xbc', 'isdecimal') # VULGAR FRACTION ONE QUARTER
        self.checkequalnofix(True, '\u0660', 'isdecimal') # ARABIC-INDIC DIGIT ZERO
        self.checkequalnofix(True, '0123456789', 'isdecimal')
        self.checkequalnofix(False, '0123456789a', 'isdecimal')

        self.checkraises(TypeError, 'abc', 'isdecimal', 42)

        for ch in ['\U00010401', '\U00010427', '\U00010429', '\U0001044E',
                   '\U0001F40D', '\U0001F46F', '\U00011065', '\U0001F107']:
            self.assertFalse(ch.isdecimal(), '{!a} is not decimal.'.format(ch))
        for ch in ['\U0001D7F6', '\U00011066', '\U000104A0']:
            self.assertTrue(ch.isdecimal(), '{!a} is decimal.'.format(ch))

    def test_isdigit(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isdigit(self)
        self.checkequalnofix(True, '\u2460', 'isdigit')
        self.checkequalnofix(False, '\xbc', 'isdigit')
        self.checkequalnofix(True, '\u0660', 'isdigit')

        for ch in ['\U00010401', '\U00010427', '\U00010429', '\U0001044E',
                   '\U0001F40D', '\U0001F46F', '\U00011065']:
            self.assertFalse(ch.isdigit(), '{!a} is not a digit.'.format(ch))
        for ch in ['\U0001D7F6', '\U00011066', '\U000104A0', '\U0001F107']:
            self.assertTrue(ch.isdigit(), '{!a} is a digit.'.format(ch))

    def test_isnumeric(self):
        self.checkequalnofix(False, '', 'isnumeric')
        self.checkequalnofix(False, 'a', 'isnumeric')
        self.checkequalnofix(True, '0', 'isnumeric')
        self.checkequalnofix(True, '\u2460', 'isnumeric')
        self.checkequalnofix(True, '\xbc', 'isnumeric')
        self.checkequalnofix(True, '\u0660', 'isnumeric')
        self.checkequalnofix(True, '0123456789', 'isnumeric')
        self.checkequalnofix(False, '0123456789a', 'isnumeric')

        self.assertRaises(TypeError, "abc".isnumeric, 42)

        for ch in ['\U00010401', '\U00010427', '\U00010429', '\U0001044E',
                   '\U0001F40D', '\U0001F46F']:
            self.assertFalse(ch.isnumeric(), '{!a} is not numeric.'.format(ch))
        for ch in ['\U00011065', '\U0001D7F6', '\U00011066',
                   '\U000104A0', '\U0001F107']:
            self.assertTrue(ch.isnumeric(), '{!a} is numeric.'.format(ch))

    def test_isidentifier(self):
        self.assertTrue("a".isidentifier())
        self.assertTrue("Z".isidentifier())
        self.assertTrue("_".isidentifier())
        self.assertTrue("b0".isidentifier())
        self.assertTrue("bc".isidentifier())
        self.assertTrue("b_".isidentifier())
        self.assertTrue("µ".isidentifier())
        self.assertTrue("𝔘𝔫𝔦𝔠𝔬𝔡𝔢".isidentifier())

        self.assertFalse(" ".isidentifier())
        self.assertFalse("[".isidentifier())
        self.assertFalse("©".isidentifier())
        self.assertFalse("0".isidentifier())

    def test_isprintable(self):
        self.assertTrue("".isprintable())
        self.assertTrue(" ".isprintable())
        self.assertTrue("abcdefg".isprintable())
        self.assertFalse("abcdefg\n".isprintable())
        # some defined Unicode character
        self.assertTrue("\u0374".isprintable())
        # undefined character
        self.assertFalse("\u0378".isprintable())
        # single surrogate character
        self.assertFalse("\ud800".isprintable())

        self.assertTrue('\U0001F46F'.isprintable())
        self.assertFalse('\U000E0020'.isprintable())

    def test_surrogates(self):
        for s in ('a\uD800b\uDFFF', 'a\uDFFFb\uD800',
                  'a\uD800b\uDFFFa', 'a\uDFFFb\uD800a'):
            self.assertTrue(s.islower())
            self.assertFalse(s.isupper())
            self.assertFalse(s.istitle())
        for s in ('A\uD800B\uDFFF', 'A\uDFFFB\uD800',
                  'A\uD800B\uDFFFA', 'A\uDFFFB\uD800A'):
            self.assertFalse(s.islower())
            self.assertTrue(s.isupper())
            self.assertTrue(s.istitle())

        for meth_name in ('islower', 'isupper', 'istitle'):
            meth = getattr(str, meth_name)
            for s in ('\uD800', '\uDFFF', '\uD800\uD800', '\uDFFF\uDFFF'):
                self.assertFalse(meth(s), '%a.%s() is False' % (s, meth_name))

        for meth_name in ('isalpha', 'isalnum', 'isdigit', 'isspace',
                          'isdecimal', 'isnumeric',
                          'isidentifier', 'isprintable'):
            meth = getattr(str, meth_name)
            for s in ('\uD800', '\uDFFF', '\uD800\uD800', '\uDFFF\uDFFF',
                      'a\uD800b\uDFFF', 'a\uDFFFb\uD800',
                      'a\uD800b\uDFFFa', 'a\uDFFFb\uD800a'):
                self.assertFalse(meth(s), '%a.%s() is False' % (s, meth_name))


    def test_lower(self):
        string_tests.CommonTest.test_lower(self)
        self.assertEqual('\U00010427'.lower(), '\U0001044F')
        self.assertEqual('\U00010427\U00010427'.lower(),
                         '\U0001044F\U0001044F')
        self.assertEqual('\U00010427\U0001044F'.lower(),
                         '\U0001044F\U0001044F')
        self.assertEqual('X\U00010427x\U0001044F'.lower(),
                         'x\U0001044Fx\U0001044F')
        self.assertEqual('ﬁ'.lower(), 'ﬁ')
        self.assertEqual('\u0130'.lower(), '\u0069\u0307')
        # Special case for GREEK CAPITAL LETTER SIGMA U+03A3
        self.assertEqual('\u03a3'.lower(), '\u03c3')
        self.assertEqual('\u0345\u03a3'.lower(), '\u0345\u03c3')
        self.assertEqual('A\u0345\u03a3'.lower(), 'a\u0345\u03c2')
        self.assertEqual('A\u0345\u03a3a'.lower(), 'a\u0345\u03c3a')
        self.assertEqual('A\u0345\u03a3'.lower(), 'a\u0345\u03c2')
        self.assertEqual('A\u03a3\u0345'.lower(), 'a\u03c2\u0345')
        self.assertEqual('\u03a3\u0345 '.lower(), '\u03c3\u0345 ')
        self.assertEqual('\U0008fffe'.lower(), '\U0008fffe')
        self.assertEqual('\u2177'.lower(), '\u2177')

    def test_casefold(self):
        self.assertEqual('hello'.casefold(), 'hello')
        self.assertEqual('hELlo'.casefold(), 'hello')
        self.assertEqual('ß'.casefold(), 'ss')
        self.assertEqual('ﬁ'.casefold(), 'fi')
        self.assertEqual('\u03a3'.casefold(), '\u03c3')
        self.assertEqual('A\u0345\u03a3'.casefold(), 'a\u03b9\u03c3')
        self.assertEqual('\u00b5'.casefold(), '\u03bc')

    def test_upper(self):
        string_tests.CommonTest.test_upper(self)
        self.assertEqual('\U0001044F'.upper(), '\U00010427')
        self.assertEqual('\U0001044F\U0001044F'.upper(),
                         '\U00010427\U00010427')
        self.assertEqual('\U00010427\U0001044F'.upper(),
                         '\U00010427\U00010427')
        self.assertEqual('X\U00010427x\U0001044F'.upper(),
                         'X\U00010427X\U00010427')
        self.assertEqual('ﬁ'.upper(), 'FI')
        self.assertEqual('\u0130'.upper(), '\u0130')
        self.assertEqual('\u03a3'.upper(), '\u03a3')
        self.assertEqual('ß'.upper(), 'SS')
        self.assertEqual('\u1fd2'.upper(), '\u0399\u0308\u0300')
        self.assertEqual('\U0008fffe'.upper(), '\U0008fffe')
        self.assertEqual('\u2177'.upper(), '\u2167')

    def test_capitalize(self):
        string_tests.CommonTest.test_capitalize(self)
        self.assertEqual('\U0001044F'.capitalize(), '\U00010427')
        self.assertEqual('\U0001044F\U0001044F'.capitalize(),
                         '\U00010427\U0001044F')
        self.assertEqual('\U00010427\U0001044F'.capitalize(),
                         '\U00010427\U0001044F')
        self.assertEqual('\U0001044F\U00010427'.capitalize(),
                         '\U00010427\U0001044F')
        self.assertEqual('X\U00010427x\U0001044F'.capitalize(),
                         'X\U0001044Fx\U0001044F')
        self.assertEqual('h\u0130'.capitalize(), 'H\u0069\u0307')
        exp = '\u0399\u0308\u0300\u0069\u0307'
        self.assertEqual('\u1fd2\u0130'.capitalize(), exp)
        self.assertEqual('ﬁnnish'.capitalize(), 'FInnish')
        self.assertEqual('A\u0345\u03a3'.capitalize(), 'A\u0345\u03c2')

    def test_title(self):
        string_tests.MixinStrUnicodeUserStringTest.test_title(self)
        self.assertEqual('\U0001044F'.title(), '\U00010427')
        self.assertEqual('\U0001044F\U0001044F'.title(),
                         '\U00010427\U0001044F')
        self.assertEqual('\U0001044F\U0001044F \U0001044F\U0001044F'.title(),
                         '\U00010427\U0001044F \U00010427\U0001044F')
        self.assertEqual('\U00010427\U0001044F \U00010427\U0001044F'.title(),
                         '\U00010427\U0001044F \U00010427\U0001044F')
        self.assertEqual('\U0001044F\U00010427 \U0001044F\U00010427'.title(),
                         '\U00010427\U0001044F \U00010427\U0001044F')
        self.assertEqual('X\U00010427x\U0001044F X\U00010427x\U0001044F'.title(),
                         'X\U0001044Fx\U0001044F X\U0001044Fx\U0001044F')
        self.assertEqual('ﬁNNISH'.title(), 'Finnish')
        self.assertEqual('A\u03a3 \u1fa1xy'.title(), 'A\u03c2 \u1fa9xy')
        self.assertEqual('A\u03a3A'.title(), 'A\u03c3a')

    def test_swapcase(self):
        string_tests.CommonTest.test_swapcase(self)
        self.assertEqual('\U0001044F'.swapcase(), '\U00010427')
        self.assertEqual('\U00010427'.swapcase(), '\U0001044F')
        self.assertEqual('\U0001044F\U0001044F'.swapcase(),
                         '\U00010427\U00010427')
        self.assertEqual('\U00010427\U0001044F'.swapcase(),
                         '\U0001044F\U00010427')
        self.assertEqual('\U0001044F\U00010427'.swapcase(),
                         '\U00010427\U0001044F')
        self.assertEqual('X\U00010427x\U0001044F'.swapcase(),
                         'x\U0001044FX\U00010427')
        self.assertEqual('ﬁ'.swapcase(), 'FI')
        self.assertEqual('\u0130'.swapcase(), '\u0069\u0307')
        # Special case for GREEK CAPITAL LETTER SIGMA U+03A3
        self.assertEqual('\u03a3'.swapcase(), '\u03c3')
        self.assertEqual('\u0345\u03a3'.swapcase(), '\u0399\u03c3')
        self.assertEqual('A\u0345\u03a3'.swapcase(), 'a\u0399\u03c2')
        self.assertEqual('A\u0345\u03a3a'.swapcase(), 'a\u0399\u03c3A')
        self.assertEqual('A\u0345\u03a3'.swapcase(), 'a\u0399\u03c2')
        self.assertEqual('A\u03a3\u0345'.swapcase(), 'a\u03c2\u0399')
        self.assertEqual('\u03a3\u0345 '.swapcase(), '\u03c3\u0399 ')
        self.assertEqual('\u03a3'.swapcase(), '\u03c3')
        self.assertEqual('ß'.swapcase(), 'SS')
        self.assertEqual('\u1fd2'.swapcase(), '\u0399\u0308\u0300')

    def test_center(self):
        string_tests.CommonTest.test_center(self)
        self.assertEqual('x'.center(2, '\U0010FFFF'),
                         'x\U0010FFFF')
        self.assertEqual('x'.center(3, '\U0010FFFF'),
                         '\U0010FFFFx\U0010FFFF')
        self.assertEqual('x'.center(4, '\U0010FFFF'),
                         '\U0010FFFFx\U0010FFFF\U0010FFFF')

    def test_contains(self):
        # Testing Unicode contains method
        self.assertIn('a', 'abdb')
        self.assertIn('a', 'bdab')
        self.assertIn('a', 'bdaba')
        self.assertIn('a', 'bdba')
        self.assertNotIn('a', 'bdb')
        self.assertIn('a', 'bdba')
        self.assertIn('a', ('a',1,None))
        self.assertIn('a', (1,None,'a'))
        self.assertIn('a', ('a',1,None))
        self.assertIn('a', (1,None,'a'))
        self.assertNotIn('a', ('x',1,'y'))
        self.assertNotIn('a', ('x',1,None))
        self.assertNotIn('abcd', 'abcxxxx')
        self.assertIn('ab', 'abcd')
        self.assertIn('ab', 'abc')
        self.assertIn('ab', (1,None,'ab'))
        self.assertIn('', 'abc')
        self.assertIn('', '')
        self.assertIn('', 'abc')
        self.assertNotIn('\0', 'abc')
        self.assertIn('\0', '\0abc')
        self.assertIn('\0', 'abc\0')
        self.assertIn('a', '\0abc')
        self.assertIn('asdf', 'asdf')
        self.assertNotIn('asdf', 'asd')
        self.assertNotIn('asdf', '')

        self.assertRaises(TypeError, "abc".__contains__)

    def test_issue18183(self):
        '\U00010000\U00100000'.lower()
        '\U00010000\U00100000'.casefold()
        '\U00010000\U00100000'.upper()
        '\U00010000\U00100000'.capitalize()
        '\U00010000\U00100000'.title()
        '\U00010000\U00100000'.swapcase()
        '\U00100000'.center(3, '\U00010000')
        '\U00100000'.ljust(3, '\U00010000')
        '\U00100000'.rjust(3, '\U00010000')

    def test_format(self):
        self.assertEqual(''.format(), '')
        self.assertEqual('a'.format(), 'a')
        self.assertEqual('ab'.format(), 'ab')
        self.assertEqual('a{{'.format(), 'a{')
        self.assertEqual('a}}'.format(), 'a}')
        self.assertEqual('{{b'.format(), '{b')
        self.assertEqual('}}b'.format(), '}b')
        self.assertEqual('a{{b'.format(), 'a{b')

        # examples from the PEP:
        import datetime
        self.assertEqual("My name is {0}".format('Fred'), "My name is Fred")
        self.assertEqual("My name is {0[name]}".format(dict(name='Fred')),
                         "My name is Fred")
        self.assertEqual("My name is {0} :-{{}}".format('Fred'),
                         "My name is Fred :-{}")

        d = datetime.date(2007, 8, 18)
        self.assertEqual("The year is {0.year}".format(d),
                         "The year is 2007")

        # classes we'll use for testing
        class C:
            def __init__(self, x=100):
                self._x = x
            def __format__(self, spec):
                return spec

        class D:
            def __init__(self, x):
                self.x = x
            def __format__(self, spec):
                return str(self.x)

        # class with __str__, but no __format__
        class E:
            def __init__(self, x):
                self.x = x
            def __str__(self):
                return 'E(' + self.x + ')'

        # class with __repr__, but no __format__ or __str__
        class F:
            def __init__(self, x):
                self.x = x
            def __repr__(self):
                return 'F(' + self.x + ')'

        # class with __format__ that forwards to string, for some format_spec's
        class G:
            def __init__(self, x):
                self.x = x
            def __str__(self):
                return "string is " + self.x
            def __format__(self, format_spec):
                if format_spec == 'd':
                    return 'G(' + self.x + ')'
                return object.__format__(self, format_spec)

        class I(datetime.date):
            def __format__(self, format_spec):
                return self.strftime(format_spec)

        class J(int):
            def __format__(self, format_spec):
                return int.__format__(self * 2, format_spec)


        self.assertEqual(''.format(), '')
        self.assertEqual('abc'.format(), 'abc')
        self.assertEqual('{0}'.format('abc'), 'abc')
        self.assertEqual('{0:}'.format('abc'), 'abc')
#        self.assertEqual('{ 0 }'.format('abc'), 'abc')
        self.assertEqual('X{0}'.format('abc'), 'Xabc')
        self.assertEqual('{0}X'.format('abc'), 'abcX')
        self.assertEqual('X{0}Y'.format('abc'), 'XabcY')
        self.assertEqual('{1}'.format(1, 'abc'), 'abc')
        self.assertEqual('X{1}'.format(1, 'abc'), 'Xabc')
        self.assertEqual('{1}X'.format(1, 'abc'), 'abcX')
        self.assertEqual('X{1}Y'.format(1, 'abc'), 'XabcY')
        self.assertEqual('{0}'.format(-15), '-15')
        self.assertEqual('{0}{1}'.format(-15, 'abc'), '-15abc')
        self.assertEqual('{0}X{1}'.format(-15, 'abc'), '-15Xabc')
        self.assertEqual('{{'.format(), '{')
        self.assertEqual('}}'.format(), '}')
        self.assertEqual('{{}}'.format(), '{}')
        self.assertEqual('{{x}}'.format(), '{x}')
        self.assertEqual('{{{0}}}'.format(123), '{123}')
        self.assertEqual('{{{{0}}}}'.format(), '{{0}}')
        self.assertEqual('}}{{'.format(), '}{')
        self.assertEqual('}}x{{'.format(), '}x{')

        # weird field names
        self.assertEqual("{0[foo-bar]}".format({'foo-bar':'baz'}), 'baz')
        self.assertEqual("{0[foo bar]}".format({'foo bar':'baz'}), 'baz')
        self.assertEqual("{0[ ]}".format({' ':3}), '3')

        self.assertEqual('{foo._x}'.format(foo=C(20)), '20')
        self.assertEqual('{1}{0}'.format(D(10), D(20)), '2010')
        self.assertEqual('{0._x.x}'.format(C(D('abc'))), 'abc')
        self.assertEqual('{0[0]}'.format(['abc', 'def']), 'abc')
        self.assertEqual('{0[1]}'.format(['abc', 'def']), 'def')
        self.assertEqual('{0[1][0]}'.format(['abc', ['def']]), 'def')
        self.assertEqual('{0[1][0].x}'.format(['abc', [D('def')]]), 'def')

        # strings
        self.assertEqual('{0:.3s}'.format('abc'), 'abc')
        self.assertEqual('{0:.3s}'.format('ab'), 'ab')
        self.assertEqual('{0:.3s}'.format('abcdef'), 'abc')
        self.assertEqual('{0:.0s}'.format('abcdef'), '')
        self.assertEqual('{0:3.3s}'.format('abc'), 'abc')
        self.assertEqual('{0:2.3s}'.format('abc'), 'abc')
        self.assertEqual('{0:2.2s}'.format('abc'), 'ab')
        self.assertEqual('{0:3.2s}'.format('abc'), 'ab ')
        self.assertEqual('{0:x<0s}'.format('result'), 'result')
        self.assertEqual('{0:x<5s}'.format('result'), 'result')
        self.assertEqual('{0:x<6s}'.format('result'), 'result')
        self.assertEqual('{0:x<7s}'.format('result'), 'resultx')
        self.assertEqual('{0:x<8s}'.format('result'), 'resultxx')
        self.assertEqual('{0: <7s}'.format('result'), 'result ')
        self.assertEqual('{0:<7s}'.format('result'), 'result ')
        self.assertEqual('{0:>7s}'.format('result'), ' result')
        self.assertEqual('{0:>8s}'.format('result'), '  result')
        self.assertEqual('{0:^8s}'.format('result'), ' result ')
        self.assertEqual('{0:^9s}'.format('result'), ' result  ')
        self.assertEqual('{0:^10s}'.format('result'), '  result  ')
        self.assertEqual('{0:10000}'.format('a'), 'a' + ' ' * 9999)
        self.assertEqual('{0:10000}'.format(''), ' ' * 10000)
        self.assertEqual('{0:10000000}'.format(''), ' ' * 10000000)

        # format specifiers for user defined type
        self.assertEqual('{0:abc}'.format(C()), 'abc')

        # !r, !s and !a coercions
        self.assertEqual('{0!s}'.format('Hello'), 'Hello')
        self.assertEqual('{0!s:}'.format('Hello'), 'Hello')
        self.assertEqual('{0!s:15}'.format('Hello'), 'Hello          ')
        self.assertEqual('{0!s:15s}'.format('Hello'), 'Hello          ')
        self.assertEqual('{0!r}'.format('Hello'), "'Hello'")
        self.assertEqual('{0!r:}'.format('Hello'), "'Hello'")
        self.assertEqual('{0!r}'.format(F('Hello')), 'F(Hello)')
        self.assertEqual('{0!r}'.format('\u0378'), "'\\u0378'") # nonprintable
        self.assertEqual('{0!r}'.format('\u0374'), "'\u0374'")  # printable
        self.assertEqual('{0!r}'.format(F('\u0374')), 'F(\u0374)')
        self.assertEqual('{0!a}'.format('Hello'), "'Hello'")
        self.assertEqual('{0!a}'.format('\u0378'), "'\\u0378'") # nonprintable
        self.assertEqual('{0!a}'.format('\u0374'), "'\\u0374'") # printable
        self.assertEqual('{0!a:}'.format('Hello'), "'Hello'")
        self.assertEqual('{0!a}'.format(F('Hello')), 'F(Hello)')
        self.assertEqual('{0!a}'.format(F('\u0374')), 'F(\\u0374)')

        # test fallback to object.__format__
        self.assertEqual('{0}'.format({}), '{}')
        self.assertEqual('{0}'.format([]), '[]')
        self.assertEqual('{0}'.format([1]), '[1]')

        self.assertEqual('{0:d}'.format(G('data')), 'G(data)')
        self.assertEqual('{0!s}'.format(G('data')), 'string is data')

        msg = 'object.__format__ with a non-empty format string is deprecated'
        with support.check_warnings((msg, DeprecationWarning)):
            self.assertEqual('{0:^10}'.format(E('data')), ' E(data)  ')
            self.assertEqual('{0:^10s}'.format(E('data')), ' E(data)  ')
            self.assertEqual('{0:>15s}'.format(G('data')), ' string is data')

        self.assertEqual("{0:date: %Y-%m-%d}".format(I(year=2007,
                                                       month=8,
                                                       day=27)),
                         "date: 2007-08-27")

        # test deriving from a builtin type and overriding __format__
        self.assertEqual("{0}".format(J(10)), "20")


        # string format specifiers
        self.assertEqual('{0:}'.format('a'), 'a')

        # computed format specifiers
        self.assertEqual("{0:.{1}}".format('hello world', 5), 'hello')
        self.assertEqual("{0:.{1}s}".format('hello world', 5), 'hello')
        self.assertEqual("{0:.{precision}s}".format('hello world', precision=5), 'hello')
        self.assertEqual("{0:{width}.{precision}s}".format('hello world', width=10, precision=5), 'hello     ')
        self.assertEqual("{0:{width}.{precision}s}".format('hello world', width='10', precision='5'), 'hello     ')

        # test various errors
        self.assertRaises(ValueError, '{'.format)
        self.assertRaises(ValueError, '}'.format)
        self.assertRaises(ValueError, 'a{'.format)
        self.assertRaises(ValueError, 'a}'.format)
        self.assertRaises(ValueError, '{a'.format)
        self.assertRaises(ValueError, '}a'.format)
        self.assertRaises(IndexError, '{0}'.format)
        self.assertRaises(IndexError, '{1}'.format, 'abc')
        self.assertRaises(KeyError,   '{x}'.format)
        self.assertRaises(ValueError, "}{".format)
        self.assertRaises(ValueError, "abc{0:{}".format)
        self.assertRaises(ValueError, "{0".format)
        self.assertRaises(IndexError, "{0.}".format)
        self.assertRaises(ValueError, "{0.}".format, 0)
        self.assertRaises(IndexError, "{0[}".format)
        self.assertRaises(ValueError, "{0[}".format, [])
        self.assertRaises(KeyError,   "{0]}".format)
        self.assertRaises(ValueError, "{0.[]}".format, 0)
        self.assertRaises(ValueError, "{0..foo}".format, 0)
        self.assertRaises(ValueError, "{0[0}".format, 0)
        self.assertRaises(ValueError, "{0[0:foo}".format, 0)
        self.assertRaises(KeyError,   "{c]}".format)
        self.assertRaises(ValueError, "{{ {{{0}}".format, 0)
        self.assertRaises(ValueError, "{0}}".format, 0)
        self.assertRaises(KeyError,   "{foo}".format, bar=3)
        self.assertRaises(ValueError, "{0!x}".format, 3)
        self.assertRaises(ValueError, "{0!}".format, 0)
        self.assertRaises(ValueError, "{0!rs}".format, 0)
        self.assertRaises(ValueError, "{!}".format)
        self.assertRaises(IndexError, "{:}".format)
        self.assertRaises(IndexError, "{:s}".format)
        self.assertRaises(IndexError, "{}".format)
        big = "23098475029384702983476098230754973209482573"
        self.assertRaises(ValueError, ("{" + big + "}").format)
        self.assertRaises(ValueError, ("{[" + big + "]}").format, [0])

        # issue 6089
        self.assertRaises(ValueError, "{0[0]x}".format, [None])
        self.assertRaises(ValueError, "{0[0](10)}".format, [None])

        # can't have a replacement on the field name portion
        self.assertRaises(TypeError, '{0[{1}]}'.format, 'abcdefg', 4)

        # exceed maximum recursion depth
        self.assertRaises(ValueError, "{0:{1:{2}}}".format, 'abc', 's', '')
        self.assertRaises(ValueError, "{0:{1:{2:{3:{4:{5:{6}}}}}}}".format,
                          0, 1, 2, 3, 4, 5, 6, 7)

        # string format spec errors
        self.assertRaises(ValueError, "{0:-s}".format, '')
        self.assertRaises(ValueError, format, "", "-")
        self.assertRaises(ValueError, "{0:=s}".format, '')

        # Alternate formatting is not supported
        self.assertRaises(ValueError, format, '', '#')
        self.assertRaises(ValueError, format, '', '#20')

        # Non-ASCII
        self.assertEqual("{0:s}{1:s}".format("ABC", "\u0410\u0411\u0412"),
                         'ABC\u0410\u0411\u0412')
        self.assertEqual("{0:.3s}".format("ABC\u0410\u0411\u0412"),
                         'ABC')
        self.assertEqual("{0:.0s}".format("ABC\u0410\u0411\u0412"),
                         '')

        self.assertEqual("{[{}]}".format({"{}": 5}), "5")

    def test_format_map(self):
        self.assertEqual(''.format_map({}), '')
        self.assertEqual('a'.format_map({}), 'a')
        self.assertEqual('ab'.format_map({}), 'ab')
        self.assertEqual('a{{'.format_map({}), 'a{')
        self.assertEqual('a}}'.format_map({}), 'a}')
        self.assertEqual('{{b'.format_map({}), '{b')
        self.assertEqual('}}b'.format_map({}), '}b')
        self.assertEqual('a{{b'.format_map({}), 'a{b')

        # using mappings
        class Mapping(dict):
            def __missing__(self, key):
                return key
        self.assertEqual('{hello}'.format_map(Mapping()), 'hello')
        self.assertEqual('{a} {world}'.format_map(Mapping(a='hello')), 'hello world')

        class InternalMapping:
            def __init__(self):
                self.mapping = {'a': 'hello'}
            def __getitem__(self, key):
                return self.mapping[key]
        self.assertEqual('{a}'.format_map(InternalMapping()), 'hello')


        class C:
            def __init__(self, x=100):
                self._x = x
            def __format__(self, spec):
                return spec
        self.assertEqual('{foo._x}'.format_map({'foo': C(20)}), '20')

        # test various errors
        self.assertRaises(TypeError, ''.format_map)
        self.assertRaises(TypeError, 'a'.format_map)

        self.assertRaises(ValueError, '{'.format_map, {})
        self.assertRaises(ValueError, '}'.format_map, {})
        self.assertRaises(ValueError, 'a{'.format_map, {})
        self.assertRaises(ValueError, 'a}'.format_map, {})
        self.assertRaises(ValueError, '{a'.format_map, {})
        self.assertRaises(ValueError, '}a'.format_map, {})

        # issue #12579: can't supply positional params to format_map
        self.assertRaises(ValueError, '{}'.format_map, {'a' : 2})
        self.assertRaises(ValueError, '{}'.format_map, 'a')
        self.assertRaises(ValueError, '{a} {}'.format_map, {"a" : 2, "b" : 1})

    def test_format_huge_precision(self):
        format_string = ".{}f".format(sys.maxsize + 1)
        with self.assertRaises(ValueError):
            result = format(2.34, format_string)

    def test_format_huge_width(self):
        format_string = "{}f".format(sys.maxsize + 1)
        with self.assertRaises(ValueError):
            result = format(2.34, format_string)

    def test_format_huge_item_number(self):
        format_string = "{{{}:.6f}}".format(sys.maxsize + 1)
        with self.assertRaises(ValueError):
            result = format_string.format(2.34)

    def test_format_auto_numbering(self):
        class C:
            def __init__(self, x=100):
                self._x = x
            def __format__(self, spec):
                return spec

        self.assertEqual('{}'.format(10), '10')
        self.assertEqual('{:5}'.format('s'), 's    ')
        self.assertEqual('{!r}'.format('s'), "'s'")
        self.assertEqual('{._x}'.format(C(10)), '10')
        self.assertEqual('{[1]}'.format([1, 2]), '2')
        self.assertEqual('{[a]}'.format({'a':4, 'b':2}), '4')
        self.assertEqual('a{}b{}c'.format(0, 1), 'a0b1c')

        self.assertEqual('a{:{}}b'.format('x', '^10'), 'a    x     b')
        self.assertEqual('a{:{}x}b'.format(20, '#'), 'a0x14b')

        # can't mix and match numbering and auto-numbering
        self.assertRaises(ValueError, '{}{1}'.format, 1, 2)
        self.assertRaises(ValueError, '{1}{}'.format, 1, 2)
        self.assertRaises(ValueError, '{:{1}}'.format, 1, 2)
        self.assertRaises(ValueError, '{0:{}}'.format, 1, 2)

        # can mix and match auto-numbering and named
        self.assertEqual('{f}{}'.format(4, f='test'), 'test4')
        self.assertEqual('{}{f}'.format(4, f='test'), '4test')
        self.assertEqual('{:{f}}{g}{}'.format(1, 3, g='g', f=2), ' 1g3')
        self.assertEqual('{f:{}}{}{g}'.format(2, 4, f=1, g='g'), ' 14g')

    def test_formatting(self):
        string_tests.MixinStrUnicodeUserStringTest.test_formatting(self)
        # Testing Unicode formatting strings...
        self.assertEqual("%s, %s" % ("abc", "abc"), 'abc, abc')
        self.assertEqual("%s, %s, %i, %f, %5.2f" % ("abc", "abc", 1, 2, 3), 'abc, abc, 1, 2.000000,  3.00')
        self.assertEqual("%s, %s, %i, %f, %5.2f" % ("abc", "abc", 1, -2, 3), 'abc, abc, 1, -2.000000,  3.00')
        self.assertEqual("%s, %s, %i, %f, %5.2f" % ("abc", "abc", -1, -2, 3.5), 'abc, abc, -1, -2.000000,  3.50')
        self.assertEqual("%s, %s, %i, %f, %5.2f" % ("abc", "abc", -1, -2, 3.57), 'abc, abc, -1, -2.000000,  3.57')
        self.assertEqual("%s, %s, %i, %f, %5.2f" % ("abc", "abc", -1, -2, 1003.57), 'abc, abc, -1, -2.000000, 1003.57')
        if not sys.platform.startswith('java'):
            self.assertEqual("%r, %r" % (b"abc", "abc"), "b'abc', 'abc'")
            self.assertEqual("%r" % ("\u1234",), "'\u1234'")
            self.assertEqual("%a" % ("\u1234",), "'\\u1234'")
        self.assertEqual("%(x)s, %(y)s" % {'x':"abc", 'y':"def"}, 'abc, def')
        self.assertEqual("%(x)s, %(\xfc)s" % {'x':"abc", '\xfc':"def"}, 'abc, def')

        self.assertEqual('%c' % 0x1234, '\u1234')
        self.assertEqual('%c' % 0x21483, '\U00021483')
        self.assertRaises(OverflowError, "%c".__mod__, (0x110000,))
        self.assertEqual('%c' % '\U00021483', '\U00021483')
        self.assertRaises(TypeError, "%c".__mod__, "aa")
        self.assertRaises(ValueError, "%.1\u1032f".__mod__, (1.0/3))
        self.assertRaises(TypeError, "%i".__mod__, "aa")

        # formatting jobs delegated from the string implementation:
        self.assertEqual('...%(foo)s...' % {'foo':"abc"}, '...abc...')
        self.assertEqual('...%(foo)s...' % {'foo':"abc"}, '...abc...')
        self.assertEqual('...%(foo)s...' % {'foo':"abc"}, '...abc...')
        self.assertEqual('...%(foo)s...' % {'foo':"abc"}, '...abc...')
        self.assertEqual('...%(foo)s...' % {'foo':"abc",'def':123},  '...abc...')
        self.assertEqual('...%(foo)s...' % {'foo':"abc",'def':123}, '...abc...')
        self.assertEqual('...%s...%s...%s...%s...' % (1,2,3,"abc"), '...1...2...3...abc...')
        self.assertEqual('...%%...%%s...%s...%s...%s...%s...' % (1,2,3,"abc"), '...%...%s...1...2...3...abc...')
        self.assertEqual('...%s...' % "abc", '...abc...')
        self.assertEqual('%*s' % (5,'abc',), '  abc')
        self.assertEqual('%*s' % (-5,'abc',), 'abc  ')
        self.assertEqual('%*.*s' % (5,2,'abc',), '   ab')
        self.assertEqual('%*.*s' % (5,3,'abc',), '  abc')
        self.assertEqual('%i %*.*s' % (10, 5,3,'abc',), '10   abc')
        self.assertEqual('%i%s %*.*s' % (10, 3, 5, 3, 'abc',), '103   abc')
        self.assertEqual('%c' % 'a', 'a')
        class Wrapper:
            def __str__(self):
                return '\u1234'
        self.assertEqual('%s' % Wrapper(), '\u1234')

        # issue 3382
        NAN = float('nan')
        INF = float('inf')
        self.assertEqual('%f' % NAN, 'nan')
        self.assertEqual('%F' % NAN, 'NAN')
        self.assertEqual('%f' % INF, 'inf')
        self.assertEqual('%F' % INF, 'INF')

        # PEP 393
        self.assertEqual('%.1s' % "a\xe9\u20ac", 'a')
        self.assertEqual('%.2s' % "a\xe9\u20ac", 'a\xe9')

    @support.cpython_only
    def test_formatting_huge_precision(self):
        from _testcapi import INT_MAX
        format_string = "%.{}f".format(INT_MAX + 1)
        with self.assertRaises(ValueError):
            result = format_string % 2.34

    def test_formatting_huge_width(self):
        format_string = "%{}f".format(sys.maxsize + 1)
        with self.assertRaises(ValueError):
            result = format_string % 2.34

    def test_startswith_endswith_errors(self):
        for meth in ('foo'.startswith, 'foo'.endswith):
            with self.assertRaises(TypeError) as cm:
                meth(['f'])
            exc = str(cm.exception)
            self.assertIn('str', exc)
            self.assertIn('tuple', exc)

    @support.run_with_locale('LC_ALL', 'de_DE', 'fr_FR')
    def test_format_float(self):
        # should not format with a comma, but always with C locale
        self.assertEqual('1.0', '%.1f' % 1.0)

    def test_constructor(self):
        # unicode(obj) tests (this maps to PyObject_Unicode() at C level)

        self.assertEqual(
            str('unicode remains unicode'),
            'unicode remains unicode'
        )

        class UnicodeSubclass(str):
            pass

        for text in ('ascii', '\xe9', '\u20ac', '\U0010FFFF'):
            subclass = UnicodeSubclass(text)
            self.assertEqual(str(subclass), text)
            self.assertEqual(len(subclass), len(text))
            if text == 'ascii':
                self.assertEqual(subclass.encode('ascii'), b'ascii')
                self.assertEqual(subclass.encode('utf-8'), b'ascii')

        self.assertEqual(
            str('strings are converted to unicode'),
            'strings are converted to unicode'
        )

        class StringCompat:
            def __init__(self, x):
                self.x = x
            def __str__(self):
                return self.x

        self.assertEqual(
            str(StringCompat('__str__ compatible objects are recognized')),
            '__str__ compatible objects are recognized'
        )

        # unicode(obj) is compatible to str():

        o = StringCompat('unicode(obj) is compatible to str()')
        self.assertEqual(str(o), 'unicode(obj) is compatible to str()')
        self.assertEqual(str(o), 'unicode(obj) is compatible to str()')

        for obj in (123, 123.45, 123):
            self.assertEqual(str(obj), str(str(obj)))

        # unicode(obj, encoding, error) tests (this maps to
        # PyUnicode_FromEncodedObject() at C level)

        if not sys.platform.startswith('java'):
            self.assertRaises(
                TypeError,
                str,
                'decoding unicode is not supported',
                'utf-8',
                'strict'
            )

        self.assertEqual(
            str(b'strings are decoded to unicode', 'utf-8', 'strict'),
            'strings are decoded to unicode'
        )

        if not sys.platform.startswith('java'):
            self.assertEqual(
                str(
                    memoryview(b'character buffers are decoded to unicode'),
                    'utf-8',
                    'strict'
                ),
                'character buffers are decoded to unicode'
            )

        self.assertRaises(TypeError, str, 42, 42, 42)

    def test_constructor_keyword_args(self):
        """Pass various keyword argument combinations to the constructor."""
        # The object argument can be passed as a keyword.
        self.assertEqual(str(object='foo'), 'foo')
        self.assertEqual(str(object=b'foo', encoding='utf-8'), 'foo')
        # The errors argument without encoding triggers "decode" mode.
        self.assertEqual(str(b'foo', errors='strict'), 'foo')  # not "b'foo'"
        self.assertEqual(str(object=b'foo', errors='strict'), 'foo')

    def test_constructor_defaults(self):
        """Check the constructor argument defaults."""
        # The object argument defaults to '' or b''.
        self.assertEqual(str(), '')
        self.assertEqual(str(errors='strict'), '')
        utf8_cent = '¢'.encode('utf-8')
        # The encoding argument defaults to utf-8.
        self.assertEqual(str(utf8_cent, errors='strict'), '¢')
        # The errors argument defaults to strict.
        self.assertRaises(UnicodeDecodeError, str, utf8_cent, encoding='ascii')

    def test_codecs_utf7(self):
        utfTests = [
            ('A\u2262\u0391.', b'A+ImIDkQ.'),             # RFC2152 example
            ('Hi Mom -\u263a-!', b'Hi Mom -+Jjo--!'),     # RFC2152 example
            ('\u65E5\u672C\u8A9E', b'+ZeVnLIqe-'),        # RFC2152 example
            ('Item 3 is \u00a31.', b'Item 3 is +AKM-1.'), # RFC2152 example
            ('+', b'+-'),
            ('+-', b'+--'),
            ('+?', b'+-?'),
            ('\?', b'+AFw?'),
            ('+?', b'+-?'),
            (r'\\?', b'+AFwAXA?'),
            (r'\\\?', b'+AFwAXABc?'),
            (r'++--', b'+-+---'),
            ('\U000abcde', b'+2m/c3g-'),                  # surrogate pairs
            ('/', b'/'),
        ]

        for (x, y) in utfTests:
            self.assertEqual(x.encode('utf-7'), y)

        # Unpaired surrogates are passed through
        self.assertEqual('\uD801'.encode('utf-7'), b'+2AE-')
        self.assertEqual('\uD801x'.encode('utf-7'), b'+2AE-x')
        self.assertEqual('\uDC01'.encode('utf-7'), b'+3AE-')
        self.assertEqual('\uDC01x'.encode('utf-7'), b'+3AE-x')
        self.assertEqual(b'+2AE-'.decode('utf-7'), '\uD801')
        self.assertEqual(b'+2AE-x'.decode('utf-7'), '\uD801x')
        self.assertEqual(b'+3AE-'.decode('utf-7'), '\uDC01')
        self.assertEqual(b'+3AE-x'.decode('utf-7'), '\uDC01x')

        self.assertEqual('\uD801\U000abcde'.encode('utf-7'), b'+2AHab9ze-')
        self.assertEqual(b'+2AHab9ze-'.decode('utf-7'), '\uD801\U000abcde')

        # Issue #2242: crash on some Windows/MSVC versions
        self.assertEqual(b'+\xc1'.decode('utf-7'), '\xc1')

        # Direct encoded characters
        set_d = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'(),-./:?"
        # Optional direct characters
        set_o = '!"#$%&*;<=>@[]^_`{|}'
        for c in set_d:
            self.assertEqual(c.encode('utf7'), c.encode('ascii'))
            self.assertEqual(c.encode('ascii').decode('utf7'), c)
        for c in set_o:
            self.assertEqual(c.encode('ascii').decode('utf7'), c)

    def test_codecs_utf8(self):
        self.assertEqual(''.encode('utf-8'), b'')
        self.assertEqual('\u20ac'.encode('utf-8'), b'\xe2\x82\xac')
        self.assertEqual('\U00010002'.encode('utf-8'), b'\xf0\x90\x80\x82')
        self.assertEqual('\U00023456'.encode('utf-8'), b'\xf0\xa3\x91\x96')
        self.assertEqual('\ud800'.encode('utf-8', 'surrogatepass'), b'\xed\xa0\x80')
        self.assertEqual('\udc00'.encode('utf-8', 'surrogatepass'), b'\xed\xb0\x80')
        self.assertEqual(('\U00010002'*10).encode('utf-8'),
                         b'\xf0\x90\x80\x82'*10)
        self.assertEqual(
            '\u6b63\u78ba\u306b\u8a00\u3046\u3068\u7ffb\u8a33\u306f'
            '\u3055\u308c\u3066\u3044\u307e\u305b\u3093\u3002\u4e00'
            '\u90e8\u306f\u30c9\u30a4\u30c4\u8a9e\u3067\u3059\u304c'
            '\u3001\u3042\u3068\u306f\u3067\u305f\u3089\u3081\u3067'
            '\u3059\u3002\u5b9f\u969b\u306b\u306f\u300cWenn ist das'
            ' Nunstuck git und'.encode('utf-8'),
            b'\xe6\xad\xa3\xe7\xa2\xba\xe3\x81\xab\xe8\xa8\x80\xe3\x81'
            b'\x86\xe3\x81\xa8\xe7\xbf\xbb\xe8\xa8\xb3\xe3\x81\xaf\xe3'
            b'\x81\x95\xe3\x82\x8c\xe3\x81\xa6\xe3\x81\x84\xe3\x81\xbe'
            b'\xe3\x81\x9b\xe3\x82\x93\xe3\x80\x82\xe4\xb8\x80\xe9\x83'
            b'\xa8\xe3\x81\xaf\xe3\x83\x89\xe3\x82\xa4\xe3\x83\x84\xe8'
            b'\xaa\x9e\xe3\x81\xa7\xe3\x81\x99\xe3\x81\x8c\xe3\x80\x81'
            b'\xe3\x81\x82\xe3\x81\xa8\xe3\x81\xaf\xe3\x81\xa7\xe3\x81'
            b'\x9f\xe3\x82\x89\xe3\x82\x81\xe3\x81\xa7\xe3\x81\x99\xe3'
            b'\x80\x82\xe5\xae\x9f\xe9\x9a\x9b\xe3\x81\xab\xe3\x81\xaf'
            b'\xe3\x80\x8cWenn ist das Nunstuck git und'
        )

        # UTF-8 specific decoding tests
        self.assertEqual(str(b'\xf0\xa3\x91\x96', 'utf-8'), '\U00023456' )
        self.assertEqual(str(b'\xf0\x90\x80\x82', 'utf-8'), '\U00010002' )
        self.assertEqual(str(b'\xe2\x82\xac', 'utf-8'), '\u20ac' )

        # Other possible utf-8 test cases:
        # * strict decoding testing for all of the
        #   UTF8_ERROR cases in PyUnicode_DecodeUTF8

    def test_utf8_decode_valid_sequences(self):
        sequences = [
            # single byte
            (b'\x00', '\x00'), (b'a', 'a'), (b'\x7f', '\x7f'),
            # 2 bytes
            (b'\xc2\x80', '\x80'), (b'\xdf\xbf', '\u07ff'),
            # 3 bytes
            (b'\xe0\xa0\x80', '\u0800'), (b'\xed\x9f\xbf', '\ud7ff'),
            (b'\xee\x80\x80', '\uE000'), (b'\xef\xbf\xbf', '\uffff'),
            # 4 bytes
            (b'\xF0\x90\x80\x80', '\U00010000'),
            (b'\xf4\x8f\xbf\xbf', '\U0010FFFF')
        ]
        for seq, res in sequences:
            self.assertEqual(seq.decode('utf-8'), res)


    def test_utf8_decode_invalid_sequences(self):
        # continuation bytes in a sequence of 2, 3, or 4 bytes
        continuation_bytes = [bytes([x]) for x in range(0x80, 0xC0)]
        # start bytes of a 2-byte sequence equivalent to codepoints < 0x7F
        invalid_2B_seq_start_bytes = [bytes([x]) for x in range(0xC0, 0xC2)]
        # start bytes of a 4-byte sequence equivalent to codepoints > 0x10FFFF
        invalid_4B_seq_start_bytes = [bytes([x]) for x in range(0xF5, 0xF8)]
        invalid_start_bytes = (
            continuation_bytes + invalid_2B_seq_start_bytes +
            invalid_4B_seq_start_bytes + [bytes([x]) for x in range(0xF7, 0x100)]
        )

        for byte in invalid_start_bytes:
            self.assertRaises(UnicodeDecodeError, byte.decode, 'utf-8')

        for sb in invalid_2B_seq_start_bytes:
            for cb in continuation_bytes:
                self.assertRaises(UnicodeDecodeError, (sb+cb).decode, 'utf-8')

        for sb in invalid_4B_seq_start_bytes:
            for cb1 in continuation_bytes[:3]:
                for cb3 in continuation_bytes[:3]:
                    self.assertRaises(UnicodeDecodeError,
                                      (sb+cb1+b'\x80'+cb3).decode, 'utf-8')

        for cb in [bytes([x]) for x in range(0x80, 0xA0)]:
            self.assertRaises(UnicodeDecodeError,
                              (b'\xE0'+cb+b'\x80').decode, 'utf-8')
            self.assertRaises(UnicodeDecodeError,
                              (b'\xE0'+cb+b'\xBF').decode, 'utf-8')
        # surrogates
        for cb in [bytes([x]) for x in range(0xA0, 0xC0)]:
            self.assertRaises(UnicodeDecodeError,
                              (b'\xED'+cb+b'\x80').decode, 'utf-8')
            self.assertRaises(UnicodeDecodeError,
                              (b'\xED'+cb+b'\xBF').decode, 'utf-8')
        for cb in [bytes([x]) for x in range(0x80, 0x90)]:
            self.assertRaises(UnicodeDecodeError,
                              (b'\xF0'+cb+b'\x80\x80').decode, 'utf-8')
            self.assertRaises(UnicodeDecodeError,
                              (b'\xF0'+cb+b'\xBF\xBF').decode, 'utf-8')
        for cb in [bytes([x]) for x in range(0x90, 0xC0)]:
            self.assertRaises(UnicodeDecodeError,
                              (b'\xF4'+cb+b'\x80\x80').decode, 'utf-8')
            self.assertRaises(UnicodeDecodeError,
                              (b'\xF4'+cb+b'\xBF\xBF').decode, 'utf-8')

    def test_issue8271(self):
        # Issue #8271: during the decoding of an invalid UTF-8 byte sequence,
        # only the start byte and the continuation byte(s) are now considered
        # invalid, instead of the number of bytes specified by the start byte.
        # See http://www.unicode.org/versions/Unicode5.2.0/ch03.pdf (page 95,
        # table 3-8, Row 2) for more information about the algorithm used.
        FFFD = '\ufffd'
        sequences = [
            # invalid start bytes
            (b'\x80', FFFD), # continuation byte
            (b'\x80\x80', FFFD*2), # 2 continuation bytes
            (b'\xc0', FFFD),
            (b'\xc0\xc0', FFFD*2),
            (b'\xc1', FFFD),
            (b'\xc1\xc0', FFFD*2),
            (b'\xc0\xc1', FFFD*2),
            # with start byte of a 2-byte sequence
            (b'\xc2', FFFD), # only the start byte
            (b'\xc2\xc2', FFFD*2), # 2 start bytes
            (b'\xc2\xc2\xc2', FFFD*3), # 3 start bytes
            (b'\xc2\x41', FFFD+'A'), # invalid continuation byte
            # with start byte of a 3-byte sequence
            (b'\xe1', FFFD), # only the start byte
            (b'\xe1\xe1', FFFD*2), # 2 start bytes
            (b'\xe1\xe1\xe1', FFFD*3), # 3 start bytes
            (b'\xe1\xe1\xe1\xe1', FFFD*4), # 4 start bytes
            (b'\xe1\x80', FFFD), # only 1 continuation byte
            (b'\xe1\x41', FFFD+'A'), # invalid continuation byte
            (b'\xe1\x41\x80', FFFD+'A'+FFFD), # invalid cb followed by valid cb
            (b'\xe1\x41\x41', FFFD+'AA'), # 2 invalid continuation bytes
            (b'\xe1\x80\x41', FFFD+'A'), # only 1 valid continuation byte
            (b'\xe1\x80\xe1\x41', FFFD*2+'A'), # 1 valid and the other invalid
            (b'\xe1\x41\xe1\x80', FFFD+'A'+FFFD), # 1 invalid and the other valid
            # with start byte of a 4-byte sequence
            (b'\xf1', FFFD), # only the start byte
            (b'\xf1\xf1', FFFD*2), # 2 start bytes
            (b'\xf1\xf1\xf1', FFFD*3), # 3 start bytes
            (b'\xf1\xf1\xf1\xf1', FFFD*4), # 4 start bytes
            (b'\xf1\xf1\xf1\xf1\xf1', FFFD*5), # 5 start bytes
            (b'\xf1\x80', FFFD), # only 1 continuation bytes
            (b'\xf1\x80\x80', FFFD), # only 2 continuation bytes
            (b'\xf1\x80\x41', FFFD+'A'), # 1 valid cb and 1 invalid
            (b'\xf1\x80\x41\x41', FFFD+'AA'), # 1 valid cb and 1 invalid
            (b'\xf1\x80\x80\x41', FFFD+'A'), # 2 valid cb and 1 invalid
            (b'\xf1\x41\x80', FFFD+'A'+FFFD), # 1 invalid cv and 1 valid
            (b'\xf1\x41\x80\x80', FFFD+'A'+FFFD*2), # 1 invalid cb and 2 invalid
            (b'\xf1\x41\x80\x41', FFFD+'A'+FFFD+'A'), # 2 invalid cb and 1 invalid
            (b'\xf1\x41\x41\x80', FFFD+'AA'+FFFD), # 1 valid cb and 1 invalid
            (b'\xf1\x41\xf1\x80', FFFD+'A'+FFFD),
            (b'\xf1\x41\x80\xf1', FFFD+'A'+FFFD*2),
            (b'\xf1\xf1\x80\x41', FFFD*2+'A'),
            (b'\xf1\x41\xf1\xf1', FFFD+'A'+FFFD*2),
            # with invalid start byte of a 4-byte sequence (rfc2279)
            (b'\xf5', FFFD), # only the start byte
            (b'\xf5\xf5', FFFD*2), # 2 start bytes
            (b'\xf5\x80', FFFD*2), # only 1 continuation byte
            (b'\xf5\x80\x80', FFFD*3), # only 2 continuation byte
            (b'\xf5\x80\x80\x80', FFFD*4), # 3 continuation bytes
            (b'\xf5\x80\x41', FFFD*2+'A'), #  1 valid cb and 1 invalid
            (b'\xf5\x80\x41\xf5', FFFD*2+'A'+FFFD),
            (b'\xf5\x41\x80\x80\x41', FFFD+'A'+FFFD*2+'A'),
            # with invalid start byte of a 5-byte sequence (rfc2279)
            (b'\xf8', FFFD), # only the start byte
            (b'\xf8\xf8', FFFD*2), # 2 start bytes
            (b'\xf8\x80', FFFD*2), # only one continuation byte
            (b'\xf8\x80\x41', FFFD*2 + 'A'), # 1 valid cb and 1 invalid
            (b'\xf8\x80\x80\x80\x80', FFFD*5), # invalid 5 bytes seq with 5 bytes
            # with invalid start byte of a 6-byte sequence (rfc2279)
            (b'\xfc', FFFD), # only the start byte
            (b'\xfc\xfc', FFFD*2), # 2 start bytes
            (b'\xfc\x80\x80', FFFD*3), # only 2 continuation bytes
            (b'\xfc\x80\x80\x80\x80\x80', FFFD*6), # 6 continuation bytes
            # invalid start byte
            (b'\xfe', FFFD),
            (b'\xfe\x80\x80', FFFD*3),
            # other sequences
            (b'\xf1\x80\x41\x42\x43', '\ufffd\x41\x42\x43'),
            (b'\xf1\x80\xff\x42\x43', '\ufffd\ufffd\x42\x43'),
            (b'\xf1\x80\xc2\x81\x43', '\ufffd\x81\x43'),
            (b'\x61\xF1\x80\x80\xE1\x80\xC2\x62\x80\x63\x80\xBF\x64',
             '\x61\uFFFD\uFFFD\uFFFD\x62\uFFFD\x63\uFFFD\uFFFD\x64'),
        ]
        for n, (seq, res) in enumerate(sequences):
            self.assertRaises(UnicodeDecodeError, seq.decode, 'utf-8', 'strict')
            self.assertEqual(seq.decode('utf-8', 'replace'), res)
            self.assertEqual((seq+b'b').decode('utf-8', 'replace'), res+'b')
            self.assertEqual(seq.decode('utf-8', 'ignore'),
                             res.replace('\uFFFD', ''))

    def to_bytestring(self, seq):
        return bytes(int(c, 16) for c in seq.split())

    def assertCorrectUTF8Decoding(self, seq, res, err):
        """
        Check that an invalid UTF-8 sequence raises an UnicodeDecodeError when
        'strict' is used, returns res when 'replace' is used, and that doesn't
        return anything when 'ignore' is used.
        """
        with self.assertRaises(UnicodeDecodeError) as cm:
            seq.decode('utf-8')
        exc = cm.exception

        self.assertIn(err, str(exc))
        self.assertEqual(seq.decode('utf-8', 'replace'), res)
        self.assertEqual((b'aaaa' + seq + b'bbbb').decode('utf-8', 'replace'),
                         'aaaa' + res + 'bbbb')
        res = res.replace('\ufffd', '')
        self.assertEqual(seq.decode('utf-8', 'ignore'), res)
        self.assertEqual((b'aaaa' + seq + b'bbbb').decode('utf-8', 'ignore'),
                          'aaaa' + res + 'bbbb')

    def test_invalid_start_byte(self):
        """
        Test that an 'invalid start byte' error is raised when the first byte
        is not in the ASCII range or is not a valid start byte of a 2-, 3-, or
        4-bytes sequence. The invalid start byte is replaced with a single
        U+FFFD when errors='replace'.
        E.g. <80> is a continuation byte and can appear only after a start byte.
        """
        FFFD = '\ufffd'
        for byte in b'\x80\xA0\x9F\xBF\xC0\xC1\xF5\xFF':
            self.assertCorrectUTF8Decoding(bytes([byte]), '\ufffd',
                                           'invalid start byte')

    def test_unexpected_end_of_data(self):
        """
        Test that an 'unexpected end of data' error is raised when the string
        ends after a start byte of a 2-, 3-, or 4-bytes sequence without having
        enough continuation bytes.  The incomplete sequence is replaced with a
        single U+FFFD when errors='replace'.
        E.g. in the sequence <F3 80 80>, F3 is the start byte of a 4-bytes
        sequence, but it's followed by only 2 valid continuation bytes and the
        last continuation bytes is missing.
        Note: the continuation bytes must be all valid, if one of them is
        invalid another error will be raised.
        """
        sequences = [
            'C2', 'DF',
            'E0 A0', 'E0 BF', 'E1 80', 'E1 BF', 'EC 80', 'EC BF',
            'ED 80', 'ED 9F', 'EE 80', 'EE BF', 'EF 80', 'EF BF',
            'F0 90', 'F0 BF', 'F0 90 80', 'F0 90 BF', 'F0 BF 80', 'F0 BF BF',
            'F1 80', 'F1 BF', 'F1 80 80', 'F1 80 BF', 'F1 BF 80', 'F1 BF BF',
            'F3 80', 'F3 BF', 'F3 80 80', 'F3 80 BF', 'F3 BF 80', 'F3 BF BF',
            'F4 80', 'F4 8F', 'F4 80 80', 'F4 80 BF', 'F4 8F 80', 'F4 8F BF'
        ]
        FFFD = '\ufffd'
        for seq in sequences:
            self.assertCorrectUTF8Decoding(self.to_bytestring(seq), '\ufffd',
                                           'unexpected end of data')

    def test_invalid_cb_for_2bytes_seq(self):
        """
        Test that an 'invalid continuation byte' error is raised when the
        continuation byte of a 2-bytes sequence is invalid.  The start byte
        is replaced by a single U+FFFD and the second byte is handled
        separately when errors='replace'.
        E.g. in the sequence <C2 41>, C2 is the start byte of a 2-bytes
        sequence, but 41 is not a valid continuation byte because it's the
        ASCII letter 'A'.
        """
        FFFD = '\ufffd'
        FFFDx2 = FFFD * 2
        sequences = [
            ('C2 00', FFFD+'\x00'), ('C2 7F', FFFD+'\x7f'),
            ('C2 C0', FFFDx2), ('C2 FF', FFFDx2),
            ('DF 00', FFFD+'\x00'), ('DF 7F', FFFD+'\x7f'),
            ('DF C0', FFFDx2), ('DF FF', FFFDx2),
        ]
        for seq, res in sequences:
            self.assertCorrectUTF8Decoding(self.to_bytestring(seq), res,
                                           'invalid continuation byte')

    def test_invalid_cb_for_3bytes_seq(self):
        """
        Test that an 'invalid continuation byte' error is raised when the
        continuation byte(s) of a 3-bytes sequence are invalid.  When
        errors='replace', if the first continuation byte is valid, the first
        two bytes (start byte + 1st cb) are replaced by a single U+FFFD and the
        third byte is handled separately, otherwise only the start byte is
        replaced with a U+FFFD and the other continuation bytes are handled
        separately.
        E.g. in the sequence <E1 80 41>, E1 is the start byte of a 3-bytes
        sequence, 80 is a valid continuation byte, but 41 is not a valid cb
        because it's the ASCII letter 'A'.
        Note: when the start byte is E0 or ED, the valid ranges for the first
        continuation byte are limited to A0..BF and 80..9F respectively.
        Python 2 used to consider all the bytes in range 80..BF valid when the
        start byte was ED.  This is fixed in Python 3.
        """
        FFFD = '\ufffd'
        FFFDx2 = FFFD * 2
        sequences = [
            ('E0 00', FFFD+'\x00'), ('E0 7F', FFFD+'\x7f'), ('E0 80', FFFDx2),
            ('E0 9F', FFFDx2), ('E0 C0', FFFDx2), ('E0 FF', FFFDx2),
            ('E0 A0 00', FFFD+'\x00'), ('E0 A0 7F', FFFD+'\x7f'),
            ('E0 A0 C0', FFFDx2), ('E0 A0 FF', FFFDx2),
            ('E0 BF 00', FFFD+'\x00'), ('E0 BF 7F', FFFD+'\x7f'),
            ('E0 BF C0', FFFDx2), ('E0 BF FF', FFFDx2), ('E1 00', FFFD+'\x00'),
            ('E1 7F', FFFD+'\x7f'), ('E1 C0', FFFDx2), ('E1 FF', FFFDx2),
            ('E1 80 00', FFFD+'\x00'), ('E1 80 7F', FFFD+'\x7f'),
            ('E1 80 C0', FFFDx2), ('E1 80 FF', FFFDx2),
            ('E1 BF 00', FFFD+'\x00'), ('E1 BF 7F', FFFD+'\x7f'),
            ('E1 BF C0', FFFDx2), ('E1 BF FF', FFFDx2), ('EC 00', FFFD+'\x00'),
            ('EC 7F', FFFD+'\x7f'), ('EC C0', FFFDx2), ('EC FF', FFFDx2),
            ('EC 80 00', FFFD+'\x00'), ('EC 80 7F', FFFD+'\x7f'),
            ('EC 80 C0', FFFDx2), ('EC 80 FF', FFFDx2),
            ('EC BF 00', FFFD+'\x00'), ('EC BF 7F', FFFD+'\x7f'),
            ('EC BF C0', FFFDx2), ('EC BF FF', FFFDx2), ('ED 00', FFFD+'\x00'),
            ('ED 7F', FFFD+'\x7f'),
            ('ED A0', FFFDx2), ('ED BF', FFFDx2), # see note ^
            ('ED C0', FFFDx2), ('ED FF', FFFDx2), ('ED 80 00', FFFD+'\x00'),
            ('ED 80 7F', FFFD+'\x7f'), ('ED 80 C0', FFFDx2),
            ('ED 80 FF', FFFDx2), ('ED 9F 00', FFFD+'\x00'),
            ('ED 9F 7F', FFFD+'\x7f'), ('ED 9F C0', FFFDx2),
            ('ED 9F FF', FFFDx2), ('EE 00', FFFD+'\x00'),
            ('EE 7F', FFFD+'\x7f'), ('EE C0', FFFDx2), ('EE FF', FFFDx2),
            ('EE 80 00', FFFD+'\x00'), ('EE 80 7F', FFFD+'\x7f'),
            ('EE 80 C0', FFFDx2), ('EE 80 FF', FFFDx2),
            ('EE BF 00', FFFD+'\x00'), ('EE BF 7F', FFFD+'\x7f'),
            ('EE BF C0', FFFDx2), ('EE BF FF', FFFDx2), ('EF 00', FFFD+'\x00'),
            ('EF 7F', FFFD+'\x7f'), ('EF C0', FFFDx2), ('EF FF', FFFDx2),
            ('EF 80 00', FFFD+'\x00'), ('EF 80 7F', FFFD+'\x7f'),
            ('EF 80 C0', FFFDx2), ('EF 80 FF', FFFDx2),
            ('EF BF 00', FFFD+'\x00'), ('EF BF 7F', FFFD+'\x7f'),
            ('EF BF C0', FFFDx2), ('EF BF FF', FFFDx2),
        ]
        for seq, res in sequences:
            self.assertCorrectUTF8Decoding(self.to_bytestring(seq), res,
                                           'invalid continuation byte')

    def test_invalid_cb_for_4bytes_seq(self):
        """
        Test that an 'invalid continuation byte' error is raised when the
        continuation byte(s) of a 4-bytes sequence are invalid.  When
        errors='replace',the start byte and all the following valid
        continuation bytes are replaced with a single U+FFFD, and all the bytes
        starting from the first invalid continuation bytes (included) are
        handled separately.
        E.g. in the sequence <E1 80 41>, E1 is the start byte of a 3-bytes
        sequence, 80 is a valid continuation byte, but 41 is not a valid cb
        because it's the ASCII letter 'A'.
        Note: when the start byte is E0 or ED, the valid ranges for the first
        continuation byte are limited to A0..BF and 80..9F respectively.
        However, when the start byte is ED, Python 2 considers all the bytes
        in range 80..BF valid.  This is fixed in Python 3.
        """
        FFFD = '\ufffd'
        FFFDx2 = FFFD * 2
        sequences = [
            ('F0 00', FFFD+'\x00'), ('F0 7F', FFFD+'\x7f'), ('F0 80', FFFDx2),
            ('F0 8F', FFFDx2), ('F0 C0', FFFDx2), ('F0 FF', FFFDx2),
            ('F0 90 00', FFFD+'\x00'), ('F0 90 7F', FFFD+'\x7f'),
            ('F0 90 C0', FFFDx2), ('F0 90 FF', FFFDx2),
            ('F0 BF 00', FFFD+'\x00'), ('F0 BF 7F', FFFD+'\x7f'),
            ('F0 BF C0', FFFDx2), ('F0 BF FF', FFFDx2),
            ('F0 90 80 00', FFFD+'\x00'), ('F0 90 80 7F', FFFD+'\x7f'),
            ('F0 90 80 C0', FFFDx2), ('F0 90 80 FF', FFFDx2),
            ('F0 90 BF 00', FFFD+'\x00'), ('F0 90 BF 7F', FFFD+'\x7f'),
            ('F0 90 BF C0', FFFDx2), ('F0 90 BF FF', FFFDx2),
            ('F0 BF 80 00', FFFD+'\x00'), ('F0 BF 80 7F', FFFD+'\x7f'),
            ('F0 BF 80 C0', FFFDx2), ('F0 BF 80 FF', FFFDx2),
            ('F0 BF BF 00', FFFD+'\x00'), ('F0 BF BF 7F', FFFD+'\x7f'),
            ('F0 BF BF C0', FFFDx2), ('F0 BF BF FF', FFFDx2),
            ('F1 00', FFFD+'\x00'), ('F1 7F', FFFD+'\x7f'), ('F1 C0', FFFDx2),
            ('F1 FF', FFFDx2), ('F1 80 00', FFFD+'\x00'),
            ('F1 80 7F', FFFD+'\x7f'), ('F1 80 C0', FFFDx2),
            ('F1 80 FF', FFFDx2), ('F1 BF 00', FFFD+'\x00'),
            ('F1 BF 7F', FFFD+'\x7f'), ('F1 BF C0', FFFDx2),
            ('F1 BF FF', FFFDx2), ('F1 80 80 00', FFFD+'\x00'),
            ('F1 80 80 7F', FFFD+'\x7f'), ('F1 80 80 C0', FFFDx2),
            ('F1 80 80 FF', FFFDx2), ('F1 80 BF 00', FFFD+'\x00'),
            ('F1 80 BF 7F', FFFD+'\x7f'), ('F1 80 BF C0', FFFDx2),
            ('F1 80 BF FF', FFFDx2), ('F1 BF 80 00', FFFD+'\x00'),
            ('F1 BF 80 7F', FFFD+'\x7f'), ('F1 BF 80 C0', FFFDx2),
            ('F1 BF 80 FF', FFFDx2), ('F1 BF BF 00', FFFD+'\x00'),
            ('F1 BF BF 7F', FFFD+'\x7f'), ('F1 BF BF C0', FFFDx2),
            ('F1 BF BF FF', FFFDx2), ('F3 00', FFFD+'\x00'),
            ('F3 7F', FFFD+'\x7f'), ('F3 C0', FFFDx2), ('F3 FF', FFFDx2),
            ('F3 80 00', FFFD+'\x00'), ('F3 80 7F', FFFD+'\x7f'),
            ('F3 80 C0', FFFDx2), ('F3 80 FF', FFFDx2),
            ('F3 BF 00', FFFD+'\x00'), ('F3 BF 7F', FFFD+'\x7f'),
            ('F3 BF C0', FFFDx2), ('F3 BF FF', FFFDx2),
            ('F3 80 80 00', FFFD+'\x00'), ('F3 80 80 7F', FFFD+'\x7f'),
            ('F3 80 80 C0', FFFDx2), ('F3 80 80 FF', FFFDx2),
            ('F3 80 BF 00', FFFD+'\x00'), ('F3 80 BF 7F', FFFD+'\x7f'),
            ('F3 80 BF C0', FFFDx2), ('F3 80 BF FF', FFFDx2),
            ('F3 BF 80 00', FFFD+'\x00'), ('F3 BF 80 7F', FFFD+'\x7f'),
            ('F3 BF 80 C0', FFFDx2), ('F3 BF 80 FF', FFFDx2),
            ('F3 BF BF 00', FFFD+'\x00'), ('F3 BF BF 7F', FFFD+'\x7f'),
            ('F3 BF BF C0', FFFDx2), ('F3 BF BF FF', FFFDx2),
            ('F4 00', FFFD+'\x00'), ('F4 7F', FFFD+'\x7f'), ('F4 90', FFFDx2),
            ('F4 BF', FFFDx2), ('F4 C0', FFFDx2), ('F4 FF', FFFDx2),
            ('F4 80 00', FFFD+'\x00'), ('F4 80 7F', FFFD+'\x7f'),
            ('F4 80 C0', FFFDx2), ('F4 80 FF', FFFDx2),
            ('F4 8F 00', FFFD+'\x00'), ('F4 8F 7F', FFFD+'\x7f'),
            ('F4 8F C0', FFFDx2), ('F4 8F FF', FFFDx2),
            ('F4 80 80 00', FFFD+'\x00'), ('F4 80 80 7F', FFFD+'\x7f'),
            ('F4 80 80 C0', FFFDx2), ('F4 80 80 FF', FFFDx2),
            ('F4 80 BF 00', FFFD+'\x00'), ('F4 80 BF 7F', FFFD+'\x7f'),
            ('F4 80 BF C0', FFFDx2), ('F4 80 BF FF', FFFDx2),
            ('F4 8F 80 00', FFFD+'\x00'), ('F4 8F 80 7F', FFFD+'\x7f'),
            ('F4 8F 80 C0', FFFDx2), ('F4 8F 80 FF', FFFDx2),
            ('F4 8F BF 00', FFFD+'\x00'), ('F4 8F BF 7F', FFFD+'\x7f'),
            ('F4 8F BF C0', FFFDx2), ('F4 8F BF FF', FFFDx2)
        ]
        for seq, res in sequences:
            self.assertCorrectUTF8Decoding(self.to_bytestring(seq), res,
                                           'invalid continuation byte')

    def test_codecs_idna(self):
        # Test whether trailing dot is preserved
        self.assertEqual("www.python.org.".encode("idna"), b"www.python.org.")

    def test_codecs_errors(self):
        # Error handling (encoding)
        self.assertRaises(UnicodeError, 'Andr\202 x'.encode, 'ascii')
        self.assertRaises(UnicodeError, 'Andr\202 x'.encode, 'ascii','strict')
        self.assertEqual('Andr\202 x'.encode('ascii','ignore'), b"Andr x")
        self.assertEqual('Andr\202 x'.encode('ascii','replace'), b"Andr? x")
        self.assertEqual('Andr\202 x'.encode('ascii', 'replace'),
                         'Andr\202 x'.encode('ascii', errors='replace'))
        self.assertEqual('Andr\202 x'.encode('ascii', 'ignore'),
                         'Andr\202 x'.encode(encoding='ascii', errors='ignore'))

        # Error handling (decoding)
        self.assertRaises(UnicodeError, str, b'Andr\202 x', 'ascii')
        self.assertRaises(UnicodeError, str, b'Andr\202 x', 'ascii', 'strict')
        self.assertEqual(str(b'Andr\202 x', 'ascii', 'ignore'), "Andr x")
        self.assertEqual(str(b'Andr\202 x', 'ascii', 'replace'), 'Andr\uFFFD x')

        # Error handling (unknown character names)
        self.assertEqual(b"\\N{foo}xx".decode("unicode-escape", "ignore"), "xx")

        # Error handling (truncated escape sequence)
        self.assertRaises(UnicodeError, b"\\".decode, "unicode-escape")

        self.assertRaises(TypeError, b"hello".decode, "test.unicode1")
        self.assertRaises(TypeError, str, b"hello", "test.unicode2")
        self.assertRaises(TypeError, "hello".encode, "test.unicode1")
        self.assertRaises(TypeError, "hello".encode, "test.unicode2")

        # Error handling (wrong arguments)
        self.assertRaises(TypeError, "hello".encode, 42, 42, 42)

        # Error handling (lone surrogate in PyUnicode_TransformDecimalToASCII())
        self.assertRaises(UnicodeError, float, "\ud800")
        self.assertRaises(UnicodeError, float, "\udf00")
        self.assertRaises(UnicodeError, complex, "\ud800")
        self.assertRaises(UnicodeError, complex, "\udf00")

    def test_codecs(self):
        # Encoding
        self.assertEqual('hello'.encode('ascii'), b'hello')
        self.assertEqual('hello'.encode('utf-7'), b'hello')
        self.assertEqual('hello'.encode('utf-8'), b'hello')
        self.assertEqual('hello'.encode('utf-8'), b'hello')
        self.assertEqual('hello'.encode('utf-16-le'), b'h\000e\000l\000l\000o\000')
        self.assertEqual('hello'.encode('utf-16-be'), b'\000h\000e\000l\000l\000o')
        self.assertEqual('hello'.encode('latin-1'), b'hello')

        # Default encoding is utf-8
        self.assertEqual('\u2603'.encode(), b'\xe2\x98\x83')

        # Roundtrip safety for BMP (just the first 1024 chars)
        for c in range(1024):
            u = chr(c)
            for encoding in ('utf-7', 'utf-8', 'utf-16', 'utf-16-le',
                             'utf-16-be', 'raw_unicode_escape',
                             'unicode_escape', 'unicode_internal'):
                with warnings.catch_warnings():
                    # unicode-internal has been deprecated
                    warnings.simplefilter("ignore", DeprecationWarning)

                    self.assertEqual(str(u.encode(encoding),encoding), u)

        # Roundtrip safety for BMP (just the first 256 chars)
        for c in range(256):
            u = chr(c)
            for encoding in ('latin-1',):
                self.assertEqual(str(u.encode(encoding),encoding), u)

        # Roundtrip safety for BMP (just the first 128 chars)
        for c in range(128):
            u = chr(c)
            for encoding in ('ascii',):
                self.assertEqual(str(u.encode(encoding),encoding), u)

        # Roundtrip safety for non-BMP (just a few chars)
        with warnings.catch_warnings():
            # unicode-internal has been deprecated
            warnings.simplefilter("ignore", DeprecationWarning)

            u = '\U00010001\U00020002\U00030003\U00040004\U00050005'
            for encoding in ('utf-8', 'utf-16', 'utf-16-le', 'utf-16-be',
                             'raw_unicode_escape',
                             'unicode_escape', 'unicode_internal'):
                self.assertEqual(str(u.encode(encoding),encoding), u)

        # UTF-8 must be roundtrip safe for all code points
        # (except surrogates, which are forbidden).
        u = ''.join(map(chr, list(range(0, 0xd800)) +
                             list(range(0xe000, 0x110000))))
        for encoding in ('utf-8',):
            self.assertEqual(str(u.encode(encoding),encoding), u)

    def test_codecs_charmap(self):
        # 0-127
        s = bytes(range(128))
        for encoding in (
            'cp037', 'cp1026',
            'cp437', 'cp500', 'cp720', 'cp737', 'cp775', 'cp850',
            'cp852', 'cp855', 'cp858', 'cp860', 'cp861', 'cp862',
            'cp863', 'cp865', 'cp866',
            'iso8859_10', 'iso8859_13', 'iso8859_14', 'iso8859_15',
            'iso8859_2', 'iso8859_3', 'iso8859_4', 'iso8859_5', 'iso8859_6',
            'iso8859_7', 'iso8859_9', 'koi8_r', 'latin_1',
            'mac_cyrillic', 'mac_latin2',

            'cp1250', 'cp1251', 'cp1252', 'cp1253', 'cp1254', 'cp1255',
            'cp1256', 'cp1257', 'cp1258',
            'cp856', 'cp857', 'cp864', 'cp869', 'cp874',

            'mac_greek', 'mac_iceland','mac_roman', 'mac_turkish',
            'cp1006', 'iso8859_8',

            ### These have undefined mappings:
            #'cp424',

            ### These fail the round-trip:
            #'cp875'

            ):
            self.assertEqual(str(s, encoding).encode(encoding), s)

        # 128-255
        s = bytes(range(128, 256))
        for encoding in (
            'cp037', 'cp1026',
            'cp437', 'cp500', 'cp720', 'cp737', 'cp775', 'cp850',
            'cp852', 'cp855', 'cp858', 'cp860', 'cp861', 'cp862',
            'cp863', 'cp865', 'cp866',
            'iso8859_10', 'iso8859_13', 'iso8859_14', 'iso8859_15',
            'iso8859_2', 'iso8859_4', 'iso8859_5',
            'iso8859_9', 'koi8_r', 'latin_1',
            'mac_cyrillic', 'mac_latin2',

            ### These have undefined mappings:
            #'cp1250', 'cp1251', 'cp1252', 'cp1253', 'cp1254', 'cp1255',
            #'cp1256', 'cp1257', 'cp1258',
            #'cp424', 'cp856', 'cp857', 'cp864', 'cp869', 'cp874',
            #'iso8859_3', 'iso8859_6', 'iso8859_7',
            #'mac_greek', 'mac_iceland','mac_roman', 'mac_turkish',

            ### These fail the round-trip:
            #'cp1006', 'cp875', 'iso8859_8',

            ):
            self.assertEqual(str(s, encoding).encode(encoding), s)

    def test_concatenation(self):
        self.assertEqual(("abc" "def"), "abcdef")
        self.assertEqual(("abc" "def"), "abcdef")
        self.assertEqual(("abc" "def"), "abcdef")
        self.assertEqual(("abc" "def" "ghi"), "abcdefghi")
        self.assertEqual(("abc" "def" "ghi"), "abcdefghi")

    def test_printing(self):
        class BitBucket:
            def write(self, text):
                pass

        out = BitBucket()
        print('abc', file=out)
        print('abc', 'def', file=out)
        print('abc', 'def', file=out)
        print('abc', 'def', file=out)
        print('abc\n', file=out)
        print('abc\n', end=' ', file=out)
        print('abc\n', end=' ', file=out)
        print('def\n', file=out)
        print('def\n', file=out)

    def test_ucs4(self):
        x = '\U00100000'
        y = x.encode("raw-unicode-escape").decode("raw-unicode-escape")
        self.assertEqual(x, y)

        y = br'\U00100000'
        x = y.decode("raw-unicode-escape").encode("raw-unicode-escape")
        self.assertEqual(x, y)
        y = br'\U00010000'
        x = y.decode("raw-unicode-escape").encode("raw-unicode-escape")
        self.assertEqual(x, y)

        try:
            br'\U11111111'.decode("raw-unicode-escape")
        except UnicodeDecodeError as e:
            self.assertEqual(e.start, 0)
            self.assertEqual(e.end, 10)
        else:
            self.fail("Should have raised UnicodeDecodeError")

    def test_conversion(self):
        # Make sure __unicode__() works properly
        class Foo0:
            def __str__(self):
                return "foo"

        class Foo1:
            def __str__(self):
                return "foo"

        class Foo2(object):
            def __str__(self):
                return "foo"

        class Foo3(object):
            def __str__(self):
                return "foo"

        class Foo4(str):
            def __str__(self):
                return "foo"

        class Foo5(str):
            def __str__(self):
                return "foo"

        class Foo6(str):
            def __str__(self):
                return "foos"

            def __str__(self):
                return "foou"

        class Foo7(str):
            def __str__(self):
                return "foos"
            def __str__(self):
                return "foou"

        class Foo8(str):
            def __new__(cls, content=""):
                return str.__new__(cls, 2*content)
            def __str__(self):
                return self

        class Foo9(str):
            def __str__(self):
                return "not unicode"

        self.assertEqual(str(Foo0()), "foo")
        self.assertEqual(str(Foo1()), "foo")
        self.assertEqual(str(Foo2()), "foo")
        self.assertEqual(str(Foo3()), "foo")
        self.assertEqual(str(Foo4("bar")), "foo")
        self.assertEqual(str(Foo5("bar")), "foo")
        self.assertEqual(str(Foo6("bar")), "foou")
        self.assertEqual(str(Foo7("bar")), "foou")
        self.assertEqual(str(Foo8("foo")), "foofoo")
        self.assertEqual(str(Foo9("foo")), "not unicode")

    def test_unicode_repr(self):
        class s1:
            def __repr__(self):
                return '\\n'

        class s2:
            def __repr__(self):
                return '\\n'

        self.assertEqual(repr(s1()), '\\n')
        self.assertEqual(repr(s2()), '\\n')

    def test_printable_repr(self):
        self.assertEqual(repr('\U00010000'), "'%c'" % (0x10000,)) # printable
        self.assertEqual(repr('\U00014000'), "'\\U00014000'")     # nonprintable

    def test_expandtabs_overflows_gracefully(self):
        # This test only affects 32-bit platforms because expandtabs can only take
        # an int as the max value, not a 64-bit C long.  If expandtabs is changed
        # to take a 64-bit long, this test should apply to all platforms.
        if sys.maxsize > (1 << 32) or struct.calcsize('P') != 4:
            return
        self.assertRaises(OverflowError, 't\tt\t'.expandtabs, sys.maxsize)

    @support.cpython_only
    def test_expandtabs_optimization(self):
        s = 'abc'
        self.assertIs(s.expandtabs(), s)

    def test_raiseMemError(self):
        if struct.calcsize('P') == 8:
            # 64 bits pointers
            ascii_struct_size = 48
            compact_struct_size = 72
        else:
            # 32 bits pointers
            ascii_struct_size = 24
            compact_struct_size = 36

        for char in ('a', '\xe9', '\u20ac', '\U0010ffff'):
            code = ord(char)
            if code < 0x100:
                char_size = 1  # sizeof(Py_UCS1)
                struct_size = ascii_struct_size
            elif code < 0x10000:
                char_size = 2  # sizeof(Py_UCS2)
                struct_size = compact_struct_size
            else:
                char_size = 4  # sizeof(Py_UCS4)
                struct_size = compact_struct_size
            # Note: sys.maxsize is half of the actual max allocation because of
            # the signedness of Py_ssize_t. Strings of maxlen-1 should in principle
            # be allocatable, given enough memory.
            maxlen = ((sys.maxsize - struct_size) // char_size)
            alloc = lambda: char * maxlen
            self.assertRaises(MemoryError, alloc)
            self.assertRaises(MemoryError, alloc)

    def test_format_subclass(self):
        class S(str):
            def __str__(self):
                return '__str__ overridden'
        s = S('xxx')
        self.assertEqual("%s" % s, '__str__ overridden')
        self.assertEqual("{}".format(s), '__str__ overridden')

    # Test PyUnicode_FromFormat()
    def test_from_format(self):
        support.import_module('ctypes')
        from ctypes import (pythonapi, py_object,
            c_int, c_long, c_longlong, c_ssize_t,
            c_uint, c_ulong, c_ulonglong, c_size_t)
        name = "PyUnicode_FromFormat"
        _PyUnicode_FromFormat = getattr(pythonapi, name)
        _PyUnicode_FromFormat.restype = py_object

        def PyUnicode_FromFormat(format, *args):
            cargs = tuple(
                py_object(arg) if isinstance(arg, str) else arg
                for arg in args)
            return _PyUnicode_FromFormat(format, *cargs)

        # ascii format, non-ascii argument
        text = PyUnicode_FromFormat(b'ascii\x7f=%U', 'unicode\xe9')
        self.assertEqual(text, 'ascii\x7f=unicode\xe9')

        # non-ascii format, ascii argument: ensure that PyUnicode_FromFormatV()
        # raises an error
        self.assertRaisesRegex(ValueError,
            '^PyUnicode_FromFormatV\(\) expects an ASCII-encoded format '
            'string, got a non-ASCII byte: 0xe9$',
            PyUnicode_FromFormat, b'unicode\xe9=%s', 'ascii')

        # test "%c"
        self.assertEqual(PyUnicode_FromFormat(b'%c', c_int(0xabcd)), '\uabcd')
        self.assertEqual(PyUnicode_FromFormat(b'%c', c_int(0x10ffff)), '\U0010ffff')
        with self.assertRaises(OverflowError):
            PyUnicode_FromFormat(b'%c', c_int(0x110000))
        # Issue #18183
        self.assertEqual(
            PyUnicode_FromFormat(b'%c%c', c_int(0x10000), c_int(0x100000)),
            '\U00010000\U00100000')

        # test "%"
        self.assertEqual(PyUnicode_FromFormat(b'%'), '%')
        self.assertEqual(PyUnicode_FromFormat(b'%%'), '%')
        self.assertEqual(PyUnicode_FromFormat(b'%%s'), '%s')
        self.assertEqual(PyUnicode_FromFormat(b'[%%]'), '[%]')
        self.assertEqual(PyUnicode_FromFormat(b'%%%s', b'abc'), '%abc')

        # test integer formats (%i, %d, %u)
        self.assertEqual(PyUnicode_FromFormat(b'%03i', c_int(10)), '010')
        self.assertEqual(PyUnicode_FromFormat(b'%0.4i', c_int(10)), '0010')
        self.assertEqual(PyUnicode_FromFormat(b'%i', c_int(-123)), '-123')
        self.assertEqual(PyUnicode_FromFormat(b'%li', c_long(-123)), '-123')
        self.assertEqual(PyUnicode_FromFormat(b'%lli', c_longlong(-123)), '-123')
        self.assertEqual(PyUnicode_FromFormat(b'%zi', c_ssize_t(-123)), '-123')

        self.assertEqual(PyUnicode_FromFormat(b'%d', c_int(-123)), '-123')
        self.assertEqual(PyUnicode_FromFormat(b'%ld', c_long(-123)), '-123')
        self.assertEqual(PyUnicode_FromFormat(b'%lld', c_longlong(-123)), '-123')
        self.assertEqual(PyUnicode_FromFormat(b'%zd', c_ssize_t(-123)), '-123')

        self.assertEqual(PyUnicode_FromFormat(b'%u', c_uint(123)), '123')
        self.assertEqual(PyUnicode_FromFormat(b'%lu', c_ulong(123)), '123')
        self.assertEqual(PyUnicode_FromFormat(b'%llu', c_ulonglong(123)), '123')
        self.assertEqual(PyUnicode_FromFormat(b'%zu', c_size_t(123)), '123')

        # test %A
        text = PyUnicode_FromFormat(b'%%A:%A', 'abc\xe9\uabcd\U0010ffff')
        self.assertEqual(text, r"%A:'abc\xe9\uabcd\U0010ffff'")

        # test %V
        text = PyUnicode_FromFormat(b'repr=%V', 'abc', b'xyz')
        self.assertEqual(text, 'repr=abc')

        # Test string decode from parameter of %s using utf-8.
        # b'\xe4\xba\xba\xe6\xb0\x91' is utf-8 encoded byte sequence of
        # '\u4eba\u6c11'
        text = PyUnicode_FromFormat(b'repr=%V', None, b'\xe4\xba\xba\xe6\xb0\x91')
        self.assertEqual(text, 'repr=\u4eba\u6c11')

        #Test replace error handler.
        text = PyUnicode_FromFormat(b'repr=%V', None, b'abc\xff')
        self.assertEqual(text, 'repr=abc\ufffd')

        # not supported: copy the raw format string. these tests are just here
        # to check for crashs and should not be considered as specifications
        self.assertEqual(PyUnicode_FromFormat(b'%1%s', b'abc'), '%s')
        self.assertEqual(PyUnicode_FromFormat(b'%1abc'), '%1abc')
        self.assertEqual(PyUnicode_FromFormat(b'%+i', c_int(10)), '%+i')
        self.assertEqual(PyUnicode_FromFormat(b'%.%s', b'abc'), '%.%s')

    # Test PyUnicode_AsWideChar()
    def test_aswidechar(self):
        from _testcapi import unicode_aswidechar
        support.import_module('ctypes')
        from ctypes import c_wchar, sizeof

        wchar, size = unicode_aswidechar('abcdef', 2)
        self.assertEqual(size, 2)
        self.assertEqual(wchar, 'ab')

        wchar, size = unicode_aswidechar('abc', 3)
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc')

        wchar, size = unicode_aswidechar('abc', 4)
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc\0')

        wchar, size = unicode_aswidechar('abc', 10)
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc\0')

        wchar, size = unicode_aswidechar('abc\0def', 20)
        self.assertEqual(size, 7)
        self.assertEqual(wchar, 'abc\0def\0')

        nonbmp = chr(0x10ffff)
        if sizeof(c_wchar) == 2:
            buflen = 3
            nchar = 2
        else: # sizeof(c_wchar) == 4
            buflen = 2
            nchar = 1
        wchar, size = unicode_aswidechar(nonbmp, buflen)
        self.assertEqual(size, nchar)
        self.assertEqual(wchar, nonbmp + '\0')

    # Test PyUnicode_AsWideCharString()
    def test_aswidecharstring(self):
        from _testcapi import unicode_aswidecharstring
        support.import_module('ctypes')
        from ctypes import c_wchar, sizeof

        wchar, size = unicode_aswidecharstring('abc')
        self.assertEqual(size, 3)
        self.assertEqual(wchar, 'abc\0')

        wchar, size = unicode_aswidecharstring('abc\0def')
        self.assertEqual(size, 7)
        self.assertEqual(wchar, 'abc\0def\0')

        nonbmp = chr(0x10ffff)
        if sizeof(c_wchar) == 2:
            nchar = 2
        else: # sizeof(c_wchar) == 4
            nchar = 1
        wchar, size = unicode_aswidecharstring(nonbmp)
        self.assertEqual(size, nchar)
        self.assertEqual(wchar, nonbmp + '\0')

    def test_subclass_add(self):
        class S(str):
            def __add__(self, o):
                return "3"
        self.assertEqual(S("4") + S("5"), "3")
        class S(str):
            def __iadd__(self, o):
                return "3"
        s = S("1")
        s += "4"
        self.assertEqual(s, "3")

    def test_encode_decimal(self):
        from _testcapi import unicode_encodedecimal
        self.assertEqual(unicode_encodedecimal('123'),
                         b'123')
        self.assertEqual(unicode_encodedecimal('\u0663.\u0661\u0664'),
                         b'3.14')
        self.assertEqual(unicode_encodedecimal("\N{EM SPACE}3.14\N{EN SPACE}"),
                         b' 3.14 ')
        self.assertRaises(UnicodeEncodeError,
                          unicode_encodedecimal, "123\u20ac", "strict")
        self.assertRaisesRegex(
            ValueError,
            "^'decimal' codec can't encode character",
            unicode_encodedecimal, "123\u20ac", "replace")

    def test_transform_decimal(self):
        from _testcapi import unicode_transformdecimaltoascii as transform_decimal
        self.assertEqual(transform_decimal('123'),
                         '123')
        self.assertEqual(transform_decimal('\u0663.\u0661\u0664'),
                         '3.14')
        self.assertEqual(transform_decimal("\N{EM SPACE}3.14\N{EN SPACE}"),
                         "\N{EM SPACE}3.14\N{EN SPACE}")
        self.assertEqual(transform_decimal('123\u20ac'),
                         '123\u20ac')

    def test_getnewargs(self):
        text = 'abc'
        args = text.__getnewargs__()
        self.assertIsNot(args[0], text)
        self.assertEqual(args[0], text)
        self.assertEqual(len(args), 1)

    def test_resize(self):
        for length in range(1, 100, 7):
            # generate a fresh string (refcount=1)
            text = 'a' * length + 'b'

            with support.check_warnings(('unicode_internal codec has been '
                                         'deprecated', DeprecationWarning)):
                # fill wstr internal field
                abc = text.encode('unicode_internal')
                self.assertEqual(abc.decode('unicode_internal'), text)

                # resize text: wstr field must be cleared and then recomputed
                text += 'c'
                abcdef = text.encode('unicode_internal')
                self.assertNotEqual(abc, abcdef)
                self.assertEqual(abcdef.decode('unicode_internal'), text)


class StringModuleTest(unittest.TestCase):
    def test_formatter_parser(self):
        def parse(format):
            return list(_string.formatter_parser(format))

        formatter = parse("prefix {2!s}xxx{0:^+10.3f}{obj.attr!s} {z[0]!s:10}")
        self.assertEqual(formatter, [
            ('prefix ', '2', '', 's'),
            ('xxx', '0', '^+10.3f', None),
            ('', 'obj.attr', '', 's'),
            (' ', 'z[0]', '10', 's'),
        ])

        formatter = parse("prefix {} suffix")
        self.assertEqual(formatter, [
            ('prefix ', '', '', None),
            (' suffix', None, None, None),
        ])

        formatter = parse("str")
        self.assertEqual(formatter, [
            ('str', None, None, None),
        ])

        formatter = parse("")
        self.assertEqual(formatter, [])

        formatter = parse("{0}")
        self.assertEqual(formatter, [
            ('', '0', '', None),
        ])

        self.assertRaises(TypeError, _string.formatter_parser, 1)

    def test_formatter_field_name_split(self):
        def split(name):
            items = list(_string.formatter_field_name_split(name))
            items[1] = list(items[1])
            return items
        self.assertEqual(split("obj"), ["obj", []])
        self.assertEqual(split("obj.arg"), ["obj", [(True, 'arg')]])
        self.assertEqual(split("obj[key]"), ["obj", [(False, 'key')]])
        self.assertEqual(split("obj.arg[key1][key2]"), [
            "obj",
            [(True, 'arg'),
             (False, 'key1'),
             (False, 'key2'),
            ]])
        self.assertRaises(TypeError, _string.formatter_field_name_split, 1)


if __name__ == "__main__":
    unittest.main()
