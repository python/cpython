from __future__ import annotations
import dataclasses as dc
import copy
import enum
import functools
import inspect
from collections.abc import Iterable, Iterator, Sequence
from typing import Final, Any, TYPE_CHECKING
if TYPE_CHECKING:
    from libclinic.converter import CConverter
    from libclinic.converters import self_converter
    from libclinic.return_converters import CReturnConverter
    from libclinic.app import Clinic

from libclinic import VersionTuple, unspecified


ClassDict = dict[str, "Class"]
ModuleDict = dict[str, "Module"]
ParamDict = dict[str, "Parameter"]


@dc.dataclass(repr=False)
class Module:
    name: str
    module: Module | Clinic

    def __post_init__(self) -> None:
        self.parent = self.module
        self.modules: ModuleDict = {}
        self.classes: ClassDict = {}
        self.functions: list[Function] = []

    def __repr__(self) -> str:
        return "<clinic.Module " + repr(self.name) + " at " + str(id(self)) + ">"


@dc.dataclass(repr=False)
class Class:
    name: str
    module: Module | Clinic
    cls: Class | None
    typedef: str
    type_object: str

    def __post_init__(self) -> None:
        self.parent = self.cls or self.module
        self.classes: ClassDict = {}
        self.functions: list[Function] = []

    def __repr__(self) -> str:
        return "<clinic.Class " + repr(self.name) + " at " + str(id(self)) + ">"


class FunctionKind(enum.Enum):
    CALLABLE        = enum.auto()
    STATIC_METHOD   = enum.auto()
    CLASS_METHOD    = enum.auto()
    METHOD_INIT     = enum.auto()
    METHOD_NEW      = enum.auto()
    GETTER          = enum.auto()
    SETTER          = enum.auto()
    SETTER_AND_DELETER  = enum.auto()

    @functools.cached_property
    def new_or_init(self) -> bool:
        return self in {FunctionKind.METHOD_INIT, FunctionKind.METHOD_NEW}

    def __repr__(self) -> str:
        return f"<clinic.FunctionKind.{self.name}>"


CALLABLE: Final = FunctionKind.CALLABLE
STATIC_METHOD: Final = FunctionKind.STATIC_METHOD
CLASS_METHOD: Final = FunctionKind.CLASS_METHOD
METHOD_INIT: Final = FunctionKind.METHOD_INIT
METHOD_NEW: Final = FunctionKind.METHOD_NEW
GETTER: Final = FunctionKind.GETTER
SETTER: Final = FunctionKind.SETTER
SETTER_AND_DELETER: Final = FunctionKind.SETTER_AND_DELETER

# The kinds which implement the setter of an entry of PyGetSetDef.
SETTERS: Final = frozenset({SETTER, SETTER_AND_DELETER})
# The kinds which implement an entry of PyGetSetDef.
ACCESSORS: Final = SETTERS | {GETTER}


@dc.dataclass(repr=False)
class Function:
    """
    Mutable duck type for inspect.Function.

    docstring - a str containing
        * embedded line breaks
        * text outdented to the left margin
        * no trailing whitespace.
        It will always be true that
            (not docstring) or ((not docstring[0].isspace()) and (docstring.rstrip() == docstring))
    """
    parameters: ParamDict = dc.field(default_factory=dict)
    _: dc.KW_ONLY
    name: str
    module: Module | Clinic
    cls: Class | None
    c_basename: str
    full_name: str
    return_converter: CReturnConverter
    kind: FunctionKind
    coexist: bool
    return_annotation: object = inspect.Signature.empty
    docstring: str = ''
    # docstring_only means "don't generate a machine-readable
    # signature, just a normal docstring".  it's True for
    # functions with optional groups because we can't represent
    # those accurately with inspect.Signature in 3.4.
    docstring_only: bool = False
    forced_text_signature: str | None = None
    critical_section: bool = False
    disable_fastcall: bool = False
    target_critical_section: list[str] = dc.field(default_factory=list)
    # Line of the file on which the function is declared.
    line_number: int | None = None
    # Line on which the docstring starts (`None` if there is no docstring).
    docstring_line_number: int | None = None

    def __post_init__(self) -> None:
        self.parent = self.cls or self.module
        self.self_converter: self_converter | None = None
        self.__render_parameters__: list[Parameter] | None = None

    @functools.cached_property
    def displayname(self) -> str:
        """Pretty-printable name."""
        if self.kind.new_or_init:
            assert isinstance(self.cls, Class)
            return self.cls.name
        else:
            return self.name

    @functools.cached_property
    def fulldisplayname(self) -> str:
        parent: Class | Module | Clinic | None
        if self.kind.new_or_init:
            parent = getattr(self.cls, "parent", None)
        else:
            parent = self.parent
        name = self.displayname
        while isinstance(parent, (Module, Class)):
            name = f"{parent.name}.{name}"
            parent = parent.parent
        return name

    @property
    def render_parameters(self) -> list[Parameter]:
        if not self.__render_parameters__:
            l: list[Parameter] = []
            self.__render_parameters__ = l
            for p in self.parameters.values():
                p = p.copy()
                p.converter.pre_render()
                l.append(p)
        return self.__render_parameters__

    @property
    def methoddef_flags(self) -> str | None:
        if self.kind.new_or_init:
            return None
        flags = []
        match self.kind:
            case FunctionKind.CLASS_METHOD:
                flags.append('METH_CLASS')
            case FunctionKind.STATIC_METHOD:
                flags.append('METH_STATIC')
            case _ as kind:
                acceptable_kinds = {FunctionKind.CALLABLE} | ACCESSORS
                assert kind in acceptable_kinds, f"unknown kind: {kind!r}"
        if self.coexist:
            flags.append('METH_COEXIST')
        return '|'.join(flags)

    @property
    def docstring_line_width(self) -> int:
        """Return the maximum line width for docstring lines.

        Pydoc adds indentation when displaying functions and methods.
        To keep the total width of within 80 characters, we use a
        maximum of 72 characters for global functions and classes,
        and 68 characters for methods.
        """
        if self.cls is not None and not self.kind.new_or_init:
            return 68
        return 72

    def __repr__(self) -> str:
        return f'<clinic.Function {self.name!r}>'

    def copy(self, **overrides: Any) -> Function:
        f = dc.replace(self, **overrides)
        f.parameters = {
            name: value.copy(function=f)
            for name, value in f.parameters.items()
        }
        return f


@dc.dataclass(repr=False, slots=True)
class Parameter:
    """
    Mutable duck type of inspect.Parameter.
    """
    name: str
    kind: inspect._ParameterKind
    _: dc.KW_ONLY
    default: object = inspect.Parameter.empty
    function: Function
    converter: CConverter
    annotation: object = inspect.Parameter.empty
    docstring: str = ''
    # Identifier of the optional group containing the parameter (0 if none).
    # It is negative for groups before the required parameters.
    group: int = 0
    # Nesting level of that group (0 if none).
    group_depth: int = 0
    # (`None` signifies that there is no deprecation)
    deprecated_positional: VersionTuple | None = None
    deprecated_keyword: VersionTuple | None = None
    # Line of the file on which the parameter is declared.
    line_number: int | None = None
    right_bracket_count: int = dc.field(init=False, default=0)

    def __repr__(self) -> str:
        return f'<clinic.Parameter {self.name!r}>'

    def is_keyword_only(self) -> bool:
        return self.kind == inspect.Parameter.KEYWORD_ONLY

    def is_positional_only(self) -> bool:
        return self.kind == inspect.Parameter.POSITIONAL_ONLY

    def is_positional_or_keyword(self) -> bool:
        return self.kind == inspect.Parameter.POSITIONAL_OR_KEYWORD

    def is_vararg(self) -> bool:
        return self.kind == inspect.Parameter.VAR_POSITIONAL

    def is_var_keyword(self) -> bool:
        return self.kind == inspect.Parameter.VAR_KEYWORD

    def is_variable_length(self) -> bool:
        return self.is_vararg() or self.is_var_keyword()

    def is_optional(self) -> bool:
        return not self.is_vararg() and (self.default is not unspecified)

    def copy(
        self,
        /,
        *,
        converter: CConverter | None = None,
        function: Function | None = None,
        **overrides: Any
    ) -> Parameter:
        function = function or self.function
        if not converter:
            converter = copy.copy(self.converter)
            converter.function = function
        return dc.replace(self, **overrides, function=function, converter=converter)

    def get_displayname(self, i: int) -> str:
        if i == 0:
            return 'argument'
        if not self.is_positional_only():
            return f'argument {self.name!r}'
        else:
            return f'argument {i}'

    def render_docstring(self) -> str:
        lines = [f"  {self.name}"]
        lines.extend(f"    {line}" for line in self.docstring.split("\n"))
        return "\n".join(lines).rstrip()


ParamTuple = tuple["Parameter", ...]

Definition = Module | Class | Function


def walk_definitions(
    parent: Clinic | Module | Class,
    prefix: str = '',
    depth: int = 0,
) -> Iterator[tuple[int, str, Definition]]:
    """Yield (depth, dotted name, definition) for every nested definition.

    The name of a module is already fully qualified, but the name of
    a class is not, hence the prefix.
    """
    for function in parent.functions:
        if function.kind.new_or_init:
            # __new__() and __init__() are called as the class itself.
            name = prefix
        else:
            name = f'{prefix}.{function.name}' if prefix else function.name
        yield depth, name, function
    for cls in parent.classes.values():
        name = f'{prefix}.{cls.name}' if prefix else cls.name
        yield depth, name, cls
        yield from walk_definitions(cls, name, depth + 1)
    if not isinstance(parent, Class):
        # Only a module can contain modules.
        for module in parent.modules.values():
            yield depth, module.name, module
            yield from walk_definitions(module, module.name, depth + 1)


def group_to_variable_name(group: int) -> str:
    adjective = "left_" if group < 0 else "right_"
    return "group_" + adjective + str(abs(group))


def count_required(subset: ParamTuple) -> int:
    """Return the number of arguments which cannot be omitted.

    A parameter in an optional group is passed together with its group,
    so only trailing parameters with a default value can be omitted.
    """
    count = len(subset)
    for p in reversed(subset):
        if p.group or not p.is_optional():
            break
        count -= 1
    return count


def permute_left_option_groups(
    l: Sequence[Iterable[Parameter]]
) -> Iterator[ParamTuple]:
    """
    Given [(1,), (2,), (3,)], should yield:
       ()
       (3,)
       (2, 3)
       (1, 2, 3)
    """
    yield tuple()
    accumulator: list[Parameter] = []
    for group in reversed(l):
        accumulator = list(group) + accumulator
        yield tuple(accumulator)


def permute_right_option_groups(
    l: Sequence[Iterable[Parameter]]
) -> Iterator[ParamTuple]:
    """
    Given [(1,), (2,), (3,)], should yield:
      ()
      (1,)
      (1, 2)
      (1, 2, 3)
    """
    yield tuple()
    accumulator: list[Parameter] = []
    for group in l:
        accumulator.extend(group)
        yield tuple(accumulator)


def permute_optional_groups(
    left: Sequence[Sequence[Iterable[Parameter]]],
    required: Iterable[Parameter],
    right: Sequence[Sequence[Iterable[Parameter]]]
) -> tuple[ParamTuple, ...]:
    """
    Generator function that computes the set of acceptable
    argument lists for the provided iterables of
    argument groups.  (Actually it generates a tuple of tuples.)

    "left" and "right" are sequences of chains of nested groups.
    Groups of different chains are independent of each other.

    Algorithm: prefer left options over right options.

    If required is empty, left must also be empty.
    """
    required = tuple(required)
    if not required:
        if left:
            raise ValueError("required is empty but left is not")

    left_options: list[ParamTuple] = [()]
    for chain in left:
        left_options = [option + t
                        for option in left_options
                        for t in permute_left_option_groups(chain)]
    right_options: list[ParamTuple] = [()]
    for chain in reversed(right):
        right_options = [t + option
                         for option in right_options
                         for t in permute_right_option_groups(chain)]

    accumulator: list[ParamTuple] = []
    counts = set()
    for r in right_options:
        for l in left_options:
            t = l + required + r
            if len(t) in counts:
                continue
            counts.add(len(t))
            accumulator.append(t)

    accumulator.sort(key=len)
    return tuple(accumulator)
