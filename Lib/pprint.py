#  Author:      Fred L. Drake, Jr.
#               fdrake@cnri.reston.va.us, fdrake@acm.org
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

from types import DictType, ListType, TupleType

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

__all__ = ["pprint","pformat","isreadable","isrecursive","saferepr",
           "PrettyPrinter"]

def pprint(object, stream=None):
    """Pretty-print a Python object to a stream [default is sys.sydout]."""
    printer = PrettyPrinter(stream=stream)
    printer.pprint(object)

def pformat(object):
    """Format a Python object into a pretty-printed representation."""
    return PrettyPrinter().pformat(object)

def saferepr(object):
    """Version of repr() which can handle recursive data structures."""
    return _safe_repr(object, {})[0]

def isreadable(object):
    """Determine if saferepr(object) is readable by eval()."""
    return _safe_repr(object, {})[1]

def isrecursive(object):
    """Determine if object requires a recursive representation."""
    return _safe_repr(object, {})[2]

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
            import sys
            self.__stream = sys.stdout

    def pprint(self, object):
        self.__stream.write(self.pformat(object) + "\n")

    def pformat(self, object):
        sio = StringIO()
        self.__format(object, sio, 0, 0, {}, 0)
        return sio.getvalue()

    def isrecursive(self, object):
        self.__recursive = 0
        self.pformat(object)
        return self.__recursive

    def isreadable(self, object):
        self.__recursive = 0
        self.__readable = 1
        self.pformat(object)
        return self.__readable and not self.__recursive

    def __format(self, object, stream, indent, allowance, context, level):
        level = level + 1
        if context.has_key(id(object)):
            object = _Recursion(object)
            self.__recursive = 1
        rep = self.__repr(object, context, level - 1)
        objid = id(object)
        context[objid] = 1
        typ = type(object)
        sepLines = len(rep) > (self.__width - 1 - indent - allowance)

        if sepLines and typ in (ListType, TupleType):
            #  Pretty-print the sequence.
            stream.write((typ is ListType) and '[' or '(')
            if self.__indent_per_level > 1:
                stream.write((self.__indent_per_level - 1) * ' ')
            length = len(object)
            if length:
                indent = indent + self.__indent_per_level
                self.__format(object[0], stream, indent, allowance + 1,
                              context, level)
                if length > 1:
                    for ent in object[1:]:
                        stream.write(',\n' + ' '*indent)
                        self.__format(ent, stream, indent,
                                      allowance + 1, context, level)
                indent = indent - self.__indent_per_level
            if typ is TupleType and length == 1:
                stream.write(',')
            stream.write(((typ is ListType) and ']') or ')')

        elif sepLines and typ is DictType:
            stream.write('{')
            if self.__indent_per_level > 1:
                stream.write((self.__indent_per_level - 1) * ' ')
            length = len(object)
            if length:
                indent = indent + self.__indent_per_level
                items  = object.items()
                items.sort()
                key, ent = items[0]
                rep = self.__repr(key, context, level) + ': '
                stream.write(rep)
                self.__format(ent, stream, indent + len(rep),
                              allowance + 1, context, level)
                if len(items) > 1:
                    for key, ent in items[1:]:
                        rep = self.__repr(key, context, level) + ': '
                        stream.write(',\n' + ' '*indent + rep)
                        self.__format(ent, stream, indent + len(rep),
                                      allowance + 1, context, level)
                indent = indent - self.__indent_per_level
            stream.write('}')

        else:
            stream.write(rep)

        del context[objid]

    def __repr(self, object, context, level):
        repr, readable, recursive = _safe_repr(object, context,
                                               self.__depth, level)
        if not readable:
            self.__readable = 0
        if recursive:
            self.__recursive = 1
        return repr

# Return triple (repr_string, isreadable, isrecursive).

def _safe_repr(object, context, maxlevels=None, level=0):
    level += 1
    typ = type(object)
    if not (typ in (DictType, ListType, TupleType) and object):
        rep = `object`
        return rep, (rep and (rep[0] != '<')), 0

    if context.has_key(id(object)):
        return `_Recursion(object)`, 0, 1
    objid = id(object)
    context[objid] = 1

    readable = 1
    recursive = 0
    startchar, endchar = {ListType:  "[]",
                          TupleType: "()",
                          DictType:  "{}"}[typ]
    if maxlevels and level > maxlevels:
        with_commas = "..."
        readable = 0

    elif typ is DictType:
        components = []
        for k, v in object.iteritems():
            krepr, kreadable, krecur = _safe_repr(k, context, maxlevels,
                                                  level)
            vrepr, vreadable, vrecur = _safe_repr(v, context, maxlevels,
                                                  level)
            components.append("%s: %s" % (krepr, vrepr))
            readable = readable and kreadable and vreadable
            recursive = recursive or krecur or vrecur
        with_commas = ", ".join(components)

    else: # list or tuple
        assert typ in (ListType, TupleType)
        components = []
        for element in object:
            subrepr, subreadable, subrecur = _safe_repr(
                  element, context, maxlevels, level)
            components.append(subrepr)
            readable = readable and subreadable
            recursive = recursive or subrecur
        if len(components) == 1 and typ is TupleType:
            components[0] += ","
        with_commas = ", ".join(components)

    s = "%s%s%s" % (startchar, with_commas, endchar)
    del context[objid]
    return s, readable and not recursive, recursive

class _Recursion:
    # represent a recursive relationship; really only used for the __repr__()
    # method...
    def __init__(self, object):
        self.__repr = "<Recursion on %s with id=%s>" \
                      % (type(object).__name__, id(object))

    def __repr__(self):
        return self.__repr
