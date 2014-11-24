""" Test script for the Unicode implementation.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
import sys
import struct
import codecs
import unittest
from test import test_support, string_tests

# decorator to skip tests on narrow builds
requires_wide_build = unittest.skipIf(sys.maxunicode == 65535,
                                      'requires wide build')

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
    type2test = unicode

    def assertEqual(self, first, second, msg=None):
        # strict assertEqual method: reject implicit bytes/unicode equality
        super(UnicodeTest, self).assertEqual(first, second, msg)
        if isinstance(first, unicode) or isinstance(second, unicode):
            self.assertIsInstance(first, unicode)
            self.assertIsInstance(second, unicode)
        elif isinstance(first, str) or isinstance(second, str):
            self.assertIsInstance(first, str)
            self.assertIsInstance(second, str)

    def checkequalnofix(self, result, object, methodname, *args):
        method = getattr(object, methodname)
        realresult = method(*args)
        self.assertEqual(realresult, result)
        self.assertTrue(type(realresult) is type(result))

        # if the original is returned make sure that
        # this doesn't happen with subclasses
        if realresult is object:
            class usub(unicode):
                def __repr__(self):
                    return 'usub(%r)' % unicode.__repr__(self)
            object = usub(object)
            method = getattr(object, methodname)
            realresult = method(*args)
            self.assertEqual(realresult, result)
            self.assertTrue(object is not realresult)

    def test_literals(self):
        self.assertEqual(u'\xff', u'\u00ff')
        self.assertEqual(u'\uffff', u'\U0000ffff')
        self.assertRaises(SyntaxError, eval, 'u\'\\Ufffffffe\'')
        self.assertRaises(SyntaxError, eval, 'u\'\\Uffffffff\'')
        self.assertRaises(SyntaxError, eval, 'u\'\\U%08x\'' % 0x110000)

    def test_repr(self):
        if not sys.platform.startswith('java'):
            # Test basic sanity of repr()
            self.assertEqual(repr(u'abc'), "u'abc'")
            self.assertEqual(repr(u'ab\\c'), "u'ab\\\\c'")
            self.assertEqual(repr(u'ab\\'), "u'ab\\\\'")
            self.assertEqual(repr(u'\\c'), "u'\\\\c'")
            self.assertEqual(repr(u'\\'), "u'\\\\'")
            self.assertEqual(repr(u'\n'), "u'\\n'")
            self.assertEqual(repr(u'\r'), "u'\\r'")
            self.assertEqual(repr(u'\t'), "u'\\t'")
            self.assertEqual(repr(u'\b'), "u'\\x08'")
            self.assertEqual(repr(u"'\""), """u'\\'"'""")
            self.assertEqual(repr(u"'\""), """u'\\'"'""")
            self.assertEqual(repr(u"'"), '''u"'"''')
            self.assertEqual(repr(u'"'), """u'"'""")
            latin1repr = (
                "u'\\x00\\x01\\x02\\x03\\x04\\x05\\x06\\x07\\x08\\t\\n\\x0b\\x0c\\r"
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
            testrepr = repr(u''.join(map(unichr, xrange(256))))
            self.assertEqual(testrepr, latin1repr)
            # Test repr works on wide unicode escapes without overflow.
            self.assertEqual(repr(u"\U00010000" * 39 + u"\uffff" * 4096),
                             repr(u"\U00010000" * 39 + u"\uffff" * 4096))


    def test_count(self):
        string_tests.CommonTest.test_count(self)
        # check mixed argument types
        self.checkequalnofix(3,  'aaa', 'count', u'a')
        self.checkequalnofix(0,  'aaa', 'count', u'b')
        self.checkequalnofix(3, u'aaa', 'count',  'a')
        self.checkequalnofix(0, u'aaa', 'count',  'b')
        self.checkequalnofix(0, u'aaa', 'count',  'b')
        self.checkequalnofix(1, u'aaa', 'count',  'a', -1)
        self.checkequalnofix(3, u'aaa', 'count',  'a', -10)
        self.checkequalnofix(2, u'aaa', 'count',  'a', 0, -1)
        self.checkequalnofix(0, u'aaa', 'count',  'a', 0, -10)

    def test_find(self):
        self.checkequalnofix(0,  u'abcdefghiabc', 'find', u'abc')
        self.checkequalnofix(9,  u'abcdefghiabc', 'find', u'abc', 1)
        self.checkequalnofix(-1, u'abcdefghiabc', 'find', u'def', 4)

        self.assertRaises(TypeError, u'hello'.find)
        self.assertRaises(TypeError, u'hello'.find, 42)

    def test_rfind(self):
        string_tests.CommonTest.test_rfind(self)
        # check mixed argument types
        self.checkequalnofix(9,   'abcdefghiabc', 'rfind', u'abc')
        self.checkequalnofix(12,  'abcdefghiabc', 'rfind', u'')
        self.checkequalnofix(12, u'abcdefghiabc', 'rfind',  '')

    def test_index(self):
        string_tests.CommonTest.test_index(self)
        # check mixed argument types
        for (t1, t2) in ((str, unicode), (unicode, str)):
            self.checkequalnofix(0, t1('abcdefghiabc'), 'index',  t2(''))
            self.checkequalnofix(3, t1('abcdefghiabc'), 'index',  t2('def'))
            self.checkequalnofix(0, t1('abcdefghiabc'), 'index',  t2('abc'))
            self.checkequalnofix(9, t1('abcdefghiabc'), 'index',  t2('abc'), 1)
            self.assertRaises(ValueError, t1('abcdefghiabc').index, t2('hib'))
            self.assertRaises(ValueError, t1('abcdefghiab').index,  t2('abc'), 1)
            self.assertRaises(ValueError, t1('abcdefghi').index,  t2('ghi'), 8)
            self.assertRaises(ValueError, t1('abcdefghi').index,  t2('ghi'), -1)

    def test_rindex(self):
        string_tests.CommonTest.test_rindex(self)
        # check mixed argument types
        for (t1, t2) in ((str, unicode), (unicode, str)):
            self.checkequalnofix(12, t1('abcdefghiabc'), 'rindex',  t2(''))
            self.checkequalnofix(3,  t1('abcdefghiabc'), 'rindex',  t2('def'))
            self.checkequalnofix(9,  t1('abcdefghiabc'), 'rindex',  t2('abc'))
            self.checkequalnofix(0,  t1('abcdefghiabc'), 'rindex',  t2('abc'), 0, -1)

            self.assertRaises(ValueError, t1('abcdefghiabc').rindex,  t2('hib'))
            self.assertRaises(ValueError, t1('defghiabc').rindex,  t2('def'), 1)
            self.assertRaises(ValueError, t1('defghiabc').rindex,  t2('abc'), 0, -1)
            self.assertRaises(ValueError, t1('abcdefghi').rindex,  t2('ghi'), 0, 8)
            self.assertRaises(ValueError, t1('abcdefghi').rindex,  t2('ghi'), 0, -1)

    def test_translate(self):
        self.checkequalnofix(u'bbbc', u'abababc', 'translate', {ord('a'):None})
        self.checkequalnofix(u'iiic', u'abababc', 'translate', {ord('a'):None, ord('b'):ord('i')})
        self.checkequalnofix(u'iiix', u'abababc', 'translate', {ord('a'):None, ord('b'):ord('i'), ord('c'):u'x'})
        self.checkequalnofix(u'<i><i><i>c', u'abababc', 'translate', {ord('a'):None, ord('b'):u'<i>'})
        self.checkequalnofix(u'c', u'abababc', 'translate', {ord('a'):None, ord('b'):u''})
        self.checkequalnofix(u'xyyx', u'xzx', 'translate', {ord('z'):u'yy'})

        self.assertRaises(TypeError, u'hello'.translate)
        self.assertRaises(TypeError, u'abababc'.translate, {ord('a'):''})

    def test_split(self):
        string_tests.CommonTest.test_split(self)

        # Mixed arguments
        self.checkequalnofix([u'a', u'b', u'c', u'd'], u'a//b//c//d', 'split', '//')
        self.checkequalnofix([u'a', u'b', u'c', u'd'], 'a//b//c//d', 'split', u'//')
        self.checkequalnofix([u'endcase ', u''], u'endcase test', 'split', 'test')

    def test_join(self):
        string_tests.MixinStrUnicodeUserStringTest.test_join(self)

        # mixed arguments
        self.checkequalnofix(u'a b c d', u' ', 'join', ['a', 'b', u'c', u'd'])
        self.checkequalnofix(u'abcd', u'', 'join', (u'a', u'b', u'c', u'd'))
        self.checkequalnofix(u'w x y z', u' ', 'join', string_tests.Sequence('wxyz'))
        self.checkequalnofix(u'a b c d', ' ', 'join', [u'a', u'b', u'c', u'd'])
        self.checkequalnofix(u'a b c d', ' ', 'join', ['a', 'b', u'c', u'd'])
        self.checkequalnofix(u'abcd', '', 'join', (u'a', u'b', u'c', u'd'))
        self.checkequalnofix(u'w x y z', ' ', 'join', string_tests.Sequence(u'wxyz'))

    def test_strip(self):
        string_tests.CommonTest.test_strip(self)
        self.assertRaises(UnicodeError, u"hello".strip, "\xff")

    def test_replace(self):
        string_tests.CommonTest.test_replace(self)

        # method call forwarded from str implementation because of unicode argument
        self.checkequalnofix(u'one@two!three!', 'one!two!three!', 'replace', u'!', u'@', 1)
        self.assertRaises(TypeError, 'replace'.replace, u"r", 42)

    def test_comparison(self):
        # Comparisons:
        self.assertTrue(u'abc' == 'abc')
        self.assertTrue('abc' == u'abc')
        self.assertTrue(u'abc' == u'abc')
        self.assertTrue(u'abcd' > 'abc')
        self.assertTrue('abcd' > u'abc')
        self.assertTrue(u'abcd' > u'abc')
        self.assertTrue(u'abc' < 'abcd')
        self.assertTrue('abc' < u'abcd')
        self.assertTrue(u'abc' < u'abcd')

        if 0:
            # Move these tests to a Unicode collation module test...
            # Testing UTF-16 code point order comparisons...

            # No surrogates, no fixup required.
            self.assertTrue(u'\u0061' < u'\u20ac')
            # Non surrogate below surrogate value, no fixup required
            self.assertTrue(u'\u0061' < u'\ud800\udc02')

            # Non surrogate above surrogate value, fixup required
            def test_lecmp(s, s2):
                self.assertTrue(s < s2)

            def test_fixup(s):
                s2 = u'\ud800\udc01'
                test_lecmp(s, s2)
                s2 = u'\ud900\udc01'
                test_lecmp(s, s2)
                s2 = u'\uda00\udc01'
                test_lecmp(s, s2)
                s2 = u'\udb00\udc01'
                test_lecmp(s, s2)
                s2 = u'\ud800\udd01'
                test_lecmp(s, s2)
                s2 = u'\ud900\udd01'
                test_lecmp(s, s2)
                s2 = u'\uda00\udd01'
                test_lecmp(s, s2)
                s2 = u'\udb00\udd01'
                test_lecmp(s, s2)
                s2 = u'\ud800\ude01'
                test_lecmp(s, s2)
                s2 = u'\ud900\ude01'
                test_lecmp(s, s2)
                s2 = u'\uda00\ude01'
                test_lecmp(s, s2)
                s2 = u'\udb00\ude01'
                test_lecmp(s, s2)
                s2 = u'\ud800\udfff'
                test_lecmp(s, s2)
                s2 = u'\ud900\udfff'
                test_lecmp(s, s2)
                s2 = u'\uda00\udfff'
                test_lecmp(s, s2)
                s2 = u'\udb00\udfff'
                test_lecmp(s, s2)

                test_fixup(u'\ue000')
                test_fixup(u'\uff61')

        # Surrogates on both sides, no fixup required
        self.assertTrue(u'\ud800\udc02' < u'\ud84d\udc56')

    def test_capitalize(self):
        string_tests.CommonTest.test_capitalize(self)
        # check that titlecased chars are lowered correctly
        # \u1ffc is the titlecased char
        self.checkequal(u'\u1ffc\u1ff3\u1ff3\u1ff3',
                        u'\u1ff3\u1ff3\u1ffc\u1ffc', 'capitalize')
        # check with cased non-letter chars
        self.checkequal(u'\u24c5\u24e8\u24e3\u24d7\u24de\u24dd',
                        u'\u24c5\u24ce\u24c9\u24bd\u24c4\u24c3', 'capitalize')
        self.checkequal(u'\u24c5\u24e8\u24e3\u24d7\u24de\u24dd',
                        u'\u24df\u24e8\u24e3\u24d7\u24de\u24dd', 'capitalize')
        self.checkequal(u'\u2160\u2171\u2172',
                        u'\u2160\u2161\u2162', 'capitalize')
        self.checkequal(u'\u2160\u2171\u2172',
                        u'\u2170\u2171\u2172', 'capitalize')
        # check with Ll chars with no upper - nothing changes here
        self.checkequal(u'\u019b\u1d00\u1d86\u0221\u1fb7',
                        u'\u019b\u1d00\u1d86\u0221\u1fb7', 'capitalize')

    def test_islower(self):
        string_tests.MixinStrUnicodeUserStringTest.test_islower(self)
        self.checkequalnofix(False, u'\u1FFc', 'islower')

    @requires_wide_build
    def test_islower_non_bmp(self):
        # non-BMP, uppercase
        self.assertFalse(u'\U00010401'.islower())
        self.assertFalse(u'\U00010427'.islower())
        # non-BMP, lowercase
        self.assertTrue(u'\U00010429'.islower())
        self.assertTrue(u'\U0001044E'.islower())
        # non-BMP, non-cased
        self.assertFalse(u'\U0001F40D'.islower())
        self.assertFalse(u'\U0001F46F'.islower())

    def test_isupper(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isupper(self)
        if not sys.platform.startswith('java'):
            self.checkequalnofix(False, u'\u1FFc', 'isupper')

    @requires_wide_build
    def test_isupper_non_bmp(self):
        # non-BMP, uppercase
        self.assertTrue(u'\U00010401'.isupper())
        self.assertTrue(u'\U00010427'.isupper())
        # non-BMP, lowercase
        self.assertFalse(u'\U00010429'.isupper())
        self.assertFalse(u'\U0001044E'.isupper())
        # non-BMP, non-cased
        self.assertFalse(u'\U0001F40D'.isupper())
        self.assertFalse(u'\U0001F46F'.isupper())

    def test_istitle(self):
        string_tests.MixinStrUnicodeUserStringTest.test_istitle(self)
        self.checkequalnofix(True, u'\u1FFc', 'istitle')
        self.checkequalnofix(True, u'Greek \u1FFcitlecases ...', 'istitle')

    @requires_wide_build
    def test_istitle_non_bmp(self):
        # non-BMP, uppercase + lowercase
        self.assertTrue(u'\U00010401\U00010429'.istitle())
        self.assertTrue(u'\U00010427\U0001044E'.istitle())
        # apparently there are no titlecased (Lt) non-BMP chars in Unicode 6
        for ch in [u'\U00010429', u'\U0001044E', u'\U0001F40D', u'\U0001F46F']:
            self.assertFalse(ch.istitle(), '{!r} is not title'.format(ch))

    def test_isspace(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isspace(self)
        self.checkequalnofix(True, u'\u2000', 'isspace')
        self.checkequalnofix(True, u'\u200a', 'isspace')
        self.checkequalnofix(False, u'\u2014', 'isspace')

    @requires_wide_build
    def test_isspace_non_bmp(self):
        # apparently there are no non-BMP spaces chars in Unicode 6
        for ch in [u'\U00010401', u'\U00010427', u'\U00010429', u'\U0001044E',
                   u'\U0001F40D', u'\U0001F46F']:
            self.assertFalse(ch.isspace(), '{!r} is not space.'.format(ch))

    @requires_wide_build
    def test_isalnum_non_bmp(self):
        for ch in [u'\U00010401', u'\U00010427', u'\U00010429', u'\U0001044E',
                   u'\U0001D7F6', u'\U000104A0', u'\U000104A0', u'\U0001F107']:
            self.assertTrue(ch.isalnum(), '{!r} is alnum.'.format(ch))

    def test_isalpha(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isalpha(self)
        self.checkequalnofix(True, u'\u1FFc', 'isalpha')

    @requires_wide_build
    def test_isalpha_non_bmp(self):
        # non-BMP, cased
        self.assertTrue(u'\U00010401'.isalpha())
        self.assertTrue(u'\U00010427'.isalpha())
        self.assertTrue(u'\U00010429'.isalpha())
        self.assertTrue(u'\U0001044E'.isalpha())
        # non-BMP, non-cased
        self.assertFalse(u'\U0001F40D'.isalpha())
        self.assertFalse(u'\U0001F46F'.isalpha())

    def test_isdecimal(self):
        self.checkequalnofix(False, u'', 'isdecimal')
        self.checkequalnofix(False, u'a', 'isdecimal')
        self.checkequalnofix(True, u'0', 'isdecimal')
        self.checkequalnofix(False, u'\u2460', 'isdecimal') # CIRCLED DIGIT ONE
        self.checkequalnofix(False, u'\xbc', 'isdecimal') # VULGAR FRACTION ONE QUARTER
        self.checkequalnofix(True, u'\u0660', 'isdecimal') # ARABIC-INDIC DIGIT ZERO
        self.checkequalnofix(True, u'0123456789', 'isdecimal')
        self.checkequalnofix(False, u'0123456789a', 'isdecimal')

        self.checkraises(TypeError, 'abc', 'isdecimal', 42)

    @requires_wide_build
    def test_isdecimal_non_bmp(self):
        for ch in [u'\U00010401', u'\U00010427', u'\U00010429', u'\U0001044E',
                   u'\U0001F40D', u'\U0001F46F', u'\U00011065', u'\U0001F107']:
            self.assertFalse(ch.isdecimal(), '{!r} is not decimal.'.format(ch))
        for ch in [u'\U0001D7F6', u'\U000104A0', u'\U000104A0']:
            self.assertTrue(ch.isdecimal(), '{!r} is decimal.'.format(ch))

    def test_isdigit(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isdigit(self)
        self.checkequalnofix(True, u'\u2460', 'isdigit')
        self.checkequalnofix(False, u'\xbc', 'isdigit')
        self.checkequalnofix(True, u'\u0660', 'isdigit')

    @requires_wide_build
    def test_isdigit_non_bmp(self):
        for ch in [u'\U00010401', u'\U00010427', u'\U00010429', u'\U0001044E',
                   u'\U0001F40D', u'\U0001F46F', u'\U00011065']:
            self.assertFalse(ch.isdigit(), '{!r} is not a digit.'.format(ch))
        for ch in [u'\U0001D7F6', u'\U000104A0', u'\U000104A0', u'\U0001F107']:
            self.assertTrue(ch.isdigit(), '{!r} is a digit.'.format(ch))

    def test_isnumeric(self):
        self.checkequalnofix(False, u'', 'isnumeric')
        self.checkequalnofix(False, u'a', 'isnumeric')
        self.checkequalnofix(True, u'0', 'isnumeric')
        self.checkequalnofix(True, u'\u2460', 'isnumeric')
        self.checkequalnofix(True, u'\xbc', 'isnumeric')
        self.checkequalnofix(True, u'\u0660', 'isnumeric')
        self.checkequalnofix(True, u'0123456789', 'isnumeric')
        self.checkequalnofix(False, u'0123456789a', 'isnumeric')

        self.assertRaises(TypeError, u"abc".isnumeric, 42)

    @requires_wide_build
    def test_isnumeric_non_bmp(self):
        for ch in [u'\U00010401', u'\U00010427', u'\U00010429', u'\U0001044E',
                   u'\U0001F40D', u'\U0001F46F']:
            self.assertFalse(ch.isnumeric(), '{!r} is not numeric.'.format(ch))
        for ch in [u'\U00010107', u'\U0001D7F6', u'\U00023b1b',
                   u'\U000104A0', u'\U0001F107']:
            self.assertTrue(ch.isnumeric(), '{!r} is numeric.'.format(ch))

    @requires_wide_build
    def test_surrogates(self):
        # this test actually passes on narrow too, but it's just by accident.
        # Surrogates are seen as non-cased chars, so u'X\uD800X' is as
        # uppercase as 'X X'
        for s in (u'a\uD800b\uDFFF', u'a\uDFFFb\uD800',
                  u'a\uD800b\uDFFFa', u'a\uDFFFb\uD800a'):
            self.assertTrue(s.islower())
            self.assertFalse(s.isupper())
            self.assertFalse(s.istitle())
        for s in (u'A\uD800B\uDFFF', u'A\uDFFFB\uD800',
                  u'A\uD800B\uDFFFA', u'A\uDFFFB\uD800A'):
            self.assertFalse(s.islower())
            self.assertTrue(s.isupper())
            self.assertTrue(s.istitle())

        for meth_name in ('islower', 'isupper', 'istitle'):
            meth = getattr(unicode, meth_name)
            for s in (u'\uD800', u'\uDFFF', u'\uD800\uD800', u'\uDFFF\uDFFF'):
                self.assertFalse(meth(s), '%r.%s() is False' % (s, meth_name))

        for meth_name in ('isalpha', 'isalnum', 'isdigit', 'isspace',
                          'isdecimal', 'isnumeric'):
            meth = getattr(unicode, meth_name)
            for s in (u'\uD800', u'\uDFFF', u'\uD800\uD800', u'\uDFFF\uDFFF',
                      u'a\uD800b\uDFFF', u'a\uDFFFb\uD800',
                      u'a\uD800b\uDFFFa', u'a\uDFFFb\uD800a'):
                self.assertFalse(meth(s), '%r.%s() is False' % (s, meth_name))


    @requires_wide_build
    def test_lower(self):
        string_tests.CommonTest.test_lower(self)
        self.assertEqual(u'\U00010427'.lower(), u'\U0001044F')
        self.assertEqual(u'\U00010427\U00010427'.lower(),
                         u'\U0001044F\U0001044F')
        self.assertEqual(u'\U00010427\U0001044F'.lower(),
                         u'\U0001044F\U0001044F')
        self.assertEqual(u'X\U00010427x\U0001044F'.lower(),
                         u'x\U0001044Fx\U0001044F')

    @requires_wide_build
    def test_upper(self):
        string_tests.CommonTest.test_upper(self)
        self.assertEqual(u'\U0001044F'.upper(), u'\U00010427')
        self.assertEqual(u'\U0001044F\U0001044F'.upper(),
                         u'\U00010427\U00010427')
        self.assertEqual(u'\U00010427\U0001044F'.upper(),
                         u'\U00010427\U00010427')
        self.assertEqual(u'X\U00010427x\U0001044F'.upper(),
                         u'X\U00010427X\U00010427')

    @requires_wide_build
    def test_capitalize_wide_build(self):
        string_tests.CommonTest.test_capitalize(self)
        self.assertEqual(u'\U0001044F'.capitalize(), u'\U00010427')
        self.assertEqual(u'\U0001044F\U0001044F'.capitalize(),
                         u'\U00010427\U0001044F')
        self.assertEqual(u'\U00010427\U0001044F'.capitalize(),
                         u'\U00010427\U0001044F')
        self.assertEqual(u'\U0001044F\U00010427'.capitalize(),
                         u'\U00010427\U0001044F')
        self.assertEqual(u'X\U00010427x\U0001044F'.capitalize(),
                         u'X\U0001044Fx\U0001044F')

    @requires_wide_build
    def test_title(self):
        string_tests.MixinStrUnicodeUserStringTest.test_title(self)
        self.assertEqual(u'\U0001044F'.title(), u'\U00010427')
        self.assertEqual(u'\U0001044F\U0001044F'.title(),
                         u'\U00010427\U0001044F')
        self.assertEqual(u'\U0001044F\U0001044F \U0001044F\U0001044F'.title(),
                         u'\U00010427\U0001044F \U00010427\U0001044F')
        self.assertEqual(u'\U00010427\U0001044F \U00010427\U0001044F'.title(),
                         u'\U00010427\U0001044F \U00010427\U0001044F')
        self.assertEqual(u'\U0001044F\U00010427 \U0001044F\U00010427'.title(),
                         u'\U00010427\U0001044F \U00010427\U0001044F')
        self.assertEqual(u'X\U00010427x\U0001044F X\U00010427x\U0001044F'.title(),
                         u'X\U0001044Fx\U0001044F X\U0001044Fx\U0001044F')

    @requires_wide_build
    def test_swapcase(self):
        string_tests.CommonTest.test_swapcase(self)
        self.assertEqual(u'\U0001044F'.swapcase(), u'\U00010427')
        self.assertEqual(u'\U00010427'.swapcase(), u'\U0001044F')
        self.assertEqual(u'\U0001044F\U0001044F'.swapcase(),
                         u'\U00010427\U00010427')
        self.assertEqual(u'\U00010427\U0001044F'.swapcase(),
                         u'\U0001044F\U00010427')
        self.assertEqual(u'\U0001044F\U00010427'.swapcase(),
                         u'\U00010427\U0001044F')
        self.assertEqual(u'X\U00010427x\U0001044F'.swapcase(),
                         u'x\U0001044FX\U00010427')

    def test_contains(self):
        # Testing Unicode contains method
        self.assertIn('a', u'abdb')
        self.assertIn('a', u'bdab')
        self.assertIn('a', u'bdaba')
        self.assertIn('a', u'bdba')
        self.assertIn('a', u'bdba')
        self.assertIn(u'a', u'bdba')
        self.assertNotIn(u'a', u'bdb')
        self.assertNotIn(u'a', 'bdb')
        self.assertIn(u'a', 'bdba')
        self.assertIn(u'a', ('a',1,None))
        self.assertIn(u'a', (1,None,'a'))
        self.assertIn(u'a', (1,None,u'a'))
        self.assertIn('a', ('a',1,None))
        self.assertIn('a', (1,None,'a'))
        self.assertIn('a', (1,None,u'a'))
        self.assertNotIn('a', ('x',1,u'y'))
        self.assertNotIn('a', ('x',1,None))
        self.assertNotIn(u'abcd', u'abcxxxx')
        self.assertIn(u'ab', u'abcd')
        self.assertIn('ab', u'abc')
        self.assertIn(u'ab', 'abc')
        self.assertIn(u'ab', (1,None,u'ab'))
        self.assertIn(u'', u'abc')
        self.assertIn('', u'abc')

        # If the following fails either
        # the contains operator does not propagate UnicodeErrors or
        # someone has changed the default encoding
        self.assertRaises(UnicodeDecodeError, 'g\xe2teau'.__contains__, u'\xe2')
        self.assertRaises(UnicodeDecodeError, u'g\xe2teau'.__contains__, '\xe2')

        self.assertIn(u'', '')
        self.assertIn('', u'')
        self.assertIn(u'', u'')
        self.assertIn(u'', 'abc')
        self.assertIn('', u'abc')
        self.assertIn(u'', u'abc')
        self.assertNotIn(u'\0', 'abc')
        self.assertNotIn('\0', u'abc')
        self.assertNotIn(u'\0', u'abc')
        self.assertIn(u'\0', '\0abc')
        self.assertIn('\0', u'\0abc')
        self.assertIn(u'\0', u'\0abc')
        self.assertIn(u'\0', 'abc\0')
        self.assertIn('\0', u'abc\0')
        self.assertIn(u'\0', u'abc\0')
        self.assertIn(u'a', '\0abc')
        self.assertIn('a', u'\0abc')
        self.assertIn(u'a', u'\0abc')
        self.assertIn(u'asdf', 'asdf')
        self.assertIn('asdf', u'asdf')
        self.assertIn(u'asdf', u'asdf')
        self.assertNotIn(u'asdf', 'asd')
        self.assertNotIn('asdf', u'asd')
        self.assertNotIn(u'asdf', u'asd')
        self.assertNotIn(u'asdf', '')
        self.assertNotIn('asdf', u'')
        self.assertNotIn(u'asdf', u'')

        self.assertRaises(TypeError, u"abc".__contains__)
        self.assertRaises(TypeError, u"abc".__contains__, object())

    def test_formatting(self):
        string_tests.MixinStrUnicodeUserStringTest.test_formatting(self)
        # Testing Unicode formatting strings...
        self.assertEqual(u"%s, %s" % (u"abc", "abc"), u'abc, abc')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", 1, 2, 3), u'abc, abc, 1, 2.000000,  3.00')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", 1, -2, 3), u'abc, abc, 1, -2.000000,  3.00')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", -1, -2, 3.5), u'abc, abc, -1, -2.000000,  3.50')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", -1, -2, 3.57), u'abc, abc, -1, -2.000000,  3.57')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", -1, -2, 1003.57), u'abc, abc, -1, -2.000000, 1003.57')
        if not sys.platform.startswith('java'):
            self.assertEqual(u"%r, %r" % (u"abc", "abc"), u"u'abc', 'abc'")
        self.assertEqual(u"%(x)s, %(y)s" % {'x':u"abc", 'y':"def"}, u'abc, def')
        self.assertEqual(u"%(x)s, %(\xfc)s" % {'x':u"abc", u'\xfc':"def"}, u'abc, def')

        self.assertEqual(u'%c' % 0x1234, u'\u1234')
        self.assertRaises(OverflowError, u"%c".__mod__, (sys.maxunicode+1,))
        self.assertRaises(ValueError, u"%.1\u1032f".__mod__, (1.0/3))

        for num in range(0x00,0x80):
            char = chr(num)
            self.assertEqual(u"%c" % char, unicode(char))
            self.assertEqual(u"%c" % num, unicode(char))
            self.assertTrue(char == u"%c" % char)
            self.assertTrue(char == u"%c" % num)
        # Issue 7649
        for num in range(0x80,0x100):
            uchar = unichr(num)
            self.assertEqual(uchar, u"%c" % num)   # works only with ints
            self.assertEqual(uchar, u"%c" % uchar) # and unicode chars
            # the implicit decoding should fail for non-ascii chars
            self.assertRaises(UnicodeDecodeError, u"%c".__mod__, chr(num))
            self.assertRaises(UnicodeDecodeError, u"%s".__mod__, chr(num))

        # formatting jobs delegated from the string implementation:
        self.assertEqual('...%(foo)s...' % {'foo':u"abc"}, u'...abc...')
        self.assertEqual('...%(foo)s...' % {'foo':"abc"}, '...abc...')
        self.assertEqual('...%(foo)s...' % {u'foo':"abc"}, '...abc...')
        self.assertEqual('...%(foo)s...' % {u'foo':u"abc"}, u'...abc...')
        self.assertEqual('...%(foo)s...' % {u'foo':u"abc",'def':123},  u'...abc...')
        self.assertEqual('...%(foo)s...' % {u'foo':u"abc",u'def':123}, u'...abc...')
        self.assertEqual('...%s...%s...%s...%s...' % (1,2,3,u"abc"), u'...1...2...3...abc...')
        self.assertEqual('...%%...%%s...%s...%s...%s...%s...' % (1,2,3,u"abc"), u'...%...%s...1...2...3...abc...')
        self.assertEqual('...%s...' % u"abc", u'...abc...')
        self.assertEqual('%*s' % (5,u'abc',), u'  abc')
        self.assertEqual('%*s' % (-5,u'abc',), u'abc  ')
        self.assertEqual('%*.*s' % (5,2,u'abc',), u'   ab')
        self.assertEqual('%*.*s' % (5,3,u'abc',), u'  abc')
        self.assertEqual('%i %*.*s' % (10, 5,3,u'abc',), u'10   abc')
        self.assertEqual('%i%s %*.*s' % (10, 3, 5, 3, u'abc',), u'103   abc')
        self.assertEqual('%c' % u'a', u'a')
        class Wrapper:
            def __str__(self):
                return u'\u1234'
        self.assertEqual('%s' % Wrapper(), u'\u1234')

    def test_formatting_huge_precision(self):
        format_string = u"%.{}f".format(sys.maxsize + 1)
        with self.assertRaises(ValueError):
            result = format_string % 2.34

    @test_support.cpython_only
    def test_formatting_huge_precision_c_limits(self):
        from _testcapi import INT_MAX
        format_string = u"%.{}f".format(INT_MAX + 1)
        with self.assertRaises(ValueError):
            result = format_string % 2.34

    def test_formatting_huge_width(self):
        format_string = u"%{}f".format(sys.maxsize + 1)
        with self.assertRaises(ValueError):
            result = format_string % 2.34

    def test_startswith_endswith_errors(self):
        for meth in (u'foo'.startswith, u'foo'.endswith):
            with self.assertRaises(UnicodeDecodeError):
                meth('\xff')
            with self.assertRaises(TypeError) as cm:
                meth(['f'])
            exc = str(cm.exception)
            self.assertIn('unicode', exc)
            self.assertIn('str', exc)
            self.assertIn('tuple', exc)

    @test_support.run_with_locale('LC_ALL', 'de_DE', 'fr_FR')
    def test_format_float(self):
        # should not format with a comma, but always with C locale
        self.assertEqual(u'1.0', u'%.1f' % 1.0)

    def test_constructor(self):
        # unicode(obj) tests (this maps to PyObject_Unicode() at C level)

        self.assertEqual(
            unicode(u'unicode remains unicode'),
            u'unicode remains unicode'
        )

        class UnicodeSubclass(unicode):
            pass

        self.assertEqual(
            unicode(UnicodeSubclass('unicode subclass becomes unicode')),
            u'unicode subclass becomes unicode'
        )

        self.assertEqual(
            unicode('strings are converted to unicode'),
            u'strings are converted to unicode'
        )

        class UnicodeCompat:
            def __init__(self, x):
                self.x = x
            def __unicode__(self):
                return self.x

        self.assertEqual(
            unicode(UnicodeCompat('__unicode__ compatible objects are recognized')),
            u'__unicode__ compatible objects are recognized')

        class StringCompat:
            def __init__(self, x):
                self.x = x
            def __str__(self):
                return self.x

        self.assertEqual(
            unicode(StringCompat('__str__ compatible objects are recognized')),
            u'__str__ compatible objects are recognized'
        )

        # unicode(obj) is compatible to str():

        o = StringCompat('unicode(obj) is compatible to str()')
        self.assertEqual(unicode(o), u'unicode(obj) is compatible to str()')
        self.assertEqual(str(o), 'unicode(obj) is compatible to str()')

        # %-formatting and .__unicode__()
        self.assertEqual(u'%s' %
                         UnicodeCompat(u"u'%s' % obj uses obj.__unicode__()"),
                         u"u'%s' % obj uses obj.__unicode__()")
        self.assertEqual(u'%s' %
                         UnicodeCompat(u"u'%s' % obj falls back to obj.__str__()"),
                         u"u'%s' % obj falls back to obj.__str__()")

        for obj in (123, 123.45, 123L):
            self.assertEqual(unicode(obj), unicode(str(obj)))

        # unicode(obj, encoding, error) tests (this maps to
        # PyUnicode_FromEncodedObject() at C level)

        if not sys.platform.startswith('java'):
            self.assertRaises(
                TypeError,
                unicode,
                u'decoding unicode is not supported',
                'utf-8',
                'strict'
            )

        self.assertEqual(
            unicode('strings are decoded to unicode', 'utf-8', 'strict'),
            u'strings are decoded to unicode'
        )

        if not sys.platform.startswith('java'):
            with test_support.check_py3k_warnings():
                buf = buffer('character buffers are decoded to unicode')
            self.assertEqual(
                unicode(
                    buf,
                    'utf-8',
                    'strict'
                ),
                u'character buffers are decoded to unicode'
            )

        self.assertRaises(TypeError, unicode, 42, 42, 42)

    def test_codecs_utf7(self):
        utfTests = [
            (u'A\u2262\u0391.', 'A+ImIDkQ.'),             # RFC2152 example
            (u'Hi Mom -\u263a-!', 'Hi Mom -+Jjo--!'),     # RFC2152 example
            (u'\u65E5\u672C\u8A9E', '+ZeVnLIqe-'),        # RFC2152 example
            (u'Item 3 is \u00a31.', 'Item 3 is +AKM-1.'), # RFC2152 example
            (u'+', '+-'),
            (u'+-', '+--'),
            (u'+?', '+-?'),
            (u'\?', '+AFw?'),
            (u'+?', '+-?'),
            (ur'\\?', '+AFwAXA?'),
            (ur'\\\?', '+AFwAXABc?'),
            (ur'++--', '+-+---'),
            (u'\U000abcde', '+2m/c3g-'),                  # surrogate pairs
            (u'/', '/'),
        ]

        for (x, y) in utfTests:
            self.assertEqual(x.encode('utf-7'), y)

        # Unpaired surrogates are passed through
        self.assertEqual(u'\uD801'.encode('utf-7'), '+2AE-')
        self.assertEqual(u'\uD801x'.encode('utf-7'), '+2AE-x')
        self.assertEqual(u'\uDC01'.encode('utf-7'), '+3AE-')
        self.assertEqual(u'\uDC01x'.encode('utf-7'), '+3AE-x')
        self.assertEqual('+2AE-'.decode('utf-7'), u'\uD801')
        self.assertEqual('+2AE-x'.decode('utf-7'), u'\uD801x')
        self.assertEqual('+3AE-'.decode('utf-7'), u'\uDC01')
        self.assertEqual('+3AE-x'.decode('utf-7'), u'\uDC01x')

        self.assertEqual(u'\uD801\U000abcde'.encode('utf-7'), '+2AHab9ze-')
        self.assertEqual('+2AHab9ze-'.decode('utf-7'), u'\uD801\U000abcde')

        # Direct encoded characters
        set_d = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'(),-./:?"
        # Optional direct characters
        set_o = '!"#$%&*;<=>@[]^_`{|}'
        for c in set_d:
            self.assertEqual(c.encode('utf7'), c.encode('ascii'))
            self.assertEqual(c.encode('ascii').decode('utf7'), unicode(c))
            self.assertTrue(c == c.encode('ascii').decode('utf7'))
        for c in set_o:
            self.assertEqual(c.encode('ascii').decode('utf7'), unicode(c))
            self.assertTrue(c == c.encode('ascii').decode('utf7'))

    def test_codecs_utf8(self):
        self.assertEqual(u''.encode('utf-8'), '')
        self.assertEqual(u'\u20ac'.encode('utf-8'), '\xe2\x82\xac')
        self.assertEqual(u'\ud800\udc02'.encode('utf-8'), '\xf0\x90\x80\x82')
        self.assertEqual(u'\ud84d\udc56'.encode('utf-8'), '\xf0\xa3\x91\x96')
        self.assertEqual(u'\ud800'.encode('utf-8'), '\xed\xa0\x80')
        self.assertEqual(u'\udc00'.encode('utf-8'), '\xed\xb0\x80')
        self.assertEqual(
            (u'\ud800\udc02'*1000).encode('utf-8'),
            '\xf0\x90\x80\x82'*1000
        )
        self.assertEqual(
            u'\u6b63\u78ba\u306b\u8a00\u3046\u3068\u7ffb\u8a33\u306f'
            u'\u3055\u308c\u3066\u3044\u307e\u305b\u3093\u3002\u4e00'
            u'\u90e8\u306f\u30c9\u30a4\u30c4\u8a9e\u3067\u3059\u304c'
            u'\u3001\u3042\u3068\u306f\u3067\u305f\u3089\u3081\u3067'
            u'\u3059\u3002\u5b9f\u969b\u306b\u306f\u300cWenn ist das'
            u' Nunstuck git und'.encode('utf-8'),
            '\xe6\xad\xa3\xe7\xa2\xba\xe3\x81\xab\xe8\xa8\x80\xe3\x81'
            '\x86\xe3\x81\xa8\xe7\xbf\xbb\xe8\xa8\xb3\xe3\x81\xaf\xe3'
            '\x81\x95\xe3\x82\x8c\xe3\x81\xa6\xe3\x81\x84\xe3\x81\xbe'
            '\xe3\x81\x9b\xe3\x82\x93\xe3\x80\x82\xe4\xb8\x80\xe9\x83'
            '\xa8\xe3\x81\xaf\xe3\x83\x89\xe3\x82\xa4\xe3\x83\x84\xe8'
            '\xaa\x9e\xe3\x81\xa7\xe3\x81\x99\xe3\x81\x8c\xe3\x80\x81'
            '\xe3\x81\x82\xe3\x81\xa8\xe3\x81\xaf\xe3\x81\xa7\xe3\x81'
            '\x9f\xe3\x82\x89\xe3\x82\x81\xe3\x81\xa7\xe3\x81\x99\xe3'
            '\x80\x82\xe5\xae\x9f\xe9\x9a\x9b\xe3\x81\xab\xe3\x81\xaf'
            '\xe3\x80\x8cWenn ist das Nunstuck git und'
        )

        # UTF-8 specific decoding tests
        self.assertEqual(unicode('\xf0\xa3\x91\x96', 'utf-8'), u'\U00023456')
        self.assertEqual(unicode('\xf0\x90\x80\x82', 'utf-8'), u'\U00010002')
        self.assertEqual(unicode('\xe2\x82\xac', 'utf-8'), u'\u20ac')

        # Other possible utf-8 test cases:
        # * strict decoding testing for all of the
        #   UTF8_ERROR cases in PyUnicode_DecodeUTF8

    def test_utf8_decode_valid_sequences(self):
        sequences = [
            # single byte
            ('\x00', u'\x00'), ('a', u'a'), ('\x7f', u'\x7f'),
            # 2 bytes
            ('\xc2\x80', u'\x80'), ('\xdf\xbf', u'\u07ff'),
            # 3 bytes
            ('\xe0\xa0\x80', u'\u0800'), ('\xed\x9f\xbf', u'\ud7ff'),
            ('\xee\x80\x80', u'\uE000'), ('\xef\xbf\xbf', u'\uffff'),
            # 4 bytes
            ('\xF0\x90\x80\x80', u'\U00010000'),
            ('\xf4\x8f\xbf\xbf', u'\U0010FFFF')
        ]
        for seq, res in sequences:
            self.assertEqual(seq.decode('utf-8'), res)

        for ch in map(unichr, range(0, sys.maxunicode)):
            self.assertEqual(ch, ch.encode('utf-8').decode('utf-8'))

    def test_utf8_decode_invalid_sequences(self):
        # continuation bytes in a sequence of 2, 3, or 4 bytes
        continuation_bytes = map(chr, range(0x80, 0xC0))
        # start bytes of a 2-byte sequence equivalent to codepoints < 0x7F
        invalid_2B_seq_start_bytes = map(chr, range(0xC0, 0xC2))
        # start bytes of a 4-byte sequence equivalent to codepoints > 0x10FFFF
        invalid_4B_seq_start_bytes = map(chr, range(0xF5, 0xF8))
        invalid_start_bytes = (
            continuation_bytes + invalid_2B_seq_start_bytes +
            invalid_4B_seq_start_bytes + map(chr, range(0xF7, 0x100))
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
                                      (sb+cb1+'\x80'+cb3).decode, 'utf-8')

        for cb in map(chr, range(0x80, 0xA0)):
            self.assertRaises(UnicodeDecodeError,
                              ('\xE0'+cb+'\x80').decode, 'utf-8')
            self.assertRaises(UnicodeDecodeError,
                              ('\xE0'+cb+'\xBF').decode, 'utf-8')
        # XXX: surrogates shouldn't be valid UTF-8!
        # see http://www.unicode.org/versions/Unicode5.2.0/ch03.pdf
        # (table 3-7) and http://www.rfc-editor.org/rfc/rfc3629.txt
        #for cb in map(chr, range(0xA0, 0xC0)):
            #self.assertRaises(UnicodeDecodeError,
                              #('\xED'+cb+'\x80').decode, 'utf-8')
            #self.assertRaises(UnicodeDecodeError,
                              #('\xED'+cb+'\xBF').decode, 'utf-8')
        # but since they are valid on Python 2 add a test for that:
        for cb, surrogate in zip(map(chr, range(0xA0, 0xC0)),
                                 map(unichr, range(0xd800, 0xe000, 64))):
            encoded = '\xED'+cb+'\x80'
            self.assertEqual(encoded.decode('utf-8'), surrogate)
            self.assertEqual(surrogate.encode('utf-8'), encoded)

        for cb in map(chr, range(0x80, 0x90)):
            self.assertRaises(UnicodeDecodeError,
                              ('\xF0'+cb+'\x80\x80').decode, 'utf-8')
            self.assertRaises(UnicodeDecodeError,
                              ('\xF0'+cb+'\xBF\xBF').decode, 'utf-8')
        for cb in map(chr, range(0x90, 0xC0)):
            self.assertRaises(UnicodeDecodeError,
                              ('\xF4'+cb+'\x80\x80').decode, 'utf-8')
            self.assertRaises(UnicodeDecodeError,
                              ('\xF4'+cb+'\xBF\xBF').decode, 'utf-8')

    def test_issue8271(self):
        # Issue #8271: during the decoding of an invalid UTF-8 byte sequence,
        # only the start byte and the continuation byte(s) are now considered
        # invalid, instead of the number of bytes specified by the start byte.
        # See http://www.unicode.org/versions/Unicode5.2.0/ch03.pdf (page 95,
        # table 3-8, Row 2) for more information about the algorithm used.
        FFFD = u'\ufffd'
        sequences = [
            # invalid start bytes
            ('\x80', FFFD), # continuation byte
            ('\x80\x80', FFFD*2), # 2 continuation bytes
            ('\xc0', FFFD),
            ('\xc0\xc0', FFFD*2),
            ('\xc1', FFFD),
            ('\xc1\xc0', FFFD*2),
            ('\xc0\xc1', FFFD*2),
            # with start byte of a 2-byte sequence
            ('\xc2', FFFD), # only the start byte
            ('\xc2\xc2', FFFD*2), # 2 start bytes
            ('\xc2\xc2\xc2', FFFD*3), # 2 start bytes
            ('\xc2\x41', FFFD+'A'), # invalid continuation byte
            # with start byte of a 3-byte sequence
            ('\xe1', FFFD), # only the start byte
            ('\xe1\xe1', FFFD*2), # 2 start bytes
            ('\xe1\xe1\xe1', FFFD*3), # 3 start bytes
            ('\xe1\xe1\xe1\xe1', FFFD*4), # 4 start bytes
            ('\xe1\x80', FFFD), # only 1 continuation byte
            ('\xe1\x41', FFFD+'A'), # invalid continuation byte
            ('\xe1\x41\x80', FFFD+'A'+FFFD), # invalid cb followed by valid cb
            ('\xe1\x41\x41', FFFD+'AA'), # 2 invalid continuation bytes
            ('\xe1\x80\x41', FFFD+'A'), # only 1 valid continuation byte
            ('\xe1\x80\xe1\x41', FFFD*2+'A'), # 1 valid and the other invalid
            ('\xe1\x41\xe1\x80', FFFD+'A'+FFFD), # 1 invalid and the other valid
            # with start byte of a 4-byte sequence
            ('\xf1', FFFD), # only the start byte
            ('\xf1\xf1', FFFD*2), # 2 start bytes
            ('\xf1\xf1\xf1', FFFD*3), # 3 start bytes
            ('\xf1\xf1\xf1\xf1', FFFD*4), # 4 start bytes
            ('\xf1\xf1\xf1\xf1\xf1', FFFD*5), # 5 start bytes
            ('\xf1\x80', FFFD), # only 1 continuation bytes
            ('\xf1\x80\x80', FFFD), # only 2 continuation bytes
            ('\xf1\x80\x41', FFFD+'A'), # 1 valid cb and 1 invalid
            ('\xf1\x80\x41\x41', FFFD+'AA'), # 1 valid cb and 1 invalid
            ('\xf1\x80\x80\x41', FFFD+'A'), # 2 valid cb and 1 invalid
            ('\xf1\x41\x80', FFFD+'A'+FFFD), # 1 invalid cv and 1 valid
            ('\xf1\x41\x80\x80', FFFD+'A'+FFFD*2), # 1 invalid cb and 2 invalid
            ('\xf1\x41\x80\x41', FFFD+'A'+FFFD+'A'), # 2 invalid cb and 1 invalid
            ('\xf1\x41\x41\x80', FFFD+'AA'+FFFD), # 1 valid cb and 1 invalid
            ('\xf1\x41\xf1\x80', FFFD+'A'+FFFD),
            ('\xf1\x41\x80\xf1', FFFD+'A'+FFFD*2),
            ('\xf1\xf1\x80\x41', FFFD*2+'A'),
            ('\xf1\x41\xf1\xf1', FFFD+'A'+FFFD*2),
            # with invalid start byte of a 4-byte sequence (rfc2279)
            ('\xf5', FFFD), # only the start byte
            ('\xf5\xf5', FFFD*2), # 2 start bytes
            ('\xf5\x80', FFFD*2), # only 1 continuation byte
            ('\xf5\x80\x80', FFFD*3), # only 2 continuation byte
            ('\xf5\x80\x80\x80', FFFD*4), # 3 continuation bytes
            ('\xf5\x80\x41', FFFD*2+'A'), #  1 valid cb and 1 invalid
            ('\xf5\x80\x41\xf5', FFFD*2+'A'+FFFD),
            ('\xf5\x41\x80\x80\x41', FFFD+'A'+FFFD*2+'A'),
            # with invalid start byte of a 5-byte sequence (rfc2279)
            ('\xf8', FFFD), # only the start byte
            ('\xf8\xf8', FFFD*2), # 2 start bytes
            ('\xf8\x80', FFFD*2), # only one continuation byte
            ('\xf8\x80\x41', FFFD*2 + 'A'), # 1 valid cb and 1 invalid
            ('\xf8\x80\x80\x80\x80', FFFD*5), # invalid 5 bytes seq with 5 bytes
            # with invalid start byte of a 6-byte sequence (rfc2279)
            ('\xfc', FFFD), # only the start byte
            ('\xfc\xfc', FFFD*2), # 2 start bytes
            ('\xfc\x80\x80', FFFD*3), # only 2 continuation bytes
            ('\xfc\x80\x80\x80\x80\x80', FFFD*6), # 6 continuation bytes
            # invalid start byte
            ('\xfe', FFFD),
            ('\xfe\x80\x80', FFFD*3),
            # other sequences
            ('\xf1\x80\x41\x42\x43', u'\ufffd\x41\x42\x43'),
            ('\xf1\x80\xff\x42\x43', u'\ufffd\ufffd\x42\x43'),
            ('\xf1\x80\xc2\x81\x43', u'\ufffd\x81\x43'),
            ('\x61\xF1\x80\x80\xE1\x80\xC2\x62\x80\x63\x80\xBF\x64',
             u'\x61\uFFFD\uFFFD\uFFFD\x62\uFFFD\x63\uFFFD\uFFFD\x64'),
        ]
        for n, (seq, res) in enumerate(sequences):
            self.assertRaises(UnicodeDecodeError, seq.decode, 'utf-8', 'strict')
            self.assertEqual(seq.decode('utf-8', 'replace'), res)
            self.assertEqual((seq+'b').decode('utf-8', 'replace'), res+'b')
            self.assertEqual(seq.decode('utf-8', 'ignore'),
                             res.replace(u'\uFFFD', ''))

    def test_codecs_idna(self):
        # Test whether trailing dot is preserved
        self.assertEqual(u"www.python.org.".encode("idna"), "www.python.org.")

    def test_codecs_errors(self):
        # Error handling (encoding)
        self.assertRaises(UnicodeError, u'Andr\202 x'.encode, 'ascii')
        self.assertRaises(UnicodeError, u'Andr\202 x'.encode, 'ascii','strict')
        self.assertEqual(u'Andr\202 x'.encode('ascii','ignore'), "Andr x")
        self.assertEqual(u'Andr\202 x'.encode('ascii','replace'), "Andr? x")
        self.assertEqual(u'Andr\202 x'.encode('ascii', 'replace'),
                         u'Andr\202 x'.encode('ascii', errors='replace'))
        self.assertEqual(u'Andr\202 x'.encode('ascii', 'ignore'),
                         u'Andr\202 x'.encode(encoding='ascii', errors='ignore'))

        # Error handling (decoding)
        self.assertRaises(UnicodeError, unicode, 'Andr\202 x', 'ascii')
        self.assertRaises(UnicodeError, unicode, 'Andr\202 x', 'ascii','strict')
        self.assertEqual(unicode('Andr\202 x','ascii','ignore'), u"Andr x")
        self.assertEqual(unicode('Andr\202 x','ascii','replace'), u'Andr\uFFFD x')
        self.assertEqual(u'abcde'.decode('ascii', 'ignore'),
                         u'abcde'.decode('ascii', errors='ignore'))
        self.assertEqual(u'abcde'.decode('ascii', 'replace'),
                         u'abcde'.decode(encoding='ascii', errors='replace'))

        # Error handling (unknown character names)
        self.assertEqual("\\N{foo}xx".decode("unicode-escape", "ignore"), u"xx")

        # Error handling (truncated escape sequence)
        self.assertRaises(UnicodeError, "\\".decode, "unicode-escape")

        self.assertRaises(TypeError, "hello".decode, "test.unicode1")
        self.assertRaises(TypeError, unicode, "hello", "test.unicode2")
        self.assertRaises(TypeError, u"hello".encode, "test.unicode1")
        self.assertRaises(TypeError, u"hello".encode, "test.unicode2")
        # executes PyUnicode_Encode()
        import imp
        self.assertRaises(
            ImportError,
            imp.find_module,
            "non-existing module",
            [u"non-existing dir"]
        )

        # Error handling (wrong arguments)
        self.assertRaises(TypeError, u"hello".encode, 42, 42, 42)

        # Error handling (PyUnicode_EncodeDecimal())
        self.assertRaises(UnicodeError, int, u"\u0200")

    def test_codecs(self):
        # Encoding
        self.assertEqual(u'hello'.encode('ascii'), 'hello')
        self.assertEqual(u'hello'.encode('utf-7'), 'hello')
        self.assertEqual(u'hello'.encode('utf-8'), 'hello')
        self.assertEqual(u'hello'.encode('utf8'), 'hello')
        self.assertEqual(u'hello'.encode('utf-16-le'), 'h\000e\000l\000l\000o\000')
        self.assertEqual(u'hello'.encode('utf-16-be'), '\000h\000e\000l\000l\000o')
        self.assertEqual(u'hello'.encode('latin-1'), 'hello')

        # Roundtrip safety for BMP (just the first 1024 chars)
        for c in xrange(1024):
            u = unichr(c)
            for encoding in ('utf-7', 'utf-8', 'utf-16', 'utf-16-le',
                             'utf-16-be', 'raw_unicode_escape',
                             'unicode_escape', 'unicode_internal'):
                self.assertEqual(unicode(u.encode(encoding),encoding), u)

        # Roundtrip safety for BMP (just the first 256 chars)
        for c in xrange(256):
            u = unichr(c)
            for encoding in ('latin-1',):
                self.assertEqual(unicode(u.encode(encoding),encoding), u)

        # Roundtrip safety for BMP (just the first 128 chars)
        for c in xrange(128):
            u = unichr(c)
            for encoding in ('ascii',):
                self.assertEqual(unicode(u.encode(encoding),encoding), u)

        # Roundtrip safety for non-BMP (just a few chars)
        u = u'\U00010001\U00020002\U00030003\U00040004\U00050005'
        for encoding in ('utf-8', 'utf-16', 'utf-16-le', 'utf-16-be',
                         #'raw_unicode_escape',
                         'unicode_escape', 'unicode_internal'):
            self.assertEqual(unicode(u.encode(encoding),encoding), u)

        # UTF-8 must be roundtrip safe for all UCS-2 code points
        # This excludes surrogates: in the full range, there would be
        # a surrogate pair (\udbff\udc00), which gets converted back
        # to a non-BMP character (\U0010fc00)
        u = u''.join(map(unichr, range(0,0xd800)+range(0xe000,0x10000)))
        for encoding in ('utf-8',):
            self.assertEqual(unicode(u.encode(encoding),encoding), u)

    def test_codecs_charmap(self):
        # 0-127
        s = ''.join(map(chr, xrange(128)))
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
            self.assertEqual(unicode(s, encoding).encode(encoding), s)

        # 128-255
        s = ''.join(map(chr, xrange(128, 256)))
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
            self.assertEqual(unicode(s, encoding).encode(encoding), s)

    def test_concatenation(self):
        self.assertEqual((u"abc" u"def"), u"abcdef")
        self.assertEqual(("abc" u"def"), u"abcdef")
        self.assertEqual((u"abc" "def"), u"abcdef")
        self.assertEqual((u"abc" u"def" "ghi"), u"abcdefghi")
        self.assertEqual(("abc" "def" u"ghi"), u"abcdefghi")

    def test_printing(self):
        class BitBucket:
            def write(self, text):
                pass

        out = BitBucket()
        print >>out, u'abc'
        print >>out, u'abc', u'def'
        print >>out, u'abc', 'def'
        print >>out, 'abc', u'def'
        print >>out, u'abc\n'
        print >>out, u'abc\n',
        print >>out, u'abc\n',
        print >>out, u'def\n'
        print >>out, u'def\n'

    def test_ucs4(self):
        x = u'\U00100000'
        y = x.encode("raw-unicode-escape").decode("raw-unicode-escape")
        self.assertEqual(x, y)

        y = r'\U00100000'
        x = y.decode("raw-unicode-escape").encode("raw-unicode-escape")
        self.assertEqual(x, y)
        y = r'\U00010000'
        x = y.decode("raw-unicode-escape").encode("raw-unicode-escape")
        self.assertEqual(x, y)

        try:
            '\U11111111'.decode("raw-unicode-escape")
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
            def __unicode__(self):
                return u"foo"

        class Foo2(object):
            def __unicode__(self):
                return u"foo"

        class Foo3(object):
            def __unicode__(self):
                return "foo"

        class Foo4(str):
            def __unicode__(self):
                return "foo"

        class Foo5(unicode):
            def __unicode__(self):
                return "foo"

        class Foo6(str):
            def __str__(self):
                return "foos"

            def __unicode__(self):
                return u"foou"

        class Foo7(unicode):
            def __str__(self):
                return "foos"
            def __unicode__(self):
                return u"foou"

        class Foo8(unicode):
            def __new__(cls, content=""):
                return unicode.__new__(cls, 2*content)
            def __unicode__(self):
                return self

        class Foo9(unicode):
            def __str__(self):
                return "string"
            def __unicode__(self):
                return "not unicode"

        self.assertEqual(unicode(Foo0()), u"foo")
        self.assertEqual(unicode(Foo1()), u"foo")
        self.assertEqual(unicode(Foo2()), u"foo")
        self.assertEqual(unicode(Foo3()), u"foo")
        self.assertEqual(unicode(Foo4("bar")), u"foo")
        self.assertEqual(unicode(Foo5("bar")), u"foo")
        self.assertEqual(unicode(Foo6("bar")), u"foou")
        self.assertEqual(unicode(Foo7("bar")), u"foou")
        self.assertEqual(unicode(Foo8("foo")), u"foofoo")
        self.assertEqual(str(Foo9("foo")), "string")
        self.assertEqual(unicode(Foo9("foo")), u"not unicode")

    def test_unicode_repr(self):
        class s1:
            def __repr__(self):
                return '\\n'

        class s2:
            def __repr__(self):
                return u'\\n'

        self.assertEqual(repr(s1()), '\\n')
        self.assertEqual(repr(s2()), '\\n')

    # This test only affects 32-bit platforms because expandtabs can only take
    # an int as the max value, not a 64-bit C long.  If expandtabs is changed
    # to take a 64-bit long, this test should apply to all platforms.
    @unittest.skipIf(sys.maxint > (1 << 32) or struct.calcsize('P') != 4,
                     'only applies to 32-bit platforms')
    def test_expandtabs_overflows_gracefully(self):
        self.assertRaises(OverflowError, u't\tt\t'.expandtabs, sys.maxint)

    def test__format__(self):
        def test(value, format, expected):
            # test both with and without the trailing 's'
            self.assertEqual(value.__format__(format), expected)
            self.assertEqual(value.__format__(format + u's'), expected)

        test(u'', u'', u'')
        test(u'abc', u'', u'abc')
        test(u'abc', u'.3', u'abc')
        test(u'ab', u'.3', u'ab')
        test(u'abcdef', u'.3', u'abc')
        test(u'abcdef', u'.0', u'')
        test(u'abc', u'3.3', u'abc')
        test(u'abc', u'2.3', u'abc')
        test(u'abc', u'2.2', u'ab')
        test(u'abc', u'3.2', u'ab ')
        test(u'result', u'x<0', u'result')
        test(u'result', u'x<5', u'result')
        test(u'result', u'x<6', u'result')
        test(u'result', u'x<7', u'resultx')
        test(u'result', u'x<8', u'resultxx')
        test(u'result', u' <7', u'result ')
        test(u'result', u'<7', u'result ')
        test(u'result', u'>7', u' result')
        test(u'result', u'>8', u'  result')
        test(u'result', u'^8', u' result ')
        test(u'result', u'^9', u' result  ')
        test(u'result', u'^10', u'  result  ')
        test(u'a', u'10000', u'a' + u' ' * 9999)
        test(u'', u'10000', u' ' * 10000)
        test(u'', u'10000000', u' ' * 10000000)

        # test mixing unicode and str
        self.assertEqual(u'abc'.__format__('s'), u'abc')
        self.assertEqual(u'abc'.__format__('->10s'), u'-------abc')

    def test_format(self):
        self.assertEqual(u''.format(), u'')
        self.assertEqual(u'a'.format(), u'a')
        self.assertEqual(u'ab'.format(), u'ab')
        self.assertEqual(u'a{{'.format(), u'a{')
        self.assertEqual(u'a}}'.format(), u'a}')
        self.assertEqual(u'{{b'.format(), u'{b')
        self.assertEqual(u'}}b'.format(), u'}b')
        self.assertEqual(u'a{{b'.format(), u'a{b')

        # examples from the PEP:
        import datetime
        self.assertEqual(u"My name is {0}".format(u'Fred'), u"My name is Fred")
        self.assertEqual(u"My name is {0[name]}".format(dict(name=u'Fred')),
                         u"My name is Fred")
        self.assertEqual(u"My name is {0} :-{{}}".format(u'Fred'),
                         u"My name is Fred :-{}")

        # datetime.__format__ doesn't work with unicode
        #d = datetime.date(2007, 8, 18)
        #self.assertEqual("The year is {0.year}".format(d),
        #                 "The year is 2007")

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
                return u'E(' + self.x + u')'

        # class with __repr__, but no __format__ or __str__
        class F:
            def __init__(self, x):
                self.x = x
            def __repr__(self):
                return u'F(' + self.x + u')'

        # class with __format__ that forwards to string, for some format_spec's
        class G:
            def __init__(self, x):
                self.x = x
            def __str__(self):
                return u"string is " + self.x
            def __format__(self, format_spec):
                if format_spec == 'd':
                    return u'G(' + self.x + u')'
                return object.__format__(self, format_spec)

        # class that returns a bad type from __format__
        class H:
            def __format__(self, format_spec):
                return 1.0

        class I(datetime.date):
            def __format__(self, format_spec):
                return self.strftime(format_spec)

        class J(int):
            def __format__(self, format_spec):
                return int.__format__(self * 2, format_spec)


        self.assertEqual(u''.format(), u'')
        self.assertEqual(u'abc'.format(), u'abc')
        self.assertEqual(u'{0}'.format(u'abc'), u'abc')
        self.assertEqual(u'{0:}'.format(u'abc'), u'abc')
        self.assertEqual(u'X{0}'.format(u'abc'), u'Xabc')
        self.assertEqual(u'{0}X'.format(u'abc'), u'abcX')
        self.assertEqual(u'X{0}Y'.format(u'abc'), u'XabcY')
        self.assertEqual(u'{1}'.format(1, u'abc'), u'abc')
        self.assertEqual(u'X{1}'.format(1, u'abc'), u'Xabc')
        self.assertEqual(u'{1}X'.format(1, u'abc'), u'abcX')
        self.assertEqual(u'X{1}Y'.format(1, u'abc'), u'XabcY')
        self.assertEqual(u'{0}'.format(-15), u'-15')
        self.assertEqual(u'{0}{1}'.format(-15, u'abc'), u'-15abc')
        self.assertEqual(u'{0}X{1}'.format(-15, u'abc'), u'-15Xabc')
        self.assertEqual(u'{{'.format(), u'{')
        self.assertEqual(u'}}'.format(), u'}')
        self.assertEqual(u'{{}}'.format(), u'{}')
        self.assertEqual(u'{{x}}'.format(), u'{x}')
        self.assertEqual(u'{{{0}}}'.format(123), u'{123}')
        self.assertEqual(u'{{{{0}}}}'.format(), u'{{0}}')
        self.assertEqual(u'}}{{'.format(), u'}{')
        self.assertEqual(u'}}x{{'.format(), u'}x{')

        # weird field names
        self.assertEqual(u"{0[foo-bar]}".format({u'foo-bar':u'baz'}), u'baz')
        self.assertEqual(u"{0[foo bar]}".format({u'foo bar':u'baz'}), u'baz')
        self.assertEqual(u"{0[ ]}".format({u' ':3}), u'3')

        self.assertEqual(u'{foo._x}'.format(foo=C(20)), u'20')
        self.assertEqual(u'{1}{0}'.format(D(10), D(20)), u'2010')
        self.assertEqual(u'{0._x.x}'.format(C(D(u'abc'))), u'abc')
        self.assertEqual(u'{0[0]}'.format([u'abc', u'def']), u'abc')
        self.assertEqual(u'{0[1]}'.format([u'abc', u'def']), u'def')
        self.assertEqual(u'{0[1][0]}'.format([u'abc', [u'def']]), u'def')
        self.assertEqual(u'{0[1][0].x}'.format(['abc', [D(u'def')]]), u'def')

        # strings
        self.assertEqual(u'{0:.3s}'.format(u'abc'), u'abc')
        self.assertEqual(u'{0:.3s}'.format(u'ab'), u'ab')
        self.assertEqual(u'{0:.3s}'.format(u'abcdef'), u'abc')
        self.assertEqual(u'{0:.0s}'.format(u'abcdef'), u'')
        self.assertEqual(u'{0:3.3s}'.format(u'abc'), u'abc')
        self.assertEqual(u'{0:2.3s}'.format(u'abc'), u'abc')
        self.assertEqual(u'{0:2.2s}'.format(u'abc'), u'ab')
        self.assertEqual(u'{0:3.2s}'.format(u'abc'), u'ab ')
        self.assertEqual(u'{0:x<0s}'.format(u'result'), u'result')
        self.assertEqual(u'{0:x<5s}'.format(u'result'), u'result')
        self.assertEqual(u'{0:x<6s}'.format(u'result'), u'result')
        self.assertEqual(u'{0:x<7s}'.format(u'result'), u'resultx')
        self.assertEqual(u'{0:x<8s}'.format(u'result'), u'resultxx')
        self.assertEqual(u'{0: <7s}'.format(u'result'), u'result ')
        self.assertEqual(u'{0:<7s}'.format(u'result'), u'result ')
        self.assertEqual(u'{0:>7s}'.format(u'result'), u' result')
        self.assertEqual(u'{0:>8s}'.format(u'result'), u'  result')
        self.assertEqual(u'{0:^8s}'.format(u'result'), u' result ')
        self.assertEqual(u'{0:^9s}'.format(u'result'), u' result  ')
        self.assertEqual(u'{0:^10s}'.format(u'result'), u'  result  ')
        self.assertEqual(u'{0:10000}'.format(u'a'), u'a' + u' ' * 9999)
        self.assertEqual(u'{0:10000}'.format(u''), u' ' * 10000)
        self.assertEqual(u'{0:10000000}'.format(u''), u' ' * 10000000)

        # issue 12546: use \x00 as a fill character
        self.assertEqual('{0:\x00<6s}'.format('foo'), 'foo\x00\x00\x00')
        self.assertEqual('{0:\x01<6s}'.format('foo'), 'foo\x01\x01\x01')
        self.assertEqual('{0:\x00^6s}'.format('foo'), '\x00foo\x00\x00')
        self.assertEqual('{0:^6s}'.format('foo'), ' foo  ')

        self.assertEqual('{0:\x00<6}'.format(3), '3\x00\x00\x00\x00\x00')
        self.assertEqual('{0:\x01<6}'.format(3), '3\x01\x01\x01\x01\x01')
        self.assertEqual('{0:\x00^6}'.format(3), '\x00\x003\x00\x00\x00')
        self.assertEqual('{0:<6}'.format(3), '3     ')

        self.assertEqual('{0:\x00<6}'.format(3.14), '3.14\x00\x00')
        self.assertEqual('{0:\x01<6}'.format(3.14), '3.14\x01\x01')
        self.assertEqual('{0:\x00^6}'.format(3.14), '\x003.14\x00')
        self.assertEqual('{0:^6}'.format(3.14), ' 3.14 ')

        self.assertEqual('{0:\x00<12}'.format(3+2.0j), '(3+2j)\x00\x00\x00\x00\x00\x00')
        self.assertEqual('{0:\x01<12}'.format(3+2.0j), '(3+2j)\x01\x01\x01\x01\x01\x01')
        self.assertEqual('{0:\x00^12}'.format(3+2.0j), '\x00\x00\x00(3+2j)\x00\x00\x00')
        self.assertEqual('{0:^12}'.format(3+2.0j), '   (3+2j)   ')

        # format specifiers for user defined type
        self.assertEqual(u'{0:abc}'.format(C()), u'abc')

        # !r and !s coercions
        self.assertEqual(u'{0!s}'.format(u'Hello'), u'Hello')
        self.assertEqual(u'{0!s:}'.format(u'Hello'), u'Hello')
        self.assertEqual(u'{0!s:15}'.format(u'Hello'), u'Hello          ')
        self.assertEqual(u'{0!s:15s}'.format(u'Hello'), u'Hello          ')
        self.assertEqual(u'{0!r}'.format(u'Hello'), u"u'Hello'")
        self.assertEqual(u'{0!r:}'.format(u'Hello'), u"u'Hello'")
        self.assertEqual(u'{0!r}'.format(F(u'Hello')), u'F(Hello)')

        # test fallback to object.__format__
        self.assertEqual(u'{0}'.format({}), u'{}')
        self.assertEqual(u'{0}'.format([]), u'[]')
        self.assertEqual(u'{0}'.format([1]), u'[1]')
        self.assertEqual(u'{0}'.format(E(u'data')), u'E(data)')
        self.assertEqual(u'{0:d}'.format(G(u'data')), u'G(data)')
        self.assertEqual(u'{0!s}'.format(G(u'data')), u'string is data')

        msg = 'object.__format__ with a non-empty format string is deprecated'
        with test_support.check_warnings((msg, PendingDeprecationWarning)):
            self.assertEqual(u'{0:^10}'.format(E(u'data')), u' E(data)  ')
            self.assertEqual(u'{0:^10s}'.format(E(u'data')), u' E(data)  ')
            self.assertEqual(u'{0:>15s}'.format(G(u'data')), u' string is data')

        self.assertEqual(u"{0:date: %Y-%m-%d}".format(I(year=2007,
                                                        month=8,
                                                        day=27)),
                         u"date: 2007-08-27")

        # test deriving from a builtin type and overriding __format__
        self.assertEqual(u"{0}".format(J(10)), u"20")


        # string format specifiers
        self.assertEqual(u'{0:}'.format('a'), u'a')

        # computed format specifiers
        self.assertEqual(u"{0:.{1}}".format(u'hello world', 5), u'hello')
        self.assertEqual(u"{0:.{1}s}".format(u'hello world', 5), u'hello')
        self.assertEqual(u"{0:.{precision}s}".format('hello world', precision=5), u'hello')
        self.assertEqual(u"{0:{width}.{precision}s}".format('hello world', width=10, precision=5), u'hello     ')
        self.assertEqual(u"{0:{width}.{precision}s}".format('hello world', width='10', precision='5'), u'hello     ')

        # test various errors
        self.assertRaises(ValueError, u'{'.format)
        self.assertRaises(ValueError, u'}'.format)
        self.assertRaises(ValueError, u'a{'.format)
        self.assertRaises(ValueError, u'a}'.format)
        self.assertRaises(ValueError, u'{a'.format)
        self.assertRaises(ValueError, u'}a'.format)
        self.assertRaises(IndexError, u'{0}'.format)
        self.assertRaises(IndexError, u'{1}'.format, u'abc')
        self.assertRaises(KeyError,   u'{x}'.format)
        self.assertRaises(ValueError, u"}{".format)
        self.assertRaises(ValueError, u"{".format)
        self.assertRaises(ValueError, u"}".format)
        self.assertRaises(ValueError, u"abc{0:{}".format)
        self.assertRaises(ValueError, u"{0".format)
        self.assertRaises(IndexError, u"{0.}".format)
        self.assertRaises(ValueError, u"{0.}".format, 0)
        self.assertRaises(IndexError, u"{0[}".format)
        self.assertRaises(ValueError, u"{0[}".format, [])
        self.assertRaises(KeyError,   u"{0]}".format)
        self.assertRaises(ValueError, u"{0.[]}".format, 0)
        self.assertRaises(ValueError, u"{0..foo}".format, 0)
        self.assertRaises(ValueError, u"{0[0}".format, 0)
        self.assertRaises(ValueError, u"{0[0:foo}".format, 0)
        self.assertRaises(KeyError,   u"{c]}".format)
        self.assertRaises(ValueError, u"{{ {{{0}}".format, 0)
        self.assertRaises(ValueError, u"{0}}".format, 0)
        self.assertRaises(KeyError,   u"{foo}".format, bar=3)
        self.assertRaises(ValueError, u"{0!x}".format, 3)
        self.assertRaises(ValueError, u"{0!}".format, 0)
        self.assertRaises(ValueError, u"{0!rs}".format, 0)
        self.assertRaises(ValueError, u"{!}".format)
        self.assertRaises(IndexError, u"{:}".format)
        self.assertRaises(IndexError, u"{:s}".format)
        self.assertRaises(IndexError, u"{}".format)
        big = u"23098475029384702983476098230754973209482573"
        self.assertRaises(ValueError, (u"{" + big + u"}").format)
        self.assertRaises(ValueError, (u"{[" + big + u"]}").format, [0])

        # issue 6089
        self.assertRaises(ValueError, u"{0[0]x}".format, [None])
        self.assertRaises(ValueError, u"{0[0](10)}".format, [None])

        # can't have a replacement on the field name portion
        self.assertRaises(TypeError, u'{0[{1}]}'.format, u'abcdefg', 4)

        # exceed maximum recursion depth
        self.assertRaises(ValueError, u"{0:{1:{2}}}".format, u'abc', u's', u'')
        self.assertRaises(ValueError, u"{0:{1:{2:{3:{4:{5:{6}}}}}}}".format,
                          0, 1, 2, 3, 4, 5, 6, 7)

        # string format spec errors
        self.assertRaises(ValueError, u"{0:-s}".format, u'')
        self.assertRaises(ValueError, format, u"", u"-")
        self.assertRaises(ValueError, u"{0:=s}".format, u'')

        # test combining string and unicode
        self.assertEqual(u"foo{0}".format('bar'), u'foobar')
        # This will try to convert the argument from unicode to str, which
        #  will succeed
        self.assertEqual("foo{0}".format(u'bar'), 'foobar')
        # This will try to convert the argument from unicode to str, which
        #  will fail
        self.assertRaises(UnicodeEncodeError, "foo{0}".format, u'\u1000bar')

    def test_format_huge_precision(self):
        format_string = u".{}f".format(sys.maxsize + 1)
        with self.assertRaises(ValueError):
            result = format(2.34, format_string)

    def test_format_huge_width(self):
        format_string = u"{}f".format(sys.maxsize + 1)
        with self.assertRaises(ValueError):
            result = format(2.34, format_string)

    def test_format_huge_item_number(self):
        format_string = u"{{{}:.6f}}".format(sys.maxsize + 1)
        with self.assertRaises(ValueError):
            result = format_string.format(2.34)

    def test_format_auto_numbering(self):
        class C:
            def __init__(self, x=100):
                self._x = x
            def __format__(self, spec):
                return spec

        self.assertEqual(u'{}'.format(10), u'10')
        self.assertEqual(u'{:5}'.format('s'), u's    ')
        self.assertEqual(u'{!r}'.format('s'), u"'s'")
        self.assertEqual(u'{._x}'.format(C(10)), u'10')
        self.assertEqual(u'{[1]}'.format([1, 2]), u'2')
        self.assertEqual(u'{[a]}'.format({'a':4, 'b':2}), u'4')
        self.assertEqual(u'a{}b{}c'.format(0, 1), u'a0b1c')

        self.assertEqual(u'a{:{}}b'.format('x', '^10'), u'a    x     b')
        self.assertEqual(u'a{:{}x}b'.format(20, '#'), u'a0x14b')

        # can't mix and match numbering and auto-numbering
        self.assertRaises(ValueError, u'{}{1}'.format, 1, 2)
        self.assertRaises(ValueError, u'{1}{}'.format, 1, 2)
        self.assertRaises(ValueError, u'{:{1}}'.format, 1, 2)
        self.assertRaises(ValueError, u'{0:{}}'.format, 1, 2)

        # can mix and match auto-numbering and named
        self.assertEqual(u'{f}{}'.format(4, f='test'), u'test4')
        self.assertEqual(u'{}{f}'.format(4, f='test'), u'4test')
        self.assertEqual(u'{:{f}}{g}{}'.format(1, 3, g='g', f=2), u' 1g3')
        self.assertEqual(u'{f:{}}{}{g}'.format(2, 4, f=1, g='g'), u' 14g')

    def test_raiseMemError(self):
        # Ensure that the freelist contains a consistent object, even
        # when a string allocation fails with a MemoryError.
        # This used to crash the interpreter,
        # or leak references when the number was smaller.
        charwidth = 4 if sys.maxunicode >= 0x10000 else 2
        # Note: sys.maxsize is half of the actual max allocation because of
        # the signedness of Py_ssize_t.
        alloc = lambda: u"a" * (sys.maxsize // charwidth * 2)
        self.assertRaises(MemoryError, alloc)
        self.assertRaises(MemoryError, alloc)

    def test_format_subclass(self):
        class U(unicode):
            def __unicode__(self):
                return u'__unicode__ overridden'
        u = U(u'xxx')
        self.assertEqual("%s" % u, u'__unicode__ overridden')
        self.assertEqual("{}".format(u), '__unicode__ overridden')

    # Test PyUnicode_FromFormat()
    def test_from_format(self):
        test_support.import_module('ctypes')
        from ctypes import (
            pythonapi, py_object, sizeof,
            c_int, c_long, c_longlong, c_ssize_t,
            c_uint, c_ulong, c_ulonglong, c_size_t, c_void_p)
        if sys.maxunicode == 0xffff:
            name = "PyUnicodeUCS2_FromFormat"
        else:
            name = "PyUnicodeUCS4_FromFormat"
        _PyUnicode_FromFormat = getattr(pythonapi, name)
        _PyUnicode_FromFormat.restype = py_object

        def PyUnicode_FromFormat(format, *args):
            cargs = tuple(
                py_object(arg) if isinstance(arg, unicode) else arg
                for arg in args)
            return _PyUnicode_FromFormat(format, *cargs)

        def check_format(expected, format, *args):
            text = PyUnicode_FromFormat(format, *args)
            self.assertEqual(expected, text)

        # ascii format, non-ascii argument
        check_format(u'ascii\x7f=unicode\xe9',
                     b'ascii\x7f=%U', u'unicode\xe9')

        # non-ascii format, ascii argument: ensure that PyUnicode_FromFormatV()
        # raises an error
        #self.assertRaisesRegex(ValueError,
        #    '^PyUnicode_FromFormatV\(\) expects an ASCII-encoded format '
        #    'string, got a non-ASCII byte: 0xe9$',
        #    PyUnicode_FromFormat, b'unicode\xe9=%s', u'ascii')

        # test "%c"
        check_format(u'\uabcd',
                     b'%c', c_int(0xabcd))
        if sys.maxunicode > 0xffff:
            check_format(u'\U0010ffff',
                         b'%c', c_int(0x10ffff))
        with self.assertRaises(OverflowError):
            PyUnicode_FromFormat(b'%c', c_int(0x110000))
        # Issue #18183
        if sys.maxunicode > 0xffff:
            check_format(u'\U00010000\U00100000',
                         b'%c%c', c_int(0x10000), c_int(0x100000))

        # test "%"
        check_format(u'%',
                     b'%')
        check_format(u'%',
                     b'%%')
        check_format(u'%s',
                     b'%%s')
        check_format(u'[%]',
                     b'[%%]')
        check_format(u'%abc',
                     b'%%%s', b'abc')

        # test %S
        check_format(u"repr=abc",
                     b'repr=%S', u'abc')

        # test %R
        check_format(u"repr=u'abc'",
                     b'repr=%R', u'abc')

        # test integer formats (%i, %d, %u)
        check_format(u'010',
                     b'%03i', c_int(10))
        check_format(u'0010',
                     b'%0.4i', c_int(10))
        check_format(u'-123',
                     b'%i', c_int(-123))

        check_format(u'-123',
                     b'%d', c_int(-123))
        check_format(u'-123',
                     b'%ld', c_long(-123))
        check_format(u'-123',
                     b'%zd', c_ssize_t(-123))

        check_format(u'123',
                     b'%u', c_uint(123))
        check_format(u'123',
                     b'%lu', c_ulong(123))
        check_format(u'123',
                     b'%zu', c_size_t(123))

        # test long output
        PyUnicode_FromFormat(b'%p', c_void_p(-1))

        # test %V
        check_format(u'repr=abc',
                     b'repr=%V', u'abc', b'xyz')
        check_format(u'repr=\xe4\xba\xba\xe6\xb0\x91',
                     b'repr=%V', None, b'\xe4\xba\xba\xe6\xb0\x91')
        check_format(u'repr=abc\xff',
                     b'repr=%V', None, b'abc\xff')

        # not supported: copy the raw format string. these tests are just here
        # to check for crashs and should not be considered as specifications
        check_format(u'%s',
                     b'%1%s', b'abc')
        check_format(u'%1abc',
                     b'%1abc')
        check_format(u'%+i',
                     b'%+i', c_int(10))
        check_format(u'%s',
                     b'%.%s', b'abc')

    @test_support.cpython_only
    def test_encode_decimal(self):
        from _testcapi import unicode_encodedecimal
        self.assertEqual(unicode_encodedecimal(u'123'),
                         b'123')
        self.assertEqual(unicode_encodedecimal(u'\u0663.\u0661\u0664'),
                         b'3.14')
        self.assertEqual(unicode_encodedecimal(u"\N{EM SPACE}3.14\N{EN SPACE}"),
                         b' 3.14 ')
        self.assertRaises(UnicodeEncodeError,
                          unicode_encodedecimal, u"123\u20ac", "strict")
        self.assertEqual(unicode_encodedecimal(u"123\u20ac", "replace"),
                         b'123?')
        self.assertEqual(unicode_encodedecimal(u"123\u20ac", "ignore"),
                         b'123')
        self.assertEqual(unicode_encodedecimal(u"123\u20ac", "xmlcharrefreplace"),
                         b'123&#8364;')
        self.assertEqual(unicode_encodedecimal(u"123\u20ac", "backslashreplace"),
                         b'123\\u20ac')
        self.assertEqual(unicode_encodedecimal(u"123\u20ac\N{EM SPACE}", "replace"),
                         b'123? ')
        self.assertEqual(unicode_encodedecimal(u"123\u20ac\u20ac", "replace"),
                         b'123??')
        self.assertEqual(unicode_encodedecimal(u"123\u20ac\u0660", "replace"),
                         b'123?0')

    @test_support.cpython_only
    def test_encode_decimal_with_surrogates(self):
        from _testcapi import unicode_encodedecimal
        tests = [(u'\U0001f49d', '&#128157;'),
                 (u'\ud83d', '&#55357;'),
                 (u'\udc9d', '&#56477;'),
                ]
        if u'\ud83d\udc9d' != u'\U0001f49d':
            tests += [(u'\ud83d\udc9d', '&#55357;&#56477;')]
        for s, exp in tests:
            self.assertEqual(
                    unicode_encodedecimal(u"123" + s, "xmlcharrefreplace"),
                    '123' + exp)

def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
