# -*- coding: iso-8859-1 -*-
""" Test script for the Unicode implementation.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
import sys, struct, codecs
from test import test_support, string_tests

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

    def checkequalnofix(self, result, object, methodname, *args):
        method = getattr(object, methodname)
        realresult = method(*args)
        self.assertEqual(realresult, result)
        self.assert_(type(realresult) is type(result))

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
            self.assert_(object is not realresult)

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
        self.assertEqual(u'abc', 'abc')
        self.assertEqual('abc', u'abc')
        self.assertEqual(u'abc', u'abc')
        self.assert_(u'abcd' > 'abc')
        self.assert_('abcd' > u'abc')
        self.assert_(u'abcd' > u'abc')
        self.assert_(u'abc' < 'abcd')
        self.assert_('abc' < u'abcd')
        self.assert_(u'abc' < u'abcd')

        if 0:
            # Move these tests to a Unicode collation module test...
            # Testing UTF-16 code point order comparisons...

            # No surrogates, no fixup required.
            self.assert_(u'\u0061' < u'\u20ac')
            # Non surrogate below surrogate value, no fixup required
            self.assert_(u'\u0061' < u'\ud800\udc02')

            # Non surrogate above surrogate value, fixup required
            def test_lecmp(s, s2):
                self.assert_(s < s2)

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
        self.assert_(u'\ud800\udc02' < u'\ud84d\udc56')

    def test_islower(self):
        string_tests.MixinStrUnicodeUserStringTest.test_islower(self)
        self.checkequalnofix(False, u'\u1FFc', 'islower')

    def test_isupper(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isupper(self)
        if not sys.platform.startswith('java'):
            self.checkequalnofix(False, u'\u1FFc', 'isupper')

    def test_istitle(self):
        string_tests.MixinStrUnicodeUserStringTest.test_title(self)
        self.checkequalnofix(True, u'\u1FFc', 'istitle')
        self.checkequalnofix(True, u'Greek \u1FFcitlecases ...', 'istitle')

    def test_isspace(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isspace(self)
        self.checkequalnofix(True, u'\u2000', 'isspace')
        self.checkequalnofix(True, u'\u200a', 'isspace')
        self.checkequalnofix(False, u'\u2014', 'isspace')

    def test_isalpha(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isalpha(self)
        self.checkequalnofix(True, u'\u1FFc', 'isalpha')

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

    def test_isdigit(self):
        string_tests.MixinStrUnicodeUserStringTest.test_isdigit(self)
        self.checkequalnofix(True, u'\u2460', 'isdigit')
        self.checkequalnofix(False, u'\xbc', 'isdigit')
        self.checkequalnofix(True, u'\u0660', 'isdigit')

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

    def test_contains(self):
        # Testing Unicode contains method
        self.assert_('a' in u'abdb')
        self.assert_('a' in u'bdab')
        self.assert_('a' in u'bdaba')
        self.assert_('a' in u'bdba')
        self.assert_('a' in u'bdba')
        self.assert_(u'a' in u'bdba')
        self.assert_(u'a' not in u'bdb')
        self.assert_(u'a' not in 'bdb')
        self.assert_(u'a' in 'bdba')
        self.assert_(u'a' in ('a',1,None))
        self.assert_(u'a' in (1,None,'a'))
        self.assert_(u'a' in (1,None,u'a'))
        self.assert_('a' in ('a',1,None))
        self.assert_('a' in (1,None,'a'))
        self.assert_('a' in (1,None,u'a'))
        self.assert_('a' not in ('x',1,u'y'))
        self.assert_('a' not in ('x',1,None))
        self.assert_(u'abcd' not in u'abcxxxx')
        self.assert_(u'ab' in u'abcd')
        self.assert_('ab' in u'abc')
        self.assert_(u'ab' in 'abc')
        self.assert_(u'ab' in (1,None,u'ab'))
        self.assert_(u'' in u'abc')
        self.assert_('' in u'abc')

        # If the following fails either
        # the contains operator does not propagate UnicodeErrors or
        # someone has changed the default encoding
        self.assertRaises(UnicodeError, 'g\xe2teau'.__contains__, u'\xe2')

        self.assert_(u'' in '')
        self.assert_('' in u'')
        self.assert_(u'' in u'')
        self.assert_(u'' in 'abc')
        self.assert_('' in u'abc')
        self.assert_(u'' in u'abc')
        self.assert_(u'\0' not in 'abc')
        self.assert_('\0' not in u'abc')
        self.assert_(u'\0' not in u'abc')
        self.assert_(u'\0' in '\0abc')
        self.assert_('\0' in u'\0abc')
        self.assert_(u'\0' in u'\0abc')
        self.assert_(u'\0' in 'abc\0')
        self.assert_('\0' in u'abc\0')
        self.assert_(u'\0' in u'abc\0')
        self.assert_(u'a' in '\0abc')
        self.assert_('a' in u'\0abc')
        self.assert_(u'a' in u'\0abc')
        self.assert_(u'asdf' in 'asdf')
        self.assert_('asdf' in u'asdf')
        self.assert_(u'asdf' in u'asdf')
        self.assert_(u'asdf' not in 'asd')
        self.assert_('asdf' not in u'asd')
        self.assert_(u'asdf' not in u'asd')
        self.assert_(u'asdf' not in '')
        self.assert_('asdf' not in u'')
        self.assert_(u'asdf' not in u'')

        self.assertRaises(TypeError, u"abc".__contains__)

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
            self.assertEqual(
                unicode(
                    buffer('character buffers are decoded to unicode'),
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
            (ur'++--', '+-+---')
        ]

        for (x, y) in utfTests:
            self.assertEqual(x.encode('utf-7'), y)

        # surrogates not supported
        self.assertRaises(UnicodeError, unicode, '+3ADYAA-', 'utf-7')

        self.assertEqual(unicode('+3ADYAA-', 'utf-7', 'replace'), u'\ufffd')

        # Issue #2242: crash on some Windows/MSVC versions
        self.assertRaises(UnicodeDecodeError, '+\xc1'.decode, 'utf-7')

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
        self.assertEqual(unicode('\xf0\xa3\x91\x96', 'utf-8'), u'\U00023456' )
        self.assertEqual(unicode('\xf0\x90\x80\x82', 'utf-8'), u'\U00010002' )
        self.assertEqual(unicode('\xe2\x82\xac', 'utf-8'), u'\u20ac' )

        # Other possible utf-8 test cases:
        # * strict decoding testing for all of the
        #   UTF8_ERROR cases in PyUnicode_DecodeUTF8

    def test_codecs_idna(self):
        # Test whether trailing dot is preserved
        self.assertEqual(u"www.python.org.".encode("idna"), "www.python.org.")

    def test_codecs_errors(self):
        # Error handling (encoding)
        self.assertRaises(UnicodeError, u'Andr\202 x'.encode, 'ascii')
        self.assertRaises(UnicodeError, u'Andr\202 x'.encode, 'ascii','strict')
        self.assertEqual(u'Andr\202 x'.encode('ascii','ignore'), "Andr x")
        self.assertEqual(u'Andr\202 x'.encode('ascii','replace'), "Andr? x")

        # Error handling (decoding)
        self.assertRaises(UnicodeError, unicode, 'Andr\202 x', 'ascii')
        self.assertRaises(UnicodeError, unicode, 'Andr\202 x', 'ascii','strict')
        self.assertEqual(unicode('Andr\202 x','ascii','ignore'), u"Andr x")
        self.assertEqual(unicode('Andr\202 x','ascii','replace'), u'Andr\uFFFD x')

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
            self.assertEqual(unicode(s, encoding).encode(encoding), s)

        # 128-255
        s = ''.join(map(chr, xrange(128, 256)))
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

    def test_expandtabs_overflows_gracefully(self):
        # This test only affects 32-bit platforms because expandtabs can only take
        # an int as the max value, not a 64-bit C long.  If expandtabs is changed
        # to take a 64-bit long, this test should apply to all platforms.
        if sys.maxint > (1 << 32) or struct.calcsize('P') != 4:
            return
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

        # format specifiers for user defined type
        self.assertEqual(u'{0:abc}'.format(C()), u'abc')

        # !r and !s coersions
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
        self.assertEqual(u'{0:^10}'.format(E(u'data')), u' E(data)  ')
        self.assertEqual(u'{0:^10s}'.format(E(u'data')), u' E(data)  ')
        self.assertEqual(u'{0:d}'.format(G(u'data')), u'G(data)')
        self.assertEqual(u'{0:>15s}'.format(G(u'data')), u' string is data')
        self.assertEqual(u'{0!s}'.format(G(u'data')), u'string is data')

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
        self.assertRaises(ValueError, u"{:}".format)
        self.assertRaises(ValueError, u"{:s}".format)
        self.assertRaises(ValueError, u"{}".format)

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

def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
