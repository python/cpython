# regression test for SAX 2.0

from xml.sax import make_parser, ContentHandler, \
                    SAXException, SAXReaderNotAvailable, SAXParseException
import unittest
from unittest import mock
try:
    make_parser()
except SAXReaderNotAvailable:
    # don't try to test this module if we cannot create a parser
    raise unittest.SkipTest("no XML parsers available")
from xml.sax.saxutils import XMLGenerator, escape, unescape, quoteattr, \
                             XMLFilterBase, prepare_input_source
from xml.sax.expatreader import create_parser
from xml.sax.handler import (feature_namespaces, feature_external_ges,
                             LexicalHandler)
from xml.sax.xmlreader import InputSource, AttributesImpl, AttributesNSImpl
from xml import sax
from io import BytesIO, StringIO
import codecs
import os.path
import pyexpat
import shutil
import sys
from urllib.error import URLError
import urllib.request
from test.support import os_helper
from test.support import findfile, check__all__
from test.support.os_helper import FakePath, TESTFN


TEST_XMLFILE = findfile("test.xml", subdir="xmltestdata")
TEST_XMLFILE_OUT = findfile("test.xml.out", subdir="xmltestdata")
try:
    TEST_XMLFILE.encode("utf-8")
    TEST_XMLFILE_OUT.encode("utf-8")
except UnicodeEncodeError:
    raise unittest.SkipTest("filename is not encodable to utf8")

supports_nonascii_filenames = True
if not os.path.supports_unicode_filenames:
    try:
        os_helper.TESTFN_UNICODE.encode(sys.getfilesystemencoding())
    except (UnicodeError, TypeError):
        # Either the file system encoding is None, or the file name
        # cannot be encoded in the file system encoding.
        supports_nonascii_filenames = False
requires_nonascii_filenames = unittest.skipUnless(
        supports_nonascii_filenames,
        'Requires non-ascii filenames support')

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
        self.assertNotIn("attr", attrs)
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
        self.assertNotIn((ns_uri, "attr"), attrs)
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
        self.assertIn("attr", attrs)
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


def xml_str(doc, encoding=None):
    if encoding is None:
        return doc
    return '<?xml version="1.0" encoding="%s"?>\n%s' % (encoding, doc)

def xml_bytes(doc, encoding, decl_encoding=...):
    if decl_encoding is ...:
        decl_encoding = encoding
    return xml_str(doc, decl_encoding).encode(encoding, 'xmlcharrefreplace')

def make_xml_file(doc, encoding, decl_encoding=...):
    if decl_encoding is ...:
        decl_encoding = encoding
    with open(TESTFN, 'w', encoding=encoding, errors='xmlcharrefreplace') as f:
        f.write(xml_str(doc, decl_encoding))


class ParseTest(unittest.TestCase):
    data = '<money value="$\xa3\u20ac\U0001017b">$\xa3\u20ac\U0001017b</money>'

    def tearDown(self):
        os_helper.unlink(TESTFN)

    def check_parse(self, f):
        from xml.sax import parse
        result = StringIO()
        parse(f, XMLGenerator(result, 'utf-8'))
        self.assertEqual(result.getvalue(), xml_str(self.data, 'utf-8'))

    def test_parse_text(self):
        encodings = ('us-ascii', 'iso-8859-1', 'utf-8',
                     'utf-16', 'utf-16le', 'utf-16be')
        for encoding in encodings:
            self.check_parse(StringIO(xml_str(self.data, encoding)))
            make_xml_file(self.data, encoding)
            with open(TESTFN, 'r', encoding=encoding) as f:
                self.check_parse(f)
            self.check_parse(StringIO(self.data))
            make_xml_file(self.data, encoding, None)
            with open(TESTFN, 'r', encoding=encoding) as f:
                self.check_parse(f)

    def test_parse_bytes(self):
        # UTF-8 is default encoding, US-ASCII is compatible with UTF-8,
        # UTF-16 is autodetected
        encodings = ('us-ascii', 'utf-8', 'utf-16', 'utf-16le', 'utf-16be')
        for encoding in encodings:
            self.check_parse(BytesIO(xml_bytes(self.data, encoding)))
            make_xml_file(self.data, encoding)
            self.check_parse(TESTFN)
            with open(TESTFN, 'rb') as f:
                self.check_parse(f)
            self.check_parse(BytesIO(xml_bytes(self.data, encoding, None)))
            make_xml_file(self.data, encoding, None)
            self.check_parse(TESTFN)
            with open(TESTFN, 'rb') as f:
                self.check_parse(f)
        # accept UTF-8 with BOM
        self.check_parse(BytesIO(xml_bytes(self.data, 'utf-8-sig', 'utf-8')))
        make_xml_file(self.data, 'utf-8-sig', 'utf-8')
        self.check_parse(TESTFN)
        with open(TESTFN, 'rb') as f:
            self.check_parse(f)
        self.check_parse(BytesIO(xml_bytes(self.data, 'utf-8-sig', None)))
        make_xml_file(self.data, 'utf-8-sig', None)
        self.check_parse(TESTFN)
        with open(TESTFN, 'rb') as f:
            self.check_parse(f)
        # accept data with declared encoding
        self.check_parse(BytesIO(xml_bytes(self.data, 'iso-8859-1')))
        make_xml_file(self.data, 'iso-8859-1')
        self.check_parse(TESTFN)
        with open(TESTFN, 'rb') as f:
            self.check_parse(f)
        # fail on non-UTF-8 incompatible data without declared encoding
        with self.assertRaises(SAXException):
            self.check_parse(BytesIO(xml_bytes(self.data, 'iso-8859-1', None)))
        make_xml_file(self.data, 'iso-8859-1', None)
        with self.assertRaises(SAXException):
            self.check_parse(TESTFN)
        with open(TESTFN, 'rb') as f:
            with self.assertRaises(SAXException):
                self.check_parse(f)

    def test_parse_path_object(self):
        make_xml_file(self.data, 'utf-8', None)
        self.check_parse(FakePath(TESTFN))

    def test_parse_InputSource(self):
        # accept data without declared but with explicitly specified encoding
        make_xml_file(self.data, 'iso-8859-1', None)
        with open(TESTFN, 'rb') as f:
            input = InputSource()
            input.setByteStream(f)
            input.setEncoding('iso-8859-1')
            self.check_parse(input)

    def test_parse_close_source(self):
        builtin_open = open
        fileobj = None

        def mock_open(*args):
            nonlocal fileobj
            fileobj = builtin_open(*args)
            return fileobj

        with mock.patch('xml.sax.saxutils.open', side_effect=mock_open):
            make_xml_file(self.data, 'iso-8859-1', None)
            with self.assertRaises(SAXException):
                self.check_parse(TESTFN)
            self.assertTrue(fileobj.closed)

    def check_parseString(self, s):
        from xml.sax import parseString
        result = StringIO()
        parseString(s, XMLGenerator(result, 'utf-8'))
        self.assertEqual(result.getvalue(), xml_str(self.data, 'utf-8'))

    def test_parseString_text(self):
        encodings = ('us-ascii', 'iso-8859-1', 'utf-8',
                     'utf-16', 'utf-16le', 'utf-16be')
        for encoding in encodings:
            self.check_parseString(xml_str(self.data, encoding))
        self.check_parseString(self.data)

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

    def test_make_parser3(self):
        # Testing that make_parser can handle different types of
        # iterables.
        make_parser(['module'])
        make_parser(('module', ))
        make_parser({'module'})
        make_parser(frozenset({'module'}))
        make_parser({'module': None})
        make_parser(iter(['module']))

    def test_make_parser4(self):
        # Testing that make_parser can handle empty iterables.
        make_parser([])
        make_parser(tuple())
        make_parser(set())
        make_parser(frozenset())
        make_parser({})
        make_parser(iter([]))

    def test_make_parser5(self):
        # Testing that make_parser can handle iterables with more than
        # one item.
        make_parser(['module1', 'module2'])
        make_parser(('module1', 'module2'))
        make_parser({'module1', 'module2'})
        make_parser(frozenset({'module1', 'module2'}))
        make_parser({'module1': None, 'module2': None})
        make_parser(iter(['module1', 'module2']))

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
        self.file = os_helper.TESTFN
        with open(self.file, "w") as tmp:
            tmp.write("This was read from a file.")

    def tearDown(self):
        os_helper.unlink(self.file)

    def make_byte_stream(self):
        return BytesIO(b"This is a byte stream.")

    def make_character_stream(self):
        return StringIO("This is a character stream.")

    def checkContent(self, stream, content):
        self.assertIsNotNone(stream)
        self.assertEqual(stream.read(), content)
        stream.close()


    def test_character_stream(self):
        # If the source is an InputSource with a character stream, use it.
        src = InputSource(self.file)
        src.setCharacterStream(self.make_character_stream())
        prep = prepare_input_source(src)
        self.assertIsNone(prep.getByteStream())
        self.checkContent(prep.getCharacterStream(),
                          "This is a character stream.")

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

    def test_path_objects(self):
        # If the source is a Path object, use it as a system ID and open it.
        prep = prepare_input_source(FakePath(self.file))
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

    def test_text_file(self):
        # If the source is a text file-like object, use it as a character
        # stream.
        prep = prepare_input_source(self.make_character_stream())
        self.assertIsNone(prep.getByteStream())
        self.checkContent(prep.getCharacterStream(),
                          "This is a character stream.")


# ===== XMLGenerator

class XmlgenTest:
    def test_xmlgen_basic(self):
        result = self.ioclass()
        gen = XMLGenerator(result)
        gen.startDocument()
        gen.startElement("doc", {})
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml("<doc></doc>"))

    def test_xmlgen_basic_empty(self):
        result = self.ioclass()
        gen = XMLGenerator(result, short_empty_elements=True)
        gen.startDocument()
        gen.startElement("doc", {})
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml("<doc/>"))

    def test_xmlgen_content(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.characters("huhei")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml("<doc>huhei</doc>"))

    def test_xmlgen_content_empty(self):
        result = self.ioclass()
        gen = XMLGenerator(result, short_empty_elements=True)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.characters("huhei")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml("<doc>huhei</doc>"))

    def test_xmlgen_pi(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.processingInstruction("test", "data")
        gen.startElement("doc", {})
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(),
            self.xml("<?test data?><doc></doc>"))

    def test_xmlgen_content_escape(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.characters("<huhei&")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(),
            self.xml("<doc>&lt;huhei&amp;</doc>"))

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

        self.assertEqual(result.getvalue(), self.xml(
            "<doc a='\"'><e a=\"'\"></e>"
            "<e a=\"'&quot;\"></e>"
            "<e a=\"&#10;&#13;&#9;\"></e></doc>"))

    def test_xmlgen_encoding(self):
        encodings = ('iso-8859-15', 'utf-8', 'utf-8-sig',
                     'utf-16', 'utf-16be', 'utf-16le',
                     'utf-32', 'utf-32be', 'utf-32le')
        for encoding in encodings:
            result = self.ioclass()
            gen = XMLGenerator(result, encoding=encoding)

            gen.startDocument()
            gen.startElement("doc", {"a": '\u20ac'})
            gen.characters("\u20ac")
            gen.endElement("doc")
            gen.endDocument()

            self.assertEqual(result.getvalue(),
                self.xml('<doc a="\u20ac">\u20ac</doc>', encoding=encoding))

    def test_xmlgen_unencodable(self):
        result = self.ioclass()
        gen = XMLGenerator(result, encoding='ascii')

        gen.startDocument()
        gen.startElement("doc", {"a": '\u20ac'})
        gen.characters("\u20ac")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(),
            self.xml('<doc a="&#8364;">&#8364;</doc>', encoding='ascii'))

    def test_xmlgen_ignorable(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.ignorableWhitespace(" ")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml("<doc> </doc>"))

    def test_xmlgen_ignorable_empty(self):
        result = self.ioclass()
        gen = XMLGenerator(result, short_empty_elements=True)

        gen.startDocument()
        gen.startElement("doc", {})
        gen.ignorableWhitespace(" ")
        gen.endElement("doc")
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml("<doc> </doc>"))

    def test_xmlgen_encoding_bytes(self):
        encodings = ('iso-8859-15', 'utf-8', 'utf-8-sig',
                     'utf-16', 'utf-16be', 'utf-16le',
                     'utf-32', 'utf-32be', 'utf-32le')
        for encoding in encodings:
            result = self.ioclass()
            gen = XMLGenerator(result, encoding=encoding)

            gen.startDocument()
            gen.startElement("doc", {"a": '\u20ac'})
            gen.characters("\u20ac".encode(encoding))
            gen.ignorableWhitespace(" ".encode(encoding))
            gen.endElement("doc")
            gen.endDocument()

            self.assertEqual(result.getvalue(),
                self.xml('<doc a="\u20ac">\u20ac </doc>', encoding=encoding))

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

        self.assertEqual(result.getvalue(), self.xml(
           '<ns1:doc xmlns:ns1="%s"><udoc></udoc></ns1:doc>' %
                                         ns_uri))

    def test_xmlgen_ns_empty(self):
        result = self.ioclass()
        gen = XMLGenerator(result, short_empty_elements=True)

        gen.startDocument()
        gen.startPrefixMapping("ns1", ns_uri)
        gen.startElementNS((ns_uri, "doc"), "ns1:doc", {})
        # add an unqualified name
        gen.startElementNS((None, "udoc"), None, {})
        gen.endElementNS((None, "udoc"), None)
        gen.endElementNS((ns_uri, "doc"), "ns1:doc")
        gen.endPrefixMapping("ns1")
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml(
           '<ns1:doc xmlns:ns1="%s"><udoc/></ns1:doc>' %
                                         ns_uri))

    def test_1463026_1(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startElementNS((None, 'a'), 'a', {(None, 'b'):'c'})
        gen.endElementNS((None, 'a'), 'a')
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml('<a b="c"></a>'))

    def test_1463026_1_empty(self):
        result = self.ioclass()
        gen = XMLGenerator(result, short_empty_elements=True)

        gen.startDocument()
        gen.startElementNS((None, 'a'), 'a', {(None, 'b'):'c'})
        gen.endElementNS((None, 'a'), 'a')
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml('<a b="c"/>'))

    def test_1463026_2(self):
        result = self.ioclass()
        gen = XMLGenerator(result)

        gen.startDocument()
        gen.startPrefixMapping(None, 'qux')
        gen.startElementNS(('qux', 'a'), 'a', {})
        gen.endElementNS(('qux', 'a'), 'a')
        gen.endPrefixMapping(None)
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml('<a xmlns="qux"></a>'))

    def test_1463026_2_empty(self):
        result = self.ioclass()
        gen = XMLGenerator(result, short_empty_elements=True)

        gen.startDocument()
        gen.startPrefixMapping(None, 'qux')
        gen.startElementNS(('qux', 'a'), 'a', {})
        gen.endElementNS(('qux', 'a'), 'a')
        gen.endPrefixMapping(None)
        gen.endDocument()

        self.assertEqual(result.getvalue(), self.xml('<a xmlns="qux"/>'))

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
            self.xml('<my:a xmlns:my="qux" b="c"></my:a>'))

    def test_1463026_3_empty(self):
        result = self.ioclass()
        gen = XMLGenerator(result, short_empty_elements=True)

        gen.startDocument()
        gen.startPrefixMapping('my', 'qux')
        gen.startElementNS(('qux', 'a'), 'a', {(None, 'b'):'c'})
        gen.endElementNS(('qux', 'a'), 'a')
        gen.endPrefixMapping('my')
        gen.endDocument()

        self.assertEqual(result.getvalue(),
            self.xml('<my:a xmlns:my="qux" b="c"/>'))

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
                         self.xml(
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
                         self.xml(
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
            self.xml('<foo a="1.0">Hello</foo><bar b="2.0"></bar>')[len(self.xml('')):])

class StringXmlgenTest(XmlgenTest, unittest.TestCase):
    ioclass = StringIO

    def xml(self, doc, encoding='iso-8859-1'):
        return '<?xml version="1.0" encoding="%s"?>\n%s' % (encoding, doc)

    test_xmlgen_unencodable = None

class BytesXmlgenTest(XmlgenTest, unittest.TestCase):
    ioclass = BytesIO

    def xml(self, doc, encoding='iso-8859-1'):
        return ('<?xml version="1.0" encoding="%s"?>\n%s' %
                (encoding, doc)).encode(encoding, 'xmlcharrefreplace')

class WriterXmlgenTest(BytesXmlgenTest):
    class ioclass(list):
        write = list.append
        closed = False

        def seekable(self):
            return True

        def tell(self):
            # return 0 at start and not 0 after start
            return len(self)

        def getvalue(self):
            return b''.join(self)

class StreamWriterXmlgenTest(XmlgenTest, unittest.TestCase):
    def ioclass(self):
        raw = BytesIO()
        writer = codecs.getwriter('ascii')(raw, 'xmlcharrefreplace')
        writer.getvalue = raw.getvalue
        return writer

    def xml(self, doc, encoding='iso-8859-1'):
        return ('<?xml version="1.0" encoding="%s"?>\n%s' %
                (encoding, doc)).encode('ascii', 'xmlcharrefreplace')

class StreamReaderWriterXmlgenTest(XmlgenTest, unittest.TestCase):
    fname = os_helper.TESTFN + '-codecs'

    def ioclass(self):
        with self.assertWarns(DeprecationWarning):
            writer = codecs.open(self.fname, 'w', encoding='ascii',
                                errors='xmlcharrefreplace', buffering=0)
        def cleanup():
            writer.close()
            os_helper.unlink(self.fname)
        self.addCleanup(cleanup)
        def getvalue():
            # Windows will not let use reopen without first closing
            writer.close()
            with open(writer.name, 'rb') as f:
                return f.read()
        writer.getvalue = getvalue
        return writer

    def xml(self, doc, encoding='iso-8859-1'):
        return ('<?xml version="1.0" encoding="%s"?>\n%s' %
                (encoding, doc)).encode('ascii', 'xmlcharrefreplace')

start = b'<?xml version="1.0" encoding="iso-8859-1"?>\n'


class XMLFilterBaseTest(unittest.TestCase):
    def test_filter_basic(self):
        result = BytesIO()
        gen = XMLGenerator(result)
        filter = XMLFilterBase()
        filter.setContentHandler(gen)

        filter.startDocument()
        filter.startElement("doc", {})
        filter.characters("content")
        filter.ignorableWhitespace(" ")
        filter.endElement("doc")
        filter.endDocument()

        self.assertEqual(result.getvalue(), start + b"<doc>content </doc>")

# ===========================================================================
#
#   expatreader tests
#
# ===========================================================================

with open(TEST_XMLFILE_OUT, 'rb') as f:
    xml_test_out = f.read()

class ExpatReaderTest(XmlTestBase):

    # ===== XMLReader support

    def test_expat_binary_file(self):
        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        with open(TEST_XMLFILE, 'rb') as f:
            parser.parse(f)

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_text_file(self):
        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        with open(TEST_XMLFILE, 'rt', encoding='iso-8859-1') as f:
            parser.parse(f)

        self.assertEqual(result.getvalue(), xml_test_out)

    @requires_nonascii_filenames
    def test_expat_binary_file_nonascii(self):
        fname = os_helper.TESTFN_UNICODE
        shutil.copyfile(TEST_XMLFILE, fname)
        self.addCleanup(os_helper.unlink, fname)

        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(open(fname, 'rb'))

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_binary_file_bytes_name(self):
        fname = os.fsencode(TEST_XMLFILE)
        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        with open(fname, 'rb') as f:
            parser.parse(f)

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_binary_file_int_name(self):
        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        with open(TEST_XMLFILE, 'rb') as f:
            with open(f.fileno(), 'rb', closefd=False) as f2:
                parser.parse(f2)

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


    class TestEntityRecorder:
        def __init__(self):
            self.entities = []

        def resolveEntity(self, publicId, systemId):
            self.entities.append((publicId, systemId))
            source = InputSource()
            source.setPublicId(publicId)
            source.setSystemId(systemId)
            return source

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

    def test_expat_external_dtd_enabled(self):
        # clear _opener global variable
        self.addCleanup(urllib.request.urlcleanup)

        parser = create_parser()
        parser.setFeature(feature_external_ges, True)
        resolver = self.TestEntityRecorder()
        parser.setEntityResolver(resolver)

        with self.assertRaises(URLError):
            parser.feed(
                '<!DOCTYPE external SYSTEM "unsupported://non-existing">\n'
            )
        self.assertEqual(
            resolver.entities, [(None, 'unsupported://non-existing')]
        )

    def test_expat_external_dtd_default(self):
        parser = create_parser()
        resolver = self.TestEntityRecorder()
        parser.setEntityResolver(resolver)

        parser.feed(
            '<!DOCTYPE external SYSTEM "unsupported://non-existing">\n'
        )
        parser.feed('<doc />')
        parser.close()
        self.assertEqual(resolver.entities, [])

    # ===== EntityResolver support

    class TestEntityResolver:

        def resolveEntity(self, publicId, systemId):
            inpsrc = InputSource()
            inpsrc.setByteStream(BytesIO(b"<entity/>"))
            return inpsrc

    def test_expat_entityresolver_enabled(self):
        parser = create_parser()
        parser.setFeature(feature_external_ges, True)
        parser.setEntityResolver(self.TestEntityResolver())
        result = BytesIO()
        parser.setContentHandler(XMLGenerator(result))

        parser.feed('<!DOCTYPE doc [\n')
        parser.feed('  <!ENTITY test SYSTEM "whatever">\n')
        parser.feed(']>\n')
        parser.feed('<doc>&test;</doc>')
        parser.close()

        self.assertEqual(result.getvalue(), start +
                         b"<doc><entity></entity></doc>")

    def test_expat_entityresolver_default(self):
        parser = create_parser()
        self.assertEqual(parser.getFeature(feature_external_ges), False)
        parser.setEntityResolver(self.TestEntityResolver())
        result = BytesIO()
        parser.setContentHandler(XMLGenerator(result))

        parser.feed('<!DOCTYPE doc [\n')
        parser.feed('  <!ENTITY test SYSTEM "whatever">\n')
        parser.feed(']>\n')
        parser.feed('<doc>&test;</doc>')
        parser.close()

        self.assertEqual(result.getvalue(), start +
                         b"<doc></doc>")

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
        self.assertIn((ns_uri, "attr"), attrs)
        self.assertEqual(attrs.get((ns_uri, "attr")), "val")
        self.assertEqual(attrs.get((ns_uri, "attr"), 25), "val")
        self.assertEqual(list(attrs.items()), [((ns_uri, "attr"), "val")])
        self.assertEqual(list(attrs.values()), ["val"])
        self.assertEqual(attrs.getValue((ns_uri, "attr")), "val")
        self.assertEqual(attrs[(ns_uri, "attr")], "val")

    # ===== InputSource support

    def test_expat_inpsource_filename(self):
        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(TEST_XMLFILE)

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_inpsource_sysid(self):
        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(InputSource(TEST_XMLFILE))

        self.assertEqual(result.getvalue(), xml_test_out)

    @requires_nonascii_filenames
    def test_expat_inpsource_sysid_nonascii(self):
        fname = os_helper.TESTFN_UNICODE
        shutil.copyfile(TEST_XMLFILE, fname)
        self.addCleanup(os_helper.unlink, fname)

        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        parser.parse(InputSource(fname))

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_inpsource_byte_stream(self):
        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        inpsrc = InputSource()
        with open(TEST_XMLFILE, 'rb') as f:
            inpsrc.setByteStream(f)
            parser.parse(inpsrc)

        self.assertEqual(result.getvalue(), xml_test_out)

    def test_expat_inpsource_character_stream(self):
        parser = create_parser()
        result = BytesIO()
        xmlgen = XMLGenerator(result)

        parser.setContentHandler(xmlgen)
        inpsrc = InputSource()
        with open(TEST_XMLFILE, 'rt', encoding='iso-8859-1') as f:
            inpsrc.setCharacterStream(f)
            parser.parse(inpsrc)

        self.assertEqual(result.getvalue(), xml_test_out)

    # ===== IncrementalParser support

    def test_expat_incremental(self):
        result = BytesIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)

        parser.feed("<doc>")
        parser.feed("</doc>")
        parser.close()

        self.assertEqual(result.getvalue(), start + b"<doc></doc>")

    def test_expat_incremental_reset(self):
        result = BytesIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)

        parser.feed("<doc>")
        parser.feed("text")

        result = BytesIO()
        xmlgen = XMLGenerator(result)
        parser.setContentHandler(xmlgen)
        parser.reset()

        parser.feed("<doc>")
        parser.feed("text")
        parser.feed("</doc>")
        parser.close()

        self.assertEqual(result.getvalue(), start + b"<doc>text</doc>")

    @unittest.skipIf(pyexpat.version_info < (2, 6, 0),
                     f'Expat {pyexpat.version_info} does not '
                     'support reparse deferral')
    def test_flush_reparse_deferral_enabled(self):
        result = BytesIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)

        for chunk in ("<doc", ">"):
            parser.feed(chunk)

        self.assertEqual(result.getvalue(), start)  # i.e. no elements started
        self.assertTrue(parser._parser.GetReparseDeferralEnabled())

        parser.flush()

        self.assertTrue(parser._parser.GetReparseDeferralEnabled())
        self.assertEqual(result.getvalue(), start + b"<doc>")

        parser.feed("</doc>")
        parser.close()

        self.assertEqual(result.getvalue(), start + b"<doc></doc>")

    def test_flush_reparse_deferral_disabled(self):
        result = BytesIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)

        for chunk in ("<doc", ">"):
            parser.feed(chunk)

        if pyexpat.version_info >= (2, 6, 0):
            parser._parser.SetReparseDeferralEnabled(False)
            self.assertEqual(result.getvalue(), start)  # i.e. no elements started

        self.assertFalse(parser._parser.GetReparseDeferralEnabled())

        parser.flush()

        self.assertFalse(parser._parser.GetReparseDeferralEnabled())
        self.assertEqual(result.getvalue(), start + b"<doc>")

        parser.feed("</doc>")
        parser.close()

        self.assertEqual(result.getvalue(), start + b"<doc></doc>")

    # ===== Locator support

    def test_expat_locator_noinfo(self):
        result = BytesIO()
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
        result = BytesIO()
        xmlgen = XMLGenerator(result)
        parser = create_parser()
        parser.setContentHandler(xmlgen)
        parser.parse(TEST_XMLFILE)

        self.assertEqual(parser.getSystemId(), TEST_XMLFILE)
        self.assertEqual(parser.getPublicId(), None)

    @requires_nonascii_filenames
    def test_expat_locator_withinfo_nonascii(self):
        fname = os_helper.TESTFN_UNICODE
        shutil.copyfile(TEST_XMLFILE, fname)
        self.addCleanup(os_helper.unlink, fname)

        result = BytesIO()
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
        source.setByteStream(BytesIO(b"<foo bar foobar>"))   #ill-formed
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
        self.assertIn((ns_uri, "attr"), attrs)
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


class LexicalHandlerTest(unittest.TestCase):
    def setUp(self):
        self.parser = None

        self.specified_version = '1.0'
        self.specified_encoding = 'UTF-8'
        self.specified_doctype = 'wish'
        self.specified_entity_names = ('nbsp', 'source', 'target')
        self.specified_comment = ('Comment in a DTD',
                                  'Really! You think so?')
        self.test_data = StringIO()
        self.test_data.write('<?xml version="{}" encoding="{}"?>\n'.
                             format(self.specified_version,
                                    self.specified_encoding))
        self.test_data.write('<!DOCTYPE {} [\n'.
                             format(self.specified_doctype))
        self.test_data.write('<!-- {} -->\n'.
                             format(self.specified_comment[0]))
        self.test_data.write('<!ELEMENT {} (to,from,heading,body,footer)>\n'.
                             format(self.specified_doctype))
        self.test_data.write('<!ELEMENT to (#PCDATA)>\n')
        self.test_data.write('<!ELEMENT from (#PCDATA)>\n')
        self.test_data.write('<!ELEMENT heading (#PCDATA)>\n')
        self.test_data.write('<!ELEMENT body (#PCDATA)>\n')
        self.test_data.write('<!ELEMENT footer (#PCDATA)>\n')
        self.test_data.write('<!ENTITY {} "&#xA0;">\n'.
                             format(self.specified_entity_names[0]))
        self.test_data.write('<!ENTITY {} "Written by: Alexander.">\n'.
                             format(self.specified_entity_names[1]))
        self.test_data.write('<!ENTITY {} "Hope it gets to: Aristotle.">\n'.
                             format(self.specified_entity_names[2]))
        self.test_data.write(']>\n')
        self.test_data.write('<{}>'.format(self.specified_doctype))
        self.test_data.write('<to>Aristotle</to>\n')
        self.test_data.write('<from>Alexander</from>\n')
        self.test_data.write('<heading>Supplication</heading>\n')
        self.test_data.write('<body>Teach me patience!</body>\n')
        self.test_data.write('<footer>&{};&{};&{};</footer>\n'.
                             format(self.specified_entity_names[1],
                                    self.specified_entity_names[0],
                                    self.specified_entity_names[2]))
        self.test_data.write('<!-- {} -->\n'.format(self.specified_comment[1]))
        self.test_data.write('</{}>\n'.format(self.specified_doctype))
        self.test_data.seek(0)

        # Data received from handlers - to be validated
        self.version = None
        self.encoding = None
        self.standalone = None
        self.doctype = None
        self.publicID = None
        self.systemID = None
        self.end_of_dtd = False
        self.comments = []

    def test_handlers(self):
        class TestLexicalHandler(LexicalHandler):
            def __init__(self, test_harness, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.test_harness = test_harness

            def startDTD(self, doctype, publicID, systemID):
                self.test_harness.doctype = doctype
                self.test_harness.publicID = publicID
                self.test_harness.systemID = systemID

            def endDTD(self):
                self.test_harness.end_of_dtd = True

            def comment(self, text):
                self.test_harness.comments.append(text)

        self.parser = create_parser()
        self.parser.setContentHandler(ContentHandler())
        self.parser.setProperty(
            'http://xml.org/sax/properties/lexical-handler',
            TestLexicalHandler(self))
        source = InputSource()
        source.setCharacterStream(self.test_data)
        self.parser.parse(source)
        self.assertEqual(self.doctype, self.specified_doctype)
        self.assertIsNone(self.publicID)
        self.assertIsNone(self.systemID)
        self.assertTrue(self.end_of_dtd)
        self.assertEqual(len(self.comments),
                         len(self.specified_comment))
        self.assertEqual(f' {self.specified_comment[0]} ', self.comments[0])


class CDATAHandlerTest(unittest.TestCase):
    def setUp(self):
        self.parser = None
        self.specified_chars = []
        self.specified_chars.append(('Parseable character data', False))
        self.specified_chars.append(('<> &% - assorted other XML junk.', True))
        self.char_index = 0  # Used to index specified results within handlers
        self.test_data = StringIO()
        self.test_data.write('<root_doc>\n')
        self.test_data.write('<some_pcdata>\n')
        self.test_data.write(f'{self.specified_chars[0][0]}\n')
        self.test_data.write('</some_pcdata>\n')
        self.test_data.write('<some_cdata>\n')
        self.test_data.write(f'<![CDATA[{self.specified_chars[1][0]}]]>\n')
        self.test_data.write('</some_cdata>\n')
        self.test_data.write('</root_doc>\n')
        self.test_data.seek(0)

        # Data received from handlers - to be validated
        self.chardata = []
        self.in_cdata = False

    def test_handlers(self):
        class TestLexicalHandler(LexicalHandler):
            def __init__(self, test_harness, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.test_harness = test_harness

            def startCDATA(self):
                self.test_harness.in_cdata = True

            def endCDATA(self):
                self.test_harness.in_cdata = False

        class TestCharHandler(ContentHandler):
            def __init__(self, test_harness, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.test_harness = test_harness

            def characters(self, content):
                if content != '\n':
                    h = self.test_harness
                    t = h.specified_chars[h.char_index]
                    h.assertEqual(t[0], content)
                    h.assertEqual(t[1], h.in_cdata)
                    h.char_index += 1

        self.parser = create_parser()
        self.parser.setContentHandler(TestCharHandler(self))
        self.parser.setProperty(
            'http://xml.org/sax/properties/lexical-handler',
            TestLexicalHandler(self))
        source = InputSource()
        source.setCharacterStream(self.test_data)
        self.parser.parse(source)

        self.assertFalse(self.in_cdata)
        self.assertEqual(self.char_index, 2)


class TestModuleAll(unittest.TestCase):
    def test_all(self):
        extra = (
            'ContentHandler',
            'ErrorHandler',
            'InputSource',
            'SAXException',
            'SAXNotRecognizedException',
            'SAXNotSupportedException',
            'SAXParseException',
            'SAXReaderNotAvailable',
        )
        check__all__(self, sax, extra=extra)


if __name__ == "__main__":
    unittest.main()
