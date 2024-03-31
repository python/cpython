import io
import unittest
import xml.sax

from xml.sax.xmlreader import AttributesImpl
from xml.sax.handler import feature_external_ges
from xml.dom import pulldom

from test.support import findfile


tstfile = findfile("test.xml", subdir="xmltestdata")

# A handy XML snippet, containing attributes, a namespace prefix, and a
# self-closing tag:
SMALL_SAMPLE = """<?xml version="1.0"?>
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:xdc="http://www.xml.com/books">
<!-- A comment -->
<title>Introduction to XSL</title>
<hr/>
<p><xdc:author xdc:attrib="prefixed attribute" attrib="other attrib">A. Namespace</xdc:author></p>
</html>"""


class PullDOMTestCase(unittest.TestCase):

    def test_parse(self):
        """Minimal test of DOMEventStream.parse()"""

        # This just tests that parsing from a stream works. Actual parser
        # semantics are tested using parseString with a more focused XML
        # fragment.

        # Test with a filename:
        handler = pulldom.parse(tstfile)
        self.addCleanup(handler.stream.close)
        list(handler)

        # Test with a file object:
        with open(tstfile, "rb") as fin:
            list(pulldom.parse(fin))

    def test_parse_semantics(self):
        """Test DOMEventStream parsing semantics."""

        items = pulldom.parseString(SMALL_SAMPLE)
        evt, node = next(items)
        # Just check the node is a Document:
        self.assertTrue(hasattr(node, "createElement"))
        self.assertEqual(pulldom.START_DOCUMENT, evt)
        evt, node = next(items)
        self.assertEqual(pulldom.START_ELEMENT, evt)
        self.assertEqual("html", node.tagName)
        self.assertEqual(2, len(node.attributes))
        self.assertEqual(node.attributes.getNamedItem("xmlns:xdc").value,
              "http://www.xml.com/books")
        evt, node = next(items)
        self.assertEqual(pulldom.CHARACTERS, evt) # Line break
        evt, node = next(items)
        # XXX - A comment should be reported here!
        # self.assertEqual(pulldom.COMMENT, evt)
        # Line break after swallowed comment:
        self.assertEqual(pulldom.CHARACTERS, evt)
        evt, node = next(items)
        self.assertEqual("title", node.tagName)
        title_node = node
        evt, node = next(items)
        self.assertEqual(pulldom.CHARACTERS, evt)
        self.assertEqual("Introduction to XSL", node.data)
        evt, node = next(items)
        self.assertEqual(pulldom.END_ELEMENT, evt)
        self.assertEqual("title", node.tagName)
        self.assertTrue(title_node is node)
        evt, node = next(items)
        self.assertEqual(pulldom.CHARACTERS, evt)
        evt, node = next(items)
        self.assertEqual(pulldom.START_ELEMENT, evt)
        self.assertEqual("hr", node.tagName)
        evt, node = next(items)
        self.assertEqual(pulldom.END_ELEMENT, evt)
        self.assertEqual("hr", node.tagName)
        evt, node = next(items)
        self.assertEqual(pulldom.CHARACTERS, evt)
        evt, node = next(items)
        self.assertEqual(pulldom.START_ELEMENT, evt)
        self.assertEqual("p", node.tagName)
        evt, node = next(items)
        self.assertEqual(pulldom.START_ELEMENT, evt)
        self.assertEqual("xdc:author", node.tagName)
        evt, node = next(items)
        self.assertEqual(pulldom.CHARACTERS, evt)
        evt, node = next(items)
        self.assertEqual(pulldom.END_ELEMENT, evt)
        self.assertEqual("xdc:author", node.tagName)
        evt, node = next(items)
        self.assertEqual(pulldom.END_ELEMENT, evt)
        evt, node = next(items)
        self.assertEqual(pulldom.CHARACTERS, evt)
        evt, node = next(items)
        self.assertEqual(pulldom.END_ELEMENT, evt)
        # XXX No END_DOCUMENT item is ever obtained:
        #evt, node = next(items)
        #self.assertEqual(pulldom.END_DOCUMENT, evt)

    def test_expandItem(self):
        """Ensure expandItem works as expected."""
        items = pulldom.parseString(SMALL_SAMPLE)
        # Loop through the nodes until we get to a "title" start tag:
        for evt, item in items:
            if evt == pulldom.START_ELEMENT and item.tagName == "title":
                items.expandNode(item)
                self.assertEqual(1, len(item.childNodes))
                break
        else:
            self.fail("No \"title\" element detected in SMALL_SAMPLE!")
        # Loop until we get to the next start-element:
        for evt, node in items:
            if evt == pulldom.START_ELEMENT:
                break
        self.assertEqual("hr", node.tagName,
            "expandNode did not leave DOMEventStream in the correct state.")
        # Attempt to expand a standalone element:
        items.expandNode(node)
        self.assertEqual(next(items)[0], pulldom.CHARACTERS)
        evt, node = next(items)
        self.assertEqual(node.tagName, "p")
        items.expandNode(node)
        next(items) # Skip character data
        evt, node = next(items)
        self.assertEqual(node.tagName, "html")
        with self.assertRaises(StopIteration):
            next(items)
        items.clear()
        self.assertIsNone(items.parser)
        self.assertIsNone(items.stream)

    @unittest.expectedFailure
    def test_comment(self):
        """PullDOM does not receive "comment" events."""
        items = pulldom.parseString(SMALL_SAMPLE)
        for evt, _ in items:
            if evt == pulldom.COMMENT:
                break
        else:
            self.fail("No comment was encountered")

    @unittest.expectedFailure
    def test_end_document(self):
        """PullDOM does not receive "end-document" events."""
        items = pulldom.parseString(SMALL_SAMPLE)
        # Read all of the nodes up to and including </html>:
        for evt, node in items:
            if evt == pulldom.END_ELEMENT and node.tagName == "html":
                break
        try:
            # Assert that the next node is END_DOCUMENT:
            evt, node = next(items)
            self.assertEqual(pulldom.END_DOCUMENT, evt)
        except StopIteration:
            self.fail(
                "Ran out of events, but should have received END_DOCUMENT")

    def test_external_ges_default(self):
        parser = pulldom.parseString(SMALL_SAMPLE)
        saxparser = parser.parser
        ges = saxparser.getFeature(feature_external_ges)
        self.assertEqual(ges, False)


class ThoroughTestCase(unittest.TestCase):
    """Test the hard-to-reach parts of pulldom."""

    def test_thorough_parse(self):
        """Test some of the hard-to-reach parts of PullDOM."""
        self._test_thorough(pulldom.parse(None, parser=SAXExerciser()))

    @unittest.expectedFailure
    def test_sax2dom_fail(self):
        """SAX2DOM can"t handle a PI before the root element."""
        pd = SAX2DOMTestHelper(None, SAXExerciser(), 12)
        self._test_thorough(pd)

    def test_thorough_sax2dom(self):
        """Test some of the hard-to-reach parts of SAX2DOM."""
        pd = SAX2DOMTestHelper(None, SAX2DOMExerciser(), 12)
        self._test_thorough(pd, False)

    def _test_thorough(self, pd, before_root=True):
        """Test some of the hard-to-reach parts of the parser, using a mock
        parser."""

        evt, node = next(pd)
        self.assertEqual(pulldom.START_DOCUMENT, evt)
        # Just check the node is a Document:
        self.assertTrue(hasattr(node, "createElement"))

        if before_root:
            evt, node = next(pd)
            self.assertEqual(pulldom.COMMENT, evt)
            self.assertEqual("a comment", node.data)
            evt, node = next(pd)
            self.assertEqual(pulldom.PROCESSING_INSTRUCTION, evt)
            self.assertEqual("target", node.target)
            self.assertEqual("data", node.data)

        evt, node = next(pd)
        self.assertEqual(pulldom.START_ELEMENT, evt)
        self.assertEqual("html", node.tagName)

        evt, node = next(pd)
        self.assertEqual(pulldom.COMMENT, evt)
        self.assertEqual("a comment", node.data)
        evt, node = next(pd)
        self.assertEqual(pulldom.PROCESSING_INSTRUCTION, evt)
        self.assertEqual("target", node.target)
        self.assertEqual("data", node.data)

        evt, node = next(pd)
        self.assertEqual(pulldom.START_ELEMENT, evt)
        self.assertEqual("p", node.tagName)

        evt, node = next(pd)
        self.assertEqual(pulldom.CHARACTERS, evt)
        self.assertEqual("text", node.data)
        evt, node = next(pd)
        self.assertEqual(pulldom.END_ELEMENT, evt)
        self.assertEqual("p", node.tagName)
        evt, node = next(pd)
        self.assertEqual(pulldom.END_ELEMENT, evt)
        self.assertEqual("html", node.tagName)
        evt, node = next(pd)
        self.assertEqual(pulldom.END_DOCUMENT, evt)


class SAXExerciser(object):
    """A fake sax parser that calls some of the harder-to-reach sax methods to
    ensure it emits the correct events"""

    def setContentHandler(self, handler):
        self._handler = handler

    def parse(self, _):
        h = self._handler
        h.startDocument()

        # The next two items ensure that items preceding the first
        # start_element are properly stored and emitted:
        h.comment("a comment")
        h.processingInstruction("target", "data")

        h.startElement("html", AttributesImpl({}))

        h.comment("a comment")
        h.processingInstruction("target", "data")

        h.startElement("p", AttributesImpl({"class": "paraclass"}))
        h.characters("text")
        h.endElement("p")
        h.endElement("html")
        h.endDocument()

    def stub(self, *args, **kwargs):
        """Stub method. Does nothing."""
        pass
    setProperty = stub
    setFeature = stub


class SAX2DOMExerciser(SAXExerciser):
    """The same as SAXExerciser, but without the processing instruction and
    comment before the root element, because S2D can"t handle it"""

    def parse(self, _):
        h = self._handler
        h.startDocument()
        h.startElement("html", AttributesImpl({}))
        h.comment("a comment")
        h.processingInstruction("target", "data")
        h.startElement("p", AttributesImpl({"class": "paraclass"}))
        h.characters("text")
        h.endElement("p")
        h.endElement("html")
        h.endDocument()


class SAX2DOMTestHelper(pulldom.DOMEventStream):
    """Allows us to drive SAX2DOM from a DOMEventStream."""

    def reset(self):
        self.pulldom = pulldom.SAX2DOM()
        # This content handler relies on namespace support
        self.parser.setFeature(xml.sax.handler.feature_namespaces, 1)
        self.parser.setContentHandler(self.pulldom)


class SAX2DOMTestCase(unittest.TestCase):

    def confirm(self, test, testname="Test"):
        self.assertTrue(test, testname)

    def test_basic(self):
        """Ensure SAX2DOM can parse from a stream."""
        with io.StringIO(SMALL_SAMPLE) as fin:
            sd = SAX2DOMTestHelper(fin, xml.sax.make_parser(),
                                   len(SMALL_SAMPLE))
            for evt, node in sd:
                if evt == pulldom.START_ELEMENT and node.tagName == "html":
                    break
            # Because the buffer is the same length as the XML, all the
            # nodes should have been parsed and added:
            self.assertGreater(len(node.childNodes), 0)

    def testSAX2DOM(self):
        """Ensure SAX2DOM expands nodes as expected."""
        sax2dom = pulldom.SAX2DOM()
        sax2dom.startDocument()
        sax2dom.startElement("doc", {})
        sax2dom.characters("text")
        sax2dom.startElement("subelm", {})
        sax2dom.characters("text")
        sax2dom.endElement("subelm")
        sax2dom.characters("text")
        sax2dom.endElement("doc")
        sax2dom.endDocument()

        doc = sax2dom.document
        root = doc.documentElement
        (text1, elm1, text2) = root.childNodes
        text3 = elm1.childNodes[0]

        self.assertIsNone(text1.previousSibling)
        self.assertIs(text1.nextSibling, elm1)
        self.assertIs(elm1.previousSibling, text1)
        self.assertIs(elm1.nextSibling, text2)
        self.assertIs(text2.previousSibling, elm1)
        self.assertIsNone(text2.nextSibling)
        self.assertIsNone(text3.previousSibling)
        self.assertIsNone(text3.nextSibling)

        self.assertIs(root.parentNode, doc)
        self.assertIs(text1.parentNode, root)
        self.assertIs(elm1.parentNode, root)
        self.assertIs(text2.parentNode, root)
        self.assertIs(text3.parentNode, elm1)
        doc.unlink()


if __name__ == "__main__":
    unittest.main()
