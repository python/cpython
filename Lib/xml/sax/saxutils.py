"""
A library of useful helper classes to the sax classes, for the
convenience of application and driver writers.

$Id$
"""

import types, string, sys, urllib
import handler

def escape(data, entities = {}):
    """Escape &, <, and > in a string of data.
    You can escape other strings of data by passing a dictionary as 
    the optional entities parameter.  The keys and values must all be
    strings; each key will be replaced with its corresponding value.
    """
    data = string.replace(data, "&", "&amp;")
    data = string.replace(data, "<", "&lt;")
    data = string.replace(data, ">", "&gt;")
    for chars, entity in entities.items():
        data = string.replace(data, chars, entity)        
    return data

class XMLGenerator(handler.ContentHandler):

    def __init__(self, out = sys.stdout):
        handler.ContentHandler.__init__(self)
        self._out = out

    # ContentHandler methods
        
    def startDocument(self):
        self._out.write('<?xml version="1.0" encoding="iso-8859-1"?>\n')

    def startPrefixMapping(self, prefix, uri):
        pass

    def endPrefixMapping(self, prefix):
        pass

    def startElement(self, name, attrs):
        if type(name)==type(()):
            uri, localname, prefix=name
            name="%s:%s"%(prefix,localname)
        self._out.write('<' + name)
        for (name, value) in attrs.items():
            self._out.write(' %s="%s"' % (name, escape(value)))
        self._out.write('>')

    def endElement(self, name):
        # FIXME: not namespace friendly yet
        self._out.write('</%s>' % name)

    def characters(self, content):
        self._out.write(escape(content))

    def ignorableWhitespace(self, content):
        self._out.write(content)
        
    def processingInstruction(self, target, data):
        self._out.write('<?%s %s?>' % (target, data))

class XMLFilterBase:
    """This class is designed to sit between an XMLReader and the
    client application's event handlers.  By default, it does nothing
    but pass requests up to the reader and events on to the handlers
    unmodified, but subclasses can override specific methods to modify
    the event stream or the configuration requests as they pass
    through."""

    # ErrorHandler methods

    def error(self, exception):
        self._err_handler.error(exception)

    def fatalError(self, exception):
        self._err_handler.fatalError(exception)

    def warning(self, exception):
        self._err_handler.warning(exception)

    # ContentHandler methods
        
    def setDocumentLocator(self, locator):
        self._cont_handler.setDocumentLocator(locator)
        
    def startDocument(self):
        self._cont_handler.startDocument()

    def endDocument(self):
        self._cont_handler.endDocument()

    def startPrefixMapping(self, prefix, uri):
        self._cont_handler.startPrefixMapping(prefix, uri)

    def endPrefixMapping(self, prefix):
        self._cont_handler.endPrefixMapping(prefix)

    def startElement(self, name, attrs):
        self._cont_handler.startElement(name, attrs)

    def endElement(self, name, qname):
        self._cont_handler.endElement(name, qname)

    def characters(self, content):
        self._cont_handler.characters(content)

    def ignorableWhitespace(self, chars, start, end):
        self._cont_handler.ignorableWhitespace(chars, start, end)

    def processingInstruction(self, target, data):
        self._cont_handler.processingInstruction(target, data)

    def skippedEntity(self, name):
        self._cont_handler.skippedEntity(name)

    # DTDHandler methods

    def notationDecl(self, name, publicId, systemId):
        self._dtd_handler.notationDecl(name, publicId, systemId)

    def unparsedEntityDecl(self, name, publicId, systemId, ndata):
        self._dtd_handler.unparsedEntityDecl(name, publicId, systemId, ndata)

    # EntityResolver methods

    def resolveEntity(self, publicId, systemId):
        self._ent_handler.resolveEntity(publicId, systemId)

    # XMLReader methods

    def parse(self, source):
        self._parent.setContentHandler(self)
        self._parent.setErrorHandler(self)
        self._parent.setEntityResolver(self)
        self._parent.setDTDHandler(self)
        self._parent.parse(source)

    def setLocale(self, locale):
        self._parent.setLocale(locale)
    
    def getFeature(self, name):
        return self._parent.getFeature(name)

    def setFeature(self, name, state):
        self._parent.setFeature(name, state)

    def getProperty(self, name):
        return self._parent.getProperty(name)

    def setProperty(self, name, value):
        self._parent.setProperty(name, value)
        
