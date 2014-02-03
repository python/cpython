from test.support import verbose, run_unittest, gc_collect, bigmemtest, _2G, \
        cpython_only, captured_stdout
import io
import re
from re import Scanner
import sre_compile
import sre_constants
import sys
import string
import traceback
import unittest
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
        self.assertEqual(re.search('x', 'aaa'), None)
        self.assertEqual(re.match('a*', 'xxx').span(0), (0, 0))
        self.assertEqual(re.match('a*', 'xxx').span(), (0, 0))
        self.assertEqual(re.match('x*', 'xxxa').span(0), (0, 3))
        self.assertEqual(re.match('x*', 'xxxa').span(), (0, 3))
        self.assertEqual(re.match('a+', 'xxx'), None)

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

        self.assertEqual(re.sub('.', lambda m: r"\n", 'x'), '\\n')
        self.assertEqual(re.sub('.', r"\n", 'x'), '\n')

        s = r"\1\1"
        self.assertEqual(re.sub('(.)', s, 'x'), 'xx')
        self.assertEqual(re.sub('(.)', re.escape(s), 'x'), s)
        self.assertEqual(re.sub('(.)', lambda m: s, 'x'), s)

        self.assertEqual(re.sub('(?P<a>x)', '\g<a>\g<a>', 'xx'), 'xxxx')
        self.assertEqual(re.sub('(?P<a>x)', '\g<a>\g<1>', 'xx'), 'xxxx')
        self.assertEqual(re.sub('(?P<unk>x)', '\g<unk>\g<unk>', 'xx'), 'xxxx')
        self.assertEqual(re.sub('(?P<unk>x)', '\g<1>\g<1>', 'xx'), 'xxxx')

        self.assertEqual(re.sub('a',r'\t\n\v\r\f\a\b\B\Z\a\A\w\W\s\S\d\D','a'),
                         '\t\n\v\r\f\a\b\\B\\Z\a\\A\\w\\W\\s\\S\\d\\D')
        self.assertEqual(re.sub('a', '\t\n\v\r\f\a', 'a'), '\t\n\v\r\f\a')
        self.assertEqual(re.sub('a', '\t\n\v\r\f\a', 'a'),
                         (chr(9)+chr(10)+chr(11)+chr(13)+chr(12)+chr(7)))

        self.assertEqual(re.sub('^\s*', 'X', 'test'), 'Xtest')

    def test_bug_449964(self):
        # fails for group followed by other escape
        self.assertEqual(re.sub(r'(?P<unk>x)', '\g<1>\g<1>\\b', 'xx'),
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

        self.assertEqual(re.sub('x', r'\1111', 'x'), '\1111')
        self.assertEqual(re.sub('x', r'\1111', 'x'), '\111' + '1')

        self.assertEqual(re.sub('x', r'\00', 'x'), '\x00')
        self.assertEqual(re.sub('x', r'\07', 'x'), '\x07')
        self.assertEqual(re.sub('x', r'\08', 'x'), '\0' + '8')
        self.assertEqual(re.sub('x', r'\09', 'x'), '\0' + '9')
        self.assertEqual(re.sub('x', r'\0a', 'x'), '\0' + 'a')

        self.assertEqual(re.sub('x', r'\400', 'x'), '\0')
        self.assertEqual(re.sub('x', r'\777', 'x'), '\377')

        self.assertRaises(re.error, re.sub, 'x', r'\1', 'x')
        self.assertRaises(re.error, re.sub, 'x', r'\8', 'x')
        self.assertRaises(re.error, re.sub, 'x', r'\9', 'x')
        self.assertRaises(re.error, re.sub, 'x', r'\11', 'x')
        self.assertRaises(re.error, re.sub, 'x', r'\18', 'x')
        self.assertRaises(re.error, re.sub, 'x', r'\1a', 'x')
        self.assertRaises(re.error, re.sub, 'x', r'\90', 'x')
        self.assertRaises(re.error, re.sub, 'x', r'\99', 'x')
        self.assertRaises(re.error, re.sub, 'x', r'\118', 'x') # r'\11' + '8'
        self.assertRaises(re.error, re.sub, 'x', r'\11a', 'x')
        self.assertRaises(re.error, re.sub, 'x', r'\181', 'x') # r'\18' + '1'
        self.assertRaises(re.error, re.sub, 'x', r'\800', 'x') # r'\80' + '0'

        # in python2.3 (etc), these loop endlessly in sre_parser.py
        self.assertEqual(re.sub('(((((((((((x)))))))))))', r'\11', 'x'), 'x')
        self.assertEqual(re.sub('((((((((((y))))))))))(.)', r'\118', 'xyz'),
                         'xz8')
        self.assertEqual(re.sub('((((((((((y))))))))))(.)', r'\11a', 'xyz'),
                         'xza')

    def test_qualified_re_sub(self):
        self.assertEqual(re.sub('a', 'b', 'aaaaa'), 'bbbbb')
        self.assertEqual(re.sub('a', 'b', 'aaaaa', 1), 'baaaa')

    def test_bug_114660(self):
        self.assertEqual(re.sub(r'(\S)\s+(\S)', r'\1 \2', 'hello  there'),
                         'hello there')

    def test_bug_462270(self):
        # Test for empty sub() behaviour, see SF bug #462270
        self.assertEqual(re.sub('x*', '-', 'abxd'), '-a-b-d-')
        self.assertEqual(re.sub('x+', '-', 'abxd'), 'ab-d')

    def test_symbolic_groups(self):
        re.compile('(?P<a>x)(?P=a)(?(a)y)')
        re.compile('(?P<a1>x)(?P=a1)(?(a1)y)')
        self.assertRaises(re.error, re.compile, '(?P<a>)(?P<a>)')
        self.assertRaises(re.error, re.compile, '(?Px)')
        self.assertRaises(re.error, re.compile, '(?P=)')
        self.assertRaises(re.error, re.compile, '(?P=1)')
        self.assertRaises(re.error, re.compile, '(?P=a)')
        self.assertRaises(re.error, re.compile, '(?P=a1)')
        self.assertRaises(re.error, re.compile, '(?P=a.)')
        self.assertRaises(re.error, re.compile, '(?P<)')
        self.assertRaises(re.error, re.compile, '(?P<>)')
        self.assertRaises(re.error, re.compile, '(?P<1>)')
        self.assertRaises(re.error, re.compile, '(?P<a.>)')
        self.assertRaises(re.error, re.compile, '(?())')
        self.assertRaises(re.error, re.compile, '(?(a))')
        self.assertRaises(re.error, re.compile, '(?(1a))')
        self.assertRaises(re.error, re.compile, '(?(a.))')
        # New valid/invalid identifiers in Python 3
        re.compile('(?P<µ>x)(?P=µ)(?(µ)y)')
        re.compile('(?P<𝔘𝔫𝔦𝔠𝔬𝔡𝔢>x)(?P=𝔘𝔫𝔦𝔠𝔬𝔡𝔢)(?(𝔘𝔫𝔦𝔠𝔬𝔡𝔢)y)')
        self.assertRaises(re.error, re.compile, '(?P<©>x)')

    def test_symbolic_refs(self):
        self.assertRaises(re.error, re.sub, '(?P<a>x)', '\g<a', 'xx')
        self.assertRaises(re.error, re.sub, '(?P<a>x)', '\g<', 'xx')
        self.assertRaises(re.error, re.sub, '(?P<a>x)', '\g', 'xx')
        self.assertRaises(re.error, re.sub, '(?P<a>x)', '\g<a a>', 'xx')
        self.assertRaises(re.error, re.sub, '(?P<a>x)', '\g<>', 'xx')
        self.assertRaises(re.error, re.sub, '(?P<a>x)', '\g<1a1>', 'xx')
        self.assertRaises(IndexError, re.sub, '(?P<a>x)', '\g<ab>', 'xx')
        self.assertRaises(re.error, re.sub, '(?P<a>x)|(?P<b>y)', '\g<b>', 'xx')
        self.assertRaises(re.error, re.sub, '(?P<a>x)|(?P<b>y)', '\\2', 'xx')
        self.assertRaises(re.error, re.sub, '(?P<a>x)', '\g<-1>', 'xx')
        # New valid/invalid identifiers in Python 3
        self.assertEqual(re.sub('(?P<µ>x)', r'\g<µ>', 'xx'), 'xx')
        self.assertEqual(re.sub('(?P<𝔘𝔫𝔦𝔠𝔬𝔡𝔢>x)', r'\g<𝔘𝔫𝔦𝔠𝔬𝔡𝔢>', 'xx'), 'xx')
        self.assertRaises(re.error, re.sub, '(?P<a>x)', r'\g<©>', 'xx')

    def test_re_subn(self):
        self.assertEqual(re.subn("(?i)b+", "x", "bbbb BBBB"), ('x x', 2))
        self.assertEqual(re.subn("b+", "x", "bbbb BBBB"), ('x BBBB', 1))
        self.assertEqual(re.subn("b+", "x", "xyz"), ('xyz', 0))
        self.assertEqual(re.subn("b*", "x", "xyz"), ('xxxyxzx', 4))
        self.assertEqual(re.subn("b*", "x", "xyz", 2), ('xxxyz', 2))

    def test_re_split(self):
        for string in ":a:b::c", S(":a:b::c"):
            self.assertTypedEqual(re.split(":", string),
                                  ['', 'a', 'b', '', 'c'])
            self.assertTypedEqual(re.split(":*", string),
                                  ['', 'a', 'b', 'c'])
            self.assertTypedEqual(re.split("(:*)", string),
                                  ['', ':', 'a', ':', 'b', '::', 'c'])
        for string in (b":a:b::c", B(b":a:b::c"), bytearray(b":a:b::c"),
                       memoryview(b":a:b::c")):
            self.assertTypedEqual(re.split(b":", string),
                                  [b'', b'a', b'b', b'', b'c'])
            self.assertTypedEqual(re.split(b":*", string),
                                  [b'', b'a', b'b', b'c'])
            self.assertTypedEqual(re.split(b"(:*)", string),
                                  [b'', b':', b'a', b':', b'b', b'::', b'c'])
        for a, b, c in ("\xe0\xdf\xe7", "\u0430\u0431\u0432",
                        "\U0001d49c\U0001d49e\U0001d4b5"):
            string = ":%s:%s::%s" % (a, b, c)
            self.assertEqual(re.split(":", string), ['', a, b, '', c])
            self.assertEqual(re.split(":*", string), ['', a, b, c])
            self.assertEqual(re.split("(:*)", string),
                             ['', ':', a, ':', b, '::', c])

        self.assertEqual(re.split("(?::*)", ":a:b::c"), ['', 'a', 'b', 'c'])
        self.assertEqual(re.split("(:)*", ":a:b::c"),
                         ['', ':', 'a', ':', 'b', ':', 'c'])
        self.assertEqual(re.split("([b:]+)", ":a:b::c"),
                         ['', ':', 'a', ':b::', 'c'])
        self.assertEqual(re.split("(b)|(:+)", ":a:b::c"),
                         ['', None, ':', 'a', None, ':', '', 'b', None, '',
                          None, '::', 'c'])
        self.assertEqual(re.split("(?:b)|(?::+)", ":a:b::c"),
                         ['', 'a', '', '', 'c'])

    def test_qualified_re_split(self):
        self.assertEqual(re.split(":", ":a:b::c", 2), ['', 'a', 'b::c'])
        self.assertEqual(re.split(':', 'a:b:c:d', 2), ['a', 'b', 'c:d'])
        self.assertEqual(re.split("(:)", ":a:b::c", 2),
                         ['', ':', 'a', ':', 'b::c'])
        self.assertEqual(re.split("(:*)", ":a:b::c", 2),
                         ['', ':', 'a', ':', 'b::c'])

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

        # A single group
        m = re.match('(a)', 'a')
        self.assertEqual(m.group(0), 'a')
        self.assertEqual(m.group(0), 'a')
        self.assertEqual(m.group(1), 'a')
        self.assertEqual(m.group(1, 1), ('a', 'a'))

        pat = re.compile('(?:(?P<a1>a)|(?P<b2>b))(?P<c3>c)?')
        self.assertEqual(pat.match('a').group(1, 2, 3), ('a', None, None))
        self.assertEqual(pat.match('b').group('a1', 'b2', 'c3'),
                         (None, 'b', None))
        self.assertEqual(pat.match('ac').group(1, 'b2', 3), ('a', None, 'c'))

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
        self.assertEqual(re.match('^(\()?([^()]+)(?(1)\))$', '(a)').groups(),
                         ('(', 'a'))
        self.assertEqual(re.match('^(\()?([^()]+)(?(1)\))$', 'a').groups(),
                         (None, 'a'))
        self.assertEqual(re.match('^(\()?([^()]+)(?(1)\))$', 'a)'), None)
        self.assertEqual(re.match('^(\()?([^()]+)(?(1)\))$', '(a'), None)
        self.assertEqual(re.match('^(?:(a)|c)((?(1)b|d))$', 'ab').groups(),
                         ('a', 'b'))
        self.assertEqual(re.match('^(?:(a)|c)((?(1)b|d))$', 'cd').groups(),
                         (None, 'd'))
        self.assertEqual(re.match('^(?:(a)|c)((?(1)|d))$', 'cd').groups(),
                         (None, 'd'))
        self.assertEqual(re.match('^(?:(a)|c)((?(1)|d))$', 'a').groups(),
                         ('a', ''))

        # Tests for bug #1177831: exercise groups other than the first group
        p = re.compile('(?P<g1>a)(?P<g2>b)?((?(g2)c|d))')
        self.assertEqual(p.match('abc').groups(),
                         ('a', 'b', 'c'))
        self.assertEqual(p.match('ad').groups(),
                         ('a', None, 'd'))
        self.assertEqual(p.match('abd'), None)
        self.assertEqual(p.match('ac'), None)


    def test_re_groupref(self):
        self.assertEqual(re.match(r'^(\|)?([^()]+)\1$', '|a|').groups(),
                         ('|', 'a'))
        self.assertEqual(re.match(r'^(\|)?([^()]+)\1?$', 'a').groups(),
                         (None, 'a'))
        self.assertEqual(re.match(r'^(\|)?([^()]+)\1$', 'a|'), None)
        self.assertEqual(re.match(r'^(\|)?([^()]+)\1$', '|a'), None)
        self.assertEqual(re.match(r'^(?:(a)|c)(\1)$', 'aa').groups(),
                         ('a', 'a'))
        self.assertEqual(re.match(r'^(?:(a)|c)(\1)?$', 'c').groups(),
                         (None, None))

    def test_groupdict(self):
        self.assertEqual(re.match('(?P<first>first) (?P<second>second)',
                                  'first second').groupdict(),
                         {'first':'first', 'second':'second'})

    def test_expand(self):
        self.assertEqual(re.match("(?P<first>first) (?P<second>second)",
                                  "first second")
                                  .expand(r"\2 \1 \g<second> \g<first>"),
                         "second first second first")

    def test_repeat_minmax(self):
        self.assertEqual(re.match("^(\w){1}$", "abc"), None)
        self.assertEqual(re.match("^(\w){1}?$", "abc"), None)
        self.assertEqual(re.match("^(\w){1,2}$", "abc"), None)
        self.assertEqual(re.match("^(\w){1,2}?$", "abc"), None)

        self.assertEqual(re.match("^(\w){3}$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){1,3}$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){1,4}$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){3,4}?$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){3}?$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){1,3}?$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){1,4}?$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){3,4}?$", "abc").group(1), "c")

        self.assertEqual(re.match("^x{1}$", "xxx"), None)
        self.assertEqual(re.match("^x{1}?$", "xxx"), None)
        self.assertEqual(re.match("^x{1,2}$", "xxx"), None)
        self.assertEqual(re.match("^x{1,2}?$", "xxx"), None)

        self.assertNotEqual(re.match("^x{3}$", "xxx"), None)
        self.assertNotEqual(re.match("^x{1,3}$", "xxx"), None)
        self.assertNotEqual(re.match("^x{1,4}$", "xxx"), None)
        self.assertNotEqual(re.match("^x{3,4}?$", "xxx"), None)
        self.assertNotEqual(re.match("^x{3}?$", "xxx"), None)
        self.assertNotEqual(re.match("^x{1,3}?$", "xxx"), None)
        self.assertNotEqual(re.match("^x{1,4}?$", "xxx"), None)
        self.assertNotEqual(re.match("^x{3,4}?$", "xxx"), None)

        self.assertEqual(re.match("^x{}$", "xxx"), None)
        self.assertNotEqual(re.match("^x{}$", "x{}"), None)

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
        self.assertNotEqual(re.match("(a)", "a").re, None)

    def test_special_escapes(self):
        self.assertEqual(re.search(r"\b(b.)\b",
                                   "abcd abc bcd bx").group(1), "bx")
        self.assertEqual(re.search(r"\B(b.)\B",
                                   "abc bcd bc abxd").group(1), "bx")
        self.assertEqual(re.search(r"\b(b.)\b",
                                   "abcd abc bcd bx", re.LOCALE).group(1), "bx")
        self.assertEqual(re.search(r"\B(b.)\B",
                                   "abc bcd bc abxd", re.LOCALE).group(1), "bx")
        self.assertEqual(re.search(r"\b(b.)\b",
                                   "abcd abc bcd bx", re.UNICODE).group(1), "bx")
        self.assertEqual(re.search(r"\B(b.)\B",
                                   "abc bcd bc abxd", re.UNICODE).group(1), "bx")
        self.assertEqual(re.search(r"^abc$", "\nabc\n", re.M).group(0), "abc")
        self.assertEqual(re.search(r"^\Aabc\Z$", "abc", re.M).group(0), "abc")
        self.assertEqual(re.search(r"^\Aabc\Z$", "\nabc\n", re.M), None)
        self.assertEqual(re.search(r"\b(b.)\b",
                                   "abcd abc bcd bx").group(1), "bx")
        self.assertEqual(re.search(r"\B(b.)\B",
                                   "abc bcd bc abxd").group(1), "bx")
        self.assertEqual(re.search(r"^abc$", "\nabc\n", re.M).group(0), "abc")
        self.assertEqual(re.search(r"^\Aabc\Z$", "abc", re.M).group(0), "abc")
        self.assertEqual(re.search(r"^\Aabc\Z$", "\nabc\n", re.M), None)
        self.assertEqual(re.search(r"\d\D\w\W\s\S",
                                   "1aa! a").group(0), "1aa! a")
        self.assertEqual(re.search(r"\d\D\w\W\s\S",
                                   "1aa! a", re.LOCALE).group(0), "1aa! a")
        self.assertEqual(re.search(r"\d\D\w\W\s\S",
                                   "1aa! a", re.UNICODE).group(0), "1aa! a")

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
        self.assertEqual(re.search(r"\B", ""), None)
        # This one is questionable and different from the perlre behaviour,
        # but describes current behavior.
        self.assertEqual(re.search(r"\b", ""), None)
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
        self.assertEqual(re.match("([\u2222\u2223])",
                                  "\u2222", re.UNICODE).group(1), "\u2222")
        r = '[%s]' % ''.join(map(chr, range(256, 2**16, 255)))
        self.assertEqual(re.match(r,
                                  "\uff01", re.UNICODE).group(), "\uff01")

    def test_big_codesize(self):
        # Issue #1160
        r = re.compile('|'.join(('%d'%x for x in range(10000))))
        self.assertIsNotNone(r.match('1000'))
        self.assertIsNotNone(r.match('9999'))

    def test_anyall(self):
        self.assertEqual(re.match("a.b", "a\nb", re.DOTALL).group(0),
                         "a\nb")
        self.assertEqual(re.match("a.*b", "a\n\nb", re.DOTALL).group(0),
                         "a\n\nb")

    def test_non_consuming(self):
        self.assertEqual(re.match("(a(?=\s[^a]))", "a b").group(1), "a")
        self.assertEqual(re.match("(a(?=\s[^a]*))", "a b").group(1), "a")
        self.assertEqual(re.match("(a(?=\s[abc]))", "a b").group(1), "a")
        self.assertEqual(re.match("(a(?=\s[abc]*))", "a bc").group(1), "a")
        self.assertEqual(re.match(r"(a)(?=\s\1)", "a a").group(1), "a")
        self.assertEqual(re.match(r"(a)(?=\s\1*)", "a aa").group(1), "a")
        self.assertEqual(re.match(r"(a)(?=\s(abc|a))", "a a").group(1), "a")

        self.assertEqual(re.match(r"(a(?!\s[^a]))", "a a").group(1), "a")
        self.assertEqual(re.match(r"(a(?!\s[abc]))", "a d").group(1), "a")
        self.assertEqual(re.match(r"(a)(?!\s\1)", "a b").group(1), "a")
        self.assertEqual(re.match(r"(a)(?!\s(abc|a))", "a b").group(1), "a")

    def test_ignore_case(self):
        self.assertEqual(re.match("abc", "ABC", re.I).group(0), "ABC")
        self.assertEqual(re.match("abc", "ABC", re.I).group(0), "ABC")
        self.assertEqual(re.match(r"(a\s[^a])", "a b", re.I).group(1), "a b")
        self.assertEqual(re.match(r"(a\s[^a]*)", "a bb", re.I).group(1), "a bb")
        self.assertEqual(re.match(r"(a\s[abc])", "a b", re.I).group(1), "a b")
        self.assertEqual(re.match(r"(a\s[abc]*)", "a bb", re.I).group(1), "a bb")
        self.assertEqual(re.match(r"((a)\s\2)", "a a", re.I).group(1), "a a")
        self.assertEqual(re.match(r"((a)\s\2*)", "a aa", re.I).group(1), "a aa")
        self.assertEqual(re.match(r"((a)\s(abc|a))", "a a", re.I).group(1), "a a")
        self.assertEqual(re.match(r"((a)\s(abc|a)*)", "a aa", re.I).group(1), "a aa")

    def test_category(self):
        self.assertEqual(re.match(r"(\s)", " ").group(1), " ")

    def test_getlower(self):
        import _sre
        self.assertEqual(_sre.getlower(ord('A'), 0), ord('a'))
        self.assertEqual(_sre.getlower(ord('A'), re.LOCALE), ord('a'))
        self.assertEqual(_sre.getlower(ord('A'), re.UNICODE), ord('a'))

        self.assertEqual(re.match("abc", "ABC", re.I).group(0), "ABC")
        self.assertEqual(re.match("abc", "ABC", re.I).group(0), "ABC")

    def test_not_literal(self):
        self.assertEqual(re.search("\s([^a])", " b").group(1), "b")
        self.assertEqual(re.search("\s([^a]*)", " bb").group(1), "bb")

    def test_search_coverage(self):
        self.assertEqual(re.search("\s(b)", " b").group(1), "b")
        self.assertEqual(re.search("a\s", "a ").group(0), "a ")

    def assertMatch(self, pattern, text, match=None, span=None,
                    matcher=re.match):
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

    def test_re_escape(self):
        alnum_chars = string.ascii_letters + string.digits + '_'
        p = ''.join(chr(i) for i in range(256))
        for c in p:
            if c in alnum_chars:
                self.assertEqual(re.escape(c), c)
            elif c == '\x00':
                self.assertEqual(re.escape(c), '\\000')
            else:
                self.assertEqual(re.escape(c), '\\' + c)
            self.assertMatch(re.escape(c), c)
        self.assertMatch(re.escape(p), p)

    def test_re_escape_byte(self):
        alnum_chars = (string.ascii_letters + string.digits + '_').encode('ascii')
        p = bytes(range(256))
        for i in p:
            b = bytes([i])
            if b in alnum_chars:
                self.assertEqual(re.escape(b), b)
            elif i == 0:
                self.assertEqual(re.escape(b), b'\\000')
            else:
                self.assertEqual(re.escape(b), b'\\' + b)
            self.assertMatch(re.escape(b), b)
        self.assertMatch(re.escape(p), p)

    def test_re_escape_non_ascii(self):
        s = 'xxx\u2620\u2620\u2620xxx'
        s_escaped = re.escape(s)
        self.assertEqual(s_escaped, 'xxx\\\u2620\\\u2620\\\u2620xxx')
        self.assertMatch(s_escaped, s)
        self.assertMatch('.%s+.' % re.escape('\u2620'), s,
                         'x\u2620\u2620\u2620x', (2, 7), re.search)

    def test_re_escape_non_ascii_bytes(self):
        b = 'y\u2620y\u2620y'.encode('utf-8')
        b_escaped = re.escape(b)
        self.assertEqual(b_escaped, b'y\\\xe2\\\x98\\\xa0y\\\xe2\\\x98\\\xa0y')
        self.assertMatch(b_escaped, b)
        res = re.findall(re.escape('\u2620'.encode('utf-8')), b)
        self.assertEqual(len(res), 2)

    def pickle_test(self, pickle):
        oldpat = re.compile('a(?:b|(c|e){1,2}?|d)+?(.)')
        s = pickle.dumps(oldpat)
        newpat = pickle.loads(s)
        self.assertEqual(oldpat, newpat)

    def test_constants(self):
        self.assertEqual(re.I, re.IGNORECASE)
        self.assertEqual(re.L, re.LOCALE)
        self.assertEqual(re.M, re.MULTILINE)
        self.assertEqual(re.S, re.DOTALL)
        self.assertEqual(re.X, re.VERBOSE)

    def test_flags(self):
        for flag in [re.I, re.M, re.X, re.S, re.L]:
            self.assertNotEqual(re.compile('^pattern$', flag), None)

    def test_sre_character_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255, 256, 0xFFFF, 0x10000, 0x10FFFF]:
            if i < 256:
                self.assertIsNotNone(re.match(r"\%03o" % i, chr(i)))
                self.assertIsNotNone(re.match(r"\%03o0" % i, chr(i)+"0"))
                self.assertIsNotNone(re.match(r"\%03o8" % i, chr(i)+"8"))
                self.assertIsNotNone(re.match(r"\x%02x" % i, chr(i)))
                self.assertIsNotNone(re.match(r"\x%02x0" % i, chr(i)+"0"))
                self.assertIsNotNone(re.match(r"\x%02xz" % i, chr(i)+"z"))
            if i < 0x10000:
                self.assertIsNotNone(re.match(r"\u%04x" % i, chr(i)))
                self.assertIsNotNone(re.match(r"\u%04x0" % i, chr(i)+"0"))
                self.assertIsNotNone(re.match(r"\u%04xz" % i, chr(i)+"z"))
            self.assertIsNotNone(re.match(r"\U%08x" % i, chr(i)))
            self.assertIsNotNone(re.match(r"\U%08x0" % i, chr(i)+"0"))
            self.assertIsNotNone(re.match(r"\U%08xz" % i, chr(i)+"z"))
        self.assertIsNotNone(re.match(r"\0", "\000"))
        self.assertIsNotNone(re.match(r"\08", "\0008"))
        self.assertIsNotNone(re.match(r"\01", "\001"))
        self.assertIsNotNone(re.match(r"\018", "\0018"))
        self.assertIsNotNone(re.match(r"\567", chr(0o167)))
        self.assertRaises(re.error, re.match, r"\911", "")
        self.assertRaises(re.error, re.match, r"\x1", "")
        self.assertRaises(re.error, re.match, r"\x1z", "")
        self.assertRaises(re.error, re.match, r"\u123", "")
        self.assertRaises(re.error, re.match, r"\u123z", "")
        self.assertRaises(re.error, re.match, r"\U0001234", "")
        self.assertRaises(re.error, re.match, r"\U0001234z", "")
        self.assertRaises(re.error, re.match, r"\U00110000", "")

    def test_sre_character_class_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255, 256, 0xFFFF, 0x10000, 0x10FFFF]:
            if i < 256:
                self.assertIsNotNone(re.match(r"[\%o]" % i, chr(i)))
                self.assertIsNotNone(re.match(r"[\%o8]" % i, chr(i)))
                self.assertIsNotNone(re.match(r"[\%03o]" % i, chr(i)))
                self.assertIsNotNone(re.match(r"[\%03o0]" % i, chr(i)))
                self.assertIsNotNone(re.match(r"[\%03o8]" % i, chr(i)))
                self.assertIsNotNone(re.match(r"[\x%02x]" % i, chr(i)))
                self.assertIsNotNone(re.match(r"[\x%02x0]" % i, chr(i)))
                self.assertIsNotNone(re.match(r"[\x%02xz]" % i, chr(i)))
            if i < 0x10000:
                self.assertIsNotNone(re.match(r"[\u%04x]" % i, chr(i)))
                self.assertIsNotNone(re.match(r"[\u%04x0]" % i, chr(i)))
                self.assertIsNotNone(re.match(r"[\u%04xz]" % i, chr(i)))
            self.assertIsNotNone(re.match(r"[\U%08x]" % i, chr(i)))
            self.assertIsNotNone(re.match(r"[\U%08x0]" % i, chr(i)+"0"))
            self.assertIsNotNone(re.match(r"[\U%08xz]" % i, chr(i)+"z"))
        self.assertIsNotNone(re.match(r"[\U0001d49c-\U0001d4b5]", "\U0001d49e"))
        self.assertRaises(re.error, re.match, r"[\911]", "")
        self.assertRaises(re.error, re.match, r"[\x1z]", "")
        self.assertRaises(re.error, re.match, r"[\u123z]", "")
        self.assertRaises(re.error, re.match, r"[\U0001234z]", "")
        self.assertRaises(re.error, re.match, r"[\U00110000]", "")

    def test_sre_byte_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255]:
            self.assertIsNotNone(re.match((r"\%03o" % i).encode(), bytes([i])))
            self.assertIsNotNone(re.match((r"\%03o0" % i).encode(), bytes([i])+b"0"))
            self.assertIsNotNone(re.match((r"\%03o8" % i).encode(), bytes([i])+b"8"))
            self.assertIsNotNone(re.match((r"\x%02x" % i).encode(), bytes([i])))
            self.assertIsNotNone(re.match((r"\x%02x0" % i).encode(), bytes([i])+b"0"))
            self.assertIsNotNone(re.match((r"\x%02xz" % i).encode(), bytes([i])+b"z"))
        self.assertIsNotNone(re.match(br"\u", b'u'))
        self.assertIsNotNone(re.match(br"\U", b'U'))
        self.assertIsNotNone(re.match(br"\0", b"\000"))
        self.assertIsNotNone(re.match(br"\08", b"\0008"))
        self.assertIsNotNone(re.match(br"\01", b"\001"))
        self.assertIsNotNone(re.match(br"\018", b"\0018"))
        self.assertIsNotNone(re.match(br"\567", bytes([0o167])))
        self.assertRaises(re.error, re.match, br"\911", b"")
        self.assertRaises(re.error, re.match, br"\x1", b"")
        self.assertRaises(re.error, re.match, br"\x1z", b"")

    def test_sre_byte_class_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255]:
            self.assertIsNotNone(re.match((r"[\%o]" % i).encode(), bytes([i])))
            self.assertIsNotNone(re.match((r"[\%o8]" % i).encode(), bytes([i])))
            self.assertIsNotNone(re.match((r"[\%03o]" % i).encode(), bytes([i])))
            self.assertIsNotNone(re.match((r"[\%03o0]" % i).encode(), bytes([i])))
            self.assertIsNotNone(re.match((r"[\%03o8]" % i).encode(), bytes([i])))
            self.assertIsNotNone(re.match((r"[\x%02x]" % i).encode(), bytes([i])))
            self.assertIsNotNone(re.match((r"[\x%02x0]" % i).encode(), bytes([i])))
            self.assertIsNotNone(re.match((r"[\x%02xz]" % i).encode(), bytes([i])))
        self.assertIsNotNone(re.match(br"[\u]", b'u'))
        self.assertIsNotNone(re.match(br"[\U]", b'U'))
        self.assertRaises(re.error, re.match, br"[\911]", "")
        self.assertRaises(re.error, re.match, br"[\x1z]", "")

    def test_bug_113254(self):
        self.assertEqual(re.match(r'(a)|(b)', 'b').start(1), -1)
        self.assertEqual(re.match(r'(a)|(b)', 'b').end(1), -1)
        self.assertEqual(re.match(r'(a)|(b)', 'b').span(1), (-1, -1))

    def test_bug_527371(self):
        # bug described in patches 527371/672491
        self.assertEqual(re.match(r'(a)?a','a').lastindex, None)
        self.assertEqual(re.match(r'(a)(b)?b','ab').lastindex, 1)
        self.assertEqual(re.match(r'(?P<a>a)(?P<b>b)?b','ab').lastgroup, 'a')
        self.assertEqual(re.match("(?P<a>a(b))", "ab").lastgroup, 'a')
        self.assertEqual(re.match("((a))", "a").lastindex, 1)

    def test_bug_545855(self):
        # bug 545855 -- This pattern failed to cause a compile error as it
        # should, instead provoking a TypeError.
        self.assertRaises(re.error, re.compile, 'foo[a-')

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

        self.assertNotEqual(scanner.scanner.scanner("").pattern, None)

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
        self.assertEqual(pat.match("xyz"), None)

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
        self.assertTrue(re.compile('bug_926075') is not
                     re.compile(b'bug_926075'))

    def test_bug_931848(self):
        pattern = eval('"[\u002E\u3002\uFF0E\uFF61]"')
        self.assertEqual(re.compile(pattern).split("a.b.c"),
                         ['a','b','c'])

    def test_bug_581080(self):
        iter = re.finditer(r"\s", "a b")
        self.assertEqual(next(iter).span(), (1,2))
        self.assertRaises(StopIteration, next, iter)

        scanner = re.compile(r"\s").scanner("a b")
        self.assertEqual(scanner.search().span(), (1, 2))
        self.assertEqual(scanner.search(), None)

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
            self.assertEqual(re.match('^\d$', x).group(0), x)

        not_decimal_digits = [
            '\u2165', # '\N{ROMAN NUMERAL SIX}', category 'Nl'
            '\u3039', # '\N{HANGZHOU NUMERAL TWENTY}', category 'Nl'
            '\u2082', # '\N{SUBSCRIPT TWO}', category 'No'
            '\u32b4', # '\N{CIRCLED NUMBER THIRTY NINE}', category 'No'
            ]
        for x in not_decimal_digits:
            self.assertIsNone(re.match('^\d$', x))

    def test_empty_array(self):
        # SF buf 1647541
        import array
        for typecode in 'bBuhHiIlLfd':
            a = array.array(typecode)
            self.assertEqual(re.compile(b"bla").match(a), None)
            self.assertEqual(re.compile(b"").match(a).groups(), ())

    def test_inline_flags(self):
        # Bug #1700
        upper_char = chr(0x1ea0) # Latin Capital Letter A with Dot Bellow
        lower_char = chr(0x1ea1) # Latin Small Letter A with Dot Bellow

        p = re.compile(upper_char, re.I | re.U)
        q = p.match(lower_char)
        self.assertNotEqual(q, None)

        p = re.compile(lower_char, re.I | re.U)
        q = p.match(upper_char)
        self.assertNotEqual(q, None)

        p = re.compile('(?i)' + upper_char, re.U)
        q = p.match(lower_char)
        self.assertNotEqual(q, None)

        p = re.compile('(?i)' + lower_char, re.U)
        q = p.match(upper_char)
        self.assertNotEqual(q, None)

        p = re.compile('(?iu)' + upper_char)
        q = p.match(lower_char)
        self.assertNotEqual(q, None)

        p = re.compile('(?iu)' + lower_char)
        q = p.match(upper_char)
        self.assertNotEqual(q, None)

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
            self.assertNotEqual(pat.match('\xe0'), None)
            pat = re.compile('\w', flags)
            self.assertNotEqual(pat.match('\xe0'), None)
        pat = re.compile('\xc0', re.ASCII | re.IGNORECASE)
        self.assertEqual(pat.match('\xe0'), None)
        pat = re.compile('(?a)\xc0', re.IGNORECASE)
        self.assertEqual(pat.match('\xe0'), None)
        pat = re.compile('\w', re.ASCII)
        self.assertEqual(pat.match('\xe0'), None)
        pat = re.compile('(?a)\w')
        self.assertEqual(pat.match('\xe0'), None)
        # Bytes patterns
        for flags in (0, re.ASCII):
            pat = re.compile(b'\xc0', re.IGNORECASE)
            self.assertEqual(pat.match(b'\xe0'), None)
            pat = re.compile(b'\w')
            self.assertEqual(pat.match(b'\xe0'), None)
        # Incompatibilities
        self.assertRaises(ValueError, re.compile, b'\w', re.UNICODE)
        self.assertRaises(ValueError, re.compile, b'(?u)\w')
        self.assertRaises(ValueError, re.compile, '\w', re.UNICODE | re.ASCII)
        self.assertRaises(ValueError, re.compile, '(?u)\w', re.ASCII)
        self.assertRaises(ValueError, re.compile, '(?a)\w', re.UNICODE)
        self.assertRaises(ValueError, re.compile, '(?au)\w')

    def test_bug_6509(self):
        # Replacement strings of both types must parse properly.
        # all strings
        pat = re.compile('a(\w)')
        self.assertEqual(pat.sub('b\\1', 'ac'), 'bc')
        pat = re.compile('a(.)')
        self.assertEqual(pat.sub('b\\1', 'a\u1234'), 'b\u1234')
        pat = re.compile('..')
        self.assertEqual(pat.sub(lambda m: 'str', 'a5'), 'str')

        # all bytes
        pat = re.compile(b'a(\w)')
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
        self.assertRaises(OverflowError, _sre.compile, "abc", 0, [long_overflow])
        self.assertRaises(TypeError, _sre.compile, {}, 0, [])

    def test_search_dot_unicode(self):
        self.assertIsNotNone(re.search("123.*-", '123abc-'))
        self.assertIsNotNone(re.search("123.*-", '123\xe9-'))
        self.assertIsNotNone(re.search("123.*-", '123\u20ac-'))
        self.assertIsNotNone(re.search("123.*-", '123\U0010ffff-'))
        self.assertIsNotNone(re.search("123.*-", '123\xe9\u20ac\U0010ffff-'))

    def test_compile(self):
        # Test return value when given string and pattern as parameter
        pattern = re.compile('random pattern')
        self.assertIsInstance(pattern, re._pattern_type)
        same_pattern = re.compile(pattern)
        self.assertIsInstance(same_pattern, re._pattern_type)
        self.assertIs(same_pattern, pattern)
        # Test behaviour when not given a string or pattern as parameter
        self.assertRaises(TypeError, re.compile, 0)

    def test_bug_13899(self):
        # Issue #13899: re pattern r"[\A]" should work like "A" but matches
        # nothing. Ditto B and Z.
        self.assertEqual(re.findall(r'[\A\B\b\C\Z]', 'AB\bCZ'),
                         ['A', 'B', '\b', 'C', 'Z'])

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
        with self.assertRaisesRegex(sre_constants.error, '<foo>'):
            re.compile('(?P=<foo>)')

    def test_group_name_in_exception(self):
        # Issue 17341: Poor error message when compiling invalid regex
        with self.assertRaisesRegex(sre_constants.error, '\?foo'):
            re.compile('(?P<?foo>)')

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
            self.assertEqual(repr(m), "<%s.%s object; "
                             "span=(1, 12), match='abracadabra'>" %
                             (type(m).__module__, type(m).__qualname__))
        for string in (b'[abracadabra]', B(b'[abracadabra]'),
                       bytearray(b'[abracadabra]'),
                       memoryview(b'[abracadabra]')):
            m = re.search(rb'(.+)(.*?)\1', string)
            self.assertEqual(repr(m), "<%s.%s object; "
                             "span=(1, 12), match=b'abracadabra'>" %
                             (type(m).__module__, type(m).__qualname__))

        first, second = list(re.finditer("(aa)|(bb)", "aa bb"))
        self.assertEqual(repr(first), "<%s.%s object; "
                         "span=(0, 2), match='aa'>" %
                         (type(second).__module__, type(first).__qualname__))
        self.assertEqual(repr(second), "<%s.%s object; "
                         "span=(3, 5), match='bb'>" %
                         (type(second).__module__, type(second).__qualname__))


    def test_bug_2537(self):
        # issue 2537: empty submatches
        for outer_op in ('{0,}', '*', '+', '{1,187}'):
            for inner_op in ('{0,}', '*', '?'):
                r = re.compile("^((x|y)%s)%s" % (inner_op, outer_op))
                m = r.match("xyyzy")
                self.assertEqual(m.group(0), "xyy")
                self.assertEqual(m.group(1), "")
                self.assertEqual(m.group(2), "y")

    def test_debug_flag(self):
        with captured_stdout() as out:
            re.compile('foo', re.DEBUG)
        self.assertEqual(out.getvalue().splitlines(),
                         ['literal 102 ', 'literal 111 ', 'literal 111 '])
        # Debug output is output again even a second time (bypassing
        # the cache -- issue #20426).
        with captured_stdout() as out:
            re.compile('foo', re.DEBUG)
        self.assertEqual(out.getvalue().splitlines(),
                         ['literal 102 ', 'literal 111 ', 'literal 111 '])


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


class ImplementationTest(unittest.TestCase):
    """
    Test implementation details of the re module.
    """

    def test_overlap_table(self):
        f = sre_compile._generate_overlap_table
        self.assertEqual(f(""), [])
        self.assertEqual(f("a"), [0])
        self.assertEqual(f("abcd"), [0, 0, 0, 0])
        self.assertEqual(f("aaaa"), [0, 1, 2, 3])
        self.assertEqual(f("ababba"), [0, 0, 1, 2, 0, 1])
        self.assertEqual(f("abcabdac"), [0, 0, 0, 1, 2, 0, 1, 0])


def run_re_tests():
    from test.re_tests import tests, SUCCEED, FAIL, SYNTAX_ERROR
    if verbose:
        print('Running re_tests test suite')
    else:
        # To save time, only run the first and last 10 tests
        #tests = tests[:10] + tests[-10:]
        pass

    for t in tests:
        sys.stdout.flush()
        pattern = s = outcome = repl = expected = None
        if len(t) == 5:
            pattern, s, outcome, repl, expected = t
        elif len(t) == 3:
            pattern, s, outcome = t
        else:
            raise ValueError('Test tuples should have 3 or 5 fields', t)

        try:
            obj = re.compile(pattern)
        except re.error:
            if outcome == SYNTAX_ERROR: pass  # Expected a syntax error
            else:
                print('=== Syntax error:', t)
        except KeyboardInterrupt: raise KeyboardInterrupt
        except:
            print('*** Unexpected error ***', t)
            if verbose:
                traceback.print_exc(file=sys.stdout)
        else:
            try:
                result = obj.search(s)
            except re.error as msg:
                print('=== Unexpected exception', t, repr(msg))
            if outcome == SYNTAX_ERROR:
                # This should have been a syntax error; forget it.
                pass
            elif outcome == FAIL:
                if result is None: pass   # No match, as expected
                else: print('=== Succeeded incorrectly', t)
            elif outcome == SUCCEED:
                if result is not None:
                    # Matched, as expected, so now we compute the
                    # result string and compare it to our expected result.
                    start, end = result.span(0)
                    vardict={'found': result.group(0),
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
                    repl = eval(repl, vardict)
                    if repl != expected:
                        print('=== grouping error', t, end=' ')
                        print(repr(repl) + ' should be ' + repr(expected))
                else:
                    print('=== Failed incorrectly', t)

                # Try the match with both pattern and string converted to
                # bytes, and check that it still succeeds.
                try:
                    bpat = bytes(pattern, "ascii")
                    bs = bytes(s, "ascii")
                except UnicodeEncodeError:
                    # skip non-ascii tests
                    pass
                else:
                    try:
                        bpat = re.compile(bpat)
                    except Exception:
                        print('=== Fails on bytes pattern compile', t)
                        if verbose:
                            traceback.print_exc(file=sys.stdout)
                    else:
                        bytes_result = bpat.search(bs)
                        if bytes_result is None:
                            print('=== Fails on bytes pattern match', t)

                # Try the match with the search area limited to the extent
                # of the match and see if it still succeeds.  \B will
                # break (because it won't match at the end or start of a
                # string), so we'll ignore patterns that feature it.

                if pattern[:2] != '\\B' and pattern[-2:] != '\\B' \
                               and result is not None:
                    obj = re.compile(pattern)
                    result = obj.search(s, result.start(0), result.end(0) + 1)
                    if result is None:
                        print('=== Failed on range-limited match', t)

                # Try the match with IGNORECASE enabled, and check that it
                # still succeeds.
                obj = re.compile(pattern, re.IGNORECASE)
                result = obj.search(s)
                if result is None:
                    print('=== Fails on case-insensitive match', t)

                # Try the match with LOCALE enabled, and check that it
                # still succeeds.
                if '(?u)' not in pattern:
                    obj = re.compile(pattern, re.LOCALE)
                    result = obj.search(s)
                    if result is None:
                        print('=== Fails on locale-sensitive match', t)

                # Try the match with UNICODE locale enabled, and check
                # that it still succeeds.
                obj = re.compile(pattern, re.UNICODE)
                result = obj.search(s)
                if result is None:
                    print('=== Fails on unicode-sensitive match', t)


def test_main():
    run_unittest(__name__)
    run_re_tests()

if __name__ == "__main__":
    test_main()
