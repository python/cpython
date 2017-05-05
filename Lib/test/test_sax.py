# regression test for SAX 2.0            -*- coding: utf-8 -*-
# $Id$

from xml.sax import make_parser, ContentHandler, \
                    SAXException, SAXReaderNotAvailable, SAXParseException, \
                    saxutils
try:
    make_parser()
except SAXReaderNotAvailable:
    # don't try to test this module if we cannot create a parser
    raise ImportError("no XML parsers available")
from xml.sax.saxutils import XMLGenerator, escape, unescape, quoteattr, \
                             XMLFilterBase, prepare_input_source
from xml.sax.expatreader import create_parser
from xml.sax.handler import feature_namespaces
from xml.sax.xmlreader import InputSource, AttributesImpl, AttributesNSImpl
from cStringIO import StringIO
import io
import gc
import os.path
import shutil
import test.test_support as support
from test.test_support import findfile, run_unittest, TESTFN
import unittest

TEST_XMLFILE = findfile("test.xml", subdir="xmltestdata")
TEST_XMLFILE_OUT = findfile("test.xml.out", subdir="xmltestdata")

supports_unicode_filenames = True
if not os.path.supports_unicode_filenames:
    try:
        support.TESTFN_UNICODE.encode(support.TESTFN_ENCODING)
    except (AttributeError, UnicodeError, TypeError):
        # Either the file system encoding is None, or the file name
        # cannot be encoded in the file system encoding.
        supports_unicode_filenames = False
requires_unicode_filenames = unittest.skipUnless(
        supports_unicode_filenames,
        'Requires unicode filenames support')

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
        self.assertFalse(attrs.has_key("attr"))
        self.assertEqual(attrs.keys(), [])
        self.assertEqual(attrs.get("attrs"), None)
        self.assertEqual(attrs.get("attrs", 25), 25)
        self.assertEqual(attrs.items(), [])
        self.assertEqual(attrs.values(), [])

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
        self.assertFalse(attrs.has_key((ns_uri, "attr")))
        self.assertEqual(attrs.keys(), [])
        self.assertEqual(attrs.get((ns_uri, "attr")), None)
        self.assertEqual(attrs.get((ns_uri, "attr"), 25), 25)
        self.assertEqual(attrs.items(), [])
        self.assertEqual(attrs.values(), [])

    def verify_attrs_wattr(self, attrs):
        self.assertEqual(attrs.getLength(), 1)
        self.assertEqual(attrs.getNames(), ["attr"])
        self.assertEqual(attrs.getQNames(), ["attr"])
        self.assertEqual(len(attrs), 1)
        self.assertTrue(attrs.has_key("attr"))
        self.assertEqual(attrs.keys(), ["attr"])
        self.assertEqual(attrs.get("attr"), "val")
        self.assertEqual(attrs.get("attr", 25), "val")
        self.assertEqual(attrs.items(), [("attr", "val")])
        self.assertEqual(attrs.values(), ["val"])
        self.assertEqual(attrs.getValue("attr"), "val")
        self.assertEqual(attrs.getValueByQName("attr"), "val")
        self.assertEqual(attrs.getNameByQName("attr"), "attr")
        self.assertEqual(attrs["attr"], "val")
        self.assertEqual(attrs.getQNameByName("attr"), "attr")


def xml_unicode(doc, encoding=None):
    if encoding is None:
        return doc
    return u'<?xml version="1.0" encoding="%s"?>\n%s' % (encoding, doc)

def xml_bytes(doc, encoding, decl_encoding=Ellipsis):
    if decl_encoding is Ellipsis:
        decl_encoding = encoding
    return xml_unicode(doc, decl_encoding).encode(encoding, 'xmlcharrefreplace')

def make_xml_file(doc, encoding, decl_encoding=Ellipsis):
    if decl_encoding is Ellipsis:
        decl_encoding = encoding
    with io.open(TESTFN, 'w', encoding=encoding, errors='xmlcharrefreplace') as f:
        f.write(xml_unicode(doc, decl_encoding))


class ParseTest(unittest.TestCase):
    data = support.u(r'<money value="$\xa3\u20ac\U0001017b">'
                     r'$\xa3\u20ac\U0001017b</money>')

    def tearDown(self):
        support.unlink(TESTFN)

    def check_parse(self, f):
        from xml.sax import parse
        result = StringIO()
        parse(f, XMLGenerator(result, 'utf-8'))
        self.assertEqual(result.getvalue(), xml_bytes(self.data, 'utf-8'))

    def test_parse_bytes(self):
        # UTF-8 is default encoding, US-ASCII is compatible with UTF-8,
        # UTF-16 is autodetected
        encodings = ('us-ascii', 'utf-8', 'utf-16', 'utf-16le', 'utf-16be')
        for encoding in encodings:
            self.check_parse(io.BytesIO(xml_bytes(self.data, encoding)))
            make_xml_file(self.data, encoding)
            self.check_parse(TESTFN)
            with io.open(TESTFN, 'rb') as f:
                self.check_parse(f)
            self.check_parse(io.BytesIO(xml_bytes(self.data, encoding, None)))
            make_xml_file(self.data, encoding, None)
            self.check_parse(TESTFN)
            with io.open(TESTFN, 'rb') as f:
                self.check_parse(f)
        # accept UTF-8 with BOM
        self.check_parse(io.BytesIO(xml_bytes(self.data, 'utf-8-sig', 'utf-8')))
        make_xml_file(self.data, 'utf-8-sig', 'utf-8')
        self.check_parse(TESTFN)
        with io.open(TESTFN, 'rb') as f:
            self.check_parse(f)
        self.check_parse(io.BytesIO(xml_bytes(self.data, 'utf-8-sig', None)))
        make_xml_file(self.data, 'utf-8-sig', None)
        self.check_parse(TESTFN)
        with io.open(TESTFN, 'rb') as f:
            self.check_parse(f)
        # accept data with declared encoding
        self.check_parse(io.BytesIO(xml_bytes(self.data, 'iso-8859-1')))
        make_xml_file(self.data, 'iso-8859-1')
        self.check_parse(TESTFN)
        with io.open(TESTFN, 'rb') as f:
            self.check_parse(f)
        # fail on non-UTF-8 incompatible data without declared encoding
        with self.assertRaises(SAXException):
            self.check_parse(io.BytesIO(xml_bytes(self.data, 'iso-8859-1', None)))
        make_xml_file(self.data, 'iso-8859-1', None)
        with self.assertRaises(SAXException):
            self.check_parse(TESTFN)
        with io.open(TESTFN, 'rb') as f:
            with self.assertRaises(SAXException):
                self.check_parse(f)

    def test_parse_InputSource(self):
        # accept data without declared but with explicitly specified encoding
        make_xml_file(self.data, 'iso-8859-1', None)
        with io.open(TESTFN, 'rb') as f:
            input = InputSource()
            input.setByteStream(f)
            input.setEncoding('iso-8859-1')
            self.check_parse(input)

    def test_parse_close_source(self):
        builtin_open = open
        non_local = {'fileobj': None}

        def mock_open(*args):
            fileobj = builtin_open(*args)
            non_local['fileobj'] = fileobj
            return fileobj

        with support.swap_attr(saxutils, 'open', mock_open):
            make_xml_file(self.data, 'iso-8859-1', None)
            with self.assertRaises(SAXException):
                self.check_parse(TESTFN)
            self.assertTrue(non_local['fileobj'].closed)

    def check_parseString(self, s):
        from xml.sax import parseString
        result = StringIO()
        parseString(s, XMLGenerator(result, 'utf-8'))
        self.assertEqual(result.getvalue(), xml_bytes(self.data, 'utf-8'))

    def test_parseString_bytes(self):
        # UTF-8 is default encoding, US-ASCII is compatible with UTF-8,
        # UTF-16 is autodetected
        encodings = ('us-ascii', 'utf-8', 'utf-16', 'utf-16le', 'utf-16be')
        for encoding in encodings:
            self.check_parseString(xml_bytes(self.data, encoding))
            self.check_parseString(xml_bytes(self.data, encoding, None))
        # accept UTF-8 with BOM
        self.check_parseString(xml_bytes(self.data, 'utf-8-sig', 'utf-8'))
        self.check_parseString(xml_bytes(self.data, 'utf-8-sig', None))
        # accept data with declared encoding
        self.check_parseString(xml_bytes(self.data, 'iso-8859-1'))
        # fail on non-UTF-8 incompatible data without declared encoding
        with self.assertRaises(SAXException):
            self.check_parseString(xml_bytes(self.data, 'iso-8859-1', None))


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


class PrepareInputSourceTest(unittest.TestCase):

    def setUp(self):
        self.file = support.TESTFN
        with open(self.file, "w") as tmp:
            tmp.write("This was read from a file.")

    def tearDown(self):
        support.unlink(self.file)

    def make_byte_stream(self):
        return io.BytesIO(b"This is a byte stream.")

    def checkContent(self, stream, content):
        self.assertIsNotNone(stream)
        self.assertEqual(stream.read(), content)
        stream.close()


    def test_byte_stream(self):
        # If the source is an InputSource that does not have a character
        # stream but does have a byte stream, use the byte stream.
        src = InputSource(self.file)
        src.setByteStream(self.make_byte_stream())
        prep = prepare_input_source(src)
        self.assertIsNone(prep.getCharacterStream())
        self.checkContent(prep.getByteStream(),
                          b"This is a byte stream.")

    def test_system_id(self):
        # If the source is an InputSource that has neither a character
        # stream nor a byte stream, open the system ID.
        src = InputSource(self.file)
        prep = prepare_input_source(src)
        self.assertIsNone(prep.getCharacterStream())
        self.checkContent(prep.getByteStream(),
                          b"This was read from a file.")

    def test_string(self):
        # If the source is a string, use it as a system ID and open it.
        prep = prepare_input_source(self.file)
        self.assertIsNone(prep.getCharacterStream())
        self.checkContent(prep.getByteStream(),
                          b"This was read from a file.")

    def test_binary_file(self):
        # If the source is a binary file-like object, use it as a byte
        # stream.
        prep = prepare_input_source(self.make_byte_stream())
        self.assertIsNone(prep.getCharacterStream())
        self.checkContent(prep.getByteStream(),
                          b"This is a byte stream.")


# ===== XMLGenerator

start = '<?xml version="1.0" encoding="iso-8859-1"?>\n'

class XmlgenTest:
    def test_xmlgen_basic(self):
        result = self.ioclass()
        gen = XMLGenerator(result)
        gen.startDocument()
        gen.startElement("doc", {})
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start + "<doc></doc>")

    def test_xmlgen_content(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.characters("huhei")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start + "<doc>huhei</doc>")

    def test_xmlgen_pi(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.processingInstruction("test", "data")
        gen.startElement("doc", {})
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start + "<?test data?><doc></doc>")

    def test_xmlgen_content_escape(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.characters("<huhei&")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(),
            start + "<doc>&lt;huhei&amp;</doc>")

    def test_xmlgen_attr_escape(self):
        result = self.ioclass()
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

    def test_xmlgen_encoding(self):
        encodings = ('iso-8859-15', 'utf-8',
                     'utf-16be', 'utf-16le',
                     'utf-32be', 'utf-32le')
        for encoding in encodings:
            result = self.ioclass()
            gen = XMLGenerator(result, encoding=encoding)

            gen.startDocument()
            gen.startElement("doc", {"a": u'\u20ac'})
            gen.characters(u"\u20ac")
            gen.endElement("doc")
            gen.endDocument()

            self.assertEqual(result.getvalue(), (
                u'<?xml version="1.0" encoding="%s"?>\n'
                u'<doc a="\u20ac">\u20ac</doc>' % encoding
                ).encode(encoding, 'xmlcharrefreplace'))

    def test_xmlgen_unencodable(self):
        result = self.ioclass()
        gen = XMLGenerator(result, encoding='ascii')

        gen.startDocument()
        gen.startElement("doc", {"a": u'\u20ac'})
        gen.characters(u"\u20ac")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(),
                '<?xml version="1.0" encoding="ascii"?>\n'
                '<doc a="&#8364;">&#8364;</doc>')

    def test_xmlgen_ignorable(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.ignorableWhitespace(" ")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), start + "<doc> </doc>")

    def test_xmlgen_encoding_bytes(self):
        encodings = ('iso-8859-15', 'utf-8',
                     'utf-16be', 'utf-16le',
                     'utf-32be', 'utf-32le')
        for encoding in encodings:
            result = self.ioclass()
            gen = XMLGenerator(result, encoding=encoding)

            gen.startDocument()
            gen.startElement("doc", {"a": u'\u20ac'})
            gen.characters(u"\u20ac".encode(encoding))
            gen.ignorableWhitespace(" ".encode(encoding))
            gen.endElement("doc")
            gen.endDocument()

            self.assertEqual(result.getvalue(), (
                u'<?xml version="1.0" encoding="%s"?>\n'
                u'<doc a="\u20ac">\u20ac </doc>' % encoding
                ).encode(encoding, 'xmlcharrefreplace'))

    def test_xmlgen_ns(self):
        result = self.ioclass()
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
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElementNS((None, 'a'), 'a', {(None, 'b'):'c'})
        gen.endElementNS((None, 'a'), 'a')
        gen.endDocument()

        self.assertEqual(result.getvalue(), start+'<a b="c"></a>')

    def test_1463026_2(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startPrefixMapping(None, 'qux')
        gen.startElementNS(('qux', 'a'), 'a', {})
        gen.endElementNS(('qux', 'a'), 'a')
        gen.endPrefixMapping(None)
        gen.endDocument()

        self.assertEqual(result.getvalue(), start+'<a xmlns="qux"></a>')

    def test_1463026_3(self):
        result = self.ioclass()
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
        # a bug whereby a KeyError is raised because this namespace is missing
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
        result = self.ioclass()
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
        # a bug whereby a KeyError is raised because this namespace is missing
        # from a dictionary.
        #
        # This test demonstrates the bug by direct manipulation of the
        # XMLGenerator.
        result = self.ioclass()
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

    def test_no_close_file(self):
        result = self.ioclass()
        def func(out):
            gen = XMLGenerator(out)
            gen.startDocument()
            gen.startElement("doc", {})
        func(result)
        self.assertFalse(result.closed)

    def test_xmlgen_fragment(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        # Don't call gen.startDocument()
        gen.startElement("foo", {"a": "1.0"})
        gen.characters("Hello")
        gen.endElement("foo")
        gen.startElement("bar", {"b": "2.0"})
        gen.endElement("bar")
        # Don't call gen.endDocument()

        self.assertEqual(result.getvalue(),
                         '<foo a="1.0">Hello</foo><bar b="2.0"></bar>')

class StringXmlgenTest(XmlgenTest, unittest.TestCase):
    ioclass = StringIO

class BytesIOXmlgenTest(XmlgenTest, unittest.TestCase):
    ioclass = io.BytesIO

class WriterXmlgenTest(XmlgenTest, unittest.TestCase):
    class ioclass(list):
        write = list.append
        closed = False

        def getvalue(self):
            return b''.join(self)


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

xml_test_out = open(TEST_XMLFILE_OUT).read()

class ExpatReaderTest(XmlTestBase):

    # ===== XMLReader support

    def test_expat_binary_file(self):
        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(open(TEST_XMLFILE))

        self.assertEqual(result.getvalue(), xml_test_out)

    @requires_unicode_filenames
    def test_expat_file_unicode(self):
        fname = support.TESTFN_UNICODE
        shutil.copyfile(TEST_XMLFILE, fname)
        self.addCleanup(support.unlink, fname)

        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(open(fname))

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
        self.assertTrue(attrs.has_key((ns_uri, "attr")))
        self.assertEqual(attrs.get((ns_uri, "attr")), "val")
        self.assertEqual(attrs.get((ns_uri, "attr"), 25), "val")
        self.assertEqual(attrs.items(), [((ns_uri, "attr"), "val")])
        self.assertEqual(attrs.values(), ["val"])
        self.assertEqual(attrs.getValue((ns_uri, "attr")), "val")
        self.assertEqual(attrs[(ns_uri, "attr")], "val")

    # ===== InputSource support

    def test_expat_inpsource_filename(self):
        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(TEST_XMLFILE)

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_inpsource_sysid(self):
        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(InputSource(TEST_XMLFILE))

        self.assertEqual(result.getvalue(), xml_test_out)

    @requires_unicode_filenames
    def test_expat_inpsource_sysid_unicode(self):
        fname = support.TESTFN_UNICODE
        shutil.copyfile(TEST_XMLFILE, fname)
        self.addCleanup(support.unlink, fname)

        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(InputSource(fname))

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_inpsource_byte_stream(self):
        parser = create_parser()
        result = StringIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        inpsrc = InputSource()
        inpsrc.setByteStream(open(TEST_XMLFILE))
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
        parser.parse(TEST_XMLFILE)

        self.assertEqual(parser.getSystemId(), TEST_XMLFILE)
        self.assertEqual(parser.getPublicId(), None)

    @requires_unicode_filenames
    def test_expat_locator_withinfo_unicode(self):
        fname = support.TESTFN_UNICODE
        shutil.copyfile(TEST_XMLFILE, fname)
        self.addCleanup(support.unlink, fname)

        result = StringIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)
        parser.parse(fname)

        self.assertEqual(parser.getSystemId(), fname)
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
        except SAXException, e:
            self.assertEqual(e.getSystemId(), name)

    def test_expat_incomplete(self):
        parser = create_parser()
        parser.setContentHandler(ContentHandler()) # do nothing
        self.assertRaises(SAXParseException, parser.parse, StringIO("<foo>"))
        self.assertEqual(parser.getColumnNumber(), 5)
        self.assertEqual(parser.getLineNumber(), 1)

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
        self.assertTrue(attrs.has_key((ns_uri, "attr")))
        self.assertEqual(attrs.keys(), [(ns_uri, "attr")])
        self.assertEqual(attrs.get((ns_uri, "attr")), "val")
        self.assertEqual(attrs.get((ns_uri, "attr"), 25), "val")
        self.assertEqual(attrs.items(), [((ns_uri, "attr"), "val")])
        self.assertEqual(attrs.values(), ["val"])
        self.assertEqual(attrs.getValue((ns_uri, "attr")), "val")
        self.assertEqual(attrs.getValueByQName("ns:attr"), "val")
        self.assertEqual(attrs.getNameByQName("ns:attr"), (ns_uri, "attr"))
        self.assertEqual(attrs[(ns_uri, "attr")], "val")
        self.assertEqual(attrs.getQNameByName((ns_uri, "attr")), "ns:attr")


    # During the development of Python 2.5, an attempt to move the "xml"
    # package implementation to a new package ("xmlcore") proved painful.
    # The goal of this change was to allow applications to be able to
    # obtain and rely on behavior in the standard library implementation
    # of the XML support without needing to be concerned about the
    # availability of the PyXML implementation.
    #
    # While the existing import hackery in Lib/xml/__init__.py can cause
    # PyXML's _xmlpus package to supplant the "xml" package, that only
    # works because either implementation uses the "xml" package name for
    # imports.
    #
    # The move resulted in a number of problems related to the fact that
    # the import machinery's "package context" is based on the name that's
    # being imported rather than the __name__ of the actual package
    # containment; it wasn't possible for the "xml" package to be replaced
    # by a simple module that indirected imports to the "xmlcore" package.
    #
    # The following two tests exercised bugs that were introduced in that
    # attempt.  Keeping these tests around will help detect problems with
    # other attempts to provide reliable access to the standard library's
    # implementation of the XML support.

    def test_sf_1511497(self):
        # Bug report: http://www.python.org/sf/1511497
        import sys
        old_modules = sys.modules.copy()
        for modname in sys.modules.keys():
            if modname.startswith("xml."):
                del sys.modules[modname]
        try:
            import xml.sax.expatreader
            module = xml.sax.expatreader
            self.assertEqual(module.__name__, "xml.sax.expatreader")
        finally:
            sys.modules.update(old_modules)

    def test_sf_1513611(self):
        # Bug report: http://www.python.org/sf/1513611
        sio = StringIO("invalid")
        parser = make_parser()
        from xml.sax import SAXParseException
        self.assertRaises(SAXParseException, parser.parse, sio)


def test_main():
    run_unittest(MakeParserTest,
                 ParseTest,
                 SaxutilsTest,
                 PrepareInputSourceTest,
                 StringXmlgenTest,
                 BytesIOXmlgenTest,
                 WriterXmlgenTest,
                 ExpatReaderTest,
                 ErrorReportingTest,
                 XmlReaderTest)

if __name__ == "__main__":
    test_main()
