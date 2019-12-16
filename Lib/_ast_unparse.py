import sys
from ast import And, Constant, If, Name, NodeVisitor, Or
from contextlib import contextmanager

# Large float and imaginary literals get turned into infinities in the AST.
# We unparse those infinities to INFSTR.
_INFSTR = "1e" + repr(sys.float_info.max_10_exp + 1)


class _Unparser(NodeVisitor):
    """Methods in this class recursively traverse an AST and
    output source code for the abstract syntax; original formatting
    is disregarded."""

    def __init__(self):
        self._source = []
        self._buffer = []
        self._indent = 0

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

    def fill(self, text=""):
        """Indent a piece of text and append it, according to the current
        indentation level"""
        self.write("\n" + "    " * self._indent + text)

    def write(self, text):
        """Append a piece of text"""
        self._source.append(text)

    def buffer_writer(self, text):
        self._buffer.append(text)

    @property
    def buffer(self):
        value = "".join(self._buffer)
        self._buffer.clear()
        return value

    @contextmanager
    def block(self):
        """A context manager for preparing the source for blocks. It adds
        the character':', increases the indentation on enter and decreases
        the indentation on exit."""
        self.write(":")
        self._indent += 1
        yield
        self._indent -= 1

    def traverse(self, node):
        if isinstance(node, list):
            for item in node:
                self.traverse(item)
        else:
            super().visit(node)

    def visit(self, node):
        """Outputs a source code string that, if converted back to an ast
        (using ast.parse) will generate an AST equivalent to *node*"""
        self._source = []
        self.traverse(node)
        return "".join(self._source)

    def visit_Module(self, node):
        for subnode in node.body:
            self.traverse(subnode)

    def visit_Expr(self, node):
        self.fill()
        self.traverse(node.value)

    def visit_NamedExpr(self, node):
        self.write("(")
        self.traverse(node.target)
        self.write(" := ")
        self.traverse(node.value)
        self.write(")")

    def visit_Import(self, node):
        self.fill("import ")
        self.interleave(lambda: self.write(", "), self.traverse, node.names)

    def visit_ImportFrom(self, node):
        self.fill("from ")
        self.write("." * node.level)
        if node.module:
            self.write(node.module)
        self.write(" import ")
        self.interleave(lambda: self.write(", "), self.traverse, node.names)

    def visit_Assign(self, node):
        self.fill()
        for target in node.targets:
            self.traverse(target)
            self.write(" = ")
        self.traverse(node.value)

    def visit_AugAssign(self, node):
        self.fill()
        self.traverse(node.target)
        self.write(" " + self.binop[node.op.__class__.__name__] + "= ")
        self.traverse(node.value)

    def visit_AnnAssign(self, node):
        self.fill()
        if not node.simple and isinstance(node.target, Name):
            self.write("(")
        self.traverse(node.target)
        if not node.simple and isinstance(node.target, Name):
            self.write(")")
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
        self.write("(")
        self.write("await")
        if node.value:
            self.write(" ")
            self.traverse(node.value)
        self.write(")")

    def visit_Yield(self, node):
        self.write("(")
        self.write("yield")
        if node.value:
            self.write(" ")
            self.traverse(node.value)
        self.write(")")

    def visit_YieldFrom(self, node):
        self.write("(")
        self.write("yield from")
        if node.value:
            self.write(" ")
            self.traverse(node.value)
        self.write(")")

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

    def visit_Try(self, node):
        self.fill("try")
        with self.block():
            self.traverse(node.body)
        for ex in node.handlers:
            self.traverse(ex)
        if node.orelse:
            self.fill("else")
            with self.block():
                self.traverse(node.orelse)
        if node.finalbody:
            self.fill("finally")
            with self.block():
                self.traverse(node.finalbody)

    def visit_ExceptHandler(self, node):
        self.fill("except")
        if node.type:
            self.write(" ")
            self.traverse(node.type)
        if node.name:
            self.write(" as ")
            self.write(node.name)
        with self.block():
            self.traverse(node.body)

    def visit_ClassDef(self, node):
        self.write("\n")
        for deco in node.decorator_list:
            self.fill("@")
            self.traverse(deco)
        self.fill("class " + node.name)
        self.write("(")
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
        self.write(")")

        with self.block():
            self.traverse(node.body)

    def visit_FunctionDef(self, node):
        self.__FunctionDef_helper(node, "def")

    def visit_AsyncFunctionDef(self, node):
        self.__FunctionDef_helper(node, "async def")

    def __FunctionDef_helper(self, node, fill_suffix):
        self.write("\n")
        for deco in node.decorator_list:
            self.fill("@")
            self.traverse(deco)
        def_str = fill_suffix + " " + node.name + "("
        self.fill(def_str)
        self.traverse(node.args)
        self.write(")")
        if node.returns:
            self.write(" -> ")
            self.traverse(node.returns)
        with self.block():
            self.traverse(node.body)

    def visit_For(self, node):
        self.__For_helper("for ", node)

    def visit_AsyncFor(self, node):
        self.__For_helper("async for ", node)

    def __For_helper(self, fill, node):
        self.fill(fill)
        self.traverse(node.target)
        self.write(" in ")
        self.traverse(node.iter)
        with self.block():
            self.traverse(node.body)
        if node.orelse:
            self.fill("else")
            with self.block():
                self.traverse(node.orelse)

    def visit_If(self, node):
        self.fill("if ")
        self.traverse(node.test)
        with self.block():
            self.traverse(node.body)
        # collapse nested ifs into equivalent elifs.
        while node.orelse and len(node.orelse) == 1 and isinstance(node.orelse[0], If):
            node = node.orelse[0]
            self.fill("elif ")
            self.traverse(node.test)
            with self.block():
                self.traverse(node.body)
        # final else
        if node.orelse:
            self.fill("else")
            with self.block():
                self.traverse(node.orelse)

    def visit_While(self, node):
        self.fill("while ")
        self.traverse(node.test)
        with self.block():
            self.traverse(node.body)
        if node.orelse:
            self.fill("else")
            with self.block():
                self.traverse(node.orelse)

    def visit_With(self, node):
        self.fill("with ")
        self.interleave(lambda: self.write(", "), self.traverse, node.items)
        with self.block():
            self.traverse(node.body)

    def visit_AsyncWith(self, node):
        self.fill("async with ")
        self.interleave(lambda: self.write(", "), self.traverse, node.items)
        with self.block():
            self.traverse(node.body)

    def visit_JoinedStr(self, node):
        self.write("f")
        self._fstring_JoinedStr(node, self.buffer_writer)
        self.write(repr(self.buffer))

    def visit_FormattedValue(self, node):
        self.write("f")
        self._fstring_FormattedValue(node, self.buffer_writer)
        self.write(repr(self.buffer))

    def _fstring_JoinedStr(self, node, write):
        for value in node.values:
            meth = getattr(self, "_fstring_" + type(value).__name__)
            meth(value, write)

    def _fstring_Constant(self, node, write):
        if not isinstance(node.value, str):
            raise ValueError("Constants inside JoinedStr should be a string.")
        value = node.value.replace("{", "{{").replace("}", "}}")
        write(value)

    def _fstring_FormattedValue(self, node, write):
        write("{")
        expr = type(self)().visit(node.value).rstrip("\n")
        if expr.startswith("{"):
            write(" ")  # Separate pair of opening brackets as "{ {"
        write(expr)
        if node.conversion != -1:
            conversion = chr(node.conversion)
            if conversion not in "sra":
                raise ValueError("Unknown f-string conversion.")
            write(f"!{conversion}")
        if node.format_spec:
            write(":")
            meth = getattr(self, "_fstring_" + type(node.format_spec).__name__)
            meth(node.format_spec, write)
        write("}")

    def visit_Name(self, node):
        self.write(node.id)

    def _write_constant(self, value):
        if isinstance(value, (float, complex)):
            # Substitute overflowing decimal literal for AST infinities.
            self.write(repr(value).replace("inf", _INFSTR))
        else:
            self.write(repr(value))

    def visit_Constant(self, node):
        value = node.value
        if isinstance(value, tuple):
            self.write("(")
            if len(value) == 1:
                self._write_constant(value[0])
                self.write(",")
            else:
                self.interleave(lambda: self.write(", "), self._write_constant, value)
            self.write(")")
        elif value is ...:
            self.write("...")
        else:
            if node.kind == "u":
                self.write("u")
            self._write_constant(node.value)

    def visit_List(self, node):
        self.write("[")
        self.interleave(lambda: self.write(", "), self.traverse, node.elts)
        self.write("]")

    def visit_ListComp(self, node):
        self.write("[")
        self.traverse(node.elt)
        for gen in node.generators:
            self.traverse(gen)
        self.write("]")

    def visit_GeneratorExp(self, node):
        self.write("(")
        self.traverse(node.elt)
        for gen in node.generators:
            self.traverse(gen)
        self.write(")")

    def visit_SetComp(self, node):
        self.write("{")
        self.traverse(node.elt)
        for gen in node.generators:
            self.traverse(gen)
        self.write("}")

    def visit_DictComp(self, node):
        self.write("{")
        self.traverse(node.key)
        self.write(": ")
        self.traverse(node.value)
        for gen in node.generators:
            self.traverse(gen)
        self.write("}")

    def visit_comprehension(self, node):
        if node.is_async:
            self.write(" async for ")
        else:
            self.write(" for ")
        self.traverse(node.target)
        self.write(" in ")
        self.traverse(node.iter)
        for if_clause in node.ifs:
            self.write(" if ")
            self.traverse(if_clause)

    def visit_IfExp(self, node):
        self.write("(")
        self.traverse(node.body)
        self.write(" if ")
        self.traverse(node.test)
        self.write(" else ")
        self.traverse(node.orelse)
        self.write(")")

    def visit_Set(self, node):
        if not node.elts:
            raise ValueError("Set node should has at least one item")
        self.write("{")
        self.interleave(lambda: self.write(", "), self.traverse, node.elts)
        self.write("}")

    def visit_Dict(self, node):
        self.write("{")

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
                self.traverse(v)
            else:
                write_key_value_pair(k, v)

        self.interleave(
            lambda: self.write(", "), write_item, zip(node.keys, node.values)
        )
        self.write("}")

    def visit_Tuple(self, node):
        self.write("(")
        if len(node.elts) == 1:
            elt = node.elts[0]
            self.traverse(elt)
            self.write(",")
        else:
            self.interleave(lambda: self.write(", "), self.traverse, node.elts)
        self.write(")")

    unop = {"Invert": "~", "Not": "not", "UAdd": "+", "USub": "-"}

    def visit_UnaryOp(self, node):
        self.write("(")
        self.write(self.unop[node.op.__class__.__name__])
        self.write(" ")
        self.traverse(node.operand)
        self.write(")")

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

    def visit_BinOp(self, node):
        self.write("(")
        self.traverse(node.left)
        self.write(" " + self.binop[node.op.__class__.__name__] + " ")
        self.traverse(node.right)
        self.write(")")

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
        self.write("(")
        self.traverse(node.left)
        for o, e in zip(node.ops, node.comparators):
            self.write(" " + self.cmpops[o.__class__.__name__] + " ")
            self.traverse(e)
        self.write(")")

    boolops = {And: "and", Or: "or"}

    def visit_BoolOp(self, node):
        self.write("(")
        s = " %s " % self.boolops[node.op.__class__]
        self.interleave(lambda: self.write(s), self.traverse, node.values)
        self.write(")")

    def visit_Attribute(self, node):
        self.traverse(node.value)
        # Special case: 3.__abs__() is a syntax error, so if node.value
        # is an integer literal then we need to either parenthesize
        # it or add an extra space to get 3 .__abs__().
        if isinstance(node.value, Constant) and isinstance(node.value.value, int):
            self.write(" ")
        self.write(".")
        self.write(node.attr)

    def visit_Call(self, node):
        self.traverse(node.func)
        self.write("(")
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
        self.write(")")

    def visit_Subscript(self, node):
        self.traverse(node.value)
        self.write("[")
        self.traverse(node.slice)
        self.write("]")

    def visit_Starred(self, node):
        self.write("*")
        self.traverse(node.value)

    def visit_Ellipsis(self, node):
        self.write("...")

    def visit_Index(self, node):
        self.traverse(node.value)

    def visit_Slice(self, node):
        if node.lower:
            self.traverse(node.lower)
        self.write(":")
        if node.upper:
            self.traverse(node.upper)
        if node.step:
            self.write(":")
            self.traverse(node.step)

    def visit_ExtSlice(self, node):
        self.interleave(lambda: self.write(", "), self.traverse, node.dims)

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
        self.write("(")
        self.write("lambda ")
        self.traverse(node.args)
        self.write(": ")
        self.traverse(node.body)
        self.write(")")

    def visit_alias(self, node):
        self.write(node.name)
        if node.asname:
            self.write(" as " + node.asname)

    def visit_withitem(self, node):
        self.traverse(node.context_expr)
        if node.optional_vars:
            self.write(" as ")
            self.traverse(node.optional_vars)


def unparse(ast_obj):
    unparser = _Unparser()
    return unparser.visit(ast_obj)
