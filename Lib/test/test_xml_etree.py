# xml.etree test.  This file contains enough tests to make sure that
# all included components work as they should.
# Large parts are extracted from the upstream test suite.
#
# PLEASE write all new tests using the standard unittest infrastructure and
# not doctest.
#
# IMPORTANT: the same tests are run from "test_xml_etree_c" in order
# to ensure consistency between the C implementation and the Python
# implementation.
#
# For this purpose, the module-level "ET" symbol is temporarily
# monkey-patched when running the "test_xml_etree_c" test suite.
# Don't re-import "xml.etree.ElementTree" module in the docstring,
# except if the test is specific to the Python implementation.

import html
import io
import sys
import unittest
import weakref

from test import support
from test.support import findfile, import_fresh_module, gc_collect

pyET = import_fresh_module('xml.etree.ElementTree', blocked=['_elementtree'])

SIMPLE_XMLFILE = findfile("simple.xml", subdir="xmltestdata")
try:
    SIMPLE_XMLFILE.encode("utf-8")
except UnicodeEncodeError:
    raise unittest.SkipTest("filename is not encodable to utf8")
SIMPLE_NS_XMLFILE = findfile("simple-ns.xml", subdir="xmltestdata")

SAMPLE_XML = """\
<body>
  <tag class='a'>text</tag>
  <tag class='b' />
  <section>
    <tag class='b' id='inner'>subtext</tag>
  </section>
</body>
"""

SAMPLE_SECTION = """\
<section>
  <tag class='b' id='inner'>subtext</tag>
  <nexttag />
  <nextsection>
    <tag />
  </nextsection>
</section>
"""

SAMPLE_XML_NS = """
<body xmlns="http://effbot.org/ns">
  <tag>text</tag>
  <tag />
  <section>
    <tag>subtext</tag>
  </section>
</body>
"""


def sanity():
    """
    Import sanity.

    >>> from xml.etree import ElementTree
    >>> from xml.etree import ElementInclude
    >>> from xml.etree import ElementPath
    """

def check_method(method):
    if not hasattr(method, '__call__'):
        print(method, "not callable")

def serialize(elem, to_string=True, encoding='unicode', **options):
    import io
    if encoding != 'unicode':
        file = io.BytesIO()
    else:
        file = io.StringIO()
    tree = ET.ElementTree(elem)
    tree.write(file, encoding=encoding, **options)
    if to_string:
        return file.getvalue()
    else:
        file.seek(0)
        return file

def summarize(elem):
    if elem.tag == ET.Comment:
        return "<Comment>"
    return elem.tag

def summarize_list(seq):
    return [summarize(elem) for elem in seq]

def normalize_crlf(tree):
    for elem in tree.iter():
        if elem.text:
            elem.text = elem.text.replace("\r\n", "\n")
        if elem.tail:
            elem.tail = elem.tail.replace("\r\n", "\n")

def normalize_exception(func, *args, **kwargs):
    # Ignore the exception __module__
    try:
        func(*args, **kwargs)
    except Exception as err:
        print("Traceback (most recent call last):")
        print("{}: {}".format(err.__class__.__name__, err))

def check_string(string):
    len(string)
    for char in string:
        if len(char) != 1:
            print("expected one-character string, got %r" % char)
    new_string = string + ""
    new_string = string + " "
    string[:0]

def check_mapping(mapping):
    len(mapping)
    keys = mapping.keys()
    items = mapping.items()
    for key in keys:
        item = mapping[key]
    mapping["key"] = "value"
    if mapping["key"] != "value":
        print("expected value string, got %r" % mapping["key"])

def check_element(element):
    if not ET.iselement(element):
        print("not an element")
    if not hasattr(element, "tag"):
        print("no tag member")
    if not hasattr(element, "attrib"):
        print("no attrib member")
    if not hasattr(element, "text"):
        print("no text member")
    if not hasattr(element, "tail"):
        print("no tail member")

    check_string(element.tag)
    check_mapping(element.attrib)
    if element.text is not None:
        check_string(element.text)
    if element.tail is not None:
        check_string(element.tail)
    for elem in element:
        check_element(elem)

# --------------------------------------------------------------------
# element tree tests

def interface():
    """
    Test element tree interface.

    >>> element = ET.Element("tag")
    >>> check_element(element)
    >>> tree = ET.ElementTree(element)
    >>> check_element(tree.getroot())

    >>> element = ET.Element("t\\xe4g", key="value")
    >>> tree = ET.ElementTree(element)
    >>> repr(element)   # doctest: +ELLIPSIS
    "<Element 't\\xe4g' at 0x...>"
    >>> element = ET.Element("tag", key="value")

    Make sure all standard element methods exist.

    >>> check_method(element.append)
    >>> check_method(element.extend)
    >>> check_method(element.insert)
    >>> check_method(element.remove)
    >>> check_method(element.getchildren)
    >>> check_method(element.find)
    >>> check_method(element.iterfind)
    >>> check_method(element.findall)
    >>> check_method(element.findtext)
    >>> check_method(element.clear)
    >>> check_method(element.get)
    >>> check_method(element.set)
    >>> check_method(element.keys)
    >>> check_method(element.items)
    >>> check_method(element.iter)
    >>> check_method(element.itertext)
    >>> check_method(element.getiterator)

    These methods return an iterable. See bug 6472.

    >>> check_method(element.iter("tag").__next__)
    >>> check_method(element.iterfind("tag").__next__)
    >>> check_method(element.iterfind("*").__next__)
    >>> check_method(tree.iter("tag").__next__)
    >>> check_method(tree.iterfind("tag").__next__)
    >>> check_method(tree.iterfind("*").__next__)

    These aliases are provided:

    >>> assert ET.XML == ET.fromstring
    >>> assert ET.PI == ET.ProcessingInstruction
    >>> assert ET.XMLParser == ET.XMLTreeBuilder
    """

def simpleops():
    """
    Basic method sanity checks.

    >>> elem = ET.XML("<body><tag/></body>")
    >>> serialize(elem)
    '<body><tag /></body>'
    >>> e = ET.Element("tag2")
    >>> elem.append(e)
    >>> serialize(elem)
    '<body><tag /><tag2 /></body>'
    >>> elem.remove(e)
    >>> serialize(elem)
    '<body><tag /></body>'
    >>> elem.insert(0, e)
    >>> serialize(elem)
    '<body><tag2 /><tag /></body>'
    >>> elem.remove(e)
    >>> elem.extend([e])
    >>> serialize(elem)
    '<body><tag /><tag2 /></body>'
    >>> elem.remove(e)

    >>> element = ET.Element("tag", key="value")
    >>> serialize(element) # 1
    '<tag key="value" />'
    >>> subelement = ET.Element("subtag")
    >>> element.append(subelement)
    >>> serialize(element) # 2
    '<tag key="value"><subtag /></tag>'
    >>> element.insert(0, subelement)
    >>> serialize(element) # 3
    '<tag key="value"><subtag /><subtag /></tag>'
    >>> element.remove(subelement)
    >>> serialize(element) # 4
    '<tag key="value"><subtag /></tag>'
    >>> element.remove(subelement)
    >>> serialize(element) # 5
    '<tag key="value" />'
    >>> element.remove(subelement)
    Traceback (most recent call last):
    ValueError: list.remove(x): x not in list
    >>> serialize(element) # 6
    '<tag key="value" />'
    >>> element[0:0] = [subelement, subelement, subelement]
    >>> serialize(element[1])
    '<subtag />'
    >>> element[1:9] == [element[1], element[2]]
    True
    >>> element[:9:2] == [element[0], element[2]]
    True
    >>> del element[1:2]
    >>> serialize(element)
    '<tag key="value"><subtag /><subtag /></tag>'
    """

def cdata():
    """
    Test CDATA handling (etc).

    >>> serialize(ET.XML("<tag>hello</tag>"))
    '<tag>hello</tag>'
    >>> serialize(ET.XML("<tag>&#104;&#101;&#108;&#108;&#111;</tag>"))
    '<tag>hello</tag>'
    >>> serialize(ET.XML("<tag><![CDATA[hello]]></tag>"))
    '<tag>hello</tag>'
    """

# Only with Python implementation
def simplefind():
    """
    Test find methods using the elementpath fallback.

    >>> ElementTree = pyET

    >>> CurrentElementPath = ElementTree.ElementPath
    >>> ElementTree.ElementPath = ElementTree._SimpleElementPath()
    >>> elem = ElementTree.XML(SAMPLE_XML)
    >>> elem.find("tag").tag
    'tag'
    >>> ElementTree.ElementTree(elem).find("tag").tag
    'tag'
    >>> elem.findtext("tag")
    'text'
    >>> elem.findtext("tog")
    >>> elem.findtext("tog", "default")
    'default'
    >>> ElementTree.ElementTree(elem).findtext("tag")
    'text'
    >>> summarize_list(elem.findall("tag"))
    ['tag', 'tag']
    >>> summarize_list(elem.findall(".//tag"))
    ['tag', 'tag', 'tag']

    Path syntax doesn't work in this case.

    >>> elem.find("section/tag")
    >>> elem.findtext("section/tag")
    >>> summarize_list(elem.findall("section/tag"))
    []

    >>> ElementTree.ElementPath = CurrentElementPath
    """

def find():
    """
    Test find methods (including xpath syntax).

    >>> elem = ET.XML(SAMPLE_XML)
    >>> elem.find("tag").tag
    'tag'
    >>> ET.ElementTree(elem).find("tag").tag
    'tag'
    >>> elem.find("section/tag").tag
    'tag'
    >>> elem.find("./tag").tag
    'tag'
    >>> ET.ElementTree(elem).find("./tag").tag
    'tag'
    >>> ET.ElementTree(elem).find("/tag").tag
    'tag'
    >>> elem[2] = ET.XML(SAMPLE_SECTION)
    >>> elem.find("section/nexttag").tag
    'nexttag'
    >>> ET.ElementTree(elem).find("section/tag").tag
    'tag'
    >>> ET.ElementTree(elem).find("tog")
    >>> ET.ElementTree(elem).find("tog/foo")
    >>> elem.findtext("tag")
    'text'
    >>> elem.findtext("section/nexttag")
    ''
    >>> elem.findtext("section/nexttag", "default")
    ''
    >>> elem.findtext("tog")
    >>> elem.findtext("tog", "default")
    'default'
    >>> ET.ElementTree(elem).findtext("tag")
    'text'
    >>> ET.ElementTree(elem).findtext("tog/foo")
    >>> ET.ElementTree(elem).findtext("tog/foo", "default")
    'default'
    >>> ET.ElementTree(elem).findtext("./tag")
    'text'
    >>> ET.ElementTree(elem).findtext("/tag")
    'text'
    >>> elem.findtext("section/tag")
    'subtext'
    >>> ET.ElementTree(elem).findtext("section/tag")
    'subtext'
    >>> summarize_list(elem.findall("."))
    ['body']
    >>> summarize_list(elem.findall("tag"))
    ['tag', 'tag']
    >>> summarize_list(elem.findall("tog"))
    []
    >>> summarize_list(elem.findall("tog/foo"))
    []
    >>> summarize_list(elem.findall("*"))
    ['tag', 'tag', 'section']
    >>> summarize_list(elem.findall(".//tag"))
    ['tag', 'tag', 'tag', 'tag']
    >>> summarize_list(elem.findall("section/tag"))
    ['tag']
    >>> summarize_list(elem.findall("section//tag"))
    ['tag', 'tag']
    >>> summarize_list(elem.findall("section/*"))
    ['tag', 'nexttag', 'nextsection']
    >>> summarize_list(elem.findall("section//*"))
    ['tag', 'nexttag', 'nextsection', 'tag']
    >>> summarize_list(elem.findall("section/.//*"))
    ['tag', 'nexttag', 'nextsection', 'tag']
    >>> summarize_list(elem.findall("*/*"))
    ['tag', 'nexttag', 'nextsection']
    >>> summarize_list(elem.findall("*//*"))
    ['tag', 'nexttag', 'nextsection', 'tag']
    >>> summarize_list(elem.findall("*/tag"))
    ['tag']
    >>> summarize_list(elem.findall("*/./tag"))
    ['tag']
    >>> summarize_list(elem.findall("./tag"))
    ['tag', 'tag']
    >>> summarize_list(elem.findall(".//tag"))
    ['tag', 'tag', 'tag', 'tag']
    >>> summarize_list(elem.findall("././tag"))
    ['tag', 'tag']
    >>> summarize_list(elem.findall(".//tag[@class]"))
    ['tag', 'tag', 'tag']
    >>> summarize_list(elem.findall(".//tag[@class='a']"))
    ['tag']
    >>> summarize_list(elem.findall(".//tag[@class='b']"))
    ['tag', 'tag']
    >>> summarize_list(elem.findall(".//tag[@id]"))
    ['tag']
    >>> summarize_list(elem.findall(".//section[tag]"))
    ['section']
    >>> summarize_list(elem.findall(".//section[element]"))
    []
    >>> summarize_list(elem.findall("../tag"))
    []
    >>> summarize_list(elem.findall("section/../tag"))
    ['tag', 'tag']
    >>> summarize_list(ET.ElementTree(elem).findall("./tag"))
    ['tag', 'tag']

    Following example is invalid in 1.2.
    A leading '*' is assumed in 1.3.

    >>> elem.findall("section//") == elem.findall("section//*")
    True

    ET's Path module handles this case incorrectly; this gives
    a warning in 1.3, and the behaviour will be modified in 1.4.

    >>> summarize_list(ET.ElementTree(elem).findall("/tag"))
    ['tag', 'tag']

    >>> elem = ET.XML(SAMPLE_XML_NS)
    >>> summarize_list(elem.findall("tag"))
    []
    >>> summarize_list(elem.findall("{http://effbot.org/ns}tag"))
    ['{http://effbot.org/ns}tag', '{http://effbot.org/ns}tag']
    >>> summarize_list(elem.findall(".//{http://effbot.org/ns}tag"))
    ['{http://effbot.org/ns}tag', '{http://effbot.org/ns}tag', '{http://effbot.org/ns}tag']
    """

def file_init():
    """
    >>> import io

    >>> stringfile = io.BytesIO(SAMPLE_XML.encode("utf-8"))
    >>> tree = ET.ElementTree(file=stringfile)
    >>> tree.find("tag").tag
    'tag'
    >>> tree.find("section/tag").tag
    'tag'

    >>> tree = ET.ElementTree(file=SIMPLE_XMLFILE)
    >>> tree.find("element").tag
    'element'
    >>> tree.find("element/../empty-element").tag
    'empty-element'
    """

def bad_find():
    """
    Check bad or unsupported path expressions.

    >>> elem = ET.XML(SAMPLE_XML)
    >>> elem.findall("/tag")
    Traceback (most recent call last):
    SyntaxError: cannot use absolute path on element
    """

def path_cache():
    """
    Check that the path cache behaves sanely.

    >>> from xml.etree import ElementPath

    >>> elem = ET.XML(SAMPLE_XML)
    >>> for i in range(10): ET.ElementTree(elem).find('./'+str(i))
    >>> cache_len_10 = len(ElementPath._cache)
    >>> for i in range(10): ET.ElementTree(elem).find('./'+str(i))
    >>> len(ElementPath._cache) == cache_len_10
    True
    >>> for i in range(20): ET.ElementTree(elem).find('./'+str(i))
    >>> len(ElementPath._cache) > cache_len_10
    True
    >>> for i in range(600): ET.ElementTree(elem).find('./'+str(i))
    >>> len(ElementPath._cache) < 500
    True
    """

def copy():
    """
    Test copy handling (etc).

    >>> import copy
    >>> e1 = ET.XML("<tag>hello<foo/></tag>")
    >>> e2 = copy.copy(e1)
    >>> e3 = copy.deepcopy(e1)
    >>> e1.find("foo").tag = "bar"
    >>> serialize(e1)
    '<tag>hello<bar /></tag>'
    >>> serialize(e2)
    '<tag>hello<bar /></tag>'
    >>> serialize(e3)
    '<tag>hello<foo /></tag>'

    """

def attrib():
    """
    Test attribute handling.

    >>> elem = ET.Element("tag")
    >>> elem.get("key") # 1.1
    >>> elem.get("key", "default") # 1.2
    'default'
    >>> elem.set("key", "value")
    >>> elem.get("key") # 1.3
    'value'

    >>> elem = ET.Element("tag", key="value")
    >>> elem.get("key") # 2.1
    'value'
    >>> elem.attrib # 2.2
    {'key': 'value'}

    >>> attrib = {"key": "value"}
    >>> elem = ET.Element("tag", attrib)
    >>> attrib.clear() # check for aliasing issues
    >>> elem.get("key") # 3.1
    'value'
    >>> elem.attrib # 3.2
    {'key': 'value'}

    >>> attrib = {"key": "value"}
    >>> elem = ET.Element("tag", **attrib)
    >>> attrib.clear() # check for aliasing issues
    >>> elem.get("key") # 4.1
    'value'
    >>> elem.attrib # 4.2
    {'key': 'value'}

    >>> elem = ET.Element("tag", {"key": "other"}, key="value")
    >>> elem.get("key") # 5.1
    'value'
    >>> elem.attrib # 5.2
    {'key': 'value'}

    >>> elem = ET.Element('test')
    >>> elem.text = "aa"
    >>> elem.set('testa', 'testval')
    >>> elem.set('testb', 'test2')
    >>> ET.tostring(elem)
    b'<test testa="testval" testb="test2">aa</test>'
    >>> sorted(elem.keys())
    ['testa', 'testb']
    >>> sorted(elem.items())
    [('testa', 'testval'), ('testb', 'test2')]
    >>> elem.attrib['testb']
    'test2'
    >>> elem.attrib['testb'] = 'test1'
    >>> elem.attrib['testc'] = 'test2'
    >>> ET.tostring(elem)
    b'<test testa="testval" testb="test1" testc="test2">aa</test>'
    """

def makeelement():
    """
    Test makeelement handling.

    >>> elem = ET.Element("tag")
    >>> attrib = {"key": "value"}
    >>> subelem = elem.makeelement("subtag", attrib)
    >>> if subelem.attrib is attrib:
    ...     print("attrib aliasing")
    >>> elem.append(subelem)
    >>> serialize(elem)
    '<tag><subtag key="value" /></tag>'

    >>> elem.clear()
    >>> serialize(elem)
    '<tag />'
    >>> elem.append(subelem)
    >>> serialize(elem)
    '<tag><subtag key="value" /></tag>'
    >>> elem.extend([subelem, subelem])
    >>> serialize(elem)
    '<tag><subtag key="value" /><subtag key="value" /><subtag key="value" /></tag>'
    >>> elem[:] = [subelem]
    >>> serialize(elem)
    '<tag><subtag key="value" /></tag>'
    >>> elem[:] = tuple([subelem])
    >>> serialize(elem)
    '<tag><subtag key="value" /></tag>'

    """

def parsefile():
    """
    Test parsing from file.

    >>> tree = ET.parse(SIMPLE_XMLFILE)
    >>> normalize_crlf(tree)
    >>> tree.write(sys.stdout, encoding='unicode')
    <root>
       <element key="value">text</element>
       <element>text</element>tail
       <empty-element />
    </root>
    >>> tree = ET.parse(SIMPLE_NS_XMLFILE)
    >>> normalize_crlf(tree)
    >>> tree.write(sys.stdout, encoding='unicode')
    <ns0:root xmlns:ns0="namespace">
       <ns0:element key="value">text</ns0:element>
       <ns0:element>text</ns0:element>tail
       <ns0:empty-element />
    </ns0:root>

    >>> with open(SIMPLE_XMLFILE) as f:
    ...     data = f.read()

    >>> parser = ET.XMLParser()
    >>> parser.version  # doctest: +ELLIPSIS
    'Expat ...'
    >>> parser.feed(data)
    >>> print(serialize(parser.close()))
    <root>
       <element key="value">text</element>
       <element>text</element>tail
       <empty-element />
    </root>

    >>> parser = ET.XMLTreeBuilder() # 1.2 compatibility
    >>> parser.feed(data)
    >>> print(serialize(parser.close()))
    <root>
       <element key="value">text</element>
       <element>text</element>tail
       <empty-element />
    </root>

    >>> target = ET.TreeBuilder()
    >>> parser = ET.XMLParser(target=target)
    >>> parser.feed(data)
    >>> print(serialize(parser.close()))
    <root>
       <element key="value">text</element>
       <element>text</element>tail
       <empty-element />
    </root>
    """

def parseliteral():
    """
    >>> element = ET.XML("<html><body>text</body></html>")
    >>> ET.ElementTree(element).write(sys.stdout, encoding='unicode')
    <html><body>text</body></html>
    >>> element = ET.fromstring("<html><body>text</body></html>")
    >>> ET.ElementTree(element).write(sys.stdout, encoding='unicode')
    <html><body>text</body></html>
    >>> sequence = ["<html><body>", "text</bo", "dy></html>"]
    >>> element = ET.fromstringlist(sequence)
    >>> ET.tostring(element)
    b'<html><body>text</body></html>'
    >>> b"".join(ET.tostringlist(element))
    b'<html><body>text</body></html>'
    >>> ET.tostring(element, "ascii")
    b"<?xml version='1.0' encoding='ascii'?>\\n<html><body>text</body></html>"
    >>> _, ids = ET.XMLID("<html><body>text</body></html>")
    >>> len(ids)
    0
    >>> _, ids = ET.XMLID("<html><body id='body'>text</body></html>")
    >>> len(ids)
    1
    >>> ids["body"].tag
    'body'
    """

def iterparse():
    """
    Test iterparse interface.

    >>> iterparse = ET.iterparse

    >>> context = iterparse(SIMPLE_XMLFILE)
    >>> action, elem = next(context)
    >>> print(action, elem.tag)
    end element
    >>> for action, elem in context:
    ...   print(action, elem.tag)
    end element
    end empty-element
    end root
    >>> context.root.tag
    'root'

    >>> context = iterparse(SIMPLE_NS_XMLFILE)
    >>> for action, elem in context:
    ...   print(action, elem.tag)
    end {namespace}element
    end {namespace}element
    end {namespace}empty-element
    end {namespace}root

    >>> events = ()
    >>> context = iterparse(SIMPLE_XMLFILE, events)
    >>> for action, elem in context:
    ...   print(action, elem.tag)

    >>> events = ()
    >>> context = iterparse(SIMPLE_XMLFILE, events=events)
    >>> for action, elem in context:
    ...   print(action, elem.tag)

    >>> events = ("start", "end")
    >>> context = iterparse(SIMPLE_XMLFILE, events)
    >>> for action, elem in context:
    ...   print(action, elem.tag)
    start root
    start element
    end element
    start element
    end element
    start empty-element
    end empty-element
    end root

    >>> events = ("start", "end", "start-ns", "end-ns")
    >>> context = iterparse(SIMPLE_NS_XMLFILE, events)
    >>> for action, elem in context:
    ...   if action in ("start", "end"):
    ...     print(action, elem.tag)
    ...   else:
    ...     print(action, elem)
    start-ns ('', 'namespace')
    start {namespace}root
    start {namespace}element
    end {namespace}element
    start {namespace}element
    end {namespace}element
    start {namespace}empty-element
    end {namespace}empty-element
    end {namespace}root
    end-ns None

    >>> events = ("start", "end", "bogus")
    >>> with open(SIMPLE_XMLFILE, "rb") as f:
    ...     iterparse(f, events)
    Traceback (most recent call last):
    ValueError: unknown event 'bogus'

    >>> import io

    >>> source = io.BytesIO(
    ...     b"<?xml version='1.0' encoding='iso-8859-1'?>\\n"
    ...     b"<body xmlns='http://&#233;ffbot.org/ns'\\n"
    ...     b"      xmlns:cl\\xe9='http://effbot.org/ns'>text</body>\\n")
    >>> events = ("start-ns",)
    >>> context = iterparse(source, events)
    >>> for action, elem in context:
    ...     print(action, elem)
    start-ns ('', 'http://\\xe9ffbot.org/ns')
    start-ns ('cl\\xe9', 'http://effbot.org/ns')

    >>> source = io.StringIO("<document />junk")
    >>> try:
    ...   for action, elem in iterparse(source):
    ...     print(action, elem.tag)
    ... except ET.ParseError as v:
    ...   print(v)
    end document
    junk after document element: line 1, column 12
    """

def writefile():
    """
    >>> elem = ET.Element("tag")
    >>> elem.text = "text"
    >>> serialize(elem)
    '<tag>text</tag>'
    >>> ET.SubElement(elem, "subtag").text = "subtext"
    >>> serialize(elem)
    '<tag>text<subtag>subtext</subtag></tag>'

    Test tag suppression
    >>> elem.tag = None
    >>> serialize(elem)
    'text<subtag>subtext</subtag>'
    >>> elem.insert(0, ET.Comment("comment"))
    >>> serialize(elem)     # assumes 1.3
    'text<!--comment--><subtag>subtext</subtag>'
    >>> elem[0] = ET.PI("key", "value")
    >>> serialize(elem)
    'text<?key value?><subtag>subtext</subtag>'
    """

def custom_builder():
    """
    Test parser w. custom builder.

    >>> with open(SIMPLE_XMLFILE) as f:
    ...     data = f.read()
    >>> class Builder:
    ...     def start(self, tag, attrib):
    ...         print("start", tag)
    ...     def end(self, tag):
    ...         print("end", tag)
    ...     def data(self, text):
    ...         pass
    >>> builder = Builder()
    >>> parser = ET.XMLParser(target=builder)
    >>> parser.feed(data)
    start root
    start element
    end element
    start element
    end element
    start empty-element
    end empty-element
    end root

    >>> with open(SIMPLE_NS_XMLFILE) as f:
    ...     data = f.read()
    >>> class Builder:
    ...     def start(self, tag, attrib):
    ...         print("start", tag)
    ...     def end(self, tag):
    ...         print("end", tag)
    ...     def data(self, text):
    ...         pass
    ...     def pi(self, target, data):
    ...         print("pi", target, repr(data))
    ...     def comment(self, data):
    ...         print("comment", repr(data))
    >>> builder = Builder()
    >>> parser = ET.XMLParser(target=builder)
    >>> parser.feed(data)
    pi pi 'data'
    comment ' comment '
    start {namespace}root
    start {namespace}element
    end {namespace}element
    start {namespace}element
    end {namespace}element
    start {namespace}empty-element
    end {namespace}empty-element
    end {namespace}root

    """

def getchildren():
    """
    Test Element.getchildren()

    >>> with open(SIMPLE_XMLFILE, "rb") as f:
    ...     tree = ET.parse(f)
    >>> for elem in tree.getroot().iter():
    ...     summarize_list(elem.getchildren())
    ['element', 'element', 'empty-element']
    []
    []
    []
    >>> for elem in tree.getiterator():
    ...     summarize_list(elem.getchildren())
    ['element', 'element', 'empty-element']
    []
    []
    []

    >>> elem = ET.XML(SAMPLE_XML)
    >>> len(elem.getchildren())
    3
    >>> len(elem[2].getchildren())
    1
    >>> elem[:] == elem.getchildren()
    True
    >>> child1 = elem[0]
    >>> child2 = elem[2]
    >>> del elem[1:2]
    >>> len(elem.getchildren())
    2
    >>> child1 == elem[0]
    True
    >>> child2 == elem[1]
    True
    >>> elem[0:2] = [child2, child1]
    >>> child2 == elem[0]
    True
    >>> child1 == elem[1]
    True
    >>> child1 == elem[0]
    False
    >>> elem.clear()
    >>> elem.getchildren()
    []
    """

def writestring():
    """
    >>> elem = ET.XML("<html><body>text</body></html>")
    >>> ET.tostring(elem)
    b'<html><body>text</body></html>'
    >>> elem = ET.fromstring("<html><body>text</body></html>")
    >>> ET.tostring(elem)
    b'<html><body>text</body></html>'
    """

def check_encoding(encoding):
    """
    >>> check_encoding("ascii")
    >>> check_encoding("us-ascii")
    >>> check_encoding("iso-8859-1")
    >>> check_encoding("iso-8859-15")
    >>> check_encoding("cp437")
    >>> check_encoding("mac-roman")
    """
    ET.XML("<?xml version='1.0' encoding='%s'?><xml />" % encoding)

def encoding():
    r"""
    Test encoding issues.

    >>> elem = ET.Element("tag")
    >>> elem.text = "abc"
    >>> serialize(elem)
    '<tag>abc</tag>'
    >>> serialize(elem, encoding="utf-8")
    b'<tag>abc</tag>'
    >>> serialize(elem, encoding="us-ascii")
    b'<tag>abc</tag>'
    >>> serialize(elem, encoding="iso-8859-1")
    b"<?xml version='1.0' encoding='iso-8859-1'?>\n<tag>abc</tag>"

    >>> elem.text = "<&\"\'>"
    >>> serialize(elem)
    '<tag>&lt;&amp;"\'&gt;</tag>'
    >>> serialize(elem, encoding="utf-8")
    b'<tag>&lt;&amp;"\'&gt;</tag>'
    >>> serialize(elem, encoding="us-ascii") # cdata characters
    b'<tag>&lt;&amp;"\'&gt;</tag>'
    >>> serialize(elem, encoding="iso-8859-1")
    b'<?xml version=\'1.0\' encoding=\'iso-8859-1\'?>\n<tag>&lt;&amp;"\'&gt;</tag>'

    >>> elem.attrib["key"] = "<&\"\'>"
    >>> elem.text = None
    >>> serialize(elem)
    '<tag key="&lt;&amp;&quot;\'&gt;" />'
    >>> serialize(elem, encoding="utf-8")
    b'<tag key="&lt;&amp;&quot;\'&gt;" />'
    >>> serialize(elem, encoding="us-ascii")
    b'<tag key="&lt;&amp;&quot;\'&gt;" />'
    >>> serialize(elem, encoding="iso-8859-1")
    b'<?xml version=\'1.0\' encoding=\'iso-8859-1\'?>\n<tag key="&lt;&amp;&quot;\'&gt;" />'

    >>> elem.text = '\xe5\xf6\xf6<>'
    >>> elem.attrib.clear()
    >>> serialize(elem)
    '<tag>\xe5\xf6\xf6&lt;&gt;</tag>'
    >>> serialize(elem, encoding="utf-8")
    b'<tag>\xc3\xa5\xc3\xb6\xc3\xb6&lt;&gt;</tag>'
    >>> serialize(elem, encoding="us-ascii")
    b'<tag>&#229;&#246;&#246;&lt;&gt;</tag>'
    >>> serialize(elem, encoding="iso-8859-1")
    b"<?xml version='1.0' encoding='iso-8859-1'?>\n<tag>\xe5\xf6\xf6&lt;&gt;</tag>"

    >>> elem.attrib["key"] = '\xe5\xf6\xf6<>'
    >>> elem.text = None
    >>> serialize(elem)
    '<tag key="\xe5\xf6\xf6&lt;&gt;" />'
    >>> serialize(elem, encoding="utf-8")
    b'<tag key="\xc3\xa5\xc3\xb6\xc3\xb6&lt;&gt;" />'
    >>> serialize(elem, encoding="us-ascii")
    b'<tag key="&#229;&#246;&#246;&lt;&gt;" />'
    >>> serialize(elem, encoding="iso-8859-1")
    b'<?xml version=\'1.0\' encoding=\'iso-8859-1\'?>\n<tag key="\xe5\xf6\xf6&lt;&gt;" />'
    """

def methods():
    r"""
    Test serialization methods.

    >>> e = ET.XML("<html><link/><script>1 &lt; 2</script></html>")
    >>> e.tail = "\n"
    >>> serialize(e)
    '<html><link /><script>1 &lt; 2</script></html>\n'
    >>> serialize(e, method=None)
    '<html><link /><script>1 &lt; 2</script></html>\n'
    >>> serialize(e, method="xml")
    '<html><link /><script>1 &lt; 2</script></html>\n'
    >>> serialize(e, method="html")
    '<html><link><script>1 < 2</script></html>\n'
    >>> serialize(e, method="text")
    '1 < 2\n'
    """

def iterators():
    """
    Test iterators.

    >>> e = ET.XML("<html><body>this is a <i>paragraph</i>.</body>..</html>")
    >>> summarize_list(e.iter())
    ['html', 'body', 'i']
    >>> summarize_list(e.find("body").iter())
    ['body', 'i']
    >>> summarize(next(e.iter()))
    'html'
    >>> "".join(e.itertext())
    'this is a paragraph...'
    >>> "".join(e.find("body").itertext())
    'this is a paragraph.'
    >>> next(e.itertext())
    'this is a '

    Method iterparse should return an iterator. See bug 6472.

    >>> sourcefile = serialize(e, to_string=False)
    >>> next(ET.iterparse(sourcefile))  # doctest: +ELLIPSIS
    ('end', <Element 'i' at 0x...>)

    >>> tree = ET.ElementTree(None)
    >>> tree.iter()
    Traceback (most recent call last):
    AttributeError: 'NoneType' object has no attribute 'iter'
    """

ENTITY_XML = """\
<!DOCTYPE points [
<!ENTITY % user-entities SYSTEM 'user-entities.xml'>
%user-entities;
]>
<document>&entity;</document>
"""

def entity():
    """
    Test entity handling.

    1) good entities

    >>> e = ET.XML("<document title='&#x8230;'>test</document>")
    >>> serialize(e, encoding="us-ascii")
    b'<document title="&#33328;">test</document>'
    >>> serialize(e)
    '<document title="\u8230">test</document>'

    2) bad entities

    >>> normalize_exception(ET.XML, "<document>&entity;</document>")
    Traceback (most recent call last):
    ParseError: undefined entity: line 1, column 10

    >>> normalize_exception(ET.XML, ENTITY_XML)
    Traceback (most recent call last):
    ParseError: undefined entity &entity;: line 5, column 10

    3) custom entity

    >>> parser = ET.XMLParser()
    >>> parser.entity["entity"] = "text"
    >>> parser.feed(ENTITY_XML)
    >>> root = parser.close()
    >>> serialize(root)
    '<document>text</document>'
    """

def namespace():
    """
    Test namespace issues.

    1) xml namespace

    >>> elem = ET.XML("<tag xml:lang='en' />")
    >>> serialize(elem) # 1.1
    '<tag xml:lang="en" />'

    2) other "well-known" namespaces

    >>> elem = ET.XML("<rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#' />")
    >>> serialize(elem) # 2.1
    '<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" />'

    >>> elem = ET.XML("<html:html xmlns:html='http://www.w3.org/1999/xhtml' />")
    >>> serialize(elem) # 2.2
    '<html:html xmlns:html="http://www.w3.org/1999/xhtml" />'

    >>> elem = ET.XML("<soap:Envelope xmlns:soap='http://schemas.xmlsoap.org/soap/envelope' />")
    >>> serialize(elem) # 2.3
    '<ns0:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope" />'

    3) unknown namespaces
    >>> elem = ET.XML(SAMPLE_XML_NS)
    >>> print(serialize(elem))
    <ns0:body xmlns:ns0="http://effbot.org/ns">
      <ns0:tag>text</ns0:tag>
      <ns0:tag />
      <ns0:section>
        <ns0:tag>subtext</ns0:tag>
      </ns0:section>
    </ns0:body>
    """

def qname():
    """
    Test QName handling.

    1) decorated tags

    >>> elem = ET.Element("{uri}tag")
    >>> serialize(elem) # 1.1
    '<ns0:tag xmlns:ns0="uri" />'
    >>> elem = ET.Element(ET.QName("{uri}tag"))
    >>> serialize(elem) # 1.2
    '<ns0:tag xmlns:ns0="uri" />'
    >>> elem = ET.Element(ET.QName("uri", "tag"))
    >>> serialize(elem) # 1.3
    '<ns0:tag xmlns:ns0="uri" />'
    >>> elem = ET.Element(ET.QName("uri", "tag"))
    >>> subelem = ET.SubElement(elem, ET.QName("uri", "tag1"))
    >>> subelem = ET.SubElement(elem, ET.QName("uri", "tag2"))
    >>> serialize(elem) # 1.4
    '<ns0:tag xmlns:ns0="uri"><ns0:tag1 /><ns0:tag2 /></ns0:tag>'

    2) decorated attributes

    >>> elem.clear()
    >>> elem.attrib["{uri}key"] = "value"
    >>> serialize(elem) # 2.1
    '<ns0:tag xmlns:ns0="uri" ns0:key="value" />'

    >>> elem.clear()
    >>> elem.attrib[ET.QName("{uri}key")] = "value"
    >>> serialize(elem) # 2.2
    '<ns0:tag xmlns:ns0="uri" ns0:key="value" />'

    3) decorated values are not converted by default, but the
       QName wrapper can be used for values

    >>> elem.clear()
    >>> elem.attrib["{uri}key"] = "{uri}value"
    >>> serialize(elem) # 3.1
    '<ns0:tag xmlns:ns0="uri" ns0:key="{uri}value" />'

    >>> elem.clear()
    >>> elem.attrib["{uri}key"] = ET.QName("{uri}value")
    >>> serialize(elem) # 3.2
    '<ns0:tag xmlns:ns0="uri" ns0:key="ns0:value" />'

    >>> elem.clear()
    >>> subelem = ET.Element("tag")
    >>> subelem.attrib["{uri1}key"] = ET.QName("{uri2}value")
    >>> elem.append(subelem)
    >>> elem.append(subelem)
    >>> serialize(elem) # 3.3
    '<ns0:tag xmlns:ns0="uri" xmlns:ns1="uri1" xmlns:ns2="uri2"><tag ns1:key="ns2:value" /><tag ns1:key="ns2:value" /></ns0:tag>'

    4) Direct QName tests

    >>> str(ET.QName('ns', 'tag'))
    '{ns}tag'
    >>> str(ET.QName('{ns}tag'))
    '{ns}tag'
    >>> q1 = ET.QName('ns', 'tag')
    >>> q2 = ET.QName('ns', 'tag')
    >>> q1 == q2
    True
    >>> q2 = ET.QName('ns', 'other-tag')
    >>> q1 == q2
    False
    >>> q1 == 'ns:tag'
    False
    >>> q1 == '{ns}tag'
    True
    """

def doctype_public():
    """
    Test PUBLIC doctype.

    >>> elem = ET.XML('<!DOCTYPE html PUBLIC'
    ...   ' "-//W3C//DTD XHTML 1.0 Transitional//EN"'
    ...   ' "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">'
    ...   '<html>text</html>')

    """

def xpath_tokenizer(p):
    """
    Test the XPath tokenizer.

    >>> # tests from the xml specification
    >>> xpath_tokenizer("*")
    ['*']
    >>> xpath_tokenizer("text()")
    ['text', '()']
    >>> xpath_tokenizer("@name")
    ['@', 'name']
    >>> xpath_tokenizer("@*")
    ['@', '*']
    >>> xpath_tokenizer("para[1]")
    ['para', '[', '1', ']']
    >>> xpath_tokenizer("para[last()]")
    ['para', '[', 'last', '()', ']']
    >>> xpath_tokenizer("*/para")
    ['*', '/', 'para']
    >>> xpath_tokenizer("/doc/chapter[5]/section[2]")
    ['/', 'doc', '/', 'chapter', '[', '5', ']', '/', 'section', '[', '2', ']']
    >>> xpath_tokenizer("chapter//para")
    ['chapter', '//', 'para']
    >>> xpath_tokenizer("//para")
    ['//', 'para']
    >>> xpath_tokenizer("//olist/item")
    ['//', 'olist', '/', 'item']
    >>> xpath_tokenizer(".")
    ['.']
    >>> xpath_tokenizer(".//para")
    ['.', '//', 'para']
    >>> xpath_tokenizer("..")
    ['..']
    >>> xpath_tokenizer("../@lang")
    ['..', '/', '@', 'lang']
    >>> xpath_tokenizer("chapter[title]")
    ['chapter', '[', 'title', ']']
    >>> xpath_tokenizer("employee[@secretary and @assistant]")
    ['employee', '[', '@', 'secretary', '', 'and', '', '@', 'assistant', ']']

    >>> # additional tests
    >>> xpath_tokenizer("{http://spam}egg")
    ['{http://spam}egg']
    >>> xpath_tokenizer("./spam.egg")
    ['.', '/', 'spam.egg']
    >>> xpath_tokenizer(".//{http://spam}egg")
    ['.', '//', '{http://spam}egg']
    """
    from xml.etree import ElementPath
    out = []
    for op, tag in ElementPath.xpath_tokenizer(p):
        out.append(op or tag)
    return out

def processinginstruction():
    """
    Test ProcessingInstruction directly

    >>> ET.tostring(ET.ProcessingInstruction('test', 'instruction'))
    b'<?test instruction?>'
    >>> ET.tostring(ET.PI('test', 'instruction'))
    b'<?test instruction?>'

    Issue #2746

    >>> ET.tostring(ET.PI('test', '<testing&>'))
    b'<?test <testing&>?>'
    >>> ET.tostring(ET.PI('test', '<testing&>\xe3'), 'latin-1')
    b"<?xml version='1.0' encoding='latin-1'?>\\n<?test <testing&>\\xe3?>"
    """

#
# xinclude tests (samples from appendix C of the xinclude specification)

XINCLUDE = {}

XINCLUDE["C1.xml"] = """\
<?xml version='1.0'?>
<document xmlns:xi="http://www.w3.org/2001/XInclude">
  <p>120 Mz is adequate for an average home user.</p>
  <xi:include href="disclaimer.xml"/>
</document>
"""

XINCLUDE["disclaimer.xml"] = """\
<?xml version='1.0'?>
<disclaimer>
  <p>The opinions represented herein represent those of the individual
  and should not be interpreted as official policy endorsed by this
  organization.</p>
</disclaimer>
"""

XINCLUDE["C2.xml"] = """\
<?xml version='1.0'?>
<document xmlns:xi="http://www.w3.org/2001/XInclude">
  <p>This document has been accessed
  <xi:include href="count.txt" parse="text"/> times.</p>
</document>
"""

XINCLUDE["count.txt"] = "324387"

XINCLUDE["C2b.xml"] = """\
<?xml version='1.0'?>
<document xmlns:xi="http://www.w3.org/2001/XInclude">
  <p>This document has been <em>accessed</em>
  <xi:include href="count.txt" parse="text"/> times.</p>
</document>
"""

XINCLUDE["C3.xml"] = """\
<?xml version='1.0'?>
<document xmlns:xi="http://www.w3.org/2001/XInclude">
  <p>The following is the source of the "data.xml" resource:</p>
  <example><xi:include href="data.xml" parse="text"/></example>
</document>
"""

XINCLUDE["data.xml"] = """\
<?xml version='1.0'?>
<data>
  <item><![CDATA[Brooks & Shields]]></item>
</data>
"""

XINCLUDE["C5.xml"] = """\
<?xml version='1.0'?>
<div xmlns:xi="http://www.w3.org/2001/XInclude">
  <xi:include href="example.txt" parse="text">
    <xi:fallback>
      <xi:include href="fallback-example.txt" parse="text">
        <xi:fallback><a href="mailto:bob@example.org">Report error</a></xi:fallback>
      </xi:include>
    </xi:fallback>
  </xi:include>
</div>
"""

XINCLUDE["default.xml"] = """\
<?xml version='1.0'?>
<document xmlns:xi="http://www.w3.org/2001/XInclude">
  <p>Example.</p>
  <xi:include href="{}"/>
</document>
""".format(html.escape(SIMPLE_XMLFILE, True))

def xinclude_loader(href, parse="xml", encoding=None):
    try:
        data = XINCLUDE[href]
    except KeyError:
        raise OSError("resource not found")
    if parse == "xml":
        from xml.etree.ElementTree import XML
        return XML(data)
    return data

def xinclude():
    r"""
    Basic inclusion example (XInclude C.1)

    >>> from xml.etree import ElementInclude

    >>> document = xinclude_loader("C1.xml")
    >>> ElementInclude.include(document, xinclude_loader)
    >>> print(serialize(document)) # C1
    <document>
      <p>120 Mz is adequate for an average home user.</p>
      <disclaimer>
      <p>The opinions represented herein represent those of the individual
      and should not be interpreted as official policy endorsed by this
      organization.</p>
    </disclaimer>
    </document>

    Textual inclusion example (XInclude C.2)

    >>> document = xinclude_loader("C2.xml")
    >>> ElementInclude.include(document, xinclude_loader)
    >>> print(serialize(document)) # C2
    <document>
      <p>This document has been accessed
      324387 times.</p>
    </document>

    Textual inclusion after sibling element (based on modified XInclude C.2)

    >>> document = xinclude_loader("C2b.xml")
    >>> ElementInclude.include(document, xinclude_loader)
    >>> print(serialize(document)) # C2b
    <document>
      <p>This document has been <em>accessed</em>
      324387 times.</p>
    </document>

    Textual inclusion of XML example (XInclude C.3)

    >>> document = xinclude_loader("C3.xml")
    >>> ElementInclude.include(document, xinclude_loader)
    >>> print(serialize(document)) # C3
    <document>
      <p>The following is the source of the "data.xml" resource:</p>
      <example>&lt;?xml version='1.0'?&gt;
    &lt;data&gt;
      &lt;item&gt;&lt;![CDATA[Brooks &amp; Shields]]&gt;&lt;/item&gt;
    &lt;/data&gt;
    </example>
    </document>

    Fallback example (XInclude C.5)
    Note! Fallback support is not yet implemented

    >>> document = xinclude_loader("C5.xml")
    >>> ElementInclude.include(document, xinclude_loader)
    Traceback (most recent call last):
    OSError: resource not found
    >>> # print(serialize(document)) # C5
    """

def xinclude_default():
    """
    >>> from xml.etree import ElementInclude

    >>> document = xinclude_loader("default.xml")
    >>> ElementInclude.include(document)
    >>> print(serialize(document)) # default
    <document>
      <p>Example.</p>
      <root>
       <element key="value">text</element>
       <element>text</element>tail
       <empty-element />
    </root>
    </document>
    """

#
# badly formatted xi:include tags

XINCLUDE_BAD = {}

XINCLUDE_BAD["B1.xml"] = """\
<?xml version='1.0'?>
<document xmlns:xi="http://www.w3.org/2001/XInclude">
  <p>120 Mz is adequate for an average home user.</p>
  <xi:include href="disclaimer.xml" parse="BAD_TYPE"/>
</document>
"""

XINCLUDE_BAD["B2.xml"] = """\
<?xml version='1.0'?>
<div xmlns:xi="http://www.w3.org/2001/XInclude">
    <xi:fallback></xi:fallback>
</div>
"""

def xinclude_failures():
    r"""
    Test failure to locate included XML file.

    >>> from xml.etree import ElementInclude

    >>> def none_loader(href, parser, encoding=None):
    ...     return None

    >>> document = ET.XML(XINCLUDE["C1.xml"])
    >>> ElementInclude.include(document, loader=none_loader)
    Traceback (most recent call last):
    xml.etree.ElementInclude.FatalIncludeError: cannot load 'disclaimer.xml' as 'xml'

    Test failure to locate included text file.

    >>> document = ET.XML(XINCLUDE["C2.xml"])
    >>> ElementInclude.include(document, loader=none_loader)
    Traceback (most recent call last):
    xml.etree.ElementInclude.FatalIncludeError: cannot load 'count.txt' as 'text'

    Test bad parse type.

    >>> document = ET.XML(XINCLUDE_BAD["B1.xml"])
    >>> ElementInclude.include(document, loader=none_loader)
    Traceback (most recent call last):
    xml.etree.ElementInclude.FatalIncludeError: unknown parse type in xi:include tag ('BAD_TYPE')

    Test xi:fallback outside xi:include.

    >>> document = ET.XML(XINCLUDE_BAD["B2.xml"])
    >>> ElementInclude.include(document, loader=none_loader)
    Traceback (most recent call last):
    xml.etree.ElementInclude.FatalIncludeError: xi:fallback tag must be child of xi:include ('{http://www.w3.org/2001/XInclude}fallback')
    """

# --------------------------------------------------------------------
# reported bugs

def bug_xmltoolkit21():
    """

    marshaller gives obscure errors for non-string values

    >>> elem = ET.Element(123)
    >>> serialize(elem) # tag
    Traceback (most recent call last):
    TypeError: cannot serialize 123 (type int)
    >>> elem = ET.Element("elem")
    >>> elem.text = 123
    >>> serialize(elem) # text
    Traceback (most recent call last):
    TypeError: cannot serialize 123 (type int)
    >>> elem = ET.Element("elem")
    >>> elem.tail = 123
    >>> serialize(elem) # tail
    Traceback (most recent call last):
    TypeError: cannot serialize 123 (type int)
    >>> elem = ET.Element("elem")
    >>> elem.set(123, "123")
    >>> serialize(elem) # attribute key
    Traceback (most recent call last):
    TypeError: cannot serialize 123 (type int)
    >>> elem = ET.Element("elem")
    >>> elem.set("123", 123)
    >>> serialize(elem) # attribute value
    Traceback (most recent call last):
    TypeError: cannot serialize 123 (type int)

    """

def bug_xmltoolkit25():
    """

    typo in ElementTree.findtext

    >>> elem = ET.XML(SAMPLE_XML)
    >>> tree = ET.ElementTree(elem)
    >>> tree.findtext("tag")
    'text'
    >>> tree.findtext("section/tag")
    'subtext'

    """

def bug_xmltoolkit28():
    """

    .//tag causes exceptions

    >>> tree = ET.XML("<doc><table><tbody/></table></doc>")
    >>> summarize_list(tree.findall(".//thead"))
    []
    >>> summarize_list(tree.findall(".//tbody"))
    ['tbody']

    """

def bug_xmltoolkitX1():
    """

    dump() doesn't flush the output buffer

    >>> tree = ET.XML("<doc><table><tbody/></table></doc>")
    >>> ET.dump(tree); print("tail")
    <doc><table><tbody /></table></doc>
    tail

    """

def bug_xmltoolkit39():
    """

    non-ascii element and attribute names doesn't work

    >>> tree = ET.XML(b"<?xml version='1.0' encoding='iso-8859-1'?><t\\xe4g />")
    >>> ET.tostring(tree, "utf-8")
    b'<t\\xc3\\xa4g />'

    >>> tree = ET.XML(b"<?xml version='1.0' encoding='iso-8859-1'?><tag \\xe4ttr='v&#228;lue' />")
    >>> tree.attrib
    {'\\xe4ttr': 'v\\xe4lue'}
    >>> ET.tostring(tree, "utf-8")
    b'<tag \\xc3\\xa4ttr="v\\xc3\\xa4lue" />'

    >>> tree = ET.XML(b"<?xml version='1.0' encoding='iso-8859-1'?><t\\xe4g>text</t\\xe4g>")
    >>> ET.tostring(tree, "utf-8")
    b'<t\\xc3\\xa4g>text</t\\xc3\\xa4g>'

    >>> tree = ET.Element("t\u00e4g")
    >>> ET.tostring(tree, "utf-8")
    b'<t\\xc3\\xa4g />'

    >>> tree = ET.Element("tag")
    >>> tree.set("\u00e4ttr", "v\u00e4lue")
    >>> ET.tostring(tree, "utf-8")
    b'<tag \\xc3\\xa4ttr="v\\xc3\\xa4lue" />'

    """

def bug_xmltoolkit54():
    """

    problems handling internally defined entities

    >>> e = ET.XML("<!DOCTYPE doc [<!ENTITY ldots '&#x8230;'>]><doc>&ldots;</doc>")
    >>> serialize(e, encoding="us-ascii")
    b'<doc>&#33328;</doc>'
    >>> serialize(e)
    '<doc>\u8230</doc>'

    """

def bug_xmltoolkit55():
    """

    make sure we're reporting the first error, not the last

    >>> normalize_exception(ET.XML, b"<!DOCTYPE doc SYSTEM 'doc.dtd'><doc>&ldots;&ndots;&rdots;</doc>")
    Traceback (most recent call last):
    ParseError: undefined entity &ldots;: line 1, column 36

    """

class ExceptionFile:
    def read(self, x):
        raise OSError

def xmltoolkit60():
    """

    Handle crash in stream source.
    >>> tree = ET.parse(ExceptionFile())
    Traceback (most recent call last):
    OSError

    """

XMLTOOLKIT62_DOC = """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE patent-application-publication SYSTEM "pap-v15-2001-01-31.dtd" []>
<patent-application-publication>
<subdoc-abstract>
<paragraph id="A-0001" lvl="0">A new cultivar of Begonia plant named &lsquo;BCT9801BEG&rsquo;.</paragraph>
</subdoc-abstract>
</patent-application-publication>"""


def xmltoolkit62():
    """

    Don't crash when using custom entities.

    >>> xmltoolkit62()
    'A new cultivar of Begonia plant named \u2018BCT9801BEG\u2019.'

    """
    ENTITIES = {'rsquo': '\u2019', 'lsquo': '\u2018'}
    parser = ET.XMLTreeBuilder()
    parser.entity.update(ENTITIES)
    parser.feed(XMLTOOLKIT62_DOC)
    t = parser.close()
    return t.find('.//paragraph').text

def xmltoolkit63():
    """

    Check reference leak.
    >>> xmltoolkit63()
    >>> count = sys.getrefcount(None)
    >>> for i in range(1000):
    ...     xmltoolkit63()
    >>> sys.getrefcount(None) - count
    0

    """
    tree = ET.TreeBuilder()
    tree.start("tag", {})
    tree.data("text")
    tree.end("tag")

# --------------------------------------------------------------------


def bug_200708_newline():
    r"""

    Preserve newlines in attributes.

    >>> e = ET.Element('SomeTag', text="def _f():\n  return 3\n")
    >>> ET.tostring(e)
    b'<SomeTag text="def _f():&#10;  return 3&#10;" />'
    >>> ET.XML(ET.tostring(e)).get("text")
    'def _f():\n  return 3\n'
    >>> ET.tostring(ET.XML(ET.tostring(e)))
    b'<SomeTag text="def _f():&#10;  return 3&#10;" />'

    """

def bug_200708_close():
    """

    Test default builder.
    >>> parser = ET.XMLParser() # default
    >>> parser.feed("<element>some text</element>")
    >>> summarize(parser.close())
    'element'

    Test custom builder.
    >>> class EchoTarget:
    ...     def close(self):
    ...         return ET.Element("element") # simulate root
    >>> parser = ET.XMLParser(EchoTarget())
    >>> parser.feed("<element>some text</element>")
    >>> summarize(parser.close())
    'element'

    """

def bug_200709_default_namespace():
    """

    >>> e = ET.Element("{default}elem")
    >>> s = ET.SubElement(e, "{default}elem")
    >>> serialize(e, default_namespace="default") # 1
    '<elem xmlns="default"><elem /></elem>'

    >>> e = ET.Element("{default}elem")
    >>> s = ET.SubElement(e, "{default}elem")
    >>> s = ET.SubElement(e, "{not-default}elem")
    >>> serialize(e, default_namespace="default") # 2
    '<elem xmlns="default" xmlns:ns1="not-default"><elem /><ns1:elem /></elem>'

    >>> e = ET.Element("{default}elem")
    >>> s = ET.SubElement(e, "{default}elem")
    >>> s = ET.SubElement(e, "elem") # unprefixed name
    >>> serialize(e, default_namespace="default") # 3
    Traceback (most recent call last):
    ValueError: cannot use non-qualified names with default_namespace option

    """

def bug_200709_register_namespace():
    """

    >>> ET.tostring(ET.Element("{http://namespace.invalid/does/not/exist/}title"))
    b'<ns0:title xmlns:ns0="http://namespace.invalid/does/not/exist/" />'
    >>> ET.register_namespace("foo", "http://namespace.invalid/does/not/exist/")
    >>> ET.tostring(ET.Element("{http://namespace.invalid/does/not/exist/}title"))
    b'<foo:title xmlns:foo="http://namespace.invalid/does/not/exist/" />'

    And the Dublin Core namespace is in the default list:

    >>> ET.tostring(ET.Element("{http://purl.org/dc/elements/1.1/}title"))
    b'<dc:title xmlns:dc="http://purl.org/dc/elements/1.1/" />'

    """

def bug_200709_element_comment():
    """

    Not sure if this can be fixed, really (since the serializer needs
    ET.Comment, not cET.comment).

    >>> a = ET.Element('a')
    >>> a.append(ET.Comment('foo'))
    >>> a[0].tag == ET.Comment
    True

    >>> a = ET.Element('a')
    >>> a.append(ET.PI('foo'))
    >>> a[0].tag == ET.PI
    True

    """

def bug_200709_element_insert():
    """

    >>> a = ET.Element('a')
    >>> b = ET.SubElement(a, 'b')
    >>> c = ET.SubElement(a, 'c')
    >>> d = ET.Element('d')
    >>> a.insert(0, d)
    >>> summarize_list(a)
    ['d', 'b', 'c']
    >>> a.insert(-1, d)
    >>> summarize_list(a)
    ['d', 'b', 'd', 'c']

    """

def bug_200709_iter_comment():
    """

    >>> a = ET.Element('a')
    >>> b = ET.SubElement(a, 'b')
    >>> comment_b = ET.Comment("TEST-b")
    >>> b.append(comment_b)
    >>> summarize_list(a.iter(ET.Comment))
    ['<Comment>']

    """

# --------------------------------------------------------------------
# reported on bugs.python.org

def bug_1534630():
    """

    >>> bob = ET.TreeBuilder()
    >>> e = bob.data("data")
    >>> e = bob.start("tag", {})
    >>> e = bob.end("tag")
    >>> e = bob.close()
    >>> serialize(e)
    '<tag />'

    """

def check_issue6233():
    """

    >>> e = ET.XML(b"<?xml version='1.0' encoding='utf-8'?><body>t\\xc3\\xa3g</body>")
    >>> ET.tostring(e, 'ascii')
    b"<?xml version='1.0' encoding='ascii'?>\\n<body>t&#227;g</body>"
    >>> e = ET.XML(b"<?xml version='1.0' encoding='iso-8859-1'?><body>t\\xe3g</body>")
    >>> ET.tostring(e, 'ascii')
    b"<?xml version='1.0' encoding='ascii'?>\\n<body>t&#227;g</body>"

    """

def check_issue3151():
    """

    >>> e = ET.XML('<prefix:localname xmlns:prefix="${stuff}"/>')
    >>> e.tag
    '{${stuff}}localname'
    >>> t = ET.ElementTree(e)
    >>> ET.tostring(e)
    b'<ns0:localname xmlns:ns0="${stuff}" />'

    """

def check_issue6565():
    """

    >>> elem = ET.XML("<body><tag/></body>")
    >>> summarize_list(elem)
    ['tag']
    >>> newelem = ET.XML(SAMPLE_XML)
    >>> elem[:] = newelem[:]
    >>> summarize_list(elem)
    ['tag', 'tag', 'section']

    """

def check_issue10777():
    """
    Registering a namespace twice caused a "dictionary changed size during
    iteration" bug.

    >>> ET.register_namespace('test10777', 'http://myuri/')
    >>> ET.register_namespace('test10777', 'http://myuri/')
    """

# --------------------------------------------------------------------


class BasicElementTest(unittest.TestCase):
    def test_augmentation_type_errors(self):
        e = ET.Element('joe')
        self.assertRaises(TypeError, e.append, 'b')
        self.assertRaises(TypeError, e.extend, [ET.Element('bar'), 'foo'])
        self.assertRaises(TypeError, e.insert, 0, 'foo')

    def test_cyclic_gc(self):
        class Dummy:
            pass

        # Test the shortest cycle: d->element->d
        d = Dummy()
        d.dummyref = ET.Element('joe', attr=d)
        wref = weakref.ref(d)
        del d
        gc_collect()
        self.assertIsNone(wref())

        # A longer cycle: d->e->e2->d
        e = ET.Element('joe')
        d = Dummy()
        d.dummyref = e
        wref = weakref.ref(d)
        e2 = ET.SubElement(e, 'foo', attr=d)
        del d, e, e2
        gc_collect()
        self.assertIsNone(wref())

        # A cycle between Element objects as children of one another
        # e1->e2->e3->e1
        e1 = ET.Element('e1')
        e2 = ET.Element('e2')
        e3 = ET.Element('e3')
        e1.append(e2)
        e2.append(e2)
        e3.append(e1)
        wref = weakref.ref(e1)
        del e1, e2, e3
        gc_collect()
        self.assertIsNone(wref())

    def test_weakref(self):
        flag = False
        def wref_cb(w):
            nonlocal flag
            flag = True
        e = ET.Element('e')
        wref = weakref.ref(e, wref_cb)
        self.assertEqual(wref().tag, 'e')
        del e
        self.assertEqual(flag, True)
        self.assertEqual(wref(), None)


class ElementTreeTest(unittest.TestCase):
    def test_istype(self):
        self.assertIsInstance(ET.ParseError, type)
        self.assertIsInstance(ET.QName, type)
        self.assertIsInstance(ET.ElementTree, type)
        self.assertIsInstance(ET.Element, type)
        # XXX issue 14128 with C ElementTree
        # self.assertIsInstance(ET.TreeBuilder, type)
        # self.assertIsInstance(ET.XMLParser, type)

    def test_Element_subclass_trivial(self):
        class MyElement(ET.Element):
            pass

        mye = MyElement('foo')
        self.assertIsInstance(mye, ET.Element)
        self.assertIsInstance(mye, MyElement)
        self.assertEqual(mye.tag, 'foo')

        # test that attribute assignment works (issue 14849)
        mye.text = "joe"
        self.assertEqual(mye.text, "joe")

    def test_Element_subclass_constructor(self):
        class MyElement(ET.Element):
            def __init__(self, tag, attrib={}, **extra):
                super(MyElement, self).__init__(tag + '__', attrib, **extra)

        mye = MyElement('foo', {'a': 1, 'b': 2}, c=3, d=4)
        self.assertEqual(mye.tag, 'foo__')
        self.assertEqual(sorted(mye.items()),
            [('a', 1), ('b', 2), ('c', 3), ('d', 4)])

    def test_Element_subclass_new_method(self):
        class MyElement(ET.Element):
            def newmethod(self):
                return self.tag

        mye = MyElement('joe')
        self.assertEqual(mye.newmethod(), 'joe')


class TreeBuilderTest(unittest.TestCase):
    sample1 = ('<!DOCTYPE html PUBLIC'
        ' "-//W3C//DTD XHTML 1.0 Transitional//EN"'
        ' "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">'
        '<html>text</html>')

    def test_dummy_builder(self):
        class BaseDummyBuilder:
            def close(self):
                return 42

        class DummyBuilder(BaseDummyBuilder):
            data = start = end = lambda *a: None

        parser = ET.XMLParser(target=DummyBuilder())
        parser.feed(self.sample1)
        self.assertEqual(parser.close(), 42)

        parser = ET.XMLParser(target=BaseDummyBuilder())
        parser.feed(self.sample1)
        self.assertEqual(parser.close(), 42)

        parser = ET.XMLParser(target=object())
        parser.feed(self.sample1)
        self.assertIsNone(parser.close())

    # XXX in _elementtree, the constructor of TreeBuilder expects no
    # arguments
    @unittest.expectedFailure
    def test_element_factory(self):
        tb = ET.TreeBuilder(element_factory=lambda: ET.Element())

    @unittest.expectedFailure   # XXX issue 14007 with C ElementTree
    def test_doctype(self):
        class DoctypeParser:
            _doctype = None

            def doctype(self, name, pubid, system):
                self._doctype = (name, pubid, system)

            def close(self):
                return self._doctype

        parser = ET.XMLParser(target=DoctypeParser())
        parser.feed(self.sample1)

        self.assertEqual(parser.close(),
            ('html', '-//W3C//DTD XHTML 1.0 Transitional//EN',
             'http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'))


class NoAcceleratorTest(unittest.TestCase):
    # Test that the C accelerator was not imported for pyET
    def test_correct_import_pyET(self):
        self.assertEqual(pyET.Element.__module__, 'xml.etree.ElementTree')
        self.assertEqual(pyET.SubElement.__module__, 'xml.etree.ElementTree')


class ElementSlicingTest(unittest.TestCase):
    def _elem_tags(self, elemlist):
        return [e.tag for e in elemlist]

    def _subelem_tags(self, elem):
        return self._elem_tags(list(elem))

    def _make_elem_with_children(self, numchildren):
        """Create an Element with a tag 'a', with the given amount of children
           named 'a0', 'a1' ... and so on.

        """
        e = ET.Element('a')
        for i in range(numchildren):
            ET.SubElement(e, 'a%s' % i)
        return e

    def test_getslice_single_index(self):
        e = self._make_elem_with_children(10)

        self.assertEqual(e[1].tag, 'a1')
        self.assertEqual(e[-2].tag, 'a8')

        self.assertRaises(IndexError, lambda: e[12])

    def test_getslice_range(self):
        e = self._make_elem_with_children(6)

        self.assertEqual(self._elem_tags(e[3:]), ['a3', 'a4', 'a5'])
        self.assertEqual(self._elem_tags(e[3:6]), ['a3', 'a4', 'a5'])
        self.assertEqual(self._elem_tags(e[3:16]), ['a3', 'a4', 'a5'])
        self.assertEqual(self._elem_tags(e[3:5]), ['a3', 'a4'])
        self.assertEqual(self._elem_tags(e[3:-1]), ['a3', 'a4'])
        self.assertEqual(self._elem_tags(e[:2]), ['a0', 'a1'])

    def test_getslice_steps(self):
        e = self._make_elem_with_children(10)

        self.assertEqual(self._elem_tags(e[8:10:1]), ['a8', 'a9'])
        self.assertEqual(self._elem_tags(e[::3]), ['a0', 'a3', 'a6', 'a9'])
        self.assertEqual(self._elem_tags(e[::8]), ['a0', 'a8'])
        self.assertEqual(self._elem_tags(e[1::8]), ['a1', 'a9'])

    def test_getslice_negative_steps(self):
        e = self._make_elem_with_children(4)

        self.assertEqual(self._elem_tags(e[::-1]), ['a3', 'a2', 'a1', 'a0'])
        self.assertEqual(self._elem_tags(e[::-2]), ['a3', 'a1'])

    def test_delslice(self):
        e = self._make_elem_with_children(4)
        del e[0:2]
        self.assertEqual(self._subelem_tags(e), ['a2', 'a3'])

        e = self._make_elem_with_children(4)
        del e[0:]
        self.assertEqual(self._subelem_tags(e), [])

        e = self._make_elem_with_children(4)
        del e[::-1]
        self.assertEqual(self._subelem_tags(e), [])

        e = self._make_elem_with_children(4)
        del e[::-2]
        self.assertEqual(self._subelem_tags(e), ['a0', 'a2'])

        e = self._make_elem_with_children(4)
        del e[1::2]
        self.assertEqual(self._subelem_tags(e), ['a0', 'a2'])

        e = self._make_elem_with_children(2)
        del e[::2]
        self.assertEqual(self._subelem_tags(e), ['a1'])


class StringIOTest(unittest.TestCase):
    def test_read_from_stringio(self):
        tree = ET.ElementTree()
        stream = io.StringIO()
        stream.write('''<?xml version="1.0"?><site></site>''')
        stream.seek(0)
        tree.parse(stream)

        self.assertEqual(tree.getroot().tag, 'site')


class ParseErrorTest(unittest.TestCase):
    def test_subclass(self):
        self.assertIsInstance(ET.ParseError(), SyntaxError)

    def _get_error(self, s):
        try:
            ET.fromstring(s)
        except ET.ParseError as e:
            return e

    def test_error_position(self):
        self.assertEqual(self._get_error('foo').position, (1, 0))
        self.assertEqual(self._get_error('<tag>&foo;</tag>').position, (1, 5))
        self.assertEqual(self._get_error('foobar<').position, (1, 6))

    def test_error_code(self):
        import xml.parsers.expat.errors as ERRORS
        self.assertEqual(self._get_error('foo').code,
                ERRORS.codes[ERRORS.XML_ERROR_SYNTAX])


# --------------------------------------------------------------------


class CleanContext(object):
    """Provide default namespace mapping and path cache."""
    checkwarnings = None

    def __init__(self, quiet=False):
        if sys.flags.optimize >= 2:
            # under -OO, doctests cannot be run and therefore not all warnings
            # will be emitted
            quiet = True
        deprecations = (
            # Search behaviour is broken if search path starts with "/".
            ("This search is broken in 1.3 and earlier, and will be fixed "
             "in a future version.  If you rely on the current behaviour, "
             "change it to '.+'", FutureWarning),
            # Element.getchildren() and Element.getiterator() are deprecated.
            ("This method will be removed in future versions.  "
             "Use .+ instead.", DeprecationWarning),
            ("This method will be removed in future versions.  "
             "Use .+ instead.", PendingDeprecationWarning))
        self.checkwarnings = support.check_warnings(*deprecations, quiet=quiet)

    def __enter__(self):
        from xml.etree import ElementPath
        self._nsmap = ET.register_namespace._namespace_map
        # Copy the default namespace mapping
        self._nsmap_copy = self._nsmap.copy()
        # Copy the path cache (should be empty)
        self._path_cache = ElementPath._cache
        ElementPath._cache = self._path_cache.copy()
        self.checkwarnings.__enter__()

    def __exit__(self, *args):
        from xml.etree import ElementPath
        # Restore mapping and path cache
        self._nsmap.clear()
        self._nsmap.update(self._nsmap_copy)
        ElementPath._cache = self._path_cache
        self.checkwarnings.__exit__(*args)


def test_main(module=pyET):
    from test import test_xml_etree

    # The same doctests are used for both the Python and the C implementations
    test_xml_etree.ET = module

    test_classes = [
        ElementSlicingTest,
        BasicElementTest,
        StringIOTest,
        ParseErrorTest,
        ElementTreeTest,
        TreeBuilderTest]
    if module is pyET:
        # Run the tests specific to the Python implementation
        test_classes += [NoAcceleratorTest]

    support.run_unittest(*test_classes)

    # XXX the C module should give the same warnings as the Python module
    with CleanContext(quiet=(module is not pyET)):
        support.run_doctest(test_xml_etree, verbosity=True)

if __name__ == '__main__':
    test_main()
