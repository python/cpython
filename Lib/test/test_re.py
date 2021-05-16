from test.support import (gc_collect, bigmemtest, _2G,
                          cpython_only, captured_stdout)
import locale
import re
import sre_compile
import string
import unittest
import warnings
from re import Scanner
from weakref import proxy

# Misc tests from Tim Peters' re.doc

# WARNING: Don't change details in these tests if you don't know
# what you're doing. Some of these tests were carefully modeled to
# cover most of the code.

class S(str):
    def __getitem__(self, index):
        return S(super().__getitem__(index))

class B(bytes):
    def __getitem__(self, index):
        return B(super().__getitem__(index))

class ReTests(unittest.TestCase):

    def assertTypedEqual(self, actual, expect, msg=None):
        self.assertEqual(actual, expect, msg)
        def recurse(actual, expect):
            if isinstance(expect, (tuple, list)):
                for x, y in zip(actual, expect):
                    recurse(x, y)
            else:
                self.assertIs(type(actual), type(expect), msg)
        recurse(actual, expect)

    def checkPatternError(self, pattern, errmsg, pos=None):
        with self.assertRaises(re.error) as cm:
            re.compile(pattern)
        with self.subTest(pattern=pattern):
            err = cm.exception
            self.assertEqual(err.msg, errmsg)
            if pos is not None:
                self.assertEqual(err.pos, pos)

    def checkTemplateError(self, pattern, repl, string, errmsg, pos=None):
        with self.assertRaises(re.error) as cm:
            re.sub(pattern, repl, string)
        with self.subTest(pattern=pattern, repl=repl):
            err = cm.exception
            self.assertEqual(err.msg, errmsg)
            if pos is not None:
                self.assertEqual(err.pos, pos)

    def test_keep_buffer(self):
        # See bug 14212
        b = bytearray(b'x')
        it = re.finditer(b'a', b)
        with self.assertRaises(BufferError):
            b.extend(b'x'*400)
        list(it)
        del it
        gc_collect()
        b.extend(b'x'*400)

    def test_weakref(self):
        s = 'QabbbcR'
        x = re.compile('ab+c')
        y = proxy(x)
        self.assertEqual(x.findall('QabbbcR'), y.findall('QabbbcR'))

    def test_search_star_plus(self):
        self.assertEqual(re.search('x*', 'axx').span(0), (0, 0))
        self.assertEqual(re.search('x*', 'axx').span(), (0, 0))
        self.assertEqual(re.search('x+', 'axx').span(0), (1, 3))
        self.assertEqual(re.search('x+', 'axx').span(), (1, 3))
        self.assertIsNone(re.search('x', 'aaa'))
        self.assertEqual(re.match('a*', 'xxx').span(0), (0, 0))
        self.assertEqual(re.match('a*', 'xxx').span(), (0, 0))
        self.assertEqual(re.match('x*', 'xxxa').span(0), (0, 3))
        self.assertEqual(re.match('x*', 'xxxa').span(), (0, 3))
        self.assertIsNone(re.match('a+', 'xxx'))

    def bump_num(self, matchobj):
        int_value = int(matchobj.group(0))
        return str(int_value + 1)

    def test_basic_re_sub(self):
        self.assertTypedEqual(re.sub('y', 'a', 'xyz'), 'xaz')
        self.assertTypedEqual(re.sub('y', S('a'), S('xyz')), 'xaz')
        self.assertTypedEqual(re.sub(b'y', b'a', b'xyz'), b'xaz')
        self.assertTypedEqual(re.sub(b'y', B(b'a'), B(b'xyz')), b'xaz')
        self.assertTypedEqual(re.sub(b'y', bytearray(b'a'), bytearray(b'xyz')), b'xaz')
        self.assertTypedEqual(re.sub(b'y', memoryview(b'a'), memoryview(b'xyz')), b'xaz')
        for y in ("\xe0", "\u0430", "\U0001d49c"):
            self.assertEqual(re.sub(y, 'a', 'x%sz' % y), 'xaz')

        self.assertEqual(re.sub("(?i)b+", "x", "bbbb BBBB"), 'x x')
        self.assertEqual(re.sub(r'\d+', self.bump_num, '08.2 -2 23x99y'),
                         '9.3 -3 24x100y')
        self.assertEqual(re.sub(r'\d+', self.bump_num, '08.2 -2 23x99y', 3),
                         '9.3 -3 23x99y')
        self.assertEqual(re.sub(r'\d+', self.bump_num, '08.2 -2 23x99y', count=3),
                         '9.3 -3 23x99y')

        self.assertEqual(re.sub('.', lambda m: r"\n", 'x'), '\\n')
        self.assertEqual(re.sub('.', r"\n", 'x'), '\n')

        s = r"\1\1"
        self.assertEqual(re.sub('(.)', s, 'x'), 'xx')
        self.assertEqual(re.sub('(.)', s.replace('\\', r'\\'), 'x'), s)
        self.assertEqual(re.sub('(.)', lambda m: s, 'x'), s)

        self.assertEqual(re.sub('(?P<a>x)', r'\g<a>\g<a>', 'xx'), 'xxxx')
        self.assertEqual(re.sub('(?P<a>x)', r'\g<a>\g<1>', 'xx'), 'xxxx')
        self.assertEqual(re.sub('(?P<unk>x)', r'\g<unk>\g<unk>', 'xx'), 'xxxx')
        self.assertEqual(re.sub('(?P<unk>x)', r'\g<1>\g<1>', 'xx'), 'xxxx')

        self.assertEqual(re.sub('a', r'\t\n\v\r\f\a\b', 'a'), '\t\n\v\r\f\a\b')
        self.assertEqual(re.sub('a', '\t\n\v\r\f\a\b', 'a'), '\t\n\v\r\f\a\b')
        self.assertEqual(re.sub('a', '\t\n\v\r\f\a\b', 'a'),
                         (chr(9)+chr(10)+chr(11)+chr(13)+chr(12)+chr(7)+chr(8)))
        for c in 'cdehijklmopqsuwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ':
            with self.subTest(c):
                with self.assertRaises(re.error):
                    self.assertEqual(re.sub('a', '\\' + c, 'a'), '\\' + c)

        self.assertEqual(re.sub(r'^\s*', 'X', 'test'), 'Xtest')

    def test_bug_449964(self):
        # fails for group followed by other escape
        self.assertEqual(re.sub(r'(?P<unk>x)', r'\g<1>\g<1>\b', 'xx'),
                         'xx\bxx\b')

    def test_bug_449000(self):
        # Test for sub() on escaped characters
        self.assertEqual(re.sub(r'\r\n', r'\n', 'abc\r\ndef\r\n'),
                         'abc\ndef\n')
        self.assertEqual(re.sub('\r\n', r'\n', 'abc\r\ndef\r\n'),
                         'abc\ndef\n')
        self.assertEqual(re.sub(r'\r\n', '\n', 'abc\r\ndef\r\n'),
                         'abc\ndef\n')
        self.assertEqual(re.sub('\r\n', '\n', 'abc\r\ndef\r\n'),
                         'abc\ndef\n')

    def test_bug_1661(self):
        # Verify that flags do not get silently ignored with compiled patterns
        pattern = re.compile('.')
        self.assertRaises(ValueError, re.match, pattern, 'A', re.I)
        self.assertRaises(ValueError, re.search, pattern, 'A', re.I)
        self.assertRaises(ValueError, re.findall, pattern, 'A', re.I)
        self.assertRaises(ValueError, re.compile, pattern, re.I)

    def test_bug_3629(self):
        # A regex that triggered a bug in the sre-code validator
        re.compile("(?P<quote>)(?(quote))")

    def test_sub_template_numeric_escape(self):
        # bug 776311 and friends
        self.assertEqual(re.sub('x', r'\0', 'x'), '\0')
        self.assertEqual(re.sub('x', r'\000', 'x'), '\000')
        self.assertEqual(re.sub('x', r'\001', 'x'), '\001')
        self.assertEqual(re.sub('x', r'\008', 'x'), '\0' + '8')
        self.assertEqual(re.sub('x', r'\009', 'x'), '\0' + '9')
        self.assertEqual(re.sub('x', r'\111', 'x'), '\111')
        self.assertEqual(re.sub('x', r'\117', 'x'), '\117')
        self.assertEqual(re.sub('x', r'\377', 'x'), '\377')

        self.assertEqual(re.sub('x', r'\1111', 'x'), '\1111')
        self.assertEqual(re.sub('x', r'\1111', 'x'), '\111' + '1')

        self.assertEqual(re.sub('x', r'\00', 'x'), '\x00')
        self.assertEqual(re.sub('x', r'\07', 'x'), '\x07')
        self.assertEqual(re.sub('x', r'\08', 'x'), '\0' + '8')
        self.assertEqual(re.sub('x', r'\09', 'x'), '\0' + '9')
        self.assertEqual(re.sub('x', r'\0a', 'x'), '\0' + 'a')

        self.checkTemplateError('x', r'\400', 'x',
                                r'octal escape value \400 outside of '
                                r'range 0-0o377', 0)
        self.checkTemplateError('x', r'\777', 'x',
                                r'octal escape value \777 outside of '
                                r'range 0-0o377', 0)

        self.checkTemplateError('x', r'\1', 'x', 'invalid group reference 1', 1)
        self.checkTemplateError('x', r'\8', 'x', 'invalid group reference 8', 1)
        self.checkTemplateError('x', r'\9', 'x', 'invalid group reference 9', 1)
        self.checkTemplateError('x', r'\11', 'x', 'invalid group reference 11', 1)
        self.checkTemplateError('x', r'\18', 'x', 'invalid group reference 18', 1)
        self.checkTemplateError('x', r'\1a', 'x', 'invalid group reference 1', 1)
        self.checkTemplateError('x', r'\90', 'x', 'invalid group reference 90', 1)
        self.checkTemplateError('x', r'\99', 'x', 'invalid group reference 99', 1)
        self.checkTemplateError('x', r'\118', 'x', 'invalid group reference 11', 1)
        self.checkTemplateError('x', r'\11a', 'x', 'invalid group reference 11', 1)
        self.checkTemplateError('x', r'\181', 'x', 'invalid group reference 18', 1)
        self.checkTemplateError('x', r'\800', 'x', 'invalid group reference 80', 1)
        self.checkTemplateError('x', r'\8', '', 'invalid group reference 8', 1)

        # in python2.3 (etc), these loop endlessly in sre_parser.py
        self.assertEqual(re.sub('(((((((((((x)))))))))))', r'\11', 'x'), 'x')
        self.assertEqual(re.sub('((((((((((y))))))))))(.)', r'\118', 'xyz'),
                         'xz8')
        self.assertEqual(re.sub('((((((((((y))))))))))(.)', r'\11a', 'xyz'),
                         'xza')

    def test_qualified_re_sub(self):
        self.assertEqual(re.sub('a', 'b', 'aaaaa'), 'bbbbb')
        self.assertEqual(re.sub('a', 'b', 'aaaaa', 1), 'baaaa')
        self.assertEqual(re.sub('a', 'b', 'aaaaa', count=1), 'baaaa')

    def test_bug_114660(self):
        self.assertEqual(re.sub(r'(\S)\s+(\S)', r'\1 \2', 'hello  there'),
                         'hello there')

    def test_symbolic_groups(self):
        re.compile(r'(?P<a>x)(?P=a)(?(a)y)')
        re.compile(r'(?P<a1>x)(?P=a1)(?(a1)y)')
        re.compile(r'(?P<a1>x)\1(?(1)y)')
        self.checkPatternError(r'(?P<a>)(?P<a>)',
                               "redefinition of group name 'a' as group 2; "
                               "was group 1")
        self.checkPatternError(r'(?P<a>(?P=a))',
                               "cannot refer to an open group", 10)
        self.checkPatternError(r'(?Pxy)', 'unknown extension ?Px')
        self.checkPatternError(r'(?P<a>)(?P=a', 'missing ), unterminated name', 11)
        self.checkPatternError(r'(?P=', 'missing group name', 4)
        self.checkPatternError(r'(?P=)', 'missing group name', 4)
        self.checkPatternError(r'(?P=1)', "bad character in group name '1'", 4)
        self.checkPatternError(r'(?P=a)', "unknown group name 'a'")
        self.checkPatternError(r'(?P=a1)', "unknown group name 'a1'")
        self.checkPatternError(r'(?P=a.)', "bad character in group name 'a.'", 4)
        self.checkPatternError(r'(?P<)', 'missing >, unterminated name', 4)
        self.checkPatternError(r'(?P<a', 'missing >, unterminated name', 4)
        self.checkPatternError(r'(?P<', 'missing group name', 4)
        self.checkPatternError(r'(?P<>)', 'missing group name', 4)
        self.checkPatternError(r'(?P<1>)', "bad character in group name '1'", 4)
        self.checkPatternError(r'(?P<a.>)', "bad character in group name 'a.'", 4)
        self.checkPatternError(r'(?(', 'missing group name', 3)
        self.checkPatternError(r'(?())', 'missing group name', 3)
        self.checkPatternError(r'(?(a))', "unknown group name 'a'", 3)
        self.checkPatternError(r'(?(-1))', "bad character in group name '-1'", 3)
        self.checkPatternError(r'(?(1a))', "bad character in group name '1a'", 3)
        self.checkPatternError(r'(?(a.))', "bad character in group name 'a.'", 3)
        # New valid/invalid identifiers in Python 3
        re.compile('(?P<µ>x)(?P=µ)(?(µ)y)')
        re.compile('(?P<𝔘𝔫𝔦𝔠𝔬𝔡𝔢>x)(?P=𝔘𝔫𝔦𝔠𝔬𝔡𝔢)(?(𝔘𝔫𝔦𝔠𝔬𝔡𝔢)y)')
        self.checkPatternError('(?P<©>x)', "bad character in group name '©'", 4)
        # Support > 100 groups.
        pat = '|'.join('x(?P<a%d>%x)y' % (i, i) for i in range(1, 200 + 1))
        pat = '(?:%s)(?(200)z|t)' % pat
        self.assertEqual(re.match(pat, 'xc8yz').span(), (0, 5))

    def test_symbolic_refs(self):
        self.checkTemplateError('(?P<a>x)', r'\g<a', 'xx',
                                'missing >, unterminated name', 3)
        self.checkTemplateError('(?P<a>x)', r'\g<', 'xx',
                                'missing group name', 3)
        self.checkTemplateError('(?P<a>x)', r'\g', 'xx', 'missing <', 2)
        self.checkTemplateError('(?P<a>x)', r'\g<a a>', 'xx',
                                "bad character in group name 'a a'", 3)
        self.checkTemplateError('(?P<a>x)', r'\g<>', 'xx',
                                'missing group name', 3)
        self.checkTemplateError('(?P<a>x)', r'\g<1a1>', 'xx',
                                "bad character in group name '1a1'", 3)
        self.checkTemplateError('(?P<a>x)', r'\g<2>', 'xx',
                                'invalid group reference 2', 3)
        self.checkTemplateError('(?P<a>x)', r'\2', 'xx',
                                'invalid group reference 2', 1)
        with self.assertRaisesRegex(IndexError, "unknown group name 'ab'"):
            re.sub('(?P<a>x)', r'\g<ab>', 'xx')
        self.assertEqual(re.sub('(?P<a>x)|(?P<b>y)', r'\g<b>', 'xx'), '')
        self.assertEqual(re.sub('(?P<a>x)|(?P<b>y)', r'\2', 'xx'), '')
        self.checkTemplateError('(?P<a>x)', r'\g<-1>', 'xx',
                                "bad character in group name '-1'", 3)
        # New valid/invalid identifiers in Python 3
        self.assertEqual(re.sub('(?P<µ>x)', r'\g<µ>', 'xx'), 'xx')
        self.assertEqual(re.sub('(?P<𝔘𝔫𝔦𝔠𝔬𝔡𝔢>x)', r'\g<𝔘𝔫𝔦𝔠𝔬𝔡𝔢>', 'xx'), 'xx')
        self.checkTemplateError('(?P<a>x)', r'\g<©>', 'xx',
                                "bad character in group name '©'", 3)
        # Support > 100 groups.
        pat = '|'.join('x(?P<a%d>%x)y' % (i, i) for i in range(1, 200 + 1))
        self.assertEqual(re.sub(pat, r'\g<200>', 'xc8yzxc8y'), 'c8zc8')

    def test_re_subn(self):
        self.assertEqual(re.subn("(?i)b+", "x", "bbbb BBBB"), ('x x', 2))
        self.assertEqual(re.subn("b+", "x", "bbbb BBBB"), ('x BBBB', 1))
        self.assertEqual(re.subn("b+", "x", "xyz"), ('xyz', 0))
        self.assertEqual(re.subn("b*", "x", "xyz"), ('xxxyxzx', 4))
        self.assertEqual(re.subn("b*", "x", "xyz", 2), ('xxxyz', 2))
        self.assertEqual(re.subn("b*", "x", "xyz", count=2), ('xxxyz', 2))

    def test_re_split(self):
        for string in ":a:b::c", S(":a:b::c"):
            self.assertTypedEqual(re.split(":", string),
                                  ['', 'a', 'b', '', 'c'])
            self.assertTypedEqual(re.split(":+", string),
                                  ['', 'a', 'b', 'c'])
            self.assertTypedEqual(re.split("(:+)", string),
                                  ['', ':', 'a', ':', 'b', '::', 'c'])
        for string in (b":a:b::c", B(b":a:b::c"), bytearray(b":a:b::c"),
                       memoryview(b":a:b::c")):
            self.assertTypedEqual(re.split(b":", string),
                                  [b'', b'a', b'b', b'', b'c'])
            self.assertTypedEqual(re.split(b":+", string),
                                  [b'', b'a', b'b', b'c'])
            self.assertTypedEqual(re.split(b"(:+)", string),
                                  [b'', b':', b'a', b':', b'b', b'::', b'c'])
        for a, b, c in ("\xe0\xdf\xe7", "\u0430\u0431\u0432",
                        "\U0001d49c\U0001d49e\U0001d4b5"):
            string = ":%s:%s::%s" % (a, b, c)
            self.assertEqual(re.split(":", string), ['', a, b, '', c])
            self.assertEqual(re.split(":+", string), ['', a, b, c])
            self.assertEqual(re.split("(:+)", string),
                             ['', ':', a, ':', b, '::', c])

        self.assertEqual(re.split("(?::+)", ":a:b::c"), ['', 'a', 'b', 'c'])
        self.assertEqual(re.split("(:)+", ":a:b::c"),
                         ['', ':', 'a', ':', 'b', ':', 'c'])
        self.assertEqual(re.split("([b:]+)", ":a:b::c"),
                         ['', ':', 'a', ':b::', 'c'])
        self.assertEqual(re.split("(b)|(:+)", ":a:b::c"),
                         ['', None, ':', 'a', None, ':', '', 'b', None, '',
                          None, '::', 'c'])
        self.assertEqual(re.split("(?:b)|(?::+)", ":a:b::c"),
                         ['', 'a', '', '', 'c'])

        for sep, expected in [
            (':*', ['', '', 'a', '', 'b', '', 'c', '']),
            ('(?::*)', ['', '', 'a', '', 'b', '', 'c', '']),
            ('(:*)', ['', ':', '', '', 'a', ':', '', '', 'b', '::', '', '', 'c', '', '']),
            ('(:)*', ['', ':', '', None, 'a', ':', '', None, 'b', ':', '', None, 'c', None, '']),
        ]:
            with self.subTest(sep=sep):
                self.assertTypedEqual(re.split(sep, ':a:b::c'), expected)

        for sep, expected in [
            ('', ['', ':', 'a', ':', 'b', ':', ':', 'c', '']),
            (r'\b', [':', 'a', ':', 'b', '::', 'c', '']),
            (r'(?=:)', ['', ':a', ':b', ':', ':c']),
            (r'(?<=:)', [':', 'a:', 'b:', ':', 'c']),
        ]:
            with self.subTest(sep=sep):
                self.assertTypedEqual(re.split(sep, ':a:b::c'), expected)

    def test_qualified_re_split(self):
        self.assertEqual(re.split(":", ":a:b::c", 2), ['', 'a', 'b::c'])
        self.assertEqual(re.split(":", ":a:b::c", maxsplit=2), ['', 'a', 'b::c'])
        self.assertEqual(re.split(':', 'a:b:c:d', maxsplit=2), ['a', 'b', 'c:d'])
        self.assertEqual(re.split("(:)", ":a:b::c", maxsplit=2),
                         ['', ':', 'a', ':', 'b::c'])
        self.assertEqual(re.split("(:+)", ":a:b::c", maxsplit=2),
                         ['', ':', 'a', ':', 'b::c'])
        self.assertEqual(re.split("(:*)", ":a:b::c", maxsplit=2),
                         ['', ':', '', '', 'a:b::c'])

    def test_re_findall(self):
        self.assertEqual(re.findall(":+", "abc"), [])
        for string in "a:b::c:::d", S("a:b::c:::d"):
            self.assertTypedEqual(re.findall(":+", string),
                                  [":", "::", ":::"])
            self.assertTypedEqual(re.findall("(:+)", string),
                                  [":", "::", ":::"])
            self.assertTypedEqual(re.findall("(:)(:*)", string),
                                  [(":", ""), (":", ":"), (":", "::")])
        for string in (b"a:b::c:::d", B(b"a:b::c:::d"), bytearray(b"a:b::c:::d"),
                       memoryview(b"a:b::c:::d")):
            self.assertTypedEqual(re.findall(b":+", string),
                                  [b":", b"::", b":::"])
            self.assertTypedEqual(re.findall(b"(:+)", string),
                                  [b":", b"::", b":::"])
            self.assertTypedEqual(re.findall(b"(:)(:*)", string),
                                  [(b":", b""), (b":", b":"), (b":", b"::")])
        for x in ("\xe0", "\u0430", "\U0001d49c"):
            xx = x * 2
            xxx = x * 3
            string = "a%sb%sc%sd" % (x, xx, xxx)
            self.assertEqual(re.findall("%s+" % x, string), [x, xx, xxx])
            self.assertEqual(re.findall("(%s+)" % x, string), [x, xx, xxx])
            self.assertEqual(re.findall("(%s)(%s*)" % (x, x), string),
                             [(x, ""), (x, x), (x, xx)])

    def test_bug_117612(self):
        self.assertEqual(re.findall(r"(a|(b))", "aba"),
                         [("a", ""),("b", "b"),("a", "")])

    def test_re_match(self):
        for string in 'a', S('a'):
            self.assertEqual(re.match('a', string).groups(), ())
            self.assertEqual(re.match('(a)', string).groups(), ('a',))
            self.assertEqual(re.match('(a)', string).group(0), 'a')
            self.assertEqual(re.match('(a)', string).group(1), 'a')
            self.assertEqual(re.match('(a)', string).group(1, 1), ('a', 'a'))
        for string in b'a', B(b'a'), bytearray(b'a'), memoryview(b'a'):
            self.assertEqual(re.match(b'a', string).groups(), ())
            self.assertEqual(re.match(b'(a)', string).groups(), (b'a',))
            self.assertEqual(re.match(b'(a)', string).group(0), b'a')
            self.assertEqual(re.match(b'(a)', string).group(1), b'a')
            self.assertEqual(re.match(b'(a)', string).group(1, 1), (b'a', b'a'))
        for a in ("\xe0", "\u0430", "\U0001d49c"):
            self.assertEqual(re.match(a, a).groups(), ())
            self.assertEqual(re.match('(%s)' % a, a).groups(), (a,))
            self.assertEqual(re.match('(%s)' % a, a).group(0), a)
            self.assertEqual(re.match('(%s)' % a, a).group(1), a)
            self.assertEqual(re.match('(%s)' % a, a).group(1, 1), (a, a))

        pat = re.compile('((a)|(b))(c)?')
        self.assertEqual(pat.match('a').groups(), ('a', 'a', None, None))
        self.assertEqual(pat.match('b').groups(), ('b', None, 'b', None))
        self.assertEqual(pat.match('ac').groups(), ('a', 'a', None, 'c'))
        self.assertEqual(pat.match('bc').groups(), ('b', None, 'b', 'c'))
        self.assertEqual(pat.match('bc').groups(""), ('b', "", 'b', 'c'))

        pat = re.compile('(?:(?P<a1>a)|(?P<b2>b))(?P<c3>c)?')
        self.assertEqual(pat.match('a').group(1, 2, 3), ('a', None, None))
        self.assertEqual(pat.match('b').group('a1', 'b2', 'c3'),
                         (None, 'b', None))
        self.assertEqual(pat.match('ac').group(1, 'b2', 3), ('a', None, 'c'))

    def test_group(self):
        class Index:
            def __init__(self, value):
                self.value = value
            def __index__(self):
                return self.value
        # A single group
        m = re.match('(a)(b)', 'ab')
        self.assertEqual(m.group(), 'ab')
        self.assertEqual(m.group(0), 'ab')
        self.assertEqual(m.group(1), 'a')
        self.assertEqual(m.group(Index(1)), 'a')
        self.assertRaises(IndexError, m.group, -1)
        self.assertRaises(IndexError, m.group, 3)
        self.assertRaises(IndexError, m.group, 1<<1000)
        self.assertRaises(IndexError, m.group, Index(1<<1000))
        self.assertRaises(IndexError, m.group, 'x')
        # Multiple groups
        self.assertEqual(m.group(2, 1), ('b', 'a'))
        self.assertEqual(m.group(Index(2), Index(1)), ('b', 'a'))

    def test_match_getitem(self):
        pat = re.compile('(?:(?P<a1>a)|(?P<b2>b))(?P<c3>c)?')

        m = pat.match('a')
        self.assertEqual(m['a1'], 'a')
        self.assertEqual(m['b2'], None)
        self.assertEqual(m['c3'], None)
        self.assertEqual('a1={a1} b2={b2} c3={c3}'.format_map(m), 'a1=a b2=None c3=None')
        self.assertEqual(m[0], 'a')
        self.assertEqual(m[1], 'a')
        self.assertEqual(m[2], None)
        self.assertEqual(m[3], None)
        with self.assertRaisesRegex(IndexError, 'no such group'):
            m['X']
        with self.assertRaisesRegex(IndexError, 'no such group'):
            m[-1]
        with self.assertRaisesRegex(IndexError, 'no such group'):
            m[4]
        with self.assertRaisesRegex(IndexError, 'no such group'):
            m[0, 1]
        with self.assertRaisesRegex(IndexError, 'no such group'):
            m[(0,)]
        with self.assertRaisesRegex(IndexError, 'no such group'):
            m[(0, 1)]
        with self.assertRaisesRegex(IndexError, 'no such group'):
            'a1={a2}'.format_map(m)

        m = pat.match('ac')
        self.assertEqual(m['a1'], 'a')
        self.assertEqual(m['b2'], None)
        self.assertEqual(m['c3'], 'c')
        self.assertEqual('a1={a1} b2={b2} c3={c3}'.format_map(m), 'a1=a b2=None c3=c')
        self.assertEqual(m[0], 'ac')
        self.assertEqual(m[1], 'a')
        self.assertEqual(m[2], None)
        self.assertEqual(m[3], 'c')

        # Cannot assign.
        with self.assertRaises(TypeError):
            m[0] = 1

        # No len().
        self.assertRaises(TypeError, len, m)

    def test_re_fullmatch(self):
        # Issue 16203: Proposal: add re.fullmatch() method.
        self.assertEqual(re.fullmatch(r"a", "a").span(), (0, 1))
        for string in "ab", S("ab"):
            self.assertEqual(re.fullmatch(r"a|ab", string).span(), (0, 2))
        for string in b"ab", B(b"ab"), bytearray(b"ab"), memoryview(b"ab"):
            self.assertEqual(re.fullmatch(br"a|ab", string).span(), (0, 2))
        for a, b in "\xe0\xdf", "\u0430\u0431", "\U0001d49c\U0001d49e":
            r = r"%s|%s" % (a, a + b)
            self.assertEqual(re.fullmatch(r, a + b).span(), (0, 2))
        self.assertEqual(re.fullmatch(r".*?$", "abc").span(), (0, 3))
        self.assertEqual(re.fullmatch(r".*?", "abc").span(), (0, 3))
        self.assertEqual(re.fullmatch(r"a.*?b", "ab").span(), (0, 2))
        self.assertEqual(re.fullmatch(r"a.*?b", "abb").span(), (0, 3))
        self.assertEqual(re.fullmatch(r"a.*?b", "axxb").span(), (0, 4))
        self.assertIsNone(re.fullmatch(r"a+", "ab"))
        self.assertIsNone(re.fullmatch(r"abc$", "abc\n"))
        self.assertIsNone(re.fullmatch(r"abc\Z", "abc\n"))
        self.assertIsNone(re.fullmatch(r"(?m)abc$", "abc\n"))
        self.assertEqual(re.fullmatch(r"ab(?=c)cd", "abcd").span(), (0, 4))
        self.assertEqual(re.fullmatch(r"ab(?<=b)cd", "abcd").span(), (0, 4))
        self.assertEqual(re.fullmatch(r"(?=a|ab)ab", "ab").span(), (0, 2))

        self.assertEqual(
            re.compile(r"bc").fullmatch("abcd", pos=1, endpos=3).span(), (1, 3))
        self.assertEqual(
            re.compile(r".*?$").fullmatch("abcd", pos=1, endpos=3).span(), (1, 3))
        self.assertEqual(
            re.compile(r".*?").fullmatch("abcd", pos=1, endpos=3).span(), (1, 3))

    def test_re_groupref_exists(self):
        self.assertEqual(re.match(r'^(\()?([^()]+)(?(1)\))$', '(a)').groups(),
                         ('(', 'a'))
        self.assertEqual(re.match(r'^(\()?([^()]+)(?(1)\))$', 'a').groups(),
                         (None, 'a'))
        self.assertIsNone(re.match(r'^(\()?([^()]+)(?(1)\))$', 'a)'))
        self.assertIsNone(re.match(r'^(\()?([^()]+)(?(1)\))$', '(a'))
        self.assertEqual(re.match('^(?:(a)|c)((?(1)b|d))$', 'ab').groups(),
                         ('a', 'b'))
        self.assertEqual(re.match(r'^(?:(a)|c)((?(1)b|d))$', 'cd').groups(),
                         (None, 'd'))
        self.assertEqual(re.match(r'^(?:(a)|c)((?(1)|d))$', 'cd').groups(),
                         (None, 'd'))
        self.assertEqual(re.match(r'^(?:(a)|c)((?(1)|d))$', 'a').groups(),
                         ('a', ''))

        # Tests for bug #1177831: exercise groups other than the first group
        p = re.compile('(?P<g1>a)(?P<g2>b)?((?(g2)c|d))')
        self.assertEqual(p.match('abc').groups(),
                         ('a', 'b', 'c'))
        self.assertEqual(p.match('ad').groups(),
                         ('a', None, 'd'))
        self.assertIsNone(p.match('abd'))
        self.assertIsNone(p.match('ac'))

        # Support > 100 groups.
        pat = '|'.join('x(?P<a%d>%x)y' % (i, i) for i in range(1, 200 + 1))
        pat = '(?:%s)(?(200)z)' % pat
        self.assertEqual(re.match(pat, 'xc8yz').span(), (0, 5))

        self.checkPatternError(r'(?P<a>)(?(0))', 'bad group number', 10)
        self.checkPatternError(r'()(?(1)a|b',
                               'missing ), unterminated subpattern', 2)
        self.checkPatternError(r'()(?(1)a|b|c)',
                               'conditional backref with more than '
                               'two branches', 10)

    def test_re_groupref_overflow(self):
        from sre_constants import MAXGROUPS
        self.checkTemplateError('()', r'\g<%s>' % MAXGROUPS, 'xx',
                                'invalid group reference %d' % MAXGROUPS, 3)
        self.checkPatternError(r'(?P<a>)(?(%d))' % MAXGROUPS,
                               'invalid group reference %d' % MAXGROUPS, 10)

    def test_re_groupref(self):
        self.assertEqual(re.match(r'^(\|)?([^()]+)\1$', '|a|').groups(),
                         ('|', 'a'))
        self.assertEqual(re.match(r'^(\|)?([^()]+)\1?$', 'a').groups(),
                         (None, 'a'))
        self.assertIsNone(re.match(r'^(\|)?([^()]+)\1$', 'a|'))
        self.assertIsNone(re.match(r'^(\|)?([^()]+)\1$', '|a'))
        self.assertEqual(re.match(r'^(?:(a)|c)(\1)$', 'aa').groups(),
                         ('a', 'a'))
        self.assertEqual(re.match(r'^(?:(a)|c)(\1)?$', 'c').groups(),
                         (None, None))

        self.checkPatternError(r'(abc\1)', 'cannot refer to an open group', 4)

    def test_groupdict(self):
        self.assertEqual(re.match('(?P<first>first) (?P<second>second)',
                                  'first second').groupdict(),
                         {'first':'first', 'second':'second'})

    def test_expand(self):
        self.assertEqual(re.match("(?P<first>first) (?P<second>second)",
                                  "first second")
                                  .expand(r"\2 \1 \g<second> \g<first>"),
                         "second first second first")
        self.assertEqual(re.match("(?P<first>first)|(?P<second>second)",
                                  "first")
                                  .expand(r"\2 \g<second>"),
                         " ")

    def test_repeat_minmax(self):
        self.assertIsNone(re.match(r"^(\w){1}$", "abc"))
        self.assertIsNone(re.match(r"^(\w){1}?$", "abc"))
        self.assertIsNone(re.match(r"^(\w){1,2}$", "abc"))
        self.assertIsNone(re.match(r"^(\w){1,2}?$", "abc"))

        self.assertEqual(re.match(r"^(\w){3}$", "abc").group(1), "c")
        self.assertEqual(re.match(r"^(\w){1,3}$", "abc").group(1), "c")
        self.assertEqual(re.match(r"^(\w){1,4}$", "abc").group(1), "c")
        self.assertEqual(re.match(r"^(\w){3,4}?$", "abc").group(1), "c")
        self.assertEqual(re.match(r"^(\w){3}?$", "abc").group(1), "c")
        self.assertEqual(re.match(r"^(\w){1,3}?$", "abc").group(1), "c")
        self.assertEqual(re.match(r"^(\w){1,4}?$", "abc").group(1), "c")
        self.assertEqual(re.match(r"^(\w){3,4}?$", "abc").group(1), "c")

        self.assertIsNone(re.match(r"^x{1}$", "xxx"))
        self.assertIsNone(re.match(r"^x{1}?$", "xxx"))
        self.assertIsNone(re.match(r"^x{1,2}$", "xxx"))
        self.assertIsNone(re.match(r"^x{1,2}?$", "xxx"))

        self.assertTrue(re.match(r"^x{3}$", "xxx"))
        self.assertTrue(re.match(r"^x{1,3}$", "xxx"))
        self.assertTrue(re.match(r"^x{3,3}$", "xxx"))
        self.assertTrue(re.match(r"^x{1,4}$", "xxx"))
        self.assertTrue(re.match(r"^x{3,4}?$", "xxx"))
        self.assertTrue(re.match(r"^x{3}?$", "xxx"))
        self.assertTrue(re.match(r"^x{1,3}?$", "xxx"))
        self.assertTrue(re.match(r"^x{1,4}?$", "xxx"))
        self.assertTrue(re.match(r"^x{3,4}?$", "xxx"))

        self.assertIsNone(re.match(r"^x{}$", "xxx"))
        self.assertTrue(re.match(r"^x{}$", "x{}"))

        self.checkPatternError(r'x{2,1}',
                               'min repeat greater than max repeat', 2)

    def test_getattr(self):
        self.assertEqual(re.compile("(?i)(a)(b)").pattern, "(?i)(a)(b)")
        self.assertEqual(re.compile("(?i)(a)(b)").flags, re.I | re.U)
        self.assertEqual(re.compile("(?i)(a)(b)").groups, 2)
        self.assertEqual(re.compile("(?i)(a)(b)").groupindex, {})
        self.assertEqual(re.compile("(?i)(?P<first>a)(?P<other>b)").groupindex,
                         {'first': 1, 'other': 2})

        self.assertEqual(re.match("(a)", "a").pos, 0)
        self.assertEqual(re.match("(a)", "a").endpos, 1)
        self.assertEqual(re.match("(a)", "a").string, "a")
        self.assertEqual(re.match("(a)", "a").regs, ((0, 1), (0, 1)))
        self.assertTrue(re.match("(a)", "a").re)

        # Issue 14260. groupindex should be non-modifiable mapping.
        p = re.compile(r'(?i)(?P<first>a)(?P<other>b)')
        self.assertEqual(sorted(p.groupindex), ['first', 'other'])
        self.assertEqual(p.groupindex['other'], 2)
        with self.assertRaises(TypeError):
            p.groupindex['other'] = 0
        self.assertEqual(p.groupindex['other'], 2)

    def test_special_escapes(self):
        self.assertEqual(re.search(r"\b(b.)\b",
                                   "abcd abc bcd bx").group(1), "bx")
        self.assertEqual(re.search(r"\B(b.)\B",
                                   "abc bcd bc abxd").group(1), "bx")
        self.assertEqual(re.search(r"\b(b.)\b",
                                   "abcd abc bcd bx", re.ASCII).group(1), "bx")
        self.assertEqual(re.search(r"\B(b.)\B",
                                   "abc bcd bc abxd", re.ASCII).group(1), "bx")
        self.assertEqual(re.search(r"^abc$", "\nabc\n", re.M).group(0), "abc")
        self.assertEqual(re.search(r"^\Aabc\Z$", "abc", re.M).group(0), "abc")
        self.assertIsNone(re.search(r"^\Aabc\Z$", "\nabc\n", re.M))
        self.assertEqual(re.search(br"\b(b.)\b",
                                   b"abcd abc bcd bx").group(1), b"bx")
        self.assertEqual(re.search(br"\B(b.)\B",
                                   b"abc bcd bc abxd").group(1), b"bx")
        self.assertEqual(re.search(br"\b(b.)\b",
                                   b"abcd abc bcd bx", re.LOCALE).group(1), b"bx")
        self.assertEqual(re.search(br"\B(b.)\B",
                                   b"abc bcd bc abxd", re.LOCALE).group(1), b"bx")
        self.assertEqual(re.search(br"^abc$", b"\nabc\n", re.M).group(0), b"abc")
        self.assertEqual(re.search(br"^\Aabc\Z$", b"abc", re.M).group(0), b"abc")
        self.assertIsNone(re.search(br"^\Aabc\Z$", b"\nabc\n", re.M))
        self.assertEqual(re.search(r"\d\D\w\W\s\S",
                                   "1aa! a").group(0), "1aa! a")
        self.assertEqual(re.search(br"\d\D\w\W\s\S",
                                   b"1aa! a").group(0), b"1aa! a")
        self.assertEqual(re.search(r"\d\D\w\W\s\S",
                                   "1aa! a", re.ASCII).group(0), "1aa! a")
        self.assertEqual(re.search(br"\d\D\w\W\s\S",
                                   b"1aa! a", re.LOCALE).group(0), b"1aa! a")

    def test_other_escapes(self):
        self.checkPatternError("\\", 'bad escape (end of pattern)', 0)
        self.assertEqual(re.match(r"\(", '(').group(), '(')
        self.assertIsNone(re.match(r"\(", ')'))
        self.assertEqual(re.match(r"\\", '\\').group(), '\\')
        self.assertEqual(re.match(r"[\]]", ']').group(), ']')
        self.assertIsNone(re.match(r"[\]]", '['))
        self.assertEqual(re.match(r"[a\-c]", '-').group(), '-')
        self.assertIsNone(re.match(r"[a\-c]", 'b'))
        self.assertEqual(re.match(r"[\^a]+", 'a^').group(), 'a^')
        self.assertIsNone(re.match(r"[\^a]+", 'b'))
        re.purge()  # for warnings
        for c in 'ceghijklmopqyzCEFGHIJKLMNOPQRTVXY':
            with self.subTest(c):
                self.assertRaises(re.error, re.compile, '\\%c' % c)
        for c in 'ceghijklmopqyzABCEFGHIJKLMNOPQRTVXYZ':
            with self.subTest(c):
                self.assertRaises(re.error, re.compile, '[\\%c]' % c)

    def test_named_unicode_escapes(self):
        # test individual Unicode named escapes
        self.assertTrue(re.match(r'\N{LESS-THAN SIGN}', '<'))
        self.assertTrue(re.match(r'\N{less-than sign}', '<'))
        self.assertIsNone(re.match(r'\N{LESS-THAN SIGN}', '>'))
        self.assertTrue(re.match(r'\N{SNAKE}', '\U0001f40d'))
        self.assertTrue(re.match(r'\N{ARABIC LIGATURE UIGHUR KIRGHIZ YEH WITH '
                                 r'HAMZA ABOVE WITH ALEF MAKSURA ISOLATED FORM}',
                                 '\ufbf9'))
        self.assertTrue(re.match(r'[\N{LESS-THAN SIGN}-\N{GREATER-THAN SIGN}]',
                                 '='))
        self.assertIsNone(re.match(r'[\N{LESS-THAN SIGN}-\N{GREATER-THAN SIGN}]',
                                   ';'))

        # test errors in \N{name} handling - only valid names should pass
        self.checkPatternError(r'\N', 'missing {', 2)
        self.checkPatternError(r'[\N]', 'missing {', 3)
        self.checkPatternError(r'\N{', 'missing character name', 3)
        self.checkPatternError(r'[\N{', 'missing character name', 4)
        self.checkPatternError(r'\N{}', 'missing character name', 3)
        self.checkPatternError(r'[\N{}]', 'missing character name', 4)
        self.checkPatternError(r'\NSNAKE}', 'missing {', 2)
        self.checkPatternError(r'[\NSNAKE}]', 'missing {', 3)
        self.checkPatternError(r'\N{SNAKE',
                               'missing }, unterminated name', 3)
        self.checkPatternError(r'[\N{SNAKE]',
                               'missing }, unterminated name', 4)
        self.checkPatternError(r'[\N{SNAKE]}',
                               "undefined character name 'SNAKE]'", 1)
        self.checkPatternError(r'\N{SPAM}',
                               "undefined character name 'SPAM'", 0)
        self.checkPatternError(r'[\N{SPAM}]',
                               "undefined character name 'SPAM'", 1)
        self.checkPatternError(br'\N{LESS-THAN SIGN}', r'bad escape \N', 0)
        self.checkPatternError(br'[\N{LESS-THAN SIGN}]', r'bad escape \N', 1)

    def test_string_boundaries(self):
        # See http://bugs.python.org/issue10713
        self.assertEqual(re.search(r"\b(abc)\b", "abc").group(1),
                         "abc")
        # There's a word boundary at the start of a string.
        self.assertTrue(re.match(r"\b", "abc"))
        # A non-empty string includes a non-boundary zero-length match.
        self.assertTrue(re.search(r"\B", "abc"))
        # There is no non-boundary match at the start of a string.
        self.assertFalse(re.match(r"\B", "abc"))
        # However, an empty string contains no word boundaries, and also no
        # non-boundaries.
        self.assertIsNone(re.search(r"\B", ""))
        # This one is questionable and different from the perlre behaviour,
        # but describes current behavior.
        self.assertIsNone(re.search(r"\b", ""))
        # A single word-character string has two boundaries, but no
        # non-boundary gaps.
        self.assertEqual(len(re.findall(r"\b", "a")), 2)
        self.assertEqual(len(re.findall(r"\B", "a")), 0)
        # If there are no words, there are no boundaries
        self.assertEqual(len(re.findall(r"\b", " ")), 0)
        self.assertEqual(len(re.findall(r"\b", "   ")), 0)
        # Can match around the whitespace.
        self.assertEqual(len(re.findall(r"\B", " ")), 2)

    def test_bigcharset(self):
        self.assertEqual(re.match("([\u2222\u2223])",
                                  "\u2222").group(1), "\u2222")
        r = '[%s]' % ''.join(map(chr, range(256, 2**16, 255)))
        self.assertEqual(re.match(r, "\uff01").group(), "\uff01")

    def test_big_codesize(self):
        # Issue #1160
        r = re.compile('|'.join(('%d'%x for x in range(10000))))
        self.assertTrue(r.match('1000'))
        self.assertTrue(r.match('9999'))

    def test_anyall(self):
        self.assertEqual(re.match("a.b", "a\nb", re.DOTALL).group(0),
                         "a\nb")
        self.assertEqual(re.match("a.*b", "a\n\nb", re.DOTALL).group(0),
                         "a\n\nb")

    def test_lookahead(self):
        self.assertEqual(re.match(r"(a(?=\s[^a]))", "a b").group(1), "a")
        self.assertEqual(re.match(r"(a(?=\s[^a]*))", "a b").group(1), "a")
        self.assertEqual(re.match(r"(a(?=\s[abc]))", "a b").group(1), "a")
        self.assertEqual(re.match(r"(a(?=\s[abc]*))", "a bc").group(1), "a")
        self.assertEqual(re.match(r"(a)(?=\s\1)", "a a").group(1), "a")
        self.assertEqual(re.match(r"(a)(?=\s\1*)", "a aa").group(1), "a")
        self.assertEqual(re.match(r"(a)(?=\s(abc|a))", "a a").group(1), "a")

        self.assertEqual(re.match(r"(a(?!\s[^a]))", "a a").group(1), "a")
        self.assertEqual(re.match(r"(a(?!\s[abc]))", "a d").group(1), "a")
        self.assertEqual(re.match(r"(a)(?!\s\1)", "a b").group(1), "a")
        self.assertEqual(re.match(r"(a)(?!\s(abc|a))", "a b").group(1), "a")

        # Group reference.
        self.assertTrue(re.match(r'(a)b(?=\1)a', 'aba'))
        self.assertIsNone(re.match(r'(a)b(?=\1)c', 'abac'))
        # Conditional group reference.
        self.assertTrue(re.match(r'(?:(a)|(x))b(?=(?(2)x|c))c', 'abc'))
        self.assertIsNone(re.match(r'(?:(a)|(x))b(?=(?(2)c|x))c', 'abc'))
        self.assertTrue(re.match(r'(?:(a)|(x))b(?=(?(2)x|c))c', 'abc'))
        self.assertIsNone(re.match(r'(?:(a)|(x))b(?=(?(1)b|x))c', 'abc'))
        self.assertTrue(re.match(r'(?:(a)|(x))b(?=(?(1)c|x))c', 'abc'))
        # Group used before defined.
        self.assertTrue(re.match(r'(a)b(?=(?(2)x|c))(c)', 'abc'))
        self.assertIsNone(re.match(r'(a)b(?=(?(2)b|x))(c)', 'abc'))
        self.assertTrue(re.match(r'(a)b(?=(?(1)c|x))(c)', 'abc'))

    def test_lookbehind(self):
        self.assertTrue(re.match(r'ab(?<=b)c', 'abc'))
        self.assertIsNone(re.match(r'ab(?<=c)c', 'abc'))
        self.assertIsNone(re.match(r'ab(?<!b)c', 'abc'))
        self.assertTrue(re.match(r'ab(?<!c)c', 'abc'))
        # Group reference.
        self.assertTrue(re.match(r'(a)a(?<=\1)c', 'aac'))
        self.assertIsNone(re.match(r'(a)b(?<=\1)a', 'abaa'))
        self.assertIsNone(re.match(r'(a)a(?<!\1)c', 'aac'))
        self.assertTrue(re.match(r'(a)b(?<!\1)a', 'abaa'))
        # Conditional group reference.
        self.assertIsNone(re.match(r'(?:(a)|(x))b(?<=(?(2)x|c))c', 'abc'))
        self.assertIsNone(re.match(r'(?:(a)|(x))b(?<=(?(2)b|x))c', 'abc'))
        self.assertTrue(re.match(r'(?:(a)|(x))b(?<=(?(2)x|b))c', 'abc'))
        self.assertIsNone(re.match(r'(?:(a)|(x))b(?<=(?(1)c|x))c', 'abc'))
        self.assertTrue(re.match(r'(?:(a)|(x))b(?<=(?(1)b|x))c', 'abc'))
        # Group used before defined.
        self.assertRaises(re.error, re.compile, r'(a)b(?<=(?(2)b|x))(c)')
        self.assertIsNone(re.match(r'(a)b(?<=(?(1)c|x))(c)', 'abc'))
        self.assertTrue(re.match(r'(a)b(?<=(?(1)b|x))(c)', 'abc'))
        # Group defined in the same lookbehind pattern
        self.assertRaises(re.error, re.compile, r'(a)b(?<=(.)\2)(c)')
        self.assertRaises(re.error, re.compile, r'(a)b(?<=(?P<a>.)(?P=a))(c)')
        self.assertRaises(re.error, re.compile, r'(a)b(?<=(a)(?(2)b|x))(c)')
        self.assertRaises(re.error, re.compile, r'(a)b(?<=(.)(?<=\2))(c)')

    def test_ignore_case(self):
        self.assertEqual(re.match("abc", "ABC", re.I).group(0), "ABC")
        self.assertEqual(re.match(b"abc", b"ABC", re.I).group(0), b"ABC")
        self.assertEqual(re.match(r"(a\s[^a])", "a b", re.I).group(1), "a b")
        self.assertEqual(re.match(r"(a\s[^a]*)", "a bb", re.I).group(1), "a bb")
        self.assertEqual(re.match(r"(a\s[abc])", "a b", re.I).group(1), "a b")
        self.assertEqual(re.match(r"(a\s[abc]*)", "a bb", re.I).group(1), "a bb")
        self.assertEqual(re.match(r"((a)\s\2)", "a a", re.I).group(1), "a a")
        self.assertEqual(re.match(r"((a)\s\2*)", "a aa", re.I).group(1), "a aa")
        self.assertEqual(re.match(r"((a)\s(abc|a))", "a a", re.I).group(1), "a a")
        self.assertEqual(re.match(r"((a)\s(abc|a)*)", "a aa", re.I).group(1), "a aa")

        assert '\u212a'.lower() == 'k' # 'K'
        self.assertTrue(re.match(r'K', '\u212a', re.I))
        self.assertTrue(re.match(r'k', '\u212a', re.I))
        self.assertTrue(re.match(r'\u212a', 'K', re.I))
        self.assertTrue(re.match(r'\u212a', 'k', re.I))
        assert '\u017f'.upper() == 'S' # 'ſ'
        self.assertTrue(re.match(r'S', '\u017f', re.I))
        self.assertTrue(re.match(r's', '\u017f', re.I))
        self.assertTrue(re.match(r'\u017f', 'S', re.I))
        self.assertTrue(re.match(r'\u017f', 's', re.I))
        assert '\ufb05'.upper() == '\ufb06'.upper() == 'ST' # 'ﬅ', 'ﬆ'
        self.assertTrue(re.match(r'\ufb05', '\ufb06', re.I))
        self.assertTrue(re.match(r'\ufb06', '\ufb05', re.I))

    def test_ignore_case_set(self):
        self.assertTrue(re.match(r'[19A]', 'A', re.I))
        self.assertTrue(re.match(r'[19a]', 'a', re.I))
        self.assertTrue(re.match(r'[19a]', 'A', re.I))
        self.assertTrue(re.match(r'[19A]', 'a', re.I))
        self.assertTrue(re.match(br'[19A]', b'A', re.I))
        self.assertTrue(re.match(br'[19a]', b'a', re.I))
        self.assertTrue(re.match(br'[19a]', b'A', re.I))
        self.assertTrue(re.match(br'[19A]', b'a', re.I))
        assert '\u212a'.lower() == 'k' # 'K'
        self.assertTrue(re.match(r'[19K]', '\u212a', re.I))
        self.assertTrue(re.match(r'[19k]', '\u212a', re.I))
        self.assertTrue(re.match(r'[19\u212a]', 'K', re.I))
        self.assertTrue(re.match(r'[19\u212a]', 'k', re.I))
        assert '\u017f'.upper() == 'S' # 'ſ'
        self.assertTrue(re.match(r'[19S]', '\u017f', re.I))
        self.assertTrue(re.match(r'[19s]', '\u017f', re.I))
        self.assertTrue(re.match(r'[19\u017f]', 'S', re.I))
        self.assertTrue(re.match(r'[19\u017f]', 's', re.I))
        assert '\ufb05'.upper() == '\ufb06'.upper() == 'ST' # 'ﬅ', 'ﬆ'
        self.assertTrue(re.match(r'[19\ufb05]', '\ufb06', re.I))
        self.assertTrue(re.match(r'[19\ufb06]', '\ufb05', re.I))

    def test_ignore_case_range(self):
        # Issues #3511, #17381.
        self.assertTrue(re.match(r'[9-a]', '_', re.I))
        self.assertIsNone(re.match(r'[9-A]', '_', re.I))
        self.assertTrue(re.match(br'[9-a]', b'_', re.I))
        self.assertIsNone(re.match(br'[9-A]', b'_', re.I))
        self.assertTrue(re.match(r'[\xc0-\xde]', '\xd7', re.I))
        self.assertIsNone(re.match(r'[\xc0-\xde]', '\xf7', re.I))
        self.assertTrue(re.match(r'[\xe0-\xfe]', '\xf7', re.I))
        self.assertIsNone(re.match(r'[\xe0-\xfe]', '\xd7', re.I))
        self.assertTrue(re.match(r'[\u0430-\u045f]', '\u0450', re.I))
        self.assertTrue(re.match(r'[\u0430-\u045f]', '\u0400', re.I))
        self.assertTrue(re.match(r'[\u0400-\u042f]', '\u0450', re.I))
        self.assertTrue(re.match(r'[\u0400-\u042f]', '\u0400', re.I))
        self.assertTrue(re.match(r'[\U00010428-\U0001044f]', '\U00010428', re.I))
        self.assertTrue(re.match(r'[\U00010428-\U0001044f]', '\U00010400', re.I))
        self.assertTrue(re.match(r'[\U00010400-\U00010427]', '\U00010428', re.I))
        self.assertTrue(re.match(r'[\U00010400-\U00010427]', '\U00010400', re.I))

        assert '\u212a'.lower() == 'k' # 'K'
        self.assertTrue(re.match(r'[J-M]', '\u212a', re.I))
        self.assertTrue(re.match(r'[j-m]', '\u212a', re.I))
        self.assertTrue(re.match(r'[\u2129-\u212b]', 'K', re.I))
        self.assertTrue(re.match(r'[\u2129-\u212b]', 'k', re.I))
        assert '\u017f'.upper() == 'S' # 'ſ'
        self.assertTrue(re.match(r'[R-T]', '\u017f', re.I))
        self.assertTrue(re.match(r'[r-t]', '\u017f', re.I))
        self.assertTrue(re.match(r'[\u017e-\u0180]', 'S', re.I))
        self.assertTrue(re.match(r'[\u017e-\u0180]', 's', re.I))
        assert '\ufb05'.upper() == '\ufb06'.upper() == 'ST' # 'ﬅ', 'ﬆ'
        self.assertTrue(re.match(r'[\ufb04-\ufb05]', '\ufb06', re.I))
        self.assertTrue(re.match(r'[\ufb06-\ufb07]', '\ufb05', re.I))

    def test_category(self):
        self.assertEqual(re.match(r"(\s)", " ").group(1), " ")

    @cpython_only
    def test_case_helpers(self):
        import _sre
        for i in range(128):
            c = chr(i)
            lo = ord(c.lower())
            self.assertEqual(_sre.ascii_tolower(i), lo)
            self.assertEqual(_sre.unicode_tolower(i), lo)
            iscased = c in string.ascii_letters
            self.assertEqual(_sre.ascii_iscased(i), iscased)
            self.assertEqual(_sre.unicode_iscased(i), iscased)

        for i in list(range(128, 0x1000)) + [0x10400, 0x10428]:
            c = chr(i)
            self.assertEqual(_sre.ascii_tolower(i), i)
            if i != 0x0130:
                self.assertEqual(_sre.unicode_tolower(i), ord(c.lower()))
            iscased = c != c.lower() or c != c.upper()
            self.assertFalse(_sre.ascii_iscased(i))
            self.assertEqual(_sre.unicode_iscased(i),
                             c != c.lower() or c != c.upper())

        self.assertEqual(_sre.ascii_tolower(0x0130), 0x0130)
        self.assertEqual(_sre.unicode_tolower(0x0130), ord('i'))
        self.assertFalse(_sre.ascii_iscased(0x0130))
        self.assertTrue(_sre.unicode_iscased(0x0130))

    def test_not_literal(self):
        self.assertEqual(re.search(r"\s([^a])", " b").group(1), "b")
        self.assertEqual(re.search(r"\s([^a]*)", " bb").group(1), "bb")

    def test_possible_set_operations(self):
        s = bytes(range(128)).decode()
        with self.assertWarns(FutureWarning):
            p = re.compile(r'[0-9--1]')
        self.assertEqual(p.findall(s), list('-./0123456789'))
        self.assertEqual(re.findall(r'[--1]', s), list('-./01'))
        with self.assertWarns(FutureWarning):
            p = re.compile(r'[%--1]')
        self.assertEqual(p.findall(s), list("%&'()*+,-1"))
        with self.assertWarns(FutureWarning):
            p = re.compile(r'[%--]')
        self.assertEqual(p.findall(s), list("%&'()*+,-"))

        with self.assertWarns(FutureWarning):
            p = re.compile(r'[0-9&&1]')
        self.assertEqual(p.findall(s), list('&0123456789'))
        with self.assertWarns(FutureWarning):
            p = re.compile(r'[\d&&1]')
        self.assertEqual(p.findall(s), list('&0123456789'))
        self.assertEqual(re.findall(r'[&&1]', s), list('&1'))

        with self.assertWarns(FutureWarning):
            p = re.compile(r'[0-9||a]')
        self.assertEqual(p.findall(s), list('0123456789a|'))
        with self.assertWarns(FutureWarning):
            p = re.compile(r'[\d||a]')
        self.assertEqual(p.findall(s), list('0123456789a|'))
        self.assertEqual(re.findall(r'[||1]', s), list('1|'))

        with self.assertWarns(FutureWarning):
            p = re.compile(r'[0-9~~1]')
        self.assertEqual(p.findall(s), list('0123456789~'))
        with self.assertWarns(FutureWarning):
            p = re.compile(r'[\d~~1]')
        self.assertEqual(p.findall(s), list('0123456789~'))
        self.assertEqual(re.findall(r'[~~1]', s), list('1~'))

        with self.assertWarns(FutureWarning):
            p = re.compile(r'[[0-9]|]')
        self.assertEqual(p.findall(s), list('0123456789[]'))

        with self.assertWarns(FutureWarning):
            p = re.compile(r'[[:digit:]|]')
        self.assertEqual(p.findall(s), list(':[]dgit'))

    def test_search_coverage(self):
        self.assertEqual(re.search(r"\s(b)", " b").group(1), "b")
        self.assertEqual(re.search(r"a\s", "a ").group(0), "a ")

    def assertMatch(self, pattern, text, match=None, span=None,
                    matcher=re.fullmatch):
        if match is None and span is None:
            # the pattern matches the whole text
            match = text
            span = (0, len(text))
        elif match is None or span is None:
            raise ValueError('If match is not None, span should be specified '
                             '(and vice versa).')
        m = matcher(pattern, text)
        self.assertTrue(m)
        self.assertEqual(m.group(), match)
        self.assertEqual(m.span(), span)

    LITERAL_CHARS = string.ascii_letters + string.digits + '!"%\',/:;<=>@_`'

    def test_re_escape(self):
        p = ''.join(chr(i) for i in range(256))
        for c in p:
            self.assertMatch(re.escape(c), c)
            self.assertMatch('[' + re.escape(c) + ']', c)
            self.assertMatch('(?x)' + re.escape(c), c)
        self.assertMatch(re.escape(p), p)
        for c in '-.]{}':
            self.assertEqual(re.escape(c)[:1], '\\')
        literal_chars = self.LITERAL_CHARS
        self.assertEqual(re.escape(literal_chars), literal_chars)

    def test_re_escape_bytes(self):
        p = bytes(range(256))
        for i in p:
            b = bytes([i])
            self.assertMatch(re.escape(b), b)
            self.assertMatch(b'[' + re.escape(b) + b']', b)
            self.assertMatch(b'(?x)' + re.escape(b), b)
        self.assertMatch(re.escape(p), p)
        for i in b'-.]{}':
            b = bytes([i])
            self.assertEqual(re.escape(b)[:1], b'\\')
        literal_chars = self.LITERAL_CHARS.encode('ascii')
        self.assertEqual(re.escape(literal_chars), literal_chars)

    def test_re_escape_non_ascii(self):
        s = 'xxx\u2620\u2620\u2620xxx'
        s_escaped = re.escape(s)
        self.assertEqual(s_escaped, s)
        self.assertMatch(s_escaped, s)
        self.assertMatch('.%s+.' % re.escape('\u2620'), s,
                         'x\u2620\u2620\u2620x', (2, 7), re.search)

    def test_re_escape_non_ascii_bytes(self):
        b = 'y\u2620y\u2620y'.encode('utf-8')
        b_escaped = re.escape(b)
        self.assertEqual(b_escaped, b)
        self.assertMatch(b_escaped, b)
        res = re.findall(re.escape('\u2620'.encode('utf-8')), b)
        self.assertEqual(len(res), 2)

    def test_pickling(self):
        import pickle
        oldpat = re.compile('a(?:b|(c|e){1,2}?|d)+?(.)', re.UNICODE)
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            pickled = pickle.dumps(oldpat, proto)
            newpat = pickle.loads(pickled)
            self.assertEqual(newpat, oldpat)
        # current pickle expects the _compile() reconstructor in re module
        from re import _compile

    def test_copying(self):
        import copy
        p = re.compile(r'(?P<int>\d+)(?:\.(?P<frac>\d*))?')
        self.assertIs(copy.copy(p), p)
        self.assertIs(copy.deepcopy(p), p)
        m = p.match('12.34')
        self.assertIs(copy.copy(m), m)
        self.assertIs(copy.deepcopy(m), m)

    def test_constants(self):
        self.assertEqual(re.I, re.IGNORECASE)
        self.assertEqual(re.L, re.LOCALE)
        self.assertEqual(re.M, re.MULTILINE)
        self.assertEqual(re.S, re.DOTALL)
        self.assertEqual(re.X, re.VERBOSE)

    def test_flags(self):
        for flag in [re.I, re.M, re.X, re.S, re.A, re.U]:
            self.assertTrue(re.compile('^pattern$', flag))
        for flag in [re.I, re.M, re.X, re.S, re.A, re.L]:
            self.assertTrue(re.compile(b'^pattern$', flag))

    def test_sre_character_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255, 256, 0xFFFF, 0x10000, 0x10FFFF]:
            if i < 256:
                self.assertTrue(re.match(r"\%03o" % i, chr(i)))
                self.assertTrue(re.match(r"\%03o0" % i, chr(i)+"0"))
                self.assertTrue(re.match(r"\%03o8" % i, chr(i)+"8"))
                self.assertTrue(re.match(r"\x%02x" % i, chr(i)))
                self.assertTrue(re.match(r"\x%02x0" % i, chr(i)+"0"))
                self.assertTrue(re.match(r"\x%02xz" % i, chr(i)+"z"))
            if i < 0x10000:
                self.assertTrue(re.match(r"\u%04x" % i, chr(i)))
                self.assertTrue(re.match(r"\u%04x0" % i, chr(i)+"0"))
                self.assertTrue(re.match(r"\u%04xz" % i, chr(i)+"z"))
            self.assertTrue(re.match(r"\U%08x" % i, chr(i)))
            self.assertTrue(re.match(r"\U%08x0" % i, chr(i)+"0"))
            self.assertTrue(re.match(r"\U%08xz" % i, chr(i)+"z"))
        self.assertTrue(re.match(r"\0", "\000"))
        self.assertTrue(re.match(r"\08", "\0008"))
        self.assertTrue(re.match(r"\01", "\001"))
        self.assertTrue(re.match(r"\018", "\0018"))
        self.checkPatternError(r"\567",
                               r'octal escape value \567 outside of '
                               r'range 0-0o377', 0)
        self.checkPatternError(r"\911", 'invalid group reference 91', 1)
        self.checkPatternError(r"\x1", r'incomplete escape \x1', 0)
        self.checkPatternError(r"\x1z", r'incomplete escape \x1', 0)
        self.checkPatternError(r"\u123", r'incomplete escape \u123', 0)
        self.checkPatternError(r"\u123z", r'incomplete escape \u123', 0)
        self.checkPatternError(r"\U0001234", r'incomplete escape \U0001234', 0)
        self.checkPatternError(r"\U0001234z", r'incomplete escape \U0001234', 0)
        self.checkPatternError(r"\U00110000", r'bad escape \U00110000', 0)

    def test_sre_character_class_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255, 256, 0xFFFF, 0x10000, 0x10FFFF]:
            if i < 256:
                self.assertTrue(re.match(r"[\%o]" % i, chr(i)))
                self.assertTrue(re.match(r"[\%o8]" % i, chr(i)))
                self.assertTrue(re.match(r"[\%03o]" % i, chr(i)))
                self.assertTrue(re.match(r"[\%03o0]" % i, chr(i)))
                self.assertTrue(re.match(r"[\%03o8]" % i, chr(i)))
                self.assertTrue(re.match(r"[\x%02x]" % i, chr(i)))
                self.assertTrue(re.match(r"[\x%02x0]" % i, chr(i)))
                self.assertTrue(re.match(r"[\x%02xz]" % i, chr(i)))
            if i < 0x10000:
                self.assertTrue(re.match(r"[\u%04x]" % i, chr(i)))
                self.assertTrue(re.match(r"[\u%04x0]" % i, chr(i)))
                self.assertTrue(re.match(r"[\u%04xz]" % i, chr(i)))
            self.assertTrue(re.match(r"[\U%08x]" % i, chr(i)))
            self.assertTrue(re.match(r"[\U%08x0]" % i, chr(i)+"0"))
            self.assertTrue(re.match(r"[\U%08xz]" % i, chr(i)+"z"))
        self.checkPatternError(r"[\567]",
                               r'octal escape value \567 outside of '
                               r'range 0-0o377', 1)
        self.checkPatternError(r"[\911]", r'bad escape \9', 1)
        self.checkPatternError(r"[\x1z]", r'incomplete escape \x1', 1)
        self.checkPatternError(r"[\u123z]", r'incomplete escape \u123', 1)
        self.checkPatternError(r"[\U0001234z]", r'incomplete escape \U0001234', 1)
        self.checkPatternError(r"[\U00110000]", r'bad escape \U00110000', 1)
        self.assertTrue(re.match(r"[\U0001d49c-\U0001d4b5]", "\U0001d49e"))

    def test_sre_byte_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255]:
            self.assertTrue(re.match((r"\%03o" % i).encode(), bytes([i])))
            self.assertTrue(re.match((r"\%03o0" % i).encode(), bytes([i])+b"0"))
            self.assertTrue(re.match((r"\%03o8" % i).encode(), bytes([i])+b"8"))
            self.assertTrue(re.match((r"\x%02x" % i).encode(), bytes([i])))
            self.assertTrue(re.match((r"\x%02x0" % i).encode(), bytes([i])+b"0"))
            self.assertTrue(re.match((r"\x%02xz" % i).encode(), bytes([i])+b"z"))
        self.assertRaises(re.error, re.compile, br"\u1234")
        self.assertRaises(re.error, re.compile, br"\U00012345")
        self.assertTrue(re.match(br"\0", b"\000"))
        self.assertTrue(re.match(br"\08", b"\0008"))
        self.assertTrue(re.match(br"\01", b"\001"))
        self.assertTrue(re.match(br"\018", b"\0018"))
        self.checkPatternError(br"\567",
                               r'octal escape value \567 outside of '
                               r'range 0-0o377', 0)
        self.checkPatternError(br"\911", 'invalid group reference 91', 1)
        self.checkPatternError(br"\x1", r'incomplete escape \x1', 0)
        self.checkPatternError(br"\x1z", r'incomplete escape \x1', 0)

    def test_sre_byte_class_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255]:
            self.assertTrue(re.match((r"[\%o]" % i).encode(), bytes([i])))
            self.assertTrue(re.match((r"[\%o8]" % i).encode(), bytes([i])))
            self.assertTrue(re.match((r"[\%03o]" % i).encode(), bytes([i])))
            self.assertTrue(re.match((r"[\%03o0]" % i).encode(), bytes([i])))
            self.assertTrue(re.match((r"[\%03o8]" % i).encode(), bytes([i])))
            self.assertTrue(re.match((r"[\x%02x]" % i).encode(), bytes([i])))
            self.assertTrue(re.match((r"[\x%02x0]" % i).encode(), bytes([i])))
            self.assertTrue(re.match((r"[\x%02xz]" % i).encode(), bytes([i])))
        self.assertRaises(re.error, re.compile, br"[\u1234]")
        self.assertRaises(re.error, re.compile, br"[\U00012345]")
        self.checkPatternError(br"[\567]",
                               r'octal escape value \567 outside of '
                               r'range 0-0o377', 1)
        self.checkPatternError(br"[\911]", r'bad escape \9', 1)
        self.checkPatternError(br"[\x1z]", r'incomplete escape \x1', 1)

    def test_character_set_errors(self):
        self.checkPatternError(r'[', 'unterminated character set', 0)
        self.checkPatternError(r'[^', 'unterminated character set', 0)
        self.checkPatternError(r'[a', 'unterminated character set', 0)
        # bug 545855 -- This pattern failed to cause a compile error as it
        # should, instead provoking a TypeError.
        self.checkPatternError(r"[a-", 'unterminated character set', 0)
        self.checkPatternError(r"[\w-b]", r'bad character range \w-b', 1)
        self.checkPatternError(r"[a-\w]", r'bad character range a-\w', 1)
        self.checkPatternError(r"[b-a]", 'bad character range b-a', 1)

    def test_bug_113254(self):
        self.assertEqual(re.match(r'(a)|(b)', 'b').start(1), -1)
        self.assertEqual(re.match(r'(a)|(b)', 'b').end(1), -1)
        self.assertEqual(re.match(r'(a)|(b)', 'b').span(1), (-1, -1))

    def test_bug_527371(self):
        # bug described in patches 527371/672491
        self.assertIsNone(re.match(r'(a)?a','a').lastindex)
        self.assertEqual(re.match(r'(a)(b)?b','ab').lastindex, 1)
        self.assertEqual(re.match(r'(?P<a>a)(?P<b>b)?b','ab').lastgroup, 'a')
        self.assertEqual(re.match(r"(?P<a>a(b))", "ab").lastgroup, 'a')
        self.assertEqual(re.match(r"((a))", "a").lastindex, 1)

    def test_bug_418626(self):
        # bugs 418626 at al. -- Testing Greg Chapman's addition of op code
        # SRE_OP_MIN_REPEAT_ONE for eliminating recursion on simple uses of
        # pattern '*?' on a long string.
        self.assertEqual(re.match('.*?c', 10000*'ab'+'cd').end(0), 20001)
        self.assertEqual(re.match('.*?cd', 5000*'ab'+'c'+5000*'ab'+'cde').end(0),
                         20003)
        self.assertEqual(re.match('.*?cd', 20000*'abc'+'de').end(0), 60001)
        # non-simple '*?' still used to hit the recursion limit, before the
        # non-recursive scheme was implemented.
        self.assertEqual(re.search('(a|b)*?c', 10000*'ab'+'cd').end(0), 20001)

    def test_bug_612074(self):
        pat="["+re.escape("\u2039")+"]"
        self.assertEqual(re.compile(pat) and 1, 1)

    def test_stack_overflow(self):
        # nasty cases that used to overflow the straightforward recursive
        # implementation of repeated groups.
        self.assertEqual(re.match('(x)*', 50000*'x').group(1), 'x')
        self.assertEqual(re.match('(x)*y', 50000*'x'+'y').group(1), 'x')
        self.assertEqual(re.match('(x)*?y', 50000*'x'+'y').group(1), 'x')

    def test_nothing_to_repeat(self):
        for reps in '*', '+', '?', '{1,2}':
            for mod in '', '?':
                self.checkPatternError('%s%s' % (reps, mod),
                                       'nothing to repeat', 0)
                self.checkPatternError('(?:%s%s)' % (reps, mod),
                                       'nothing to repeat', 3)

    def test_multiple_repeat(self):
        for outer_reps in '*', '+', '{1,2}':
            for outer_mod in '', '?':
                outer_op = outer_reps + outer_mod
                for inner_reps in '*', '+', '?', '{1,2}':
                    for inner_mod in '', '?':
                        inner_op = inner_reps + inner_mod
                        self.checkPatternError(r'x%s%s' % (inner_op, outer_op),
                                'multiple repeat', 1 + len(inner_op))

    def test_unlimited_zero_width_repeat(self):
        # Issue #9669
        self.assertIsNone(re.match(r'(?:a?)*y', 'z'))
        self.assertIsNone(re.match(r'(?:a?)+y', 'z'))
        self.assertIsNone(re.match(r'(?:a?){2,}y', 'z'))
        self.assertIsNone(re.match(r'(?:a?)*?y', 'z'))
        self.assertIsNone(re.match(r'(?:a?)+?y', 'z'))
        self.assertIsNone(re.match(r'(?:a?){2,}?y', 'z'))

    def test_scanner(self):
        def s_ident(scanner, token): return token
        def s_operator(scanner, token): return "op%s" % token
        def s_float(scanner, token): return float(token)
        def s_int(scanner, token): return int(token)

        scanner = Scanner([
            (r"[a-zA-Z_]\w*", s_ident),
            (r"\d+\.\d*", s_float),
            (r"\d+", s_int),
            (r"=|\+|-|\*|/", s_operator),
            (r"\s+", None),
            ])

        self.assertTrue(scanner.scanner.scanner("").pattern)

        self.assertEqual(scanner.scan("sum = 3*foo + 312.50 + bar"),
                         (['sum', 'op=', 3, 'op*', 'foo', 'op+', 312.5,
                           'op+', 'bar'], ''))

    def test_bug_448951(self):
        # bug 448951 (similar to 429357, but with single char match)
        # (Also test greedy matches.)
        for op in '','?','*':
            self.assertEqual(re.match(r'((.%s):)?z'%op, 'z').groups(),
                             (None, None))
            self.assertEqual(re.match(r'((.%s):)?z'%op, 'a:z').groups(),
                             ('a:', 'a'))

    def test_bug_725106(self):
        # capturing groups in alternatives in repeats
        self.assertEqual(re.match('^((a)|b)*', 'abc').groups(),
                         ('b', 'a'))
        self.assertEqual(re.match('^(([ab])|c)*', 'abc').groups(),
                         ('c', 'b'))
        self.assertEqual(re.match('^((d)|[ab])*', 'abc').groups(),
                         ('b', None))
        self.assertEqual(re.match('^((a)c|[ab])*', 'abc').groups(),
                         ('b', None))
        self.assertEqual(re.match('^((a)|b)*?c', 'abc').groups(),
                         ('b', 'a'))
        self.assertEqual(re.match('^(([ab])|c)*?d', 'abcd').groups(),
                         ('c', 'b'))
        self.assertEqual(re.match('^((d)|[ab])*?c', 'abc').groups(),
                         ('b', None))
        self.assertEqual(re.match('^((a)c|[ab])*?c', 'abc').groups(),
                         ('b', None))

    def test_bug_725149(self):
        # mark_stack_base restoring before restoring marks
        self.assertEqual(re.match('(a)(?:(?=(b)*)c)*', 'abb').groups(),
                         ('a', None))
        self.assertEqual(re.match('(a)((?!(b)*))*', 'abb').groups(),
                         ('a', None, None))

    def test_bug_764548(self):
        # bug 764548, re.compile() barfs on str/unicode subclasses
        class my_unicode(str): pass
        pat = re.compile(my_unicode("abc"))
        self.assertIsNone(pat.match("xyz"))

    def test_finditer(self):
        iter = re.finditer(r":+", "a:b::c:::d")
        self.assertEqual([item.group(0) for item in iter],
                         [":", "::", ":::"])

        pat = re.compile(r":+")
        iter = pat.finditer("a:b::c:::d", 1, 10)
        self.assertEqual([item.group(0) for item in iter],
                         [":", "::", ":::"])

        pat = re.compile(r":+")
        iter = pat.finditer("a:b::c:::d", pos=1, endpos=10)
        self.assertEqual([item.group(0) for item in iter],
                         [":", "::", ":::"])

        pat = re.compile(r":+")
        iter = pat.finditer("a:b::c:::d", endpos=10, pos=1)
        self.assertEqual([item.group(0) for item in iter],
                         [":", "::", ":::"])

        pat = re.compile(r":+")
        iter = pat.finditer("a:b::c:::d", pos=3, endpos=8)
        self.assertEqual([item.group(0) for item in iter],
                         ["::", "::"])

    def test_bug_926075(self):
        self.assertIsNot(re.compile('bug_926075'),
                         re.compile(b'bug_926075'))

    def test_bug_931848(self):
        pattern = "[\u002E\u3002\uFF0E\uFF61]"
        self.assertEqual(re.compile(pattern).split("a.b.c"),
                         ['a','b','c'])

    def test_bug_581080(self):
        iter = re.finditer(r"\s", "a b")
        self.assertEqual(next(iter).span(), (1,2))
        self.assertRaises(StopIteration, next, iter)

        scanner = re.compile(r"\s").scanner("a b")
        self.assertEqual(scanner.search().span(), (1, 2))
        self.assertIsNone(scanner.search())

    def test_bug_817234(self):
        iter = re.finditer(r".*", "asdf")
        self.assertEqual(next(iter).span(), (0, 4))
        self.assertEqual(next(iter).span(), (4, 4))
        self.assertRaises(StopIteration, next, iter)

    def test_bug_6561(self):
        # '\d' should match characters in Unicode category 'Nd'
        # (Number, Decimal Digit), but not those in 'Nl' (Number,
        # Letter) or 'No' (Number, Other).
        decimal_digits = [
            '\u0037', # '\N{DIGIT SEVEN}', category 'Nd'
            '\u0e58', # '\N{THAI DIGIT SIX}', category 'Nd'
            '\uff10', # '\N{FULLWIDTH DIGIT ZERO}', category 'Nd'
            ]
        for x in decimal_digits:
            self.assertEqual(re.match(r'^\d$', x).group(0), x)

        not_decimal_digits = [
            '\u2165', # '\N{ROMAN NUMERAL SIX}', category 'Nl'
            '\u3039', # '\N{HANGZHOU NUMERAL TWENTY}', category 'Nl'
            '\u2082', # '\N{SUBSCRIPT TWO}', category 'No'
            '\u32b4', # '\N{CIRCLED NUMBER THIRTY NINE}', category 'No'
            ]
        for x in not_decimal_digits:
            self.assertIsNone(re.match(r'^\d$', x))

    def test_empty_array(self):
        # SF buf 1647541
        import array
        for typecode in 'bBuhHiIlLfd':
            a = array.array(typecode)
            self.assertIsNone(re.compile(b"bla").match(a))
            self.assertEqual(re.compile(b"").match(a).groups(), ())

    def test_inline_flags(self):
        # Bug #1700
        upper_char = '\u1ea0' # Latin Capital Letter A with Dot Below
        lower_char = '\u1ea1' # Latin Small Letter A with Dot Below

        p = re.compile('.' + upper_char, re.I | re.S)
        q = p.match('\n' + lower_char)
        self.assertTrue(q)

        p = re.compile('.' + lower_char, re.I | re.S)
        q = p.match('\n' + upper_char)
        self.assertTrue(q)

        p = re.compile('(?i).' + upper_char, re.S)
        q = p.match('\n' + lower_char)
        self.assertTrue(q)

        p = re.compile('(?i).' + lower_char, re.S)
        q = p.match('\n' + upper_char)
        self.assertTrue(q)

        p = re.compile('(?is).' + upper_char)
        q = p.match('\n' + lower_char)
        self.assertTrue(q)

        p = re.compile('(?is).' + lower_char)
        q = p.match('\n' + upper_char)
        self.assertTrue(q)

        p = re.compile('(?s)(?i).' + upper_char)
        q = p.match('\n' + lower_char)
        self.assertTrue(q)

        p = re.compile('(?s)(?i).' + lower_char)
        q = p.match('\n' + upper_char)
        self.assertTrue(q)

        self.assertTrue(re.match('(?ix) ' + upper_char, lower_char))
        self.assertTrue(re.match('(?ix) ' + lower_char, upper_char))
        self.assertTrue(re.match(' (?i) ' + upper_char, lower_char, re.X))
        self.assertTrue(re.match('(?x) (?i) ' + upper_char, lower_char))
        self.assertTrue(re.match(' (?x) (?i) ' + upper_char, lower_char, re.X))

        p = upper_char + '(?i)'
        with self.assertWarns(DeprecationWarning) as warns:
            self.assertTrue(re.match(p, lower_char))
        self.assertEqual(
            str(warns.warnings[0].message),
            'Flags not at the start of the expression %r' % p
        )
        self.assertEqual(warns.warnings[0].filename, __file__)

        p = upper_char + '(?i)%s' % ('.?' * 100)
        with self.assertWarns(DeprecationWarning) as warns:
            self.assertTrue(re.match(p, lower_char))
        self.assertEqual(
            str(warns.warnings[0].message),
            'Flags not at the start of the expression %r (truncated)' % p[:20]
        )
        self.assertEqual(warns.warnings[0].filename, __file__)

        # bpo-30605: Compiling a bytes instance regex was throwing a BytesWarning
        with warnings.catch_warnings():
            warnings.simplefilter('error', BytesWarning)
            p = b'A(?i)'
            with self.assertWarns(DeprecationWarning) as warns:
                self.assertTrue(re.match(p, b'a'))
            self.assertEqual(
                str(warns.warnings[0].message),
                'Flags not at the start of the expression %r' % p
            )
            self.assertEqual(warns.warnings[0].filename, __file__)

        with self.assertWarns(DeprecationWarning):
            self.assertTrue(re.match('(?s).(?i)' + upper_char, '\n' + lower_char))
        with self.assertWarns(DeprecationWarning):
            self.assertTrue(re.match('(?i) ' + upper_char + ' (?x)', lower_char))
        with self.assertWarns(DeprecationWarning):
            self.assertTrue(re.match(' (?x) (?i) ' + upper_char, lower_char))
        with self.assertWarns(DeprecationWarning):
            self.assertTrue(re.match('^(?i)' + upper_char, lower_char))
        with self.assertWarns(DeprecationWarning):
            self.assertTrue(re.match('$|(?i)' + upper_char, lower_char))
        with self.assertWarns(DeprecationWarning) as warns:
            self.assertTrue(re.match('(?:(?i)' + upper_char + ')', lower_char))
        self.assertRegex(str(warns.warnings[0].message),
                         'Flags not at the start')
        self.assertEqual(warns.warnings[0].filename, __file__)
        with self.assertWarns(DeprecationWarning) as warns:
            self.assertTrue(re.fullmatch('(^)?(?(1)(?i)' + upper_char + ')',
                                         lower_char))
        self.assertRegex(str(warns.warnings[0].message),
                         'Flags not at the start')
        self.assertEqual(warns.warnings[0].filename, __file__)
        with self.assertWarns(DeprecationWarning) as warns:
            self.assertTrue(re.fullmatch('($)?(?(1)|(?i)' + upper_char + ')',
                                         lower_char))
        self.assertRegex(str(warns.warnings[0].message),
                         'Flags not at the start')
        self.assertEqual(warns.warnings[0].filename, __file__)


    def test_dollar_matches_twice(self):
        "$ matches the end of string, and just before the terminating \n"
        pattern = re.compile('$')
        self.assertEqual(pattern.sub('#', 'a\nb\n'), 'a\nb#\n#')
        self.assertEqual(pattern.sub('#', 'a\nb\nc'), 'a\nb\nc#')
        self.assertEqual(pattern.sub('#', '\n'), '#\n#')

        pattern = re.compile('$', re.MULTILINE)
        self.assertEqual(pattern.sub('#', 'a\nb\n' ), 'a#\nb#\n#' )
        self.assertEqual(pattern.sub('#', 'a\nb\nc'), 'a#\nb#\nc#')
        self.assertEqual(pattern.sub('#', '\n'), '#\n#')

    def test_bytes_str_mixing(self):
        # Mixing str and bytes is disallowed
        pat = re.compile('.')
        bpat = re.compile(b'.')
        self.assertRaises(TypeError, pat.match, b'b')
        self.assertRaises(TypeError, bpat.match, 'b')
        self.assertRaises(TypeError, pat.sub, b'b', 'c')
        self.assertRaises(TypeError, pat.sub, 'b', b'c')
        self.assertRaises(TypeError, pat.sub, b'b', b'c')
        self.assertRaises(TypeError, bpat.sub, b'b', 'c')
        self.assertRaises(TypeError, bpat.sub, 'b', b'c')
        self.assertRaises(TypeError, bpat.sub, 'b', 'c')

    def test_ascii_and_unicode_flag(self):
        # String patterns
        for flags in (0, re.UNICODE):
            pat = re.compile('\xc0', flags | re.IGNORECASE)
            self.assertTrue(pat.match('\xe0'))
            pat = re.compile(r'\w', flags)
            self.assertTrue(pat.match('\xe0'))
        pat = re.compile('\xc0', re.ASCII | re.IGNORECASE)
        self.assertIsNone(pat.match('\xe0'))
        pat = re.compile('(?a)\xc0', re.IGNORECASE)
        self.assertIsNone(pat.match('\xe0'))
        pat = re.compile(r'\w', re.ASCII)
        self.assertIsNone(pat.match('\xe0'))
        pat = re.compile(r'(?a)\w')
        self.assertIsNone(pat.match('\xe0'))
        # Bytes patterns
        for flags in (0, re.ASCII):
            pat = re.compile(b'\xc0', flags | re.IGNORECASE)
            self.assertIsNone(pat.match(b'\xe0'))
            pat = re.compile(br'\w', flags)
            self.assertIsNone(pat.match(b'\xe0'))
        # Incompatibilities
        self.assertRaises(ValueError, re.compile, br'\w', re.UNICODE)
        self.assertRaises(re.error, re.compile, br'(?u)\w')
        self.assertRaises(ValueError, re.compile, r'\w', re.UNICODE | re.ASCII)
        self.assertRaises(ValueError, re.compile, r'(?u)\w', re.ASCII)
        self.assertRaises(ValueError, re.compile, r'(?a)\w', re.UNICODE)
        self.assertRaises(re.error, re.compile, r'(?au)\w')

    def test_locale_flag(self):
        enc = locale.getpreferredencoding()
        # Search non-ASCII letter
        for i in range(128, 256):
            try:
                c = bytes([i]).decode(enc)
                sletter = c.lower()
                if sletter == c: continue
                bletter = sletter.encode(enc)
                if len(bletter) != 1: continue
                if bletter.decode(enc) != sletter: continue
                bpat = re.escape(bytes([i]))
                break
            except (UnicodeError, TypeError):
                pass
        else:
            bletter = None
            bpat = b'A'
        # Bytes patterns
        pat = re.compile(bpat, re.LOCALE | re.IGNORECASE)
        if bletter:
            self.assertTrue(pat.match(bletter))
        pat = re.compile(b'(?L)' + bpat, re.IGNORECASE)
        if bletter:
            self.assertTrue(pat.match(bletter))
        pat = re.compile(bpat, re.IGNORECASE)
        if bletter:
            self.assertIsNone(pat.match(bletter))
        pat = re.compile(br'\w', re.LOCALE)
        if bletter:
            self.assertTrue(pat.match(bletter))
        pat = re.compile(br'(?L)\w')
        if bletter:
            self.assertTrue(pat.match(bletter))
        pat = re.compile(br'\w')
        if bletter:
            self.assertIsNone(pat.match(bletter))
        # Incompatibilities
        self.assertRaises(ValueError, re.compile, '', re.LOCALE)
        self.assertRaises(re.error, re.compile, '(?L)')
        self.assertRaises(ValueError, re.compile, b'', re.LOCALE | re.ASCII)
        self.assertRaises(ValueError, re.compile, b'(?L)', re.ASCII)
        self.assertRaises(ValueError, re.compile, b'(?a)', re.LOCALE)
        self.assertRaises(re.error, re.compile, b'(?aL)')

    def test_scoped_flags(self):
        self.assertTrue(re.match(r'(?i:a)b', 'Ab'))
        self.assertIsNone(re.match(r'(?i:a)b', 'aB'))
        self.assertIsNone(re.match(r'(?-i:a)b', 'Ab', re.IGNORECASE))
        self.assertTrue(re.match(r'(?-i:a)b', 'aB', re.IGNORECASE))
        self.assertIsNone(re.match(r'(?i:(?-i:a)b)', 'Ab'))
        self.assertTrue(re.match(r'(?i:(?-i:a)b)', 'aB'))

        self.assertTrue(re.match(r'(?x: a) b', 'a b'))
        self.assertIsNone(re.match(r'(?x: a) b', ' a b'))
        self.assertTrue(re.match(r'(?-x: a) b', ' ab', re.VERBOSE))
        self.assertIsNone(re.match(r'(?-x: a) b', 'ab', re.VERBOSE))

        self.assertTrue(re.match(r'\w(?a:\W)\w', '\xe0\xe0\xe0'))
        self.assertTrue(re.match(r'(?a:\W(?u:\w)\W)', '\xe0\xe0\xe0'))
        self.assertTrue(re.match(r'\W(?u:\w)\W', '\xe0\xe0\xe0', re.ASCII))

        self.checkPatternError(r'(?a)(?-a:\w)',
                "bad inline flags: cannot turn off flags 'a', 'u' and 'L'", 8)
        self.checkPatternError(r'(?i-i:a)',
                'bad inline flags: flag turned on and off', 5)
        self.checkPatternError(r'(?au:a)',
                "bad inline flags: flags 'a', 'u' and 'L' are incompatible", 4)
        self.checkPatternError(br'(?aL:a)',
                "bad inline flags: flags 'a', 'u' and 'L' are incompatible", 4)

        self.checkPatternError(r'(?-', 'missing flag', 3)
        self.checkPatternError(r'(?-+', 'missing flag', 3)
        self.checkPatternError(r'(?-z', 'unknown flag', 3)
        self.checkPatternError(r'(?-i', 'missing :', 4)
        self.checkPatternError(r'(?-i)', 'missing :', 4)
        self.checkPatternError(r'(?-i+', 'missing :', 4)
        self.checkPatternError(r'(?-iz', 'unknown flag', 4)
        self.checkPatternError(r'(?i:', 'missing ), unterminated subpattern', 0)
        self.checkPatternError(r'(?i', 'missing -, : or )', 3)
        self.checkPatternError(r'(?i+', 'missing -, : or )', 3)
        self.checkPatternError(r'(?iz', 'unknown flag', 3)

    def test_bug_6509(self):
        # Replacement strings of both types must parse properly.
        # all strings
        pat = re.compile(r'a(\w)')
        self.assertEqual(pat.sub('b\\1', 'ac'), 'bc')
        pat = re.compile('a(.)')
        self.assertEqual(pat.sub('b\\1', 'a\u1234'), 'b\u1234')
        pat = re.compile('..')
        self.assertEqual(pat.sub(lambda m: 'str', 'a5'), 'str')

        # all bytes
        pat = re.compile(br'a(\w)')
        self.assertEqual(pat.sub(b'b\\1', b'ac'), b'bc')
        pat = re.compile(b'a(.)')
        self.assertEqual(pat.sub(b'b\\1', b'a\xCD'), b'b\xCD')
        pat = re.compile(b'..')
        self.assertEqual(pat.sub(lambda m: b'bytes', b'a5'), b'bytes')

    def test_dealloc(self):
        # issue 3299: check for segfault in debug build
        import _sre
        # the overflow limit is different on wide and narrow builds and it
        # depends on the definition of SRE_CODE (see sre.h).
        # 2**128 should be big enough to overflow on both. For smaller values
        # a RuntimeError is raised instead of OverflowError.
        long_overflow = 2**128
        self.assertRaises(TypeError, re.finditer, "a", {})
        with self.assertRaises(OverflowError):
            _sre.compile("abc", 0, [long_overflow], 0, {}, ())
        with self.assertRaises(TypeError):
            _sre.compile({}, 0, [], 0, [], [])

    def test_search_dot_unicode(self):
        self.assertTrue(re.search("123.*-", '123abc-'))
        self.assertTrue(re.search("123.*-", '123\xe9-'))
        self.assertTrue(re.search("123.*-", '123\u20ac-'))
        self.assertTrue(re.search("123.*-", '123\U0010ffff-'))
        self.assertTrue(re.search("123.*-", '123\xe9\u20ac\U0010ffff-'))

    def test_compile(self):
        # Test return value when given string and pattern as parameter
        pattern = re.compile('random pattern')
        self.assertIsInstance(pattern, re.Pattern)
        same_pattern = re.compile(pattern)
        self.assertIsInstance(same_pattern, re.Pattern)
        self.assertIs(same_pattern, pattern)
        # Test behaviour when not given a string or pattern as parameter
        self.assertRaises(TypeError, re.compile, 0)

    @bigmemtest(size=_2G, memuse=1)
    def test_large_search(self, size):
        # Issue #10182: indices were 32-bit-truncated.
        s = 'a' * size
        m = re.search('$', s)
        self.assertIsNotNone(m)
        self.assertEqual(m.start(), size)
        self.assertEqual(m.end(), size)

    # The huge memuse is because of re.sub() using a list and a join()
    # to create the replacement result.
    @bigmemtest(size=_2G, memuse=16 + 2)
    def test_large_subn(self, size):
        # Issue #10182: indices were 32-bit-truncated.
        s = 'a' * size
        r, n = re.subn('', '', s)
        self.assertEqual(r, s)
        self.assertEqual(n, size + 1)

    def test_bug_16688(self):
        # Issue 16688: Backreferences make case-insensitive regex fail on
        # non-ASCII strings.
        self.assertEqual(re.findall(r"(?i)(a)\1", "aa \u0100"), ['a'])
        self.assertEqual(re.match(r"(?s).{1,3}", "\u0100\u0100").span(), (0, 2))

    def test_repeat_minmax_overflow(self):
        # Issue #13169
        string = "x" * 100000
        self.assertEqual(re.match(r".{65535}", string).span(), (0, 65535))
        self.assertEqual(re.match(r".{,65535}", string).span(), (0, 65535))
        self.assertEqual(re.match(r".{65535,}?", string).span(), (0, 65535))
        self.assertEqual(re.match(r".{65536}", string).span(), (0, 65536))
        self.assertEqual(re.match(r".{,65536}", string).span(), (0, 65536))
        self.assertEqual(re.match(r".{65536,}?", string).span(), (0, 65536))
        # 2**128 should be big enough to overflow both SRE_CODE and Py_ssize_t.
        self.assertRaises(OverflowError, re.compile, r".{%d}" % 2**128)
        self.assertRaises(OverflowError, re.compile, r".{,%d}" % 2**128)
        self.assertRaises(OverflowError, re.compile, r".{%d,}?" % 2**128)
        self.assertRaises(OverflowError, re.compile, r".{%d,%d}" % (2**129, 2**128))

    @cpython_only
    def test_repeat_minmax_overflow_maxrepeat(self):
        try:
            from _sre import MAXREPEAT
        except ImportError:
            self.skipTest('requires _sre.MAXREPEAT constant')
        string = "x" * 100000
        self.assertIsNone(re.match(r".{%d}" % (MAXREPEAT - 1), string))
        self.assertEqual(re.match(r".{,%d}" % (MAXREPEAT - 1), string).span(),
                         (0, 100000))
        self.assertIsNone(re.match(r".{%d,}?" % (MAXREPEAT - 1), string))
        self.assertRaises(OverflowError, re.compile, r".{%d}" % MAXREPEAT)
        self.assertRaises(OverflowError, re.compile, r".{,%d}" % MAXREPEAT)
        self.assertRaises(OverflowError, re.compile, r".{%d,}?" % MAXREPEAT)

    def test_backref_group_name_in_exception(self):
        # Issue 17341: Poor error message when compiling invalid regex
        self.checkPatternError('(?P=<foo>)',
                               "bad character in group name '<foo>'", 4)

    def test_group_name_in_exception(self):
        # Issue 17341: Poor error message when compiling invalid regex
        self.checkPatternError('(?P<?foo>)',
                               "bad character in group name '?foo'", 4)

    def test_issue17998(self):
        for reps in '*', '+', '?', '{1}':
            for mod in '', '?':
                pattern = '.' + reps + mod + 'yz'
                self.assertEqual(re.compile(pattern, re.S).findall('xyz'),
                                 ['xyz'], msg=pattern)
                pattern = pattern.encode()
                self.assertEqual(re.compile(pattern, re.S).findall(b'xyz'),
                                 [b'xyz'], msg=pattern)

    def test_match_repr(self):
        for string in '[abracadabra]', S('[abracadabra]'):
            m = re.search(r'(.+)(.*?)\1', string)
            pattern = r"<(%s\.)?%s object; span=\(1, 12\), match='abracadabra'>" % (
                type(m).__module__, type(m).__qualname__
            )
            self.assertRegex(repr(m), pattern)
        for string in (b'[abracadabra]', B(b'[abracadabra]'),
                       bytearray(b'[abracadabra]'),
                       memoryview(b'[abracadabra]')):
            m = re.search(br'(.+)(.*?)\1', string)
            pattern = r"<(%s\.)?%s object; span=\(1, 12\), match=b'abracadabra'>" % (
                type(m).__module__, type(m).__qualname__
            )
            self.assertRegex(repr(m), pattern)

        first, second = list(re.finditer("(aa)|(bb)", "aa bb"))
        pattern = r"<(%s\.)?%s object; span=\(0, 2\), match='aa'>" % (
            type(second).__module__, type(second).__qualname__
        )
        self.assertRegex(repr(first), pattern)
        pattern = r"<(%s\.)?%s object; span=\(3, 5\), match='bb'>" % (
            type(second).__module__, type(second).__qualname__
        )
        self.assertRegex(repr(second), pattern)

    def test_zerowidth(self):
        # Issues 852532, 1647489, 3262, 25054.
        self.assertEqual(re.split(r"\b", "a::bc"), ['', 'a', '::', 'bc', ''])
        self.assertEqual(re.split(r"\b|:+", "a::bc"), ['', 'a', '', '', 'bc', ''])
        self.assertEqual(re.split(r"(?<!\w)(?=\w)|:+", "a::bc"), ['', 'a', '', 'bc'])
        self.assertEqual(re.split(r"(?<=\w)(?!\w)|:+", "a::bc"), ['a', '', 'bc', ''])

        self.assertEqual(re.sub(r"\b", "-", "a::bc"), '-a-::-bc-')
        self.assertEqual(re.sub(r"\b|:+", "-", "a::bc"), '-a---bc-')
        self.assertEqual(re.sub(r"(\b|:+)", r"[\1]", "a::bc"), '[]a[][::][]bc[]')

        self.assertEqual(re.findall(r"\b|:+", "a::bc"), ['', '', '::', '', ''])
        self.assertEqual(re.findall(r"\b|\w+", "a::bc"),
                         ['', 'a', '', '', 'bc', ''])

        self.assertEqual([m.span() for m in re.finditer(r"\b|:+", "a::bc")],
                         [(0, 0), (1, 1), (1, 3), (3, 3), (5, 5)])
        self.assertEqual([m.span() for m in re.finditer(r"\b|\w+", "a::bc")],
                         [(0, 0), (0, 1), (1, 1), (3, 3), (3, 5), (5, 5)])

    def test_bug_2537(self):
        # issue 2537: empty submatches
        for outer_op in ('{0,}', '*', '+', '{1,187}'):
            for inner_op in ('{0,}', '*', '?'):
                r = re.compile("^((x|y)%s)%s" % (inner_op, outer_op))
                m = r.match("xyyzy")
                self.assertEqual(m.group(0), "xyy")
                self.assertEqual(m.group(1), "")
                self.assertEqual(m.group(2), "y")

    @cpython_only
    def test_debug_flag(self):
        pat = r'(\.)(?:[ch]|py)(?(1)$|: )'
        with captured_stdout() as out:
            re.compile(pat, re.DEBUG)
        self.maxDiff = None
        dump = '''\
SUBPATTERN 1 0 0
  LITERAL 46
BRANCH
  IN
    LITERAL 99
    LITERAL 104
OR
  LITERAL 112
  LITERAL 121
GROUPREF_EXISTS 1
  AT AT_END
ELSE
  LITERAL 58
  LITERAL 32

 0. INFO 8 0b1 2 5 (to 9)
      prefix_skip 0
      prefix [0x2e] ('.')
      overlap [0]
 9: MARK 0
11. LITERAL 0x2e ('.')
13. MARK 1
15. BRANCH 10 (to 26)
17.   IN 6 (to 24)
19.     LITERAL 0x63 ('c')
21.     LITERAL 0x68 ('h')
23.     FAILURE
24:   JUMP 9 (to 34)
26: branch 7 (to 33)
27.   LITERAL 0x70 ('p')
29.   LITERAL 0x79 ('y')
31.   JUMP 2 (to 34)
33: FAILURE
34: GROUPREF_EXISTS 0 6 (to 41)
37. AT END
39. JUMP 5 (to 45)
41: LITERAL 0x3a (':')
43. LITERAL 0x20 (' ')
45: SUCCESS
'''
        self.assertEqual(out.getvalue(), dump)
        # Debug output is output again even a second time (bypassing
        # the cache -- issue #20426).
        with captured_stdout() as out:
            re.compile(pat, re.DEBUG)
        self.assertEqual(out.getvalue(), dump)

    def test_keyword_parameters(self):
        # Issue #20283: Accepting the string keyword parameter.
        pat = re.compile(r'(ab)')
        self.assertEqual(
            pat.match(string='abracadabra', pos=7, endpos=10).span(), (7, 9))
        self.assertEqual(
            pat.fullmatch(string='abracadabra', pos=7, endpos=9).span(), (7, 9))
        self.assertEqual(
            pat.search(string='abracadabra', pos=3, endpos=10).span(), (7, 9))
        self.assertEqual(
            pat.findall(string='abracadabra', pos=3, endpos=10), ['ab'])
        self.assertEqual(
            pat.split(string='abracadabra', maxsplit=1),
            ['', 'ab', 'racadabra'])
        self.assertEqual(
            pat.scanner(string='abracadabra', pos=3, endpos=10).search().span(),
            (7, 9))

    def test_bug_20998(self):
        # Issue #20998: Fullmatch of repeated single character pattern
        # with ignore case.
        self.assertEqual(re.fullmatch('[a-c]+', 'ABC', re.I).span(), (0, 3))

    def test_locale_caching(self):
        # Issue #22410
        oldlocale = locale.setlocale(locale.LC_CTYPE)
        self.addCleanup(locale.setlocale, locale.LC_CTYPE, oldlocale)
        for loc in 'en_US.iso88591', 'en_US.utf8':
            try:
                locale.setlocale(locale.LC_CTYPE, loc)
            except locale.Error:
                # Unsupported locale on this system
                self.skipTest('test needs %s locale' % loc)

        re.purge()
        self.check_en_US_iso88591()
        self.check_en_US_utf8()
        re.purge()
        self.check_en_US_utf8()
        self.check_en_US_iso88591()

    def check_en_US_iso88591(self):
        locale.setlocale(locale.LC_CTYPE, 'en_US.iso88591')
        self.assertTrue(re.match(b'\xc5\xe5', b'\xc5\xe5', re.L|re.I))
        self.assertTrue(re.match(b'\xc5', b'\xe5', re.L|re.I))
        self.assertTrue(re.match(b'\xe5', b'\xc5', re.L|re.I))
        self.assertTrue(re.match(b'(?Li)\xc5\xe5', b'\xc5\xe5'))
        self.assertTrue(re.match(b'(?Li)\xc5', b'\xe5'))
        self.assertTrue(re.match(b'(?Li)\xe5', b'\xc5'))

    def check_en_US_utf8(self):
        locale.setlocale(locale.LC_CTYPE, 'en_US.utf8')
        self.assertTrue(re.match(b'\xc5\xe5', b'\xc5\xe5', re.L|re.I))
        self.assertIsNone(re.match(b'\xc5', b'\xe5', re.L|re.I))
        self.assertIsNone(re.match(b'\xe5', b'\xc5', re.L|re.I))
        self.assertTrue(re.match(b'(?Li)\xc5\xe5', b'\xc5\xe5'))
        self.assertIsNone(re.match(b'(?Li)\xc5', b'\xe5'))
        self.assertIsNone(re.match(b'(?Li)\xe5', b'\xc5'))

    def test_locale_compiled(self):
        oldlocale = locale.setlocale(locale.LC_CTYPE)
        self.addCleanup(locale.setlocale, locale.LC_CTYPE, oldlocale)
        for loc in 'en_US.iso88591', 'en_US.utf8':
            try:
                locale.setlocale(locale.LC_CTYPE, loc)
            except locale.Error:
                # Unsupported locale on this system
                self.skipTest('test needs %s locale' % loc)

        locale.setlocale(locale.LC_CTYPE, 'en_US.iso88591')
        p1 = re.compile(b'\xc5\xe5', re.L|re.I)
        p2 = re.compile(b'[a\xc5][a\xe5]', re.L|re.I)
        p3 = re.compile(b'[az\xc5][az\xe5]', re.L|re.I)
        p4 = re.compile(b'[^\xc5][^\xe5]', re.L|re.I)
        for p in p1, p2, p3:
            self.assertTrue(p.match(b'\xc5\xe5'))
            self.assertTrue(p.match(b'\xe5\xe5'))
            self.assertTrue(p.match(b'\xc5\xc5'))
        self.assertIsNone(p4.match(b'\xe5\xc5'))
        self.assertIsNone(p4.match(b'\xe5\xe5'))
        self.assertIsNone(p4.match(b'\xc5\xc5'))

        locale.setlocale(locale.LC_CTYPE, 'en_US.utf8')
        for p in p1, p2, p3:
            self.assertTrue(p.match(b'\xc5\xe5'))
            self.assertIsNone(p.match(b'\xe5\xe5'))
            self.assertIsNone(p.match(b'\xc5\xc5'))
        self.assertTrue(p4.match(b'\xe5\xc5'))
        self.assertIsNone(p4.match(b'\xe5\xe5'))
        self.assertIsNone(p4.match(b'\xc5\xc5'))

    def test_error(self):
        with self.assertRaises(re.error) as cm:
            re.compile('(\u20ac))')
        err = cm.exception
        self.assertIsInstance(err.pattern, str)
        self.assertEqual(err.pattern, '(\u20ac))')
        self.assertEqual(err.pos, 3)
        self.assertEqual(err.lineno, 1)
        self.assertEqual(err.colno, 4)
        self.assertIn(err.msg, str(err))
        self.assertIn(' at position 3', str(err))
        self.assertNotIn(' at position 3', err.msg)
        # Bytes pattern
        with self.assertRaises(re.error) as cm:
            re.compile(b'(\xa4))')
        err = cm.exception
        self.assertIsInstance(err.pattern, bytes)
        self.assertEqual(err.pattern, b'(\xa4))')
        self.assertEqual(err.pos, 3)
        # Multiline pattern
        with self.assertRaises(re.error) as cm:
            re.compile("""
                (
                    abc
                )
                )
                (
                """, re.VERBOSE)
        err = cm.exception
        self.assertEqual(err.pos, 77)
        self.assertEqual(err.lineno, 5)
        self.assertEqual(err.colno, 17)
        self.assertIn(err.msg, str(err))
        self.assertIn(' at position 77', str(err))
        self.assertIn('(line 5, column 17)', str(err))

    def test_misc_errors(self):
        self.checkPatternError(r'(', 'missing ), unterminated subpattern', 0)
        self.checkPatternError(r'((a|b)', 'missing ), unterminated subpattern', 0)
        self.checkPatternError(r'(a|b))', 'unbalanced parenthesis', 5)
        self.checkPatternError(r'(?P', 'unexpected end of pattern', 3)
        self.checkPatternError(r'(?z)', 'unknown extension ?z', 1)
        self.checkPatternError(r'(?iz)', 'unknown flag', 3)
        self.checkPatternError(r'(?i', 'missing -, : or )', 3)
        self.checkPatternError(r'(?#abc', 'missing ), unterminated comment', 0)
        self.checkPatternError(r'(?<', 'unexpected end of pattern', 3)
        self.checkPatternError(r'(?<>)', 'unknown extension ?<>', 1)
        self.checkPatternError(r'(?', 'unexpected end of pattern', 2)

    def test_enum(self):
        # Issue #28082: Check that str(flag) returns a human readable string
        # instead of an integer
        self.assertIn('ASCII', str(re.A))
        self.assertIn('DOTALL', str(re.S))

    def test_pattern_compare(self):
        pattern1 = re.compile('abc', re.IGNORECASE)

        # equal to itself
        self.assertEqual(pattern1, pattern1)
        self.assertFalse(pattern1 != pattern1)

        # equal
        re.purge()
        pattern2 = re.compile('abc', re.IGNORECASE)
        self.assertEqual(hash(pattern2), hash(pattern1))
        self.assertEqual(pattern2, pattern1)

        # not equal: different pattern
        re.purge()
        pattern3 = re.compile('XYZ', re.IGNORECASE)
        # Don't test hash(pattern3) != hash(pattern1) because there is no
        # warranty that hash values are different
        self.assertNotEqual(pattern3, pattern1)

        # not equal: different flag (flags=0)
        re.purge()
        pattern4 = re.compile('abc')
        self.assertNotEqual(pattern4, pattern1)

        # only == and != comparison operators are supported
        with self.assertRaises(TypeError):
            pattern1 < pattern2

    def test_pattern_compare_bytes(self):
        pattern1 = re.compile(b'abc')

        # equal: test bytes patterns
        re.purge()
        pattern2 = re.compile(b'abc')
        self.assertEqual(hash(pattern2), hash(pattern1))
        self.assertEqual(pattern2, pattern1)

        # not equal: pattern of a different types (str vs bytes),
        # comparison must not raise a BytesWarning
        re.purge()
        pattern3 = re.compile('abc')
        with warnings.catch_warnings():
            warnings.simplefilter('error', BytesWarning)
            self.assertNotEqual(pattern3, pattern1)

    def test_bug_29444(self):
        s = bytearray(b'abcdefgh')
        m = re.search(b'[a-h]+', s)
        m2 = re.search(b'[e-h]+', s)
        self.assertEqual(m.group(), b'abcdefgh')
        self.assertEqual(m2.group(), b'efgh')
        s[:] = b'xyz'
        self.assertEqual(m.group(), b'xyz')
        self.assertEqual(m2.group(), b'')

    def test_bug_34294(self):
        # Issue 34294: wrong capturing groups

        # exists since Python 2
        s = "a\tx"
        p = r"\b(?=(\t)|(x))x"
        self.assertEqual(re.search(p, s).groups(), (None, 'x'))

        # introduced in Python 3.7.0
        s = "ab"
        p = r"(?=(.)(.)?)"
        self.assertEqual(re.findall(p, s),
                         [('a', 'b'), ('b', '')])
        self.assertEqual([m.groups() for m in re.finditer(p, s)],
                         [('a', 'b'), ('b', None)])

        # test-cases provided by issue34294, introduced in Python 3.7.0
        p = r"(?=<(?P<tag>\w+)/?>(?:(?P<text>.+?)</(?P=tag)>)?)"
        s = "<test><foo2/></test>"
        self.assertEqual(re.findall(p, s),
                         [('test', '<foo2/>'), ('foo2', '')])
        self.assertEqual([m.groupdict() for m in re.finditer(p, s)],
                         [{'tag': 'test', 'text': '<foo2/>'},
                          {'tag': 'foo2', 'text': None}])
        s = "<test>Hello</test><foo/>"
        self.assertEqual([m.groupdict() for m in re.finditer(p, s)],
                         [{'tag': 'test', 'text': 'Hello'},
                          {'tag': 'foo', 'text': None}])
        s = "<test>Hello</test><foo/><foo/>"
        self.assertEqual([m.groupdict() for m in re.finditer(p, s)],
                         [{'tag': 'test', 'text': 'Hello'},
                          {'tag': 'foo', 'text': None},
                          {'tag': 'foo', 'text': None}])


class PatternReprTests(unittest.TestCase):
    def check(self, pattern, expected):
        self.assertEqual(repr(re.compile(pattern)), expected)

    def check_flags(self, pattern, flags, expected):
        self.assertEqual(repr(re.compile(pattern, flags)), expected)

    def test_without_flags(self):
        self.check('random pattern',
                   "re.compile('random pattern')")

    def test_single_flag(self):
        self.check_flags('random pattern', re.IGNORECASE,
            "re.compile('random pattern', re.IGNORECASE)")

    def test_multiple_flags(self):
        self.check_flags('random pattern', re.I|re.S|re.X,
            "re.compile('random pattern', "
            "re.IGNORECASE|re.DOTALL|re.VERBOSE)")

    def test_unicode_flag(self):
        self.check_flags('random pattern', re.U,
                         "re.compile('random pattern')")
        self.check_flags('random pattern', re.I|re.S|re.U,
                         "re.compile('random pattern', "
                         "re.IGNORECASE|re.DOTALL)")

    def test_inline_flags(self):
        self.check('(?i)pattern',
                   "re.compile('(?i)pattern', re.IGNORECASE)")

    def test_unknown_flags(self):
        self.check_flags('random pattern', 0x123000,
                         "re.compile('random pattern', 0x123000)")
        self.check_flags('random pattern', 0x123000|re.I,
            "re.compile('random pattern', re.IGNORECASE|0x123000)")

    def test_bytes(self):
        self.check(b'bytes pattern',
                   "re.compile(b'bytes pattern')")
        self.check_flags(b'bytes pattern', re.A,
                         "re.compile(b'bytes pattern', re.ASCII)")

    def test_locale(self):
        self.check_flags(b'bytes pattern', re.L,
                         "re.compile(b'bytes pattern', re.LOCALE)")

    def test_quotes(self):
        self.check('random "double quoted" pattern',
            '''re.compile('random "double quoted" pattern')''')
        self.check("random 'single quoted' pattern",
            '''re.compile("random 'single quoted' pattern")''')
        self.check('''both 'single' and "double" quotes''',
            '''re.compile('both \\'single\\' and "double" quotes')''')

    def test_long_pattern(self):
        pattern = 'Very %spattern' % ('long ' * 1000)
        r = repr(re.compile(pattern))
        self.assertLess(len(r), 300)
        self.assertEqual(r[:30], "re.compile('Very long long lon")
        r = repr(re.compile(pattern, re.I))
        self.assertLess(len(r), 300)
        self.assertEqual(r[:30], "re.compile('Very long long lon")
        self.assertEqual(r[-16:], ", re.IGNORECASE)")

    def test_flags_repr(self):
        self.assertEqual(repr(re.I), "re.IGNORECASE")
        self.assertEqual(repr(re.I|re.S|re.X),
                         "re.IGNORECASE|re.DOTALL|re.VERBOSE")
        self.assertEqual(repr(re.I|re.S|re.X|(1<<20)),
                         "re.IGNORECASE|re.DOTALL|re.VERBOSE|0x100000")
        self.assertEqual(
                repr(~re.I),
                "re.ASCII|re.LOCALE|re.UNICODE|re.MULTILINE|re.DOTALL|re.VERBOSE|re.TEMPLATE|re.DEBUG")
        self.assertEqual(repr(~(re.I|re.S|re.X)),
                         "re.ASCII|re.LOCALE|re.UNICODE|re.MULTILINE|re.TEMPLATE|re.DEBUG")
        self.assertEqual(repr(~(re.I|re.S|re.X|(1<<20))),
                         "re.ASCII|re.LOCALE|re.UNICODE|re.MULTILINE|re.TEMPLATE|re.DEBUG|0xffe00")


class ImplementationTest(unittest.TestCase):
    """
    Test implementation details of the re module.
    """

    @cpython_only
    def test_immutable(self):
        # bpo-43908: check that re types are immutable
        with self.assertRaises(TypeError):
            re.Match.foo = 1
        with self.assertRaises(TypeError):
            re.Pattern.foo = 1
        with self.assertRaises(TypeError):
            pat = re.compile("")
            tp = type(pat.scanner(""))
            tp.foo = 1

    def test_overlap_table(self):
        f = sre_compile._generate_overlap_table
        self.assertEqual(f(""), [])
        self.assertEqual(f("a"), [0])
        self.assertEqual(f("abcd"), [0, 0, 0, 0])
        self.assertEqual(f("aaaa"), [0, 1, 2, 3])
        self.assertEqual(f("ababba"), [0, 0, 1, 2, 0, 1])
        self.assertEqual(f("abcabdac"), [0, 0, 0, 1, 2, 0, 1, 0])

    def test_signedness(self):
        self.assertGreaterEqual(sre_compile.MAXREPEAT, 0)
        self.assertGreaterEqual(sre_compile.MAXGROUPS, 0)

    @cpython_only
    def test_disallow_instantiation(self):
        # Ensure that the type disallows instantiation (bpo-43916)
        self.assertRaises(TypeError, re.Match)
        self.assertRaises(TypeError, re.Pattern)
        pat = re.compile("")
        tp = type(pat.scanner(""))
        self.assertRaises(TypeError, tp)


class ExternalTests(unittest.TestCase):

    def test_re_benchmarks(self):
        're_tests benchmarks'
        from test.re_tests import benchmarks
        for pattern, s in benchmarks:
            with self.subTest(pattern=pattern, string=s):
                p = re.compile(pattern)
                self.assertTrue(p.search(s))
                self.assertTrue(p.match(s))
                self.assertTrue(p.fullmatch(s))
                s2 = ' '*10000 + s + ' '*10000
                self.assertTrue(p.search(s2))
                self.assertTrue(p.match(s2, 10000))
                self.assertTrue(p.match(s2, 10000, 10000 + len(s)))
                self.assertTrue(p.fullmatch(s2, 10000, 10000 + len(s)))

    def test_re_tests(self):
        're_tests test suite'
        from test.re_tests import tests, FAIL, SYNTAX_ERROR
        for t in tests:
            pattern = s = outcome = repl = expected = None
            if len(t) == 5:
                pattern, s, outcome, repl, expected = t
            elif len(t) == 3:
                pattern, s, outcome = t
            else:
                raise ValueError('Test tuples should have 3 or 5 fields', t)

            with self.subTest(pattern=pattern, string=s):
                if outcome == SYNTAX_ERROR:  # Expected a syntax error
                    with self.assertRaises(re.error):
                        re.compile(pattern)
                    continue

                obj = re.compile(pattern)
                result = obj.search(s)
                if outcome == FAIL:
                    self.assertIsNone(result, 'Succeeded incorrectly')
                    continue

                with self.subTest():
                    self.assertTrue(result, 'Failed incorrectly')
                    # Matched, as expected, so now we compute the
                    # result string and compare it to our expected result.
                    start, end = result.span(0)
                    vardict = {'found': result.group(0),
                               'groups': result.group(),
                               'flags': result.re.flags}
                    for i in range(1, 100):
                        try:
                            gi = result.group(i)
                            # Special hack because else the string concat fails:
                            if gi is None:
                                gi = "None"
                        except IndexError:
                            gi = "Error"
                        vardict['g%d' % i] = gi
                    for i in result.re.groupindex.keys():
                        try:
                            gi = result.group(i)
                            if gi is None:
                                gi = "None"
                        except IndexError:
                            gi = "Error"
                        vardict[i] = gi
                    self.assertEqual(eval(repl, vardict), expected,
                                     'grouping error')

                # Try the match with both pattern and string converted to
                # bytes, and check that it still succeeds.
                try:
                    bpat = bytes(pattern, "ascii")
                    bs = bytes(s, "ascii")
                except UnicodeEncodeError:
                    # skip non-ascii tests
                    pass
                else:
                    with self.subTest('bytes pattern match'):
                        obj = re.compile(bpat)
                        self.assertTrue(obj.search(bs))

                    # Try the match with LOCALE enabled, and check that it
                    # still succeeds.
                    with self.subTest('locale-sensitive match'):
                        obj = re.compile(bpat, re.LOCALE)
                        result = obj.search(bs)
                        if result is None:
                            print('=== Fails on locale-sensitive match', t)

                # Try the match with the search area limited to the extent
                # of the match and see if it still succeeds.  \B will
                # break (because it won't match at the end or start of a
                # string), so we'll ignore patterns that feature it.
                if (pattern[:2] != r'\B' and pattern[-2:] != r'\B'
                            and result is not None):
                    with self.subTest('range-limited match'):
                        obj = re.compile(pattern)
                        self.assertTrue(obj.search(s, start, end + 1))

                # Try the match with IGNORECASE enabled, and check that it
                # still succeeds.
                with self.subTest('case-insensitive match'):
                    obj = re.compile(pattern, re.IGNORECASE)
                    self.assertTrue(obj.search(s))

                # Try the match with UNICODE locale enabled, and check
                # that it still succeeds.
                with self.subTest('unicode-sensitive match'):
                    obj = re.compile(pattern, re.UNICODE)
                    self.assertTrue(obj.search(s))


if __name__ == "__main__":
    unittest.main()
