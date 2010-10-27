""" Test script for the Unicode implementation.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
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

class UnicodeTest(
    string_tests.CommonTest,
    string_tests.MixinStrUnicodeUserStringTest,
    string_tests.MixinStrUnicodeTest,
    ):
    type2test = str

    def setUp(self):
        self.warning_filters = warnings.filters[:]

    def tearDown(self):
        warnings.filters = self.warning_filters

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
        self.assertNotEquals(r"\u0020", " ")

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
        self.checkequalnofix(0,  'abcdefghiabc', 'find', 'abc')
        self.checkequalnofix(9,  'abcdefghiabc', 'find', 'abc', 1)
        self.checkequalnofix(-1, 'abcdefghiabc', 'find', 'def', 4)

        self.assertRaises(TypeError, 'hello'.find)
        self.assertRaises(TypeError, 'hello'.find, 42)

    def test_rfind(self):
        string_tests.CommonTest.test_rfind(self)
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

    def test_bytes_comparison(self):
        warnings.simplefilter('ignore', BytesWarning)
        self.assertEqual('abc' == b'abc', False)
        self.assertEqual('abc' != b'abc', True)
        self.assertEqual('abc' == bytearray(b'abc'), False)
        self.assertEqual('abc' != bytearray(b'abc'), True)

    def test_comparison(self):
        # Comparisons:
        self.assertEqual('abc', 'abc')
        self.assertEqual('abc', 'abc')
        self.assertEqual('abc', 'abc')
        self.assertTrue('abcd' > 'abc')
        self.assertTrue('abcd' > 'abc')
        self.assertTrue('abcd' > 'abc')
        self.assertTrue('abc' < 'abcd')
        self.assertTrue('abc' < 'abcd')
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

    def test_isupper(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isupper(self)
        if not sys.platform.startswith('java'):
            self.checkequalnofix(False, '\u1FFc', 'isupper')

    def test_istitle(self):
        string_tests.MixinStrUnicodeUserStringTest.test_title(self)
        self.checkequalnofix(True, '\u1FFc', 'istitle')
        self.checkequalnofix(True, 'Greek \u1FFcitlecases ...', 'istitle')

    def test_isspace(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isspace(self)
        self.checkequalnofix(True, '\u2000', 'isspace')
        self.checkequalnofix(True, '\u200a', 'isspace')
        self.checkequalnofix(False, '\u2014', 'isspace')

    def test_isalpha(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isalpha(self)
        self.checkequalnofix(True, '\u1FFc', 'isalpha')

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

    def test_isdigit(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isdigit(self)
        self.checkequalnofix(True, '\u2460', 'isdigit')
        self.checkequalnofix(False, '\xbc', 'isdigit')
        self.checkequalnofix(True, '\u0660', 'isdigit')

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

    def test_isidentifier(self):
        self.assertTrue("a".isidentifier())
        self.assertTrue("Z".isidentifier())
        self.assertTrue("_".isidentifier())
        self.assertTrue("b0".isidentifier())
        self.assertTrue("bc".isidentifier())
        self.assertTrue("b_".isidentifier())
        self.assertTrue("µ".isidentifier())

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

    def test_contains(self):
        # Testing Unicode contains method
        self.assertTrue('a' in 'abdb')
        self.assertTrue('a' in 'bdab')
        self.assertTrue('a' in 'bdaba')
        self.assertTrue('a' in 'bdba')
        self.assertTrue('a' not in 'bdb')
        self.assertTrue('a' in 'bdba')
        self.assertTrue('a' in ('a',1,None))
        self.assertTrue('a' in (1,None,'a'))
        self.assertTrue('a' in ('a',1,None))
        self.assertTrue('a' in (1,None,'a'))
        self.assertTrue('a' not in ('x',1,'y'))
        self.assertTrue('a' not in ('x',1,None))
        self.assertTrue('abcd' not in 'abcxxxx')
        self.assertTrue('ab' in 'abcd')
        self.assertTrue('ab' in 'abc')
        self.assertTrue('ab' in (1,None,'ab'))
        self.assertTrue('' in 'abc')
        self.assertTrue('' in '')
        self.assertTrue('' in 'abc')
        self.assertTrue('\0' not in 'abc')
        self.assertTrue('\0' in '\0abc')
        self.assertTrue('\0' in 'abc\0')
        self.assertTrue('a' in '\0abc')
        self.assertTrue('asdf' in 'asdf')
        self.assertTrue('asdf' not in 'asd')
        self.assertTrue('asdf' not in '')

        self.assertRaises(TypeError, "abc".__contains__)

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
        self.assertEqual('{0}'.format(E('data')), 'E(data)')
        self.assertEqual('{0:^10}'.format(E('data')), ' E(data)  ')
        self.assertEqual('{0:^10s}'.format(E('data')), ' E(data)  ')
        self.assertEqual('{0:d}'.format(G('data')), 'G(data)')
        self.assertEqual('{0:>15s}'.format(G('data')), ' string is data')
        self.assertEqual('{0!s}'.format(G('data')), 'string is data')

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
        self.assertRaises(ValueError, "{".format)
        self.assertRaises(ValueError, "}".format)
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

        self.assertEqual(
            str(UnicodeSubclass('unicode subclass becomes unicode')),
            'unicode subclass becomes unicode'
        )

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

        # Unpaired surrogates not supported
        self.assertRaises(UnicodeError, str, b'+3ADYAA-', 'utf-7')

        self.assertEqual(str(b'+3ADYAA-', 'utf-7', 'replace'), '\ufffd\ufffd')

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
        if sys.maxunicode == 65535:
            self.assertEqual('\ud800\udc02'.encode('utf-8'), b'\xf0\x90\x80\x82')
            self.assertEqual('\ud84d\udc56'.encode('utf-8'), b'\xf0\xa3\x91\x96')
        self.assertEqual('\ud800'.encode('utf-8', 'surrogatepass'), b'\xed\xa0\x80')
        self.assertEqual('\udc00'.encode('utf-8', 'surrogatepass'), b'\xed\xb0\x80')
        if sys.maxunicode == 65535:
            self.assertEqual(
                ('\ud800\udc02'*1000).encode('utf-8'),
                b'\xf0\x90\x80\x82'*1000)
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
            (b'\xc2\xc2\xc2', FFFD*3), # 2 start bytes
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

    def test_codecs_idna(self):
        # Test whether trailing dot is preserved
        self.assertEqual("www.python.org.".encode("idna"), b"www.python.org.")

    def test_codecs_errors(self):
        # Error handling (encoding)
        self.assertRaises(UnicodeError, 'Andr\202 x'.encode, 'ascii')
        self.assertRaises(UnicodeError, 'Andr\202 x'.encode, 'ascii','strict')
        self.assertEqual('Andr\202 x'.encode('ascii','ignore'), b"Andr x")
        self.assertEqual('Andr\202 x'.encode('ascii','replace'), b"Andr? x")

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
        # executes PyUnicode_Encode()
        import imp
        self.assertRaises(
            ImportError,
            imp.find_module,
            "non-existing module",
            ["non-existing dir"]
        )

        # Error handling (wrong arguments)
        self.assertRaises(TypeError, "hello".encode, 42, 42, 42)

        # Error handling (PyUnicode_EncodeDecimal())
        self.assertRaises(UnicodeError, int, "\u0200")

    def test_codecs(self):
        # Encoding
        self.assertEqual('hello'.encode('ascii'), b'hello')
        self.assertEqual('hello'.encode('utf-7'), b'hello')
        self.assertEqual('hello'.encode('utf-8'), b'hello')
        self.assertEqual('hello'.encode('utf8'), b'hello')
        self.assertEqual('hello'.encode('utf-16-le'), b'h\000e\000l\000l\000o\000')
        self.assertEqual('hello'.encode('utf-16-be'), b'\000h\000e\000l\000l\000o')
        self.assertEqual('hello'.encode('latin-1'), b'hello')

        # Roundtrip safety for BMP (just the first 1024 chars)
        for c in range(1024):
            u = chr(c)
            for encoding in ('utf-7', 'utf-8', 'utf-16', 'utf-16-le',
                             'utf-16-be', 'raw_unicode_escape',
                             'unicode_escape', 'unicode_internal'):
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
        u = '\U00010001\U00020002\U00030003\U00040004\U00050005'
        for encoding in ('utf-8', 'utf-16', 'utf-16-le', 'utf-16-be',
                         #'raw_unicode_escape',
                         'unicode_escape', 'unicode_internal'):
            self.assertEqual(str(u.encode(encoding),encoding), u)

        # UTF-8 must be roundtrip safe for all UCS-2 code points
        # This excludes surrogates: in the full range, there would be
        # a surrogate pair (\udbff\udc00), which gets converted back
        # to a non-BMP character (\U0010fc00)
        u = ''.join(map(chr, list(range(0,0xd800)) +
                             list(range(0xe000,0x10000))))
        for encoding in ('utf-8',):
            self.assertEqual(str(u.encode(encoding),encoding), u)

    def test_codecs_charmap(self):
        # 0-127
        s = bytes(range(128))
        for encoding in (
            'cp037', 'cp1026',
            'cp437', 'cp500', 'cp737', 'cp775', 'cp850',
            'cp852', 'cp855', 'cp860', 'cp861', 'cp862',
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
            'cp437', 'cp500', 'cp737', 'cp775', 'cp850',
            'cp852', 'cp855', 'cp860', 'cp861', 'cp862',
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

        # FIXME
        #y = r'\U00100000'
        #x = y.encode("raw-unicode-escape").decode("raw-unicode-escape")
        #self.assertEqual(x, y)
        #y = r'\U00010000'
        #x = y.encode("raw-unicode-escape").decode("raw-unicode-escape")
        #self.assertEqual(x, y)

        #try:
        #    '\U11111111'.decode("raw-unicode-escape")
        #except UnicodeDecodeError as e:
        #    self.assertEqual(e.start, 0)
        #    self.assertEqual(e.end, 10)
        #else:
        #    self.fail("Should have raised UnicodeDecodeError")

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

    def test_expandtabs_overflows_gracefully(self):
        # This test only affects 32-bit platforms because expandtabs can only take
        # an int as the max value, not a 64-bit C long.  If expandtabs is changed
        # to take a 64-bit long, this test should apply to all platforms.
        if sys.maxsize > (1 << 32) or struct.calcsize('P') != 4:
            return
        self.assertRaises(OverflowError, 't\tt\t'.expandtabs, sys.maxsize)

    def test_raiseMemError(self):
        # Ensure that the freelist contains a consistent object, even
        # when a string allocation fails with a MemoryError.
        # This used to crash the interpreter,
        # or leak references when the number was smaller.
        charwidth = 4 if sys.maxunicode >= 0x10000 else 2
        # Note: sys.maxsize is half of the actual max allocation because of
        # the signedness of Py_ssize_t.
        alloc = lambda: "a" * (sys.maxsize // charwidth * 2)
        self.assertRaises(MemoryError, alloc)
        self.assertRaises(MemoryError, alloc)

    def test_format_subclass(self):
        class S(str):
            def __str__(self):
                return '__str__ overridden'
        s = S('xxx')
        self.assertEquals("%s" % s, '__str__ overridden')
        self.assertEquals("{}".format(s), '__str__ overridden')


def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
