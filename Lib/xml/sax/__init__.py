"""Simple API for XML (SAX) implementation for Python.

This module provides an implementation of the SAX 2 interface;
information about the Java version of the interface can be found at
http://www.megginson.com/SAX/.  The Python version of the interface is
documented at <...>.

This package contains the following modules:

handler -- Base classes and constants which define the SAX 2 API for
           the 'client-side' of SAX for Python.

saxutils -- Implementation of the convenience classes commonly used to
            work with SAX.

xmlreader -- Base classes and constants which define the SAX 2 API for
             the parsers used with SAX for Python.

expatreader -- Driver that allows use of the Expat parser with the
               classes defined in saxlib.

"""

from handler import ContentHandler, ErrorHandler
from expatreader import ExpatParser
from _exceptions import SAXException, SAXNotRecognizedException, \
                        SAXParseException, SAXNotSupportedException


def parse(filename_or_stream, handler, errorHandler=ErrorHandler()):
    parser = ExpatParser()
    parser.setContentHandler(handler)
    parser.setErrorHandler(errorHandler)
    parser.parse(filename_or_stream)


def parseString(string, handler, errorHandler=ErrorHandler()):
    try:
        from cStringIO import StringIO
    except ImportError:
        from StringIO import StringIO
        
    if errorHandler is None:
        errorHandler = ErrorHandler()
    parser = ExpatParser()
    parser.setContentHandler(handler)
    parser.setErrorHandler(errorHandler)
    parser.parse(StringIO(string))
