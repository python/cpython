# xml.etree test for cElementTree

import doctest
import sys

from test import support
from test.support import precisionbigmemtest, _2G
import unittest

ET = support.import_module('xml.etree.cElementTree')

SAMPLE_XML = """
<body>
  <tag>text</tag>
  <tag />
  <section>
    <tag>subtext</tag>
  </section>
</body>
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

    >>> from xml.etree import cElementTree
    """

def check_method(method):
    if not hasattr(method, '__call__'):
        print(method, "not callable")

def serialize(ET, elem):
    import io
    file = io.StringIO()
    tree = ET.ElementTree(elem)
    tree.write(file)
    return file.getvalue()

def summarize(elem):
    return elem.tag

def summarize_list(seq):
    return list(map(summarize, seq))

def interface():
    """
    Test element tree interface.

    >>> element = ET.Element("tag", key="value")
    >>> tree = ET.ElementTree(element)

    Make sure all standard element methods exist.

    >>> check_method(element.append)
    >>> check_method(element.insert)
    >>> check_method(element.remove)
    >>> check_method(element.getchildren)
    >>> check_method(element.find)
    >>> check_method(element.findall)
    >>> check_method(element.findtext)
    >>> check_method(element.clear)
    >>> check_method(element.get)
    >>> check_method(element.set)
    >>> check_method(element.keys)
    >>> check_method(element.items)
    >>> check_method(element.getiterator)

    Basic method sanity checks.

    >>> serialize(ET, element) # 1
    '<tag key="value" />'
    >>> subelement = ET.Element("subtag")
    >>> element.append(subelement)
    >>> serialize(ET, element) #  2
    '<tag key="value"><subtag /></tag>'
    >>> element.insert(0, subelement)
    >>> serialize(ET, element) # 3
    '<tag key="value"><subtag /><subtag /></tag>'
    >>> element.remove(subelement)
    >>> serialize(ET, element) # 4
    '<tag key="value"><subtag /></tag>'
    >>> element.remove(subelement)
    >>> serialize(ET, element) # 5
    '<tag key="value" />'
    >>> element.remove(subelement)
    Traceback (most recent call last):
    ValueError: list.remove(x): x not in list
    >>> serialize(ET, element) # 6
    '<tag key="value" />'
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
    >>> ET.ElementTree(elem).find("section/tag").tag
    'tag'
    >>> elem.findtext("tag")
    'text'
    >>> elem.findtext("tog")
    >>> elem.findtext("tog", "default")
    'default'
    >>> ET.ElementTree(elem).findtext("tag")
    'text'
    >>> elem.findtext("section/tag")
    'subtext'
    >>> ET.ElementTree(elem).findtext("section/tag")
    'subtext'
    >>> summarize_list(elem.findall("tag"))
    ['tag', 'tag']
    >>> summarize_list(elem.findall("*"))
    ['tag', 'tag', 'section']
    >>> summarize_list(elem.findall(".//tag"))
    ['tag', 'tag', 'tag']
    >>> summarize_list(elem.findall("section/tag"))
    ['tag']
    >>> summarize_list(elem.findall("section//tag"))
    ['tag']
    >>> summarize_list(elem.findall("section/*"))
    ['tag']
    >>> summarize_list(elem.findall("section//*"))
    ['tag']
    >>> summarize_list(elem.findall("section/.//*"))
    ['tag']
    >>> summarize_list(elem.findall("*/*"))
    ['tag']
    >>> summarize_list(elem.findall("*//*"))
    ['tag']
    >>> summarize_list(elem.findall("*/tag"))
    ['tag']
    >>> summarize_list(elem.findall("*/./tag"))
    ['tag']
    >>> summarize_list(elem.findall("./tag"))
    ['tag', 'tag']
    >>> summarize_list(elem.findall(".//tag"))
    ['tag', 'tag', 'tag']
    >>> summarize_list(elem.findall("././tag"))
    ['tag', 'tag']
    >>> summarize_list(ET.ElementTree(elem).findall("/tag"))
    ['tag', 'tag']
    >>> summarize_list(ET.ElementTree(elem).findall("./tag"))
    ['tag', 'tag']
    >>> elem = ET.XML(SAMPLE_XML_NS)
    >>> summarize_list(elem.findall("tag"))
    []
    >>> summarize_list(elem.findall("{http://effbot.org/ns}tag"))
    ['{http://effbot.org/ns}tag', '{http://effbot.org/ns}tag']
    >>> summarize_list(elem.findall(".//{http://effbot.org/ns}tag"))
    ['{http://effbot.org/ns}tag', '{http://effbot.org/ns}tag', '{http://effbot.org/ns}tag']
    """

def parseliteral():
    r"""

    >>> element = ET.XML("<html><body>text</body></html>")
    >>> ET.ElementTree(element).write(sys.stdout)
    <html><body>text</body></html>
    >>> element = ET.fromstring("<html><body>text</body></html>")
    >>> ET.ElementTree(element).write(sys.stdout)
    <html><body>text</body></html>
    >>> print(ET.tostring(element))
    <html><body>text</body></html>
    >>> print(repr(ET.tostring(element, "ascii")))
    b"<?xml version='1.0' encoding='ascii'?>\n<html><body>text</body></html>"
    >>> _, ids = ET.XMLID("<html><body>text</body></html>")
    >>> len(ids)
    0
    >>> _, ids = ET.XMLID("<html><body id='body'>text</body></html>")
    >>> len(ids)
    1
    >>> ids["body"].tag
    'body'
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
    ET.XML(
        "<?xml version='1.0' encoding='%s'?><xml />" % encoding
        )

def bug_1534630():
    """
    >>> bob = ET.TreeBuilder()
    >>> e = bob.data("data")
    >>> e = bob.start("tag", {})
    >>> e = bob.end("tag")
    >>> e = bob.close()
    >>> serialize(ET, e)
    '<tag />'
    """

class MiscTests(unittest.TestCase):
    # Issue #8651.
    @support.precisionbigmemtest(size=support._2G + 100, memuse=1)
    def test_length_overflow(self, size):
        if size < support._2G + 100:
            self.skipTest("not enough free memory, need at least 2 GB")
        data = b'x' * size
        parser = ET.XMLParser()
        try:
            self.assertRaises(OverflowError, parser.feed, data)
        finally:
            data = None


def test_main():
    from test import test_xml_etree_c
    support.run_doctest(test_xml_etree_c, verbosity=True)

    support.run_unittest(MiscTests)

if __name__ == '__main__':
    test_main()
