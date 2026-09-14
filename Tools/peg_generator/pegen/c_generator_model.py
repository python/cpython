"""Immutable C parser descriptions shared by lowering and emission."""

from dataclasses import dataclass
from enum import Enum, auto

from pegen.grammar import RuleKind


class NodeTypes(Enum):
    NAME_TOKEN = 0
    NUMBER_TOKEN = 1
    STRING_TOKEN = 2
    GENERIC_TOKEN = 3
    KEYWORD = 4
    SOFT_KEYWORD = 5
    CUT_OPERATOR = 6
    F_STRING_CHUNK = 7


class CBindingKind(Enum):
    NORMAL = auto()
    OPTIONAL = auto()
    CUT = auto()


@dataclass(frozen=True, slots=True)
class FunctionCall:
    function: str
    arguments: tuple[str | int, ...] = ()
    assigned_variable: str | None = None
    assigned_variable_type: str | None = None
    return_type: str | None = None
    nodetype: NodeTypes | None = None
    force_true: bool = False
    comment: str | None = None
    binding_kind: CBindingKind = CBindingKind.NORMAL

    def expression(self) -> str:
        """Render the invocation without its alternative-local binding or comment."""
        expression = self.function
        if arguments := self.arguments:
            expression += f"({', '.join(map(str, arguments))})"
        if self.force_true:
            expression += ", !p->error_indicator"
        return expression

    def __str__(self) -> str:
        expression = self.expression()
        if variable := self.assigned_variable:
            cast = f"({self.assigned_variable_type})" if self.assigned_variable_type else ""
            expression = f"({variable} = {cast}{expression})"
        if comment := self.comment:
            expression += f"  // {comment}"
        return expression


@dataclass(frozen=True, slots=True)
class CRuleSignature:
    name: str
    kind: RuleKind
    return_type: str | None

    @property
    def c_return_type(self) -> str:
        return self.return_type or "void *"

    def declaration(self) -> str:
        separator = " " if self.kind is RuleKind.NORMAL and self.return_type else ""
        return f"static {self.c_return_type}{separator}{self.name}_rule(Parser *p);"


@dataclass(frozen=True, slots=True)
class CVariable:
    name: str
    type: str | None
    initializer: str | None = None
    unused: bool = False


@dataclass(frozen=True, slots=True)
class CAction:
    expression: str
    checked: bool = False
    debug_message: str | None = None


@dataclass(frozen=True, slots=True)
class CAlternative:
    text: str
    action: CAction
    calls: tuple[FunctionCall, ...]
    variables: tuple[CVariable, ...]
    cut_variable: str | None
    requires_invalid_rules: bool
    uses_locations: bool


@dataclass(frozen=True, slots=True)
class CRule:
    signature: CRuleSignature
    text: str
    alternatives: tuple[CAlternative, ...]
    left_recursive: bool
    leader: bool
    memoize: bool
    disable_invalid_rules: bool

    @property
    def uses_locations(self) -> bool:
        return any(alt.uses_locations for alt in self.alternatives)


@dataclass(frozen=True, slots=True)
class CParser:
    """Complete file-emission input, independent of compilation state."""

    source_name: str
    headers: tuple[str, ...]
    keyword_groups: tuple[tuple[tuple[str, int], ...], ...]
    soft_keywords: tuple[str, ...]
    rules: tuple[CRule, ...]
    trailer: str | None
    debug: bool
