#  Author:      Fred L. Drake, Jr.
#               fdrake@acm.org
#
#  This is a simple little module I wrote to make life easier.  I didn't
#  see anything quite like it in the library, though I may have overlooked
#  something.  I wrote this when I was trying to read some heavily nested
#  tuples with fairly non-descriptive content.  This is modeled very much
#  after Lisp/Scheme - style pretty-printing of lists.  If you find it
#  useful, thank small children who sleep at night.

"""Support to pretty-print lists, tuples, & dictionaries recursively.

Very simple, but useful, especially in debugging data structures.

Classes
-------

PrettyPrinter()
    Handle pretty-printing operations onto a stream using a configured
    set of formatting parameters.

Functions
---------

pformat()
    Format a Python object into a pretty-printed representation.

pprint()
    Pretty-print a Python object to a stream [default is sys.sydout].

saferepr()
    Generate a 'standard' repr()-like value, but protect against recursive
    data structures.

"""

from types import DictType, ListType, TupleType, StringType
import sys

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

__all__ = ["pprint","pformat","isreadable","isrecursive","saferepr",
           "PrettyPrinter"]

# cache these for faster access:
_commajoin = ", ".join
_sys_modules = sys.modules
_id = id
_len = len
_type = type


def pprint(object, stream=None):
    """Pretty-print a Python object to a stream [default is sys.sydout]."""
    printer = PrettyPrinter(stream=stream)
    printer.pprint(object)

def pformat(object):
    """Format a Python object into a pretty-printed representation."""
    return PrettyPrinter().pformat(object)

def saferepr(object):
    """Version of repr() which can handle recursive data structures."""
    return _safe_repr(object, {}, None, 0)[0]

def isreadable(object):
    """Determine if saferepr(object) is readable by eval()."""
    return _safe_repr(object, {}, None, 0)[1]

def isrecursive(object):
    """Determine if object requires a recursive representation."""
    return _safe_repr(object, {}, None, 0)[2]

class PrettyPrinter:
    def __init__(self, indent=1, width=80, depth=None, stream=None):
        """Handle pretty printing operations onto a stream using a set of
        configured parameters.

        indent
            Number of spaces to indent for each level of nesting.

        width
            Attempted maximum number of columns in the output.

        depth
            The maximum depth to print out nested structures.

        stream
            The desired output stream.  If omitted (or false), the standard
            output stream available at construction will be used.

        """
        indent = int(indent)
        width = int(width)
        assert indent >= 0
        assert depth is None or depth > 0, "depth must be > 0"
        assert width
        self.__depth = depth
        self.__indent_per_level = indent
        self.__width = width
        if stream:
            self.__stream = stream
        else:
            self.__stream = sys.stdout

    def pprint(self, object):
        self.__stream.write(self.pformat(object) + "\n")

    def pformat(self, object):
        sio = StringIO()
        self.__format(object, sio, 0, 0, {}, 0)
        return sio.getvalue()

    def isrecursive(self, object):
        self.__recursive = 0
        self.__repr(object, {}, 0)
        return self.__recursive

    def isreadable(self, object):
        self.__recursive = 0
        self.__readable = 1
        self.__repr(object, {}, 0)
        return self.__readable and not self.__recursive

    def __format(self, object, stream, indent, allowance, context, level):
        level = level + 1
        objid = _id(object)
        if objid in context:
            stream.write(_recursion(object))
            self.__recursive = 1
            self.__readable = 0
            return
        rep = self.__repr(object, context, level - 1)
        typ = _type(object)
        sepLines = _len(rep) > (self.__width - 1 - indent - allowance)
        write = stream.write

        if sepLines:
            if typ is DictType:
                write('{')
                if self.__indent_per_level > 1:
                    write((self.__indent_per_level - 1) * ' ')
                length = _len(object)
                if length:
                    context[objid] = 1
                    indent = indent + self.__indent_per_level
                    items  = object.items()
                    items.sort()
                    key, ent = items[0]
                    rep = self.__repr(key, context, level)
                    write(rep)
                    write(': ')
                    self.__format(ent, stream, indent + _len(rep) + 2,
                                  allowance + 1, context, level)
                    if length > 1:
                        for key, ent in items[1:]:
                            rep = self.__repr(key, context, level)
                            write(',\n%s%s: ' % (' '*indent, rep))
                            self.__format(ent, stream, indent + _len(rep) + 2,
                                          allowance + 1, context, level)
                    indent = indent - self.__indent_per_level
                    del context[objid]
                write('}')
                return

            if typ is ListType or typ is TupleType:
                if typ is ListType:
                    write('[')
                    endchar = ']'
                else:
                    write('(')
                    endchar = ')'
                if self.__indent_per_level > 1:
                    write((self.__indent_per_level - 1) * ' ')
                length = _len(object)
                if length:
                    context[objid] = 1
                    indent = indent + self.__indent_per_level
                    self.__format(object[0], stream, indent, allowance + 1,
                                  context, level)
                    if length > 1:
                        for ent in object[1:]:
                            write(',\n' + ' '*indent)
                            self.__format(ent, stream, indent,
                                          allowance + 1, context, level)
                    indent = indent - self.__indent_per_level
                    del context[objid]
                if typ is TupleType and length == 1:
                    write(',')
                write(endchar)
                return

        write(rep)

    def __repr(self, object, context, level):
        repr, readable, recursive = _safe_repr(object, context,
                                               self.__depth, level)
        if not readable:
            self.__readable = 0
        if recursive:
            self.__recursive = 1
        return repr

# Return triple (repr_string, isreadable, isrecursive).

def _safe_repr(object, context, maxlevels, level):
    typ = _type(object)
    if typ is StringType:
        if 'locale' not in _sys_modules:
            return `object`, 1, 0
        if "'" in object and '"' not in object:
            closure = '"'
            quotes = {'"': '\\"'}
        else:
            closure = "'"
            quotes = {"'": "\\'"}
        qget = quotes.get
        sio = StringIO()
        write = sio.write
        for char in object:
            if char.isalpha():
                write(char)
            else:
                write(qget(char, `char`[1:-1]))
        return ("%s%s%s" % (closure, sio.getvalue(), closure)), 1, 0

    if typ is DictType:
        if not object:
            return "{}", 1, 0
        objid = _id(object)
        if maxlevels and level > maxlevels:
            return "{...}", 0, objid in context
        if objid in context:
            return _recursion(object), 0, 1
        context[objid] = 1
        readable = 1
        recursive = 0
        components = []
        append = components.append
        level += 1
        saferepr = _safe_repr
        for k, v in object.iteritems():
            krepr, kreadable, krecur = saferepr(k, context, maxlevels, level)
            vrepr, vreadable, vrecur = saferepr(v, context, maxlevels, level)
            append("%s: %s" % (krepr, vrepr))
            readable = readable and kreadable and vreadable
            if krecur or vrecur:
                recursive = 1
        del context[objid]
        return "{%s}" % _commajoin(components), readable, recursive

    if typ is ListType or typ is TupleType:
        if typ is ListType:
            if not object:
                return "[]", 1, 0
            format = "[%s]"
        elif _len(object) == 1:
            format = "(%s,)"
        else:
            if not object:
                return "()", 1, 0
            format = "(%s)"
        objid = _id(object)
        if maxlevels and level > maxlevels:
            return format % "...", 0, objid in context
        if objid in context:
            return _recursion(object), 0, 1
        context[objid] = 1
        readable = 1
        recursive = 0
        components = []
        append = components.append
        level += 1
        for o in object:
            orepr, oreadable, orecur = _safe_repr(o, context, maxlevels, level)
            append(orepr)
            if not oreadable:
                readable = 0
            if orecur:
                recursive = 1
        del context[objid]
        return format % _commajoin(components), readable, recursive

    rep = `object`
    return rep, (rep and not rep.startswith('<')), 0


def _recursion(object):
    return ("<Recursion on %s with id=%s>"
            % (_type(object).__name__, _id(object)))


def _perfcheck(object=None):
    import time
    if object is None:
        object = [("string", (1, 2), [3, 4], {5: 6, 7: 8})] * 100000
    p = PrettyPrinter()
    t1 = time.time()
    _safe_repr(object, {}, None, 0)
    t2 = time.time()
    p.pformat(object)
    t3 = time.time()
    print "_safe_repr:", t2 - t1
    print "pformat:", t3 - t2

if __name__ == "__main__":
    _perfcheck()
