import xml.sax
import xml.sax.handler

START_ELEMENT = "START_ELEMENT"
END_ELEMENT = "END_ELEMENT"
COMMENT = "COMMENT"
START_DOCUMENT = "START_DOCUMENT"
END_DOCUMENT = "END_DOCUMENT"
PROCESSING_INSTRUCTION = "PROCESSING_INSTRUCTION"
IGNORABLE_WHITESPACE = "IGNORABLE_WHITESPACE"
CHARACTERS = "CHARACTERS"

class PullDOM(xml.sax.ContentHandler):
    _locator = None
    document = None

    def __init__(self, documentFactory=None):
        self.documentFactory = documentFactory
        self.firstEvent = [None, None]
        self.lastEvent = self.firstEvent
        self._ns_contexts = [{}] # contains uri -> prefix dicts
        self._current_context = self._ns_contexts[-1]

    def setDocumentLocator(self, locator):
        self._locator = locator

    def startPrefixMapping(self, prefix, uri):
        self._ns_contexts.append(self._current_context.copy())
        self._current_context[uri] = prefix or ''

    def endPrefixMapping(self, prefix):
        self._current_context = self._ns_contexts.pop()

    def startElementNS(self, name, tagName , attrs):
        uri, localname = name
        if uri:
            # When using namespaces, the reader may or may not
            # provide us with the original name. If not, create
            # *a* valid tagName from the current context.
            if tagName is None:
                tagName = self._current_context[uri] + ":" + localname
            node = self.document.createElementNS(uri, tagName)
        else:
            # When the tagname is not prefixed, it just appears as
            # localname
            node = self.document.createElement(localname)

        for aname,value in attrs.items():
            a_uri, a_localname = aname
            if a_uri:
                qname = self._current_context[a_uri] + ":" + a_localname
                attr = self.document.createAttributeNS(a_uri, qname)
            else:
                attr = self.document.createAttribute(a_localname)
            attr.value = value
            node.setAttributeNode(attr)

##        print self.curNode, self.curNode.childNodes, node, node.parentNode
        self.curNode.appendChild(node)
#        node.parentNode = self.curNode
        self.curNode = node

        self.lastEvent[1] = [(START_ELEMENT, node), None]
        self.lastEvent = self.lastEvent[1]
        #self.events.append((START_ELEMENT, node))

    def endElementNS(self, name, tagName):
        node = self.curNode
        self.lastEvent[1] = [(END_ELEMENT, node), None]
        self.lastEvent = self.lastEvent[1]
        #self.events.append((END_ELEMENT, node))
        self.curNode = self.curNode.parentNode

    def startElement(self, name, attrs):
        node = self.document.createElement(name)

        for aname,value in attrs.items():
            attr = self.document.createAttribute(aname)
            attr.value = value
            node.setAttributeNode(attr)

        #node.parentNode = self.curNode
        self.curNode.appendChild(node)
        self.curNode = node

        self.lastEvent[1] = [(START_ELEMENT, node), None]
        self.lastEvent = self.lastEvent[1]
        #self.events.append((START_ELEMENT, node))

    def endElement(self, name):
        node = self.curNode
        self.lastEvent[1] = [(END_ELEMENT, node), None]
        self.lastEvent = self.lastEvent[1]
        #self.events.append((END_ELEMENT, node))
        self.curNode = node.parentNode

    def comment(self, s):
        node = self.document.createComment(s)
        self.curNode.appendChild(node)
#        parent = self.curNode
#        node.parentNode = parent
        self.lastEvent[1] = [(COMMENT, node), None]
        self.lastEvent = self.lastEvent[1]
        #self.events.append((COMMENT, node))

    def processingInstruction(self, target, data):
        node = self.document.createProcessingInstruction(target, data)

        self.curNode.appendChild(node)
#        parent = self.curNode
#        node.parentNode = parent
        self.lastEvent[1] = [(PROCESSING_INSTRUCTION, node), None]
        self.lastEvent = self.lastEvent[1]
        #self.events.append((PROCESSING_INSTRUCTION, node))

    def ignorableWhitespace(self, chars):
        node = self.document.createTextNode(chars)
        self.curNode.appendChild(node)
#        parent = self.curNode
#        node.parentNode = parent
        self.lastEvent[1] = [(IGNORABLE_WHITESPACE, node), None]
        self.lastEvent = self.lastEvent[1]
        #self.events.append((IGNORABLE_WHITESPACE, node))

    def characters(self, chars):
        node = self.document.createTextNode(chars)
        self.curNode.appendChild(node)
#        parent = self.curNode
#        node.parentNode = parent
        self.lastEvent[1] = [(CHARACTERS, node), None]
        self.lastEvent = self.lastEvent[1]

    def startDocument(self):
        publicId = systemId = None
        if self._locator:
            publicId = self._locator.getPublicId()
            systemId = self._locator.getSystemId()
        if self.documentFactory is None:
            import xml.dom.minidom
            self.documentFactory = xml.dom.minidom.Document.implementation
        node = self.documentFactory.createDocument(None, publicId, systemId)
        self.curNode = self.document = node
        self.lastEvent[1] = [(START_DOCUMENT, node), None]
        self.lastEvent = self.lastEvent[1]
        #self.events.append((START_DOCUMENT, node))

    def endDocument(self):
        assert self.curNode.parentNode is None, \
               "not all elements have been properly closed"
        assert self.curNode.documentElement is not None, \
               "document does not contain a root element"
        node = self.curNode.documentElement
        self.lastEvent[1] = [(END_DOCUMENT, node), None]
        #self.events.append((END_DOCUMENT, self.curNode))

class ErrorHandler:
    def warning(self, exception):
        print exception
    def error(self, exception):
        raise exception
    def fatalError(self, exception):
        raise exception

class DOMEventStream:
    def __init__(self, stream, parser, bufsize):
        self.stream = stream
        self.parser = parser
        self.bufsize = bufsize
        self.reset()

    def reset(self):
        self.pulldom = PullDOM()
        # This content handler relies on namespace support
        self.parser.setFeature(xml.sax.handler.feature_namespaces, 1)
        self.parser.setContentHandler(self.pulldom)

    def __getitem__(self, pos):
        rc = self.getEvent()
        if rc:
            return rc
        raise IndexError

    def expandNode(self, node):
        event = self.getEvent()
        while event:
            token, cur_node = event
            if cur_node is node:
                return
            if token != END_ELEMENT:
                cur_node.parentNode.appendChild(cur_node)
            event = self.getEvent()

    def getEvent(self):
        if not self.pulldom.firstEvent[1]:
            self.pulldom.lastEvent = self.pulldom.firstEvent
        while not self.pulldom.firstEvent[1]:
            buf = self.stream.read(self.bufsize)
            if not buf:
                #FIXME: why doesn't Expat close work?
                #self.parser.close()
                return None
            self.parser.feed(buf)
        rc = self.pulldom.firstEvent[1][0]
        self.pulldom.firstEvent[1] = self.pulldom.firstEvent[1][1]
        return rc

class SAX2DOM(PullDOM):

    def startElementNS(self, name, tagName , attrs):
        PullDOM.startElementNS(self, name, tagName, attrs)
        self.curNode.parentNode.appendChild(self.curNode)

    def startElement(self, name, attrs):
        PullDOM.startElement(self, name, attrs)
        self.curNode.parentNode.appendChild(self.curNode)

    def processingInstruction(self, target, data):
        PullDOM.processingInstruction(self, target, data)
        node = self.lastEvent[0][1]
        node.parentNode.appendChild(node)

    def ignorableWhitespace(self, chars):
        PullDOM.ignorableWhitespace(self, chars)
        node = self.lastEvent[0][1]
        node.parentNode.appendChild(node)

    def characters(self, chars):
        PullDOM.characters(self, chars)
        node = self.lastEvent[0][1]
        node.parentNode.appendChild(node)


default_bufsize = (2 ** 14) - 20

def parse(stream_or_string, parser=None, bufsize=None):
    if bufsize is None:
        bufsize = default_bufsize
    if type(stream_or_string) in [type(""), type(u"")]:
        stream = open(stream_or_string)
    else:
        stream = stream_or_string
    if not parser:
        parser = xml.sax.make_parser()
    return DOMEventStream(stream, parser, bufsize)

def parseString(string, parser=None):
    try:
        from cStringIO import StringIO
    except ImportError:
        from StringIO import StringIO

    bufsize = len(string)
    buf = StringIO(string)
    if not parser:
        parser = xml.sax.make_parser()
    return DOMEventStream(buf, parser, bufsize)
