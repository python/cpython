"""
A simple demo that reads in an XML document and spits out an equivalent,
but not necessarily identical, document.
"""

import sys, string

from xml.sax import saxutils, handler, make_parser

# --- The ContentHandler

class ContentGenerator(handler.ContentHandler):

    def __init__(self, out = sys.stdout):
        handler.ContentHandler.__init__(self)
        self._out = out

    # ContentHandler methods

    def startDocument(self):
        self._out.write('<?xml version="1.0" encoding="iso-8859-1"?>\n')

    def startElement(self, name, attrs):
        self._out.write('<' + name)
        for (name, value) in list(attrs.items()):
            self._out.write(' %s="%s"' % (name, saxutils.escape(value)))
        self._out.write('>')

    def endElement(self, name):
        self._out.write('</%s>' % name)

    def characters(self, content):
        self._out.write(saxutils.escape(content))

    def ignorableWhitespace(self, content):
        self._out.write(content)

    def processingInstruction(self, target, data):
        self._out.write('<?%s %s?>' % (target, data))

# --- The main program

parser = make_parser()
parser.setContentHandler(ContentGenerator())
parser.parse(sys.argv[1])
