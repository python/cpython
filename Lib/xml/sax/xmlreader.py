import handler

"""An XML Reader is the SAX 2 name for an XML parser. XML Parsers
should be based on this code. """
# ===== XMLREADER =====

class XMLReader:
    def __init__(self):
        self._cont_handler = handler.ContentHandler()
        #self._dtd_handler  = handler.DTDHandler()
        #self._ent_handler  = handler.EntityResolver()
        self._err_handler  = handler.ErrorHandler()

    def parse(self, source):
        "Parse an XML document from a system identifier or an InputSource."
        raise NotImplementedError("This method must be implemented!")

    def getContentHandler(self):
        "Returns the current ContentHandler."
        return self._cont_handler

    def setContentHandler(self, handler):
        "Registers a new object to receive document content events."
        self._cont_handler = handler
        
    def getDTDHandler(self):
        "Returns the current DTD handler."
        return self._dtd_handler
        
    def setDTDHandler(self, handler):
        "Register an object to receive basic DTD-related events."
        self._dtd_handler = handler

    def getEntityResolver(self):
        "Returns the current EntityResolver."
        return self._ent_handler
        
    def setEntityResolver(self, resolver):
        "Register an object to resolve external entities."
        self._ent_handler = resolver

    def getErrorHandler(self):
        "Returns the current ErrorHandler."
        return self._err_handler
        
    def setErrorHandler(self, handler):
        "Register an object to receive error-message events."
        self._err_handler = handler

    def setLocale(self, locale):
        """Allow an application to set the locale for errors and warnings. 
   
        SAX parsers are not required to provide localisation for errors
        and warnings; if they cannot support the requested locale,
        however, they must throw a SAX exception. Applications may
        request a locale change in the middle of a parse."""
        raise SAXNotSupportedException("Locale support not implemented")
    
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

 
class IncrementalParser(XMLReader):
    """This interface adds three extra methods to the XMLReader
    interface that allow XML parsers to support incremental
    parsing. Support for this interface is optional, since not all
    underlying XML parsers support this functionality.

    When the parser is instantiated it is ready to begin accepting
    data from the feed method immediately. After parsing has been
    finished with a call to close the reset method must be called to
    make the parser ready to accept new data, either from feed or
    using the parse method.

    Note that these methods must _not_ be called during parsing, that
    is, after parse has been called and before it returns.

    By default, the class also implements the parse method of the XMLReader
    interface using the feed, close and reset methods of the
    IncrementalParser interface as a convenience to SAX 2.0 driver
    writers."""
    def __init__(self, bufsize=2**16 ):
        self._bufsize=bufsize
        XMLReader.__init__( self )
    
    def parse(self, source):
        self.prepareParser(source)
        #FIXME: do some type checking: could be already stream, URL or
        #       filename
        inf=open( source )
        buffer = inf.read(self._bufsize)
        while buffer != "":
            self.feed(buffer)
            buffer = inf.read(self._bufsize)
        self.close()
        self.reset()

    def feed(self, data):        
        """This method gives the raw XML data in the data parameter to
        the parser and makes it parse the data, emitting the
        corresponding events. It is allowed for XML constructs to be
        split across several calls to feed.

        feed may raise SAXException."""
        raise NotImplementedError("This method must be implemented!")
    def prepareParser(self, source):
        """This method is called by the parse implementation to allow
        the SAX 2.0 driver to prepare itself for parsing."""
        raise NotImplementedError("prepareParser must be overridden!")

    def close(self):
        """This method is called when the entire XML document has been
        passed to the parser through the feed method, to notify the
        parser that there are no more data. This allows the parser to
        do the final checks on the document and empty the internal
        data buffer.

        The parser will not be ready to parse another document until
        the reset method has been called.

        close may raise SAXException."""
        raise NotImplementedError("This method must be implemented!")

    def reset(self):
        """This method is called after close has been called to reset
        the parser so that it is ready to parse new documents. The
        results of calling parse or feed after close without calling
        reset are undefined."""
        raise NotImplementedError("This method must be implemented!")

# ===== LOCATOR =====
class Locator:
    """Interface for associating a SAX event with a document
    location. A locator object will return valid results only during
    calls to DocumentHandler methods; at any other time, the
    results are unpredictable."""

    def getColumnNumber(self):
        "Return the column number where the current event ends."
        return -1

    def getLineNumber(self):
        "Return the line number where the current event ends."
        return -1

    def getPublicId(self):
        "Return the public identifier for the current event."
        return None

    def getSystemId(self):
        "Return the system identifier for the current event."
        return None

# --- AttributesImpl
class AttributesImpl:
    def __init__(self, attrs, rawnames):
        self._attrs = attrs
        self._rawnames = rawnames

    def getLength(self):
        return len(self._attrs)

    def getType(self, name):
        return "CDATA"

    def getValue(self, name):
        return self._attrs[name]

    def getValueByQName(self, name):
        return self._attrs[self._rawnames[name]]

    def getNameByQName(self, name):
        return self._rawnames[name]

    def getNames(self):
        return self._attrs.keys()

    def getQNames(self):
        return self._rawnames.keys()    

    def __len__(self):
        return len(self._attrs)

    def __getitem__(self, name):
        return self._attrs[name]

    def keys(self):
        return self._attrs.keys()

    def has_key(self, name):
        return self._attrs.has_key(name)

    def get(self, name, alternative=None):
        return self._attrs.get(name, alternative)

    def copy(self):
        return self.__class__(self._attrs, self._rawnames)

    def items(self):
        return self._attrs.items()

    def values(self):
        return self._attrs.values()

def _test():
    XMLReader()
    IncrementalParser()
    Locator()
    AttributesImpl()

if __name__=="__main__":
    _test()
