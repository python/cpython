
# regression test for SAX 2.0
# $Id$

from xml.sax.saxutils import XMLGenerator, escape, XMLFilterBase
from xml.sax.expatreader import create_parser
from cStringIO import StringIO
from test_support import verbose, TestFailed

# ===== Utilities

tests = 0
fails = 0

def confirm(outcome, name):
    global tests, fails

    tests = tests + 1
    if outcome:
        print "Passed", name
    else:
        print "Failed", name
        fails = fails + 1

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
    gen.startElementNS((ns_uri, "doc"), "ns:doc", {})
    gen.endElementNS((ns_uri, "doc"), "ns:doc")
    gen.endPrefixMapping("ns1")
    gen.endDocument()

    return result.getvalue() == start + ('<ns1:doc xmlns:ns1="%s"></ns1:doc>' %
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

# ===== DTDHandler support

class TestDTDHandler:

    def __init__(self):
        self._notations = []
        self._entities  = []
    
    def notationDecl(self, name, publicId, systemId):
        self._notations.append((name, publicId, systemId))

    def unparsedEntityDecl(self, name, publicId, systemId, ndata):
        self._entities.append((name, publicId, systemId, ndata))

#  def test_expat_dtdhandler():
#      parser = create_parser()
#      handler = TestDTDHandler()
#      parser.setDTDHandler(handler)

#      parser.feed('<!DOCTYPE doc [\n')
#      parser.feed('  <!ENTITY img SYSTEM "expat.gif" NDATA GIF>\n')
#      parser.feed('  <!NOTATION GIF PUBLIC "-//CompuServe//NOTATION Graphics Interchange Format 89a//EN">\n')
#      parser.feed(']>\n')
#      parser.feed('<doc></doc>')
#      parser.close()

#      return handler._notations == [("GIF", "-//CompuServe//NOTATION Graphics Interchange Format 89a//EN", None)] and \
#             handler._entities == [("img", None, "expat.gif", "GIF")]

# ===== EntityResolver support

# can't test this until InputSource is in place

# ===== Main program

items = locals().items()
items.sort()
for (name, value) in items:
    if name[ : 5] == "test_":
        confirm(value(), name)

print "%d tests, %d failures" % (tests, fails)
if fails != 0:
    raise TestFailed, "%d of %d tests failed" % (fails, tests)
