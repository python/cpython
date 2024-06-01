"""Helpers for introspecting and wrapping annotations."""

import ast
import enum
import functools
import sys
import types


class Format(enum.IntEnum):
    VALUE = 1
    FORWARDREF = 2
    SOURCE = 3


_Union = None


class ForwardRef:
    """Internal wrapper to hold a forward reference."""

    __slots__ = (
        "__forward_arg__",
        "__forward_evaluated__",
        "__forward_value__",
        "__forward_is_argument__",
        "__forward_is_class__",
        "__forward_module__",
        "__weakref__",
        "_forward_code",
        "_globals",
        "_owner",
        "_cell",
    )

    def __init__(
        self,
        arg,
        is_argument=True,
        module=None,
        *,
        is_class=False,
        _globals=None,
        _owner=None,
        _cell=None,
    ):
        if not isinstance(arg, str):
            raise TypeError(f"Forward reference must be a string -- got {arg!r}")

        self.__forward_arg__ = arg
        self.__forward_evaluated__ = False
        self.__forward_value__ = None
        self.__forward_is_argument__ = is_argument
        self.__forward_is_class__ = is_class
        self.__forward_module__ = module
        self._forward_code = None
        self._globals = _globals
        self._cell = _cell
        self._owner = _owner

    def __init_subclass__(cls, /, *args, **kwds):
        raise TypeError("Cannot subclass ForwardRef")

    def evaluate(self, *, globals=None, locals=None):
        """Evaluate the forward reference and return the value.

        If the forward reference is not evaluatable, raise a SyntaxError.
        """
        if self.__forward_evaluated__:
            return self.__forward_value__
        if self._cell is not None:
            try:
                value = self._cell.cell_contents
            except ValueError:
                pass
            else:
                self.__forward_evaluated__ = True
                self.__forward_value__ = value
                return value

        code = self.__forward_code__
        if globals is None:
            globals = self._globals
        if globals is None:
            globals = {}
        if locals is None:
            locals = {}
            if isinstance(self._owner, type):
                locals.update(vars(self._owner))
        if self._owner is not None:
            # "Inject" type parameters into the local namespace
            # (unless they are shadowed by assignments *in* the local namespace),
            # as a way of emulating annotation scopes when calling `eval()`
            type_params = getattr(self._owner, "__type_params__", None)
            if type_params:
                locals = {param.__name__: param for param in type_params} | locals
        value = eval(code, globals=globals, locals=locals)
        self.__forward_evaluated__ = True
        self.__forward_value__ = value
        return value

    @property
    def __forward_code__(self):
        if self._forward_code is not None:
            return self._forward_code
        arg = self.__forward_arg__
        # If we do `def f(*args: *Ts)`, then we'll have `arg = '*Ts'`.
        # Unfortunately, this isn't a valid expression on its own, so we
        # do the unpacking manually.
        if arg.startswith("*"):
            arg_to_compile = f"({arg},)[0]"  # E.g. (*Ts,)[0] or (*tuple[int, int],)[0]
        else:
            arg_to_compile = arg
        try:
            self._forward_code = compile(arg_to_compile, "<string>", "eval")
        except SyntaxError:
            raise SyntaxError(f"Forward reference must be an expression -- got {arg!r}")
        return self._forward_code

    def __eq__(self, other):
        if not isinstance(other, ForwardRef):
            return NotImplemented
        if self.__forward_evaluated__ and other.__forward_evaluated__:
            return (
                self.__forward_arg__ == other.__forward_arg__
                and self.__forward_value__ == other.__forward_value__
            )
        return (
            self.__forward_arg__ == other.__forward_arg__
            and self.__forward_module__ == other.__forward_module__
        )

    def __hash__(self):
        return hash((self.__forward_arg__, self.__forward_module__))

    def __or__(self, other):
        global _Union
        if _Union is None:
            from typing import Union as _Union
        return _Union[self, other]

    def __ror__(self, other):
        global _Union
        if _Union is None:
            from typing import Union as _Union
        return _Union[other, self]

    def __repr__(self):
        if self.__forward_module__ is None:
            module_repr = ""
        else:
            module_repr = f", module={self.__forward_module__!r}"
        return f"ForwardRef({self.__forward_arg__!r}{module_repr})"


class _ForwardReffer(dict):
    def __init__(self, namespace, globals, owner, is_class):
        super().__init__(namespace)
        self.namespace = namespace
        self.globals = globals
        self.owner = owner
        self.is_class = is_class

    def __missing__(self, key):
        return ForwardRef(
            key, _globals=self.globals, _owner=self.owner, is_class=self.is_class
        )


class Stringifier:
    def __init__(self, node):
        self.node = node

    def _convert(self, other):
        if isinstance(other, Stringifier):
            return other.node
        else:
            return ast.Name(id=repr(other))

    def _make_binop(op: ast.AST):
        def binop(self, other):
            return Stringifier(ast.BinOp(self.node, op, self._convert(other)))

        return binop

    __add__ = _make_binop(ast.Add())
    __sub__ = _make_binop(ast.Sub())
    __mul__ = _make_binop(ast.Mult())
    __matmul__ = _make_binop(ast.MatMult())
    __div__ = _make_binop(ast.Div())
    __mod__ = _make_binop(ast.Mod())
    __lshift__ = _make_binop(ast.LShift())
    __rshift__ = _make_binop(ast.RShift())
    __or__ = _make_binop(ast.BitOr())
    __xor__ = _make_binop(ast.BitXor())
    __and__ = _make_binop(ast.And())
    __floordiv__ = _make_binop(ast.FloorDiv())
    __pow__ = _make_binop(ast.Pow())

    def _make_unary_op(op):
        def unary_op(self):
            return Stringifier(ast.UnaryOp(self.node, op))

        return unary_op

    __invert__ = _make_unary_op(ast.Invert())
    __pos__ = _make_binop(ast.UAdd())
    __neg__ = _make_binop(ast.USub())

    def __getitem__(self, other):
        if isinstance(other, tuple):
            elts = [self._convert(elt) for elt in other]
            other = ast.Tuple(elts)
        else:
            other = self._convert(other)
        return Stringifier(ast.Subscript(self.node, other))

    def __getattr__(self, attr):
        return Stringifier(ast.Attribute(self.node, attr))

    def __call__(self, *args, **kwargs):
        return Stringifier(
            ast.Call(
                self.node,
                [self._convert(arg) for arg in args],
                [
                    ast.keyword(key, self._convert(value))
                    for key, value in kwargs.items()
                ],
            )
        )

    def __iter__(self):
        return self

    def __next__(self):
        return Stringifier(ast.Starred(self.node))


class _StringifierDict(dict):
    def __missing__(self, key):
        return Stringifier(ast.Name(key))


def _call_dunder_annotate(annotate, format, owner=None):
    try:
        return annotate(format)
    except NotImplementedError:
        pass
    if format == Format.FORWARDREF:
        namespace = {**annotate.__builtins__, **annotate.__globals__}
        is_class = isinstance(owner, type)
        globals = _ForwardReffer(namespace, annotate.__globals__, owner, is_class)
        if annotate.__closure__:
            freevars = annotate.__code__.co_freevars
            new_closure = []
            for i, cell in enumerate(annotate.__closure__):
                try:
                    cell.cell_contents
                except ValueError:
                    if i < len(freevars):
                        name = freevars[i]
                    else:
                        name = "__cell__"
                    fwdref = ForwardRef(
                        name,
                        _cell=cell,
                        _owner=owner,
                        _globals=annotate.__globals__,
                        is_class=is_class,
                    )
                    new_closure.append(types.CellType(fwdref))
                else:
                    new_closure.append(cell)
            closure = tuple(new_closure)
        else:
            closure = None
        func = types.FunctionType(annotate.__code__, globals, closure=closure)
        return func(Format.VALUE)
    elif format == Format.SOURCE:
        globals = _StringifierDict()
        func = types.FunctionType(
            annotate.__code__,
            globals,
            # TODO: also replace the closure with stringifiers
            closure=annotate.__closure__,
        )
        annos = func(Format.VALUE)
        return {
            key: ast.unparse(val.node) if isinstance(val, Stringifier) else repr(val)
            for key, val in annos.items()
        }


def get_annotations(
    obj, *, globals=None, locals=None, eval_str=False, format=Format.VALUE
):
    """Compute the annotations dict for an object.

    obj may be a callable, class, or module.
    Passing in an object of any other type raises TypeError.

    Returns a dict.  get_annotations() returns a new dict every time
    it's called; calling it twice on the same object will return two
    different but equivalent dicts.

    This function handles several details for you:

      * If eval_str is true, values of type str will
        be un-stringized using eval().  This is intended
        for use with stringized annotations
        ("from __future__ import annotations").
      * If obj doesn't have an annotations dict, returns an
        empty dict.  (Functions and methods always have an
        annotations dict; classes, modules, and other types of
        callables may not.)
      * Ignores inherited annotations on classes.  If a class
        doesn't have its own annotations dict, returns an empty dict.
      * All accesses to object members and dict values are done
        using getattr() and dict.get() for safety.
      * Always, always, always returns a freshly-created dict.

    eval_str controls whether or not values of type str are replaced
    with the result of calling eval() on those values:

      * If eval_str is true, eval() is called on values of type str.
      * If eval_str is false (the default), values of type str are unchanged.

    globals and locals are passed in to eval(); see the documentation
    for eval() for more information.  If either globals or locals is
    None, this function may replace that value with a context-specific
    default, contingent on type(obj):

      * If obj is a module, globals defaults to obj.__dict__.
      * If obj is a class, globals defaults to
        sys.modules[obj.__module__].__dict__ and locals
        defaults to the obj class namespace.
      * If obj is a callable, globals defaults to obj.__globals__,
        although if obj is a wrapped function (using
        functools.update_wrapper()) it is first unwrapped.
    """
    annotate = getattr(obj, "__annotate__", None)
    # TODO remove format != VALUE condition
    if annotate is not None and format != Format.VALUE:
        ann = _call_dunder_annotate(annotate, format)
    elif isinstance(obj, type):
        # class
        ann = getattr(obj, '__annotations__', None)

        obj_globals = None
        module_name = getattr(obj, "__module__", None)
        if module_name:
            module = sys.modules.get(module_name, None)
            if module:
                obj_globals = getattr(module, "__dict__", None)
        obj_locals = dict(vars(obj))
        unwrap = obj
    elif isinstance(obj, types.ModuleType):
        # module
        ann = getattr(obj, "__annotations__", None)
        obj_globals = getattr(obj, "__dict__")
        obj_locals = None
        unwrap = None
    elif callable(obj):
        # this includes types.Function, types.BuiltinFunctionType,
        # types.BuiltinMethodType, functools.partial, functools.singledispatch,
        # "class funclike" from Lib/test/test_inspect... on and on it goes.
        ann = getattr(obj, "__annotations__", None)
        obj_globals = getattr(obj, "__globals__", None)
        obj_locals = None
        unwrap = obj
    elif (ann := getattr(obj, "__annotations__", None)) is not None:
        obj_globals = obj_locals = unwrap = None
    else:
        raise TypeError(f"{obj!r} is not a module, class, or callable.")

    if ann is None:
        return {}

    if not isinstance(ann, dict):
        raise ValueError(f"{obj!r}.__annotations__ is neither a dict nor None")

    if not ann:
        return {}

    if not eval_str:
        return dict(ann)

    if unwrap is not None:
        while True:
            if hasattr(unwrap, "__wrapped__"):
                unwrap = unwrap.__wrapped__
                continue
            if isinstance(unwrap, functools.partial):
                unwrap = unwrap.func
                continue
            break
        if hasattr(unwrap, "__globals__"):
            obj_globals = unwrap.__globals__

    if globals is None:
        globals = obj_globals
    if locals is None:
        locals = obj_locals

    return_value = {
        key: value if not isinstance(value, str) else eval(value, globals, locals)
        for key, value in ann.items()
    }
    return return_value
