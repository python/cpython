import pulldom
import string
from StringIO import StringIO
import types

"""
minidom.py -- a lightweight DOM implementation based on SAX.

Todo:
=====
 * convenience methods for getting elements and text.
 * more testing
 * bring some of the writer and linearizer code into conformance with this
        interface
 * SAX 2 namespaces
"""

class Node:
    ELEMENT_NODE                = 1
    ATTRIBUTE_NODE              = 2
    TEXT_NODE                   = 3
    CDATA_SECTION_NODE          = 4
    ENTITY_REFERENCE_NODE       = 5
    ENTITY_NODE                 = 6
    PROCESSING_INSTRUCTION_NODE = 7
    COMMENT_NODE                = 8
    DOCUMENT_NODE               = 9
    DOCUMENT_TYPE_NODE          = 10
    DOCUMENT_FRAGMENT_NODE      = 11
    NOTATION_NODE               = 12

    allnodes=[]

    def __init__( self ):
        self.childNodes=[]
        Node.allnodes.append( repr( id( self ))+repr( self.__class__ ))

    def __getattr__( self, key ):
        if key[0:2]=="__": raise AttributeError
        # getattr should never call getattr!
        if self.__dict__.has_key("inGetAttr"): 
            del self.inGetAttr
            raise AttributeError, key

        prefix,attrname=key[:5],key[5:]
        if prefix=="_get_":
            self.inGetAttr=1
            if hasattr( self, attrname ): 
                del self.inGetAttr
                return (lambda self=self, attrname=attrname: 
                                getattr( self, attrname ))
            else:
                del self.inGetAttr
                raise AttributeError, key
        else:
            self.inGetAttr=1
            try:
                func = getattr( self, "_get_"+key )
            except AttributeError:
                raise AttributeError, key
            del self.inGetAttr
            return func()

    def __nonzero__(self): return 1

    def toxml( self ):
        writer=StringIO()
        self.writexml( writer )
        return writer.getvalue()

    def hasChildNodes( self ):
        if self.childNodes: return 1
        else: return 0

    def insertBefore( self, newChild, refChild):
        index=self.childNodes.index( refChild )
        self.childNodes.insert( index, newChild )

    def appendChild( self, node ):
        self.childNodes.append( node )

    def unlink( self ):
        self.parentNode=None
        while self.childNodes:
            self.childNodes[-1].unlink()
            del self.childNodes[-1] # probably not most efficient!
        self.childNodes=None
        if self.attributes:
            for attr in self.attributes.values():
                attr.unlink()
        self.attributes=None
        index=Node.allnodes.index( repr( id( self ))+repr( self.__class__ ))
        del Node.allnodes[index]

def _write_data( writer, data):
    "Writes datachars to writer."
    data=string.replace(data,"&","&amp;")
    data=string.replace(data,"<","&lt;")
    data=string.replace(data,"\"","&quot;")
    data=string.replace(data,">","&gt;")
    writer.write(data)

def _closeElement( element ):
    del element.parentNode
    for node in element.elements:
        _closeElement( node )

def _getElementsByTagNameHelper( parent, name, rc ):
    for node in parent.childNodes:
        if node.nodeType==Node.ELEMENT_NODE and\
            (name=="*" or node.tagName==name):
            rc.append( node )
        _getElementsByTagNameHelper( node, name, rc )
    return rc

def _getElementsByTagNameNSHelper( parent, nsURI, localName, rc ):
    for node in parent.childNodes:
        if (node.nodeType==Node.ELEMENT_NODE ):
            if ((localName=="*" or node.tagName==localName) and
            (nsURI=="*" or node.namespaceURI==nsURI)):
                rc.append( node )
            _getElementsByTagNameNSHelper( node, name, rc )

class Attr(Node):
    nodeType=Node.ATTRIBUTE_NODE
    def __init__( self, qName, namespaceURI="", prefix="",
                  localName=None ):
        Node.__init__( self )
        assert qName
        # skip setattr for performance
        self.__dict__["nodeName"] = self.__dict__["name"] = qName
        self.__dict__["localName"]=localName or qName
        self.__dict__["prefix"]=prefix
        self.__dict__["namespaceURI"]=namespaceURI
        # nodeValue and value are set elsewhere
        self.attributes=None

    def __setattr__( self, name, value ):
        if name in ("value", "nodeValue" ):
            self.__dict__["value"]=self.__dict__["nodeValue"]=value
        else:
            self.__dict__[name]=value

class AttributeList:
    # the attribute list is a transient interface to the underlying dictionaries
    # mutations here will change the underlying element's dictionary
    def __init__( self, attrs, attrsNS ):
        self.__attrs=attrs
        self.__attrsNS=attrs
        self.length=len( self.__attrs.keys() )

    def item( self, index ):
        try:
            return self[self.keys()[index]]
        except IndexError:
            return None
        
    def items( self ):
        return map( lambda node: (node.tagName, node.value),
                    self.__attrs.values() )

    def itemsNS( self ):
        return map( lambda node: ((node.URI, node.localName), node.value),
                    self.__attrs.values() )
    
    def keys( self ):
        return self.__attrs.keys()

    def keysNS( self ):
        return self.__attrsNS.keys()

    def values( self ):
        return self.__attrs.values()

    def __len__( self ):
        return self.length

    def __cmp__( self, other ):
        if self.__attrs is other.__attrs: 
            return 0
        else: 
            return cmp( id( self ), id( other ) )

    #FIXME: is it appropriate to return .value?
    def __getitem__( self, attname_or_tuple ):
        if type( attname_or_tuple ) == type( (1,2) ):
            return self.__attrsNS[attname_or_tuple].value
        else:
            return self.__attrs[attname_or_tuple].value

    def __setitem__( self, attname ):
        raise TypeError, "object does not support item assignment"
        
class Element( Node ):
    nodeType=Node.ELEMENT_NODE
    def __init__( self, tagName, namespaceURI="", prefix="",
                  localName=None ):
        Node.__init__( self )
        self.tagName = self.nodeName = tagName
        self.localName=localName or tagName
        self.prefix=prefix
        self.namespaceURI=namespaceURI
        self.nodeValue=None

        self.__attrs={}  # attributes are double-indexed:
        self.__attrsNS={}#    tagName -> Attribute
                #    URI,localName -> Attribute
                # in the future: consider lazy generation of attribute objects
                #                this is too tricky for now because of headaches
                #                with namespaces.

    def getAttribute( self, attname ):
        return self.__attrs[attname].value

    def getAttributeNS( self, namespaceURI, localName ):
        return self.__attrsNS[(namespaceURI, localName)].value
    
    def setAttribute( self, attname, value ):
        attr=Attr( attname )
        # for performance
        attr.__dict__["value"]=attr.__dict__["nodeValue"]=value
        self.setAttributeNode( attr )

    def setAttributeNS( self, namespaceURI, qualifiedName, value ):
        attr=createAttributeNS( namespaceURI, qualifiedName )
        # for performance
        attr.__dict__["value"]=attr.__dict__["nodeValue"]=value
        self.setAttributeNode( attr )

    def setAttributeNode( self, attr ):
        self.__attrs[attr.name]=attr
        self.__attrsNS[(attr.namespaceURI,attr.localName)]=attr

    def removeAttribute( self, name ):
        attr = self.__attrs[name]
        self.removeAttributeNode( attr )

    def removeAttributeNS( self, namespaceURI, localName ):
        attr = self.__attrsNS[(uri, localName)]
        self.removeAttributeNode( attr )

    def removeAttributeNode( self, node ):
        del self.__attrs[node.name]
        del self.__attrsNS[(node.namespaceURI, node.localName)]
        
    def getElementsByTagName( self, name ):
        return _getElementsByTagNameHelper( self, name, [] )

    def getElementsByTagNameNS(self,namespaceURI,localName):
        _getElementsByTagNameNSHelper( self, namespaceURI, localName, [] )

    def __repr__( self ):
        return "<DOM Element:"+self.tagName+" at "+`id( self )` +" >"

    def writexml(self, writer):
        writer.write("<"+self.tagName)
            
        a_names=self._get_attributes().keys()
        a_names.sort()

        for a_name in a_names:
            writer.write(" "+a_name+"=\"")
            _write_data(writer, self._get_attributes()[a_name])
            writer.write("\"")
        if self.childNodes:
            writer.write(">")
            for node in self.childNodes:
                node.writexml( writer )
            writer.write("</"+self.tagName+">")
        else:
            writer.write("/>")

    def _get_attributes( self ):
        return AttributeList( self.__attrs, self.__attrsNS )

class Comment( Node ):
    nodeType=Node.COMMENT_NODE
    def __init__(self, data ):
        Node.__init__( self )
        self.data=self.nodeValue=data
        self.nodeName="#comment"
        self.attributes=None

    def writexml( self, writer ):
        writer.write( "<!--" + self.data + "-->" )

class ProcessingInstruction( Node ):
    nodeType=Node.PROCESSING_INSTRUCTION_NODE
    def __init__(self, target, data ):
        Node.__init__( self )
        self.target = self.nodeName = target
        self.data = self.nodeValue = data
        self.attributes=None

    def writexml( self, writer ):
        writer.write( "<?" + self.target +" " + self.data+ "?>" )

class Text( Node ):
    nodeType=Node.TEXT_NODE
    nodeName="#text"
    def __init__(self, data ):
        Node.__init__( self )
        self.data = self.nodeValue = data
        self.attributes=None

    def __repr__(self):
        if len( self.data )> 10:
            dotdotdot="..."
        else:
            dotdotdot=""
        return "<DOM Text node \"" + self.data[0:10] + dotdotdot+"\">"

    def writexml( self, writer ):
        _write_data( writer, self.data )

class Document( Node ):
    nodeType=Node.DOCUMENT_NODE
    def __init__( self ):
        Node.__init__( self )
        self.documentElement=None
        self.attributes=None
        self.nodeName="#document"
        self.nodeValue=None

    createElement=Element

    createTextNode=Text

    createComment=Comment

    createProcessingInstruction=ProcessingInstruction

    createAttribute=Attr

    def createElementNS(self, namespaceURI, qualifiedName):
        fields = string.split(qualifiedName, ':')
        if len(fields) == 2:
            prefix = fields[0]
            localName = fields[1]
        elif len(fields) == 1:
            prefix = ''
            localName = fields[0]            
        return Element(self, qualifiedName, namespaceURI, prefix, localName)

    def createAttributeNS(self, namespaceURI, qualifiedName):
        fields = string.split(qualifiedName,':')
        if len(fields) == 2:
            localName = fields[1]
            prefix = fields[0]
        elif len(fields) == 1:
            localName = fields[0]
            prefix = None
        return Attr(qualifiedName, namespaceURI, prefix, localName)

    def getElementsByTagNameNS(self,namespaceURI,localName):
        _getElementsByTagNameNSHelper( self, namespaceURI, localName )

    def close( self ):
        for node in self.elements:
            _closeElement( node )

    def unlink( self ):
        self.documentElement=None
        Node.unlink( self )

    def getElementsByTagName( self, name ):
        rc=[]
        _getElementsByTagNameHelper( self, name, rc )
        return rc

    def writexml( self, writer ):
        for node in self.childNodes:
            node.writexml( writer )

def _doparse( func, args, kwargs ):
    events=apply( func, args, kwargs )
    (toktype, rootNode)=events.getEvent()
    events.expandNode( rootNode )
    return rootNode

def parse( *args, **kwargs ):
    return _doparse( pulldom.parse, args, kwargs )

def parseString( *args, **kwargs ):
    return _doparse( pulldom.parseString, args, kwargs )
