"""
SAX driver for the Pyexpat C module.  This driver works with
pyexpat.__version__ == '1.5'.
"""

# Todo on driver:
#  - make it support external entities (wait for pyexpat.c)
#  - enable configuration between reset() and feed() calls
#  - support lexical events?
#  - proper inputsource handling
#  - properties and features

# Todo on pyexpat.c:
#  - support XML_ExternalEntityParserCreate
#  - exceptions in callouts from pyexpat to python code lose position info

version = "0.20"

from xml.parsers import expat
from xml.sax import xmlreader
import xml.sax

# --- ExpatParser

class ExpatParser( xmlreader.IncrementalParser, xmlreader.Locator ):
    "SAX driver for the Pyexpat C module."

    def __init__(self, namespaceHandling=0, bufsize=2**16-20):
        xmlreader.IncrementalParser.__init__(self, bufsize)
        self._source = None
        self._parser = None
        self._namespaces = namespaceHandling
        self._parsing = 0

    # XMLReader methods

    def parse(self, stream_or_string ):
        "Parse an XML document from a URL."
        if type( stream_or_string ) == type( "" ):
            stream=open( stream_or_string )
        else:
            stream=stream_or_string
 
        self.reset()
        self._cont_handler.setDocumentLocator(self)
        try:
            xmlreader.IncrementalParser.parse(self, stream)
        except expat.error:
            error_code = self._parser.ErrorCode
            raise xml.sax.SAXParseException(expat.ErrorString(error_code),
                                           None, self)
            
            self._cont_handler.endDocument()

    def prepareParser(self, filename=None):
        self._source = filename
        
        if self._source != None:
            self._parser.SetBase(self._source)
        
    def getFeature(self, name):
        "Looks up and returns the state of a SAX2 feature."
        raise SAXNotRecognizedException("Feature '%s' not recognized" % name)

    def setFeature(self, name, state):
        "Sets the state of a SAX2 feature."
        raise SAXNotRecognizedException("Feature '%s' not recognized" % name)

    def getProperty(self, name):
        "Looks up and returns the value of a SAX2 property."
        raise SAXNotRecognizedException("Property '%s' not recognized" % name)

    def setProperty(self, name, value):
        "Sets the value of a SAX2 property."
        raise SAXNotRecognizedException("Property '%s' not recognized" % name)

    # IncrementalParser methods

    def feed(self, data):
        if not self._parsing:
            self._parsing=1
            self.reset()
            self._cont_handler.startDocument()
        # FIXME: error checking and endDocument()
        self._parser.Parse(data, 0)

    def close(self):
        if self._parsing:
            self._cont_handler.endDocument()
            self._parsing=0
        self._parser.Parse("", 1)
        
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
#         self._parser.CommentHandler = 
#         self._parser.StartCdataSectionHandler = 
#         self._parser.EndCdataSectionHandler = 
#         self._parser.DefaultHandler = 
#         self._parser.DefaultHandlerExpand = 
#         self._parser.NotStandaloneHandler = 
        self._parser.ExternalEntityRefHandler = self.external_entity_ref
    
    # Locator methods

    def getColumnNumber(self):
        return self._parser.ErrorColumnNumber

    def getLineNumber(self):
        return self._parser.ErrorLineNumber

    def getPublicId(self):
        return self._source.getPublicId()

    def getSystemId(self):
        return self._parser.GetBase()
    
    # event handlers
    def start_element(self, name, attrs):
        self._cont_handler.startElement(name, name,
                                 xmlreader.AttributesImpl(attrs, attrs))

    def end_element(self, name):
        self._cont_handler.endElement( name, name )

    def start_element_ns(self, name, attrs):
        pair = name.split()
        if len(pair) == 1:
            tup = (None, name )
        else:
            tup = pair

        self._cont_handler.startElement(tup, None,
                                        xmlreader.AttributesImpl(attrs, None))        

    def end_element_ns(self, name):
        pair = name.split()
        if len(pair) == 1:
            name = (None, name, None)
        else:
            name = pair+[None] # prefix is not implemented yet!
            
        self._cont_handler.endElement(name, None)

    # this is not used
    def processing_instruction(self, target, data):
        self._cont_handler.processingInstruction(target, data)

    # this is not used
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
        assert 0 # not implemented
        source = self._ent_handler.resolveEntity(pubid, sysid)
        source = saxutils.prepare_input_source(source)
        # FIXME: create new parser, stack self._source and self._parser
        # FIXME: reuse code from self.parse(...)
        return 1
        
# ---
        
def create_parser(*args, **kwargs):
    return apply( ExpatParser, args, kwargs )
        
# ---

if __name__ == "__main__":
    import xml.sax
    p = create_parser()
    p.setContentHandler(xml.sax.XMLGenerator())
    p.setErrorHandler(xml.sax.ErrorHandler())
    p.parse("../../../hamlet.xml")
