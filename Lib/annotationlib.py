"""Helpers for introspecting and wrapping annotations."""

import ast
import builtins
import enum
import keyword
import sys
import types

__all__ = [
    "Format",
    "ForwardRef",
    "call_annotate_function",
    "call_evaluate_function",
    "get_annotate_from_class_namespace",
    "get_annotations",
    "annotations_to_string",
    "type_repr",
]


class Format(enum.IntEnum):
    VALUE = 1
    VALUE_WITH_FAKE_GLOBALS = 2
    FORWARDREF = 3
    STRING = 4


_sentinel = object()

# Slots shared by ForwardRef and _Stringifier. The __forward__ names must be
# preserved for compatibility with the old typing.ForwardRef class. The remaining
# names are private.
_SLOTS = (
    "__forward_is_argument__",
    "__forward_is_class__",
    "__forward_module__",
    "__weakref__",
    "__arg__",
    "__globals__",
    "__extra_names__",
    "__code__",
    "__ast_node__",
    "__cell__",
    "__owner__",
    "__stringifier_dict__",
)


class ForwardRef:
    """Wrapper that holds a forward reference.

    Constructor arguments:
    * arg: a string representing the code to be evaluated.
    * module: the module where the forward reference was created.
      Must be a string, not a module object.
    * owner: The owning object (module, class, or function).
    * is_argument: Does nothing, retained for compatibility.
    * is_class: True if the forward reference was created in class scope.

    """

    __slots__ = _SLOTS

    def __init__(
        self,
        arg,
        *,
        module=None,
        owner=None,
        is_argument=True,
        is_class=False,
    ):
        if not isinstance(arg, str):
            raise TypeError(f"Forward reference must be a string -- got {arg!r}")

        self.__arg__ = arg
        self.__forward_is_argument__ = is_argument
        self.__forward_is_class__ = is_class
        self.__forward_module__ = module
        self.__owner__ = owner
        # These are always set to None here but may be non-None if a ForwardRef
        # is created through __class__ assignment on a _Stringifier object.
        self.__globals__ = None
        self.__cell__ = None
        self.__extra_names__ = None
        # These are initially None but serve as a cache and may be set to a non-None
        # value later.
        self.__code__ = None
        self.__ast_node__ = None

    def __init_subclass__(cls, /, *args, **kwds):
        raise TypeError("Cannot subclass ForwardRef")

    def evaluate(
        self,
        *,
        globals=None,
        locals=None,
        type_params=None,
        owner=None,
        format=Format.VALUE,
    ):
        """Evaluate the forward reference and return the value.

        If the forward reference cannot be evaluated, raise an exception.
        """
        match format:
            case Format.STRING:
                return self.__forward_arg__
            case Format.VALUE:
                is_forwardref_format = False
            case Format.FORWARDREF:
                is_forwardref_format = True
            case _:
                raise NotImplementedError(format)
        if self.__cell__ is not None:
            try:
                return self.__cell__.cell_contents
            except ValueError:
                pass
        if owner is None:
            owner = self.__owner__

        if globals is None and self.__forward_module__ is not None:
            globals = getattr(
                sys.modules.get(self.__forward_module__, None), "__dict__", None
            )
        if globals is None:
            globals = self.__globals__
        if globals is None:
            if isinstance(owner, type):
                module_name = getattr(owner, "__module__", None)
                if module_name:
                    module = sys.modules.get(module_name, None)
                    if module:
                        globals = getattr(module, "__dict__", None)
            elif isinstance(owner, types.ModuleType):
                globals = getattr(owner, "__dict__", None)
            elif callable(owner):
                globals = getattr(owner, "__globals__", None)

        # If we pass None to eval() below, the globals of this module are used.
        if globals is None:
            globals = {}

        if locals is None:
            locals = {}
            if isinstance(owner, type):
                locals.update(vars(owner))

        if type_params is None and owner is not None:
            # "Inject" type parameters into the local namespace
            # (unless they are shadowed by assignments *in* the local namespace),
            # as a way of emulating annotation scopes when calling `eval()`
            type_params = getattr(owner, "__type_params__", None)

        # type parameters require some special handling,
        # as they exist in their own scope
        # but `eval()` does not have a dedicated parameter for that scope.
        # For classes, names in type parameter scopes should override
        # names in the global scope (which here are called `localns`!),
        # but should in turn be overridden by names in the class scope
        # (which here are called `globalns`!)
        if type_params is not None:
            globals = dict(globals)
            locals = dict(locals)
            for param in type_params:
                param_name = param.__name__
                if not self.__forward_is_class__ or param_name not in globals:
                    globals[param_name] = param
                    locals.pop(param_name, None)
        if self.__extra_names__:
            locals = {**locals, **self.__extra_names__}

        arg = self.__forward_arg__
        if arg.isidentifier() and not keyword.iskeyword(arg):
            if arg in locals:
                return locals[arg]
            elif arg in globals:
                return globals[arg]
            elif hasattr(builtins, arg):
                return getattr(builtins, arg)
            elif is_forwardref_format:
                return self
            else:
                raise NameError(arg)
        else:
            code = self.__forward_code__
            try:
                return eval(code, globals=globals, locals=locals)
            except Exception:
                if not is_forwardref_format:
                    raise
            new_locals = _StringifierDict(
                {**builtins.__dict__, **locals},
                globals=globals,
                owner=owner,
                is_class=self.__forward_is_class__,
                format=format,
            )
            try:
                result = eval(code, globals=globals, locals=new_locals)
            except Exception:
                return self
            else:
                new_locals.transmogrify()
                return result

    def _evaluate(self, globalns, localns, type_params=_sentinel, *, recursive_guard):
        import typing
        import warnings

        if type_params is _sentinel:
            typing._deprecation_warning_for_no_type_params_passed(
                "typing.ForwardRef._evaluate"
            )
            type_params = ()
        warnings._deprecated(
            "ForwardRef._evaluate",
            "{name} is a private API and is retained for compatibility, but will be removed"
            " in Python 3.16. Use ForwardRef.evaluate() or typing.evaluate_forward_ref() instead.",
            remove=(3, 16),
        )
        return typing.evaluate_forward_ref(
            self,
            globals=globalns,
            locals=localns,
            type_params=type_params,
            _recursive_guard=recursive_guard,
        )

    @property
    def __forward_arg__(self):
        if self.__arg__ is not None:
            return self.__arg__
        if self.__ast_node__ is not None:
            self.__arg__ = ast.unparse(self.__ast_node__)
            return self.__arg__
        raise AssertionError(
            "Attempted to access '__forward_arg__' on an uninitialized ForwardRef"
        )

    @property
    def __forward_code__(self):
        if self.__code__ is not None:
            return self.__code__
        arg = self.__forward_arg__
        # If we do `def f(*args: *Ts)`, then we'll have `arg = '*Ts'`.
        # Unfortunately, this isn't a valid expression on its own, so we
        # do the unpacking manually.
        if arg.startswith("*"):
            arg_to_compile = f"({arg},)[0]"  # E.g. (*Ts,)[0] or (*tuple[int, int],)[0]
        else:
            arg_to_compile = arg
        try:
            self.__code__ = compile(arg_to_compile, "<string>", "eval")
        except SyntaxError:
            raise SyntaxError(f"Forward reference must be an expression -- got {arg!r}")
        return self.__code__

    def __eq__(self, other):
        if not isinstance(other, ForwardRef):
            return NotImplemented
        return (
            self.__forward_arg__ == other.__forward_arg__
            and self.__forward_module__ == other.__forward_module__
            # Use "is" here because we use id() for this in __hash__
            # because dictionaries are not hashable.
            and self.__globals__ is other.__globals__
            and self.__forward_is_class__ == other.__forward_is_class__
            and self.__cell__ == other.__cell__
            and self.__owner__ == other.__owner__
            and (
                (tuple(sorted(self.__extra_names__.items())) if self.__extra_names__ else None) ==
                (tuple(sorted(other.__extra_names__.items())) if other.__extra_names__ else None)
            )
        )

    def __hash__(self):
        return hash((
            self.__forward_arg__,
            self.__forward_module__,
            id(self.__globals__),  # dictionaries are not hashable, so hash by identity
            self.__forward_is_class__,
            self.__cell__,
            self.__owner__,
            tuple(sorted(self.__extra_names__.items())) if self.__extra_names__ else None,
        ))

    def __or__(self, other):
        return types.UnionType[self, other]

    def __ror__(self, other):
        return types.UnionType[other, self]

    def __repr__(self):
        extra = []
        if self.__forward_module__ is not None:
            extra.append(f", module={self.__forward_module__!r}")
        if self.__forward_is_class__:
            extra.append(", is_class=True")
        if self.__owner__ is not None:
            extra.append(f", owner={self.__owner__!r}")
        return f"ForwardRef({self.__forward_arg__!r}{''.join(extra)})"


_Template = type(t"")


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
        extra_names=None,
    ):
        # Either an AST node or a simple str (for the common case where a ForwardRef
        # represent a single name).
        assert isinstance(node, (ast.AST, str))
        self.__arg__ = None
        self.__forward_is_argument__ = False
        self.__forward_is_class__ = is_class
        self.__forward_module__ = None
        self.__code__ = None
        self.__ast_node__ = node
        self.__globals__ = globals
        self.__extra_names__ = extra_names
        self.__cell__ = cell
        self.__owner__ = owner
        self.__stringifier_dict__ = stringifier_dict

    def __convert_to_ast(self, other):
        if isinstance(other, _Stringifier):
            if isinstance(other.__ast_node__, str):
                return ast.Name(id=other.__ast_node__), other.__extra_names__
            return other.__ast_node__, other.__extra_names__
        elif type(other) is _Template:
            return _template_to_ast(other), None
        elif (
            # In STRING format we don't bother with the create_unique_name() dance;
            # it's better to emit the repr() of the object instead of an opaque name.
            self.__stringifier_dict__.format == Format.STRING
            or other is None
            or type(other) in (str, int, float, bool, complex)
        ):
            return ast.Constant(value=other), None
        elif type(other) is dict:
            extra_names = {}
            keys = []
            values = []
            for key, value in other.items():
                new_key, new_extra_names = self.__convert_to_ast(key)
                if new_extra_names is not None:
                    extra_names.update(new_extra_names)
                keys.append(new_key)
                new_value, new_extra_names = self.__convert_to_ast(value)
                if new_extra_names is not None:
                    extra_names.update(new_extra_names)
                values.append(new_value)
            return ast.Dict(keys, values), extra_names
        elif type(other) in (list, tuple, set):
            extra_names = {}
            elts = []
            for elt in other:
                new_elt, new_extra_names = self.__convert_to_ast(elt)
                if new_extra_names is not None:
                    extra_names.update(new_extra_names)
                elts.append(new_elt)
            ast_class = {list: ast.List, tuple: ast.Tuple, set: ast.Set}[type(other)]
            return ast_class(elts), extra_names
        else:
            name = self.__stringifier_dict__.create_unique_name()
            return ast.Name(id=name), {name: other}

    def __convert_to_ast_getitem(self, other):
        if isinstance(other, slice):
            extra_names = {}

            def conv(obj):
                if obj is None:
                    return None
                new_obj, new_extra_names = self.__convert_to_ast(obj)
                if new_extra_names is not None:
                    extra_names.update(new_extra_names)
                return new_obj

            return ast.Slice(
                lower=conv(other.start),
                upper=conv(other.stop),
                step=conv(other.step),
            ), extra_names
        else:
            return self.__convert_to_ast(other)

    def __get_ast(self):
        node = self.__ast_node__
        if isinstance(node, str):
            return ast.Name(id=node)
        return node

    def __make_new(self, node, extra_names=None):
        new_extra_names = {}
        if self.__extra_names__ is not None:
            new_extra_names.update(self.__extra_names__)
        if extra_names is not None:
            new_extra_names.update(extra_names)
        stringifier = _Stringifier(
            node,
            self.__globals__,
            self.__owner__,
            self.__forward_is_class__,
            stringifier_dict=self.__stringifier_dict__,
            extra_names=new_extra_names or None,
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
            extra_names = {}
            elts = []
            for elt in other:
                new_elt, new_extra_names = self.__convert_to_ast_getitem(elt)
                if new_extra_names is not None:
                    extra_names.update(new_extra_names)
                elts.append(new_elt)
            other = ast.Tuple(elts)
        else:
            other, extra_names = self.__convert_to_ast_getitem(other)
        assert isinstance(other, ast.AST), repr(other)
        return self.__make_new(ast.Subscript(self.__get_ast(), other), extra_names)

    def __getattr__(self, attr):
        return self.__make_new(ast.Attribute(self.__get_ast(), attr))

    def __call__(self, *args, **kwargs):
        extra_names = {}
        ast_args = []
        for arg in args:
            new_arg, new_extra_names = self.__convert_to_ast(arg)
            if new_extra_names is not None:
                extra_names.update(new_extra_names)
            ast_args.append(new_arg)
        ast_kwargs = []
        for key, value in kwargs.items():
            new_value, new_extra_names = self.__convert_to_ast(value)
            if new_extra_names is not None:
                extra_names.update(new_extra_names)
            ast_kwargs.append(ast.keyword(key, new_value))
        return self.__make_new(ast.Call(self.__get_ast(), ast_args, ast_kwargs), extra_names)

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
            rhs, extra_names = self.__convert_to_ast(other)
            return self.__make_new(
                ast.BinOp(self.__get_ast(), op, rhs), extra_names
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
            new_other, extra_names = self.__convert_to_ast(other)
            return self.__make_new(
                ast.BinOp(new_other, op, self.__get_ast()), extra_names
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
            rhs, extra_names = self.__convert_to_ast(other)
            return self.__make_new(
                ast.Compare(
                    left=self.__get_ast(),
                    ops=[op],
                    comparators=[rhs],
                ),
                extra_names,
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


def _template_to_ast(template):
    values = []
    for part in template:
        match part:
            case str():
                values.append(ast.Constant(value=part))
            # Interpolation, but we don't want to import the string module
            case _:
                interp = ast.Interpolation(
                    str=part.expression,
                    value=ast.parse(part.expression),
                    conversion=(
                        ord(part.conversion)
                        if part.conversion is not None
                        else -1
                    ),
                    format_spec=(
                        ast.Constant(value=part.format_spec)
                        if part.format_spec != ""
                        else None
                    ),
                )
                values.append(interp)
    return ast.TemplateStr(values=values)


class _StringifierDict(dict):
    def __init__(self, namespace, *, globals=None, owner=None, is_class=False, format):
        super().__init__(namespace)
        self.namespace = namespace
        self.globals = globals
        self.owner = owner
        self.is_class = is_class
        self.stringifiers = []
        self.next_id = 1
        self.format = format

    def __missing__(self, key):
        fwdref = _Stringifier(
            key,
            globals=self.globals,
            owner=self.owner,
            is_class=self.is_class,
            stringifier_dict=self,
        )
        self.stringifiers.append(fwdref)
        return fwdref

    def transmogrify(self):
        for obj in self.stringifiers:
            obj.__class__ = ForwardRef
            obj.__stringifier_dict__ = None  # not needed for ForwardRef
            if isinstance(obj.__ast_node__, str):
                obj.__arg__ = obj.__ast_node__
                obj.__ast_node__ = None

    def create_unique_name(self):
        name = f"__annotationlib_name_{self.next_id}__"
        self.next_id += 1
        return name


def call_evaluate_function(evaluate, format, *, owner=None):
    """Call an evaluate function. Evaluate functions are normally generated for
    the value of type aliases and the bounds, constraints, and defaults of
    type parameter objects.
    """
    return call_annotate_function(evaluate, format, owner=owner, _is_evaluate=True)


def call_annotate_function(annotate, format, *, owner=None, _is_evaluate=False):
    """Call an __annotate__ function. __annotate__ functions are normally
    generated by the compiler to defer the evaluation of annotations. They
    can be called with any of the format arguments in the Format enum, but
    compiler-generated __annotate__ functions only support the VALUE format.
    This function provides additional functionality to call __annotate__
    functions with the FORWARDREF and STRING formats.

    *annotate* must be an __annotate__ function, which takes a single argument
    and returns a dict of annotations.

    *format* must be a member of the Format enum or one of the corresponding
    integer values.

    *owner* can be the object that owns the annotations (i.e., the module,
    class, or function that the __annotate__ function derives from). With the
    FORWARDREF format, it is used to provide better evaluation capabilities
    on the generated ForwardRef objects.

    """
    if format == Format.VALUE_WITH_FAKE_GLOBALS:
        raise ValueError("The VALUE_WITH_FAKE_GLOBALS format is for internal use only")
    try:
        return annotate(format)
    except NotImplementedError:
        pass
    if format == Format.STRING:
        # STRING is implemented by calling the annotate function in a special
        # environment where every name lookup results in an instance of _Stringifier.
        # _Stringifier supports every dunder operation and returns a new _Stringifier.
        # At the end, we get a dictionary that mostly contains _Stringifier objects (or
        # possibly constants if the annotate function uses them directly). We then
        # convert each of those into a string to get an approximation of the
        # original source.
        globals = _StringifierDict({}, format=format)
        is_class = isinstance(owner, type)
        closure = _build_closure(
            annotate, owner, is_class, globals, allow_evaluation=False
        )
        func = types.FunctionType(
            annotate.__code__,
            globals,
            closure=closure,
            argdefs=annotate.__defaults__,
            kwdefaults=annotate.__kwdefaults__,
        )
        annos = func(Format.VALUE_WITH_FAKE_GLOBALS)
        if _is_evaluate:
            return _stringify_single(annos)
        return {
            key: _stringify_single(val)
            for key, val in annos.items()
        }
    elif format == Format.FORWARDREF:
        # FORWARDREF is implemented similarly to STRING, but there are two changes,
        # at the beginning and the end of the process.
        # First, while STRING uses an empty dictionary as the namespace, so that all
        # name lookups result in _Stringifier objects, FORWARDREF uses the globals
        # and builtins, so that defined names map to their real values.
        # Second, instead of returning strings, we want to return either real values
        # or ForwardRef objects. To do this, we keep track of all _Stringifier objects
        # created while the annotation is being evaluated, and at the end we convert
        # them all to ForwardRef objects by assigning to __class__. To make this
        # technique work, we have to ensure that the _Stringifier and ForwardRef
        # classes share the same attributes.
        # We use this technique because while the annotations are being evaluated,
        # we want to support all operations that the language allows, including even
        # __getattr__ and __eq__, and return new _Stringifier objects so we can accurately
        # reconstruct the source. But in the dictionary that we eventually return, we
        # want to return objects with more user-friendly behavior, such as an __eq__
        # that returns a bool and an defined set of attributes.
        namespace = {**annotate.__builtins__, **annotate.__globals__}
        is_class = isinstance(owner, type)
        globals = _StringifierDict(
            namespace,
            globals=annotate.__globals__,
            owner=owner,
            is_class=is_class,
            format=format,
        )
        closure = _build_closure(
            annotate, owner, is_class, globals, allow_evaluation=True
        )
        func = types.FunctionType(
            annotate.__code__,
            globals,
            closure=closure,
            argdefs=annotate.__defaults__,
            kwdefaults=annotate.__kwdefaults__,
        )
        try:
            result = func(Format.VALUE_WITH_FAKE_GLOBALS)
        except Exception:
            pass
        else:
            globals.transmogrify()
            return result

        # Try again, but do not provide any globals. This allows us to return
        # a value in certain cases where an exception gets raised during evaluation.
        globals = _StringifierDict(
            {},
            globals=annotate.__globals__,
            owner=owner,
            is_class=is_class,
            format=format,
        )
        closure = _build_closure(
            annotate, owner, is_class, globals, allow_evaluation=False
        )
        func = types.FunctionType(
            annotate.__code__,
            globals,
            closure=closure,
            argdefs=annotate.__defaults__,
            kwdefaults=annotate.__kwdefaults__,
        )
        result = func(Format.VALUE_WITH_FAKE_GLOBALS)
        globals.transmogrify()
        if _is_evaluate:
            if isinstance(result, ForwardRef):
                return result.evaluate(format=Format.FORWARDREF)
            else:
                return result
        else:
            return {
                key: (
                    val.evaluate(format=Format.FORWARDREF)
                    if isinstance(val, ForwardRef)
                    else val
                )
                for key, val in result.items()
            }
    elif format == Format.VALUE:
        # Should be impossible because __annotate__ functions must not raise
        # NotImplementedError for this format.
        raise RuntimeError("annotate function does not support VALUE format")
    else:
        raise ValueError(f"Invalid format: {format!r}")


def _build_closure(annotate, owner, is_class, stringifier_dict, *, allow_evaluation):
    if not annotate.__closure__:
        return None
    freevars = annotate.__code__.co_freevars
    new_closure = []
    for i, cell in enumerate(annotate.__closure__):
        if i < len(freevars):
            name = freevars[i]
        else:
            name = "__cell__"
        new_cell = None
        if allow_evaluation:
            try:
                cell.cell_contents
            except ValueError:
                pass
            else:
                new_cell = cell
        if new_cell is None:
            fwdref = _Stringifier(
                name,
                cell=cell,
                owner=owner,
                globals=annotate.__globals__,
                is_class=is_class,
                stringifier_dict=stringifier_dict,
            )
            stringifier_dict.stringifiers.append(fwdref)
            new_cell = types.CellType(fwdref)
        new_closure.append(new_cell)
    return tuple(new_closure)


def _stringify_single(anno):
    if anno is ...:
        return "..."
    # We have to handle str specially to support PEP 563 stringified annotations.
    elif isinstance(anno, str):
        return anno
    elif isinstance(anno, _Template):
        return ast.unparse(_template_to_ast(anno))
    else:
        return repr(anno)


def get_annotate_from_class_namespace(obj):
    """Retrieve the annotate function from a class namespace dictionary.

    Return None if the namespace does not contain an annotate function.
    This is useful in metaclass ``__new__`` methods to retrieve the annotate function.
    """
    try:
        return obj["__annotate__"]
    except KeyError:
        return obj.get("__annotate_func__", None)


def get_annotations(
    obj, *, globals=None, locals=None, eval_str=False, format=Format.VALUE
):
    """Compute the annotations dict for an object.

    obj may be a callable, class, module, or other object with
    __annotate__ or __annotations__ attributes.
    Passing any other object raises TypeError.

    The *format* parameter controls the format in which annotations are returned,
    and must be a member of the Format enum or its integer equivalent.
    For the VALUE format, the __annotations__ is tried first; if it
    does not exist, the __annotate__ function is called. The
    FORWARDREF format uses __annotations__ if it exists and can be
    evaluated, and otherwise falls back to calling the __annotate__ function.
    The SOURCE format tries __annotate__ first, and falls back to
    using __annotations__, stringified using annotations_to_string().

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
    if eval_str and format != Format.VALUE:
        raise ValueError("eval_str=True is only supported with format=Format.VALUE")

    match format:
        case Format.VALUE:
            # For VALUE, we first look at __annotations__
            ann = _get_dunder_annotations(obj)

            # If it's not there, try __annotate__ instead
            if ann is None:
                ann = _get_and_call_annotate(obj, format)
        case Format.FORWARDREF:
            # For FORWARDREF, we use __annotations__ if it exists
            try:
                ann = _get_dunder_annotations(obj)
            except Exception:
                pass
            else:
                if ann is not None:
                    return dict(ann)

            # But if __annotations__ threw a NameError, we try calling __annotate__
            ann = _get_and_call_annotate(obj, format)
            if ann is None:
                # If that didn't work either, we have a very weird object: evaluating
                # __annotations__ threw NameError and there is no __annotate__. In that case,
                # we fall back to trying __annotations__ again.
                ann = _get_dunder_annotations(obj)
        case Format.STRING:
            # For STRING, we try to call __annotate__
            ann = _get_and_call_annotate(obj, format)
            if ann is not None:
                return dict(ann)
            # But if we didn't get it, we use __annotations__ instead.
            ann = _get_dunder_annotations(obj)
            if ann is not None:
                return annotations_to_string(ann)
        case Format.VALUE_WITH_FAKE_GLOBALS:
            raise ValueError("The VALUE_WITH_FAKE_GLOBALS format is for internal use only")
        case _:
            raise ValueError(f"Unsupported format {format!r}")

    if ann is None:
        if isinstance(obj, type) or callable(obj):
            return {}
        raise TypeError(f"{obj!r} does not have annotations")

    if not ann:
        return {}

    if not eval_str:
        return dict(ann)

    if isinstance(obj, type):
        # class
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
        obj_globals = getattr(obj, "__dict__")
        obj_locals = None
        unwrap = None
    elif callable(obj):
        # this includes types.Function, types.BuiltinFunctionType,
        # types.BuiltinMethodType, functools.partial, functools.singledispatch,
        # "class funclike" from Lib/test/test_inspect... on and on it goes.
        obj_globals = getattr(obj, "__globals__", None)
        obj_locals = None
        unwrap = obj
    else:
        obj_globals = obj_locals = unwrap = None

    if unwrap is not None:
        while True:
            if hasattr(unwrap, "__wrapped__"):
                unwrap = unwrap.__wrapped__
                continue
            if functools := sys.modules.get("functools"):
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

    # "Inject" type parameters into the local namespace
    # (unless they are shadowed by assignments *in* the local namespace),
    # as a way of emulating annotation scopes when calling `eval()`
    if type_params := getattr(obj, "__type_params__", ()):
        if locals is None:
            locals = {}
        locals = {param.__name__: param for param in type_params} | locals

    return_value = {
        key: value if not isinstance(value, str) else eval(value, globals, locals)
        for key, value in ann.items()
    }
    return return_value


def type_repr(value):
    """Convert a Python value to a format suitable for use with the STRING format.

    This is intended as a helper for tools that support the STRING format but do
    not have access to the code that originally produced the annotations. It uses
    repr() for most objects.

    """
    if isinstance(value, (type, types.FunctionType, types.BuiltinFunctionType)):
        if value.__module__ == "builtins":
            return value.__qualname__
        return f"{value.__module__}.{value.__qualname__}"
    elif isinstance(value, _Template):
        tree = _template_to_ast(value)
        return ast.unparse(tree)
    if value is ...:
        return "..."
    return repr(value)


def annotations_to_string(annotations):
    """Convert an annotation dict containing values to approximately the STRING format.

    Always returns a fresh a dictionary.
    """
    return {
        n: t if isinstance(t, str) else type_repr(t)
        for n, t in annotations.items()
    }


def _get_and_call_annotate(obj, format):
    """Get the __annotate__ function and call it.

    May not return a fresh dictionary.
    """
    annotate = getattr(obj, "__annotate__", None)
    if annotate is not None:
        ann = call_annotate_function(annotate, format, owner=obj)
        if not isinstance(ann, dict):
            raise ValueError(f"{obj!r}.__annotate__ returned a non-dict")
        return ann
    return None


def _get_dunder_annotations(obj):
    """Return the annotations for an object, checking that it is a dictionary.

    Does not return a fresh dictionary.
    """
    ann = getattr(obj, "__annotations__", None)
    if ann is None:
        return None

    if not isinstance(ann, dict):
        raise ValueError(f"{obj!r}.__annotations__ is neither a dict nor None")
    return ann
