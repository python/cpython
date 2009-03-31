import pprint
import re
import unittest
from test import test_support
sgmllib = test_support.import_module('sgmllib', deprecated=True)


class EventCollector(sgmllib.SGMLParser):

    def __init__(self):
        self.events = []
        self.append = self.events.append
        sgmllib.SGMLParser.__init__(self)

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

    def unknown_starttag(self, tag, attrs):
        self.append(("starttag", tag, attrs))

    def unknown_endtag(self, tag):
        self.append(("endtag", tag))

    # all other markup

    def handle_comment(self, data):
        self.append(("comment", data))

    def handle_charref(self, data):
        self.append(("charref", data))

    def handle_data(self, data):
        self.append(("data", data))

    def handle_decl(self, decl):
        self.append(("decl", decl))

    def handle_entityref(self, data):
        self.append(("entityref", data))

    def handle_pi(self, data):
        self.append(("pi", data))

    def unknown_decl(self, decl):
        self.append(("unknown decl", decl))


class CDATAEventCollector(EventCollector):
    def start_cdata(self, attrs):
        self.append(("starttag", "cdata", attrs))
        self.setliteral()


class HTMLEntityCollector(EventCollector):

    entity_or_charref = re.compile('(?:&([a-zA-Z][-.a-zA-Z0-9]*)'
        '|&#(x[0-9a-zA-Z]+|[0-9]+))(;?)')

    def convert_charref(self, name):
        self.append(("charref", "convert", name))
        if name[0] != "x":
            return EventCollector.convert_charref(self, name)

    def convert_codepoint(self, codepoint):
        self.append(("codepoint", "convert", codepoint))
        EventCollector.convert_codepoint(self, codepoint)

    def convert_entityref(self, name):
        self.append(("entityref", "convert", name))
        return EventCollector.convert_entityref(self, name)

    # These to record that they were called, then pass the call along
    # to the default implementation so that it's actions can be
    # recorded.

    def handle_charref(self, data):
        self.append(("charref", data))
        sgmllib.SGMLParser.handle_charref(self, data)

    def handle_entityref(self, data):
        self.append(("entityref", data))
        sgmllib.SGMLParser.handle_entityref(self, data)


class SGMLParserTestCase(unittest.TestCase):

    collector = EventCollector

    def get_events(self, source):
        parser = self.collector()
        try:
            for s in source:
                parser.feed(s)
            parser.close()
        except:
            #self.events = parser.events
            raise
        return parser.get_events()

    def check_events(self, source, expected_events):
        try:
            events = self.get_events(source)
        except:
            #import sys
            #print >>sys.stderr, pprint.pformat(self.events)
            raise
        if events != expected_events:
            self.fail("received events did not match expected events\n"
                      "Expected:\n" + pprint.pformat(expected_events) +
                      "\nReceived:\n" + pprint.pformat(events))

    def check_parse_error(self, source):
        parser = EventCollector()
        try:
            parser.feed(source)
            parser.close()
        except sgmllib.SGMLParseError:
            pass
        else:
            self.fail("expected SGMLParseError for %r\nReceived:\n%s"
                      % (source, pprint.pformat(parser.get_events())))

    def test_doctype_decl_internal(self):
        inside = """\
DOCTYPE html PUBLIC '-//W3C//DTD HTML 4.01//EN'
             SYSTEM 'http://www.w3.org/TR/html401/strict.dtd' [
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
        self.check_events(["<!%s>" % inside], [
            ("decl", inside),
            ])

    def test_doctype_decl_external(self):
        inside = "DOCTYPE html PUBLIC '-//W3C//DTD HTML 4.01//EN'"
        self.check_events("<!%s>" % inside, [
            ("decl", inside),
            ])

    def test_underscore_in_attrname(self):
        # SF bug #436621
        """Make sure attribute names with underscores are accepted"""
        self.check_events("<a has_under _under>", [
            ("starttag", "a", [("has_under", "has_under"),
                               ("_under", "_under")]),
            ])

    def test_underscore_in_tagname(self):
        # SF bug #436621
        """Make sure tag names with underscores are accepted"""
        self.check_events("<has_under></has_under>", [
            ("starttag", "has_under", []),
            ("endtag", "has_under"),
            ])

    def test_quotes_in_unquoted_attrs(self):
        # SF bug #436621
        """Be sure quotes in unquoted attributes are made part of the value"""
        self.check_events("<a href=foo'bar\"baz>", [
            ("starttag", "a", [("href", "foo'bar\"baz")]),
            ])

    def test_xhtml_empty_tag(self):
        """Handling of XHTML-style empty start tags"""
        self.check_events("<br />text<i></i>", [
            ("starttag", "br", []),
            ("data", "text"),
            ("starttag", "i", []),
            ("endtag", "i"),
            ])

    def test_processing_instruction_only(self):
        self.check_events("<?processing instruction>", [
            ("pi", "processing instruction"),
            ])

    def test_bad_nesting(self):
        self.check_events("<a><b></a></b>", [
            ("starttag", "a", []),
            ("starttag", "b", []),
            ("endtag", "a"),
            ("endtag", "b"),
            ])

    def test_bare_ampersands(self):
        self.check_events("this text & contains & ampersands &", [
            ("data", "this text & contains & ampersands &"),
            ])

    def test_bare_pointy_brackets(self):
        self.check_events("this < text > contains < bare>pointy< brackets", [
            ("data", "this < text > contains < bare>pointy< brackets"),
            ])

    def test_attr_syntax(self):
        output = [
          ("starttag", "a", [("b", "v"), ("c", "v"), ("d", "v"), ("e", "e")])
          ]
        self.check_events("""<a b='v' c="v" d=v e>""", output)
        self.check_events("""<a  b = 'v' c = "v" d = v e>""", output)
        self.check_events("""<a\nb\n=\n'v'\nc\n=\n"v"\nd\n=\nv\ne>""", output)
        self.check_events("""<a\tb\t=\t'v'\tc\t=\t"v"\td\t=\tv\te>""", output)

    def test_attr_values(self):
        self.check_events("""<a b='xxx\n\txxx' c="yyy\t\nyyy" d='\txyz\n'>""",
                        [("starttag", "a", [("b", "xxx\n\txxx"),
                                            ("c", "yyy\t\nyyy"),
                                            ("d", "\txyz\n")])
                         ])
        self.check_events("""<a b='' c="">""", [
            ("starttag", "a", [("b", ""), ("c", "")]),
            ])
        # URL construction stuff from RFC 1808:
        safe = "$-_.+"
        extra = "!*'(),"
        reserved = ";/?:@&="
        url = "http://example.com:8080/path/to/file?%s%s%s" % (
            safe, extra, reserved)
        self.check_events("""<e a=%s>""" % url, [
            ("starttag", "e", [("a", url)]),
            ])
        # Regression test for SF patch #669683.
        self.check_events("<e a=rgb(1,2,3)>", [
            ("starttag", "e", [("a", "rgb(1,2,3)")]),
            ])

    def test_attr_values_entities(self):
        """Substitution of entities and charrefs in attribute values"""
        # SF bug #1452246
        self.check_events("""<a b=&lt; c=&lt;&gt; d=&lt-&gt; e='&lt; '
                                f="&xxx;" g='&#32;&#33;' h='&#500;'
                                i='x?a=b&c=d;'
                                j='&amp;#42;' k='&#38;#42;'>""",
            [("starttag", "a", [("b", "<"),
                                ("c", "<>"),
                                ("d", "&lt->"),
                                ("e", "< "),
                                ("f", "&xxx;"),
                                ("g", " !"),
                                ("h", "&#500;"),
                                ("i", "x?a=b&c=d;"),
                                ("j", "&#42;"),
                                ("k", "&#42;"),
                                ])])

    def test_convert_overrides(self):
        # This checks that the character and entity reference
        # conversion helpers are called at the documented times.  No
        # attempt is made to really change what the parser accepts.
        #
        self.collector = HTMLEntityCollector
        self.check_events(('<a title="&ldquo;test&#x201d;">foo</a>'
                           '&foobar;&#42;'), [
            ('entityref', 'convert', 'ldquo'),
            ('charref', 'convert', 'x201d'),
            ('starttag', 'a', [('title', '&ldquo;test&#x201d;')]),
            ('data', 'foo'),
            ('endtag', 'a'),
            ('entityref', 'foobar'),
            ('entityref', 'convert', 'foobar'),
            ('charref', '42'),
            ('charref', 'convert', '42'),
            ('codepoint', 'convert', 42),
            ])

    def test_attr_funky_names(self):
        self.check_events("""<a a.b='v' c:d=v e-f=v>""", [
            ("starttag", "a", [("a.b", "v"), ("c:d", "v"), ("e-f", "v")]),
            ])

    def test_attr_value_ip6_url(self):
        # http://www.python.org/sf/853506
        self.check_events(("<a href='http://[1080::8:800:200C:417A]/'>"
                           "<a href=http://[1080::8:800:200C:417A]/>"), [
            ("starttag", "a", [("href", "http://[1080::8:800:200C:417A]/")]),
            ("starttag", "a", [("href", "http://[1080::8:800:200C:417A]/")]),
            ])

    def test_illegal_declarations(self):
        s = 'abc<!spacer type="block" height="25">def'
        self.check_events(s, [
            ("data", "abc"),
            ("unknown decl", 'spacer type="block" height="25"'),
            ("data", "def"),
            ])

    def test_weird_starttags(self):
        self.check_events("<a<a>", [
            ("starttag", "a", []),
            ("starttag", "a", []),
            ])
        self.check_events("</a<a>", [
            ("endtag", "a"),
            ("starttag", "a", []),
            ])

    def test_declaration_junk_chars(self):
        self.check_parse_error("<!DOCTYPE foo $ >")

    def test_get_starttag_text(self):
        s = """<foobar   \n   one="1"\ttwo=2   >"""
        self.check_events(s, [
            ("starttag", "foobar", [("one", "1"), ("two", "2")]),
            ])

    def test_cdata_content(self):
        s = ("<cdata> <!-- not a comment --> &not-an-entity-ref; </cdata>"
             "<notcdata> <!-- comment --> </notcdata>")
        self.collector = CDATAEventCollector
        self.check_events(s, [
            ("starttag", "cdata", []),
            ("data", " <!-- not a comment --> &not-an-entity-ref; "),
            ("endtag", "cdata"),
            ("starttag", "notcdata", []),
            ("data", " "),
            ("comment", " comment "),
            ("data", " "),
            ("endtag", "notcdata"),
            ])
        s = """<cdata> <not a='start tag'> </cdata>"""
        self.check_events(s, [
            ("starttag", "cdata", []),
            ("data", " <not a='start tag'> "),
            ("endtag", "cdata"),
            ])

    def test_illegal_declarations(self):
        s = 'abc<!spacer type="block" height="25">def'
        self.check_events(s, [
            ("data", "abc"),
            ("unknown decl", 'spacer type="block" height="25"'),
            ("data", "def"),
            ])

    def test_enumerated_attr_type(self):
        s = "<!DOCTYPE doc [<!ATTLIST doc attr (a | b) >]>"
        self.check_events(s, [
            ('decl', 'DOCTYPE doc [<!ATTLIST doc attr (a | b) >]'),
            ])

    def test_read_chunks(self):
        # SF bug #1541697, this caused sgml parser to hang
        # Just verify this code doesn't cause a hang.
        CHUNK = 1024  # increasing this to 8212 makes the problem go away

        f = open(test_support.findfile('sgml_input.html'))
        fp = sgmllib.SGMLParser()
        while 1:
            data = f.read(CHUNK)
            fp.feed(data)
            if len(data) != CHUNK:
                break

    def test_only_decode_ascii(self):
        # SF bug #1651995, make sure non-ascii character references are not decoded
        s = '<signs exclamation="&#33" copyright="&#169" quoteleft="&#8216;">'
        self.check_events(s, [
            ('starttag', 'signs',
             [('exclamation', '!'), ('copyright', '&#169'),
              ('quoteleft', '&#8216;')]),
            ])

    # XXX These tests have been disabled by prefixing their names with
    # an underscore.  The first two exercise outstanding bugs in the
    # sgmllib module, and the third exhibits questionable behavior
    # that needs to be carefully considered before changing it.

    def _test_starttag_end_boundary(self):
        self.check_events("<a b='<'>", [("starttag", "a", [("b", "<")])])
        self.check_events("<a b='>'>", [("starttag", "a", [("b", ">")])])

    def _test_buffer_artefacts(self):
        output = [("starttag", "a", [("b", "<")])]
        self.check_events(["<a b='<'>"], output)
        self.check_events(["<a ", "b='<'>"], output)
        self.check_events(["<a b", "='<'>"], output)
        self.check_events(["<a b=", "'<'>"], output)
        self.check_events(["<a b='<", "'>"], output)
        self.check_events(["<a b='<'", ">"], output)

        output = [("starttag", "a", [("b", ">")])]
        self.check_events(["<a b='>'>"], output)
        self.check_events(["<a ", "b='>'>"], output)
        self.check_events(["<a b", "='>'>"], output)
        self.check_events(["<a b=", "'>'>"], output)
        self.check_events(["<a b='>", "'>"], output)
        self.check_events(["<a b='>'", ">"], output)

        output = [("comment", "abc")]
        self.check_events(["", "<!--abc-->"], output)
        self.check_events(["<", "!--abc-->"], output)
        self.check_events(["<!", "--abc-->"], output)
        self.check_events(["<!-", "-abc-->"], output)
        self.check_events(["<!--", "abc-->"], output)
        self.check_events(["<!--a", "bc-->"], output)
        self.check_events(["<!--ab", "c-->"], output)
        self.check_events(["<!--abc", "-->"], output)
        self.check_events(["<!--abc-", "->"], output)
        self.check_events(["<!--abc--", ">"], output)
        self.check_events(["<!--abc-->", ""], output)

    def _test_starttag_junk_chars(self):
        self.check_parse_error("<")
        self.check_parse_error("<>")
        self.check_parse_error("</$>")
        self.check_parse_error("</")
        self.check_parse_error("</a")
        self.check_parse_error("<$")
        self.check_parse_error("<$>")
        self.check_parse_error("<!")
        self.check_parse_error("<a $>")
        self.check_parse_error("<a")
        self.check_parse_error("<a foo='bar'")
        self.check_parse_error("<a foo='bar")
        self.check_parse_error("<a foo='>'")
        self.check_parse_error("<a foo='>")
        self.check_parse_error("<a foo=>")


def test_main():
    test_support.run_unittest(SGMLParserTestCase)


if __name__ == "__main__":
    test_main()
