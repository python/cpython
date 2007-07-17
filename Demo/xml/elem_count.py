import sys

from xml.sax import make_parser, handler

class FancyCounter(handler.ContentHandler):

    def __init__(self):
        self._elems = 0
        self._attrs = 0
        self._elem_types = {}
        self._attr_types = {}

    def startElement(self, name, attrs):
        self._elems = self._elems + 1
        self._attrs = self._attrs + len(attrs)
        self._elem_types[name] = self._elem_types.get(name, 0) + 1

        for name in list(attrs.keys()):
            self._attr_types[name] = self._attr_types.get(name, 0) + 1

    def endDocument(self):
        print("There were", self._elems, "elements.")
        print("There were", self._attrs, "attributes.")

        print("---ELEMENT TYPES")
        for pair in  list(self._elem_types.items()):
            print("%20s %d" % pair)

        print("---ATTRIBUTE TYPES")
        for pair in  list(self._attr_types.items()):
            print("%20s %d" % pair)


parser = make_parser()
parser.setContentHandler(FancyCounter())
parser.parse(sys.argv[1])
