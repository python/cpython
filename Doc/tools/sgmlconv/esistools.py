"""Miscellaneous utility functions useful for dealing with ESIS streams."""

import re

import xml.dom.pulldom

import xml.sax
import xml.sax.handler
import xml.sax.xmlreader


_data_match = re.compile(r"[^\\][^\\]*").match

def decode(s):
    r = ''
    while s:
        m = _data_match(s)
        if m:
            r = r + m.group()
            s = s[m.end():]
        elif s[1] == "\\":
            r = r + "\\"
            s = s[2:]
        elif s[1] == "n":
            r = r + "\n"
            s = s[2:]
        elif s[1] == "%":
            s = s[2:]
            n, s = s.split(";", 1)
            r = r + unichr(int(n))
        else:
            raise ValueError, "can't handle " + `s`
    return r


_charmap = {}
for c in range(128):
    _charmap[chr(c)] = chr(c)
    _charmap[unichr(c + 128)] = chr(c + 128)
_charmap["\n"] = r"\n"
_charmap["\\"] = r"\\"
del c

_null_join = ''.join
def encode(s):
    try:
        return _null_join(map(_charmap.get, s))
    except TypeError:
        raise Exception("could not encode %r: %r" % (s, map(_charmap.get, s)))


class ESISReader(xml.sax.xmlreader.XMLReader):
    """SAX Reader which reads from an ESIS stream.

    No verification of the document structure is performed by the
    reader; a general verifier could be used as the target
    ContentHandler instance.

    """
    _decl_handler = None
    _lexical_handler = None

    _public_id = None
    _system_id = None

    _buffer = ""
    _is_empty = 0
    _lineno = 0
    _started = 0

    def __init__(self, contentHandler=None, errorHandler=None):
        xml.sax.xmlreader.XMLReader.__init__(self)
        self._attrs = {}
        self._attributes = Attributes(self._attrs)
        self._locator = Locator()
        self._empties = {}
        if contentHandler:
            self.setContentHandler(contentHandler)
        if errorHandler:
            self.setErrorHandler(errorHandler)

    def get_empties(self):
        return self._empties.keys()

    #
    #  XMLReader interface
    #

    def parse(self, source):
        raise RuntimeError
        self._locator._public_id = source.getPublicId()
        self._locator._system_id = source.getSystemId()
        fp = source.getByteStream()
        handler = self.getContentHandler()
        if handler:
            handler.startDocument()
        lineno = 0
        while 1:
            token, data = self._get_token(fp)
            if token is None:
                break
            lineno = lineno + 1
            self._locator._lineno = lineno
            self._handle_token(token, data)
        handler = self.getContentHandler()
        if handler:
            handler.startDocument()

    def feed(self, data):
        if not self._started:
            handler = self.getContentHandler()
            if handler:
                handler.startDocument()
            self._started = 1
        data = self._buffer + data
        self._buffer = None
        lines = data.split("\n")
        if lines:
            for line in lines[:-1]:
                self._lineno = self._lineno + 1
                self._locator._lineno = self._lineno
                if not line:
                    e = xml.sax.SAXParseException(
                        "ESIS input line contains no token type mark",
                        None, self._locator)
                    self.getErrorHandler().error(e)
                else:
                    self._handle_token(line[0], line[1:])
            self._buffer = lines[-1]
        else:
            self._buffer = ""

    def close(self):
        handler = self.getContentHandler()
        if handler:
            handler.endDocument()
        self._buffer = ""

    def _get_token(self, fp):
        try:
            line = fp.readline()
        except IOError, e:
            e = SAXException("I/O error reading input stream", e)
            self.getErrorHandler().fatalError(e)
            return
        if not line:
            return None, None
        if line[-1] == "\n":
            line = line[:-1]
        if not line:
            e = xml.sax.SAXParseException(
                "ESIS input line contains no token type mark",
                None, self._locator)
            self.getErrorHandler().error(e)
            return
        return line[0], line[1:]

    def _handle_token(self, token, data):
        handler = self.getContentHandler()
        if token == '-':
            if data and handler:
                handler.characters(decode(data))
        elif token == ')':
            if handler:
                handler.endElement(decode(data))
        elif token == '(':
            if self._is_empty:
                self._empties[data] = 1
                self._is_empty = 0
            if handler:
                handler.startElement(data, self._attributes)
            self._attrs.clear()
        elif token == 'A':
            name, value = data.split(' ', 1)
            if value != "IMPLIED":
                type, value = value.split(' ', 1)
                self._attrs[name] = (decode(value), type)
        elif token == '&':
            # entity reference in SAX?
            pass
        elif token == '?':
            if handler:
                if ' ' in data:
                    target, data = data.split(None, 1)
                else:
                    target, data = data, ""
                handler.processingInstruction(target, decode(data))
        elif token == 'N':
            handler = self.getDTDHandler()
            if handler:
                handler.notationDecl(data, self._public_id, self._system_id)
            self._public_id = None
            self._system_id = None
        elif token == 'p':
            self._public_id = decode(data)
        elif token == 's':
            self._system_id = decode(data)
        elif token == 'e':
            self._is_empty = 1
        elif token == 'C':
            pass
        else:
            e = SAXParseException("unknown ESIS token in event stream",
                                  None, self._locator)
            self.getErrorHandler().error(e)

    def setContentHandler(self, handler):
        old = self.getContentHandler()
        if old:
            old.setDocumentLocator(None)
        if handler:
            handler.setDocumentLocator(self._locator)
        xml.sax.xmlreader.XMLReader.setContentHandler(self, handler)

    def getProperty(self, property):
        if property == xml.sax.handler.property_lexical_handler:
            return self._lexical_handler

        elif property == xml.sax.handler.property_declaration_handler:
            return self._decl_handler

        else:
            raise xml.sax.SAXNotRecognizedException("unknown property %s"
                                                    % `property`)

    def setProperty(self, property, value):
        if property == xml.sax.handler.property_lexical_handler:
            if self._lexical_handler:
                self._lexical_handler.setDocumentLocator(None)
            if value:
                value.setDocumentLocator(self._locator)
            self._lexical_handler = value

        elif property == xml.sax.handler.property_declaration_handler:
            if self._decl_handler:
                self._decl_handler.setDocumentLocator(None)
            if value:
                value.setDocumentLocator(self._locator)
            self._decl_handler = value

        else:
            raise xml.sax.SAXNotRecognizedException()

    def getFeature(self, feature):
        if feature == xml.sax.handler.feature_namespaces:
            return 1
        else:
            return xml.sax.xmlreader.XMLReader.getFeature(self, feature)

    def setFeature(self, feature, enabled):
        if feature == xml.sax.handler.feature_namespaces:
            pass
        else:
            xml.sax.xmlreader.XMLReader.setFeature(self, feature, enabled)


class Attributes(xml.sax.xmlreader.AttributesImpl):
    # self._attrs has the form {name: (value, type)}

    def getType(self, name):
        return self._attrs[name][1]

    def getValue(self, name):
        return self._attrs[name][0]

    def getValueByQName(self, name):
        return self._attrs[name][0]

    def __getitem__(self, name):
        return self._attrs[name][0]

    def get(self, name, default=None):
        if self._attrs.has_key(name):
            return self._attrs[name][0]
        return default

    def items(self):
        L = []
        for name, (value, type) in self._attrs.items():
            L.append((name, value))
        return L

    def values(self):
        L = []
        for value, type in self._attrs.values():
            L.append(value)
        return L


class Locator(xml.sax.xmlreader.Locator):
    _lineno = -1
    _public_id = None
    _system_id = None

    def getLineNumber(self):
        return self._lineno

    def getPublicId(self):
        return self._public_id

    def getSystemId(self):
        return self._system_id


def parse(stream_or_string, parser=None):
    if type(stream_or_string) in [type(""), type(u"")]:
        stream = open(stream_or_string)
    else:
        stream = stream_or_string
    if not parser:
        parser = ESISReader()
    return xml.dom.pulldom.DOMEventStream(stream, parser, (2 ** 14) - 20)
