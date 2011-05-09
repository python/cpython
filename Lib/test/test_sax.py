# regression test for SAX 2.0
# $Id$

from xml.sax import make_parser, ContentHandler, \
                    SAXException, SAXReaderNotAvailable, SAXParseException
try:
    make_parser()
except SAXReaderNotAvailable:
    # don't try to test this module if we cannot create a parser
    raise ImportError("no XML parsers available")
from xml.sax.saxutils import XMLGenerator, escape, unescape, quoteattr, \
                             XMLFilterBase
from xml.sax.expatreader import create_parser
from xml.sax.handler import feature_namespaces
from xml.sax.xmlreader import InputSource, AttributesImpl, AttributesNSImpl
from io import StringIO
from test.support import findfile, run_unittest
import unittest
import os

ns_uri = "http://www.python.org/xml-ns/saxtest/"

class XmlTestBase(unittest.TestCase):
    def verify_empty_attrs(self, attrs):
        self.assertRaises(KeyError, attrs.getValue, "attr")
        self.assertRaises(KeyError, attrs.getValueByQName, "attr")
        self.assertRaises(KeyError, attrs.getNameByQName, "attr")
        self.assertRaises(KeyError, attrs.getQNameByName, "attr")
        self.assertRaises(KeyError, attrs.__getitem__, "attr")
        self.assertEqual(attrs.getLength(), 0)
        self.assertEqual(attrs.getNames(), [])
        self.assertEqual(attrs.getQNames(), [])
        self.assertEqual(len(attrs), 0)
        self.assertFalse("attr" in attrs)
        self.assertEqual(list(attrs.keys()), [])
        self.assertEqual(attrs.get("attrs"), None)
        self.assertEqual(attrs.get("attrs", 25), 25)
        self.assertEqual(list(attrs.items()), [])
        self.assertEqual(list(attrs.values()), [])

    def verify_empty_nsattrs(self, attrs):
        self.assertRaises(KeyError, attrs.getValue, (ns_uri, "attr"))
        self.assertRaises(KeyError, attrs.getValueByQName, "ns:attr")
        self.assertRaises(KeyError, attrs.getNameByQName, "ns:attr")
        self.assertRaises(KeyError, attrs.getQNameByName, (ns_uri, "attr"))
        self.assertRaises(KeyError, attrs.__getitem__, (ns_uri, "attr"))
        self.assertEqual(attrs.getLength(), 0)
        self.assertEqual(attrs.getNames(), [])
        self.assertEqual(attrs.getQNames(), [])
        self.assertEqual(len(attrs), 0)
        self.assertFalse((ns_uri, "attr") in attrs)
        self.assertEqual(list(attrs.keys()), [])
        self.assertEqual(attrs.get((ns_uri, "attr")), None)
        self.assertEqual(attrs.get((ns_uri, "attr"), 25), 25)
        self.assertEqual(list(attrs.items()), [])
        self.assertEqual(list(attrs.values()), [])

    def verify_attrs_wattr(self, attrs):
        self.assertEqual(attrs.getLength(), 1)
        self.assertEqual(attrs.getNames(), ["attr"])
        self.assertEqual(attrs.getQNames(), ["attr"])
        self.assertEqual(len(attrs), 1)
        self.assertTrue("attr" in attrs)
        self.assertEqual(list(attrs.keys()), ["attr"])
        self.assertEqual(attrs.get("attr"), "val")
        self.assertEqual(attrs.get("attr", 25), "val")
        self.assertEqual(list(attrs.items()), [("attr", "val")])
        self.assertEqual(list(attrs.values()), ["val"])
        self.assertEqual(attrs.getValue("attr"), "val")
        self.assertEqual(attrs.getValueByQName("attr"), "val")
        self.assertEqual(attrs.getNameByQName("attr"), "attr")
        self.assertEqual(attrs["attr"], "val")
        self.assertEqual(attrs.getQNameByName("attr"), "attr")

class MakeParserTest(unittest.TestCase):
    def test_make_parser2(self):
        # Creating parsers several times in a row should succeed.
        # Testing this because there have been failures of this kind
        # before.
        from xml.sax import make_parser
        p = make_parser()
        from xml.sax import make_parser
        p = make_parser()
        from xml.sax import make_parser
        p = make_parser()
        from xml.sax import make_parser
        p = make_parser()
        from xml.sax import make_parser
        p = make_parser()
        from xml.sax import make_parser
        p = make_parser()


# ===========================================================================
#
#   saxutils tests
#
# ===========================================================================

class SaxutilsTest(unittest.TestCase):
    # ===== escape
    def test_escape_basic(self):
        self.assertEqual(escape("Donald Duck & Co"), "Donald Duck &amp; Co")

    def test_escape_all(self):
        self.assertEqual(escape("<Donald Duck & Co>"),
                         "&lt;Donald Duck &amp; Co&gt;")

    def test_escape_extra(self):
        self.assertEqual(escape("Hei på deg", {"å" : "&aring;"}),
                         "Hei p&aring; deg")

    # ===== unescape
    def test_unescape_basic(self):
        self.assertEqual(unescape("Donald Duck &amp; Co"), "Donald Duck & Co")

    def test_unescape_all(self):
        self.assertEqual(unescape("&lt;Donald Duck &amp; Co&gt;"),
                         "<Donald Duck & Co>")

    def test_unescape_extra(self):
        self.assertEqual(unescape("Hei på deg", {"å" : "&aring;"}),
                         "Hei p&aring; deg")

    def test_unescape_amp_extra(self):
        self.assertEqual(unescape("&amp;foo;", {"&foo;": "splat"}), "&foo;")

    # ===== quoteattr
    def test_quoteattr_basic(self):
        self.assertEqual(quoteattr("Donald Duck & Co"),
                         '"Donald Duck &amp; Co"')

    def test_single_quoteattr(self):
        self.assertEqual(quoteattr('Includes "double" quotes'),
                         '\'Includes "double" quotes\'')

    def test_double_quoteattr(self):
        self.assertEqual(quoteattr("Includes 'single' quotes"),
                         "\"Includes 'single' quotes\"")

    def test_single_double_quoteattr(self):
        self.assertEqual(quoteattr("Includes 'single' and \"double\" quotes"),
                    "\"Includes 'single' and &quot;double&quot; quotes\"")

    # ===== make_parser
    def test_make_parser(self):
        # Creating a parser should succeed - it should fall back
        # to the expatreader
        p = make_parser(['xml.parsers.no_such_parser'])


# ===== XMLGenerator

start = '<?xml version="1.0" encoding="iso-8859-1"?>\n'

class XmlgenTest(unittest.TestCase):
    def test_xmlgen_basic(self):
        result = StringIO()
        gen = XMLGenerator(result)
        gen.startDocument()
        gen.startElement("doc", {})
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start + "<doc></doc>")

    def test_xmlgen_content(self):
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.characters("huhei")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start + "<doc>huhei</doc>")

    def test_xmlgen_pi(self):
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.processingInstruction("test", "data")
        gen.startElement("doc", {})
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start + "<?test data?><doc></doc>")

    def test_xmlgen_content_escape(self):
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.characters("<huhei&")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(),
            start + "<doc>&lt;huhei&amp;</doc>")

    def test_xmlgen_attr_escape(self):
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {"a": '"'})
        gen.startElement("e", {"a": "'"})
        gen.endElement("e")
        gen.startElement("e", {"a": "'\""})
        gen.endElement("e")
        gen.startElement("e", {"a": "\n\r\t"})
        gen.endElement("e")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start +
            ("<doc a='\"'><e a=\"'\"></e>"
             "<e a=\"'&quot;\"></e>"
             "<e a=\"&#10;&#13;&#9;\"></e></doc>"))

    def test_xmlgen_ignorable(self):
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.ignorableWhitespace(" ")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start + "<doc> </doc>")

    def test_xmlgen_ns(self):
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startPrefixMapping("ns1", ns_uri)
        gen.startElementNS((ns_uri, "doc"), "ns1:doc", {})
        # add an unqualified name
        gen.startElementNS((None, "udoc"), None, {})
        gen.endElementNS((None, "udoc"), None)
        gen.endElementNS((ns_uri, "doc"), "ns1:doc")
        gen.endPrefixMapping("ns1")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start + \
           ('<ns1:doc xmlns:ns1="%s"><udoc></udoc></ns1:doc>' %
                                         ns_uri))

    def test_1463026_1(self):
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElementNS((None, 'a'), 'a', {(None, 'b'):'c'})
        gen.endElementNS((None, 'a'), 'a')
        gen.endDocument()

        self.assertEqual(result.getvalue(), start+'<a b="c"></a>')

    def test_1463026_2(self):
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startPrefixMapping(None, 'qux')
        gen.startElementNS(('qux', 'a'), 'a', {})
        gen.endElementNS(('qux', 'a'), 'a')
        gen.endPrefixMapping(None)
        gen.endDocument()

        self.assertEqual(result.getvalue(), start+'<a xmlns="qux"></a>')

    def test_1463026_3(self):
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startPrefixMapping('my', 'qux')
        gen.startElementNS(('qux', 'a'), 'a', {(None, 'b'):'c'})
        gen.endElementNS(('qux', 'a'), 'a')
        gen.endPrefixMapping('my')
        gen.endDocument()

        self.assertEqual(result.getvalue(),
            start+'<my:a xmlns:my="qux" b="c"></my:a>')

    def test_5027_1(self):
        # The xml prefix (as in xml:lang below) is reserved and bound by
        # definition to http://www.w3.org/XML/1998/namespace.  XMLGenerator had
        # a bug whereby a KeyError is thrown because this namespace is missing
        # from a dictionary.
        #
        # This test demonstrates the bug by parsing a document.
        test_xml = StringIO(
            '<?xml version="1.0"?>'
            '<a:g1 xmlns:a="http://example.com/ns">'
             '<a:g2 xml:lang="en">Hello</a:g2>'
            '</a:g1>')

        parser = make_parser()
        parser.setFeature(feature_namespaces, True)
        result = StringIO()
        gen = XMLGenerator(result)
        parser.setContentHandler(gen)
        parser.parse(test_xml)

        self.assertEqual(result.getvalue(),
                         start + (
                         '<a:g1 xmlns:a="http://example.com/ns">'
                          '<a:g2 xml:lang="en">Hello</a:g2>'
                         '</a:g1>'))

    def test_5027_2(self):
        # The xml prefix (as in xml:lang below) is reserved and bound by
        # definition to http://www.w3.org/XML/1998/namespace.  XMLGenerator had
        # a bug whereby a KeyError is thrown because this namespace is missing
        # from a dictionary.
        #
        # This test demonstrates the bug by direct manipulation of the
        # XMLGenerator.
        result = StringIO()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startPrefixMapping('a', 'http://example.com/ns')
        gen.startElementNS(('http://example.com/ns', 'g1'), 'g1', {})
        lang_attr = {('http://www.w3.org/XML/1998/namespace', 'lang'): 'en'}
        gen.startElementNS(('http://example.com/ns', 'g2'), 'g2', lang_attr)
        gen.characters('Hello')
        gen.endElementNS(('http://example.com/ns', 'g2'), 'g2')
        gen.endElementNS(('http://example.com/ns', 'g1'), 'g1')
        gen.endPrefixMapping('a')
        gen.endDocument()

        self.assertEqual(result.getvalue(),
                         start + (
                         '<a:g1 xmlns:a="http://example.com/ns">'
                          '<a:g2 xml:lang="en">Hello</a:g2>'
                         '</a:g1>'))


class XMLFilterBaseTest(unittest.TestCase):
    def test_filter_basic(self):
        result = StringIO()
        gen = XMLGenerator(result)
        filter = XMLFilterBase()
        filter.setContentHandler(gen)

        filter.startDocument()
        filter.startElement("doc", {})
        filter.characters("content")
        filter.ignorableWhitespace(" ")
        filter.endElement("doc")
        filter.endDocument()

        self.assertEqual(result.getvalue(), start + "<doc>content </doc>")

# ===========================================================================
#
#   expatreader tests
#
# ===========================================================================

xml_test_out = open(findfile("test.xml.out")).read()

class ExpatReaderTest(XmlTestBase):

    # ===== XMLReader support

    def test_expat_file(self):
        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(open(findfile("test.xml")))

        self.assertEqual(result.getvalue(), xml_test_out)

    # ===== DTDHandler support

    class TestDTDHandler:

        def __init__(self):
            self._notations = []
            self._entities  = []

        def notationDecl(self, name, publicId, systemId):
            self._notations.append((name, publicId, systemId))

        def unparsedEntityDecl(self, name, publicId, systemId, ndata):
            self._entities.append((name, publicId, systemId, ndata))

    def test_expat_dtdhandler(self):
        parser = create_parser()
        handler = self.TestDTDHandler()
        parser.setDTDHandler(handler)

        parser.feed('<!DOCTYPE doc [\n')
        parser.feed('  <!ENTITY img SYSTEM "expat.gif" NDATA GIF>\n')
        parser.feed('  <!NOTATION GIF PUBLIC "-//CompuServe//NOTATION Graphics Interchange Format 89a//EN">\n')
        parser.feed(']>\n')
        parser.feed('<doc></doc>')
        parser.close()

        self.assertEqual(handler._notations,
            [("GIF", "-//CompuServe//NOTATION Graphics Interchange Format 89a//EN", None)])
        self.assertEqual(handler._entities, [("img", None, "expat.gif", "GIF")])

    # ===== EntityResolver support

    class TestEntityResolver:

        def resolveEntity(self, publicId, systemId):
            inpsrc = InputSource()
            inpsrc.setByteStream(StringIO("<entity/>"))
            return inpsrc

    def test_expat_entityresolver(self):
        parser = create_parser()
        parser.setEntityResolver(self.TestEntityResolver())
        result = StringIO()
        parser.setContentHandler(XMLGenerator(result))

        parser.feed('<!DOCTYPE doc [\n')
        parser.feed('  <!ENTITY test SYSTEM "whatever">\n')
        parser.feed(']>\n')
        parser.feed('<doc>&test;</doc>')
        parser.close()

        self.assertEqual(result.getvalue(), start +
                         "<doc><entity></entity></doc>")

    # ===== Attributes support

    class AttrGatherer(ContentHandler):

        def startElement(self, name, attrs):
            self._attrs = attrs

        def startElementNS(self, name, qname, attrs):
            self._attrs = attrs

    def test_expat_attrs_empty(self):
        parser = create_parser()
        gather = self.AttrGatherer()
        parser.setContentHandler(gather)

        parser.feed("<doc/>")
        parser.close()

        self.verify_empty_attrs(gather._attrs)

    def test_expat_attrs_wattr(self):
        parser = create_parser()
        gather = self.AttrGatherer()
        parser.setContentHandler(gather)

        parser.feed("<doc attr='val'/>")
        parser.close()

        self.verify_attrs_wattr(gather._attrs)

    def test_expat_nsattrs_empty(self):
        parser = create_parser(1)
        gather = self.AttrGatherer()
        parser.setContentHandler(gather)

        parser.feed("<doc/>")
        parser.close()

        self.verify_empty_nsattrs(gather._attrs)

    def test_expat_nsattrs_wattr(self):
        parser = create_parser(1)
        gather = self.AttrGatherer()
        parser.setContentHandler(gather)

        parser.feed("<doc xmlns:ns='%s' ns:attr='val'/>" % ns_uri)
        parser.close()

        attrs = gather._attrs

        self.assertEqual(attrs.getLength(), 1)
        self.assertEqual(attrs.getNames(), [(ns_uri, "attr")])
        self.assertTrue((attrs.getQNames() == [] or
                         attrs.getQNames() == ["ns:attr"]))
        self.assertEqual(len(attrs), 1)
        self.assertTrue((ns_uri, "attr") in attrs)
        self.assertEqual(attrs.get((ns_uri, "attr")), "val")
        self.assertEqual(attrs.get((ns_uri, "attr"), 25), "val")
        self.assertEqual(list(attrs.items()), [((ns_uri, "attr"), "val")])
        self.assertEqual(list(attrs.values()), ["val"])
        self.assertEqual(attrs.getValue((ns_uri, "attr")), "val")
        self.assertEqual(attrs[(ns_uri, "attr")], "val")

    # ===== InputSource support

    def test_expat_inpsource_filename(self):
        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(findfile("test.xml"))

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_inpsource_sysid(self):
        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(InputSource(findfile("test.xml")))

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_inpsource_stream(self):
        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        inpsrc = InputSource()
        inpsrc.setByteStream(open(findfile("test.xml")))
        parser.parse(inpsrc)

        self.assertEqual(result.getvalue(), xml_test_out)

    # ===== IncrementalParser support

    def test_expat_incremental(self):
        result = StringIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)

        parser.feed("<doc>")
        parser.feed("</doc>")
        parser.close()

        self.assertEqual(result.getvalue(), start + "<doc></doc>")

    def test_expat_incremental_reset(self):
        result = StringIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)

        parser.feed("<doc>")
        parser.feed("text")

        result = StringIO()
        xmlgen = XMLGenerator(result)
        parser.setContentHandler(xmlgen)
        parser.reset()

        parser.feed("<doc>")
        parser.feed("text")
        parser.feed("</doc>")
        parser.close()

        self.assertEqual(result.getvalue(), start + "<doc>text</doc>")

    # ===== Locator support

    def test_expat_locator_noinfo(self):
        result = StringIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)

        parser.feed("<doc>")
        parser.feed("</doc>")
        parser.close()

        self.assertEqual(parser.getSystemId(), None)
        self.assertEqual(parser.getPublicId(), None)
        self.assertEqual(parser.getLineNumber(), 1)

    def test_expat_locator_withinfo(self):
        result = StringIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)
        parser.parse(findfile("test.xml"))

        self.assertEqual(parser.getSystemId(), findfile("test.xml"))
        self.assertEqual(parser.getPublicId(), None)


# ===========================================================================
#
#   error reporting
#
# ===========================================================================

class ErrorReportingTest(unittest.TestCase):
    def test_expat_inpsource_location(self):
        parser = create_parser()
        parser.setContentHandler(ContentHandler()) # do nothing
        source = InputSource()
        source.setByteStream(StringIO("<foo bar foobar>"))   #ill-formed
        name = "a file name"
        source.setSystemId(name)
        try:
            parser.parse(source)
            self.fail()
        except SAXException as e:
            self.assertEqual(e.getSystemId(), name)

    def test_expat_incomplete(self):
        parser = create_parser()
        parser.setContentHandler(ContentHandler()) # do nothing
        self.assertRaises(SAXParseException, parser.parse, StringIO("<foo>"))

    def test_sax_parse_exception_str(self):
        # pass various values from a locator to the SAXParseException to
        # make sure that the __str__() doesn't fall apart when None is
        # passed instead of an integer line and column number
        #
        # use "normal" values for the locator:
        str(SAXParseException("message", None,
                              self.DummyLocator(1, 1)))
        # use None for the line number:
        str(SAXParseException("message", None,
                              self.DummyLocator(None, 1)))
        # use None for the column number:
        str(SAXParseException("message", None,
                              self.DummyLocator(1, None)))
        # use None for both:
        str(SAXParseException("message", None,
                              self.DummyLocator(None, None)))

    class DummyLocator:
        def __init__(self, lineno, colno):
            self._lineno = lineno
            self._colno = colno

        def getPublicId(self):
            return "pubid"

        def getSystemId(self):
            return "sysid"

        def getLineNumber(self):
            return self._lineno

        def getColumnNumber(self):
            return self._colno

# ===========================================================================
#
#   xmlreader tests
#
# ===========================================================================

class XmlReaderTest(XmlTestBase):

    # ===== AttributesImpl
    def test_attrs_empty(self):
        self.verify_empty_attrs(AttributesImpl({}))

    def test_attrs_wattr(self):
        self.verify_attrs_wattr(AttributesImpl({"attr" : "val"}))

    def test_nsattrs_empty(self):
        self.verify_empty_nsattrs(AttributesNSImpl({}, {}))

    def test_nsattrs_wattr(self):
        attrs = AttributesNSImpl({(ns_uri, "attr") : "val"},
                                 {(ns_uri, "attr") : "ns:attr"})

        self.assertEqual(attrs.getLength(), 1)
        self.assertEqual(attrs.getNames(), [(ns_uri, "attr")])
        self.assertEqual(attrs.getQNames(), ["ns:attr"])
        self.assertEqual(len(attrs), 1)
        self.assertTrue((ns_uri, "attr") in attrs)
        self.assertEqual(list(attrs.keys()), [(ns_uri, "attr")])
        self.assertEqual(attrs.get((ns_uri, "attr")), "val")
        self.assertEqual(attrs.get((ns_uri, "attr"), 25), "val")
        self.assertEqual(list(attrs.items()), [((ns_uri, "attr"), "val")])
        self.assertEqual(list(attrs.values()), ["val"])
        self.assertEqual(attrs.getValue((ns_uri, "attr")), "val")
        self.assertEqual(attrs.getValueByQName("ns:attr"), "val")
        self.assertEqual(attrs.getNameByQName("ns:attr"), (ns_uri, "attr"))
        self.assertEqual(attrs[(ns_uri, "attr")], "val")
        self.assertEqual(attrs.getQNameByName((ns_uri, "attr")), "ns:attr")


def test_main():
    run_unittest(MakeParserTest,
                 SaxutilsTest,
                 XmlgenTest,
                 ExpatReaderTest,
                 ErrorReportingTest,
                 XmlReaderTest)

if __name__ == "__main__":
    test_main()
