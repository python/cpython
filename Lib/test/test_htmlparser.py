"""Tests for HTMLParser.py."""

import HTMLParser
import pprint
import sys
import test_support
import unittest


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

    def test_doctype_decl(self):
        inside = """\
DOCTYPE html [
  <!ELEMENT html - O EMPTY>
  <!ATTLIST html
      version CDATA #IMPLIED
      profile CDATA 'DublinCore'>
  <!NOTATION datatype SYSTEM 'http://xml.python.org/notations/python-module'>
  <!ENTITY myEntity 'internal parsed entity'>
  <!ENTITY anEntity SYSTEM 'http://xml.python.org/entities/something.xml'>
  <!ENTITY % paramEntity 'name|name|name'>
  %paramEntity;
  <!-- comment -->
]"""
        self._run_check("<!%s>" % inside, [
            ("decl", inside),
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
                                            ("d", "\txyz\n")])
                         ])
        self._run_check("""<a b='' c="">""", [
            ("starttag", "a", [("b", ""), ("c", "")]),
            ])

    def test_attr_entity_replacement(self):
        self._run_check("""<a b='&amp;&gt;&lt;&quot;&apos;'>""", [
            ("starttag", "a", [("b", "&><\"'")]),
            ])

    def test_attr_funky_names(self):
        self._run_check("""<a a.b='v' c:d=v e-f=v>""", [
            ("starttag", "a", [("a.b", "v"), ("c:d", "v"), ("e-f", "v")]),
            ])

    def test_illegal_declarations(self):
        self._parse_error('<!spacer type="block" height="25">')

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

    def test_starttag_junk_chars(self):
        self._parse_error("</>")
        self._parse_error("</$>")
        self._parse_error("</")
        self._parse_error("</a")
        self._parse_error("<a<a>")
        self._parse_error("</a<a>")
        self._parse_error("<!")
        self._parse_error("<a $>")
        self._parse_error("<a")
        self._parse_error("<a foo='bar'")
        self._parse_error("<a foo='bar")
        self._parse_error("<a foo='>'")
        self._parse_error("<a foo='>")
        self._parse_error("<a foo=>")

    def test_declaration_junk_chars(self):
        self._parse_error("<!DOCTYPE foo $ >")

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

    def test_get_starttag_text(self):
        s = """<foo:bar   \n   one="1"\ttwo=2   >"""
        self._run_check_extra(s, [
            ("starttag", "foo:bar", [("one", "1"), ("two", "2")]),
            ("starttag_text", s)])

    def test_cdata_content(self):
        s = """<script> <!-- not a comment --> &not-an-entity-ref; </script>"""
        self._run_check(s, [
            ("starttag", "script", []),
            ("data", " <!-- not a comment --> &not-an-entity-ref; "),
            ("endtag", "script"),
            ])
        s = """<script> <not a='start tag'> </script>"""
        self._run_check(s, [
            ("starttag", "script", []),
            ("data", " <not a='start tag'> "),
            ("endtag", "script"),
            ])


def test_main():
    test_support.run_unittest(HTMLParserTestCase)


if __name__ == "__main__":
    test_main()
