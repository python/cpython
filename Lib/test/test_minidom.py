# test for xml.dom.minidom

import copy
import pickle
import io
from test import support
import unittest

import xml.dom.minidom

from xml.dom.minidom import parse, Attr, Node, Document, parseString
from xml.dom.minidom import getDOMImplementation
from xml.parsers.expat import ExpatError


tstfile = support.findfile("test.xml", subdir="xmltestdata")
sample = ("<?xml version='1.0' encoding='us-ascii'?>\n"
          "<!DOCTYPE doc PUBLIC 'http://xml.python.org/public'"
          " 'http://xml.python.org/system' [\n"
          "  <!ELEMENT e EMPTY>\n"
          "  <!ENTITY ent SYSTEM 'http://xml.python.org/entity'>\n"
          "]><doc attr='value'> text\n"
          "<?pi sample?> <!-- comment --> <e/> </doc>")

# The tests of DocumentType importing use these helpers to construct
# the documents to work with, since not all DOM builders actually
# create the DocumentType nodes.
def create_doc_without_doctype(doctype=None):
    return getDOMImplementation().createDocument(None, "doc", doctype)

def create_nonempty_doctype():
    doctype = getDOMImplementation().createDocumentType("doc", None, None)
    doctype.entities._seq = []
    doctype.notations._seq = []
    notation = xml.dom.minidom.Notation("my-notation", None,
                                        "http://xml.python.org/notations/my")
    doctype.notations._seq.append(notation)
    entity = xml.dom.minidom.Entity("my-entity", None,
                                    "http://xml.python.org/entities/my",
                                    "my-notation")
    entity.version = "1.0"
    entity.encoding = "utf-8"
    entity.actualEncoding = "us-ascii"
    doctype.entities._seq.append(entity)
    return doctype

def create_doc_with_doctype():
    doctype = create_nonempty_doctype()
    doc = create_doc_without_doctype(doctype)
    doctype.entities.item(0).ownerDocument = doc
    doctype.notations.item(0).ownerDocument = doc
    return doc

class MinidomTest(unittest.TestCase):
    def confirm(self, test, testname = "Test"):
        self.assertTrue(test, testname)

    def checkWholeText(self, node, s):
        t = node.wholeText
        self.assertEqual(t, s, "looking for %r, found %r" % (s, t))

    def testDocumentAsyncAttr(self):
        doc = Document()
        self.assertFalse(doc.async_)
        self.assertFalse(Document.async_)

    def testParseFromBinaryFile(self):
        with open(tstfile, 'rb') as file:
            dom = parse(file)
            dom.unlink()
            self.assertIsInstance(dom, Document)

    def testParseFromTextFile(self):
        with open(tstfile, 'r', encoding='iso-8859-1') as file:
            dom = parse(file)
            dom.unlink()
            self.assertIsInstance(dom, Document)

    def testAttrModeSetsParamsAsAttrs(self):
        attr = Attr("qName", "namespaceURI", "localName", "prefix")
        self.assertEqual(attr.name, "qName")
        self.assertEqual(attr.namespaceURI, "namespaceURI")
        self.assertEqual(attr.prefix, "prefix")
        self.assertEqual(attr.localName, "localName")

    def testAttrModeSetsNonOptionalAttrs(self):
        attr = Attr("qName", "namespaceURI", None, "prefix")
        self.assertEqual(attr.name, "qName")
        self.assertEqual(attr.namespaceURI, "namespaceURI")
        self.assertEqual(attr.prefix, "prefix")
        self.assertEqual(attr.localName, attr.name)

    def testGetElementsByTagName(self):
        dom = parse(tstfile)
        self.assertEqual(dom.getElementsByTagName("LI"),
                dom.documentElement.getElementsByTagName("LI"))
        dom.unlink()

    def testInsertBefore(self):
        dom = parseString("<doc><foo/></doc>")
        root = dom.documentElement
        elem = root.childNodes[0]
        nelem = dom.createElement("element")
        root.insertBefore(nelem, elem)
        self.assertEqual(len(root.childNodes), 2)
        self.assertEqual(root.childNodes.length, 2)
        self.assertIs(root.childNodes[0], nelem)
        self.assertIs(root.childNodes.item(0), nelem)
        self.assertIs(root.childNodes[1], elem)
        self.assertIs(root.childNodes.item(1), elem)
        self.assertIs(root.firstChild, nelem)
        self.assertIs(root.lastChild, elem)
        self.assertEqual(root.toxml(), "<doc><element/><foo/></doc>")
        nelem = dom.createElement("element")
        root.insertBefore(nelem, None)
        self.assertEqual(len(root.childNodes), 3)
        self.assertEqual(root.childNodes.length, 3)
        self.assertIs(root.childNodes[1], elem)
        self.assertIs(root.childNodes.item(1), elem)
        self.assertIs(root.childNodes[2], nelem)
        self.assertIs(root.childNodes.item(2), nelem)
        self.assertIs(root.lastChild, nelem)
        self.assertIs(nelem.previousSibling, elem)
        self.assertEqual(root.toxml(), "<doc><element/><foo/><element/></doc>")
        nelem2 = dom.createElement("bar")
        root.insertBefore(nelem2, nelem)
        self.assertEqual(len(root.childNodes), 4)
        self.assertEqual(root.childNodes.length, 4)
        self.assertIs(root.childNodes[2], nelem2)
        self.assertIs(root.childNodes.item(2), nelem2)
        self.assertIs(root.childNodes[3], nelem)
        self.assertIs(root.childNodes.item(3), nelem)
        self.assertIs(nelem2.nextSibling, nelem)
        self.assertIs(nelem.previousSibling, nelem2)
        self.assertEqual(root.toxml(),
                         "<doc><element/><foo/><bar/><element/></doc>")
        dom.unlink()

    def _create_fragment_test_nodes(self):
        dom = parseString("<doc/>")
        orig = dom.createTextNode("original")
        c1 = dom.createTextNode("foo")
        c2 = dom.createTextNode("bar")
        c3 = dom.createTextNode("bat")
        dom.documentElement.appendChild(orig)
        frag = dom.createDocumentFragment()
        frag.appendChild(c1)
        frag.appendChild(c2)
        frag.appendChild(c3)
        return dom, orig, c1, c2, c3, frag

    def testInsertBeforeFragment(self):
        dom, orig, c1, c2, c3, frag = self._create_fragment_test_nodes()
        dom.documentElement.insertBefore(frag, None)
        self.assertTupleEqual(tuple(dom.documentElement.childNodes),
                     (orig, c1, c2, c3),
                     "insertBefore(<fragment>, None)")
        frag.unlink()
        dom.unlink()

        dom, orig, c1, c2, c3, frag = self._create_fragment_test_nodes()
        dom.documentElement.insertBefore(frag, orig)
        self.assertTupleEqual(tuple(dom.documentElement.childNodes),
                     (c1, c2, c3, orig),
                     "insertBefore(<fragment>, orig)")
        frag.unlink()
        dom.unlink()

    def testAppendChild(self):
        dom = parse(tstfile)
        dom.documentElement.appendChild(dom.createComment("Hello"))
        self.assertEqual(dom.documentElement.childNodes[-1].nodeName, "#comment")
        self.assertEqual(dom.documentElement.childNodes[-1].data, "Hello")
        dom.unlink()

    def testAppendChildFragment(self):
        dom, orig, c1, c2, c3, frag = self._create_fragment_test_nodes()
        dom.documentElement.appendChild(frag)
        self.assertTupleEqual(tuple(dom.documentElement.childNodes),
                     (orig, c1, c2, c3),
                     "appendChild(<fragment>)")
        frag.unlink()
        dom.unlink()

    def testReplaceChildFragment(self):
        dom, orig, c1, c2, c3, frag = self._create_fragment_test_nodes()
        dom.documentElement.replaceChild(frag, orig)
        orig.unlink()
        self.assertTupleEqual(tuple(dom.documentElement.childNodes), (c1, c2, c3),
                "replaceChild(<fragment>)")
        frag.unlink()
        dom.unlink()

    def testLegalChildren(self):
        dom = Document()
        elem = dom.createElement('element')
        text = dom.createTextNode('text')
        self.assertRaises(xml.dom.HierarchyRequestErr, dom.appendChild, text)

        dom.appendChild(elem)
        self.assertRaises(xml.dom.HierarchyRequestErr, dom.insertBefore, text,
                          elem)
        self.assertRaises(xml.dom.HierarchyRequestErr, dom.replaceChild, text,
                          elem)

        nodemap = elem.attributes
        self.assertRaises(xml.dom.HierarchyRequestErr, nodemap.setNamedItem,
                          text)
        self.assertRaises(xml.dom.HierarchyRequestErr, nodemap.setNamedItemNS,
                          text)

        elem.appendChild(text)
        dom.unlink()

    def testNamedNodeMapSetItem(self):
        dom = Document()
        elem = dom.createElement('element')
        attrs = elem.attributes
        attrs["foo"] = "bar"
        a = attrs.item(0)
        self.assertIs(a.ownerDocument, dom,
                "NamedNodeMap.__setitem__() sets ownerDocument")
        self.assertIs(a.ownerElement, elem,
                "NamedNodeMap.__setitem__() sets ownerElement")
        self.assertEqual(a.value, "bar",
                "NamedNodeMap.__setitem__() sets value")
        self.assertEqual(a.nodeValue, "bar",
                "NamedNodeMap.__setitem__() sets nodeValue")
        elem.unlink()
        dom.unlink()

    def testNonZero(self):
        dom = parse(tstfile)
        self.assertTrue(dom)  # should not be zero
        dom.appendChild(dom.createComment("foo"))
        self.assertFalse(dom.childNodes[-1].childNodes)
        dom.unlink()

    def testUnlink(self):
        dom = parse(tstfile)
        self.assertTrue(dom.childNodes)
        dom.unlink()
        self.assertFalse(dom.childNodes)

    def testContext(self):
        with parse(tstfile) as dom:
            self.assertTrue(dom.childNodes)
        self.assertFalse(dom.childNodes)

    def testElement(self):
        dom = Document()
        dom.appendChild(dom.createElement("abc"))
        self.assertTrue(dom.documentElement)
        dom.unlink()

    def testAAA(self):
        dom = parseString("<abc/>")
        el = dom.documentElement
        el.setAttribute("spam", "jam2")
        self.assertEqual(el.toxml(), '<abc spam="jam2"/>', "testAAA")
        a = el.getAttributeNode("spam")
        self.assertIs(a.ownerDocument, dom,
                "setAttribute() sets ownerDocument")
        self.assertIs(a.ownerElement, dom.documentElement,
                "setAttribute() sets ownerElement")
        dom.unlink()

    def testAAB(self):
        dom = parseString("<abc/>")
        el = dom.documentElement
        el.setAttribute("spam", "jam")
        el.setAttribute("spam", "jam2")
        self.assertEqual(el.toxml(), '<abc spam="jam2"/>', "testAAB")
        dom.unlink()

    def testAddAttr(self):
        dom = Document()
        child = dom.appendChild(dom.createElement("abc"))

        child.setAttribute("def", "ghi")
        self.assertEqual(child.getAttribute("def"), "ghi")
        self.assertEqual(child.attributes["def"].value, "ghi")

        child.setAttribute("jkl", "mno")
        self.assertEqual(child.getAttribute("jkl"), "mno")
        self.assertEqual(child.attributes["jkl"].value, "mno")

        self.assertEqual(len(child.attributes), 2)

        child.setAttribute("def", "newval")
        self.assertEqual(child.getAttribute("def"), "newval")
        self.assertEqual(child.attributes["def"].value, "newval")

        self.assertEqual(len(child.attributes), 2)
        dom.unlink()

    def testDeleteAttr(self):
        dom = Document()
        child = dom.appendChild(dom.createElement("abc"))

        self.assertEqual(len(child.attributes), 0)
        child.setAttribute("def", "ghi")
        self.assertEqual(len(child.attributes), 1)
        del child.attributes["def"]
        self.assertEqual(len(child.attributes), 0)
        dom.unlink()

    def testRemoveAttr(self):
        dom = Document()
        child = dom.appendChild(dom.createElement("abc"))

        child.setAttribute("def", "ghi")
        self.assertEqual(len(child.attributes), 1)
        self.assertRaises(xml.dom.NotFoundErr, child.removeAttribute, "foo")
        child.removeAttribute("def")
        self.assertEqual(len(child.attributes), 0)
        dom.unlink()

    def testRemoveAttrNS(self):
        dom = Document()
        child = dom.appendChild(
                dom.createElementNS("http://www.python.org", "python:abc"))
        child.setAttributeNS("http://www.w3.org", "xmlns:python",
                                                "http://www.python.org")
        child.setAttributeNS("http://www.python.org", "python:abcattr", "foo")
        self.assertRaises(xml.dom.NotFoundErr, child.removeAttributeNS,
            "foo", "http://www.python.org")
        self.assertEqual(len(child.attributes), 2)
        child.removeAttributeNS("http://www.python.org", "abcattr")
        self.assertEqual(len(child.attributes), 1)
        dom.unlink()

    def testRemoveAttributeNode(self):
        dom = Document()
        child = dom.appendChild(dom.createElement("foo"))
        child.setAttribute("spam", "jam")
        self.assertEqual(len(child.attributes), 1)
        node = child.getAttributeNode("spam")
        self.assertRaises(xml.dom.NotFoundErr, child.removeAttributeNode,
            None)
        self.assertIs(node, child.removeAttributeNode(node))
        self.assertEqual(len(child.attributes), 0)
        self.assertIsNone(child.getAttributeNode("spam"))
        dom2 = Document()
        child2 = dom2.appendChild(dom2.createElement("foo"))
        node2 = child2.getAttributeNode("spam")
        self.assertRaises(xml.dom.NotFoundErr, child2.removeAttributeNode,
            node2)
        dom.unlink()

    def testHasAttribute(self):
        dom = Document()
        child = dom.appendChild(dom.createElement("foo"))
        child.setAttribute("spam", "jam")
        self.assertTrue(child.hasAttribute("spam"))

    def testChangeAttr(self):
        dom = parseString("<abc/>")
        el = dom.documentElement
        el.setAttribute("spam", "jam")
        self.assertEqual(len(el.attributes), 1)
        el.setAttribute("spam", "bam")
        # Set this attribute to be an ID and make sure that doesn't change
        # when changing the value:
        el.setIdAttribute("spam")
        self.assertEqual(len(el.attributes), 1)
        self.assertEqual(el.attributes["spam"].value, "bam")
        self.assertEqual(el.attributes["spam"].nodeValue, "bam")
        self.assertEqual(el.getAttribute("spam"), "bam")
        self.assertTrue(el.getAttributeNode("spam").isId)
        el.attributes["spam"] = "ham"
        self.assertEqual(len(el.attributes), 1)
        self.assertEqual(el.attributes["spam"].value, "ham")
        self.assertEqual(el.attributes["spam"].nodeValue, "ham")
        self.assertEqual(el.getAttribute("spam"), "ham")
        self.assertTrue(el.attributes["spam"].isId)
        el.setAttribute("spam2", "bam")
        self.assertEqual(len(el.attributes), 2)
        self.assertEqual(el.attributes["spam"].value, "ham")
        self.assertEqual(el.attributes["spam"].nodeValue, "ham")
        self.assertEqual(el.getAttribute("spam"), "ham")
        self.assertEqual(el.attributes["spam2"].value, "bam")
        self.assertEqual(el.attributes["spam2"].nodeValue, "bam")
        self.assertEqual(el.getAttribute("spam2"), "bam")
        el.attributes["spam2"] = "bam2"

        self.assertEqual(len(el.attributes), 2)
        self.assertEqual(el.attributes["spam"].value, "ham")
        self.assertEqual(el.attributes["spam"].nodeValue, "ham")
        self.assertEqual(el.getAttribute("spam"), "ham")
        self.assertEqual(el.attributes["spam2"].value, "bam2")
        self.assertEqual(el.attributes["spam2"].nodeValue, "bam2")
        self.assertEqual(el.getAttribute("spam2"), "bam2")
        dom.unlink()

    def testGetAttrList(self):
        dom = parseString("<abc/>")
        self.addCleanup(dom.unlink)
        el = dom.documentElement
        el.setAttribute("spam", "jam")
        self.assertEqual(len(el.attributes.items()), 1)
        el.setAttribute("foo", "bar")
        items = el.attributes.items()
        self.assertEqual(len(items), 2)
        self.assertIn(('spam', 'jam'), items)
        self.assertIn(('foo', 'bar'), items)

    def testGetAttrValues(self):
        dom = parseString("<abc/>")
        self.addCleanup(dom.unlink)
        el = dom.documentElement
        el.setAttribute("spam", "jam")
        values = [x.value for x in el.attributes.values()]
        self.assertIn("jam", values)
        el.setAttribute("foo", "bar")
        values = [x.value for x in el.attributes.values()]
        self.assertIn("bar", values)
        self.assertIn("jam", values)

    def testGetAttribute(self):
        dom = Document()
        child = dom.appendChild(
            dom.createElementNS("http://www.python.org", "python:abc"))
        self.assertEqual(child.getAttribute('missing'), '')

    def testGetAttributeNS(self):
        dom = Document()
        child = dom.appendChild(
                dom.createElementNS("http://www.python.org", "python:abc"))
        child.setAttributeNS("http://www.w3.org", "xmlns:python",
                                                "http://www.python.org")
        self.assertEqual(child.getAttributeNS("http://www.w3.org", "python"),
            'http://www.python.org')
        self.assertEqual(child.getAttributeNS("http://www.w3.org", "other"),
            '')
        child2 = child.appendChild(dom.createElement('abc'))
        self.assertEqual(child2.getAttributeNS("http://www.python.org", "missing"),
                         '')

    def testGetAttributeNode(self): pass

    def testGetElementsByTagNameNS(self):
        d="""<foo xmlns:minidom='http://pyxml.sf.net/minidom'>
        <minidom:myelem/>
        </foo>"""
        dom = parseString(d)
        elems = dom.getElementsByTagNameNS("http://pyxml.sf.net/minidom",
                                           "myelem")
        self.assertEqual(len(elems), 1)
        self.assertEqual(elems[0].namespaceURI, "http://pyxml.sf.net/minidom")
        self.assertEqual(elems[0].localName, "myelem")
        self.assertEqual(elems[0].prefix, "minidom")
        self.assertEqual(elems[0].tagName, "minidom:myelem")
        self.assertEqual(elems[0].nodeName, "minidom:myelem")
        dom.unlink()

    def get_empty_nodelist_from_elements_by_tagName_ns_helper(self, doc, nsuri,
                                                              lname):
        nodelist = doc.getElementsByTagNameNS(nsuri, lname)
        self.assertEqual(len(nodelist), 0)

    def testGetEmptyNodeListFromElementsByTagNameNS(self):
        doc = parseString('<doc/>')
        self.get_empty_nodelist_from_elements_by_tagName_ns_helper(
            doc, 'http://xml.python.org/namespaces/a', 'localname')
        self.get_empty_nodelist_from_elements_by_tagName_ns_helper(
            doc, '*', 'splat')
        self.get_empty_nodelist_from_elements_by_tagName_ns_helper(
            doc, 'http://xml.python.org/namespaces/a', '*')

        doc = parseString('<doc xmlns="http://xml.python.org/splat"><e/></doc>')
        self.get_empty_nodelist_from_elements_by_tagName_ns_helper(
            doc, "http://xml.python.org/splat", "not-there")
        self.get_empty_nodelist_from_elements_by_tagName_ns_helper(
            doc, "*", "not-there")
        self.get_empty_nodelist_from_elements_by_tagName_ns_helper(
            doc, "http://somewhere.else.net/not-there", "e")

    def testElementReprAndStr(self):
        dom = Document()
        el = dom.appendChild(dom.createElement("abc"))
        string1 = repr(el)
        string2 = str(el)
        self.assertEqual(string1, string2)
        dom.unlink()

    def testElementReprAndStrUnicode(self):
        dom = Document()
        el = dom.appendChild(dom.createElement("abc"))
        string1 = repr(el)
        string2 = str(el)
        self.assertEqual(string1, string2)
        dom.unlink()

    def testElementReprAndStrUnicodeNS(self):
        dom = Document()
        el = dom.appendChild(
            dom.createElementNS("http://www.slashdot.org", "slash:abc"))
        string1 = repr(el)
        string2 = str(el)
        self.assertEqual(string1, string2)
        self.assertIn("slash:abc", string1)
        dom.unlink()

    def testAttributeRepr(self):
        dom = Document()
        el = dom.appendChild(dom.createElement("abc"))
        node = el.setAttribute("abc", "def")
        self.assertEqual(str(node), repr(node))
        dom.unlink()

    def testWriteXML(self):
        str = '<?xml version="1.0" ?><a b="c"/>'
        dom = parseString(str)
        domstr = dom.toxml()
        dom.unlink()
        self.assertEqual(str, domstr)

    def test_toxml_quote_text(self):
        dom = Document()
        elem = dom.appendChild(dom.createElement('elem'))
        elem.appendChild(dom.createTextNode('&<>"'))
        cr = elem.appendChild(dom.createElement('cr'))
        cr.appendChild(dom.createTextNode('\r'))
        crlf = elem.appendChild(dom.createElement('crlf'))
        crlf.appendChild(dom.createTextNode('\r\n'))
        lflf = elem.appendChild(dom.createElement('lflf'))
        lflf.appendChild(dom.createTextNode('\n\n'))
        ws = elem.appendChild(dom.createElement('ws'))
        ws.appendChild(dom.createTextNode('\t\n\r '))
        domstr = dom.toxml()
        dom.unlink()
        self.assertEqual(domstr, '<?xml version="1.0" ?>'
                '<elem>&amp;&lt;&gt;"'
                '<cr>\r</cr>'
                '<crlf>\r\n</crlf>'
                '<lflf>\n\n</lflf>'
                '<ws>\t\n\r </ws></elem>')

    def test_toxml_quote_attrib(self):
        dom = Document()
        elem = dom.appendChild(dom.createElement('elem'))
        elem.setAttribute("a", '&<>"')
        elem.setAttribute("cr", "\r")
        elem.setAttribute("lf", "\n")
        elem.setAttribute("crlf", "\r\n")
        elem.setAttribute("lflf", "\n\n")
        elem.setAttribute("ws", "\t\n\r ")
        domstr = dom.toxml()
        dom.unlink()
        self.assertEqual(domstr, '<?xml version="1.0" ?>'
                '<elem a="&amp;&lt;&gt;&quot;" '
                'cr="&#13;" '
                'lf="&#10;" '
                'crlf="&#13;&#10;" '
                'lflf="&#10;&#10;" '
                'ws="&#9;&#10;&#13; "/>')

    def testAltNewline(self):
        str = '<?xml version="1.0" ?>\n<a b="c"/>\n'
        dom = parseString(str)
        domstr = dom.toprettyxml(newl="\r\n")
        dom.unlink()
        self.assertEqual(domstr, str.replace("\n", "\r\n"))

    def test_toprettyxml_with_text_nodes(self):
        # see issue #4147, text nodes are not indented
        decl = '<?xml version="1.0" ?>\n'
        self.assertEqual(parseString('<B>A</B>').toprettyxml(),
                         decl + '<B>A</B>\n')
        self.assertEqual(parseString('<C>A<B>A</B></C>').toprettyxml(),
                         decl + '<C>\n\tA\n\t<B>A</B>\n</C>\n')
        self.assertEqual(parseString('<C><B>A</B>A</C>').toprettyxml(),
                         decl + '<C>\n\t<B>A</B>\n\tA\n</C>\n')
        self.assertEqual(parseString('<C><B>A</B><B>A</B></C>').toprettyxml(),
                         decl + '<C>\n\t<B>A</B>\n\t<B>A</B>\n</C>\n')
        self.assertEqual(parseString('<C><B>A</B>A<B>A</B></C>').toprettyxml(),
                         decl + '<C>\n\t<B>A</B>\n\tA\n\t<B>A</B>\n</C>\n')

    def test_toprettyxml_with_adjacent_text_nodes(self):
        # see issue #4147, adjacent text nodes are indented normally
        dom = Document()
        elem = dom.createElement('elem')
        elem.appendChild(dom.createTextNode('TEXT'))
        elem.appendChild(dom.createTextNode('TEXT'))
        dom.appendChild(elem)
        decl = '<?xml version="1.0" ?>\n'
        self.assertEqual(dom.toprettyxml(),
                         decl + '<elem>\n\tTEXT\n\tTEXT\n</elem>\n')

    def test_toprettyxml_preserves_content_of_text_node(self):
        # see issue #4147
        for str in ('<B>A</B>', '<A><B>C</B></A>'):
            dom = parseString(str)
            dom2 = parseString(dom.toprettyxml())
            self.assertEqual(
                dom.getElementsByTagName('B')[0].childNodes[0].toxml(),
                dom2.getElementsByTagName('B')[0].childNodes[0].toxml())

    def testProcessingInstruction(self):
        dom = parseString('<e><?mypi \t\n data \t\n ?></e>')
        pi = dom.documentElement.firstChild
        self.assertEqual(pi.target, "mypi")
        self.assertEqual(pi.data, "data \t\n ")
        self.assertEqual(pi.nodeName, "mypi")
        self.assertEqual(pi.nodeType, Node.PROCESSING_INSTRUCTION_NODE)
        self.assertIsNone(pi.attributes)
        self.assertFalse(pi.hasChildNodes())
        self.assertEqual(len(pi.childNodes), 0)
        self.assertIsNone(pi.firstChild)
        self.assertIsNone(pi.lastChild)
        self.assertIsNone(pi.localName)
        self.assertEqual(pi.namespaceURI, xml.dom.EMPTY_NAMESPACE)

    def testProcessingInstructionRepr(self):
        dom = parseString('<e><?mypi \t\n data \t\n ?></e>')
        pi = dom.documentElement.firstChild
        self.assertEqual(str(pi.nodeType), repr(pi.nodeType))

    def testTextRepr(self):
        dom = Document()
        self.addCleanup(dom.unlink)
        elem = dom.createElement("elem")
        elem.appendChild(dom.createTextNode("foo"))
        el = elem.firstChild
        self.assertEqual(str(el), repr(el))
        self.assertEqual('<DOM Text node "\'foo\'">', str(el))

    def testWriteText(self): pass

    def testDocumentElement(self): pass

    def testTooManyDocumentElements(self):
        doc = parseString("<doc/>")
        elem = doc.createElement("extra")
        # Should raise an exception when adding an extra document element.
        self.assertRaises(xml.dom.HierarchyRequestErr, doc.appendChild, elem)
        elem.unlink()
        doc.unlink()

    def testCreateElementNS(self): pass

    def testCreateAttributeNS(self): pass

    def testParse(self): pass

    def testParseString(self): pass

    def testComment(self): pass

    def testAttrListItem(self): pass

    def testAttrListItems(self): pass

    def testAttrListItemNS(self): pass

    def testAttrListKeys(self): pass

    def testAttrListKeysNS(self): pass

    def testRemoveNamedItem(self):
        doc = parseString("<doc a=''/>")
        e = doc.documentElement
        attrs = e.attributes
        a1 = e.getAttributeNode("a")
        a2 = attrs.removeNamedItem("a")
        self.assertTrue(a1.isSameNode(a2))
        self.assertRaises(xml.dom.NotFoundErr, attrs.removeNamedItem, "a")

    def testRemoveNamedItemNS(self):
        doc = parseString("<doc xmlns:a='http://xml.python.org/' a:b=''/>")
        e = doc.documentElement
        attrs = e.attributes
        a1 = e.getAttributeNodeNS("http://xml.python.org/", "b")
        a2 = attrs.removeNamedItemNS("http://xml.python.org/", "b")
        self.assertTrue(a1.isSameNode(a2))
        self.assertRaises(xml.dom.NotFoundErr, attrs.removeNamedItemNS,
                          "http://xml.python.org/", "b")

    def testAttrListValues(self): pass

    def testAttrListLength(self): pass

    def testAttrList__getitem__(self): pass

    def testAttrList__setitem__(self): pass

    def testSetAttrValueandNodeValue(self): pass

    def testParseElement(self): pass

    def testParseAttributes(self): pass

    def testParseElementNamespaces(self): pass

    def testParseAttributeNamespaces(self): pass

    def testParseProcessingInstructions(self): pass

    def testChildNodes(self): pass

    def testFirstChild(self): pass

    def testHasChildNodes(self):
        dom = parseString("<doc><foo/></doc>")
        doc = dom.documentElement
        self.assertTrue(doc.hasChildNodes())
        dom2 = parseString("<doc/>")
        doc2 = dom2.documentElement
        self.assertFalse(doc2.hasChildNodes())

    def _testCloneElementCopiesAttributes(self, e1, e2, test):
        attrs1 = e1.attributes
        attrs2 = e2.attributes
        keys1 = list(attrs1.keys())
        keys2 = list(attrs2.keys())
        keys1.sort()
        keys2.sort()
        self.assertEqual(keys1, keys2)
        for i in range(len(keys1)):
            a1 = attrs1.item(i)
            a2 = attrs2.item(i)
            self.assertIsNot(a1, a2)
            self.assertEqual(a1.value, a2.value)
            self.assertEqual(a1.nodeValue, a2.nodeValue)
            self.assertEqual(a1.namespaceURI,a2.namespaceURI)
            self.assertEqual(a1.localName, a2.localName)
            self.assertIs(a2.ownerElement, e2)

    def _setupCloneElement(self, deep):
        dom = parseString("<doc attr='value'><foo/></doc>")
        root = dom.documentElement
        clone = root.cloneNode(deep)
        self._testCloneElementCopiesAttributes(
            root, clone, "testCloneElement" + (deep and "Deep" or "Shallow"))
        # mutilate the original so shared data is detected
        root.tagName = root.nodeName = "MODIFIED"
        root.setAttribute("attr", "NEW VALUE")
        root.setAttribute("added", "VALUE")
        return dom, clone

    def testCloneElementShallow(self):
        dom, clone = self._setupCloneElement(0)
        self.assertEqual(len(clone.childNodes), 0)
        self.assertEqual(clone.childNodes.length, 0)
        self.assertIsNone(clone.parentNode)
        self.assertEqual(clone.toxml(), '<doc attr="value"/>')

        dom.unlink()

    def testCloneElementDeep(self):
        dom, clone = self._setupCloneElement(1)
        self.assertEqual(len(clone.childNodes), 1)
        self.assertEqual(clone.childNodes.length, 1)
        self.assertIsNone(clone.parentNode)
        self.assertTrue(clone.toxml(), '<doc attr="value"><foo/></doc>')
        dom.unlink()

    def testCloneDocumentShallow(self):
        doc = parseString("<?xml version='1.0'?>\n"
                    "<!-- comment -->"
                    "<!DOCTYPE doc [\n"
                    "<!NOTATION notation SYSTEM 'http://xml.python.org/'>\n"
                    "]>\n"
                    "<doc attr='value'/>")
        doc2 = doc.cloneNode(0)
        self.assertIsNone(doc2,
                "testCloneDocumentShallow:"
                " shallow cloning of documents makes no sense!")

    def testCloneDocumentDeep(self):
        doc = parseString("<?xml version='1.0'?>\n"
                    "<!-- comment -->"
                    "<!DOCTYPE doc [\n"
                    "<!NOTATION notation SYSTEM 'http://xml.python.org/'>\n"
                    "]>\n"
                    "<doc attr='value'/>")
        doc2 = doc.cloneNode(1)
        self.assertFalse((doc.isSameNode(doc2) or doc2.isSameNode(doc)),
                "testCloneDocumentDeep: document objects not distinct")
        self.assertEqual(len(doc.childNodes), len(doc2.childNodes),
                "testCloneDocumentDeep: wrong number of Document children")
        self.assertEqual(doc2.documentElement.nodeType, Node.ELEMENT_NODE,
                "testCloneDocumentDeep: documentElement not an ELEMENT_NODE")
        self.assertTrue(doc2.documentElement.ownerDocument.isSameNode(doc2),
            "testCloneDocumentDeep: documentElement owner is not new document")
        self.assertFalse(doc.documentElement.isSameNode(doc2.documentElement),
                "testCloneDocumentDeep: documentElement should not be shared")
        if doc.doctype is not None:
            # check the doctype iff the original DOM maintained it
            self.assertEqual(doc2.doctype.nodeType, Node.DOCUMENT_TYPE_NODE,
                    "testCloneDocumentDeep: doctype not a DOCUMENT_TYPE_NODE")
            self.assertTrue(doc2.doctype.ownerDocument.isSameNode(doc2))
            self.assertFalse(doc.doctype.isSameNode(doc2.doctype))

    def testCloneDocumentTypeDeepOk(self):
        doctype = create_nonempty_doctype()
        clone = doctype.cloneNode(1)
        self.confirm(clone is not None
                and clone.nodeName == doctype.nodeName
                and clone.name == doctype.name
                and clone.publicId == doctype.publicId
                and clone.systemId == doctype.systemId
                and len(clone.entities) == len(doctype.entities)
                and clone.entities.item(len(clone.entities)) is None
                and len(clone.notations) == len(doctype.notations)
                and clone.notations.item(len(clone.notations)) is None
                and len(clone.childNodes) == 0)
        for i in range(len(doctype.entities)):
            se = doctype.entities.item(i)
            ce = clone.entities.item(i)
            self.confirm((not se.isSameNode(ce))
                    and (not ce.isSameNode(se))
                    and ce.nodeName == se.nodeName
                    and ce.notationName == se.notationName
                    and ce.publicId == se.publicId
                    and ce.systemId == se.systemId
                    and ce.encoding == se.encoding
                    and ce.actualEncoding == se.actualEncoding
                    and ce.version == se.version)
        for i in range(len(doctype.notations)):
            sn = doctype.notations.item(i)
            cn = clone.notations.item(i)
            self.confirm((not sn.isSameNode(cn))
                    and (not cn.isSameNode(sn))
                    and cn.nodeName == sn.nodeName
                    and cn.publicId == sn.publicId
                    and cn.systemId == sn.systemId)

    def testCloneDocumentTypeDeepNotOk(self):
        doc = create_doc_with_doctype()
        clone = doc.doctype.cloneNode(1)
        self.assertIsNone(clone)

    def testCloneDocumentTypeShallowOk(self):
        doctype = create_nonempty_doctype()
        clone = doctype.cloneNode(0)
        self.confirm(clone is not None
                and clone.nodeName == doctype.nodeName
                and clone.name == doctype.name
                and clone.publicId == doctype.publicId
                and clone.systemId == doctype.systemId
                and len(clone.entities) == 0
                and clone.entities.item(0) is None
                and len(clone.notations) == 0
                and clone.notations.item(0) is None
                and len(clone.childNodes) == 0)

    def testCloneDocumentTypeShallowNotOk(self):
        doc = create_doc_with_doctype()
        clone = doc.doctype.cloneNode(0)
        self.assertIsNone(clone)

    def check_import_document(self, deep, testName):
        doc1 = parseString("<doc/>")
        doc2 = parseString("<doc/>")
        self.assertRaises(xml.dom.NotSupportedErr, doc1.importNode, doc2, deep)

    def testImportDocumentShallow(self):
        self.check_import_document(0, "testImportDocumentShallow")

    def testImportDocumentDeep(self):
        self.check_import_document(1, "testImportDocumentDeep")

    def testImportDocumentTypeShallow(self):
        src = create_doc_with_doctype()
        target = create_doc_without_doctype()
        self.assertRaises(xml.dom.NotSupportedErr, target.importNode,
                          src.doctype, 0)

    def testImportDocumentTypeDeep(self):
        src = create_doc_with_doctype()
        target = create_doc_without_doctype()
        self.assertRaises(xml.dom.NotSupportedErr, target.importNode,
                          src.doctype, 1)

    # Testing attribute clones uses a helper, and should always be deep,
    # even if the argument to cloneNode is false.
    def check_clone_attribute(self, deep, testName):
        doc = parseString("<doc attr='value'/>")
        attr = doc.documentElement.getAttributeNode("attr")
        self.assertIsNotNone(attr)
        clone = attr.cloneNode(deep)
        self.assertFalse(clone.isSameNode(attr))
        self.assertFalse(attr.isSameNode(clone))
        self.assertIsNone(clone.ownerElement,
                testName + ": ownerElement should be None")
        self.confirm(clone.ownerDocument.isSameNode(attr.ownerDocument),
                testName + ": ownerDocument does not match")
        self.confirm(clone.specified,
                testName + ": cloned attribute must have specified == True")

    def testCloneAttributeShallow(self):
        self.check_clone_attribute(0, "testCloneAttributeShallow")

    def testCloneAttributeDeep(self):
        self.check_clone_attribute(1, "testCloneAttributeDeep")

    def check_clone_pi(self, deep, testName):
        doc = parseString("<?target data?><doc/>")
        pi = doc.firstChild
        self.assertEqual(pi.nodeType, Node.PROCESSING_INSTRUCTION_NODE)
        clone = pi.cloneNode(deep)
        self.confirm(clone.target == pi.target
                and clone.data == pi.data)

    def testClonePIShallow(self):
        self.check_clone_pi(0, "testClonePIShallow")

    def testClonePIDeep(self):
        self.check_clone_pi(1, "testClonePIDeep")

    def check_clone_node_entity(self, clone_document):
        # bpo-35052: Test user data handler in cloneNode() on a document with
        # an entity
        document = xml.dom.minidom.parseString("""
            <?xml version="1.0" ?>
            <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
                "http://www.w3.org/TR/html4/strict.dtd"
                [ <!ENTITY smile "☺"> ]
            >
            <doc>Don't let entities make you frown &smile;</doc>
        """.strip())

        class Handler:
            def handle(self, operation, key, data, src, dst):
                self.operation = operation
                self.key = key
                self.data = data
                self.src = src
                self.dst = dst

        handler = Handler()
        doctype = document.doctype
        entity = doctype.entities['smile']
        entity.setUserData("key", "data", handler)

        if clone_document:
            # clone Document
            clone = document.cloneNode(deep=True)

            self.assertEqual(clone.documentElement.firstChild.wholeText,
                             "Don't let entities make you frown ☺")
            operation = xml.dom.UserDataHandler.NODE_IMPORTED
            dst = clone.doctype.entities['smile']
        else:
            # clone DocumentType
            with support.swap_attr(doctype, 'ownerDocument', None):
                clone = doctype.cloneNode(deep=True)

            operation = xml.dom.UserDataHandler.NODE_CLONED
            dst = clone.entities['smile']

        self.assertEqual(handler.operation, operation)
        self.assertEqual(handler.key, "key")
        self.assertEqual(handler.data, "data")
        self.assertIs(handler.src, entity)
        self.assertIs(handler.dst, dst)

    def testCloneNodeEntity(self):
        self.check_clone_node_entity(False)
        self.check_clone_node_entity(True)

    def testNormalize(self):
        doc = parseString("<doc/>")
        root = doc.documentElement
        root.appendChild(doc.createTextNode("first"))
        root.appendChild(doc.createTextNode("second"))
        self.confirm(len(root.childNodes) == 2
                and root.childNodes.length == 2,
                "testNormalize -- preparation")
        doc.normalize()
        self.confirm(len(root.childNodes) == 1
                and root.childNodes.length == 1
                and root.firstChild is root.lastChild
                and root.firstChild.data == "firstsecond"
                , "testNormalize -- result")
        doc.unlink()

        doc = parseString("<doc/>")
        root = doc.documentElement
        root.appendChild(doc.createTextNode(""))
        doc.normalize()
        self.confirm(len(root.childNodes) == 0
                and root.childNodes.length == 0,
                "testNormalize -- single empty node removed")
        doc.unlink()

    def testNormalizeCombineAndNextSibling(self):
        doc = parseString("<doc/>")
        root = doc.documentElement
        root.appendChild(doc.createTextNode("first"))
        root.appendChild(doc.createTextNode("second"))
        root.appendChild(doc.createElement("i"))
        self.confirm(len(root.childNodes) == 3
                and root.childNodes.length == 3,
                "testNormalizeCombineAndNextSibling -- preparation")
        doc.normalize()
        self.confirm(len(root.childNodes) == 2
                and root.childNodes.length == 2
                and root.firstChild.data == "firstsecond"
                and root.firstChild is not root.lastChild
                and root.firstChild.nextSibling is root.lastChild
                and root.firstChild.previousSibling is None
                and root.lastChild.previousSibling is root.firstChild
                and root.lastChild.nextSibling is None
                , "testNormalizeCombinedAndNextSibling -- result")
        doc.unlink()

    def testNormalizeDeleteWithPrevSibling(self):
        doc = parseString("<doc/>")
        root = doc.documentElement
        root.appendChild(doc.createTextNode("first"))
        root.appendChild(doc.createTextNode(""))
        self.confirm(len(root.childNodes) == 2
                and root.childNodes.length == 2,
                "testNormalizeDeleteWithPrevSibling -- preparation")
        doc.normalize()
        self.confirm(len(root.childNodes) == 1
                and root.childNodes.length == 1
                and root.firstChild.data == "first"
                and root.firstChild is root.lastChild
                and root.firstChild.nextSibling is None
                and root.firstChild.previousSibling is None
                , "testNormalizeDeleteWithPrevSibling -- result")
        doc.unlink()

    def testNormalizeDeleteWithNextSibling(self):
        doc = parseString("<doc/>")
        root = doc.documentElement
        root.appendChild(doc.createTextNode(""))
        root.appendChild(doc.createTextNode("second"))
        self.confirm(len(root.childNodes) == 2
                and root.childNodes.length == 2,
                "testNormalizeDeleteWithNextSibling -- preparation")
        doc.normalize()
        self.confirm(len(root.childNodes) == 1
                and root.childNodes.length == 1
                and root.firstChild.data == "second"
                and root.firstChild is root.lastChild
                and root.firstChild.nextSibling is None
                and root.firstChild.previousSibling is None
                , "testNormalizeDeleteWithNextSibling -- result")
        doc.unlink()

    def testNormalizeDeleteWithTwoNonTextSiblings(self):
        doc = parseString("<doc/>")
        root = doc.documentElement
        root.appendChild(doc.createElement("i"))
        root.appendChild(doc.createTextNode(""))
        root.appendChild(doc.createElement("i"))
        self.confirm(len(root.childNodes) == 3
                and root.childNodes.length == 3,
                "testNormalizeDeleteWithTwoSiblings -- preparation")
        doc.normalize()
        self.confirm(len(root.childNodes) == 2
                and root.childNodes.length == 2
                and root.firstChild is not root.lastChild
                and root.firstChild.nextSibling is root.lastChild
                and root.firstChild.previousSibling is None
                and root.lastChild.previousSibling is root.firstChild
                and root.lastChild.nextSibling is None
                , "testNormalizeDeleteWithTwoSiblings -- result")
        doc.unlink()

    def testNormalizeDeleteAndCombine(self):
        doc = parseString("<doc/>")
        root = doc.documentElement
        root.appendChild(doc.createTextNode(""))
        root.appendChild(doc.createTextNode("second"))
        root.appendChild(doc.createTextNode(""))
        root.appendChild(doc.createTextNode("fourth"))
        root.appendChild(doc.createTextNode(""))
        self.confirm(len(root.childNodes) == 5
                and root.childNodes.length == 5,
                "testNormalizeDeleteAndCombine -- preparation")
        doc.normalize()
        self.confirm(len(root.childNodes) == 1
                and root.childNodes.length == 1
                and root.firstChild is root.lastChild
                and root.firstChild.data == "secondfourth"
                and root.firstChild.previousSibling is None
                and root.firstChild.nextSibling is None
                , "testNormalizeDeleteAndCombine -- result")
        doc.unlink()

    def testNormalizeRecursion(self):
        doc = parseString("<doc>"
                            "<o>"
                              "<i/>"
                              "t"
                              #
                              #x
                            "</o>"
                            "<o>"
                              "<o>"
                                "t2"
                                #x2
                              "</o>"
                              "t3"
                              #x3
                            "</o>"
                            #
                          "</doc>")
        root = doc.documentElement
        root.childNodes[0].appendChild(doc.createTextNode(""))
        root.childNodes[0].appendChild(doc.createTextNode("x"))
        root.childNodes[1].childNodes[0].appendChild(doc.createTextNode("x2"))
        root.childNodes[1].appendChild(doc.createTextNode("x3"))
        root.appendChild(doc.createTextNode(""))
        self.confirm(len(root.childNodes) == 3
                and root.childNodes.length == 3
                and len(root.childNodes[0].childNodes) == 4
                and root.childNodes[0].childNodes.length == 4
                and len(root.childNodes[1].childNodes) == 3
                and root.childNodes[1].childNodes.length == 3
                and len(root.childNodes[1].childNodes[0].childNodes) == 2
                and root.childNodes[1].childNodes[0].childNodes.length == 2
                , "testNormalize2 -- preparation")
        doc.normalize()
        self.confirm(len(root.childNodes) == 2
                and root.childNodes.length == 2
                and len(root.childNodes[0].childNodes) == 2
                and root.childNodes[0].childNodes.length == 2
                and len(root.childNodes[1].childNodes) == 2
                and root.childNodes[1].childNodes.length == 2
                and len(root.childNodes[1].childNodes[0].childNodes) == 1
                and root.childNodes[1].childNodes[0].childNodes.length == 1
                , "testNormalize2 -- childNodes lengths")
        self.confirm(root.childNodes[0].childNodes[1].data == "tx"
                and root.childNodes[1].childNodes[0].childNodes[0].data == "t2x2"
                and root.childNodes[1].childNodes[1].data == "t3x3"
                , "testNormalize2 -- joined text fields")
        self.confirm(root.childNodes[0].childNodes[1].nextSibling is None
                and root.childNodes[0].childNodes[1].previousSibling
                        is root.childNodes[0].childNodes[0]
                and root.childNodes[0].childNodes[0].previousSibling is None
                and root.childNodes[0].childNodes[0].nextSibling
                        is root.childNodes[0].childNodes[1]
                and root.childNodes[1].childNodes[1].nextSibling is None
                and root.childNodes[1].childNodes[1].previousSibling
                        is root.childNodes[1].childNodes[0]
                and root.childNodes[1].childNodes[0].previousSibling is None
                and root.childNodes[1].childNodes[0].nextSibling
                        is root.childNodes[1].childNodes[1]
                , "testNormalize2 -- sibling pointers")
        doc.unlink()


    def testBug0777884(self):
        doc = parseString("<o>text</o>")
        text = doc.documentElement.childNodes[0]
        self.assertEqual(text.nodeType, Node.TEXT_NODE)
        # Should run quietly, doing nothing.
        text.normalize()
        doc.unlink()

    def testBug1433694(self):
        doc = parseString("<o><i/>t</o>")
        node = doc.documentElement
        node.childNodes[1].nodeValue = ""
        node.normalize()
        self.assertIsNone(node.childNodes[-1].nextSibling,
                     "Final child's .nextSibling should be None")

    def testSiblings(self):
        doc = parseString("<doc><?pi?>text?<elm/></doc>")
        root = doc.documentElement
        (pi, text, elm) = root.childNodes

        self.confirm(pi.nextSibling is text and
                pi.previousSibling is None and
                text.nextSibling is elm and
                text.previousSibling is pi and
                elm.nextSibling is None and
                elm.previousSibling is text, "testSiblings")

        doc.unlink()

    def testParents(self):
        doc = parseString(
            "<doc><elm1><elm2/><elm2><elm3/></elm2></elm1></doc>")
        root = doc.documentElement
        elm1 = root.childNodes[0]
        (elm2a, elm2b) = elm1.childNodes
        elm3 = elm2b.childNodes[0]

        self.confirm(root.parentNode is doc and
                elm1.parentNode is root and
                elm2a.parentNode is elm1 and
                elm2b.parentNode is elm1 and
                elm3.parentNode is elm2b, "testParents")
        doc.unlink()

    def testNodeListItem(self):
        doc = parseString("<doc><e/><e/></doc>")
        children = doc.childNodes
        docelem = children[0]
        self.confirm(children[0] is children.item(0)
                and children.item(1) is None
                and docelem.childNodes.item(0) is docelem.childNodes[0]
                and docelem.childNodes.item(1) is docelem.childNodes[1]
                and docelem.childNodes.item(0).childNodes.item(0) is None,
                "test NodeList.item()")
        doc.unlink()

    def testEncodings(self):
        doc = parseString('<foo>&#x20ac;</foo>')
        self.assertEqual(doc.toxml(),
                         '<?xml version="1.0" ?><foo>\u20ac</foo>')
        self.assertEqual(doc.toxml('utf-8'),
            b'<?xml version="1.0" encoding="utf-8"?><foo>\xe2\x82\xac</foo>')
        self.assertEqual(doc.toxml('iso-8859-15'),
            b'<?xml version="1.0" encoding="iso-8859-15"?><foo>\xa4</foo>')
        self.assertEqual(doc.toxml('us-ascii'),
            b'<?xml version="1.0" encoding="us-ascii"?><foo>&#8364;</foo>')
        self.assertEqual(doc.toxml('utf-16'),
            '<?xml version="1.0" encoding="utf-16"?>'
            '<foo>\u20ac</foo>'.encode('utf-16'))

        # Verify that character decoding errors raise exceptions instead
        # of crashing
        with self.assertRaises((UnicodeDecodeError, ExpatError)):
            parseString(
                b'<fran\xe7ais>Comment \xe7a va ? Tr\xe8s bien ?</fran\xe7ais>'
            )

        doc.unlink()

    def testStandalone(self):
        doc = parseString('<foo>&#x20ac;</foo>')
        self.assertEqual(doc.toxml(),
                         '<?xml version="1.0" ?><foo>\u20ac</foo>')
        self.assertEqual(doc.toxml(standalone=None),
                         '<?xml version="1.0" ?><foo>\u20ac</foo>')
        self.assertEqual(doc.toxml(standalone=True),
            '<?xml version="1.0" standalone="yes"?><foo>\u20ac</foo>')
        self.assertEqual(doc.toxml(standalone=False),
            '<?xml version="1.0" standalone="no"?><foo>\u20ac</foo>')
        self.assertEqual(doc.toxml('utf-8', True),
            b'<?xml version="1.0" encoding="utf-8" standalone="yes"?>'
            b'<foo>\xe2\x82\xac</foo>')

        doc.unlink()

    class UserDataHandler:
        called = 0
        def handle(self, operation, key, data, src, dst):
            dst.setUserData(key, data + 1, self)
            src.setUserData(key, None, None)
            self.called = 1

    def testUserData(self):
        dom = Document()
        n = dom.createElement('e')
        self.assertIsNone(n.getUserData("foo"))
        n.setUserData("foo", None, None)
        self.assertIsNone(n.getUserData("foo"))
        n.setUserData("foo", 12, 12)
        n.setUserData("bar", 13, 13)
        self.assertEqual(n.getUserData("foo"), 12)
        self.assertEqual(n.getUserData("bar"), 13)
        n.setUserData("foo", None, None)
        self.assertIsNone(n.getUserData("foo"))
        self.assertEqual(n.getUserData("bar"), 13)

        handler = self.UserDataHandler()
        n.setUserData("bar", 12, handler)
        c = n.cloneNode(1)
        self.confirm(handler.called
                and n.getUserData("bar") is None
                and c.getUserData("bar") == 13)
        n.unlink()
        c.unlink()
        dom.unlink()

    def checkRenameNodeSharedConstraints(self, doc, node):
        # Make sure illegal NS usage is detected:
        self.assertRaises(xml.dom.NamespaceErr, doc.renameNode, node,
                          "http://xml.python.org/ns", "xmlns:foo")
        doc2 = parseString("<doc/>")
        self.assertRaises(xml.dom.WrongDocumentErr, doc2.renameNode, node,
                          xml.dom.EMPTY_NAMESPACE, "foo")

    def testRenameAttribute(self):
        doc = parseString("<doc a='v'/>")
        elem = doc.documentElement
        attrmap = elem.attributes
        attr = elem.attributes['a']

        # Simple renaming
        attr = doc.renameNode(attr, xml.dom.EMPTY_NAMESPACE, "b")
        self.confirm(attr.name == "b"
                and attr.nodeName == "b"
                and attr.localName is None
                and attr.namespaceURI == xml.dom.EMPTY_NAMESPACE
                and attr.prefix is None
                and attr.value == "v"
                and elem.getAttributeNode("a") is None
                and elem.getAttributeNode("b").isSameNode(attr)
                and attrmap["b"].isSameNode(attr)
                and attr.ownerDocument.isSameNode(doc)
                and attr.ownerElement.isSameNode(elem))

        # Rename to have a namespace, no prefix
        attr = doc.renameNode(attr, "http://xml.python.org/ns", "c")
        self.confirm(attr.name == "c"
                and attr.nodeName == "c"
                and attr.localName == "c"
                and attr.namespaceURI == "http://xml.python.org/ns"
                and attr.prefix is None
                and attr.value == "v"
                and elem.getAttributeNode("a") is None
                and elem.getAttributeNode("b") is None
                and elem.getAttributeNode("c").isSameNode(attr)
                and elem.getAttributeNodeNS(
                    "http://xml.python.org/ns", "c").isSameNode(attr)
                and attrmap["c"].isSameNode(attr)
                and attrmap[("http://xml.python.org/ns", "c")].isSameNode(attr))

        # Rename to have a namespace, with prefix
        attr = doc.renameNode(attr, "http://xml.python.org/ns2", "p:d")
        self.confirm(attr.name == "p:d"
                and attr.nodeName == "p:d"
                and attr.localName == "d"
                and attr.namespaceURI == "http://xml.python.org/ns2"
                and attr.prefix == "p"
                and attr.value == "v"
                and elem.getAttributeNode("a") is None
                and elem.getAttributeNode("b") is None
                and elem.getAttributeNode("c") is None
                and elem.getAttributeNodeNS(
                    "http://xml.python.org/ns", "c") is None
                and elem.getAttributeNode("p:d").isSameNode(attr)
                and elem.getAttributeNodeNS(
                    "http://xml.python.org/ns2", "d").isSameNode(attr)
                and attrmap["p:d"].isSameNode(attr)
                and attrmap[("http://xml.python.org/ns2", "d")].isSameNode(attr))

        # Rename back to a simple non-NS node
        attr = doc.renameNode(attr, xml.dom.EMPTY_NAMESPACE, "e")
        self.confirm(attr.name == "e"
                and attr.nodeName == "e"
                and attr.localName is None
                and attr.namespaceURI == xml.dom.EMPTY_NAMESPACE
                and attr.prefix is None
                and attr.value == "v"
                and elem.getAttributeNode("a") is None
                and elem.getAttributeNode("b") is None
                and elem.getAttributeNode("c") is None
                and elem.getAttributeNode("p:d") is None
                and elem.getAttributeNodeNS(
                    "http://xml.python.org/ns", "c") is None
                and elem.getAttributeNode("e").isSameNode(attr)
                and attrmap["e"].isSameNode(attr))

        self.assertRaises(xml.dom.NamespaceErr, doc.renameNode, attr,
                          "http://xml.python.org/ns", "xmlns")
        self.checkRenameNodeSharedConstraints(doc, attr)
        doc.unlink()

    def testRenameElement(self):
        doc = parseString("<doc/>")
        elem = doc.documentElement

        # Simple renaming
        elem = doc.renameNode(elem, xml.dom.EMPTY_NAMESPACE, "a")
        self.confirm(elem.tagName == "a"
                and elem.nodeName == "a"
                and elem.localName is None
                and elem.namespaceURI == xml.dom.EMPTY_NAMESPACE
                and elem.prefix is None
                and elem.ownerDocument.isSameNode(doc))

        # Rename to have a namespace, no prefix
        elem = doc.renameNode(elem, "http://xml.python.org/ns", "b")
        self.confirm(elem.tagName == "b"
                and elem.nodeName == "b"
                and elem.localName == "b"
                and elem.namespaceURI == "http://xml.python.org/ns"
                and elem.prefix is None
                and elem.ownerDocument.isSameNode(doc))

        # Rename to have a namespace, with prefix
        elem = doc.renameNode(elem, "http://xml.python.org/ns2", "p:c")
        self.confirm(elem.tagName == "p:c"
                and elem.nodeName == "p:c"
                and elem.localName == "c"
                and elem.namespaceURI == "http://xml.python.org/ns2"
                and elem.prefix == "p"
                and elem.ownerDocument.isSameNode(doc))

        # Rename back to a simple non-NS node
        elem = doc.renameNode(elem, xml.dom.EMPTY_NAMESPACE, "d")
        self.confirm(elem.tagName == "d"
                and elem.nodeName == "d"
                and elem.localName is None
                and elem.namespaceURI == xml.dom.EMPTY_NAMESPACE
                and elem.prefix is None
                and elem.ownerDocument.isSameNode(doc))

        self.checkRenameNodeSharedConstraints(doc, elem)
        doc.unlink()

    def testRenameOther(self):
        # We have to create a comment node explicitly since not all DOM
        # builders used with minidom add comments to the DOM.
        doc = xml.dom.minidom.getDOMImplementation().createDocument(
            xml.dom.EMPTY_NAMESPACE, "e", None)
        node = doc.createComment("comment")
        self.assertRaises(xml.dom.NotSupportedErr, doc.renameNode, node,
                          xml.dom.EMPTY_NAMESPACE, "foo")
        doc.unlink()

    def testWholeText(self):
        doc = parseString("<doc>a</doc>")
        elem = doc.documentElement
        text = elem.childNodes[0]
        self.assertEqual(text.nodeType, Node.TEXT_NODE)

        self.checkWholeText(text, "a")
        elem.appendChild(doc.createTextNode("b"))
        self.checkWholeText(text, "ab")
        elem.insertBefore(doc.createCDATASection("c"), text)
        self.checkWholeText(text, "cab")

        # make sure we don't cross other nodes
        splitter = doc.createComment("comment")
        elem.appendChild(splitter)
        text2 = doc.createTextNode("d")
        elem.appendChild(text2)
        self.checkWholeText(text, "cab")
        self.checkWholeText(text2, "d")

        x = doc.createElement("x")
        elem.replaceChild(x, splitter)
        splitter = x
        self.checkWholeText(text, "cab")
        self.checkWholeText(text2, "d")

        x = doc.createProcessingInstruction("y", "z")
        elem.replaceChild(x, splitter)
        splitter = x
        self.checkWholeText(text, "cab")
        self.checkWholeText(text2, "d")

        elem.removeChild(splitter)
        self.checkWholeText(text, "cabd")
        self.checkWholeText(text2, "cabd")

    def testPatch1094164(self):
        doc = parseString("<doc><e/></doc>")
        elem = doc.documentElement
        e = elem.firstChild
        self.assertIs(e.parentNode, elem, "Before replaceChild()")
        # Check that replacing a child with itself leaves the tree unchanged
        elem.replaceChild(e, e)
        self.assertIs(e.parentNode, elem, "After replaceChild()")

    def testReplaceWholeText(self):
        def setup():
            doc = parseString("<doc>a<e/>d</doc>")
            elem = doc.documentElement
            text1 = elem.firstChild
            text2 = elem.lastChild
            splitter = text1.nextSibling
            elem.insertBefore(doc.createTextNode("b"), splitter)
            elem.insertBefore(doc.createCDATASection("c"), text1)
            return doc, elem, text1, splitter, text2

        doc, elem, text1, splitter, text2 = setup()
        text = text1.replaceWholeText("new content")
        self.checkWholeText(text, "new content")
        self.checkWholeText(text2, "d")
        self.assertEqual(len(elem.childNodes), 3)

        doc, elem, text1, splitter, text2 = setup()
        text = text2.replaceWholeText("new content")
        self.checkWholeText(text, "new content")
        self.checkWholeText(text1, "cab")
        self.assertEqual(len(elem.childNodes), 5)

        doc, elem, text1, splitter, text2 = setup()
        text = text1.replaceWholeText("")
        self.checkWholeText(text2, "d")
        self.confirm(text is None
                and len(elem.childNodes) == 2)

    def testSchemaType(self):
        doc = parseString(
            "<!DOCTYPE doc [\n"
            "  <!ENTITY e1 SYSTEM 'http://xml.python.org/e1'>\n"
            "  <!ENTITY e2 SYSTEM 'http://xml.python.org/e2'>\n"
            "  <!ATTLIST doc id   ID       #IMPLIED \n"
            "                ref  IDREF    #IMPLIED \n"
            "                refs IDREFS   #IMPLIED \n"
            "                enum (a|b)    #IMPLIED \n"
            "                ent  ENTITY   #IMPLIED \n"
            "                ents ENTITIES #IMPLIED \n"
            "                nm   NMTOKEN  #IMPLIED \n"
            "                nms  NMTOKENS #IMPLIED \n"
            "                text CDATA    #IMPLIED \n"
            "    >\n"
            "]><doc id='name' notid='name' text='splat!' enum='b'"
            "       ref='name' refs='name name' ent='e1' ents='e1 e2'"
            "       nm='123' nms='123 abc' />")
        elem = doc.documentElement
        # We don't want to rely on any specific loader at this point, so
        # just make sure we can get to all the names, and that the
        # DTD-based namespace is right.  The names can vary by loader
        # since each supports a different level of DTD information.
        t = elem.schemaType
        self.confirm(t.name is None
                and t.namespace == xml.dom.EMPTY_NAMESPACE)
        names = "id notid text enum ref refs ent ents nm nms".split()
        for name in names:
            a = elem.getAttributeNode(name)
            t = a.schemaType
            self.confirm(hasattr(t, "name")
                    and t.namespace == xml.dom.EMPTY_NAMESPACE)

    def testSetIdAttribute(self):
        doc = parseString("<doc a1='v' a2='w'/>")
        e = doc.documentElement
        a1 = e.getAttributeNode("a1")
        a2 = e.getAttributeNode("a2")
        self.confirm(doc.getElementById("v") is None
                and not a1.isId
                and not a2.isId)
        e.setIdAttribute("a1")
        self.confirm(e.isSameNode(doc.getElementById("v"))
                and a1.isId
                and not a2.isId)
        e.setIdAttribute("a2")
        self.confirm(e.isSameNode(doc.getElementById("v"))
                and e.isSameNode(doc.getElementById("w"))
                and a1.isId
                and a2.isId)
        # replace the a1 node; the new node should *not* be an ID
        a3 = doc.createAttribute("a1")
        a3.value = "v"
        e.setAttributeNode(a3)
        self.confirm(doc.getElementById("v") is None
                and e.isSameNode(doc.getElementById("w"))
                and not a1.isId
                and a2.isId
                and not a3.isId)
        # renaming an attribute should not affect its ID-ness:
        doc.renameNode(a2, xml.dom.EMPTY_NAMESPACE, "an")
        self.confirm(e.isSameNode(doc.getElementById("w"))
                and a2.isId)

    def testSetIdAttributeNS(self):
        NS1 = "http://xml.python.org/ns1"
        NS2 = "http://xml.python.org/ns2"
        doc = parseString("<doc"
                          " xmlns:ns1='" + NS1 + "'"
                          " xmlns:ns2='" + NS2 + "'"
                          " ns1:a1='v' ns2:a2='w'/>")
        e = doc.documentElement
        a1 = e.getAttributeNodeNS(NS1, "a1")
        a2 = e.getAttributeNodeNS(NS2, "a2")
        self.confirm(doc.getElementById("v") is None
                and not a1.isId
                and not a2.isId)
        e.setIdAttributeNS(NS1, "a1")
        self.confirm(e.isSameNode(doc.getElementById("v"))
                and a1.isId
                and not a2.isId)
        e.setIdAttributeNS(NS2, "a2")
        self.confirm(e.isSameNode(doc.getElementById("v"))
                and e.isSameNode(doc.getElementById("w"))
                and a1.isId
                and a2.isId)
        # replace the a1 node; the new node should *not* be an ID
        a3 = doc.createAttributeNS(NS1, "a1")
        a3.value = "v"
        e.setAttributeNode(a3)
        self.assertTrue(e.isSameNode(doc.getElementById("w")))
        self.assertFalse(a1.isId)
        self.assertTrue(a2.isId)
        self.assertFalse(a3.isId)
        self.assertIsNone(doc.getElementById("v"))
        # renaming an attribute should not affect its ID-ness:
        doc.renameNode(a2, xml.dom.EMPTY_NAMESPACE, "an")
        self.confirm(e.isSameNode(doc.getElementById("w"))
                and a2.isId)

    def testSetIdAttributeNode(self):
        NS1 = "http://xml.python.org/ns1"
        NS2 = "http://xml.python.org/ns2"
        doc = parseString("<doc"
                          " xmlns:ns1='" + NS1 + "'"
                          " xmlns:ns2='" + NS2 + "'"
                          " ns1:a1='v' ns2:a2='w'/>")
        e = doc.documentElement
        a1 = e.getAttributeNodeNS(NS1, "a1")
        a2 = e.getAttributeNodeNS(NS2, "a2")
        self.confirm(doc.getElementById("v") is None
                and not a1.isId
                and not a2.isId)
        e.setIdAttributeNode(a1)
        self.confirm(e.isSameNode(doc.getElementById("v"))
                and a1.isId
                and not a2.isId)
        e.setIdAttributeNode(a2)
        self.confirm(e.isSameNode(doc.getElementById("v"))
                and e.isSameNode(doc.getElementById("w"))
                and a1.isId
                and a2.isId)
        # replace the a1 node; the new node should *not* be an ID
        a3 = doc.createAttributeNS(NS1, "a1")
        a3.value = "v"
        e.setAttributeNode(a3)
        self.assertTrue(e.isSameNode(doc.getElementById("w")))
        self.assertFalse(a1.isId)
        self.assertTrue(a2.isId)
        self.assertFalse(a3.isId)
        self.assertIsNone(doc.getElementById("v"))
        # renaming an attribute should not affect its ID-ness:
        doc.renameNode(a2, xml.dom.EMPTY_NAMESPACE, "an")
        self.confirm(e.isSameNode(doc.getElementById("w"))
                and a2.isId)

    def assert_recursive_equal(self, doc, doc2):
        stack = [(doc, doc2)]
        while stack:
            n1, n2 = stack.pop()
            self.assertEqual(n1.nodeType, n2.nodeType)
            self.assertEqual(len(n1.childNodes), len(n2.childNodes))
            self.assertEqual(n1.nodeName, n2.nodeName)
            self.assertFalse(n1.isSameNode(n2))
            self.assertFalse(n2.isSameNode(n1))
            if n1.nodeType == Node.DOCUMENT_TYPE_NODE:
                len(n1.entities)
                len(n2.entities)
                len(n1.notations)
                len(n2.notations)
                self.assertEqual(len(n1.entities), len(n2.entities))
                self.assertEqual(len(n1.notations), len(n2.notations))
                for i in range(len(n1.notations)):
                    # XXX this loop body doesn't seem to be executed?
                    no1 = n1.notations.item(i)
                    no2 = n1.notations.item(i)
                    self.assertEqual(no1.name, no2.name)
                    self.assertEqual(no1.publicId, no2.publicId)
                    self.assertEqual(no1.systemId, no2.systemId)
                    stack.append((no1, no2))
                for i in range(len(n1.entities)):
                    e1 = n1.entities.item(i)
                    e2 = n2.entities.item(i)
                    self.assertEqual(e1.notationName, e2.notationName)
                    self.assertEqual(e1.publicId, e2.publicId)
                    self.assertEqual(e1.systemId, e2.systemId)
                    stack.append((e1, e2))
            if n1.nodeType != Node.DOCUMENT_NODE:
                self.assertTrue(n1.ownerDocument.isSameNode(doc))
                self.assertTrue(n2.ownerDocument.isSameNode(doc2))
            for i in range(len(n1.childNodes)):
                stack.append((n1.childNodes[i], n2.childNodes[i]))

    def testPickledDocument(self):
        doc = parseString(sample)
        for proto in range(2, pickle.HIGHEST_PROTOCOL + 1):
            s = pickle.dumps(doc, proto)
            doc2 = pickle.loads(s)
            self.assert_recursive_equal(doc, doc2)

    def testDeepcopiedDocument(self):
        doc = parseString(sample)
        doc2 = copy.deepcopy(doc)
        self.assert_recursive_equal(doc, doc2)

    def testSerializeCommentNodeWithDoubleHyphen(self):
        doc = create_doc_without_doctype()
        doc.appendChild(doc.createComment("foo--bar"))
        self.assertRaises(ValueError, doc.toxml)


    def testEmptyXMLNSValue(self):
        doc = parseString("<element xmlns=''>\n"
                          "<foo/>\n</element>")
        doc2 = parseString(doc.toxml())
        self.assertEqual(doc2.namespaceURI, xml.dom.EMPTY_NAMESPACE)

    def testExceptionOnSpacesInXMLNSValue(self):
        with self.assertRaises((ValueError, ExpatError)):
            parseString(
                '<element xmlns:abc="http:abc.com/de f g/hi/j k">' +
                '<abc:foo /></element>'
            )

    def testDocRemoveChild(self):
        doc = parse(tstfile)
        title_tag = doc.documentElement.getElementsByTagName("TITLE")[0]
        self.assertRaises( xml.dom.NotFoundErr, doc.removeChild, title_tag)
        num_children_before = len(doc.childNodes)
        doc.removeChild(doc.childNodes[0])
        num_children_after = len(doc.childNodes)
        self.assertEqual(num_children_after, num_children_before - 1)

    def testProcessingInstructionNameError(self):
        # wrong variable in .nodeValue property will
        # lead to "NameError: name 'data' is not defined"
        doc = parse(tstfile)
        pi = doc.createProcessingInstruction("y", "z")
        pi.nodeValue = "crash"

    def test_minidom_attribute_order(self):
        xml_str = '<?xml version="1.0" ?><curriculum status="public" company="example"/>'
        doc = parseString(xml_str)
        output = io.StringIO()
        doc.writexml(output)
        self.assertEqual(output.getvalue(), xml_str)

    def test_toxml_with_attributes_ordered(self):
        xml_str = '<?xml version="1.0" ?><curriculum status="public" company="example"/>'
        doc = parseString(xml_str)
        self.assertEqual(doc.toxml(), xml_str)

    def test_toprettyxml_with_attributes_ordered(self):
        xml_str = '<?xml version="1.0" ?><curriculum status="public" company="example"/>'
        doc = parseString(xml_str)
        self.assertEqual(doc.toprettyxml(),
                         '<?xml version="1.0" ?>\n'
                         '<curriculum status="public" company="example"/>\n')

    def test_toprettyxml_with_cdata(self):
        xml_str = '<?xml version="1.0" ?><root><node><![CDATA[</data>]]></node></root>'
        doc = parseString(xml_str)
        self.assertEqual(doc.toprettyxml(),
                         '<?xml version="1.0" ?>\n'
                         '<root>\n'
                         '\t<node><![CDATA[</data>]]></node>\n'
                         '</root>\n')

    def test_cdata_parsing(self):
        xml_str = '<?xml version="1.0" ?><root><node><![CDATA[</data>]]></node></root>'
        dom1 = parseString(xml_str)
        self.checkWholeText(dom1.getElementsByTagName('node')[0].firstChild, '</data>')
        dom2 = parseString(dom1.toprettyxml())
        self.checkWholeText(dom2.getElementsByTagName('node')[0].firstChild, '</data>')

if __name__ == "__main__":
    unittest.main()
