"""
SAX driver for the Pyexpat C module.  This driver works with
pyexpat.__version__ == '2.22'.
"""

version = "0.20"

from xml.sax._exceptions import *
try:
    from xml.parsers import expat
except ImportError:
    raise SAXReaderNotAvailable("expat not supported",None)
from xml.sax import xmlreader, saxutils, handler

AttributesImpl = xmlreader.AttributesImpl
AttributesNSImpl = xmlreader.AttributesNSImpl

import string

# --- ExpatParser

class ExpatParser(xmlreader.IncrementalParser, xmlreader.Locator):
    "SAX driver for the Pyexpat C module."

    def __init__(self, namespaceHandling=0, bufsize=2**16-20):
        xmlreader.IncrementalParser.__init__(self, bufsize)
        self._source = xmlreader.InputSource()
        self._parser = None
        self._namespaces = namespaceHandling
        self._lex_handler_prop = None
        self._parsing = 0
        self._entity_stack = []

    # XMLReader methods

    def parse(self, source):
        "Parse an XML document from a URL or an InputSource."
        source = saxutils.prepare_input_source(source)

        self._source = source
        self.reset()
        self._cont_handler.setDocumentLocator(self)
        xmlreader.IncrementalParser.parse(self, source)

    def prepareParser(self, source):
        if source.getSystemId() != None:
            self._parser.SetBase(source.getSystemId())

    def getFeature(self, name):
        if name == handler.feature_namespaces:
            return self._namespaces
        raise SAXNotRecognizedException("Feature '%s' not recognized" % name)

    def setFeature(self, name, state):
        if self._parsing:
            raise SAXNotSupportedException("Cannot set features while parsing")
        if name == handler.feature_namespaces:
            self._namespaces = state
        else:
            raise SAXNotRecognizedException("Feature '%s' not recognized" %
                                            name)

    def getProperty(self, name):
        if name == handler.property_lexical_handler:
            return self._lex_handler_prop
        raise SAXNotRecognizedException("Property '%s' not recognized" % name)

    def setProperty(self, name, value):
        if name == handler.property_lexical_handler:
            self._lex_handler_prop = value
        else:
            raise SAXNotRecognizedException("Property '%s' not recognized" % name)

    # IncrementalParser methods

    def feed(self, data, isFinal = 0):
        if not self._parsing:
            self.reset()
            self._parsing = 1
            self._cont_handler.startDocument()

        try:
            # The isFinal parameter is internal to the expat reader.
            # If it is set to true, expat will check validity of the entire
            # document. When feeding chunks, they are not normally final -
            # except when invoked from close.
            self._parser.Parse(data, isFinal)
        except expat.error:
            error_code = self._parser.ErrorCode
            exc = SAXParseException(expat.ErrorString(error_code), None, self)
            # FIXME: when to invoke error()?
            self._err_handler.fatalError(exc)

    def close(self):
        if self._entity_stack:
            # If we are completing an external entity, do nothing here
            return
        self.feed("", isFinal = 1)
        self._cont_handler.endDocument()
        self._parsing = 0
        # break cycle created by expat handlers pointing to our methods
        self._parser = None

    def reset(self):
        if self._namespaces:
            self._parser = expat.ParserCreate(None, " ")
            self._parser.StartElementHandler = self.start_element_ns
            self._parser.EndElementHandler = self.end_element_ns
        else:
            self._parser = expat.ParserCreate()
            self._parser.StartElementHandler = self.start_element
            self._parser.EndElementHandler = self.end_element

        self._parser.ProcessingInstructionHandler = \
                                    self._cont_handler.processingInstruction
        self._parser.CharacterDataHandler = self._cont_handler.characters
        self._parser.UnparsedEntityDeclHandler = self.unparsed_entity_decl
        self._parser.NotationDeclHandler = self.notation_decl
        self._parser.StartNamespaceDeclHandler = self.start_namespace_decl
        self._parser.EndNamespaceDeclHandler = self.end_namespace_decl

        self._decl_handler_prop = None
        if self._lex_handler_prop:
            self._parser.CommentHandler = self._lex_handler_prop.comment
            self._parser.StartCdataSectionHandler = self._lex_handler_prop.startCDATA
            self._parser.EndCdataSectionHandler = self._lex_handler_prop.endCDATA
#         self._parser.DefaultHandler =
#         self._parser.DefaultHandlerExpand =
#         self._parser.NotStandaloneHandler =
        self._parser.ExternalEntityRefHandler = self.external_entity_ref

        self._parsing = 0
        self._entity_stack = []

    # Locator methods

    def getColumnNumber(self):
        if self._parser is None:
            return None
        return self._parser.ErrorColumnNumber

    def getLineNumber(self):
        if self._parser is None:
            return 1
        return self._parser.ErrorLineNumber

    def getPublicId(self):
        return self._source.getPublicId()

    def getSystemId(self):
        return self._source.getSystemId()

    # event handlers
    def start_element(self, name, attrs):
        self._cont_handler.startElement(name, AttributesImpl(attrs))

    def end_element(self, name):
        self._cont_handler.endElement(name)

    def start_element_ns(self, name, attrs):
        pair = string.split(name)
        if len(pair) == 1:
            pair = (None, name)
        else:
            pair = tuple(pair)

        newattrs = {}
        for (aname, value) in attrs.items():
            apair = string.split(aname)
            if len(apair) == 1:
                apair = (None, aname)
            else:
                apair = tuple(apair)

            newattrs[apair] = value

        self._cont_handler.startElementNS(pair, None,
                                          AttributesNSImpl(newattrs, {}))

    def end_element_ns(self, name):
        pair = string.split(name)
        if len(pair) == 1:
            pair = (None, name)
        else:
            pair = tuple(pair)

        self._cont_handler.endElementNS(pair, None)

    # this is not used (call directly to ContentHandler)
    def processing_instruction(self, target, data):
        self._cont_handler.processingInstruction(target, data)

    # this is not used (call directly to ContentHandler)
    def character_data(self, data):
        self._cont_handler.characters(data)

    def start_namespace_decl(self, prefix, uri):
        self._cont_handler.startPrefixMapping(prefix, uri)

    def end_namespace_decl(self, prefix):
        self._cont_handler.endPrefixMapping(prefix)

    def unparsed_entity_decl(self, name, base, sysid, pubid, notation_name):
        self._dtd_handler.unparsedEntityDecl(name, pubid, sysid, notation_name)

    def notation_decl(self, name, base, sysid, pubid):
        self._dtd_handler.notationDecl(name, pubid, sysid)

    def external_entity_ref(self, context, base, sysid, pubid):
        source = self._ent_handler.resolveEntity(pubid, sysid)
        source = saxutils.prepare_input_source(source,
                                               self._source.getSystemId() or
                                               "")

        self._entity_stack.append((self._parser, self._source))
        self._parser = self._parser.ExternalEntityParserCreate(context)
        self._source = source

        try:
            xmlreader.IncrementalParser.parse(self, source)
        except:
            return 0  # FIXME: save error info here?

        (self._parser, self._source) = self._entity_stack[-1]
        del self._entity_stack[-1]
        return 1

# ---

def create_parser(*args, **kwargs):
    return apply(ExpatParser, args, kwargs)

# ---

if __name__ == "__main__":
    import xml.sax
    p = create_parser()
    p.setContentHandler(xml.sax.XMLGenerator())
    p.setErrorHandler(xml.sax.ErrorHandler())
    p.parse("../../../hamlet.xml")
