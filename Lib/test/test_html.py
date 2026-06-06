"""
Tests for the html module functions.
"""

import unittest
from test.support import import_helper

py_html = import_helper.import_fresh_module('html', blocked=['_html'])
c_html = import_helper.import_fresh_module('html', fresh=['_html'])


class HtmlTestsMixin:
    # Subclasses set ``html`` to the pure-Python or C-accelerated module.
    html = None

    def test_escape(self):
        escape = self.html.escape
        self.assertEqual(
            escape('\'<script>"&foo;"</script>\''),
            '&#x27;&lt;script&gt;&quot;&amp;foo;&quot;&lt;/script&gt;&#x27;')
        self.assertEqual(
            escape('\'<script>"&foo;"</script>\'', False),
            '\'&lt;script&gt;"&amp;foo;"&lt;/script&gt;\'')

    def test_escape_quote_flag(self):
        escape = self.html.escape
        self.assertEqual(escape('"\'', quote=True), '&quot;&#x27;')
        self.assertEqual(escape('"\''), '&quot;&#x27;')
        self.assertEqual(escape('"\'', quote=False), '"\'')
        self.assertEqual(escape('"\'', False), '"\'')

    def test_escape_no_specials_returned_unchanged(self):
        for s in ['', 'a', 'plain text', 'x' * 100, 'caf\xe9 r\xe9sum\xe9',
                  '☃ snowman', '\U0001F600 emoji']:
            self.assertEqual(self.html.escape(s), s)

    def test_escape_specials_at_every_offset(self):
        # Exercise the word-at-a-time (SWAR) scan boundaries and tail loop by
        # placing each special at every offset of a run crossing 8-byte words.
        escape = self.html.escape
        specials = {'&': '&amp;', '<': '&lt;', '>': '&gt;',
                    '"': '&quot;', "'": '&#x27;'}
        for ch, rep in specials.items():
            for pad in range(0, 20):
                s = 'a' * pad + ch + 'b' * pad
                self.assertEqual(escape(s), 'a' * pad + rep + 'b' * pad)

    def test_escape_adjacent_specials(self):
        self.assertEqual(self.html.escape('&<>"\'' * 5),
                         '&amp;&lt;&gt;&quot;&#x27;' * 5)

    def test_escape_multiple_kinds(self):
        escape = self.html.escape
        # 2-byte (UCS-2) and 4-byte (UCS-4) strings still escape ASCII specials.
        self.assertEqual(escape('☃ <b> & </b>'),
                         '☃ &lt;b&gt; &amp; &lt;/b&gt;')
        self.assertEqual(escape('\U0001F600<&>"\''),
                         '\U0001F600&lt;&amp;&gt;&quot;&#x27;')
        # Latin-1 high bytes must not be matched by the byte-wise scan.
        self.assertEqual(escape('\xe9\xff & \xe9'), '\xe9\xff &amp; \xe9')

    def test_escape_str_subclass_returns_true_str(self):
        class S(str):
            pass
        for s in ['no specials', 'a & b']:
            result = self.html.escape(S(s))
            self.assertEqual(result, self.html.escape(s))
            self.assertIs(type(result), str)

    def test_unescape(self):
        numeric_formats = ['&#%d', '&#%d;', '&#x%x', '&#x%x;']
        errmsg = 'unescape(%r) should have returned %r'
        def check(text, expected):
            self.assertEqual(self.html.unescape(text), expected,
                             msg=errmsg % (text, expected))
        def check_num(num, expected):
            for format in numeric_formats:
                text = format % num
                self.assertEqual(self.html.unescape(text), expected,
                                 msg=errmsg % (text, expected))
        # check text with no character references
        check('no character references', 'no character references')
        # check & followed by invalid chars
        check('&\n&\t& &&', '&\n&\t& &&')
        # check & followed by numbers and letters
        check('&0 &9 &a &0; &9; &a;', '&0 &9 &a &0; &9; &a;')
        # check incomplete entities at the end of the string
        for x in ['&', '&#', '&#x', '&#X', '&#y', '&#xy', '&#Xy']:
            check(x, x)
            check(x+';', x+';')
        # check several combinations of numeric character references,
        # possibly followed by different characters
        formats = ['&#%d', '&#%07d', '&#%d;', '&#%07d;',
                   '&#x%x', '&#x%06x', '&#x%x;', '&#x%06x;',
                   '&#x%X', '&#x%06X', '&#X%x;', '&#X%06x;']
        for num, char in zip([65, 97, 34, 38, 0x2603, 0x101234],
                             ['A', 'a', '"', '&', '☃', '\U00101234']):
            for s in formats:
                check(s % num, char)
                for end in [' ', 'X']:
                    check((s+end) % num, char+end)
        # check invalid code points
        for cp in [0xD800, 0xDB00, 0xDC00, 0xDFFF, 0x110000]:
            check_num(cp, '�')
        # check more invalid code points
        for cp in [0x1, 0xb, 0xe, 0x7f, 0xfffe, 0xffff, 0x10fffe, 0x10ffff]:
            check_num(cp, '')
        # check invalid numbers
        for num, ch in zip([0x0d, 0x80, 0x95, 0x9d], '\r€•\x9d'):
            check_num(num, ch)
        # check small numbers
        check_num(0, '�')
        check_num(9, '\t')
        # check a big number
        check_num(1000000000000000000, '�')
        # check that multiple trailing semicolons are handled correctly
        for e in ['&quot;;', '&#34;;', '&#x22;;', '&#X22;;']:
            check(e, '";')
        # check that semicolons in the middle don't create problems
        for e in ['&quot;quot;', '&#34;quot;', '&#x22;quot;', '&#X22;quot;']:
            check(e, '"quot;')
        # check triple adjacent charrefs
        for e in ['&quot', '&#34', '&#x22', '&#X22']:
            check(e*3, '"""')
            check((e+';')*3, '"""')
        # check that the case is respected
        for e in ['&amp', '&amp;', '&AMP', '&AMP;']:
            check(e, '&')
        for e in ['&Amp', '&Amp;']:
            check(e, e)
        # check that non-existent named entities are returned unchanged
        check('&svadilfari;', '&svadilfari;')
        # the following examples are in the html5 specs
        check('&notit', '¬it')
        check('&notit;', '¬it;')
        check('&notin', '¬in')
        check('&notin;', '∉')
        # a similar example with a long name
        check('&notReallyAnExistingNamedCharacterReference;',
              '¬ReallyAnExistingNamedCharacterReference;')
        # longest valid name
        check('&CounterClockwiseContourIntegral;', '∳')
        # check a charref that maps to two unicode chars
        check('&acE;', '∾̳')
        check('&acE', '&acE')
        # see #12888
        check('&#123; ' * 1050, '{ ' * 1050)
        # see #15156
        check('&Eacuteric&Eacute;ric&alphacentauri&alpha;centauri',
              'ÉricÉric&alphacentauriαcentauri')
        check('&co;', '&co;')

    def test_unescape_multiple_kinds(self):
        unescape = self.html.unescape
        # references embedded in 2-byte and 4-byte strings
        self.assertEqual(unescape('☃ &amp; &#62; &copy; x'),
                         '☃ & > \xa9 x')
        self.assertEqual(unescape('\U0001F600&amp;&#x41;&notin;'),
                         '\U0001F600&A∉')

    def test_unescape_long_text_with_sparse_refs(self):
        # exercise the bulk substring copy between references
        unescape = self.html.unescape
        s = 'x' * 5000 + '&amp;' + 'y' * 5000
        self.assertEqual(unescape(s), 'x' * 5000 + '&' + 'y' * 5000)
        self.assertEqual(unescape('a' * 5000), 'a' * 5000)

    def test_unescape_str_subclass(self):
        class S(str):
            pass
        self.assertEqual(self.html.unescape(S('no refs')), 'no refs')
        self.assertEqual(self.html.unescape(S('a &amp; b')), 'a & b')


class PyHtmlTests(HtmlTestsMixin, unittest.TestCase):
    html = py_html


@unittest.skipUnless(
    c_html is not None and getattr(c_html.escape, '__module__', None) == '_html',
    'requires the _html C accelerator')
class CHtmlTests(HtmlTestsMixin, unittest.TestCase):
    html = c_html


if __name__ == '__main__':
    unittest.main()
