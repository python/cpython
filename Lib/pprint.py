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
    Pretty-print a Python object to a stream [default is sys.stdout].

saferepr()
    Generate a 'standard' repr()-like value, but protect against recursive
    data structures.

"""

import re
import sys as _sys
from collections import OrderedDict as _OrderedDict
from io import StringIO as _StringIO

__all__ = ["pprint","pformat","isreadable","isrecursive","saferepr",
           "PrettyPrinter"]


def pprint(object, stream=None, indent=1, width=80, depth=None, *,
           compact=False):
    """Pretty-print a Python object to a stream [default is sys.stdout]."""
    printer = PrettyPrinter(
        stream=stream, indent=indent, width=width, depth=depth,
        compact=compact)
    printer.pprint(object)

def pformat(object, indent=1, width=80, depth=None, *, compact=False):
    """Format a Python object into a pretty-printed representation."""
    return PrettyPrinter(indent=indent, width=width, depth=depth,
                         compact=compact).pformat(object)

def saferepr(object):
    """Version of repr() which can handle recursive data structures."""
    return _safe_repr(object, {}, None, 0)[0]

def isreadable(object):
    """Determine if saferepr(object) is readable by eval()."""
    return _safe_repr(object, {}, None, 0)[1]

def isrecursive(object):
    """Determine if object requires a recursive representation."""
    return _safe_repr(object, {}, None, 0)[2]

class _safe_key:
    """Helper function for key functions when sorting unorderable objects.

    The wrapped-object will fallback to an Py2.x style comparison for
    unorderable types (sorting first comparing the type name and then by
    the obj ids).  Does not work recursively, so dict.items() must have
    _safe_key applied to both the key and the value.

    """

    __slots__ = ['obj']

    def __init__(self, obj):
        self.obj = obj

    def __lt__(self, other):
        try:
            rv = self.obj.__lt__(other.obj)
        except TypeError:
            rv = NotImplemented

        if rv is NotImplemented:
            rv = (str(type(self.obj)), id(self.obj)) < \
                 (str(type(other.obj)), id(other.obj))
        return rv

def _safe_tuple(t):
    "Helper function for comparing 2-tuples"
    return _safe_key(t[0]), _safe_key(t[1])

class PrettyPrinter:
    def __init__(self, indent=1, width=80, depth=None, stream=None, *,
                 compact=False):
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

        compact
            If true, several items will be combined in one line.

        """
        indent = int(indent)
        width = int(width)
        assert indent >= 0, "indent must be >= 0"
        assert depth is None or depth > 0, "depth must be > 0"
        assert width, "width must be != 0"
        self._depth = depth
        self._indent_per_level = indent
        self._width = width
        if stream is not None:
            self._stream = stream
        else:
            self._stream = _sys.stdout
        self._compact = bool(compact)

    def pprint(self, object):
        self._format(object, self._stream, 0, 0, {}, 0)
        self._stream.write("\n")

    def pformat(self, object):
        sio = _StringIO()
        self._format(object, sio, 0, 0, {}, 0)
        return sio.getvalue()

    def isrecursive(self, object):
        return self.format(object, {}, 0, 0)[2]

    def isreadable(self, object):
        s, readable, recursive = self.format(object, {}, 0, 0)
        return readable and not recursive

    def _format(self, object, stream, indent, allowance, context, level):
        level = level + 1
        objid = id(object)
        if objid in context:
            stream.write(_recursion(object))
            self._recursive = True
            self._readable = False
            return
        rep = self._repr(object, context, level - 1)
        typ = type(object)
        max_width = self._width - 1 - indent - allowance
        sepLines = len(rep) > max_width
        write = stream.write

        if sepLines:
            r = getattr(typ, "__repr__", None)
            if issubclass(typ, dict):
                write('{')
                if self._indent_per_level > 1:
                    write((self._indent_per_level - 1) * ' ')
                length = len(object)
                if length:
                    context[objid] = 1
                    indent = indent + self._indent_per_level
                    if issubclass(typ, _OrderedDict):
                        items = list(object.items())
                    else:
                        items = sorted(object.items(), key=_safe_tuple)
                    key, ent = items[0]
                    rep = self._repr(key, context, level)
                    write(rep)
                    write(': ')
                    self._format(ent, stream, indent + len(rep) + 2,
                                  allowance + 1, context, level)
                    if length > 1:
                        for key, ent in items[1:]:
                            rep = self._repr(key, context, level)
                            write(',\n%s%s: ' % (' '*indent, rep))
                            self._format(ent, stream, indent + len(rep) + 2,
                                          allowance + 1, context, level)
                    indent = indent - self._indent_per_level
                    del context[objid]
                write('}')
                return

            if ((issubclass(typ, list) and r is list.__repr__) or
                (issubclass(typ, tuple) and r is tuple.__repr__) or
                (issubclass(typ, set) and r is set.__repr__) or
                (issubclass(typ, frozenset) and r is frozenset.__repr__)
               ):
                length = len(object)
                if issubclass(typ, list):
                    write('[')
                    endchar = ']'
                elif issubclass(typ, tuple):
                    write('(')
                    endchar = ')'
                else:
                    if not length:
                        write(rep)
                        return
                    if typ is set:
                        write('{')
                        endchar = '}'
                    else:
                        write(typ.__name__)
                        write('({')
                        endchar = '})'
                        indent += len(typ.__name__) + 1
                    object = sorted(object, key=_safe_key)
                if self._indent_per_level > 1:
                    write((self._indent_per_level - 1) * ' ')
                if length:
                    context[objid] = 1
                    self._format_items(object, stream,
                                       indent + self._indent_per_level,
                                       allowance + 1, context, level)
                    del context[objid]
                if issubclass(typ, tuple) and length == 1:
                    write(',')
                write(endchar)
                return

            if issubclass(typ, str) and len(object) > 0 and r is str.__repr__:
                chunks = []
                lines = object.splitlines(True)
                if level == 1:
                    indent += 1
                    max_width -= 2
                for i, line in enumerate(lines):
                    rep = repr(line)
                    if len(rep) <= max_width:
                        chunks.append(rep)
                    else:
                        # A list of alternating (non-space, space) strings
                        parts = re.split(r'(\s+)', line) + ['']
                        current = ''
                        for i in range(0, len(parts), 2):
                            part = parts[i] + parts[i+1]
                            candidate = current + part
                            if len(repr(candidate)) > max_width:
                                if current:
                                    chunks.append(repr(current))
                                current = part
                            else:
                                current = candidate
                        if current:
                            chunks.append(repr(current))
                if len(chunks) == 1:
                    write(rep)
                    return
                if level == 1:
                    write('(')
                for i, rep in enumerate(chunks):
                    if i > 0:
                        write('\n' + ' '*indent)
                    write(rep)
                if level == 1:
                    write(')')
                return
        write(rep)

    def _format_items(self, items, stream, indent, allowance, context, level):
        write = stream.write
        delimnl = ',\n' + ' ' * indent
        delim = ''
        width = max_width = self._width - indent - allowance + 2
        for ent in items:
            if self._compact:
                rep = self._repr(ent, context, level)
                w = len(rep) + 2
                if width < w:
                    width = max_width
                    if delim:
                        delim = delimnl
                if width >= w:
                    width -= w
                    write(delim)
                    delim = ', '
                    write(rep)
                    continue
            write(delim)
            delim = delimnl
            self._format(ent, stream, indent, allowance, context, level)

    def _repr(self, object, context, level):
        repr, readable, recursive = self.format(object, context.copy(),
                                                self._depth, level)
        if not readable:
            self._readable = False
        if recursive:
            self._recursive = True
        return repr

    def format(self, object, context, maxlevels, level):
        """Format object for a specific context, returning a string
        and flags indicating whether the representation is 'readable'
        and whether the object represents a recursive construct.
        """
        return _safe_repr(object, context, maxlevels, level)


# Return triple (repr_string, isreadable, isrecursive).

def _safe_repr(object, context, maxlevels, level):
    typ = type(object)
    if typ is str:
        if 'locale' not in _sys.modules:
            return repr(object), True, False
        if "'" in object and '"' not in object:
            closure = '"'
            quotes = {'"': '\\"'}
        else:
            closure = "'"
            quotes = {"'": "\\'"}
        qget = quotes.get
        sio = _StringIO()
        write = sio.write
        for char in object:
            if char.isalpha():
                write(char)
            else:
                write(qget(char, repr(char)[1:-1]))
        return ("%s%s%s" % (closure, sio.getvalue(), closure)), True, False

    r = getattr(typ, "__repr__", None)
    if issubclass(typ, dict) and r is dict.__repr__:
        if not object:
            return "{}", True, False
        objid = id(object)
        if maxlevels and level >= maxlevels:
            return "{...}", False, objid in context
        if objid in context:
            return _recursion(object), False, True
        context[objid] = 1
        readable = True
        recursive = False
        components = []
        append = components.append
        level += 1
        saferepr = _safe_repr
        items = sorted(object.items(), key=_safe_tuple)
        for k, v in items:
            krepr, kreadable, krecur = saferepr(k, context, maxlevels, level)
            vrepr, vreadable, vrecur = saferepr(v, context, maxlevels, level)
            append("%s: %s" % (krepr, vrepr))
            readable = readable and kreadable and vreadable
            if krecur or vrecur:
                recursive = True
        del context[objid]
        return "{%s}" % ", ".join(components), readable, recursive

    if (issubclass(typ, list) and r is list.__repr__) or \
       (issubclass(typ, tuple) and r is tuple.__repr__):
        if issubclass(typ, list):
            if not object:
                return "[]", True, False
            format = "[%s]"
        elif len(object) == 1:
            format = "(%s,)"
        else:
            if not object:
                return "()", True, False
            format = "(%s)"
        objid = id(object)
        if maxlevels and level >= maxlevels:
            return format % "...", False, objid in context
        if objid in context:
            return _recursion(object), False, True
        context[objid] = 1
        readable = True
        recursive = False
        components = []
        append = components.append
        level += 1
        for o in object:
            orepr, oreadable, orecur = _safe_repr(o, context, maxlevels, level)
            append(orepr)
            if not oreadable:
                readable = False
            if orecur:
                recursive = True
        del context[objid]
        return format % ", ".join(components), readable, recursive

    rep = repr(object)
    return rep, (rep and not rep.startswith('<')), False


def _recursion(object):
    return ("<Recursion on %s with id=%s>"
            % (type(object).__name__, id(object)))


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
    print("_safe_repr:", t2 - t1)
    print("pformat:", t3 - t2)

if __name__ == "__main__":
    _perfcheck()
