# -*- coding: utf-8 -*-
from test.test_support import verbose, run_unittest, import_module
from test.test_support import precisionbigmemtest, _2G, cpython_only
from test.test_support import captured_stdout, have_unicode, requires_unicode, u
import locale
import re
from re import Scanner
import sre_constants
import sys
import string
import traceback
from weakref import proxy


# Misc tests from Tim Peters' re.doc

# WARNING: Don't change details in these tests if you don't know
# what you're doing. Some of these tests were carefully modeled to
# cover most of the code.

import unittest

class ReTests(unittest.TestCase):

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

    @requires_unicode
    def test_bug_1140(self):
        # re.sub(x, y, u'') should return u'', not '', and
        # re.sub(x, y, '') should return '', not u''.
        # Also:
        # re.sub(x, y, unicode(x)) should return unicode(y), and
        # re.sub(x, y, str(x)) should return
        #     str(y) if isinstance(y, str) else unicode(y).
        for x in 'x', u'x':
            for y in 'y', u'y':
                z = re.sub(x, y, u'')
                self.assertEqual(z, u'')
                self.assertEqual(type(z), unicode)
                #
                z = re.sub(x, y, '')
                self.assertEqual(z, '')
                self.assertEqual(type(z), str)
                #
                z = re.sub(x, y, unicode(x))
                self.assertEqual(z, y)
                self.assertEqual(type(z), unicode)
                #
                z = re.sub(x, y, str(x))
                self.assertEqual(z, y)
                self.assertEqual(type(z), type(y))

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

    def test_re_subn(self):
        self.assertEqual(re.subn("(?i)b+", "x", "bbbb BBBB"), ('x x', 2))
        self.assertEqual(re.subn("b+", "x", "bbbb BBBB"), ('x BBBB', 1))
        self.assertEqual(re.subn("b+", "x", "xyz"), ('xyz', 0))
        self.assertEqual(re.subn("b*", "x", "xyz"), ('xxxyxzx', 4))
        self.assertEqual(re.subn("b*", "x", "xyz", 2), ('xxxyz', 2))

    def test_re_split(self):
        self.assertEqual(re.split(":", ":a:b::c"), ['', 'a', 'b', '', 'c'])
        self.assertEqual(re.split(":*", ":a:b::c"), ['', 'a', 'b', 'c'])
        self.assertEqual(re.split("(:*)", ":a:b::c"),
                         ['', ':', 'a', ':', 'b', '::', 'c'])
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
        self.assertEqual(re.findall(":+", "a:b::c:::d"), [":", "::", ":::"])
        self.assertEqual(re.findall("(:+)", "a:b::c:::d"), [":", "::", ":::"])
        self.assertEqual(re.findall("(:)(:*)", "a:b::c:::d"), [(":", ""),
                                                               (":", ":"),
                                                               (":", "::")])

    def test_bug_117612(self):
        self.assertEqual(re.findall(r"(a|(b))", "aba"),
                         [("a", ""),("b", "b"),("a", "")])

    def test_re_match(self):
        self.assertEqual(re.match('a', 'a').groups(), ())
        self.assertEqual(re.match('(a)', 'a').groups(), ('a',))
        self.assertEqual(re.match(r'(a)', 'a').group(0), 'a')
        self.assertEqual(re.match(r'(a)', 'a').group(1), 'a')
        self.assertEqual(re.match(r'(a)', 'a').group(1, 1), ('a', 'a'))

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

    def test_re_groupref_exists(self):
        self.assertEqual(re.match('^(\()?([^()]+)(?(1)\))$', '(a)').groups(),
                         ('(', 'a'))
        self.assertEqual(re.match('^(\()?([^()]+)(?(1)\))$', 'a').groups(),
                         (None, 'a'))
        self.assertIsNone(re.match('^(\()?([^()]+)(?(1)\))$', 'a)'))
        self.assertIsNone(re.match('^(\()?([^()]+)(?(1)\))$', '(a'))
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
        self.assertIsNone(p.match('abd'))
        self.assertIsNone(p.match('ac'))


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
        self.assertIsNone(re.match("^(\w){1}$", "abc"))
        self.assertIsNone(re.match("^(\w){1}?$", "abc"))
        self.assertIsNone(re.match("^(\w){1,2}$", "abc"))
        self.assertIsNone(re.match("^(\w){1,2}?$", "abc"))

        self.assertEqual(re.match("^(\w){3}$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){1,3}$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){1,4}$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){3,4}?$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){3}?$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){1,3}?$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){1,4}?$", "abc").group(1), "c")
        self.assertEqual(re.match("^(\w){3,4}?$", "abc").group(1), "c")

        self.assertIsNone(re.match("^x{1}$", "xxx"))
        self.assertIsNone(re.match("^x{1}?$", "xxx"))
        self.assertIsNone(re.match("^x{1,2}$", "xxx"))
        self.assertIsNone(re.match("^x{1,2}?$", "xxx"))

        self.assertTrue(re.match("^x{3}$", "xxx"))
        self.assertTrue(re.match("^x{1,3}$", "xxx"))
        self.assertTrue(re.match("^x{1,4}$", "xxx"))
        self.assertTrue(re.match("^x{3,4}?$", "xxx"))
        self.assertTrue(re.match("^x{3}?$", "xxx"))
        self.assertTrue(re.match("^x{1,3}?$", "xxx"))
        self.assertTrue(re.match("^x{1,4}?$", "xxx"))
        self.assertTrue(re.match("^x{3,4}?$", "xxx"))

        self.assertIsNone(re.match("^x{}$", "xxx"))
        self.assertTrue(re.match("^x{}$", "x{}"))

    def test_getattr(self):
        self.assertEqual(re.match("(a)", "a").pos, 0)
        self.assertEqual(re.match("(a)", "a").endpos, 1)
        self.assertEqual(re.match("(a)", "a").string, "a")
        self.assertEqual(re.match("(a)", "a").regs, ((0, 1), (0, 1)))
        self.assertTrue(re.match("(a)", "a").re)

    def test_special_escapes(self):
        self.assertEqual(re.search(r"\b(b.)\b",
                                   "abcd abc bcd bx").group(1), "bx")
        self.assertEqual(re.search(r"\B(b.)\B",
                                   "abc bcd bc abxd").group(1), "bx")
        self.assertEqual(re.search(r"\b(b.)\b",
                                   "abcd abc bcd bx", re.LOCALE).group(1), "bx")
        self.assertEqual(re.search(r"\B(b.)\B",
                                   "abc bcd bc abxd", re.LOCALE).group(1), "bx")
        if have_unicode:
            self.assertEqual(re.search(r"\b(b.)\b",
                                       "abcd abc bcd bx", re.UNICODE).group(1), "bx")
            self.assertEqual(re.search(r"\B(b.)\B",
                                       "abc bcd bc abxd", re.UNICODE).group(1), "bx")
        self.assertEqual(re.search(r"^abc$", "\nabc\n", re.M).group(0), "abc")
        self.assertEqual(re.search(r"^\Aabc\Z$", "abc", re.M).group(0), "abc")
        self.assertIsNone(re.search(r"^\Aabc\Z$", "\nabc\n", re.M))
        self.assertEqual(re.search(r"\b(b.)\b",
                                   u"abcd abc bcd bx").group(1), "bx")
        self.assertEqual(re.search(r"\B(b.)\B",
                                   u"abc bcd bc abxd").group(1), "bx")
        self.assertEqual(re.search(r"^abc$", u"\nabc\n", re.M).group(0), "abc")
        self.assertEqual(re.search(r"^\Aabc\Z$", u"abc", re.M).group(0), "abc")
        self.assertIsNone(re.search(r"^\Aabc\Z$", u"\nabc\n", re.M))
        self.assertEqual(re.search(r"\d\D\w\W\s\S",
                                   "1aa! a").group(0), "1aa! a")
        self.assertEqual(re.search(r"\d\D\w\W\s\S",
                                   "1aa! a", re.LOCALE).group(0), "1aa! a")
        if have_unicode:
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

    @requires_unicode
    def test_bigcharset(self):
        self.assertEqual(re.match(u(r"([\u2222\u2223])"),
                                  unichr(0x2222)).group(1), unichr(0x2222))
        self.assertEqual(re.match(u(r"([\u2222\u2223])"),
                                  unichr(0x2222), re.UNICODE).group(1), unichr(0x2222))
        r = u'[%s]' % u''.join(map(unichr, range(256, 2**16, 255)))
        self.assertEqual(re.match(r, unichr(0xff01), re.UNICODE).group(), unichr(0xff01))

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
        self.assertEqual(re.match("abc", u"ABC", re.I).group(0), "ABC")
        self.assertEqual(re.match(r"(a\s[^a])", "a b", re.I).group(1), "a b")
        self.assertEqual(re.match(r"(a\s[^a]*)", "a bb", re.I).group(1), "a bb")
        self.assertEqual(re.match(r"(a\s[abc])", "a b", re.I).group(1), "a b")
        self.assertEqual(re.match(r"(a\s[abc]*)", "a bb", re.I).group(1), "a bb")
        self.assertEqual(re.match(r"((a)\s\2)", "a a", re.I).group(1), "a a")
        self.assertEqual(re.match(r"((a)\s\2*)", "a aa", re.I).group(1), "a aa")
        self.assertEqual(re.match(r"((a)\s(abc|a))", "a a", re.I).group(1), "a a")
        self.assertEqual(re.match(r"((a)\s(abc|a)*)", "a aa", re.I).group(1), "a aa")

        if have_unicode:
            assert u(r'\u212a').lower() == u'k' # 'K'
            self.assertTrue(re.match(ur'K', u(r'\u212a'), re.U | re.I))
            self.assertTrue(re.match(ur'k', u(r'\u212a'), re.U | re.I))
            self.assertTrue(re.match(u(r'\u212a'), u'K', re.U | re.I))
            self.assertTrue(re.match(u(r'\u212a'), u'k', re.U | re.I))
            assert u(r'\u017f').upper() == u'S' # 'ſ'
            self.assertTrue(re.match(ur'S', u(r'\u017f'), re.U | re.I))
            self.assertTrue(re.match(ur's', u(r'\u017f'), re.U | re.I))
            self.assertTrue(re.match(u(r'\u017f'), u'S', re.U | re.I))
            self.assertTrue(re.match(u(r'\u017f'), u's', re.U | re.I))

    def test_ignore_case_set(self):
        self.assertTrue(re.match(r'[19A]', 'A', re.I))
        self.assertTrue(re.match(r'[19a]', 'a', re.I))
        self.assertTrue(re.match(r'[19a]', 'A', re.I))
        self.assertTrue(re.match(r'[19A]', 'a', re.I))
        if have_unicode:
            self.assertTrue(re.match(ur'[19A]', u'A', re.U | re.I))
            self.assertTrue(re.match(ur'[19a]', u'a', re.U | re.I))
            self.assertTrue(re.match(ur'[19a]', u'A', re.U | re.I))
            self.assertTrue(re.match(ur'[19A]', u'a', re.U | re.I))
            assert u(r'\u212a').lower() == u'k' # 'K'
            self.assertTrue(re.match(u(r'[19K]'), u(r'\u212a'), re.U | re.I))
            self.assertTrue(re.match(u(r'[19k]'), u(r'\u212a'), re.U | re.I))
            self.assertTrue(re.match(u(r'[19\u212a]'), u'K', re.U | re.I))
            self.assertTrue(re.match(u(r'[19\u212a]'), u'k', re.U | re.I))
            assert u(r'\u017f').upper() == u'S' # 'ſ'
            self.assertTrue(re.match(ur'[19S]', u(r'\u017f'), re.U | re.I))
            self.assertTrue(re.match(ur'[19s]', u(r'\u017f'), re.U | re.I))
            self.assertTrue(re.match(u(r'[19\u017f]'), u'S', re.U | re.I))
            self.assertTrue(re.match(u(r'[19\u017f]'), u's', re.U | re.I))

    def test_ignore_case_range(self):
        # Issues #3511, #17381.
        self.assertTrue(re.match(r'[9-a]', '_', re.I))
        self.assertIsNone(re.match(r'[9-A]', '_', re.I))
        self.assertTrue(re.match(r'[\xc0-\xde]', '\xd7', re.I))
        self.assertIsNone(re.match(r'[\xc0-\xde]', '\xf7', re.I))
        self.assertTrue(re.match(r'[\xe0-\xfe]', '\xf7',re.I))
        self.assertIsNone(re.match(r'[\xe0-\xfe]', '\xd7', re.I))
        if have_unicode:
            self.assertTrue(re.match(u(r'[9-a]'), u(r'_'), re.U | re.I))
            self.assertIsNone(re.match(u(r'[9-A]'), u(r'_'), re.U | re.I))
            self.assertTrue(re.match(u(r'[\xc0-\xde]'),
                                     u(r'\xd7'), re.U | re.I))
            self.assertIsNone(re.match(u(r'[\xc0-\xde]'),
                                       u(r'\xf7'), re.U | re.I))
            self.assertTrue(re.match(u(r'[\xe0-\xfe]'),
                                     u(r'\xf7'), re.U | re.I))
            self.assertIsNone(re.match(u(r'[\xe0-\xfe]'),
                                       u(r'\xd7'), re.U | re.I))
            self.assertTrue(re.match(u(r'[\u0430-\u045f]'),
                                     u(r'\u0450'), re.U | re.I))
            self.assertTrue(re.match(u(r'[\u0430-\u045f]'),
                                     u(r'\u0400'), re.U | re.I))
            self.assertTrue(re.match(u(r'[\u0400-\u042f]'),
                                     u(r'\u0450'), re.U | re.I))
            self.assertTrue(re.match(u(r'[\u0400-\u042f]'),
                                     u(r'\u0400'), re.U | re.I))
            if sys.maxunicode > 0xffff:
                self.assertTrue(re.match(u(r'[\U00010428-\U0001044f]'),
                                         u(r'\U00010428'), re.U | re.I))
                self.assertTrue(re.match(u(r'[\U00010428-\U0001044f]'),
                                         u(r'\U00010400'), re.U | re.I))
                self.assertTrue(re.match(u(r'[\U00010400-\U00010427]'),
                                         u(r'\U00010428'), re.U | re.I))
                self.assertTrue(re.match(u(r'[\U00010400-\U00010427]'),
                                         u(r'\U00010400'), re.U | re.I))

            assert u(r'\u212a').lower() == u'k' # 'K'
            self.assertTrue(re.match(ur'[J-M]', u(r'\u212a'), re.U | re.I))
            self.assertTrue(re.match(ur'[j-m]', u(r'\u212a'), re.U | re.I))
            self.assertTrue(re.match(u(r'[\u2129-\u212b]'), u'K', re.U | re.I))
            self.assertTrue(re.match(u(r'[\u2129-\u212b]'), u'k', re.U | re.I))
            assert u(r'\u017f').upper() == u'S' # 'ſ'
            self.assertTrue(re.match(ur'[R-T]', u(r'\u017f'), re.U | re.I))
            self.assertTrue(re.match(ur'[r-t]', u(r'\u017f'), re.U | re.I))
            self.assertTrue(re.match(u(r'[\u017e-\u0180]'), u'S', re.U | re.I))
            self.assertTrue(re.match(u(r'[\u017e-\u0180]'), u's', re.U | re.I))

    def test_category(self):
        self.assertEqual(re.match(r"(\s)", " ").group(1), " ")

    def test_getlower(self):
        import _sre
        self.assertEqual(_sre.getlower(ord('A'), 0), ord('a'))
        self.assertEqual(_sre.getlower(ord('A'), re.LOCALE), ord('a'))
        if have_unicode:
            self.assertEqual(_sre.getlower(ord('A'), re.UNICODE), ord('a'))

        self.assertEqual(re.match("abc", "ABC", re.I).group(0), "ABC")
        self.assertEqual(re.match("abc", u"ABC", re.I).group(0), "ABC")

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

    @requires_unicode
    def test_re_escape(self):
        alnum_chars = unicode(string.ascii_letters + string.digits)
        p = u''.join(unichr(i) for i in range(256))
        for c in p:
            if c in alnum_chars:
                self.assertEqual(re.escape(c), c)
            elif c == u'\x00':
                self.assertEqual(re.escape(c), u'\\000')
            else:
                self.assertEqual(re.escape(c), u'\\' + c)
            self.assertMatch(re.escape(c), c)
        self.assertMatch(re.escape(p), p)

    def test_re_escape_byte(self):
        alnum_chars = string.ascii_letters + string.digits
        p = ''.join(chr(i) for i in range(256))
        for b in p:
            if b in alnum_chars:
                self.assertEqual(re.escape(b), b)
            elif b == b'\x00':
                self.assertEqual(re.escape(b), b'\\000')
            else:
                self.assertEqual(re.escape(b), b'\\' + b)
            self.assertMatch(re.escape(b), b)
        self.assertMatch(re.escape(p), p)

    @requires_unicode
    def test_re_escape_non_ascii(self):
        s = u(r'xxx\u2620\u2620\u2620xxx')
        s_escaped = re.escape(s)
        self.assertEqual(s_escaped, u(r'xxx\\\u2620\\\u2620\\\u2620xxx'))
        self.assertMatch(s_escaped, s)
        self.assertMatch(u'.%s+.' % re.escape(unichr(0x2620)), s,
                         u(r'x\u2620\u2620\u2620x'), (2, 7), re.search)

    def test_re_escape_non_ascii_bytes(self):
        b = b'y\xe2\x98\xa0y\xe2\x98\xa0y'
        b_escaped = re.escape(b)
        self.assertEqual(b_escaped, b'y\\\xe2\\\x98\\\xa0y\\\xe2\\\x98\\\xa0y')
        self.assertMatch(b_escaped, b)
        res = re.findall(re.escape(b'\xe2\x98\xa0'), b)
        self.assertEqual(len(res), 2)

    def test_pickling(self):
        import pickle
        self.pickle_test(pickle)
        import cPickle
        self.pickle_test(cPickle)
        # old pickles expect the _compile() reconstructor in sre module
        import_module("sre", deprecated=True)
        from sre import _compile
        # current pickle expects the _compile() reconstructor in re module
        from re import _compile

    def pickle_test(self, pickle):
        oldpat = re.compile('a(?:b|(c|e){1,2}?|d)+?(.)')
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            pickled = pickle.dumps(oldpat, proto)
            newpat = pickle.loads(pickled)
            self.assertEqual(newpat, oldpat)

    def test_constants(self):
        self.assertEqual(re.I, re.IGNORECASE)
        self.assertEqual(re.L, re.LOCALE)
        self.assertEqual(re.M, re.MULTILINE)
        self.assertEqual(re.S, re.DOTALL)
        self.assertEqual(re.X, re.VERBOSE)

    def test_flags(self):
        for flag in [re.I, re.M, re.X, re.S, re.L]:
            self.assertTrue(re.compile('^pattern$', flag))

    def test_sre_character_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255]:
            self.assertTrue(re.match(r"\%03o" % i, chr(i)))
            self.assertTrue(re.match(r"\%03o0" % i, chr(i)+"0"))
            self.assertTrue(re.match(r"\%03o8" % i, chr(i)+"8"))
            self.assertTrue(re.match(r"\x%02x" % i, chr(i)))
            self.assertTrue(re.match(r"\x%02x0" % i, chr(i)+"0"))
            self.assertTrue(re.match(r"\x%02xz" % i, chr(i)+"z"))
        self.assertRaises(re.error, re.match, "\911", "")

    def test_sre_character_class_literals(self):
        for i in [0, 8, 16, 32, 64, 127, 128, 255]:
            self.assertTrue(re.match(r"[\%03o]" % i, chr(i)))
            self.assertTrue(re.match(r"[\%03o0]" % i, chr(i)))
            self.assertTrue(re.match(r"[\%03o8]" % i, chr(i)))
            self.assertTrue(re.match(r"[\x%02x]" % i, chr(i)))
            self.assertTrue(re.match(r"[\x%02x0]" % i, chr(i)))
            self.assertTrue(re.match(r"[\x%02xz]" % i, chr(i)))
        self.assertRaises(re.error, re.match, "[\911]", "")

    def test_bug_113254(self):
        self.assertEqual(re.match(r'(a)|(b)', 'b').start(1), -1)
        self.assertEqual(re.match(r'(a)|(b)', 'b').end(1), -1)
        self.assertEqual(re.match(r'(a)|(b)', 'b').span(1), (-1, -1))

    def test_bug_527371(self):
        # bug described in patches 527371/672491
        self.assertIsNone(re.match(r'(a)?a','a').lastindex)
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

    @requires_unicode
    def test_bug_612074(self):
        pat=u"["+re.escape(unichr(0x2039))+u"]"
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

    @requires_unicode
    def test_bug_764548(self):
        # bug 764548, re.compile() barfs on str/unicode subclasses
        class my_unicode(unicode): pass
        pat = re.compile(my_unicode("abc"))
        self.assertIsNone(pat.match("xyz"))

    def test_finditer(self):
        iter = re.finditer(r":+", "a:b::c:::d")
        self.assertEqual([item.group(0) for item in iter],
                         [":", "::", ":::"])

    @requires_unicode
    def test_bug_926075(self):
        self.assertIsNot(re.compile('bug_926075'),
                         re.compile(u'bug_926075'))

    @requires_unicode
    def test_bug_931848(self):
        pattern = u(r"[\u002E\u3002\uFF0E\uFF61]")
        self.assertEqual(re.compile(pattern).split("a.b.c"),
                         ['a','b','c'])

    def test_bug_581080(self):
        iter = re.finditer(r"\s", "a b")
        self.assertEqual(iter.next().span(), (1,2))
        self.assertRaises(StopIteration, iter.next)

        scanner = re.compile(r"\s").scanner("a b")
        self.assertEqual(scanner.search().span(), (1, 2))
        self.assertIsNone(scanner.search())

    def test_bug_817234(self):
        iter = re.finditer(r".*", "asdf")
        self.assertEqual(iter.next().span(), (0, 4))
        self.assertEqual(iter.next().span(), (4, 4))
        self.assertRaises(StopIteration, iter.next)

    @requires_unicode
    def test_bug_6561(self):
        # '\d' should match characters in Unicode category 'Nd'
        # (Number, Decimal Digit), but not those in 'Nl' (Number,
        # Letter) or 'No' (Number, Other).
        decimal_digits = [
            unichr(0x0037), # '\N{DIGIT SEVEN}', category 'Nd'
            unichr(0x0e58), # '\N{THAI DIGIT SIX}', category 'Nd'
            unichr(0xff10), # '\N{FULLWIDTH DIGIT ZERO}', category 'Nd'
            ]
        for x in decimal_digits:
            self.assertEqual(re.match('^\d$', x, re.UNICODE).group(0), x)

        not_decimal_digits = [
            unichr(0x2165), # '\N{ROMAN NUMERAL SIX}', category 'Nl'
            unichr(0x3039), # '\N{HANGZHOU NUMERAL TWENTY}', category 'Nl'
            unichr(0x2082), # '\N{SUBSCRIPT TWO}', category 'No'
            unichr(0x32b4), # '\N{CIRCLED NUMBER THIRTY NINE}', category 'No'
            ]
        for x in not_decimal_digits:
            self.assertIsNone(re.match('^\d$', x, re.UNICODE))

    def test_empty_array(self):
        # SF buf 1647541
        import array
        typecodes = 'cbBhHiIlLfd'
        if have_unicode:
            typecodes += 'u'
        for typecode in typecodes:
            a = array.array(typecode)
            self.assertIsNone(re.compile("bla").match(a))
            self.assertEqual(re.compile("").match(a).groups(), ())

    @requires_unicode
    def test_inline_flags(self):
        # Bug #1700
        upper_char = unichr(0x1ea0) # Latin Capital Letter A with Dot Bellow
        lower_char = unichr(0x1ea1) # Latin Small Letter A with Dot Bellow

        p = re.compile(upper_char, re.I | re.U)
        q = p.match(lower_char)
        self.assertTrue(q)

        p = re.compile(lower_char, re.I | re.U)
        q = p.match(upper_char)
        self.assertTrue(q)

        p = re.compile('(?i)' + upper_char, re.U)
        q = p.match(lower_char)
        self.assertTrue(q)

        p = re.compile('(?i)' + lower_char, re.U)
        q = p.match(upper_char)
        self.assertTrue(q)

        p = re.compile('(?iu)' + upper_char)
        q = p.match(lower_char)
        self.assertTrue(q)

        p = re.compile('(?iu)' + lower_char)
        q = p.match(upper_char)
        self.assertTrue(q)

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

    @precisionbigmemtest(size=_2G, memuse=1)
    def test_large_search(self, size):
        # Issue #10182: indices were 32-bit-truncated.
        s = 'a' * size
        m = re.search('$', s)
        self.assertIsNotNone(m)
        self.assertEqual(m.start(), size)
        self.assertEqual(m.end(), size)

    # The huge memuse is because of re.sub() using a list and a join()
    # to create the replacement result.
    @precisionbigmemtest(size=_2G, memuse=16 + 2)
    def test_large_subn(self, size):
        # Issue #10182: indices were 32-bit-truncated.
        s = 'a' * size
        r, n = re.subn('', '', s)
        self.assertEqual(r, s)
        self.assertEqual(n, size + 1)


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
        with self.assertRaisesRegexp(sre_constants.error, '<foo>'):
            re.compile('(?P=<foo>)')

    def test_group_name_in_exception(self):
        # Issue 17341: Poor error message when compiling invalid regex
        with self.assertRaisesRegexp(sre_constants.error, '\?foo'):
            re.compile('(?P<?foo>)')

    def test_issue17998(self):
        for reps in '*', '+', '?', '{1}':
            for mod in '', '?':
                pattern = '.' + reps + mod + 'yz'
                self.assertEqual(re.compile(pattern, re.S).findall('xyz'),
                                 ['xyz'], msg=pattern)
                if have_unicode:
                    pattern = unicode(pattern)
                    self.assertEqual(re.compile(pattern, re.S).findall(u'xyz'),
                                     [u'xyz'], msg=pattern)


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
        pat = r'(\.)(?:[ch]|py)(?(1)$|: )'
        with captured_stdout() as out:
            re.compile(pat, re.DEBUG)
        dump = '''\
subpattern 1
  literal 46
subpattern None
  branch
    in
      literal 99
      literal 104
  or
    literal 112
    literal 121
subpattern None
  groupref_exists 1
    at at_end
  else
    literal 58
    literal 32
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
            pat.search(string='abracadabra', pos=3, endpos=10).span(), (7, 9))
        self.assertEqual(
            pat.findall(string='abracadabra', pos=3, endpos=10), ['ab'])
        self.assertEqual(
            pat.split(string='abracadabra', maxsplit=1),
            ['', 'ab', 'racadabra'])

    def test_match_group_takes_long(self):
        self.assertEqual(re.match("(foo)", "foo").group(1L), "foo")
        self.assertRaises(IndexError, re.match("", "").group, sys.maxint + 1)

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


def run_re_tests():
    from test.re_tests import tests, SUCCEED, FAIL, SYNTAX_ERROR
    if verbose:
        print 'Running re_tests test suite'
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
            raise ValueError, ('Test tuples should have 3 or 5 fields', t)

        try:
            obj = re.compile(pattern)
        except re.error:
            if outcome == SYNTAX_ERROR: pass  # Expected a syntax error
            else:
                print '=== Syntax error:', t
        except KeyboardInterrupt: raise KeyboardInterrupt
        except:
            print '*** Unexpected error ***', t
            if verbose:
                traceback.print_exc(file=sys.stdout)
        else:
            try:
                result = obj.search(s)
            except re.error, msg:
                print '=== Unexpected exception', t, repr(msg)
            if outcome == SYNTAX_ERROR:
                # This should have been a syntax error; forget it.
                pass
            elif outcome == FAIL:
                if result is None: pass   # No match, as expected
                else: print '=== Succeeded incorrectly', t
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
                        print '=== grouping error', t,
                        print repr(repl) + ' should be ' + repr(expected)
                else:
                    print '=== Failed incorrectly', t

                # Try the match on a unicode string, and check that it
                # still succeeds.
                try:
                    result = obj.search(unicode(s, "latin-1"))
                    if result is None:
                        print '=== Fails on unicode match', t
                except NameError:
                    continue # 1.5.2
                except TypeError:
                    continue # unicode test case

                # Try the match on a unicode pattern, and check that it
                # still succeeds.
                obj=re.compile(unicode(pattern, "latin-1"))
                result = obj.search(s)
                if result is None:
                    print '=== Fails on unicode pattern match', t

                # Try the match with the search area limited to the extent
                # of the match and see if it still succeeds.  \B will
                # break (because it won't match at the end or start of a
                # string), so we'll ignore patterns that feature it.

                if pattern[:2] != '\\B' and pattern[-2:] != '\\B' \
                               and result is not None:
                    obj = re.compile(pattern)
                    result = obj.search(s, result.start(0), result.end(0) + 1)
                    if result is None:
                        print '=== Failed on range-limited match', t

                # Try the match with IGNORECASE enabled, and check that it
                # still succeeds.
                obj = re.compile(pattern, re.IGNORECASE)
                result = obj.search(s)
                if result is None:
                    print '=== Fails on case-insensitive match', t

                # Try the match with LOCALE enabled, and check that it
                # still succeeds.
                obj = re.compile(pattern, re.LOCALE)
                result = obj.search(s)
                if result is None:
                    print '=== Fails on locale-sensitive match', t

                # Try the match with UNICODE locale enabled, and check
                # that it still succeeds.
                obj = re.compile(pattern, re.UNICODE)
                result = obj.search(s)
                if result is None:
                    print '=== Fails on unicode-sensitive match', t

def test_main():
    run_unittest(ReTests)
    run_re_tests()

if __name__ == "__main__":
    test_main()
