import ast

from . import _SLOTS


class _Stringifier:
    # Must match the slots on ForwardRef, so we can turn an instance of one into an
    # instance of the other in place.
    __slots__ = _SLOTS

    def __init__(
        self,
        node,
        globals=None,
        owner=None,
        is_class=False,
        cell=None,
        *,
        stringifier_dict,
    ):
        # Either an AST node or a simple str (for the common case where a ForwardRef
        # represent a single name).
        assert isinstance(node, (ast.AST, str))
        self.__arg__ = None
        self.__forward_evaluated__ = False
        self.__forward_value__ = None
        self.__forward_is_argument__ = False
        self.__forward_is_class__ = is_class
        self.__forward_module__ = None
        self.__code__ = None
        self.__ast_node__ = node
        self.__globals__ = globals
        self.__cell__ = cell
        self.__owner__ = owner
        self.__stringifier_dict__ = stringifier_dict

    def __convert_to_ast(self, other):
        if isinstance(other, _Stringifier):
            if isinstance(other.__ast_node__, str):
                return ast.Name(id=other.__ast_node__)
            return other.__ast_node__
        elif isinstance(other, slice):
            return ast.Slice(
                lower=(
                    self.__convert_to_ast(other.start)
                    if other.start is not None
                    else None
                ),
                upper=(
                    self.__convert_to_ast(other.stop)
                    if other.stop is not None
                    else None
                ),
                step=(
                    self.__convert_to_ast(other.step)
                    if other.step is not None
                    else None
                ),
            )
        else:
            return ast.Constant(value=other)

    def __get_ast(self):
        node = self.__ast_node__
        if isinstance(node, str):
            return ast.Name(id=node)
        return node

    def __make_new(self, node):
        stringifier = _Stringifier(
            node,
            self.__globals__,
            self.__owner__,
            self.__forward_is_class__,
            stringifier_dict=self.__stringifier_dict__,
        )
        self.__stringifier_dict__.stringifiers.append(stringifier)
        return stringifier

    # Must implement this since we set __eq__. We hash by identity so that
    # stringifiers in dict keys are kept separate.
    def __hash__(self):
        return id(self)

    def __getitem__(self, other):
        # Special case, to avoid stringifying references to class-scoped variables
        # as '__classdict__["x"]'.
        if self.__ast_node__ == "__classdict__":
            raise KeyError
        if isinstance(other, tuple):
            elts = [self.__convert_to_ast(elt) for elt in other]
            other = ast.Tuple(elts)
        else:
            other = self.__convert_to_ast(other)
        assert isinstance(other, ast.AST), repr(other)
        return self.__make_new(ast.Subscript(self.__get_ast(), other))

    def __getattr__(self, attr):
        return self.__make_new(ast.Attribute(self.__get_ast(), attr))

    def __call__(self, *args, **kwargs):
        return self.__make_new(
            ast.Call(
                self.__get_ast(),
                [self.__convert_to_ast(arg) for arg in args],
                [
                    ast.keyword(key, self.__convert_to_ast(value))
                    for key, value in kwargs.items()
                ],
            )
        )

    def __iter__(self):
        yield self.__make_new(ast.Starred(self.__get_ast()))

    def __repr__(self):
        if isinstance(self.__ast_node__, str):
            return self.__ast_node__
        return ast.unparse(self.__ast_node__)

    def __format__(self, format_spec):
        raise TypeError("Cannot stringify annotation containing string formatting")

    def _make_binop(op: ast.AST):
        def binop(self, other):
            return self.__make_new(
                ast.BinOp(self.__get_ast(), op, self.__convert_to_ast(other))
            )

        return binop

    __add__ = _make_binop(ast.Add())
    __sub__ = _make_binop(ast.Sub())
    __mul__ = _make_binop(ast.Mult())
    __matmul__ = _make_binop(ast.MatMult())
    __truediv__ = _make_binop(ast.Div())
    __mod__ = _make_binop(ast.Mod())
    __lshift__ = _make_binop(ast.LShift())
    __rshift__ = _make_binop(ast.RShift())
    __or__ = _make_binop(ast.BitOr())
    __xor__ = _make_binop(ast.BitXor())
    __and__ = _make_binop(ast.BitAnd())
    __floordiv__ = _make_binop(ast.FloorDiv())
    __pow__ = _make_binop(ast.Pow())

    del _make_binop

    def _make_rbinop(op: ast.AST):
        def rbinop(self, other):
            return self.__make_new(
                ast.BinOp(self.__convert_to_ast(other), op, self.__get_ast())
            )

        return rbinop

    __radd__ = _make_rbinop(ast.Add())
    __rsub__ = _make_rbinop(ast.Sub())
    __rmul__ = _make_rbinop(ast.Mult())
    __rmatmul__ = _make_rbinop(ast.MatMult())
    __rtruediv__ = _make_rbinop(ast.Div())
    __rmod__ = _make_rbinop(ast.Mod())
    __rlshift__ = _make_rbinop(ast.LShift())
    __rrshift__ = _make_rbinop(ast.RShift())
    __ror__ = _make_rbinop(ast.BitOr())
    __rxor__ = _make_rbinop(ast.BitXor())
    __rand__ = _make_rbinop(ast.BitAnd())
    __rfloordiv__ = _make_rbinop(ast.FloorDiv())
    __rpow__ = _make_rbinop(ast.Pow())

    del _make_rbinop

    def _make_compare(op):
        def compare(self, other):
            return self.__make_new(
                ast.Compare(
                    left=self.__get_ast(),
                    ops=[op],
                    comparators=[self.__convert_to_ast(other)],
                )
            )

        return compare

    __lt__ = _make_compare(ast.Lt())
    __le__ = _make_compare(ast.LtE())
    __eq__ = _make_compare(ast.Eq())
    __ne__ = _make_compare(ast.NotEq())
    __gt__ = _make_compare(ast.Gt())
    __ge__ = _make_compare(ast.GtE())

    del _make_compare

    def _make_unary_op(op):
        def unary_op(self):
            return self.__make_new(ast.UnaryOp(op, self.__get_ast()))

        return unary_op

    __invert__ = _make_unary_op(ast.Invert())
    __pos__ = _make_unary_op(ast.UAdd())
    __neg__ = _make_unary_op(ast.USub())

    del _make_unary_op
