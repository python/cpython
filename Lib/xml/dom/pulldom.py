import minidom
import types
import string
import sys
from xml.sax import ExpatParser

#todo: SAX2/namespace handling

START_ELEMENT="START_ELEMENT"
END_ELEMENT="END_ELEMENT"
COMMENT="COMMENT"
START_DOCUMENT="START_DOCUMENT"
END_DOCUMENT="END_DOCUMENT"
PROCESSING_INSTRUCTION="PROCESSING_INSTRUCTION"
IGNORABLE_WHITESPACE="IGNORABLE_WHITESPACE"
CHARACTERS="CHARACTERS"

class PullDOM:
    def __init__( self ):
        self.firstEvent=[None,None]
        self.lastEvent=self.firstEvent

    def setDocumentLocator( self, locator ): pass

    def startElement( self, name, tagName , attrs  ):
        if not hasattr( self, "curNode" ):
            # FIXME: hack!
            self.startDocument( )

        node = self.document.createElement( tagName ) #FIXME namespaces!
        for attr in attrs.keys():
            node.setAttribute( attr, attrs[attr] )
        
        parent=self.curNode
        node.parentNode = parent
        if parent.childNodes:
            node.previousSibling=parent.childNodes[-1]
            node.previousSibling.nextSibling=node
        self.curNode = node
        # FIXME: do I have to screen namespace attributes
        self.lastEvent[1]=[(START_ELEMENT, node), None ]
        self.lastEvent=self.lastEvent[1]
        #self.events.append( (START_ELEMENT, node) )

    def endElement( self, name, tagName ):
        node = self.curNode
        self.lastEvent[1]=[(END_ELEMENT, node), None ]
        self.lastEvent=self.lastEvent[1]
        #self.events.append( (END_ELEMENT, node ))
        self.curNode = node.parentNode

    def comment( self, s):
        node = self.document.createComment ( s )
        parent=self.curNode
        node.parentNode=parent
        if parent.childNodes:
            node.previousSibling=parent.childNodes[-1]
            node.previousSibling.nextSibling=node
        self.lastEvent[1]=[(COMMENT, node), None ]
        self.lastEvent=self.lastEvent[1]
        #self.events.append( (COMMENT, node ))

    def processingInstruction( self, target, data ):
        node = self.document.createProcessingInstruction( target, data )
        #self.appendChild( node )
        
        parent=self.curNode
        node.parentNode=parent
        if parent.childNodes:
            node.previousSibling=parent.childNodes[-1]
            node.previousSibling.nextSibling=node
        self.lastEvent[1]=[(PROCESSING_INSTRUCTION, node), None ]
        self.lastEvent=self.lastEvent[1]
        #self.events.append( (PROCESSING_INSTRUCTION, node) )

    def ignorableWhitespace( self, chars ):
        node = self.document.createTextNode( chars[start:start+length] )
        parent=self.curNode
        node.parentNode=parent
        if parent.childNodes:
            node.previousSibling=parent.childNodes[-1]
            node.previousSibling.nextSibling=node
        self.lastEvent[1]=[(IGNORABLE_WHITESPACE, node), None ]
        self.lastEvent=self.lastEvent[1]
        #self.events.append( (IGNORABLE_WHITESPACE, node))

    def characters( self, chars ):
        node = self.document.createTextNode( chars )
        node.parentNode=self.curNode
        self.lastEvent[1]=[(CHARACTERS, node), None ]
        self.lastEvent=self.lastEvent[1]

    def startDocument( self ):
        node = self.curNode = self.document = minidom.Document()
        node.parentNode=None
        self.lastEvent[1]=[(START_DOCUMENT, node), None ]
        self.lastEvent=self.lastEvent[1]
        #self.events.append( (START_DOCUMENT, node) )
            
    def endDocument( self ):
        assert( not self.curNode.parentNode )
        for node in self.curNode.childNodes:
                if node.nodeType==node.ELEMENT_NODE:
                        self.document.documentElement = node
        #if not self.document.documentElement:
        #        raise Error, "No document element"

        self.lastEvent[1]=[(END_DOCUMENT, node), None ]
        #self.events.append( (END_DOCUMENT, self.curNode) )

class ErrorHandler:
    def warning( self, exception ):
        print exception
    def error( self, exception ):
        raise exception 
    def fatalError( self, exception ):
        raise exception 

class DOMEventStream:
    def __init__( self, stream, parser, bufsize ):
        self.stream=stream
        self.parser=parser
        self.bufsize=bufsize
        self.reset()

    def reset( self ):
        self.pulldom = PullDOM()
        self.parser.setContentHandler( self.pulldom )

    def __getitem__( self, pos ):
        rc=self.getEvent()
        if rc: return rc
        raise IndexError

    def expandNode( self, node ):
        event=self.getEvent()
        while event:
            token,cur_node=event
            if cur_node is node: return
            
            if token !=END_ELEMENT:
                cur_node.parentNode.appendChild( cur_node )
            event=self.getEvent()

    def getEvent( self ):
        if not self.pulldom.firstEvent[1]:
            self.pulldom.lastEvent=self.pulldom.firstEvent
        while not self.pulldom.firstEvent[1]:
            buf=self.stream.read( self.bufsize )
            if not buf:
                #FIXME: why doesn't Expat close work?
                #self.parser.close()
                return None
            self.parser.feed( buf )
        rc=self.pulldom.firstEvent[1][0]
        self.pulldom.firstEvent[1]=self.pulldom.firstEvent[1][1]
        return rc

# FIXME: sax2
#def _getParser( ):
 #   from xml.sax.saxexts import make_parser
    # expat doesn't report errors properly! Figure it out
  #  return make_parser()
   # return make_parser("xml.sax.drivers.drv_xmllib")


        
def _getParser():
     return ExpatParser()

default_bufsize=(2**14)-20
# FIXME: move into sax package for common usage
def parse( stream_or_string, parser=None, bufsize=default_bufsize ):
    if type( stream_or_string ) == type( "" ):
        stream=open( stream_or_string )
    else:
        stream=stream_or_string
    if not parser: 
        parser=_getParser()
    return DOMEventStream( stream, parser, bufsize )

def parseString( string, parser=None ):
    try:
        import cStringIO
        stringio=cStringIO.StringIO
    except ImportError:
        import StringIO
        stringio=StringIO.StringIO
        
    bufsize=len( string )
    buf=stringio( string )
    parser=_getParser()
    return DOMEventStream( buf, parser, bufsize )

