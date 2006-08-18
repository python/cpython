# regression test for SAX 2.0            -*- coding: iso-8859-1 -*-
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
from xml.sax.xmlreader import InputSource, AttributesImpl, AttributesNSImpl
from cStringIO import StringIO
from test.test_support import verify, verbose, TestFailed, findfile
import os

# ===== Utilities

tests = 0
failures = []

def confirm(outcome, name):
    global tests

    tests = tests + 1
    if outcome:
        if verbose:
            print "Passed", name
    else:
        failures.append(name)

def test_make_parser2():
    try:
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
    except:
        return 0
    else:
        return p


# ===========================================================================
#
#   saxutils tests
#
# ===========================================================================

# ===== escape

def test_escape_basic():
    return escape("Donald Duck & Co") == "Donald Duck &amp; Co"

def test_escape_all():
    return escape("<Donald Duck & Co>") == "&lt;Donald Duck &amp; Co&gt;"

def test_escape_extra():
    return escape("Hei på deg", {"å" : "&aring;"}) == "Hei p&aring; deg"

# ===== unescape

def test_unescape_basic():
    return unescape("Donald Duck &amp; Co") == "Donald Duck & Co"

def test_unescape_all():
    return unescape("&lt;Donald Duck &amp; Co&gt;") == "<Donald Duck & Co>"

def test_unescape_extra():
    return unescape("Hei på deg", {"å" : "&aring;"}) == "Hei p&aring; deg"

def test_unescape_amp_extra():
    return unescape("&amp;foo;", {"&foo;": "splat"}) == "&foo;"

# ===== quoteattr

def test_quoteattr_basic():
    return quoteattr("Donald Duck & Co") == '"Donald Duck &amp; Co"'

def test_single_quoteattr():
    return (quoteattr('Includes "double" quotes')
            == '\'Includes "double" quotes\'')

def test_double_quoteattr():
    return (quoteattr("Includes 'single' quotes")
            == "\"Includes 'single' quotes\"")

def test_single_double_quoteattr():
    return (quoteattr("Includes 'single' and \"double\" quotes")
            == "\"Includes 'single' and &quot;double&quot; quotes\"")

# ===== make_parser

def test_make_parser():
    try:
        # Creating a parser should succeed - it should fall back
        # to the expatreader
        p = make_parser(['xml.parsers.no_such_parser'])
    except:
        return 0
    else:
        return p


# ===== XMLGenerator

start = '<?xml version="1.0" encoding="iso-8859-1"?>\n'

def test_xmlgen_basic():
    result = StringIO()
    gen = XMLGenerator(result)
    gen.startDocument()
    gen.startElement("doc", {})
    gen.endElement("doc")
    gen.endDocument()

    return result.getvalue() == start + "<doc></doc>"

def test_xmlgen_content():
    result = StringIO()
    gen = XMLGenerator(result)

    gen.startDocument()
    gen.startElement("doc", {})
    gen.characters("huhei")
    gen.endElement("doc")
    gen.endDocument()

    return result.getvalue() == start + "<doc>huhei</doc>"

def test_xmlgen_pi():
    result = StringIO()
    gen = XMLGenerator(result)

    gen.startDocument()
    gen.processingInstruction("test", "data")
    gen.startElement("doc", {})
    gen.endElement("doc")
    gen.endDocument()

    return result.getvalue() == start + "<?test data?><doc></doc>"

def test_xmlgen_content_escape():
    result = StringIO()
    gen = XMLGenerator(result)

    gen.startDocument()
    gen.startElement("doc", {})
    gen.characters("<huhei&")
    gen.endElement("doc")
    gen.endDocument()

    return result.getvalue() == start + "<doc>&lt;huhei&amp;</doc>"

def test_xmlgen_attr_escape():
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

    return result.getvalue() == start + ("<doc a='\"'><e a=\"'\"></e>"
                                         "<e a=\"'&quot;\"></e>"
                                         "<e a=\"&#10;&#13;&#9;\"></e></doc>")

def test_xmlgen_ignorable():
    result = StringIO()
    gen = XMLGenerator(result)

    gen.startDocument()
    gen.startElement("doc", {})
    gen.ignorableWhitespace(" ")
    gen.endElement("doc")
    gen.endDocument()

    return result.getvalue() == start + "<doc> </doc>"

ns_uri = "http://www.python.org/xml-ns/saxtest/"

def test_xmlgen_ns():
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

    return result.getvalue() == start + \
           ('<ns1:doc xmlns:ns1="%s"><udoc></udoc></ns1:doc>' %
                                         ns_uri)

# ===== XMLFilterBase

def test_filter_basic():
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

    return result.getvalue() == start + "<doc>content </doc>"

# ===========================================================================
#
#   expatreader tests
#
# ===========================================================================

# ===== XMLReader support

def test_expat_file():
    parser = create_parser()
    result = StringIO()
    xmlgen = XMLGenerator(result)

    parser.setContentHandler(xmlgen)
    parser.parse(open(findfile("test"+os.extsep+"xml")))

    return result.getvalue() == xml_test_out

# ===== DTDHandler support

class TestDTDHandler:

    def __init__(self):
        self._notations = []
        self._entities  = []

    def notationDecl(self, name, publicId, systemId):
        self._notations.append((name, publicId, systemId))

    def unparsedEntityDecl(self, name, publicId, systemId, ndata):
        self._entities.append((name, publicId, systemId, ndata))

def test_expat_dtdhandler():
    parser = create_parser()
    handler = TestDTDHandler()
    parser.setDTDHandler(handler)

    parser.feed('<!DOCTYPE doc [\n')
    parser.feed('  <!ENTITY img SYSTEM "expat.gif" NDATA GIF>\n')
    parser.feed('  <!NOTATION GIF PUBLIC "-//CompuServe//NOTATION Graphics Interchange Format 89a//EN">\n')
    parser.feed(']>\n')
    parser.feed('<doc></doc>')
    parser.close()

    return handler._notations == [("GIF", "-//CompuServe//NOTATION Graphics Interchange Format 89a//EN", None)] and \
           handler._entities == [("img", None, "expat.gif", "GIF")]

# ===== EntityResolver support

class TestEntityResolver:

    def resolveEntity(self, publicId, systemId):
        inpsrc = InputSource()
        inpsrc.setByteStream(StringIO("<entity/>"))
        return inpsrc

def test_expat_entityresolver():
    parser = create_parser()
    parser.setEntityResolver(TestEntityResolver())
    result = StringIO()
    parser.setContentHandler(XMLGenerator(result))

    parser.feed('<!DOCTYPE doc [\n')
    parser.feed('  <!ENTITY test SYSTEM "whatever">\n')
    parser.feed(']>\n')
    parser.feed('<doc>&test;</doc>')
    parser.close()

    return result.getvalue() == start + "<doc><entity></entity></doc>"

# ===== Attributes support

class AttrGatherer(ContentHandler):

    def startElement(self, name, attrs):
        self._attrs = attrs

    def startElementNS(self, name, qname, attrs):
        self._attrs = attrs

def test_expat_attrs_empty():
    parser = create_parser()
    gather = AttrGatherer()
    parser.setContentHandler(gather)

    parser.feed("<doc/>")
    parser.close()

    return verify_empty_attrs(gather._attrs)

def test_expat_attrs_wattr():
    parser = create_parser()
    gather = AttrGatherer()
    parser.setContentHandler(gather)

    parser.feed("<doc attr='val'/>")
    parser.close()

    return verify_attrs_wattr(gather._attrs)

def test_expat_nsattrs_empty():
    parser = create_parser(1)
    gather = AttrGatherer()
    parser.setContentHandler(gather)

    parser.feed("<doc/>")
    parser.close()

    return verify_empty_nsattrs(gather._attrs)

def test_expat_nsattrs_wattr():
    parser = create_parser(1)
    gather = AttrGatherer()
    parser.setContentHandler(gather)

    parser.feed("<doc xmlns:ns='%s' ns:attr='val'/>" % ns_uri)
    parser.close()

    attrs = gather._attrs

    return attrs.getLength() == 1 and \
           attrs.getNames() == [(ns_uri, "attr")] and \
           (attrs.getQNames() == [] or attrs.getQNames() == ["ns:attr"]) and \
           len(attrs) == 1 and \
           (ns_uri, "attr") in attrs and \
           attrs.keys() == [(ns_uri, "attr")] and \
           attrs.get((ns_uri, "attr")) == "val" and \
           attrs.get((ns_uri, "attr"), 25) == "val" and \
           attrs.items() == [((ns_uri, "attr"), "val")] and \
           attrs.values() == ["val"] and \
           attrs.getValue((ns_uri, "attr")) == "val" and \
           attrs[(ns_uri, "attr")] == "val"

# ===== InputSource support

xml_test_out = open(findfile("test"+os.extsep+"xml"+os.extsep+"out")).read()

def test_expat_inpsource_filename():
    parser = create_parser()
    result = StringIO()
    xmlgen = XMLGenerator(result)

    parser.setContentHandler(xmlgen)
    parser.parse(findfile("test"+os.extsep+"xml"))

    return result.getvalue() == xml_test_out

def test_expat_inpsource_sysid():
    parser = create_parser()
    result = StringIO()
    xmlgen = XMLGenerator(result)

    parser.setContentHandler(xmlgen)
    parser.parse(InputSource(findfile("test"+os.extsep+"xml")))

    return result.getvalue() == xml_test_out

def test_expat_inpsource_stream():
    parser = create_parser()
    result = StringIO()
    xmlgen = XMLGenerator(result)

    parser.setContentHandler(xmlgen)
    inpsrc = InputSource()
    inpsrc.setByteStream(open(findfile("test"+os.extsep+"xml")))
    parser.parse(inpsrc)

    return result.getvalue() == xml_test_out

# ===== IncrementalParser support

def test_expat_incremental():
    result = StringIO()
    xmlgen = XMLGenerator(result)
    parser = create_parser()
    parser.setContentHandler(xmlgen)

    parser.feed("<doc>")
    parser.feed("</doc>")
    parser.close()

    return result.getvalue() == start + "<doc></doc>"

def test_expat_incremental_reset():
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

    return result.getvalue() == start + "<doc>text</doc>"

# ===== Locator support

def test_expat_locator_noinfo():
    result = StringIO()
    xmlgen = XMLGenerator(result)
    parser = create_parser()
    parser.setContentHandler(xmlgen)

    parser.feed("<doc>")
    parser.feed("</doc>")
    parser.close()

    return parser.getSystemId() is None and \
           parser.getPublicId() is None and \
           parser.getLineNumber() == 1

def test_expat_locator_withinfo():
    result = StringIO()
    xmlgen = XMLGenerator(result)
    parser = create_parser()
    parser.setContentHandler(xmlgen)
    parser.parse(findfile("test.xml"))

    return parser.getSystemId() == findfile("test.xml") and \
           parser.getPublicId() is None


# ===========================================================================
#
#   error reporting
#
# ===========================================================================

def test_expat_inpsource_location():
    parser = create_parser()
    parser.setContentHandler(ContentHandler()) # do nothing
    source = InputSource()
    source.setByteStream(StringIO("<foo bar foobar>"))   #ill-formed
    name = "a file name"
    source.setSystemId(name)
    try:
        parser.parse(source)
    except SAXException, e:
        return e.getSystemId() == name

def test_expat_incomplete():
    parser = create_parser()
    parser.setContentHandler(ContentHandler()) # do nothing
    try:
        parser.parse(StringIO("<foo>"))
    except SAXParseException:
        return 1 # ok, error found
    else:
        return 0

def test_sax_parse_exception_str():
    # pass various values from a locator to the SAXParseException to
    # make sure that the __str__() doesn't fall apart when None is
    # passed instead of an integer line and column number
    #
    # use "normal" values for the locator:
    str(SAXParseException("message", None,
                          DummyLocator(1, 1)))
    # use None for the line number:
    str(SAXParseException("message", None,
                          DummyLocator(None, 1)))
    # use None for the column number:
    str(SAXParseException("message", None,
                          DummyLocator(1, None)))
    # use None for both:
    str(SAXParseException("message", None,
                          DummyLocator(None, None)))
    return 1

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

# ===== AttributesImpl

def verify_empty_attrs(attrs):
    try:
        attrs.getValue("attr")
        gvk = 0
    except KeyError:
        gvk = 1

    try:
        attrs.getValueByQName("attr")
        gvqk = 0
    except KeyError:
        gvqk = 1

    try:
        attrs.getNameByQName("attr")
        gnqk = 0
    except KeyError:
        gnqk = 1

    try:
        attrs.getQNameByName("attr")
        gqnk = 0
    except KeyError:
        gqnk = 1

    try:
        attrs["attr"]
        gik = 0
    except KeyError:
        gik = 1

    return attrs.getLength() == 0 and \
           attrs.getNames() == [] and \
           attrs.getQNames() == [] and \
           len(attrs) == 0 and \
           "attr" not in  attrs and \
           attrs.keys() == [] and \
           attrs.get("attrs") is None and \
           attrs.get("attrs", 25) == 25 and \
           attrs.items() == [] and \
           attrs.values() == [] and \
           gvk and gvqk and gnqk and gik and gqnk

def verify_attrs_wattr(attrs):
    return attrs.getLength() == 1 and \
           attrs.getNames() == ["attr"] and \
           attrs.getQNames() == ["attr"] and \
           len(attrs) == 1 and \
           "attr" in attrs and \
           attrs.keys() == ["attr"] and \
           attrs.get("attr") == "val" and \
           attrs.get("attr", 25) == "val" and \
           attrs.items() == [("attr", "val")] and \
           attrs.values() == ["val"] and \
           attrs.getValue("attr") == "val" and \
           attrs.getValueByQName("attr") == "val" and \
           attrs.getNameByQName("attr") == "attr" and \
           attrs["attr"] == "val" and \
           attrs.getQNameByName("attr") == "attr"

def test_attrs_empty():
    return verify_empty_attrs(AttributesImpl({}))

def test_attrs_wattr():
    return verify_attrs_wattr(AttributesImpl({"attr" : "val"}))

# ===== AttributesImpl

def verify_empty_nsattrs(attrs):
    try:
        attrs.getValue((ns_uri, "attr"))
        gvk = 0
    except KeyError:
        gvk = 1

    try:
        attrs.getValueByQName("ns:attr")
        gvqk = 0
    except KeyError:
        gvqk = 1

    try:
        attrs.getNameByQName("ns:attr")
        gnqk = 0
    except KeyError:
        gnqk = 1

    try:
        attrs.getQNameByName((ns_uri, "attr"))
        gqnk = 0
    except KeyError:
        gqnk = 1

    try:
        attrs[(ns_uri, "attr")]
        gik = 0
    except KeyError:
        gik = 1

    return attrs.getLength() == 0 and \
           attrs.getNames() == [] and \
           attrs.getQNames() == [] and \
           len(attrs) == 0 and \
           (ns_uri, "attr") not in attrs and \
           attrs.keys() == [] and \
           attrs.get((ns_uri, "attr")) is None and \
           attrs.get((ns_uri, "attr"), 25) == 25 and \
           attrs.items() == [] and \
           attrs.values() == [] and \
           gvk and gvqk and gnqk and gik and gqnk

def test_nsattrs_empty():
    return verify_empty_nsattrs(AttributesNSImpl({}, {}))

def test_nsattrs_wattr():
    attrs = AttributesNSImpl({(ns_uri, "attr") : "val"},
                             {(ns_uri, "attr") : "ns:attr"})

    return attrs.getLength() == 1 and \
           attrs.getNames() == [(ns_uri, "attr")] and \
           attrs.getQNames() == ["ns:attr"] and \
           len(attrs) == 1 and \
           (ns_uri, "attr") in attrs and \
           attrs.keys() == [(ns_uri, "attr")] and \
           attrs.get((ns_uri, "attr")) == "val" and \
           attrs.get((ns_uri, "attr"), 25) == "val" and \
           attrs.items() == [((ns_uri, "attr"), "val")] and \
           attrs.values() == ["val"] and \
           attrs.getValue((ns_uri, "attr")) == "val" and \
           attrs.getValueByQName("ns:attr") == "val" and \
           attrs.getNameByQName("ns:attr") == (ns_uri, "attr") and \
           attrs[(ns_uri, "attr")] == "val" and \
           attrs.getQNameByName((ns_uri, "attr")) == "ns:attr"


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

def test_sf_1511497():
    # Bug report: http://www.python.org/sf/1511497
    import sys
    old_modules = sys.modules.copy()
    for modname in sys.modules.keys():
        if modname.startswith("xml."):
            del sys.modules[modname]
    try:
        import xml.sax.expatreader
        module = xml.sax.expatreader
        return module.__name__ == "xml.sax.expatreader"
    finally:
        sys.modules.update(old_modules)

def test_sf_1513611():
    # Bug report: http://www.python.org/sf/1513611
    sio = StringIO("invalid")
    parser = make_parser()
    from xml.sax import SAXParseException
    try:
        parser.parse(sio)
    except SAXParseException:
        return True
    else:
        return False

# ===== Main program

def make_test_output():
    parser = create_parser()
    result = StringIO()
    xmlgen = XMLGenerator(result)

    parser.setContentHandler(xmlgen)
    parser.parse(findfile("test"+os.extsep+"xml"))

    outf = open(findfile("test"+os.extsep+"xml"+os.extsep+"out"), "w")
    outf.write(result.getvalue())
    outf.close()

items = locals().items()
items.sort()
for (name, value) in items:
    if name[ : 5] == "test_":
        confirm(value(), name)
# We delete the items variable so that the assignment to items above
# doesn't pick up the old value of items (which messes with attempts
# to find reference leaks).
del items

if verbose:
    print "%d tests, %d failures" % (tests, len(failures))
if failures:
    raise TestFailed("%d of %d tests failed: %s"
                     % (len(failures), tests, ", ".join(failures)))
