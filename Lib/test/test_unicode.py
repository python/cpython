# -*- coding: iso-8859-1 -*-
""" Test script for the Unicode implementation.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
import unittest, test.test_support
import sys, string, codecs

class UnicodeTest(unittest.TestCase):

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

    def checkmethod(self, method, input, output, *args):
        f = getattr(input, method)
        value = f(*args)
        self.assertEqual(output, value)
        self.assert_(type(output) is type(value))

        # if the original is returned make sure that
        # this doesn't happen with subclasses
        if value is input:
            class usub(unicode):
                def __repr__(self):
                    return 'usub(%r)' % unicode.__repr__(self)
            input = usub(input)
            f = getattr(input, method)
            value = f(*args)
            self.assertEqual(output, value)
            self.assert_(input is not value)

    def test_capitalize(self):
        self.checkmethod('capitalize', u' hello ', u' hello ')
        self.checkmethod('capitalize', u'Hello ', u'Hello ')
        self.checkmethod('capitalize', u'hello ', u'Hello ')
        self.checkmethod('capitalize', u'aaaa', u'Aaaa')
        self.checkmethod('capitalize', u'AaAa', u'Aaaa')

        self.assertRaises(TypeError, u'hello'.capitalize, 42)

    def test_count(self):
        self.checkmethod('count', u'aaa', 3, u'a')
        self.checkmethod('count', u'aaa', 0, u'b')
        self.checkmethod('count', 'aaa', 3, u'a')
        self.checkmethod('count', 'aaa', 0, u'b')
        self.checkmethod('count', u'aaa', 3, 'a')
        self.checkmethod('count', u'aaa', 0, 'b')

        self.assertRaises(TypeError, u'hello'.count)

    def test_title(self):
        self.checkmethod('title', u' hello ', u' Hello ')
        self.checkmethod('title', u'Hello ', u'Hello ')
        self.checkmethod('title', u'hello ', u'Hello ')
        self.checkmethod('title', u"fOrMaT thIs aS titLe String", u'Format This As Title String')
        self.checkmethod('title', u"fOrMaT,thIs-aS*titLe;String", u'Format,This-As*Title;String')
        self.checkmethod('title', u"getInt", u'Getint')

        self.assertRaises(TypeError, u'hello'.title, 42)

    def test_find(self):
        self.checkmethod('find', u'abcdefghiabc', 0, u'abc')
        self.checkmethod('find', u'abcdefghiabc', 9, u'abc', 1)
        self.checkmethod('find', u'abcdefghiabc', -1, u'def', 4)

        self.assertRaises(TypeError, u'hello'.find)
        self.assertRaises(TypeError, u'hello'.find, 42)

    def test_rfind(self):
        self.checkmethod('rfind', u'abcdefghiabc', 9, u'abc')
        self.checkmethod('rfind', 'abcdefghiabc', 9, u'abc')
        self.checkmethod('rfind', 'abcdefghiabc', 12, u'')
        self.checkmethod('rfind', u'abcdefghiabc', 12, '')
        self.checkmethod('rfind', u'abcdefghiabc', 12, u'')

        self.assertRaises(TypeError, u'hello'.rfind)
        self.assertRaises(TypeError, u'hello'.rfind, 42)

    def test_index(self):
        self.checkmethod('index', u'abcdefghiabc', 0, u'')
        self.checkmethod('index', u'abcdefghiabc', 3, u'def')
        self.checkmethod('index', u'abcdefghiabc', 0, u'abc')
        self.checkmethod('index', u'abcdefghiabc', 9, u'abc', 1)

        self.assertRaises(ValueError, u'abcdefghiabc'.index, u'hib')
        self.assertRaises(ValueError, u'abcdefghiab'.index, u'abc', 1)
        self.assertRaises(ValueError, u'abcdefghi'.index, u'ghi', 8)
        self.assertRaises(ValueError, u'abcdefghi'.index, u'ghi', -1)

        self.assertRaises(TypeError, u'hello'.index)
        self.assertRaises(TypeError, u'hello'.index, 42)

    def test_rindex(self):
        self.checkmethod('rindex', u'abcdefghiabc', 12, u'')
        self.checkmethod('rindex', u'abcdefghiabc', 3, u'def')
        self.checkmethod('rindex', u'abcdefghiabc', 9, u'abc')
        self.checkmethod('rindex', u'abcdefghiabc', 0, u'abc', 0, -1)

        self.assertRaises(ValueError, u'abcdefghiabc'.rindex, u'hib')
        self.assertRaises(ValueError, u'defghiabc'.rindex, u'def', 1)
        self.assertRaises(ValueError, u'defghiabc'.rindex, u'abc', 0, -1)
        self.assertRaises(ValueError, u'abcdefghi'.rindex, u'ghi', 0, 8)
        self.assertRaises(ValueError, u'abcdefghi'.rindex, u'ghi', 0, -1)

        self.assertRaises(TypeError, u'hello'.rindex)
        self.assertRaises(TypeError, u'hello'.rindex, 42)

    def test_lower(self):
        self.checkmethod('lower', u'HeLLo', u'hello')
        self.checkmethod('lower', u'hello', u'hello')

        self.assertRaises(TypeError, u"hello".lower, 42)

    def test_upper(self):
        self.checkmethod('upper', u'HeLLo', u'HELLO')
        self.checkmethod('upper', u'HELLO', u'HELLO')

        self.assertRaises(TypeError, u'hello'.upper, 42)

    def test_translate(self):
        if 0:
            transtable = '\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`xyzdefghijklmnopqrstuvwxyz{|}~\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377'

            self.checkmethod('maketrans', u'abc', transtable, u'xyz')
            self.checkmethod('maketrans', u'abc', ValueError, u'xyzq')

            self.checkmethod('translate', u'xyzabcdef', u'xyzxyz', transtable, u'def')

            table = string.maketrans('a', u'A')
            self.checkmethod('translate', u'abc', u'Abc', table)
            self.checkmethod('translate', u'xyz', u'xyz', table)

        self.checkmethod('translate', u"abababc", u'bbbc', {ord('a'):None})
        self.checkmethod('translate', u"abababc", u'iiic', {ord('a'):None, ord('b'):ord('i')})
        self.checkmethod('translate', u"abababc", u'iiix', {ord('a'):None, ord('b'):ord('i'), ord('c'):u'x'})
        self.checkmethod('translate', u"abababc", u'<i><i><i>c', {ord('a'):None, ord('b'):u'<i>'})
        self.checkmethod('translate', u"abababc", u'c', {ord('a'):None, ord('b'):u''})

        self.assertRaises(TypeError, u'hello'.translate)

    def test_split(self):
        self.checkmethod(
            'split',
            u'this is the split function',
            [u'this', u'is', u'the', u'split', u'function']
        )
        self.checkmethod('split', u'a|b|c|d', [u'a', u'b', u'c', u'd'], u'|')
        self.checkmethod('split', u'a|b|c|d', [u'a', u'b', u'c|d'], u'|', 2)
        self.checkmethod('split', u'a b c d', [u'a', u'b c d'], None, 1)
        self.checkmethod('split', u'a b c d', [u'a', u'b', u'c d'], None, 2)
        self.checkmethod('split', u'a b c d', [u'a', u'b', u'c', u'd'], None, 3)
        self.checkmethod('split', u'a b c d', [u'a', u'b', u'c', u'd'], None, 4)
        self.checkmethod('split', u'a b c d', [u'a b c d'], None, 0)
        self.checkmethod('split', u'a  b  c  d', [u'a', u'b', u'c  d'], None, 2)
        self.checkmethod('split', u'a b c d ', [u'a', u'b', u'c', u'd'])
        self.checkmethod('split', u'a//b//c//d', [u'a', u'b', u'c', u'd'], u'//')
        self.checkmethod('split', u'a//b//c//d', [u'a', u'b', u'c', u'd'], '//')
        self.checkmethod('split', 'a//b//c//d', [u'a', u'b', u'c', u'd'], u'//')
        self.checkmethod('split', u'endcase test', [u'endcase ', u''], u'test')
        self.checkmethod('split', u'endcase test', [u'endcase ', u''], 'test')
        self.checkmethod('split', 'endcase test', [u'endcase ', u''], u'test')

        self.assertRaises(TypeError, u"hello".split, 42, 42, 42)

    def test_join(self):
        # join now works with any sequence type
        class Sequence:
            def __init__(self, seq): self.seq = seq
            def __len__(self): return len(self.seq)
            def __getitem__(self, i): return self.seq[i]

        self.checkmethod('join', u' ', u'a b c d', [u'a', u'b', u'c', u'd'])
        self.checkmethod('join', u' ', u'a b c d', ['a', 'b', u'c', u'd'])
        self.checkmethod('join', u'', u'abcd', (u'a', u'b', u'c', u'd'))
        self.checkmethod('join', u' ', u'w x y z', Sequence('wxyz'))
        self.assertRaises(TypeError, u' '.join, 7)
        self.assertRaises(TypeError, u' '.join, Sequence([7, u'hello', 123L]))
        self.checkmethod('join', ' ', u'a b c d', [u'a', u'b', u'c', u'd'])
        self.checkmethod('join', ' ', u'a b c d', ['a', 'b', u'c', u'd'])
        self.checkmethod('join', '', u'abcd', (u'a', u'b', u'c', u'd'))
        self.checkmethod('join', ' ', u'w x y z', Sequence(u'wxyz'))
        self.assertRaises(TypeError, ' '.join, TypeError)

        result = u''
        for i in range(10):
            if i > 0:
                result = result + u':'
            result = result + u'x'*10

        self.checkmethod('join', u':', result, [u'x' * 10] * 10)
        self.checkmethod('join', u':', result, (u'x' * 10,) * 10)

        self.assertRaises(TypeError, u"hello".join)

    def test_strip(self):
        self.checkmethod('strip', u'   hello   ', u'hello')
        self.checkmethod('lstrip', u'   hello   ', u'hello   ')
        self.checkmethod('rstrip', u'   hello   ', u'   hello')
        self.checkmethod('strip', u'hello', u'hello')

        # strip/lstrip/rstrip with None arg
        self.checkmethod('strip', u'   hello   ', u'hello', None)
        self.checkmethod('lstrip', u'   hello   ', u'hello   ', None)
        self.checkmethod('rstrip', u'   hello   ', u'   hello', None)
        self.checkmethod('strip', u'hello', u'hello', None)

        # strip/lstrip/rstrip with unicode arg
        self.checkmethod('strip', u'xyzzyhelloxyzzy', u'hello', u'xyz')
        self.checkmethod('lstrip', u'xyzzyhelloxyzzy', u'helloxyzzy', u'xyz')
        self.checkmethod('rstrip', u'xyzzyhelloxyzzy', u'xyzzyhello', u'xyz')
        self.checkmethod('strip', u'hello', u'hello', u'xyz')

        # strip/lstrip/rstrip with str arg
        self.checkmethod('strip', u'xyzzyhelloxyzzy', u'hello', 'xyz')
        self.checkmethod('lstrip', u'xyzzyhelloxyzzy', u'helloxyzzy', 'xyz')
        self.checkmethod('rstrip', u'xyzzyhelloxyzzy', u'xyzzyhello', 'xyz')
        self.checkmethod('strip', u'hello', u'hello', 'xyz')

        self.assertRaises(TypeError, u"hello".strip, 42, 42)
        self.assertRaises(UnicodeError, u"hello".strip, "\xff")

    def test_swapcase(self):
        self.checkmethod('swapcase', u'HeLLo cOmpUteRs', u'hEllO CoMPuTErS')

        self.assertRaises(TypeError, u"hello".swapcase, 42)

    def test_replace(self):
        self.checkmethod('replace', u'one!two!three!', u'one@two!three!', u'!', u'@', 1)
        self.checkmethod('replace', u'one!two!three!', u'onetwothree', '!', '')
        self.checkmethod('replace', u'one!two!three!', u'one@two@three!', u'!', u'@', 2)
        self.checkmethod('replace', u'one!two!three!', u'one@two@three@', u'!', u'@', 3)
        self.checkmethod('replace', u'one!two!three!', u'one@two@three@', u'!', u'@', 4)
        self.checkmethod('replace', u'one!two!three!', u'one!two!three!', u'!', u'@', 0)
        self.checkmethod('replace', u'one!two!three!', u'one@two@three@', u'!', u'@')
        self.checkmethod('replace', u'one!two!three!', u'one!two!three!', u'x', u'@')
        self.checkmethod('replace', u'one!two!three!', u'one!two!three!', u'x', u'@', 2)
        self.checkmethod('replace', u'abc', u'-a-b-c-', u'', u'-')
        self.checkmethod('replace', u'abc', u'-a-b-c', u'', u'-', 3)
        self.checkmethod('replace', u'abc', u'abc', u'', u'-', 0)
        self.checkmethod('replace', u'abc', u'abc', u'ab', u'--', 0)
        self.checkmethod('replace', u'abc', u'abc', u'xy', u'--')
        self.checkmethod('replace', u'', u'', u'', u'')

        # method call forwarded from str implementation because of unicode argument
        self.checkmethod('replace', 'one!two!three!', u'one@two!three!', u'!', u'@', 1)
        self.assertRaises(TypeError, 'replace'.replace, 42)
        self.assertRaises(TypeError, 'replace'.replace, u"r", 42)

        self.assertRaises(TypeError, u"hello".replace)
        self.assertRaises(TypeError, u"hello".replace, 42, u"h")
        self.assertRaises(TypeError, u"hello".replace, u"h", 42)

    def test_startswith(self):
        self.checkmethod('startswith', u'hello', True, u'he')
        self.checkmethod('startswith', u'hello', True, u'hello')
        self.checkmethod('startswith', u'hello', False, u'hello world')
        self.checkmethod('startswith', u'hello', True, u'')
        self.checkmethod('startswith', u'hello', False, u'ello')
        self.checkmethod('startswith', u'hello', True, u'ello', 1)
        self.checkmethod('startswith', u'hello', True, u'o', 4)
        self.checkmethod('startswith', u'hello', False, u'o', 5)
        self.checkmethod('startswith', u'hello', True, u'', 5)
        self.checkmethod('startswith', u'hello', False, u'lo', 6)
        self.checkmethod('startswith', u'helloworld', True, u'lowo', 3)
        self.checkmethod('startswith', u'helloworld', True, u'lowo', 3, 7)
        self.checkmethod('startswith', u'helloworld', False, u'lowo', 3, 6)

        self.assertRaises(TypeError, u"hello".startswith)
        self.assertRaises(TypeError, u"hello".startswith, 42)

    def test_endswith(self):
        self.checkmethod('endswith', u'hello', True, u'lo')
        self.checkmethod('endswith', u'hello', False, u'he')
        self.checkmethod('endswith', u'hello', True, u'')
        self.checkmethod('endswith', u'hello', False, u'hello world')
        self.checkmethod('endswith', u'helloworld', False, u'worl')
        self.checkmethod('endswith', u'helloworld', True, u'worl', 3, 9)
        self.checkmethod('endswith', u'helloworld', True, u'world', 3, 12)
        self.checkmethod('endswith', u'helloworld', True, u'lowo', 1, 7)
        self.checkmethod('endswith', u'helloworld', True, u'lowo', 2, 7)
        self.checkmethod('endswith', u'helloworld', True, u'lowo', 3, 7)
        self.checkmethod('endswith', u'helloworld', False, u'lowo', 4, 7)
        self.checkmethod('endswith', u'helloworld', False, u'lowo', 3, 8)
        self.checkmethod('endswith', u'ab', False, u'ab', 0, 1)
        self.checkmethod('endswith', u'ab', False, u'ab', 0, 0)
        self.checkmethod('endswith', 'helloworld', True, u'd')
        self.checkmethod('endswith', 'helloworld', False, u'l')

        self.assertRaises(TypeError, u"hello".endswith)
        self.assertRaises(TypeError, u"hello".endswith, 42)

    def test_expandtabs(self):
        self.checkmethod('expandtabs', u'abc\rab\tdef\ng\thi', u'abc\rab      def\ng       hi')
        self.checkmethod('expandtabs', u'abc\rab\tdef\ng\thi', u'abc\rab      def\ng       hi', 8)
        self.checkmethod('expandtabs', u'abc\rab\tdef\ng\thi', u'abc\rab  def\ng   hi', 4)
        self.checkmethod('expandtabs', u'abc\r\nab\tdef\ng\thi', u'abc\r\nab  def\ng   hi', 4)
        self.checkmethod('expandtabs', u'abc\r\nab\r\ndef\ng\r\nhi', u'abc\r\nab\r\ndef\ng\r\nhi', 4)

        self.assertRaises(TypeError, u"hello".expandtabs, 42, 42)

    def test_capwords(self):
        if 0:
            self.checkmethod('capwords', u'abc def ghi', u'Abc Def Ghi')
            self.checkmethod('capwords', u'abc\tdef\nghi', u'Abc Def Ghi')
            self.checkmethod('capwords', u'abc\t   def  \nghi', u'Abc Def Ghi')

    def test_zfill(self):
        self.checkmethod('zfill', u'123', u'123', 2)
        self.checkmethod('zfill', u'123', u'123', 3)
        self.checkmethod('zfill', u'123', u'0123', 4)
        self.checkmethod('zfill', u'+123', u'+123', 3)
        self.checkmethod('zfill', u'+123', u'+123', 4)
        self.checkmethod('zfill', u'+123', u'+0123', 5)
        self.checkmethod('zfill', u'-123', u'-123', 3)
        self.checkmethod('zfill', u'-123', u'-123', 4)
        self.checkmethod('zfill', u'-123', u'-0123', 5)
        self.checkmethod('zfill', u'', u'000', 3)
        self.checkmethod('zfill', u'34', u'34', 1)
        self.checkmethod('zfill', u'34', u'00034', 5)

        self.assertRaises(TypeError, u"123".zfill)

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

    def test_ljust(self):
        self.checkmethod('ljust', u'abc',  u'abc       ', 10)
        self.checkmethod('ljust', u'abc',  u'abc   ', 6)
        self.checkmethod('ljust', u'abc', u'abc', 2)

        self.assertRaises(TypeError, u"abc".ljust)

    def test_rjust(self):
        self.checkmethod('rjust', u'abc',  u'       abc', 10)
        self.checkmethod('rjust', u'abc',  u'   abc', 6)
        self.checkmethod('rjust', u'abc', u'abc', 2)

        self.assertRaises(TypeError, u"abc".rjust)

    def test_center(self):
        self.checkmethod('center', u'abc', u'   abc    ', 10)
        self.checkmethod('center', u'abc', u' abc  ', 6)
        self.checkmethod('center', u'abc', u'abc', 2)

        self.assertRaises(TypeError, u"abc".center)

    def test_islower(self):
        self.checkmethod('islower', u'', False)
        self.checkmethod('islower', u'a', True)
        self.checkmethod('islower', u'A', False)
        self.checkmethod('islower', u'\n', False)
        self.checkmethod('islower', u'\u1FFc', False)
        self.checkmethod('islower', u'abc', True)
        self.checkmethod('islower', u'aBc', False)
        self.checkmethod('islower', u'abc\n', True)

        self.assertRaises(TypeError, u"abc".islower, 42)

    def test_isupper(self):
        self.checkmethod('isupper', u'', False)
        self.checkmethod('isupper', u'a', False)
        self.checkmethod('isupper', u'A', True)
        self.checkmethod('isupper', u'\n', False)
        if sys.platform[:4] != 'java':
            self.checkmethod('isupper', u'\u1FFc', False)
        self.checkmethod('isupper', u'ABC', True)
        self.checkmethod('isupper', u'AbC', False)
        self.checkmethod('isupper', u'ABC\n', True)

        self.assertRaises(TypeError, u"abc".isupper, 42)

    def test_istitle(self):
        self.checkmethod('istitle', u'', False)
        self.checkmethod('istitle', u'a', False)
        self.checkmethod('istitle', u'A', True)
        self.checkmethod('istitle', u'\n', False)
        self.checkmethod('istitle', u'\u1FFc', True)
        self.checkmethod('istitle', u'A Titlecased Line', True)
        self.checkmethod('istitle', u'A\nTitlecased Line', True)
        self.checkmethod('istitle', u'A Titlecased, Line', True)
        self.checkmethod('istitle', u'Greek \u1FFcitlecases ...', True)
        self.checkmethod('istitle', u'Not a capitalized String', False)
        self.checkmethod('istitle', u'Not\ta Titlecase String', False)
        self.checkmethod('istitle', u'Not--a Titlecase String', False)
        self.checkmethod('istitle', u'NOT', False)

        self.assertRaises(TypeError, u"abc".istitle, 42)

    def test_isspace(self):
        self.checkmethod('isspace', u'', False)
        self.checkmethod('isspace', u'a', False)
        self.checkmethod('isspace', u' ', True)
        self.checkmethod('isspace', u'\t', True)
        self.checkmethod('isspace', u'\r', True)
        self.checkmethod('isspace', u'\n', True)
        self.checkmethod('isspace', u' \t\r\n', True)
        self.checkmethod('isspace', u' \t\r\na', False)

        self.assertRaises(TypeError, u"abc".isspace, 42)

    def test_isalpha(self):
        self.checkmethod('isalpha', u'', False)
        self.checkmethod('isalpha', u'a', True)
        self.checkmethod('isalpha', u'A', True)
        self.checkmethod('isalpha', u'\n', False)
        self.checkmethod('isalpha', u'\u1FFc', True)
        self.checkmethod('isalpha', u'abc', True)
        self.checkmethod('isalpha', u'aBc123', False)
        self.checkmethod('isalpha', u'abc\n', False)

        self.assertRaises(TypeError, u"abc".isalpha, 42)

    def test_isalnum(self):
        self.checkmethod('isalnum', u'', False)
        self.checkmethod('isalnum', u'a', True)
        self.checkmethod('isalnum', u'A', True)
        self.checkmethod('isalnum', u'\n', False)
        self.checkmethod('isalnum', u'123abc456', True)
        self.checkmethod('isalnum', u'a1b3c', True)
        self.checkmethod('isalnum', u'aBc000 ', False)
        self.checkmethod('isalnum', u'abc\n', False)

        self.assertRaises(TypeError, u"abc".isalnum, 42)

    def test_isdecimal(self):
        self.checkmethod('isdecimal', u'', False)
        self.checkmethod('isdecimal', u'a', False)
        self.checkmethod('isdecimal', u'0', True)
        self.checkmethod('isdecimal', u'\u2460', False) # CIRCLED DIGIT ONE
        self.checkmethod('isdecimal', u'\xbc', False) # VULGAR FRACTION ONE QUARTER
        self.checkmethod('isdecimal', u'\u0660', True) # ARABIC-INDIC DIGIT ZERO
        self.checkmethod('isdecimal', u'0123456789', True)
        self.checkmethod('isdecimal', u'0123456789a', False)

        self.assertRaises(TypeError, u"abc".isdecimal, 42)

    def test_isdigit(self):
        self.checkmethod('isdigit', u'', False)
        self.checkmethod('isdigit', u'a', False)
        self.checkmethod('isdigit', u'0', True)
        self.checkmethod('isdigit', u'\u2460', True)
        self.checkmethod('isdigit', u'\xbc', False)
        self.checkmethod('isdigit', u'\u0660', True)
        self.checkmethod('isdigit', u'0123456789', True)
        self.checkmethod('isdigit', u'0123456789a', False)

        self.assertRaises(TypeError, u"abc".isdigit, 42)

    def test_isnumeric(self):
        self.checkmethod('isnumeric', u'', False)
        self.checkmethod('isnumeric', u'a', False)
        self.checkmethod('isnumeric', u'0', True)
        self.checkmethod('isnumeric', u'\u2460', True)
        self.checkmethod('isnumeric', u'\xbc', True)
        self.checkmethod('isnumeric', u'\u0660', True)
        self.checkmethod('isnumeric', u'0123456789', True)
        self.checkmethod('isnumeric', u'0123456789a', False)

        self.assertRaises(TypeError, u"abc".isnumeric, 42)

    def test_splitlines(self):
        self.checkmethod('splitlines', u"abc\ndef\n\rghi", [u'abc', u'def', u'', u'ghi'])
        self.checkmethod('splitlines', u"abc\ndef\n\r\nghi", [u'abc', u'def', u'', u'ghi'])
        self.checkmethod('splitlines', u"abc\ndef\r\nghi", [u'abc', u'def', u'ghi'])
        self.checkmethod('splitlines', u"abc\ndef\r\nghi\n", [u'abc', u'def', u'ghi'])
        self.checkmethod('splitlines', u"abc\ndef\r\nghi\n\r", [u'abc', u'def', u'ghi', u''])
        self.checkmethod('splitlines', u"\nabc\ndef\r\nghi\n\r", [u'', u'abc', u'def', u'ghi', u''])
        self.checkmethod('splitlines', u"\nabc\ndef\r\nghi\n\r", [u'\n', u'abc\n', u'def\r\n', u'ghi\n', u'\r'], True)

        self.assertRaises(TypeError, u"abc".splitlines, 42, 42)

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
        # Testing Unicode formatting strings...
        self.assertEqual(u"%s, %s" % (u"abc", "abc"), u'abc, abc')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", 1, 2, 3), u'abc, abc, 1, 2.000000,  3.00')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", 1, -2, 3), u'abc, abc, 1, -2.000000,  3.00')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", -1, -2, 3.5), u'abc, abc, -1, -2.000000,  3.50')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", -1, -2, 3.57), u'abc, abc, -1, -2.000000,  3.57')
        self.assertEqual(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", -1, -2, 1003.57), u'abc, abc, -1, -2.000000, 1003.57')
        self.assertEqual(u"%c" % (u"a",), u'a')
        self.assertEqual(u"%c" % ("a",), u'a')
        self.assertEqual(u"%c" % (34,), u'"')
        self.assertEqual(u"%c" % (36,), u'$')
        self.assertEqual(u"%d".__mod__(10), u'10')
        if not sys.platform.startswith('java'):
            self.assertEqual(u"%r, %r" % (u"abc", "abc"), u"u'abc', 'abc'")
        self.assertEqual(u"%(x)s, %(y)s" % {'x':u"abc", 'y':"def"}, u'abc, def')
        self.assertEqual(u"%(x)s, %(ä)s" % {'x':u"abc", u'ä':"def"}, u'abc, def')

        for ordinal in (-100, 0x200000):
            self.assertRaises(ValueError, u"%c".__mod__, ordinal)

        # float formatting
        for prec in xrange(100):
            format = u'%%.%if' % prec
            value = 0.01
            for x in xrange(60):
                value = value * 3.141592655 / 3.0 * 10.0
                # The formatfloat() code in stringobject.c and
                # unicodeobject.c uses a 120 byte buffer and switches from
                # 'f' formatting to 'g' at precision 50, so we expect
                # OverflowErrors for the ranges x < 50 and prec >= 67.
                if x < 50 and prec >= 67:
                    self.assertRaises(OverflowError, format.__mod__, value)
                else:
                    format % value

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
        self.assertEqual('%i%s %*.*s' % (10, 3, 5,3,u'abc',), u'103   abc')

        self.assertEqual(u'%3ld' % 42, u' 42')
        self.assertEqual(u'%07.2f' % 42, u'0042.00')

        self.assertRaises(TypeError, u"abc".__mod__)
        self.assertRaises(TypeError, u"%(foo)s".__mod__, 42)
        self.assertRaises(TypeError, u"%s%s".__mod__, (42,))
        self.assertRaises(TypeError, u"%c".__mod__, (None,))
        self.assertRaises(ValueError, u"%c".__mod__, (sys.maxunicode+1,))
        self.assertRaises(ValueError, u"%(foo".__mod__, {})
        self.assertRaises(TypeError, u"%(foo)s %(bar)s".__mod__, (u"foo", 42))

        # argument names with properly nested brackets are supported
        self.assertEqual(u"%((foo))s" % {u"(foo)": u"bar"}, u"bar")

        # 100 is a magic number in PyUnicode_Format, this forces a resize
        self.assertEqual(u"%sx" % (103*u"a"), 103*u"a"+u"x")

        self.assertRaises(TypeError, u"%*s".__mod__, (u"foo", u"bar"))
        self.assertRaises(TypeError, u"%10.*f".__mod__, (u"foo", 42.))
        self.assertRaises(ValueError, u"%10".__mod__, (42,))

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
        u = u''.join(map(unichr, xrange(1024)))
        for encoding in ('utf-7', 'utf-8', 'utf-16', 'utf-16-le', 'utf-16-be',
                         'raw_unicode_escape', 'unicode_escape', 'unicode_internal'):
            self.assertEqual(unicode(u.encode(encoding),encoding), u)

        # Roundtrip safety for BMP (just the first 256 chars)
        u = u''.join(map(unichr, xrange(256)))
        for encoding in ('latin-1',):
            self.assertEqual(unicode(u.encode(encoding),encoding), u)

        # Roundtrip safety for BMP (just the first 128 chars)
        u = u''.join(map(unichr, xrange(128)))
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

    def test_mul(self):
        self.checkmethod('__mul__', u'abc', u'', -1)
        self.checkmethod('__mul__', u'abc', u'', 0)
        self.checkmethod('__mul__', u'abc', u'abc', 1)
        self.checkmethod('__mul__', u'abc', u'abcabcabc', 3)
        self.assertRaises(OverflowError, (10000*u'abc').__mul__, sys.maxint)

    def test_subscript(self):
        self.checkmethod('__getitem__', u'abc', u'a', 0)
        self.checkmethod('__getitem__', u'abc', u'c', -1)
        self.checkmethod('__getitem__', u'abc', u'a', 0L)
        self.checkmethod('__getitem__', u'abc', u'abc', slice(0, 3))
        self.checkmethod('__getitem__', u'abc', u'abc', slice(0, 1000))
        self.checkmethod('__getitem__', u'abc', u'a', slice(0, 1))
        self.checkmethod('__getitem__', u'abc', u'', slice(0, 0))
        # FIXME What about negative indizes? This is handled differently by [] and __getitem__(slice)

        self.assertRaises(TypeError, u"abc".__getitem__, "def")

    def test_slice(self):
        self.checkmethod('__getslice__', u'abc', u'abc', 0, 1000)
        self.checkmethod('__getslice__', u'abc', u'abc', 0, 3)
        self.checkmethod('__getslice__', u'abc', u'ab', 0, 2)
        self.checkmethod('__getslice__', u'abc', u'bc', 1, 3)
        self.checkmethod('__getslice__', u'abc', u'b', 1, 2)
        self.checkmethod('__getslice__', u'abc', u'', 2, 2)
        self.checkmethod('__getslice__', u'abc', u'', 1000, 1000)
        self.checkmethod('__getslice__', u'abc', u'', 2000, 1000)
        self.checkmethod('__getslice__', u'abc', u'', 2, 1)
        # FIXME What about negative indizes? This is handled differently by [] and __getslice__

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(UnicodeTest))
    test.test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
