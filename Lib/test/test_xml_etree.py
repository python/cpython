# xml.etree test.  This file contains enough tests to make sure that
# all included components work as they should.  For a more extensive
# test suite, see the selftest script in the ElementTree distribution.

import doctest
import sys

from test import support

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

    >>> from xml.etree import ElementTree
    >>> from xml.etree import ElementInclude
    >>> from xml.etree import ElementPath
    """

def check_method(method):
    if not hasattr(method, '__call__'):
        print(method, "not callable")

def serialize(ET, elem):
    import io
    tree = ET.ElementTree(elem)
    file = io.StringIO()
    tree.write(file)
    return file.getvalue()

def summarize(elem):
    return elem.tag

def summarize_list(seq):
    return list(map(summarize, seq))

def interface():
    """
    Test element tree interface.

    >>> from xml.etree import ElementTree as ET

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

def qname():
    """
    Test QName handling.

    1) decorated tags

    >>> from xml.etree import ElementTree as ET
    >>> elem = ET.Element("{uri}tag")
    >>> serialize(ET, elem) # 1.1
    '<ns0:tag xmlns:ns0="uri" />'
    >>> elem = ET.Element(ET.QName("{uri}tag"))
    >>> serialize(ET, elem) # 1.2
    '<ns0:tag xmlns:ns0="uri" />'
    >>> elem = ET.Element(ET.QName("uri", "tag"))
    >>> serialize(ET, elem) # 1.3
    '<ns0:tag xmlns:ns0="uri" />'
    >>> elem = ET.Element(ET.QName("uri", "tag"))
    >>> subelem = ET.SubElement(elem, ET.QName("uri", "tag1"))
    >>> subelem = ET.SubElement(elem, ET.QName("uri", "tag2"))
    >>> serialize(ET, elem) # 1.4
    '<ns0:tag xmlns:ns0="uri"><ns0:tag1 /><ns0:tag2 /></ns0:tag>'

    2) decorated attributes

    >>> elem.clear()
    >>> elem.attrib["{uri}key"] = "value"
    >>> serialize(ET, elem) # 2.1
    '<ns0:tag ns0:key="value" xmlns:ns0="uri" />'

    >>> elem.clear()
    >>> elem.attrib[ET.QName("{uri}key")] = "value"
    >>> serialize(ET, elem) # 2.2
    '<ns0:tag ns0:key="value" xmlns:ns0="uri" />'

    3) decorated values are not converted by default, but the
       QName wrapper can be used for values

    >>> elem.clear()
    >>> elem.attrib["{uri}key"] = "{uri}value"
    >>> serialize(ET, elem) # 3.1
    '<ns0:tag ns0:key="{uri}value" xmlns:ns0="uri" />'

    >>> elem.clear()
    >>> elem.attrib["{uri}key"] = ET.QName("{uri}value")
    >>> serialize(ET, elem) # 3.2
    '<ns0:tag ns0:key="ns0:value" xmlns:ns0="uri" />'

    >>> elem.clear()
    >>> subelem = ET.Element("tag")
    >>> subelem.attrib["{uri1}key"] = ET.QName("{uri2}value")
    >>> elem.append(subelem)
    >>> elem.append(subelem)
    >>> serialize(ET, elem) # 3.3
    '<ns0:tag xmlns:ns0="uri"><tag ns1:key="ns2:value" xmlns:ns1="uri1" xmlns:ns2="uri2" /><tag ns1:key="ns2:value" xmlns:ns1="uri1" xmlns:ns2="uri2" /></ns0:tag>'

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

def find():
    """
    Test find methods (including xpath syntax).

    >>> from xml.etree import ElementTree as ET

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

    >>> from xml.etree import ElementTree as ET

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


def check_encoding(ET, encoding):
    """
    >>> from xml.etree import ElementTree as ET

    >>> check_encoding(ET, "ascii")
    >>> check_encoding(ET, "us-ascii")
    >>> check_encoding(ET, "iso-8859-1")
    >>> check_encoding(ET, "iso-8859-15")
    >>> check_encoding(ET, "cp437")
    >>> check_encoding(ET, "mac-roman")
    """
    ET.XML("<?xml version='1.0' encoding='%s'?><xml />" % encoding)

def check_issue6233():
    """
    >>> from xml.etree import ElementTree as ET

    >>> e = ET.XML("<?xml version='1.0' encoding='utf-8'?><body>t\xe3g</body>")
    >>> ET.tostring(e, 'ascii')
    b"<?xml version='1.0' encoding='ascii'?>\\n<body>t&#227;g</body>"
    >>> e = ET.XML("<?xml version='1.0' encoding='iso-8859-1'?><body>t\xe3g</body>".encode('iso-8859-1')) # create byte string with the right encoding
    >>> ET.tostring(e, 'ascii')
    b"<?xml version='1.0' encoding='ascii'?>\\n<body>t&#227;g</body>"
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
  <xi:include href="samples/simple.xml"/>
</document>
"""

def xinclude_loader(href, parse="xml", encoding=None):
    try:
        data = XINCLUDE[href]
    except KeyError:
        raise IOError("resource not found")
    if parse == "xml":
        from xml.etree.ElementTree import XML
        return XML(data)
    return data

def xinclude():
    r"""
    Basic inclusion example (XInclude C.1)

    >>> from xml.etree import ElementTree as ET
    >>> from xml.etree import ElementInclude

    >>> document = xinclude_loader("C1.xml")
    >>> ElementInclude.include(document, xinclude_loader)
    >>> print(serialize(ET, document)) # C1
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
    >>> print(serialize(ET, document)) # C2
    <document>
      <p>This document has been accessed
      324387 times.</p>
    </document>

    Textual inclusion of XML example (XInclude C.3)

    >>> document = xinclude_loader("C3.xml")
    >>> ElementInclude.include(document, xinclude_loader)
    >>> print(serialize(ET, document)) # C3
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
    IOError: resource not found
    >>> # print serialize(ET, document) # C5

    """

def test_main():
    from test import test_xml_etree
    support.run_doctest(test_xml_etree, verbosity=True)

if __name__ == '__main__':
    test_main()
