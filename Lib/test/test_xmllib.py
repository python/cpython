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

import test_support
import unittest
import xmllib


class XMLParserTestCase(unittest.TestCase):

    def test_simple(self):
        parser = xmllib.XMLParser()
        for c in testdoc:
            parser.feed(c)
        parser.close()


def test_main():
    test_support.run_unittest(XMLParserTestCase)


if __name__ == "__main__":
    test_main()
