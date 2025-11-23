# This module contains ``ast.unparse()``, defined here
# to improve the import time for the ``ast`` module.
import sys
from _ast import *
from ast import NodeVisitor
from contextlib import contextmanager, nullcontext
from enum import IntEnum, auto, _simple_enum

# Large float and imaginary literals get turned into infinities in the AST.
# We unparse those infinities to INFSTR.
_INFSTR = "1e" + repr(sys.float_info.max_10_exp + 1)

@_simple_enum(IntEnum)
class _Precedence:
    """Precedence table that originated from python grammar."""

    NAMED_EXPR = auto()      # <target> := <expr1>
    TUPLE = auto()           # <expr1>, <expr2>
    YIELD = auto()           # 'yield', 'yield from'
    TEST = auto()            # 'if'-'else', 'lambda'
    OR = auto()              # 'or'
    AND = auto()             # 'and'
    NOT = auto()             # 'not'
    CMP = auto()             # '<', '>', '==', '>=', '<=', '!=',
                             # 'in', 'not in', 'is', 'is not'
    EXPR = auto()
    BOR = EXPR               # '|'
    BXOR = auto()            # '^'
    BAND = auto()            # '&'
    SHIFT = auto()           # '<<', '>>'
    ARITH = auto()           # '+', '-'
    TERM = auto()            # '*', '@', '/', '%', '//'
    FACTOR = auto()          # unary '+', '-', '~'
    POWER = auto()           # '**'
    AWAIT = auto()           # 'await'
    ATOM = auto()

    def next(self):
        try:
            return self.__class__(self + 1)
        except ValueError:
            return self


_SINGLE_QUOTES = ("'", '"')
_MULTI_QUOTES = ('"""', "'''")
_ALL_QUOTES = (*_SINGLE_QUOTES, *_MULTI_QUOTES)

class Unparser(NodeVisitor):
    """Methods in this class recursively traverse an AST and
    output source code for the abstract syntax; original formatting
    is disregarded."""

    def __init__(self):
        self._source = []
        self._precedences = {}
        self._type_ignores = {}
        self._indent = 0
        self._in_try_star = False
        self._in_interactive = False

    def interleave(self, inter, f, seq):
        """Call f on each item in seq, calling inter() in between."""
        seq = iter(seq)
        try:
            f(next(seq))
        except StopIteration:
            pass
        else:
            for x in seq:
                inter()
                f(x)

    def items_view(self, traverser, items):
        """Traverse and separate the given *items* with a comma and append it to
        the buffer. If *items* is a single item sequence, a trailing comma
        will be added."""
        if len(items) == 1:
            traverser(items[0])
            self.write(",")
        else:
            self.interleave(lambda: self.write(", "), traverser, items)

    def maybe_newline(self):
        """Adds a newline if it isn't the start of generated source"""
        if self._source:
            self.write("\n")

    def maybe_semicolon(self):
        """Adds a "; " delimiter if it isn't the start of generated source"""
        if self._source:
            self.write("; ")

    def fill(self, text="", *, allow_semicolon=True):
        """Indent a piece of text and append it, according to the current
        indentation level, or only delineate with semicolon if applicable"""
        if self._in_interactive and not self._indent and allow_semicolon:
            self.maybe_semicolon()
            self.write(text)
        else:
            self.maybe_newline()
            self.write("    " * self._indent + text)

    def write(self, *text):
        """Add new source parts"""
        self._source.extend(text)

    @contextmanager
    def buffered(self, buffer = None):
        if buffer is None:
            buffer = []

        original_source = self._source
        self._source = buffer
        yield buffer
        self._source = original_source

    @contextmanager
    def block(self, *, extra = None):
        """A context manager for preparing the source for blocks. It adds
        the character':', increases the indentation on enter and decreases
        the indentation on exit. If *extra* is given, it will be directly
        appended after the colon character.
        """
        self.write(":")
        if extra:
            self.write(extra)
        self._indent += 1
        yield
        self._indent -= 1

    @contextmanager
    def delimit(self, start, end):
        """A context manager for preparing the source for expressions. It adds
        *start* to the buffer and enters, after exit it adds *end*."""

        self.write(start)
        yield
        self.write(end)

    def delimit_if(self, start, end, condition):
        if condition:
            return self.delimit(start, end)
        else:
            return nullcontext()

    def require_parens(self, precedence, node):
        """Shortcut to adding precedence related parens"""
        return self.delimit_if("(", ")", self.get_precedence(node) > precedence)

    def get_precedence(self, node):
        return self._precedences.get(node, _Precedence.TEST)

    def set_precedence(self, precedence, *nodes):
        for node in nodes:
            self._precedences[node] = precedence

    def get_raw_docstring(self, node):
        """If a docstring node is found in the body of the *node* parameter,
        return that docstring node, None otherwise.

        Logic mirrored from ``_PyAST_GetDocString``."""
        if not isinstance(
            node, (AsyncFunctionDef, FunctionDef, ClassDef, Module)
        ) or len(node.body) < 1:
            return None
        node = node.body[0]
        if not isinstance(node, Expr):
            return None
        node = node.value
        if isinstance(node, Constant) and isinstance(node.value, str):
            return node

    def get_type_comment(self, node):
        comment = self._type_ignores.get(node.lineno) or node.type_comment
        if comment is not None:
            return f" # type: {comment}"

    def traverse(self, node):
        if isinstance(node, list):
            for item in node:
                self.traverse(item)
        else:
            super().visit(node)

    # Note: as visit() resets the output text, do NOT rely on
    # NodeVisitor.generic_visit to handle any nodes (as it calls back in to
    # the subclass visit() method, which resets self._source to an empty list)
    def visit(self, node):
        """Outputs a source code string that, if converted back to an ast
        (using ast.parse) will generate an AST equivalent to *node*"""
        self._source = []
        self.traverse(node)
        return "".join(self._source)

    def _write_docstring_and_traverse_body(self, node):
        if (docstring := self.get_raw_docstring(node)):
            self._write_docstring(docstring)
            self.traverse(node.body[1:])
        else:
            self.traverse(node.body)

    def visit_Module(self, node):
        self._type_ignores = {
            ignore.lineno: f"ignore{ignore.tag}"
            for ignore in node.type_ignores
        }
        try:
            self._write_docstring_and_traverse_body(node)
        finally:
            self._type_ignores.clear()

    def visit_Interactive(self, node):
        self._in_interactive = True
        try:
            self._write_docstring_and_traverse_body(node)
        finally:
            self._in_interactive = False

    def visit_FunctionType(self, node):
        with self.delimit("(", ")"):
            self.interleave(
                lambda: self.write(", "), self.traverse, node.argtypes
            )

        self.write(" -> ")
        self.traverse(node.returns)

    def visit_Expr(self, node):
        self.fill()
        self.set_precedence(_Precedence.YIELD, node.value)
        self.traverse(node.value)

    def visit_NamedExpr(self, node):
        with self.require_parens(_Precedence.NAMED_EXPR, node):
            self.set_precedence(_Precedence.ATOM, node.target, node.value)
            self.traverse(node.target)
            self.write(" := ")
            self.traverse(node.value)

    def visit_Import(self, node):
        self.fill("import ")
        self.interleave(lambda: self.write(", "), self.traverse, node.names)

    def visit_ImportFrom(self, node):
        self.fill("from ")
        self.write("." * (node.level or 0))
        if node.module:
            self.write(node.module)
        self.write(" import ")
        self.interleave(lambda: self.write(", "), self.traverse, node.names)

    def visit_Assign(self, node):
        self.fill()
        for target in node.targets:
            self.set_precedence(_Precedence.TUPLE, target)
            self.traverse(target)
            self.write(" = ")
        self.traverse(node.value)
        if type_comment := self.get_type_comment(node):
            self.write(type_comment)

    def visit_AugAssign(self, node):
        self.fill()
        self.traverse(node.target)
        self.write(" " + self.binop[node.op.__class__.__name__] + "= ")
        self.traverse(node.value)

    def visit_AnnAssign(self, node):
        self.fill()
        with self.delimit_if("(", ")", not node.simple and isinstance(node.target, Name)):
            self.traverse(node.target)
        self.write(": ")
        self.traverse(node.annotation)
        if node.value:
            self.write(" = ")
            self.traverse(node.value)

    def visit_Return(self, node):
        self.fill("return")
        if node.value:
            self.write(" ")
            self.traverse(node.value)

    def visit_Pass(self, node):
        self.fill("pass")

    def visit_Break(self, node):
        self.fill("break")

    def visit_Continue(self, node):
        self.fill("continue")

    def visit_Delete(self, node):
        self.fill("del ")
        self.interleave(lambda: self.write(", "), self.traverse, node.targets)

    def visit_Assert(self, node):
        self.fill("assert ")
        self.traverse(node.test)
        if node.msg:
            self.write(", ")
            self.traverse(node.msg)

    def visit_Global(self, node):
        self.fill("global ")
        self.interleave(lambda: self.write(", "), self.write, node.names)

    def visit_Nonlocal(self, node):
        self.fill("nonlocal ")
        self.interleave(lambda: self.write(", "), self.write, node.names)

    def visit_Await(self, node):
        with self.require_parens(_Precedence.AWAIT, node):
            self.write("await")
            if node.value:
                self.write(" ")
                self.set_precedence(_Precedence.ATOM, node.value)
                self.traverse(node.value)

    def visit_Yield(self, node):
        with self.require_parens(_Precedence.YIELD, node):
            self.write("yield")
            if node.value:
                self.write(" ")
                self.set_precedence(_Precedence.ATOM, node.value)
                self.traverse(node.value)

    def visit_YieldFrom(self, node):
        with self.require_parens(_Precedence.YIELD, node):
            self.write("yield from ")
            if not node.value:
                raise ValueError("Node can't be used without a value attribute.")
            self.set_precedence(_Precedence.ATOM, node.value)
            self.traverse(node.value)

    def visit_Raise(self, node):
        self.fill("raise")
        if not node.exc:
            if node.cause:
                raise ValueError(f"Node can't use cause without an exception.")
            return
        self.write(" ")
        self.traverse(node.exc)
        if node.cause:
            self.write(" from ")
            self.traverse(node.cause)

    def do_visit_try(self, node):
        self.fill("try", allow_semicolon=False)
        with self.block():
            self.traverse(node.body)
        for ex in node.handlers:
            self.traverse(ex)
        if node.orelse:
            self.fill("else", allow_semicolon=False)
            with self.block():
                self.traverse(node.orelse)
        if node.finalbody:
            self.fill("finally", allow_semicolon=False)
            with self.block():
                self.traverse(node.finalbody)

    def visit_Try(self, node):
        prev_in_try_star = self._in_try_star
        try:
            self._in_try_star = False
            self.do_visit_try(node)
        finally:
            self._in_try_star = prev_in_try_star

    def visit_TryStar(self, node):
        prev_in_try_star = self._in_try_star
        try:
            self._in_try_star = True
            self.do_visit_try(node)
        finally:
            self._in_try_star = prev_in_try_star

    def visit_ExceptHandler(self, node):
        self.fill("except*" if self._in_try_star else "except", allow_semicolon=False)
        if node.type:
            self.write(" ")
            self.traverse(node.type)
        if node.name:
            self.write(" as ")
            self.write(node.name)
        with self.block():
            self.traverse(node.body)

    def visit_ClassDef(self, node):
        self.maybe_newline()
        for deco in node.decorator_list:
            self.fill("@", allow_semicolon=False)
            self.traverse(deco)
        self.fill("class " + node.name, allow_semicolon=False)
        if hasattr(node, "type_params"):
            self._type_params_helper(node.type_params)
        with self.delimit_if("(", ")", condition = node.bases or node.keywords):
            comma = False
            for e in node.bases:
                if comma:
                    self.write(", ")
                else:
                    comma = True
                self.traverse(e)
            for e in node.keywords:
                if comma:
                    self.write(", ")
                else:
                    comma = True
                self.traverse(e)

        with self.block():
            self._write_docstring_and_traverse_body(node)

    def visit_FunctionDef(self, node):
        self._function_helper(node, "def")

    def visit_AsyncFunctionDef(self, node):
        self._function_helper(node, "async def")

    def _function_helper(self, node, fill_suffix):
        self.maybe_newline()
        for deco in node.decorator_list:
            self.fill("@", allow_semicolon=False)
            self.traverse(deco)
        def_str = fill_suffix + " " + node.name
        self.fill(def_str, allow_semicolon=False)
        if hasattr(node, "type_params"):
            self._type_params_helper(node.type_params)
        with self.delimit("(", ")"):
            self.traverse(node.args)
        if node.returns:
            self.write(" -> ")
            self.traverse(node.returns)
        with self.block(extra=self.get_type_comment(node)):
            self._write_docstring_and_traverse_body(node)

    def _type_params_helper(self, type_params):
        if type_params is not None and len(type_params) > 0:
            with self.delimit("[", "]"):
                self.interleave(lambda: self.write(", "), self.traverse, type_params)

    def visit_TypeVar(self, node):
        self.write(node.name)
        if node.bound:
            self.write(": ")
            self.traverse(node.bound)
        if node.default_value:
            self.write(" = ")
            self.traverse(node.default_value)

    def visit_TypeVarTuple(self, node):
        self.write("*" + node.name)
        if node.default_value:
            self.write(" = ")
            self.traverse(node.default_value)

    def visit_ParamSpec(self, node):
        self.write("**" + node.name)
        if node.default_value:
            self.write(" = ")
            self.traverse(node.default_value)

    def visit_TypeAlias(self, node):
        self.fill("type ")
        self.traverse(node.name)
        self._type_params_helper(node.type_params)
        self.write(" = ")
        self.traverse(node.value)

    def visit_For(self, node):
        self._for_helper("for ", node)

    def visit_AsyncFor(self, node):
        self._for_helper("async for ", node)

    def _for_helper(self, fill, node):
        self.fill(fill, allow_semicolon=False)
        self.set_precedence(_Precedence.TUPLE, node.target)
        self.traverse(node.target)
        self.write(" in ")
        self.traverse(node.iter)
        with self.block(extra=self.get_type_comment(node)):
            self.traverse(node.body)
        if node.orelse:
            self.fill("else", allow_semicolon=False)
            with self.block():
                self.traverse(node.orelse)

    def visit_If(self, node):
        self.fill("if ", allow_semicolon=False)
        self.traverse(node.test)
        with self.block():
            self.traverse(node.body)
        # collapse nested ifs into equivalent elifs.
        while node.orelse and len(node.orelse) == 1 and isinstance(node.orelse[0], If):
            node = node.orelse[0]
            self.fill("elif ", allow_semicolon=False)
            self.traverse(node.test)
            with self.block():
                self.traverse(node.body)
        # final else
        if node.orelse:
            self.fill("else", allow_semicolon=False)
            with self.block():
                self.traverse(node.orelse)

    def visit_While(self, node):
        self.fill("while ", allow_semicolon=False)
        self.traverse(node.test)
        with self.block():
            self.traverse(node.body)
        if node.orelse:
            self.fill("else", allow_semicolon=False)
            with self.block():
                self.traverse(node.orelse)

    def visit_With(self, node):
        self.fill("with ", allow_semicolon=False)
        self.interleave(lambda: self.write(", "), self.traverse, node.items)
        with self.block(extra=self.get_type_comment(node)):
            self.traverse(node.body)

    def visit_AsyncWith(self, node):
        self.fill("async with ", allow_semicolon=False)
        self.interleave(lambda: self.write(", "), self.traverse, node.items)
        with self.block(extra=self.get_type_comment(node)):
            self.traverse(node.body)

    def _str_literal_helper(
        self, string, *, quote_types=_ALL_QUOTES, escape_special_whitespace=False
    ):
        """Helper for writing string literals, minimizing escapes.
        Returns the tuple (string literal to write, possible quote types).
        """
        def escape_char(c):
            # \n and \t are non-printable, but we only escape them if
            # escape_special_whitespace is True
            if not escape_special_whitespace and c in "\n\t":
                return c
            # Always escape backslashes and other non-printable characters
            if c == "\\" or not c.isprintable():
                return c.encode("unicode_escape").decode("ascii")
            return c

        escaped_string = "".join(map(escape_char, string))
        possible_quotes = quote_types
        if "\n" in escaped_string:
            possible_quotes = [q for q in possible_quotes if q in _MULTI_QUOTES]
        possible_quotes = [q for q in possible_quotes if q not in escaped_string]
        if not possible_quotes:
            # If there aren't any possible_quotes, fallback to using repr
            # on the original string. Try to use a quote from quote_types,
            # e.g., so that we use triple quotes for docstrings.
            string = repr(string)
            quote = next((q for q in quote_types if string[0] in q), string[0])
            return string[1:-1], [quote]
        if escaped_string:
            # Sort so that we prefer '''"''' over """\""""
            possible_quotes.sort(key=lambda q: q[0] == escaped_string[-1])
            # If we're using triple quotes and we'd need to escape a final
            # quote, escape it
            if possible_quotes[0][0] == escaped_string[-1]:
                assert len(possible_quotes[0]) == 3
                escaped_string = escaped_string[:-1] + "\\" + escaped_string[-1]
        return escaped_string, possible_quotes

    def _write_str_avoiding_backslashes(self, string, *, quote_types=_ALL_QUOTES):
        """Write string literal value with a best effort attempt to avoid backslashes."""
        string, quote_types = self._str_literal_helper(string, quote_types=quote_types)
        quote_type = quote_types[0]
        self.write(f"{quote_type}{string}{quote_type}")

    def _ftstring_helper(self, parts):
        new_parts = []
        quote_types = list(_ALL_QUOTES)
        fallback_to_repr = False
        for value, is_constant in parts:
            if is_constant:
                value, new_quote_types = self._str_literal_helper(
                    value,
                    quote_types=quote_types,
                    escape_special_whitespace=True,
                )
                if set(new_quote_types).isdisjoint(quote_types):
                    fallback_to_repr = True
                    break
                quote_types = new_quote_types
            else:
                if "\n" in value:
                    quote_types = [q for q in quote_types if q in _MULTI_QUOTES]
                    assert quote_types

                new_quote_types = [q for q in quote_types if q not in value]
                if new_quote_types:
                    quote_types = new_quote_types
            new_parts.append(value)

        if fallback_to_repr:
            # If we weren't able to find a quote type that works for all parts
            # of the JoinedStr, fallback to using repr and triple single quotes.
            quote_types = ["'''"]
            new_parts.clear()
            for value, is_constant in parts:
                if is_constant:
                    value = repr('"' + value)  # force repr to use single quotes
                    expected_prefix = "'\""
                    assert value.startswith(expected_prefix), repr(value)
                    value = value[len(expected_prefix):-1]
                new_parts.append(value)

        value = "".join(new_parts)
        quote_type = quote_types[0]
        self.write(f"{quote_type}{value}{quote_type}")

    def _write_ftstring(self, values, prefix):
        self.write(prefix)
        fstring_parts = []
        for value in values:
            with self.buffered() as buffer:
                self._write_ftstring_inner(value)
            fstring_parts.append(
                ("".join(buffer), isinstance(value, Constant))
            )
        self._ftstring_helper(fstring_parts)

    def visit_JoinedStr(self, node):
        self._write_ftstring(node.values, "f")

    def visit_TemplateStr(self, node):
        self._write_ftstring(node.values, "t")

    def _write_ftstring_inner(self, node, is_format_spec=False):
        if isinstance(node, JoinedStr):
            # for both the f-string itself, and format_spec
            for value in node.values:
                self._write_ftstring_inner(value, is_format_spec=is_format_spec)
        elif isinstance(node, Constant) and isinstance(node.value, str):
            value = node.value.replace("{", "{{").replace("}", "}}")

            if is_format_spec:
                value = value.replace("\\", "\\\\")
                value = value.replace("'", "\\'")
                value = value.replace('"', '\\"')
                value = value.replace("\n", "\\n")
            self.write(value)
        elif isinstance(node, FormattedValue):
            self.visit_FormattedValue(node)
        elif isinstance(node, Interpolation):
            self.visit_Interpolation(node)
        else:
            raise ValueError(f"Unexpected node inside JoinedStr, {node!r}")

    def _unparse_interpolation_value(self, inner):
        unparser = type(self)()
        unparser.set_precedence(_Precedence.TEST.next(), inner)
        return unparser.visit(inner)

    def _write_interpolation(self, node, use_str_attr=False):
        with self.delimit("{", "}"):
            if use_str_attr:
                expr = node.str
            else:
                expr = self._unparse_interpolation_value(node.value)
            if expr.startswith("{"):
                # Separate pair of opening brackets as "{ {"
                self.write(" ")
            self.write(expr)
            if node.conversion != -1:
                self.write(f"!{chr(node.conversion)}")
            if node.format_spec:
                self.write(":")
                self._write_ftstring_inner(node.format_spec, is_format_spec=True)

    def visit_FormattedValue(self, node):
        self._write_interpolation(node)

    def visit_Interpolation(self, node):
        # If `str` is set to `None`, use the `value` to generate the source code.
        self._write_interpolation(node, use_str_attr=node.str is not None)

    def visit_Name(self, node):
        self.write(node.id)

    def _write_docstring(self, node):
        self.fill(allow_semicolon=False)
        if node.kind == "u":
            self.write("u")
        self._write_str_avoiding_backslashes(node.value, quote_types=_MULTI_QUOTES)

    def _write_constant(self, value):
        if isinstance(value, (float, complex)):
            # Substitute overflowing decimal literal for AST infinities,
            # and inf - inf for NaNs.
            self.write(
                repr(value)
                .replace("inf", _INFSTR)
                .replace("nan", f"({_INFSTR}-{_INFSTR})")
            )
        else:
            self.write(repr(value))

    def visit_Constant(self, node):
        value = node.value
        if isinstance(value, tuple):
            with self.delimit("(", ")"):
                self.items_view(self._write_constant, value)
        elif value is ...:
            self.write("...")
        else:
            if node.kind == "u":
                self.write("u")
            self._write_constant(node.value)

    def visit_List(self, node):
        with self.delimit("[", "]"):
            self.interleave(lambda: self.write(", "), self.traverse, node.elts)

    def visit_ListComp(self, node):
        with self.delimit("[", "]"):
            self.traverse(node.elt)
            for gen in node.generators:
                self.traverse(gen)

    def visit_GeneratorExp(self, node):
        with self.delimit("(", ")"):
            self.traverse(node.elt)
            for gen in node.generators:
                self.traverse(gen)

    def visit_SetComp(self, node):
        with self.delimit("{", "}"):
            self.traverse(node.elt)
            for gen in node.generators:
                self.traverse(gen)

    def visit_DictComp(self, node):
        with self.delimit("{", "}"):
            self.traverse(node.key)
            self.write(": ")
            self.traverse(node.value)
            for gen in node.generators:
                self.traverse(gen)

    def visit_comprehension(self, node):
        if node.is_async:
            self.write(" async for ")
        else:
            self.write(" for ")
        self.set_precedence(_Precedence.TUPLE, node.target)
        self.traverse(node.target)
        self.write(" in ")
        self.set_precedence(_Precedence.TEST.next(), node.iter, *node.ifs)
        self.traverse(node.iter)
        for if_clause in node.ifs:
            self.write(" if ")
            self.traverse(if_clause)

    def visit_IfExp(self, node):
        with self.require_parens(_Precedence.TEST, node):
            self.set_precedence(_Precedence.TEST.next(), node.body, node.test)
            self.traverse(node.body)
            self.write(" if ")
            self.traverse(node.test)
            self.write(" else ")
            self.set_precedence(_Precedence.TEST, node.orelse)
            self.traverse(node.orelse)

    def visit_Set(self, node):
        if node.elts:
            with self.delimit("{", "}"):
                self.interleave(lambda: self.write(", "), self.traverse, node.elts)
        else:
            # `{}` would be interpreted as a dictionary literal, and
            # `set` might be shadowed. Thus:
            self.write('{*()}')

    def visit_Dict(self, node):
        def write_key_value_pair(k, v):
            self.traverse(k)
            self.write(": ")
            self.traverse(v)

        def write_item(item):
            k, v = item
            if k is None:
                # for dictionary unpacking operator in dicts {**{'y': 2}}
                # see PEP 448 for details
                self.write("**")
                self.set_precedence(_Precedence.EXPR, v)
                self.traverse(v)
            else:
                write_key_value_pair(k, v)

        with self.delimit("{", "}"):
            self.interleave(
                lambda: self.write(", "), write_item, zip(node.keys, node.values)
            )

    def visit_Tuple(self, node):
        with self.delimit_if(
            "(",
            ")",
            len(node.elts) == 0 or self.get_precedence(node) > _Precedence.TUPLE
        ):
            self.items_view(self.traverse, node.elts)

    unop = {"Invert": "~", "Not": "not", "UAdd": "+", "USub": "-"}
    unop_precedence = {
        "not": _Precedence.NOT,
        "~": _Precedence.FACTOR,
        "+": _Precedence.FACTOR,
        "-": _Precedence.FACTOR,
    }

    def visit_UnaryOp(self, node):
        operator = self.unop[node.op.__class__.__name__]
        operator_precedence = self.unop_precedence[operator]
        with self.require_parens(operator_precedence, node):
            self.write(operator)
            # factor prefixes (+, -, ~) shouldn't be separated
            # from the value they belong, (e.g: +1 instead of + 1)
            if operator_precedence is not _Precedence.FACTOR:
                self.write(" ")
            self.set_precedence(operator_precedence, node.operand)
            self.traverse(node.operand)

    binop = {
        "Add": "+",
        "Sub": "-",
        "Mult": "*",
        "MatMult": "@",
        "Div": "/",
        "Mod": "%",
        "LShift": "<<",
        "RShift": ">>",
        "BitOr": "|",
        "BitXor": "^",
        "BitAnd": "&",
        "FloorDiv": "//",
        "Pow": "**",
    }

    binop_precedence = {
        "+": _Precedence.ARITH,
        "-": _Precedence.ARITH,
        "*": _Precedence.TERM,
        "@": _Precedence.TERM,
        "/": _Precedence.TERM,
        "%": _Precedence.TERM,
        "<<": _Precedence.SHIFT,
        ">>": _Precedence.SHIFT,
        "|": _Precedence.BOR,
        "^": _Precedence.BXOR,
        "&": _Precedence.BAND,
        "//": _Precedence.TERM,
        "**": _Precedence.POWER,
    }

    binop_rassoc = frozenset(("**",))
    def visit_BinOp(self, node):
        operator = self.binop[node.op.__class__.__name__]
        operator_precedence = self.binop_precedence[operator]
        with self.require_parens(operator_precedence, node):
            if operator in self.binop_rassoc:
                left_precedence = operator_precedence.next()
                right_precedence = operator_precedence
            else:
                left_precedence = operator_precedence
                right_precedence = operator_precedence.next()

            self.set_precedence(left_precedence, node.left)
            self.traverse(node.left)
            self.write(f" {operator} ")
            self.set_precedence(right_precedence, node.right)
            self.traverse(node.right)

    cmpops = {
        "Eq": "==",
        "NotEq": "!=",
        "Lt": "<",
        "LtE": "<=",
        "Gt": ">",
        "GtE": ">=",
        "Is": "is",
        "IsNot": "is not",
        "In": "in",
        "NotIn": "not in",
    }

    def visit_Compare(self, node):
        with self.require_parens(_Precedence.CMP, node):
            self.set_precedence(_Precedence.CMP.next(), node.left, *node.comparators)
            self.traverse(node.left)
            for o, e in zip(node.ops, node.comparators):
                self.write(" " + self.cmpops[o.__class__.__name__] + " ")
                self.traverse(e)

    boolops = {"And": "and", "Or": "or"}
    boolop_precedence = {"and": _Precedence.AND, "or": _Precedence.OR}

    def visit_BoolOp(self, node):
        operator = self.boolops[node.op.__class__.__name__]
        operator_precedence = self.boolop_precedence[operator]

        def increasing_level_traverse(node):
            nonlocal operator_precedence
            operator_precedence = operator_precedence.next()
            self.set_precedence(operator_precedence, node)
            self.traverse(node)

        with self.require_parens(operator_precedence, node):
            s = f" {operator} "
            self.interleave(lambda: self.write(s), increasing_level_traverse, node.values)

    def visit_Attribute(self, node):
        self.set_precedence(_Precedence.ATOM, node.value)
        self.traverse(node.value)
        # Special case: 3.__abs__() is a syntax error, so if node.value
        # is an integer literal then we need to either parenthesize
        # it or add an extra space to get 3 .__abs__().
        if isinstance(node.value, Constant) and isinstance(node.value.value, int):
            self.write(" ")
        self.write(".")
        self.write(node.attr)

    def visit_Call(self, node):
        self.set_precedence(_Precedence.ATOM, node.func)
        self.traverse(node.func)
        with self.delimit("(", ")"):
            comma = False
            for e in node.args:
                if comma:
                    self.write(", ")
                else:
                    comma = True
                self.traverse(e)
            for e in node.keywords:
                if comma:
                    self.write(", ")
                else:
                    comma = True
                self.traverse(e)

    def visit_Subscript(self, node):
        def is_non_empty_tuple(slice_value):
            return (
                isinstance(slice_value, Tuple)
                and slice_value.elts
            )

        self.set_precedence(_Precedence.ATOM, node.value)
        self.traverse(node.value)
        with self.delimit("[", "]"):
            if is_non_empty_tuple(node.slice):
                # parentheses can be omitted if the tuple isn't empty
                self.items_view(self.traverse, node.slice.elts)
            else:
                self.traverse(node.slice)

    def visit_Starred(self, node):
        self.write("*")
        self.set_precedence(_Precedence.EXPR, node.value)
        self.traverse(node.value)

    def visit_Ellipsis(self, node):
        self.write("...")

    def visit_Slice(self, node):
        if node.lower:
            self.traverse(node.lower)
        self.write(":")
        if node.upper:
            self.traverse(node.upper)
        if node.step:
            self.write(":")
            self.traverse(node.step)

    def visit_Match(self, node):
        self.fill("match ", allow_semicolon=False)
        self.traverse(node.subject)
        with self.block():
            for case in node.cases:
                self.traverse(case)

    def visit_arg(self, node):
        self.write(node.arg)
        if node.annotation:
            self.write(": ")
            self.traverse(node.annotation)

    def visit_arguments(self, node):
        first = True
        # normal arguments
        all_args = node.posonlyargs + node.args
        defaults = [None] * (len(all_args) - len(node.defaults)) + node.defaults
        for index, elements in enumerate(zip(all_args, defaults), 1):
            a, d = elements
            if first:
                first = False
            else:
                self.write(", ")
            self.traverse(a)
            if d:
                self.write("=")
                self.traverse(d)
            if index == len(node.posonlyargs):
                self.write(", /")

        # varargs, or bare '*' if no varargs but keyword-only arguments present
        if node.vararg or node.kwonlyargs:
            if first:
                first = False
            else:
                self.write(", ")
            self.write("*")
            if node.vararg:
                self.write(node.vararg.arg)
                if node.vararg.annotation:
                    self.write(": ")
                    self.traverse(node.vararg.annotation)

        # keyword-only arguments
        if node.kwonlyargs:
            for a, d in zip(node.kwonlyargs, node.kw_defaults):
                self.write(", ")
                self.traverse(a)
                if d:
                    self.write("=")
                    self.traverse(d)

        # kwargs
        if node.kwarg:
            if first:
                first = False
            else:
                self.write(", ")
            self.write("**" + node.kwarg.arg)
            if node.kwarg.annotation:
                self.write(": ")
                self.traverse(node.kwarg.annotation)

    def visit_keyword(self, node):
        if node.arg is None:
            self.write("**")
        else:
            self.write(node.arg)
            self.write("=")
        self.traverse(node.value)

    def visit_Lambda(self, node):
        with self.require_parens(_Precedence.TEST, node):
            self.write("lambda")
            with self.buffered() as buffer:
                self.traverse(node.args)
            if buffer:
                self.write(" ", *buffer)
            self.write(": ")
            self.set_precedence(_Precedence.TEST, node.body)
            self.traverse(node.body)

    def visit_alias(self, node):
        self.write(node.name)
        if node.asname:
            self.write(" as " + node.asname)

    def visit_withitem(self, node):
        self.traverse(node.context_expr)
        if node.optional_vars:
            self.write(" as ")
            self.traverse(node.optional_vars)

    def visit_match_case(self, node):
        self.fill("case ", allow_semicolon=False)
        self.traverse(node.pattern)
        if node.guard:
            self.write(" if ")
            self.traverse(node.guard)
        with self.block():
            self.traverse(node.body)

    def visit_MatchValue(self, node):
        self.traverse(node.value)

    def visit_MatchSingleton(self, node):
        self._write_constant(node.value)

    def visit_MatchSequence(self, node):
        with self.delimit("[", "]"):
            self.interleave(
                lambda: self.write(", "), self.traverse, node.patterns
            )

    def visit_MatchStar(self, node):
        name = node.name
        if name is None:
            name = "_"
        self.write(f"*{name}")

    def visit_MatchMapping(self, node):
        def write_key_pattern_pair(pair):
            k, p = pair
            self.traverse(k)
            self.write(": ")
            self.traverse(p)

        with self.delimit("{", "}"):
            keys = node.keys
            self.interleave(
                lambda: self.write(", "),
                write_key_pattern_pair,
                zip(keys, node.patterns, strict=True),
            )
            rest = node.rest
            if rest is not None:
                if keys:
                    self.write(", ")
                self.write(f"**{rest}")

    def visit_MatchClass(self, node):
        self.set_precedence(_Precedence.ATOM, node.cls)
        self.traverse(node.cls)
        with self.delimit("(", ")"):
            patterns = node.patterns
            self.interleave(
                lambda: self.write(", "), self.traverse, patterns
            )
            attrs = node.kwd_attrs
            if attrs:
                def write_attr_pattern(pair):
                    attr, pattern = pair
                    self.write(f"{attr}=")
                    self.traverse(pattern)

                if patterns:
                    self.write(", ")
                self.interleave(
                    lambda: self.write(", "),
                    write_attr_pattern,
                    zip(attrs, node.kwd_patterns, strict=True),
                )

    def visit_MatchAs(self, node):
        name = node.name
        pattern = node.pattern
        if name is None:
            self.write("_")
        elif pattern is None:
            self.write(node.name)
        else:
            with self.require_parens(_Precedence.TEST, node):
                self.set_precedence(_Precedence.BOR, node.pattern)
                self.traverse(node.pattern)
                self.write(f" as {node.name}")

    def visit_MatchOr(self, node):
        with self.require_parens(_Precedence.BOR, node):
            self.set_precedence(_Precedence.BOR.next(), *node.patterns)
            self.interleave(lambda: self.write(" | "), self.traverse, node.patterns)
