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

import collections as _collections
import sys as _sys
import types as _types
from io import StringIO as _StringIO
lazy import _colorize
lazy import re
lazy from _pyrepl.utils import disp_str, gen_colors
lazy from dataclasses import fields as dataclass_fields
lazy from dataclasses import is_dataclass

__all__ = ["pprint","pformat","isreadable","isrecursive","saferepr",
           "PrettyPrinter", "pp"]


def pprint(
    object,
    stream=None,
    indent=1,
    width=80,
    depth=None,
    *,
    color=True,
    compact=False,
    expand=False,
    sort_dicts=True,
    underscore_numbers=False,
):
    """Pretty-print a Python object to a stream [default is sys.stdout]."""
    printer = PrettyPrinter(
        stream=stream,
        indent=indent,
        width=width,
        depth=depth,
        color=color,
        compact=compact,
        expand=expand,
        sort_dicts=sort_dicts,
        underscore_numbers=underscore_numbers,
    )
    printer.pprint(object)


def pformat(object, indent=1, width=80, depth=None, *,
            compact=False, expand=False, sort_dicts=True,
            underscore_numbers=False):
    """Format a Python object into a pretty-printed representation."""
    return PrettyPrinter(indent=indent, width=width, depth=depth,
                         compact=compact, expand=expand, sort_dicts=sort_dicts,
                         underscore_numbers=underscore_numbers).pformat(object)


def pp(object, *args, sort_dicts=False, **kwargs):
    """Pretty-print a Python object"""
    pprint(object, *args, sort_dicts=sort_dicts, **kwargs)


def saferepr(object):
    """Version of repr() which can handle recursive data structures."""
    return PrettyPrinter()._safe_repr(object, {}, None, 0)[0]


def isreadable(object):
    """Determine if saferepr(object) is readable by eval()."""
    return PrettyPrinter()._safe_repr(object, {}, None, 0)[1]


def isrecursive(object):
    """Determine if object requires a recursive representation."""
    return PrettyPrinter()._safe_repr(object, {}, None, 0)[2]


class _safe_key:
    """Helper function for key functions when sorting unorderable objects.

    The wrapped-object will fallback to a Py2.x style comparison for
    unorderable types (sorting first comparing the type name and then by
    the obj ids).  Does not work recursively, so dict.items() must have
    _safe_key applied to both the key and the value.

    """

    __slots__ = ['obj']

    def __init__(self, obj):
        self.obj = obj

    def __lt__(self, other):
        try:
            return self.obj < other.obj
        except TypeError:
            return ((str(type(self.obj)), id(self.obj)) < \
                    (str(type(other.obj)), id(other.obj)))


def _safe_tuple(t):
    "Helper function for comparing 2-tuples"
    return _safe_key(t[0]), _safe_key(t[1])


def _colorize_output(text):
    """Apply syntax highlighting."""
    if "\x1b[" in text:
        # If the text already contains ANSI escape sequences
        # (for example, from a custom __repr__),
        # return as-is to avoid breaking their color.
        return text
    colors = list(gen_colors(text))
    chars, _ = disp_str(text, colors=colors, force_color=True, escape=False)
    return "".join(chars)


class PrettyPrinter:
    def __init__(
        self,
        indent=1,
        width=80,
        depth=None,
        stream=None,
        *,
        color=True,
        compact=False,
        expand=False,
        sort_dicts=True,
        underscore_numbers=False,
    ):
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

        color
            If true (the default), syntax highlighting is enabled for pprint
            when the stream and environment variables permit.
            If false, colored output is always disabled.

        compact
            If true, several items will be combined in one line.
            Incompatible with expand mode.

        expand
            If true, the output will be formatted similar to
            pretty-printed json.dumps() when ``indent`` is supplied.
            Incompatible with compact mode.

        sort_dicts
            If true, dict keys are sorted.

        underscore_numbers
            If true, digit groups are separated with underscores.

        """
        indent = int(indent)
        width = int(width)
        if indent < 0:
            raise ValueError('indent must be >= 0')
        if depth is not None and depth <= 0:
            raise ValueError('depth must be > 0')
        if not width:
            raise ValueError('width must be != 0')
        if compact and expand:
            raise ValueError('compact and expand are incompatible')
        self._depth = depth
        self._indent_per_level = indent
        self._width = width
        if stream is not None:
            self._stream = stream
        else:
            self._stream = _sys.stdout
        self._compact = bool(compact)
        self._expand = bool(expand)
        self._sort_dicts = sort_dicts
        self._underscore_numbers = underscore_numbers
        self._color = color

    def pprint(self, object):
        if self._stream is None:
            return

        use_color = False
        if self._color:
            try:
                if _colorize.can_colorize(file=self._stream):
                    # Attempt to reify lazy imports, or ImportError
                    gen_colors, disp_str
                    use_color = True
            except ImportError:
                pass

        if use_color:
            sio = _StringIO()
            self._format(object, sio, 0, 0, {}, 0)
            self._stream.write(_colorize_output(sio.getvalue()))
        else:
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
        objid = id(object)
        if objid in context:
            stream.write(_recursion(object))
            self._recursive = True
            self._readable = False
            return
        rep = self._repr(object, context, level)
        max_width = self._width - indent - allowance
        if len(rep) > max_width:
            p = self._dispatch.get(type(object).__repr__, None)
            if p is not None:
                context[objid] = 1
                p(self, object, stream, indent, allowance, context, level + 1)
                del context[objid]
                return
            elif (is_dataclass(object) and
                  not isinstance(object, type) and
                  object.__dataclass_params__.repr and
                  # Check dataclass has generated repr method.
                  hasattr(object.__repr__, "__wrapped__") and
                  "__create_fn__" in object.__repr__.__wrapped__.__qualname__):
                context[objid] = 1
                self._pprint_dataclass(object, stream, indent, allowance, context, level + 1)
                del context[objid]
                return
        stream.write(rep)

    def _format_block_start(self, start_str, indent):
        if self._expand:
            return f"{start_str}\n{' ' * indent}"
        return start_str

    def _format_block_end(self, end_str, indent):
        if self._expand:
            return f"\n{' ' * indent}{end_str}"
        return end_str

    def _child_indent(self, indent, prefix_len):
        if self._expand:
            return indent
        return indent + prefix_len

    def _write_indent_padding(self, write):
        if self._expand:
            if self._indent_per_level > 0:
                write(self._indent_per_level * " ")
        elif self._indent_per_level > 1:
            write((self._indent_per_level - 1) * " ")

    def _pprint_dataclass(self, object, stream, indent, allowance, context, level):
        cls_name = object.__class__.__name__
        if self._expand:
            indent += self._indent_per_level
        else:
            indent += len(cls_name) + 1
        items = [(f.name, getattr(object, f.name)) for f in dataclass_fields(object) if f.repr]
        stream.write(self._format_block_start(cls_name + '(', indent))
        self._format_namespace_items(items, stream, indent, allowance, context, level)
        stream.write(self._format_block_end(')', indent - self._indent_per_level))

    _dispatch = {}

    def _pprint_dict(self, object, stream, indent, allowance, context, level):
        write = stream.write
        write(self._format_block_start('{', indent))
        self._write_indent_padding(write)
        length = len(object)
        if length:
            if self._sort_dicts:
                items = sorted(object.items(), key=_safe_tuple)
            else:
                items = object.items()
            self._format_dict_items(items, stream, indent, allowance + 1,
                                    context, level)
        write(self._format_block_end('}', indent))

    _dispatch[dict.__repr__] = _pprint_dict

    def _pprint_frozendict(self, object, stream, indent, allowance, context, level):
        write = stream.write
        cls = object.__class__
        if not len(object):
            write(repr(object))
            return

        write(self._format_block_start(cls.__name__ + "({", indent))
        self._write_indent_padding(write)

        if self._sort_dicts:
            items = sorted(object.items(), key=_safe_tuple)
        else:
            items = object.items()
        self._format_dict_items(
            items,
            stream,
            self._child_indent(indent, len(cls.__name__) + 1),
            allowance + 2,
            context,
            level,
        )
        write(self._format_block_end("})", indent))

    _dispatch[frozendict.__repr__] = _pprint_frozendict

    def _pprint_ordered_dict(self, object, stream, indent, allowance, context, level):
        if not len(object):
            stream.write(repr(object))
            return
        cls = object.__class__
        stream.write(cls.__name__ + '(')
        self._format(
            list(object.items()),
            stream,
            self._child_indent(indent, len(cls.__name__) + 1),
            allowance + 1,
            context,
            level,
        )
        stream.write(')')

    _dispatch[_collections.OrderedDict.__repr__] = _pprint_ordered_dict

    def _pprint_dict_view(self, object, stream, indent, allowance, context, level):
        """Pretty print dict views (keys, values, items)."""
        if isinstance(object, self._dict_items_view):
            key = _safe_tuple
        else:
            key = _safe_key

        write = stream.write
        write(
            self._format_block_start(object.__class__.__name__ + "([", indent)
        )

        if len(object):
            if self._sort_dicts:
                entries = sorted(object, key=key)
            else:
                entries = object
            self._format_items(
                entries, stream, indent, allowance + 2, context, level
            )
        write(self._format_block_end("])", indent))

    def _pprint_mapping_abc_view(self, object, stream, indent, allowance, context, level):
        """Pretty print mapping views from collections.abc."""
        write = stream.write
        write(object.__class__.__name__ + '(')
        # Dispatch formatting to the view's _mapping
        self._format(object._mapping, stream, indent, allowance, context, level)
        write(')')

    _dict_keys_view = type({}.keys())
    _dispatch[_dict_keys_view.__repr__] = _pprint_dict_view

    _dict_values_view = type({}.values())
    _dispatch[_dict_values_view.__repr__] = _pprint_dict_view

    _dict_items_view = type({}.items())
    _dispatch[_dict_items_view.__repr__] = _pprint_dict_view

    _dispatch[_collections.abc.MappingView.__repr__] = _pprint_mapping_abc_view

    _view_reprs = {cls.__repr__ for cls in
                   (_dict_keys_view, _dict_values_view, _dict_items_view,
                    _collections.abc.MappingView)}

    def _pprint_list(self, object, stream, indent, allowance, context, level):
        stream.write(self._format_block_start('[', indent))
        self._format_items(object, stream, indent, allowance + 1,
                           context, level)
        stream.write(self._format_block_end(']', indent))

    _dispatch[list.__repr__] = _pprint_list

    def _pprint_tuple(self, object, stream, indent, allowance, context, level):
        stream.write(self._format_block_start('(', indent))
        if len(object) == 1 and not self._expand:
            endchar = ',)'
        else:
            endchar = ')'
        self._format_items(object, stream, indent, allowance + len(endchar),
                           context, level)
        stream.write(self._format_block_end(endchar, indent))

    _dispatch[tuple.__repr__] = _pprint_tuple

    def _pprint_set(self, object, stream, indent, allowance, context, level):
        if not len(object):
            stream.write(repr(object))
            return
        typ = object.__class__
        if typ is set:
            stream.write(self._format_block_start('{', indent))
            endchar = '}'
        else:
            stream.write(self._format_block_start(typ.__name__ + '({', indent))
            endchar = '})'
            if not self._expand:
                indent += len(typ.__name__) + 1
        object = sorted(object, key=_safe_key)
        self._format_items(object, stream, indent, allowance + len(endchar),
                           context, level)
        stream.write(self._format_block_end(endchar, indent))

    _dispatch[set.__repr__] = _pprint_set
    _dispatch[frozenset.__repr__] = _pprint_set

    def _pprint_str(self, object, stream, indent, allowance, context, level):
        write = stream.write
        if not len(object):
            write(repr(object))
            return
        chunks = []
        lines = object.splitlines(True)
        if level == 1:
            if self._expand:
                indent += self._indent_per_level
            else:
                indent += 1
            allowance += 1
        max_width1 = max_width = self._width - indent
        for i, line in enumerate(lines):
            rep = repr(line)
            if i == len(lines) - 1:
                max_width1 -= allowance
            if len(rep) <= max_width1:
                chunks.append(rep)
            else:
                # A list of alternating (non-space, space) strings
                parts = re.findall(r'\S*\s*', line)
                assert parts
                assert not parts[-1]
                parts.pop()  # drop empty last part
                max_width2 = max_width
                current = ''
                for j, part in enumerate(parts):
                    candidate = current + part
                    if j == len(parts) - 1 and i == len(lines) - 1:
                        max_width2 -= allowance
                    if len(repr(candidate)) > max_width2:
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
            write(self._format_block_start("(", indent))
        for i, rep in enumerate(chunks):
            if i > 0:
                write('\n' + ' '*indent)
            write(rep)
        if level == 1:
            write(self._format_block_end(")", indent - self._indent_per_level))

    _dispatch[str.__repr__] = _pprint_str

    def _pprint_bytes(self, object, stream, indent, allowance, context, level):
        write = stream.write
        if len(object) <= 4:
            write(repr(object))
            return
        parens = level == 1
        if parens:
            if self._expand:
                indent += self._indent_per_level
            else:
                indent += 1
            allowance += 1
            write(self._format_block_start('(', indent))
        delim = ''
        for rep in _wrap_bytes_repr(object, self._width - indent, allowance):
            write(delim)
            write(rep)
            if not delim:
                delim = '\n' + ' '*indent
        if parens:
            write(self._format_block_end(')', indent - self._indent_per_level))

    _dispatch[bytes.__repr__] = _pprint_bytes

    def _pprint_bytearray(self, object, stream, indent, allowance, context, level):
        write = stream.write
        write(self._format_block_start('bytearray(', indent))
        if self._expand:
            write(' ' * self._indent_per_level)
            recursive_indent = indent + self._indent_per_level
        else:
            recursive_indent = indent + 10
        self._pprint_bytes(bytes(object), stream, recursive_indent,
                           allowance + 1, context, level + 1)
        write(self._format_block_end(')', indent))

    _dispatch[bytearray.__repr__] = _pprint_bytearray

    def _pprint_mappingproxy(self, object, stream, indent, allowance, context, level):
        stream.write('mappingproxy(')
        self._format(
            object.copy(),
            stream,
            self._child_indent(indent, 13),
            allowance + 1,
            context,
            level,
        )
        stream.write(')')

    _dispatch[_types.MappingProxyType.__repr__] = _pprint_mappingproxy

    def _pprint_simplenamespace(self, object, stream, indent, allowance, context, level):
        if type(object) is _types.SimpleNamespace:
            # The SimpleNamespace repr is "namespace" instead of the class
            # name, so we do the same here. For subclasses; use the class name.
            cls_name = 'namespace'
        else:
            cls_name = object.__class__.__name__
        if self._expand:
            indent += self._indent_per_level
        else:
            indent += len(cls_name) + 1
        items = object.__dict__.items()
        stream.write(self._format_block_start(cls_name + '(', indent))
        self._format_namespace_items(items, stream, indent, allowance, context,
                                     level)
        stream.write(self._format_block_end(')', indent - self._indent_per_level))

    _dispatch[_types.SimpleNamespace.__repr__] = _pprint_simplenamespace

    def _format_dict_items(self, items, stream, indent, allowance, context,
                           level):
        write = stream.write
        indent += self._indent_per_level
        delimnl = ',\n' + ' ' * indent
        last_index = len(items) - 1
        for i, (key, ent) in enumerate(items):
            last = i == last_index
            rep = self._repr(key, context, level)
            write(rep)
            write(': ')
            self._format(
                ent,
                stream,
                self._child_indent(indent, len(rep) + 2),
                allowance if last else 1,
                context,
                level,
            )
            if not last:
                write(delimnl)
            elif self._expand:
                write(',')

    def _format_namespace_items(self, items, stream, indent, allowance, context, level):
        write = stream.write
        delimnl = ',\n' + ' ' * indent
        last_index = len(items) - 1
        for i, (key, ent) in enumerate(items):
            last = i == last_index
            write(key)
            write('=')
            if id(ent) in context:
                # Special-case representation of recursion to match standard
                # recursive dataclass repr.
                write("...")
            else:
                self._format(
                    ent,
                    stream,
                    self._child_indent(indent, len(key) + 1),
                    allowance if last else 1,
                    context,
                    level,
                )
            if not last:
                write(delimnl)
            elif self._expand:
                write(',')

    def _format_items(self, items, stream, indent, allowance, context, level):
        write = stream.write
        indent += self._indent_per_level
        self._write_indent_padding(write)
        delimnl = ',\n' + ' ' * indent
        delim = ''
        width = max_width = self._width - indent + 1
        it = iter(items)
        try:
            next_ent = next(it)
        except StopIteration:
            return
        last = False
        while not last:
            ent = next_ent
            try:
                next_ent = next(it)
            except StopIteration:
                last = True
                max_width -= allowance
                width -= allowance
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
            self._format(ent, stream, indent,
                         allowance if last else 1,
                         context, level)
            if last and self._expand:
                write(',')

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
        return self._safe_repr(object, context, maxlevels, level)

    def _pprint_default_dict(self, object, stream, indent, allowance, context, level):
        if not len(object):
            stream.write(repr(object))
            return
        rdf = self._repr(object.default_factory, context, level)
        cls = object.__class__
        if self._expand:
            stream.write('%s(%s, ' % (cls.__name__, rdf))
        else:
            indent += len(cls.__name__) + 1
            stream.write('%s(%s,\n%s' % (cls.__name__, rdf, ' ' * indent))
        self._pprint_dict(object, stream, indent, allowance + 1, context,
                          level)
        stream.write(')')

    _dispatch[_collections.defaultdict.__repr__] = _pprint_default_dict

    def _pprint_counter(self, object, stream, indent, allowance, context, level):
        if not len(object):
            stream.write(repr(object))
            return
        cls = object.__class__
        stream.write(self._format_block_start(cls.__name__ + '({', indent))
        self._write_indent_padding(stream.write)
        items = object.most_common()
        self._format_dict_items(
            items,
            stream,
            self._child_indent(indent, len(cls.__name__) + 1),
            allowance + 2,
            context,
            level,
        )
        stream.write(self._format_block_end('})', indent))

    _dispatch[_collections.Counter.__repr__] = _pprint_counter

    def _pprint_chain_map(self, object, stream, indent, allowance, context, level):
        if not len(object.maps):
            stream.write(repr(object))
            return
        cls = object.__class__
        stream.write(self._format_block_start(cls.__name__ + '(',
                                              indent + self._indent_per_level))
        if self._expand:
            indent += self._indent_per_level
        else:
            indent += len(cls.__name__) + 1
        for i, m in enumerate(object.maps):
            if i == len(object.maps) - 1:
                self._format(m, stream, indent, allowance + 1, context, level)
                if self._expand:
                    stream.write(',')
                stream.write(self._format_block_end(')', indent - self._indent_per_level))
            else:
                self._format(m, stream, indent, 1, context, level)
                stream.write(',\n' + ' ' * indent)

    _dispatch[_collections.ChainMap.__repr__] = _pprint_chain_map

    def _pprint_deque(self, object, stream, indent, allowance, context, level):
        if not len(object):
            stream.write(repr(object))
            return
        cls = object.__class__
        stream.write(self._format_block_start(cls.__name__ + '([', indent))
        if not self._expand:
            indent += len(cls.__name__) + 1
        if object.maxlen is None:
            self._format_items(object, stream, indent, allowance + 2,
                               context, level)
            stream.write(self._format_block_end('])', indent))
        else:
            self._format_items(object, stream, indent, 2,
                               context, level)
            rml = self._repr(object.maxlen, context, level)
            if self._expand:
                stream.write('%s], maxlen=%s)' % ('\n' + ' ' * indent, rml))
            else:
                stream.write('],\n%smaxlen=%s)' % (' ' * indent, rml))

    _dispatch[_collections.deque.__repr__] = _pprint_deque

    def _pprint_user_dict(self, object, stream, indent, allowance, context, level):
        self._format(object.data, stream, indent, allowance, context, level - 1)

    _dispatch[_collections.UserDict.__repr__] = _pprint_user_dict

    def _pprint_user_list(self, object, stream, indent, allowance, context, level):
        self._format(object.data, stream, indent, allowance, context, level - 1)

    _dispatch[_collections.UserList.__repr__] = _pprint_user_list

    def _pprint_user_string(self, object, stream, indent, allowance, context, level):
        self._format(object.data, stream, indent, allowance, context, level - 1)

    _dispatch[_collections.UserString.__repr__] = _pprint_user_string

    def _pprint_template(self, object, stream, indent, allowance, context, level):
        cls_name = object.__class__.__name__
        if self._expand:
            indent += self._indent_per_level
        else:
            indent += len(cls_name) + 1

        items = (
            ("strings", object.strings),
            ("interpolations", object.interpolations),
        )
        stream.write(self._format_block_start(cls_name + "(", indent))
        self._format_namespace_items(
            items, stream, indent, allowance, context, level
        )
        stream.write(
            self._format_block_end(")", indent - self._indent_per_level)
        )

    def _pprint_interpolation(self, object, stream, indent, allowance, context, level):
        cls_name = object.__class__.__name__
        if self._expand:
            indent += self._indent_per_level
            items = (
                ("value", object.value),
                ("expression", object.expression),
                ("conversion", object.conversion),
                ("format_spec", object.format_spec),
            )
            stream.write(self._format_block_start(cls_name + "(", indent))
            self._format_namespace_items(
                items, stream, indent, allowance, context, level
            )
            stream.write(
                self._format_block_end(")", indent - self._indent_per_level)
            )
        else:
            indent += len(cls_name)
            items = (
                object.value,
                object.expression,
                object.conversion,
                object.format_spec,
            )
            stream.write(cls_name + "(")
            self._format_items(
                items, stream, indent, allowance, context, level
            )
            stream.write(")")

    t = t"{0}"
    _dispatch[type(t).__repr__] = _pprint_template
    _dispatch[type(t.interpolations[0]).__repr__] = _pprint_interpolation
    del t

    def _safe_repr(self, object, context, maxlevels, level):
        # Return triple (repr_string, isreadable, isrecursive).
        typ = type(object)
        if typ in _builtin_scalars:
            return repr(object), True, False

        r = getattr(typ, "__repr__", None)

        if issubclass(typ, int) and r is int.__repr__:
            if self._underscore_numbers:
                return f"{object:_d}", True, False
            else:
                return repr(object), True, False

        if ((issubclass(typ, dict) and r is dict.__repr__)
            or (issubclass(typ, frozendict) and r is frozendict.__repr__)):
            is_frozendict = issubclass(typ, frozendict)
            if not object:
                if is_frozendict:
                    rep = f"{object.__class__.__name__}()"
                else:
                    rep = "{}"
                return rep, True, False
            objid = id(object)
            if maxlevels and level >= maxlevels:
                rep = "{...}"
                if is_frozendict:
                    rep = f"{object.__class__.__name__}({rep})"
                return rep, False, objid in context
            if objid in context:
                return _recursion(object), False, True
            context[objid] = 1
            readable = True
            recursive = False
            components = []
            append = components.append
            level += 1
            if self._sort_dicts:
                items = sorted(object.items(), key=_safe_tuple)
            else:
                items = object.items()
            for k, v in items:
                krepr, kreadable, krecur = self.format(
                    k, context, maxlevels, level)
                vrepr, vreadable, vrecur = self.format(
                    v, context, maxlevels, level)
                append("%s: %s" % (krepr, vrepr))
                readable = readable and kreadable and vreadable
                if krecur or vrecur:
                    recursive = True
            del context[objid]
            rep = "{%s}" % ", ".join(components)
            if is_frozendict:
                rep = f"{object.__class__.__name__}({rep})"
            return rep, readable, recursive

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
                orepr, oreadable, orecur = self.format(
                    o, context, maxlevels, level)
                append(orepr)
                if not oreadable:
                    readable = False
                if orecur:
                    recursive = True
            del context[objid]
            return format % ", ".join(components), readable, recursive

        if issubclass(typ, _collections.abc.MappingView) and r in self._view_reprs:
            objid = id(object)
            if maxlevels and level >= maxlevels:
                return "{...}", False, objid in context
            if objid in context:
                return _recursion(object), False, True
            key = _safe_key
            if issubclass(typ, (self._dict_items_view, _collections.abc.ItemsView)):
                key = _safe_tuple
            if hasattr(object, "_mapping"):
                # Dispatch formatting to the view's _mapping
                mapping_repr, readable, recursive = self.format(
                    object._mapping, context, maxlevels, level)
                return (typ.__name__ + '(%s)' % mapping_repr), readable, recursive
            elif hasattr(typ, "_mapping"):
                #  We have a view that somehow has lost its type's _mapping, raise
                #  an error by calling repr() instead of failing cryptically later
                return repr(object), True, False
            if self._sort_dicts:
                object = sorted(object, key=key)
            context[objid] = 1
            readable = True
            recursive = False
            components = []
            append = components.append
            level += 1
            for val in object:
                vrepr, vreadable, vrecur = self.format(
                    val, context, maxlevels, level)
                append(vrepr)
                readable = readable and vreadable
                if vrecur:
                    recursive = True
            del context[objid]
            return typ.__name__ + '([%s])' % ", ".join(components), readable, recursive

        rep = repr(object)
        return rep, (rep and not rep.startswith('<')), False


_builtin_scalars = frozenset({str, bytes, bytearray, float, complex,
                              bool, type(None)})


def _recursion(object):
    return ("<Recursion on %s with id=%s>"
            % (type(object).__name__, id(object)))


def _wrap_bytes_repr(object, width, allowance):
    current = b''
    last = len(object) // 4 * 4
    for i in range(0, len(object), 4):
        part = object[i: i+4]
        candidate = current + part
        if i == last:
            width -= allowance
        if len(repr(candidate)) > width:
            if current:
                yield repr(current)
            current = part
        else:
            current = candidate
    if current:
        yield repr(current)
