"""\
A library of useful helper classes to the SAX classes, for the
convenience of application and driver writers.
"""

import os, urlparse, urllib, types
import handler
import xmlreader

_StringTypes = [types.StringType, types.UnicodeType]

def escape(data, entities={}):
    """Escape &, <, and > in a string of data.

    You can escape other strings of data by passing a dictionary as 
    the optional entities parameter.  The keys and values must all be
    strings; each key will be replaced with its corresponding value.
    """
    data = data.replace("&", "&amp;")
    data = data.replace("<", "&lt;")
    data = data.replace(">", "&gt;")
    for chars, entity in entities.items():
        data = data.replace(chars, entity)        
    return data


class XMLGenerator(handler.ContentHandler):

    def __init__(self, out=None, encoding="iso-8859-1"):
        if out is None:
            import sys
            out = sys.stdout
        handler.ContentHandler.__init__(self)
        self._out = out
        self._ns_contexts = [{}] # contains uri -> prefix dicts
        self._current_context = self._ns_contexts[-1]
        self._undeclared_ns_maps = []
        self._encoding = encoding

    # ContentHandler methods

    def startDocument(self):
        self._out.write('<?xml version="1.0" encoding="%s"?>\n' %
                        self._encoding)

    def startPrefixMapping(self, prefix, uri):
        self._ns_contexts.append(self._current_context.copy())
        self._current_context[uri] = prefix
        self._undeclared_ns_maps.append((prefix, uri))

    def endPrefixMapping(self, prefix):
        self._current_context = self._ns_contexts[-1]
        del self._ns_contexts[-1]

    def startElement(self, name, attrs):
        self._out.write('<' + name)
        for (name, value) in attrs.items():
            self._out.write(' %s="%s"' % (name, escape(value)))
        self._out.write('>')
        
    def endElement(self, name):
        self._out.write('</%s>' % name)

    def startElementNS(self, name, qname, attrs):
        if name[0] is None:
            # if the name was not namespace-scoped, use the unqualified part
            name = name[1]
        else:
            # else try to restore the original prefix from the namespace
            name = self._current_context[name[0]] + ":" + name[1]
        self._out.write('<' + name)

        for pair in self._undeclared_ns_maps:
            self._out.write(' xmlns:%s="%s"' % pair)
        self._undeclared_ns_maps = []
        
        for (name, value) in attrs.items():
            name = self._current_context[name[0]] + ":" + name[1]
            self._out.write(' %s="%s"' % (name, escape(value)))
        self._out.write('>')

    def endElementNS(self, name, qname):
        if name[0] is None:
            name = name[1]
        else:
            name = self._current_context[name[0]] + ":" + name[1]
        self._out.write('</%s>' % name)
        
    def characters(self, content):
        self._out.write(escape(content))

    def ignorableWhitespace(self, content):
        self._out.write(content)

    def processingInstruction(self, target, data):
        self._out.write('<?%s %s?>' % (target, data))


class XMLFilterBase(xmlreader.XMLReader):
    """This class is designed to sit between an XMLReader and the
    client application's event handlers.  By default, it does nothing
    but pass requests up to the reader and events on to the handlers
    unmodified, but subclasses can override specific methods to modify
    the event stream or the configuration requests as they pass
    through."""

    def __init__(self, parent = None):
        xmlreader.XMLReader.__init__(self)
        self._parent = parent
    
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

    def endElement(self, name):
        self._cont_handler.endElement(name)

    def startElementNS(self, name, qname, attrs):
        self._cont_handler.startElement(name, attrs)

    def endElementNS(self, name, qname):
        self._cont_handler.endElementNS(name, qname)

    def characters(self, content):
        self._cont_handler.characters(content)

    def ignorableWhitespace(self, chars):
        self._cont_handler.ignorableWhitespace(chars)

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

    # XMLFilter methods

    def getParent(self):
        return self._parent

    def setParent(self, parent):
        self._parent = parent

# --- Utility functions

def prepare_input_source(source, base = ""):
    """This function takes an InputSource and an optional base URL and
    returns a fully resolved InputSource object ready for reading."""
    
    if type(source) in _StringTypes:
        source = xmlreader.InputSource(source)
    elif hasattr(source, "read"):
        f = source
        source = xmlreader.InputSource()
        source.setByteStream(f)
        if hasattr(f, "name"):
            f.setSystemId(f.name)

    if source.getByteStream() is None:
        sysid = source.getSystemId()
        if os.path.isfile(sysid):
            basehead = os.path.split(os.path.normpath(base))[0]
            source.setSystemId(os.path.join(basehead, sysid))
            f = open(sysid, "rb")
        else:
            source.setSystemId(urlparse.urljoin(base, sysid))
            f = urllib.urlopen(source.getSystemId())
            
        source.setByteStream(f)
        
    return source
