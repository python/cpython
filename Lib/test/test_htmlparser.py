"""Tests for HTMLParser.py."""

import HTMLParser
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


class EventCollectorExtra(EventCollector):

    def handle_starttag(self, tag, attrs):
        EventCollector.handle_starttag(self, tag, attrs)
        self.append(("starttag_text", self.get_starttag_text()))


class TestCaseBase(unittest.TestCase):

    # Constant pieces of source and events
    prologue = ""
    epilogue = ""
    initial_events = []
    final_events = []

    def _run_check(self, source, events, collector=EventCollector):
        parser = collector()
        parser.feed(self.prologue)
        for s in source:
            parser.feed(s)
        for c in self.epilogue:
            parser.feed(c)
        parser.close()
        self.assert_(parser.get_events() ==
                     self.initial_events + events + self.final_events,
                     parser.get_events())

    def _run_check_extra(self, source, events):
        self._run_check(source, events, EventCollectorExtra)

    def _parse_error(self, source):
        def parse(source=source):
            parser = HTMLParser.HTMLParser()
            parser.feed(source)
            parser.close()
        self.assertRaises(HTMLParser.HTMLParseError, parse)


class HTMLParserTestCase(TestCaseBase):

    def check_processing_instruction_only(self):
        self._run_check("<?processing instruction>", [
            ("pi", "processing instruction"),
            ])

    def check_simple_html(self):
        self._run_check("""
<!DOCTYPE html PUBLIC 'foo'>
<HTML>&entity;&#32;
<!--comment1a
-></foo><bar>&lt;<?pi?></foo<bar
comment1b-->
<Img sRc='Bar' isMAP>sample
text
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
    ("comment", "comment2a-- --comment2b"),
    ("data", "\n"),
    ("endtag", "html"),
    ("data", "\n"),
    ])

    def check_bad_nesting(self):
        self._run_check("<a><b></a></b>", [
            ("starttag", "a", []),
            ("starttag", "b", []),
            ("endtag", "a"),
            ("endtag", "b"),
            ])

    def check_attr_syntax(self):
        output = [
          ("starttag", "a", [("b", "v"), ("c", "v"), ("d", "v"), ("e", None)])
          ]
        self._run_check("""<a b='v' c="v" d=v e>""", output)
        self._run_check("""<a  b = 'v' c = "v" d = v e>""", output)
        self._run_check("""<a\nb\n=\n'v'\nc\n=\n"v"\nd\n=\nv\ne>""", output)
        self._run_check("""<a\tb\t=\t'v'\tc\t=\t"v"\td\t=\tv\te>""", output)

    def check_attr_values(self):
        self._run_check("""<a b='xxx\n\txxx' c="yyy\t\nyyy" d='\txyz\n'>""",
                        [("starttag", "a", [("b", "xxx\n\txxx"),
                                            ("c", "yyy\t\nyyy"),
                                            ("d", "\txyz\n")])
                         ])
        self._run_check("""<a b='' c="">""", [
            ("starttag", "a", [("b", ""), ("c", "")]),
            ])

    def check_attr_entity_replacement(self):
        self._run_check("""<a b='&amp;&gt;&lt;&quot;&apos;'>""", [
            ("starttag", "a", [("b", "&><\"'")]),
            ])

    def check_attr_funky_names(self):
        self._run_check("""<a a.b='v' c:d=v e-f=v>""", [
            ("starttag", "a", [("a.b", "v"), ("c:d", "v"), ("e-f", "v")]),
            ])

    def check_starttag_end_boundary(self):
        self._run_check("""<a b='<'>""", [("starttag", "a", [("b", "<")])])
        self._run_check("""<a b='>'>""", [("starttag", "a", [("b", ">")])])

    def check_buffer_artefacts(self):
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

    def check_starttag_junk_chars(self):
        self._parse_error("<")
        self._parse_error("<>")
        self._parse_error("</>")
        self._parse_error("</$>")
        self._parse_error("</")
        self._parse_error("</a")
        self._parse_error("</a")
        self._parse_error("<a<a>")
        self._parse_error("</a<a>")
        self._parse_error("<$")
        self._parse_error("<$>")
        self._parse_error("<!")
        self._parse_error("<a $>")
        self._parse_error("<a")
        self._parse_error("<a foo='bar'")
        self._parse_error("<a foo='bar")
        self._parse_error("<a foo='>'")
        self._parse_error("<a foo='>")
        self._parse_error("<a foo=>")

    def check_declaration_junk_chars(self):
        self._parse_error("<!DOCTYPE foo $ >")

    def check_startendtag(self):
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

    def check_get_starttag_text(self):
        s = """<foo:bar   \n   one="1"\ttwo=2   >"""
        self._run_check_extra(s, [
            ("starttag", "foo:bar", [("one", "1"), ("two", "2")]),
            ("starttag_text", s)])

    def check_cdata_content(self):
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


test_support.run_unittest(HTMLParserTestCase)
