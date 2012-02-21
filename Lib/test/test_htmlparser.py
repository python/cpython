"""Tests for HTMLParser.py."""

import HTMLParser
import pprint
import unittest
from test import test_support


class EventCollector(HTMLParser.HTMLParser):

    def __init__(self):
        self.events = []
        self.append = self.events.append
        HTMLParser.HTMLParser.__init__(self)

    def get_events(self):
        # Normalize the list of events so that buffer artefacts don't
        # separate runs of contiguous characters.
        L = []
        prevtype = None
        for event in self.events:
            type = event[0]
            if type == prevtype == "data":
                L[-1] = ("data", L[-1][1] + event[1])
            else:
                L.append(event)
            prevtype = type
        self.events = L
        return L

    # structure markup

    def handle_starttag(self, tag, attrs):
        self.append(("starttag", tag, attrs))

    def handle_startendtag(self, tag, attrs):
        self.append(("startendtag", tag, attrs))

    def handle_endtag(self, tag):
        self.append(("endtag", tag))

    # all other markup

    def handle_comment(self, data):
        self.append(("comment", data))

    def handle_charref(self, data):
        self.append(("charref", data))

    def handle_data(self, data):
        self.append(("data", data))

    def handle_decl(self, data):
        self.append(("decl", data))

    def handle_entityref(self, data):
        self.append(("entityref", data))

    def handle_pi(self, data):
        self.append(("pi", data))

    def unknown_decl(self, decl):
        self.append(("unknown decl", decl))


class EventCollectorExtra(EventCollector):

    def handle_starttag(self, tag, attrs):
        EventCollector.handle_starttag(self, tag, attrs)
        self.append(("starttag_text", self.get_starttag_text()))


class TestCaseBase(unittest.TestCase):

    def _run_check(self, source, expected_events, collector=EventCollector):
        parser = collector()
        for s in source:
            parser.feed(s)
        parser.close()
        events = parser.get_events()
        if events != expected_events:
            self.fail("received events did not match expected events\n"
                      "Expected:\n" + pprint.pformat(expected_events) +
                      "\nReceived:\n" + pprint.pformat(events))

    def _run_check_extra(self, source, events):
        self._run_check(source, events, EventCollectorExtra)

    def _parse_error(self, source):
        def parse(source=source):
            parser = HTMLParser.HTMLParser()
            parser.feed(source)
            parser.close()
        self.assertRaises(HTMLParser.HTMLParseError, parse)


class HTMLParserTestCase(TestCaseBase):

    def test_processing_instruction_only(self):
        self._run_check("<?processing instruction>", [
            ("pi", "processing instruction"),
            ])
        self._run_check("<?processing instruction ?>", [
            ("pi", "processing instruction ?"),
            ])

    def test_simple_html(self):
        self._run_check("""
<!DOCTYPE html PUBLIC 'foo'>
<HTML>&entity;&#32;
<!--comment1a
-></foo><bar>&lt;<?pi?></foo<bar
comment1b-->
<Img sRc='Bar' isMAP>sample
text
&#x201C;
<!--comment2a-- --comment2b-->
</Html>
""", [
    ("data", "\n"),
    ("decl", "DOCTYPE html PUBLIC 'foo'"),
    ("data", "\n"),
    ("starttag", "html", []),
    ("entityref", "entity"),
    ("charref", "32"),
    ("data", "\n"),
    ("comment", "comment1a\n-></foo><bar>&lt;<?pi?></foo<bar\ncomment1b"),
    ("data", "\n"),
    ("starttag", "img", [("src", "Bar"), ("ismap", None)]),
    ("data", "sample\ntext\n"),
    ("charref", "x201C"),
    ("data", "\n"),
    ("comment", "comment2a-- --comment2b"),
    ("data", "\n"),
    ("endtag", "html"),
    ("data", "\n"),
    ])

    def test_unclosed_entityref(self):
        self._run_check("&entityref foo", [
            ("entityref", "entityref"),
            ("data", " foo"),
            ])

    def test_bad_nesting(self):
        # Strangely, this *is* supposed to test that overlapping
        # elements are allowed.  HTMLParser is more geared toward
        # lexing the input that parsing the structure.
        self._run_check("<a><b></a></b>", [
            ("starttag", "a", []),
            ("starttag", "b", []),
            ("endtag", "a"),
            ("endtag", "b"),
            ])

    def test_bare_ampersands(self):
        self._run_check("this text & contains & ampersands &", [
            ("data", "this text & contains & ampersands &"),
            ])

    def test_bare_pointy_brackets(self):
        self._run_check("this < text > contains < bare>pointy< brackets", [
            ("data", "this < text > contains < bare>pointy< brackets"),
            ])

    def test_illegal_declarations(self):
        self._run_check('<!spacer type="block" height="25">',
                        [('comment', 'spacer type="block" height="25"')])

    def test_starttag_end_boundary(self):
        self._run_check("""<a b='<'>""", [("starttag", "a", [("b", "<")])])
        self._run_check("""<a b='>'>""", [("starttag", "a", [("b", ">")])])

    def test_buffer_artefacts(self):
        output = [("starttag", "a", [("b", "<")])]
        self._run_check(["<a b='<'>"], output)
        self._run_check(["<a ", "b='<'>"], output)
        self._run_check(["<a b", "='<'>"], output)
        self._run_check(["<a b=", "'<'>"], output)
        self._run_check(["<a b='<", "'>"], output)
        self._run_check(["<a b='<'", ">"], output)

        output = [("starttag", "a", [("b", ">")])]
        self._run_check(["<a b='>'>"], output)
        self._run_check(["<a ", "b='>'>"], output)
        self._run_check(["<a b", "='>'>"], output)
        self._run_check(["<a b=", "'>'>"], output)
        self._run_check(["<a b='>", "'>"], output)
        self._run_check(["<a b='>'", ">"], output)

        output = [("comment", "abc")]
        self._run_check(["", "<!--abc-->"], output)
        self._run_check(["<", "!--abc-->"], output)
        self._run_check(["<!", "--abc-->"], output)
        self._run_check(["<!-", "-abc-->"], output)
        self._run_check(["<!--", "abc-->"], output)
        self._run_check(["<!--a", "bc-->"], output)
        self._run_check(["<!--ab", "c-->"], output)
        self._run_check(["<!--abc", "-->"], output)
        self._run_check(["<!--abc-", "->"], output)
        self._run_check(["<!--abc--", ">"], output)
        self._run_check(["<!--abc-->", ""], output)

    def test_starttag_junk_chars(self):
        self._run_check("</>", [])
        self._run_check("</$>", [('comment', '$')])
        self._run_check("</", [('data', '</')])
        self._run_check("</a", [('data', '</a')])
        # XXX this might be wrong
        self._run_check("<a<a>", [('data', '<a'), ('starttag', 'a', [])])
        self._run_check("</a<a>", [('endtag', 'a<a')])
        self._run_check("<!", [('data', '<!')])
        self._run_check("<a", [('data', '<a')])
        self._run_check("<a foo='bar'", [('data', "<a foo='bar'")])
        self._run_check("<a foo='bar", [('data', "<a foo='bar")])
        self._run_check("<a foo='>'", [('data', "<a foo='>'")])
        self._run_check("<a foo='>", [('data', "<a foo='>")])

    def test_valid_doctypes(self):
        # from http://www.w3.org/QA/2002/04/valid-dtd-list.html
        dtds = ['HTML',  # HTML5 doctype
                ('HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" '
                 '"http://www.w3.org/TR/html4/strict.dtd"'),
                ('HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" '
                 '"http://www.w3.org/TR/html4/loose.dtd"'),
                ('html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" '
                 '"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"'),
                ('html PUBLIC "-//W3C//DTD XHTML 1.0 Frameset//EN" '
                 '"http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd"'),
                ('math PUBLIC "-//W3C//DTD MathML 2.0//EN" '
                 '"http://www.w3.org/Math/DTD/mathml2/mathml2.dtd"'),
                ('html PUBLIC "-//W3C//DTD '
                 'XHTML 1.1 plus MathML 2.0 plus SVG 1.1//EN" '
                 '"http://www.w3.org/2002/04/xhtml-math-svg/xhtml-math-svg.dtd"'),
                ('svg PUBLIC "-//W3C//DTD SVG 1.1//EN" '
                 '"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd"'),
                'html PUBLIC "-//IETF//DTD HTML 2.0//EN"',
                'html PUBLIC "-//W3C//DTD HTML 3.2 Final//EN"']
        for dtd in dtds:
            self._run_check("<!DOCTYPE %s>" % dtd,
                            [('decl', 'DOCTYPE ' + dtd)])

    def test_slashes_in_starttag(self):
        self._run_check('<a foo="var"/>', [('startendtag', 'a', [('foo', 'var')])])
        html = ('<img width=902 height=250px '
                'src="/sites/default/files/images/homepage/foo.jpg" '
                '/*what am I doing here*/ />')
        expected = [(
            'startendtag', 'img',
            [('width', '902'), ('height', '250px'),
             ('src', '/sites/default/files/images/homepage/foo.jpg'),
             ('*what', None), ('am', None), ('i', None),
             ('doing', None), ('here*', None)]
        )]
        self._run_check(html, expected)
        html = ('<a / /foo/ / /=/ / /bar/ / />'
                '<a / /foo/ / /=/ / /bar/ / >')
        expected = [
            ('startendtag', 'a', [('foo', None), ('=', None), ('bar', None)]),
            ('starttag', 'a', [('foo', None), ('=', None), ('bar', None)])
        ]
        self._run_check(html, expected)

    def test_declaration_junk_chars(self):
        self._run_check("<!DOCTYPE foo $ >", [('decl', 'DOCTYPE foo $ ')])

    def test_startendtag(self):
        self._run_check("<p/>", [
            ("startendtag", "p", []),
            ])
        self._run_check("<p></p>", [
            ("starttag", "p", []),
            ("endtag", "p"),
            ])
        self._run_check("<p><img src='foo' /></p>", [
            ("starttag", "p", []),
            ("startendtag", "img", [("src", "foo")]),
            ("endtag", "p"),
            ])

    def test_invalid_end_tags(self):
        # A collection of broken end tags. <br> is used as separator.
        # see http://www.w3.org/TR/html5/tokenization.html#end-tag-open-state
        # and #13993
        html = ('<br></label</p><br></div end tmAd-leaderBoard><br></<h4><br>'
                '</li class="unit"><br></li\r\n\t\t\t\t\t\t</ul><br></><br>')
        expected = [('starttag', 'br', []),
                    # < is part of the name, / is discarded, p is an attribute
                    ('endtag', 'label<'),
                    ('starttag', 'br', []),
                    # text and attributes are discarded
                    ('endtag', 'div'),
                    ('starttag', 'br', []),
                    # comment because the first char after </ is not a-zA-Z
                    ('comment', '<h4'),
                    ('starttag', 'br', []),
                    # attributes are discarded
                    ('endtag', 'li'),
                    ('starttag', 'br', []),
                    # everything till ul (included) is discarded
                    ('endtag', 'li'),
                    ('starttag', 'br', []),
                    # </> is ignored
                    ('starttag', 'br', [])]
        self._run_check(html, expected)

    def test_broken_invalid_end_tag(self):
        # This is technically wrong (the "> shouldn't be included in the 'data')
        # but is probably not worth fixing it (in addition to all the cases of
        # the previous test, it would require a full attribute parsing).
        # see #13993
        html = '<b>This</b attr=">"> confuses the parser'
        expected = [('starttag', 'b', []),
                    ('data', 'This'),
                    ('endtag', 'b'),
                    ('data', '"> confuses the parser')]
        self._run_check(html, expected)

    def test_get_starttag_text(self):
        s = """<foo:bar   \n   one="1"\ttwo=2   >"""
        self._run_check_extra(s, [
            ("starttag", "foo:bar", [("one", "1"), ("two", "2")]),
            ("starttag_text", s)])

    def test_cdata_content(self):
        contents = [
            '<!-- not a comment --> &not-an-entity-ref;',
            "<not a='start tag'>",
            '<a href="" /> <p> <span></span>',
            'foo = "</scr" + "ipt>";',
            'foo = "</SCRIPT" + ">";',
            'foo = <\n/script> ',
            '<!-- document.write("</scr" + "ipt>"); -->',
            ('\n//<![CDATA[\n'
             'document.write(\'<s\'+\'cript type="text/javascript" '
             'src="http://www.example.org/r=\'+new '
             'Date().getTime()+\'"><\\/s\'+\'cript>\');\n//]]>'),
            '\n<!-- //\nvar foo = 3.14;\n// -->\n',
            'foo = "</sty" + "le>";',
            u'<!-- \u2603 -->',
            # these two should be invalid according to the HTML 5 spec,
            # section 8.1.2.2
            #'foo = </\nscript>',
            #'foo = </ script>',
        ]
        elements = ['script', 'style', 'SCRIPT', 'STYLE', 'Script', 'Style']
        for content in contents:
            for element in elements:
                element_lower = element.lower()
                s = u'<{element}>{content}</{element}>'.format(element=element,
                                                               content=content)
                self._run_check(s, [("starttag", element_lower, []),
                                    ("data", content),
                                    ("endtag", element_lower)])

    def test_cdata_with_closing_tags(self):
        # see issue #13358
        # make sure that HTMLParser calls handle_data only once for each CDATA.
        # The normal event collector normalizes the events in get_events,
        # so we override it to return the original list of events.
        class Collector(EventCollector):
            def get_events(self):
                return self.events

        content = """<!-- not a comment --> &not-an-entity-ref;
                  <a href="" /> </p><p> &amp; <span></span></style>
                  '</script' + '>' </html> </head> </scripter>!"""
        for element in [' script', 'script ', ' script ',
                        '\nscript', 'script\n', '\nscript\n']:
            s = u'<script>{content}</{element}>'.format(element=element,
                                                        content=content)
            self._run_check(s, [("starttag", "script", []),
                                ("data", content),
                                ("endtag", "script")],
                            collector=Collector)

    def test_malformatted_charref(self):
        self._run_check("<p>&#bad;</p>", [
            ("starttag", "p", []),
            ("data", "&#bad;"),
            ("endtag", "p"),
        ])

    def test_unescape_function(self):
        parser = HTMLParser.HTMLParser()
        self.assertEqual(parser.unescape('&#bad;'),'&#bad;')
        self.assertEqual(parser.unescape('&#0038;'),'&')



class AttributesTestCase(TestCaseBase):

    def test_attr_syntax(self):
        output = [
          ("starttag", "a", [("b", "v"), ("c", "v"), ("d", "v"), ("e", None)])
        ]
        self._run_check("""<a b='v' c="v" d=v e>""", output)
        self._run_check("""<a  b = 'v' c = "v" d = v e>""", output)
        self._run_check("""<a\nb\n=\n'v'\nc\n=\n"v"\nd\n=\nv\ne>""", output)
        self._run_check("""<a\tb\t=\t'v'\tc\t=\t"v"\td\t=\tv\te>""", output)

    def test_attr_values(self):
        self._run_check("""<a b='xxx\n\txxx' c="yyy\t\nyyy" d='\txyz\n'>""",
                        [("starttag", "a", [("b", "xxx\n\txxx"),
                                            ("c", "yyy\t\nyyy"),
                                            ("d", "\txyz\n")])])
        self._run_check("""<a b='' c="">""",
                        [("starttag", "a", [("b", ""), ("c", "")])])
        # Regression test for SF patch #669683.
        self._run_check("<e a=rgb(1,2,3)>",
                        [("starttag", "e", [("a", "rgb(1,2,3)")])])
        # Regression test for SF bug #921657.
        self._run_check(
            "<a href=mailto:xyz@example.com>",
            [("starttag", "a", [("href", "mailto:xyz@example.com")])])

    def test_attr_nonascii(self):
        # see issue 7311
        self._run_check(
            u"<img src=/foo/bar.png alt=\u4e2d\u6587>",
            [("starttag", "img", [("src", "/foo/bar.png"),
                                  ("alt", u"\u4e2d\u6587")])])
        self._run_check(
            u"<a title='\u30c6\u30b9\u30c8' href='\u30c6\u30b9\u30c8.html'>",
            [("starttag", "a", [("title", u"\u30c6\u30b9\u30c8"),
                                ("href", u"\u30c6\u30b9\u30c8.html")])])
        self._run_check(
            u'<a title="\u30c6\u30b9\u30c8" href="\u30c6\u30b9\u30c8.html">',
            [("starttag", "a", [("title", u"\u30c6\u30b9\u30c8"),
                                ("href", u"\u30c6\u30b9\u30c8.html")])])

    def test_attr_entity_replacement(self):
        self._run_check(
            "<a b='&amp;&gt;&lt;&quot;&apos;'>",
            [("starttag", "a", [("b", "&><\"'")])])

    def test_attr_funky_names(self):
        self._run_check(
            "<a a.b='v' c:d=v e-f=v>",
            [("starttag", "a", [("a.b", "v"), ("c:d", "v"), ("e-f", "v")])])
        self._run_check(
            "<a $><b $=%><c \=/>",
            [("starttag", "a", [("$", None)]),
             ("starttag", "b", [("$", "%")]),
             ("starttag", "c", [("\\", "/")])])

    def test_entityrefs_in_attributes(self):
        self._run_check(
            "<html foo='&euro;&amp;&#97;&#x61;&unsupported;'>",
            [("starttag", "html", [("foo", u"\u20AC&aa&unsupported;")])])

    def test_entities_in_attribute_value(self):
        # see #1200313
        for entity in ['&', '&amp;', '&#38;', '&#x26;']:
            self._run_check('<a href="%s">' % entity,
                            [("starttag", "a", [("href", "&")])])
            self._run_check("<a href='%s'>" % entity,
                            [("starttag", "a", [("href", "&")])])
            self._run_check("<a href=%s>" % entity,
                            [("starttag", "a", [("href", "&")])])

    def test_malformed_attributes(self):
        # see #13357
        html = (
            "<a href=test'style='color:red;bad1'>test - bad1</a>"
            "<a href=test'+style='color:red;ba2'>test - bad2</a>"
            "<a href=test'&nbsp;style='color:red;bad3'>test - bad3</a>"
            "<a href = test'&nbsp;style='color:red;bad4'  >test - bad4</a>"
        )
        expected = [
            ('starttag', 'a', [('href', "test'style='color:red;bad1'")]),
            ('data', 'test - bad1'), ('endtag', 'a'),
            ('starttag', 'a', [('href', "test'+style='color:red;ba2'")]),
            ('data', 'test - bad2'), ('endtag', 'a'),
            ('starttag', 'a', [('href', u"test'\xa0style='color:red;bad3'")]),
            ('data', 'test - bad3'), ('endtag', 'a'),
            ('starttag', 'a', [('href', u"test'\xa0style='color:red;bad4'")]),
            ('data', 'test - bad4'), ('endtag', 'a')
        ]
        self._run_check(html, expected)

    def test_malformed_adjacent_attributes(self):
        # see #12629
        self._run_check('<x><y z=""o"" /></x>',
                        [('starttag', 'x', []),
                            ('startendtag', 'y', [('z', ''), ('o""', None)]),
                            ('endtag', 'x')])
        self._run_check('<x><y z="""" /></x>',
                        [('starttag', 'x', []),
                            ('startendtag', 'y', [('z', ''), ('""', None)]),
                            ('endtag', 'x')])

    # see #755670 for the following 3 tests
    def test_adjacent_attributes(self):
        self._run_check('<a width="100%"cellspacing=0>',
                        [("starttag", "a",
                          [("width", "100%"), ("cellspacing","0")])])

        self._run_check('<a id="foo"class="bar">',
                        [("starttag", "a",
                          [("id", "foo"), ("class","bar")])])

    def test_missing_attribute_value(self):
        self._run_check('<a v=>',
                        [("starttag", "a", [("v", "")])])

    def test_javascript_attribute_value(self):
        self._run_check("<a href=javascript:popup('/popup/help.html')>",
                        [("starttag", "a",
                          [("href", "javascript:popup('/popup/help.html')")])])

    def test_end_tag_in_attribute_value(self):
        # see #1745761
        self._run_check("<a href='http://www.example.org/\">;'>spam</a>",
                        [("starttag", "a",
                          [("href", "http://www.example.org/\">;")]),
                         ("data", "spam"), ("endtag", "a")])

    def test_comments(self):
        html = ("<!-- I'm a valid comment -->"
                '<!--me too!-->'
                '<!------>'
                '<!---->'
                '<!----I have many hyphens---->'
                '<!-- I have a > in the middle -->'
                '<!-- and I have -- in the middle! -->')
        expected = [('comment', " I'm a valid comment "),
                    ('comment', 'me too!'),
                    ('comment', '--'),
                    ('comment', ''),
                    ('comment', '--I have many hyphens--'),
                    ('comment', ' I have a > in the middle '),
                    ('comment', ' and I have -- in the middle! ')]
        self._run_check(html, expected)

    def test_broken_comments(self):
        html = ('<! not really a comment >'
                '<! not a comment either -->'
                '<! -- close enough -->'
                '<!><!<-- this was an empty comment>'
                '<!!! another bogus comment !!!>')
        expected = [
            ('comment', ' not really a comment '),
            ('comment', ' not a comment either --'),
            ('comment', ' -- close enough --'),
            ('comment', ''),
            ('comment', '<-- this was an empty comment'),
            ('comment', '!! another bogus comment !!!'),
        ]
        self._run_check(html, expected)

    def test_condcoms(self):
        html = ('<!--[if IE & !(lte IE 8)]>aren\'t<![endif]-->'
                '<!--[if IE 8]>condcoms<![endif]-->'
                '<!--[if lte IE 7]>pretty?<![endif]-->')
        expected = [('comment', "[if IE & !(lte IE 8)]>aren't<![endif]"),
                    ('comment', '[if IE 8]>condcoms<![endif]'),
                    ('comment', '[if lte IE 7]>pretty?<![endif]')]
        self._run_check(html, expected)

    def test_broken_condcoms(self):
        # these condcoms are missing the '--' after '<!' and before the '>'
        html = ('<![if !(IE)]>broken condcom<![endif]>'
                '<![if ! IE]><link href="favicon.tiff"/><![endif]>'
                '<![if !IE 6]><img src="firefox.png" /><![endif]>'
                '<![if !ie 6]><b>foo</b><![endif]>'
                '<![if (!IE)|(lt IE 9)]><img src="mammoth.bmp" /><![endif]>')
        # According to the HTML5 specs sections "8.2.4.44 Bogus comment state"
        # and "8.2.4.45 Markup declaration open state", comment tokens should
        # be emitted instead of 'unknown decl', but calling unknown_decl
        # provides more flexibility.
        # See also Lib/_markupbase.py:parse_declaration
        expected = [
            ('unknown decl', 'if !(IE)'),
            ('data', 'broken condcom'),
            ('unknown decl', 'endif'),
            ('unknown decl', 'if ! IE'),
            ('startendtag', 'link', [('href', 'favicon.tiff')]),
            ('unknown decl', 'endif'),
            ('unknown decl', 'if !IE 6'),
            ('startendtag', 'img', [('src', 'firefox.png')]),
            ('unknown decl', 'endif'),
            ('unknown decl', 'if !ie 6'),
            ('starttag', 'b', []),
            ('data', 'foo'),
            ('endtag', 'b'),
            ('unknown decl', 'endif'),
            ('unknown decl', 'if (!IE)|(lt IE 9)'),
            ('startendtag', 'img', [('src', 'mammoth.bmp')]),
            ('unknown decl', 'endif')
        ]
        self._run_check(html, expected)


def test_main():
    test_support.run_unittest(HTMLParserTestCase, AttributesTestCase)


if __name__ == "__main__":
    test_main()
