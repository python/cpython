# Wrapper module for _elementtree

from xml.etree.ElementTree import (ElementTree, dump, iselement, QName,
                                   fromstringlist,
                                   tostring, tostringlist, VERSION)
# These ones are not in ElementTree.__all__
from xml.etree.ElementTree import ElementPath, register_namespace

# Import the C accelerators:
#   Element, SubElement, TreeBuilder, XMLParser, ParseError
from _elementtree import *


class ElementTree(ElementTree):

    def parse(self, source, parser=None):
        close_source = False
        if not hasattr(source, 'read'):
            source = open(source, 'rb')
            close_source = True
        try:
            if parser is not None:
                while True:
                    data = source.read(65536)
                    if not data:
                        break
                    parser.feed(data)
                self._root = parser.close()
            else:
                parser = XMLParser()
                self._root = parser._parse(source)
            return self._root
        finally:
            if close_source:
                source.close()


class iterparse:
    root = None

    def __init__(self, file, events=None):
        self._close_file = False
        if not hasattr(file, 'read'):
            file = open(file, 'rb')
            self._close_file = True
        self._file = file
        self._events = []
        self._index = 0
        self._error = None
        self.root = self._root = None
        b = TreeBuilder()
        self._parser = XMLParser(b)
        self._parser._setevents(self._events, events)

    def __next__(self):
        while True:
            try:
                item = self._events[self._index]
                self._index += 1
                return item
            except IndexError:
                pass
            if self._error:
                e = self._error
                self._error = None
                raise e
            if self._parser is None:
                self.root = self._root
                if self._close_file:
                    self._file.close()
                raise StopIteration
            # load event buffer
            del self._events[:]
            self._index = 0
            data = self._file.read(16384)
            if data:
                try:
                    self._parser.feed(data)
                except SyntaxError as exc:
                    self._error = exc
            else:
                self._root = self._parser.close()
                self._parser = None

    def __iter__(self):
        return self


# =============================================================================
#
#   Everything below this line can be removed
#   after cElementTree is folded behind ElementTree.
#
# =============================================================================

from xml.etree.ElementTree import Comment as _Comment, PI as _PI


def parse(source, parser=None):
    tree = ElementTree()
    tree.parse(source, parser)
    return tree


def XML(text, parser=None):
    if not parser:
        parser = XMLParser()
    parser = XMLParser()
    parser.feed(text)
    return parser.close()


def XMLID(text, parser=None):
    tree = XML(text, parser=parser)
    ids = {}
    for elem in tree.iter():
        id = elem.get('id')
        if id:
            ids[id] = elem
    return tree, ids


class CommentProxy:

    def __call__(self, text=None):
        element = Element(_Comment)
        element.text = text
        return element

    def __eq__(self, other):
        return _Comment == other


class PIProxy:

    def __call__(self, target, text=None):
        element = Element(_PI)
        element.text = target
        if text:
            element.text = element.text + ' ' + text
        return element

    def __eq__(self, other):
        return _PI == other


Comment = CommentProxy()
PI = ProcessingInstruction = PIProxy()
del CommentProxy, PIProxy

# Aliases
fromstring = XML
XMLTreeBuilder = XMLParser
