# test for xml.dom.minidom

from xml.dom.minidom import parse, Node, Document, parseString
from xml.dom import HierarchyRequestErr
import xml.parsers.expat

import os.path
import sys
import traceback
from test_support import verbose

if __name__ == "__main__":
    base = sys.argv[0]
else:
    base = __file__
tstfile = os.path.join(os.path.dirname(base), "test.xml")
del base

def confirm(test, testname = "Test"):
    if test:
        print "Passed " + testname
    else:
        print "Failed " + testname
        raise Exception

Node._debug = 1

def testParseFromFile():
    from StringIO import StringIO
    dom = parse(StringIO(open(tstfile).read()))
    dom.unlink()
    confirm(isinstance(dom,Document))

def testGetElementsByTagName():
    dom = parse(tstfile)
    confirm(dom.getElementsByTagName("LI") == \
            dom.documentElement.getElementsByTagName("LI"))
    dom.unlink()

def testInsertBefore():
    dom = parseString("<doc><foo/></doc>")
    root = dom.documentElement
    elem = root.childNodes[0]
    nelem = dom.createElement("element")
    root.insertBefore(nelem, elem)
    confirm(len(root.childNodes) == 2
            and root.childNodes[0] is nelem
            and root.childNodes[1] is elem
            and root.firstChild is nelem
            and root.lastChild is elem
            and root.toxml() == "<doc><element/><foo/></doc>"
            , "testInsertBefore -- node properly placed in tree")
    nelem = dom.createElement("element")
    root.insertBefore(nelem, None)
    confirm(len(root.childNodes) == 3
            and root.childNodes[1] is elem
            and root.childNodes[2] is nelem
            and root.lastChild is nelem
            and nelem.previousSibling is elem
            and root.toxml() == "<doc><element/><foo/><element/></doc>"
            , "testInsertBefore -- node properly placed in tree")
    nelem2 = dom.createElement("bar")
    root.insertBefore(nelem2, nelem)
    confirm(len(root.childNodes) == 4
            and root.childNodes[2] is nelem2
            and root.childNodes[3] is nelem
            and nelem2.nextSibling is nelem
            and nelem.previousSibling is nelem2
            and root.toxml() == "<doc><element/><foo/><bar/><element/></doc>"
            , "testInsertBefore -- node properly placed in tree")
    dom.unlink()

def testAppendChild():
    dom = parse(tstfile)
    dom.documentElement.appendChild(dom.createComment(u"Hello"))
    confirm(dom.documentElement.childNodes[-1].nodeName == "#comment")
    confirm(dom.documentElement.childNodes[-1].data == "Hello")
    dom.unlink()

def testLegalChildren():
    dom = Document()
    elem = dom.createElement('element')
    text = dom.createTextNode('text')

    try: dom.appendChild(text)
    except HierarchyRequestErr: pass
    else:
        print "dom.appendChild didn't raise HierarchyRequestErr"

    dom.appendChild(elem)
    try: dom.insertBefore(text, elem)
    except HierarchyRequestErr: pass
    else:
        print "dom.appendChild didn't raise HierarchyRequestErr"

    try: dom.replaceChild(text, elem)
    except HierarchyRequestErr: pass
    else:
        print "dom.appendChild didn't raise HierarchyRequestErr"

    nodemap = elem.attributes
    try: nodemap.setNamedItem(text)
    except HierarchyRequestErr: pass
    else:
        print "NamedNodeMap.setNamedItem didn't raise HierarchyRequestErr"

    try: nodemap.setNamedItemNS(text)
    except HierarchyRequestErr: pass
    else:
        print "NamedNodeMap.setNamedItemNS didn't raise HierarchyRequestErr"

    elem.appendChild(text)
    dom.unlink()

def testNonZero():
    dom = parse(tstfile)
    confirm(dom)# should not be zero
    dom.appendChild(dom.createComment("foo"))
    confirm(not dom.childNodes[-1].childNodes)
    dom.unlink()

def testUnlink():
    dom = parse(tstfile)
    dom.unlink()

def testElement():
    dom = Document()
    dom.appendChild(dom.createElement("abc"))
    confirm(dom.documentElement)
    dom.unlink()

def testAAA():
    dom = parseString("<abc/>")
    el = dom.documentElement
    el.setAttribute("spam", "jam2")
    confirm(el.toxml() == '<abc spam="jam2"/>', "testAAA")
    dom.unlink()

def testAAB():
    dom = parseString("<abc/>")
    el = dom.documentElement
    el.setAttribute("spam", "jam")
    el.setAttribute("spam", "jam2")
    confirm(el.toxml() == '<abc spam="jam2"/>', "testAAB")
    dom.unlink()

def testAddAttr():
    dom = Document()
    child = dom.appendChild(dom.createElement("abc"))

    child.setAttribute("def", "ghi")
    confirm(child.getAttribute("def") == "ghi")
    confirm(child.attributes["def"].value == "ghi")

    child.setAttribute("jkl", "mno")
    confirm(child.getAttribute("jkl") == "mno")
    confirm(child.attributes["jkl"].value == "mno")

    confirm(len(child.attributes) == 2)

    child.setAttribute("def", "newval")
    confirm(child.getAttribute("def") == "newval")
    confirm(child.attributes["def"].value == "newval")

    confirm(len(child.attributes) == 2)
    dom.unlink()

def testDeleteAttr():
    dom = Document()
    child = dom.appendChild(dom.createElement("abc"))

    confirm(len(child.attributes) == 0)
    child.setAttribute("def", "ghi")
    confirm(len(child.attributes) == 1)
    del child.attributes["def"]
    confirm(len(child.attributes) == 0)
    dom.unlink()

def testRemoveAttr():
    dom = Document()
    child = dom.appendChild(dom.createElement("abc"))

    child.setAttribute("def", "ghi")
    confirm(len(child.attributes) == 1)
    child.removeAttribute("def")
    confirm(len(child.attributes) == 0)

    dom.unlink()

def testRemoveAttrNS():
    dom = Document()
    child = dom.appendChild(
            dom.createElementNS("http://www.python.org", "python:abc"))
    child.setAttributeNS("http://www.w3.org", "xmlns:python",
                                            "http://www.python.org")
    child.setAttributeNS("http://www.python.org", "python:abcattr", "foo")
    confirm(len(child.attributes) == 2)
    child.removeAttributeNS("http://www.python.org", "abcattr")
    confirm(len(child.attributes) == 1)

    dom.unlink()

def testRemoveAttributeNode():
    dom = Document()
    child = dom.appendChild(dom.createElement("foo"))
    child.setAttribute("spam", "jam")
    confirm(len(child.attributes) == 1)
    node = child.getAttributeNode("spam")
    child.removeAttributeNode(node)
    confirm(len(child.attributes) == 0)

    dom.unlink()

def testChangeAttr():
    dom = parseString("<abc/>")
    el = dom.documentElement
    el.setAttribute("spam", "jam")
    confirm(len(el.attributes) == 1)
    el.setAttribute("spam", "bam")
    confirm(len(el.attributes) == 1)
    el.attributes["spam"] = "ham"
    confirm(len(el.attributes) == 1)
    el.setAttribute("spam2", "bam")
    confirm(len(el.attributes) == 2)
    el.attributes[ "spam2"] = "bam2"
    confirm(len(el.attributes) == 2)
    dom.unlink()

def testGetAttrList():
    pass

def testGetAttrValues(): pass

def testGetAttrLength(): pass

def testGetAttribute(): pass

def testGetAttributeNS(): pass

def testGetAttributeNode(): pass

def testGetElementsByTagNameNS():
    d="""<foo xmlns:minidom="http://pyxml.sf.net/minidom">
    <minidom:myelem/>
    </foo>"""
    dom = parseString(d)
    elem = dom.getElementsByTagNameNS("http://pyxml.sf.net/minidom","myelem")
    confirm(len(elem) == 1)
    dom.unlink()

def testGetEmptyNodeListFromElementsByTagNameNS(): pass

def testElementReprAndStr():
    dom = Document()
    el = dom.appendChild(dom.createElement("abc"))
    string1 = repr(el)
    string2 = str(el)
    confirm(string1 == string2)
    dom.unlink()

# commented out until Fredrick's fix is checked in
def _testElementReprAndStrUnicode():
    dom = Document()
    el = dom.appendChild(dom.createElement(u"abc"))
    string1 = repr(el)
    string2 = str(el)
    confirm(string1 == string2)
    dom.unlink()

# commented out until Fredrick's fix is checked in
def _testElementReprAndStrUnicodeNS():
    dom = Document()
    el = dom.appendChild(
        dom.createElementNS(u"http://www.slashdot.org", u"slash:abc"))
    string1 = repr(el)
    string2 = str(el)
    confirm(string1 == string2)
    confirm(string1.find("slash:abc") != -1)
    dom.unlink()
    confirm(len(Node.allnodes) == 0)

def testAttributeRepr():
    dom = Document()
    el = dom.appendChild(dom.createElement(u"abc"))
    node = el.setAttribute("abc", "def")
    confirm(str(node) == repr(node))
    dom.unlink()
    confirm(len(Node.allnodes) == 0)

def testTextNodeRepr(): pass

def testWriteXML():
    str = '<?xml version="1.0" ?>\n<a b="c"/>'
    dom = parseString(str)
    domstr = dom.toxml()
    dom.unlink()
    confirm(str == domstr)
    confirm(len(Node.allnodes) == 0)

def testProcessingInstruction(): pass

def testProcessingInstructionRepr(): pass

def testTextRepr(): pass

def testWriteText(): pass

def testDocumentElement(): pass

def testTooManyDocumentElements():
    doc = parseString("<doc/>")
    elem = doc.createElement("extra")
    try:
        doc.appendChild(elem)
    except HierarchyRequestErr:
        print "Caught expected exception when adding extra document element."
    else:
        print "Failed to catch expected exception when" \
              " adding extra document element."
    elem.unlink()
    doc.unlink()

def testCreateElementNS(): pass

def testCreateAttributeNS(): pass

def testParse(): pass

def testParseString(): pass

def testComment(): pass

def testAttrListItem(): pass

def testAttrListItems(): pass

def testAttrListItemNS(): pass

def testAttrListKeys(): pass

def testAttrListKeysNS(): pass

def testAttrListValues(): pass

def testAttrListLength(): pass

def testAttrList__getitem__(): pass

def testAttrList__setitem__(): pass

def testSetAttrValueandNodeValue(): pass

def testParseElement(): pass

def testParseAttributes(): pass

def testParseElementNamespaces(): pass

def testParseAttributeNamespaces(): pass

def testParseProcessingInstructions(): pass

def testChildNodes(): pass

def testFirstChild(): pass

def testHasChildNodes(): pass

def testCloneElementShallow():
    dom, clone = _setupCloneElement(0)
    confirm(len(clone.childNodes) == 0
            and clone.parentNode is None
            and clone.toxml() == '<doc attr="value"/>'
            , "testCloneElementShallow")
    dom.unlink()

def testCloneElementDeep():
    dom, clone = _setupCloneElement(1)
    confirm(len(clone.childNodes) == 1
            and clone.parentNode is None
            and clone.toxml() == '<doc attr="value"><foo/></doc>'
            , "testCloneElementDeep")
    dom.unlink()

def _setupCloneElement(deep):
    dom = parseString("<doc attr='value'><foo/></doc>")
    root = dom.documentElement
    clone = root.cloneNode(deep)
    _testCloneElementCopiesAttributes(
        root, clone, "testCloneElement" + (deep and "Deep" or "Shallow"))
    # mutilate the original so shared data is detected
    root.tagName = root.nodeName = "MODIFIED"
    root.setAttribute("attr", "NEW VALUE")
    root.setAttribute("added", "VALUE")
    return dom, clone

def _testCloneElementCopiesAttributes(e1, e2, test):
    attrs1 = e1.attributes
    attrs2 = e2.attributes
    keys1 = attrs1.keys()
    keys2 = attrs2.keys()
    keys1.sort()
    keys2.sort()
    confirm(keys1 == keys2, "clone of element has same attribute keys")
    for i in range(len(keys1)):
        a1 = attrs1.item(i)
        a2 = attrs2.item(i)
        confirm(a1 is not a2
                and a1.value == a2.value
                and a1.nodeValue == a2.nodeValue
                and a1.namespaceURI == a2.namespaceURI
                and a1.localName == a2.localName
                , "clone of attribute node has proper attribute values")
        confirm(a2.ownerElement is e2,
                "clone of attribute node correctly owned")


def testCloneDocumentShallow(): pass

def testCloneDocumentDeep(): pass

def testCloneAttributeShallow(): pass

def testCloneAttributeDeep(): pass

def testClonePIShallow(): pass

def testClonePIDeep(): pass

def testNormalize():
    doc = parseString("<doc/>")
    root = doc.documentElement
    root.appendChild(doc.createTextNode("first"))
    root.appendChild(doc.createTextNode("second"))
    confirm(len(root.childNodes) == 2, "testNormalize -- preparation")
    doc.normalize()
    confirm(len(root.childNodes) == 1
            and root.firstChild is root.lastChild
            and root.firstChild.data == "firstsecond"
            , "testNormalize -- result")
    doc.unlink()

    doc = parseString("<doc/>")
    root = doc.documentElement
    root.appendChild(doc.createTextNode(""))
    doc.normalize()
    confirm(len(root.childNodes) == 0,
            "testNormalize -- single empty node removed")
    doc.unlink()

def testSiblings():
    doc = parseString("<doc><?pi?>text?<elm/></doc>")
    root = doc.documentElement
    (pi, text, elm) = root.childNodes

    confirm(pi.nextSibling is text and
            pi.previousSibling is None and
            text.nextSibling is elm and
            text.previousSibling is pi and
            elm.nextSibling is None and
            elm.previousSibling is text, "testSiblings")

    doc.unlink()

def testParents():
    doc = parseString("<doc><elm1><elm2/><elm2><elm3/></elm2></elm1></doc>")
    root = doc.documentElement
    elm1 = root.childNodes[0]
    (elm2a, elm2b) = elm1.childNodes
    elm3 = elm2b.childNodes[0]

    confirm(root.parentNode is doc and
            elm1.parentNode is root and
            elm2a.parentNode is elm1 and
            elm2b.parentNode is elm1 and
            elm3.parentNode is elm2b, "testParents")

    doc.unlink()

def testSAX2DOM():
    from xml.dom import pulldom

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

    confirm(text1.previousSibling is None and
            text1.nextSibling is elm1 and
            elm1.previousSibling is text1 and
            elm1.nextSibling is text2 and
            text2.previousSibling is elm1 and
            text2.nextSibling is None and
            text3.previousSibling is None and
            text3.nextSibling is None, "testSAX2DOM - siblings")

    confirm(root.parentNode is doc and
            text1.parentNode is root and
            elm1.parentNode is root and
            text2.parentNode is root and
            text3.parentNode is elm1, "testSAX2DOM - parents")

    doc.unlink()

# --- MAIN PROGRAM

names = globals().keys()
names.sort()

failed = []

for name in names:
    if name.startswith("test"):
        func = globals()[name]
        try:
            func()
            print "Test Succeeded", name
            confirm(len(Node.allnodes) == 0,
                    "assertion: len(Node.allnodes) == 0")
            if len(Node.allnodes):
                print "Garbage left over:"
                if verbose:
                    print Node.allnodes.items()[0:10]
                else:
                    # Don't print specific nodes if repeatable results
                    # are needed
                    print len(Node.allnodes)
            Node.allnodes = {}
        except:
            failed.append(name)
            print "Test Failed: ", name
            sys.stdout.flush()
            traceback.print_exception(*sys.exc_info())
            print `sys.exc_info()[1]`
            Node.allnodes = {}

if failed:
    print "\n\n\n**** Check for failures in these tests:"
    for name in failed:
        print "  " + name
    print
else:
    print "All tests succeeded"

Node.debug = None # Delete debug output collected in a StringIO object
Node._debug = 0   # And reset debug mode
