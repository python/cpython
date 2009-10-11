"""
A simple demo that reads in an XML document and displays the number of
elements and attributes as well as a tally of elements and attributes by name.
"""

import sys
from collections import defaultdict

from xml.sax import make_parser, handler

class FancyCounter(handler.ContentHandler):

    def __init__(self):
        self._elems = 0
        self._attrs = 0
        self._elem_types = defaultdict(int)
        self._attr_types = defaultdict(int)

    def startElement(self, name, attrs):
        self._elems += 1
        self._attrs += len(attrs)
        self._elem_types[name] += 1

        for name in attrs.keys():
            self._attr_types[name] += 1

    def endDocument(self):
        print "There were", self._elems, "elements."
        print "There were", self._attrs, "attributes."

        print "---ELEMENT TYPES"
        for pair in  self._elem_types.items():
            print "%20s %d" % pair

        print "---ATTRIBUTE TYPES"
        for pair in  self._attr_types.items():
            print "%20s %d" % pair

if __name__ == '__main__':
    parser = make_parser()
    parser.setContentHandler(FancyCounter())
    parser.parse(sys.argv[1])
