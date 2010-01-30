'''Test module to thest the xmllib module.
   Sjoerd Mullender
'''

testdoc = """\
<?xml version="1.0" encoding="UTF-8" standalone='yes' ?>
<!-- comments aren't allowed before the <?xml?> tag,
     but they are allowed before the <!DOCTYPE> tag -->
<?processing instructions are allowed in the same places as comments ?>
<!DOCTYPE greeting [
  <!ELEMENT greeting (#PCDATA)>
]>
<greeting>Hello, world!</greeting>
"""

nsdoc = "<foo xmlns='URI' attr='val'/>"

from test import test_support
import unittest
# Silence Py3k warning
xmllib = test_support.import_module('xmllib', deprecated=True)

class XMLParserTestCase(unittest.TestCase):

    def test_simple(self):
        parser = xmllib.XMLParser()
        for c in testdoc:
            parser.feed(c)
        parser.close()

    def test_default_namespace(self):
        class H(xmllib.XMLParser):
            def unknown_starttag(self, name, attr):
                self.name, self.attr = name, attr
        h=H()
        h.feed(nsdoc)
        h.close()
        # The default namespace applies to elements...
        self.assertEquals(h.name, "URI foo")
        # but not to attributes
        self.assertEquals(h.attr, {'attr':'val'})


def test_main():
    test_support.run_unittest(XMLParserTestCase)

if __name__ == "__main__":
    test_main()
