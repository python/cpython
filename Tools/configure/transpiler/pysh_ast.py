"""pysh_ast.py — Intermediate Python-shaped AST (AST1) for the transpiler.

Pipeline position:

    Python AST  --[py_to_pysh]-->  pysh_ast  (this module)
                --[pysh_to_awk]-->   awk_ast
                --[awk_emit]-->      awk text

Nodes are nearly 1-to-1 with Python's ``ast`` module.  The one annotation is:
every :class:`Call` carries a :class:`CallType` tag recording how the callee
communicates its return value.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum, auto


class CallType(Enum):
    """How a function communicates its return value to the caller."""

    VOID = auto()  # side-effecting call; return value ignored
    BOOL = auto()  # returns bool via exit code (0 = true)
    RETVAL = auto()  # returns string via $pyconf_retval
    MULTI = auto()  # returns N strings via $_retval_0 … $_retval_N-1
    STDOUT = auto()  # returns string captured with $(...)


# ---------------------------------------------------------------------------
# Abstract bases
# ---------------------------------------------------------------------------


@dataclass
class Expr:
    """Base class for pysh_ast expressions (produce a value)."""


@dataclass
class Stmt:
    """Base class for pysh_ast statements (executable)."""


# ---------------------------------------------------------------------------
# Expressions
# ---------------------------------------------------------------------------


@dataclass
class Const(Expr):
    """A constant value: string, int, bool, or None.

    Mirrors ``ast.Constant``.
    """

    value: str | int | bool | None


@dataclass
class Var(Expr):
    """A variable reference by name.

    Mirrors ``ast.Name``.
    """

    name: str


@dataclass
class Attr(Expr):
    """Attribute access: ``obj.attr``.

    ``obj`` is a dotted string already flattened from the Python AST, e.g.
    ``"v"``, ``"pyconf"``, ``"os.environ"``, ``"os"``.

    Mirrors ``ast.Attribute``.
    """

    obj: str
    attr: str


@dataclass
class Subscript(Expr):
    """Subscript expression: ``obj[key]``.

    Mirrors ``ast.Subscript``.
    """

    obj: Expr
    key: Expr


@dataclass
class FStr(Expr):
    """F-string (JoinedStr).

    ``parts`` is a list where each element is either:

    - a ``str`` — a literal text segment, or
    - an :class:`Expr` — an interpolated sub-expression.

    Mirrors ``ast.JoinedStr``.
    """

    parts: list = field(default_factory=list)  # list[Expr | str]


@dataclass
class Add(Expr):
    """Binary string concatenation (``+``).

    Mirrors ``ast.BinOp`` with ``ast.Add``.
    """

    left: Expr
    right: Expr


@dataclass
class BoolOp(Expr):
    """Boolean operator over a list of operands.

    ``op`` is ``"and"`` or ``"or"``.

    Mirrors ``ast.BoolOp``.
    """

    op: str  # "and" | "or"
    values: list = field(default_factory=list)  # list[Expr]


@dataclass
class Not(Expr):
    """Logical negation.

    Mirrors ``ast.UnaryOp`` with ``ast.Not``.
    """

    operand: Expr


@dataclass
class Cmp(Expr):
    """Single comparison: ``left op right``.

    ``op`` is one of ``"=="``, ``"!="``, ``"<"``, ``"<="``, ``">"``,
    ``">="``, ``"is"``, ``"is not"``.

    Mirrors a single-operator ``ast.Compare``.
    """

    left: Expr
    op: str
    right: Expr


@dataclass
class Ternary(Expr):
    """Conditional expression: ``body if test else orelse``.

    Mirrors ``ast.IfExp``.
    """

    test: Expr
    body: Expr
    orelse: Expr


@dataclass
class Call(Expr):
    """Function call with pre-computed :class:`CallType`.

    ``func`` is a dotted-name string, e.g. ``"pyconf.check_header"``,
    ``"find_prog"``, ``"OPTION.is_yes"``.

    ``args`` includes ``Var("v")`` if the Python call passed the ``v``
    argument; Transform 2 decides whether to use or strip it.

    Mirrors ``ast.Call``.
    """

    func: str
    args: list = field(default_factory=list)  # list[Expr]
    kwargs: dict = field(default_factory=dict)  # dict[str, Expr]
    call_type: CallType = CallType.VOID


@dataclass
class In(Expr):
    """Membership test: ``item in container`` (or ``item not in container``).

    Mirrors ``ast.Compare`` with ``ast.In`` / ``ast.NotIn``.
    """

    item: Expr
    container: Expr
    negated: bool = False


@dataclass
class List(Expr):
    """A list or tuple literal.

    Mirrors ``ast.List`` / ``ast.Tuple`` / ``ast.Set``.
    """

    elts: list = field(default_factory=list)  # list[Expr]


@dataclass
class Dict(Expr):
    """A dict literal with constant string keys.

    ``keys`` is a list of string key names.
    ``values`` is a parallel list of :class:`Expr` values.

    Mirrors ``ast.Dict`` (constant-key subset only).
    """

    keys: list = field(default_factory=list)  # list[str]
    values: list = field(default_factory=list)  # list[Expr]


# ---------------------------------------------------------------------------
# Statements
# ---------------------------------------------------------------------------


@dataclass
class Block(Stmt):
    """An ordered sequence of statements.

    Used as the body of compound statements (if, for, with, funcdef).
    """

    stmts: list = field(default_factory=list)  # list[Stmt]


@dataclass
class Assign(Stmt):
    """Assignment statement: ``target = value``.

    Mirrors ``ast.Assign`` / ``ast.AnnAssign``.
    """

    target: Expr
    value: Expr


@dataclass
class AugAssign(Stmt):
    """Augmented assignment: ``target op= value``.

    ``op`` is the operator name, e.g. ``"Add"``.

    Mirrors ``ast.AugAssign``.
    """

    target: Expr
    op: str
    value: Expr


@dataclass
class If(Stmt):
    """``if / elif / else`` statement.

    ``orelse`` is ``None`` for no else clause.

    Mirrors ``ast.If``.
    """

    test: Expr
    body: Block
    orelse: Block | None = None


@dataclass
class For(Stmt):
    """``for var in iter: body`` loop.

    Mirrors ``ast.For``.
    """

    var: str
    iter: Expr
    body: Block


@dataclass
class With(Stmt):
    """``with ctx: body`` statement.

    Mirrors ``ast.With`` (single context manager only).
    """

    ctx: Expr
    body: Block


@dataclass
class Return(Stmt):
    """``return [value]`` statement.

    Mirrors ``ast.Return``.
    """

    value: Expr | None = None


@dataclass
class Break(Stmt):
    """``break`` statement."""


@dataclass
class Continue(Stmt):
    """``continue`` statement."""


@dataclass
class ExprStmt(Stmt):
    """An expression used as a statement (typically a call).

    Mirrors ``ast.Expr``.
    """

    expr: Expr


# ---------------------------------------------------------------------------
# Top-level
# ---------------------------------------------------------------------------


@dataclass
class FuncDef:
    """A function definition at the pysh_ast level.

    ``params`` lists the Python-side parameter names, excluding ``"v"``.
    ``defaults`` maps parameter names to their default expressions.
    """

    name: str
    params: list = field(default_factory=list)  # list[str]
    defaults: dict = field(default_factory=dict)  # dict[str, Expr]
    body: Block = field(default_factory=Block)


@dataclass
class OptionInfo:
    """Metadata for a pyconf.arg_with / pyconf.arg_enable declaration."""

    python_var: str  # e.g. "DISABLE_GIL"
    kind: str  # "enable" or "with"
    name: str  # e.g. "gil"
    key: str  # e.g. "enable_gil"
    var: str | None = None
    default: object = None
    help_text: str = ""
    metavar: str = ""
    false_is: str | None = None
    display: str | None = None


@dataclass
class ModuleConst:
    """A module-level constant assignment (non-option)."""

    name: str
    value: str | int | float | bool


@dataclass
class ModuleInfo:
    """Parsed representation of a single conf_*.py module."""

    name: str
    filepath: str
    options: list = field(default_factory=list)  # list[OptionInfo]
    option_var_map: dict = field(default_factory=dict)  # dict[str, OptionInfo]
    constants: list = field(default_factory=list)  # list[ModuleConst]
    functions: list = field(default_factory=list)  # list[FuncDef]
    # Optional raw AST (used by test helpers that build ModuleInfo from source)
    tree: object = None


@dataclass
class Program:
    """The full parsed program ready for code generation."""

    script_dir: str
    modules: list = field(default_factory=list)  # list[ModuleInfo]
    all_option_vars: dict = field(
        default_factory=dict
    )  # dict[str, OptionInfo]
    run_order: list = field(default_factory=list)  # list[str]


# ---------------------------------------------------------------------------
# Call-type classification constants (shared by py_to_pysh and pysh_to_awk)
# ---------------------------------------------------------------------------

# pyconf.* functions returning 0/1 exit code.
PYCONF_BOOL_FUNCS: frozenset[str] = frozenset(
    {
        "check_header",
        "check_headers",
        "check_func",
        "check_funcs",
        "compile_check",
        "link_check",
        "try_link",
        "check_compile_flag",
        "check_linker_flag",
        "check_lib",
        "check_type",
        "check_member",
        "check_decl",
        "check_define",
        "run_check",
        "fnmatch",
        "fnmatch_any",
        "path_exists",
        "path_is_dir",
        "path_is_file",
        "path_is_symlink",
        "is_executable",
        "pkg_config_check",
        "run_check_with_cc_flag",
        "is_defined",
        "cmd",
    }
)

# pyconf.* functions returning a value via stdout.
PYCONF_STDOUT_FUNCS: frozenset[str] = frozenset(
    {
        "find_prog",
        "cmd_output",
        "check_prog",
        "check_progs",
        "check_tools",
        "search_libs",
        "sizeof",
        "abspath",
        "basename",
        "getcwd",
        "readlink",
        "relpath",
        "path_join",
        "path_parent",
        "read_file",
        "read_file_lines",
        "glob_files",
        "str_remove",
        "sed",
        "find_install",
        "find_mkdir_p",
        "platform_machine",
        "platform_system",
        "compile_link_check",
        "pkg_config",
        "run_program_output",
        "get_dir_arg",
    }
)

# pyconf.* functions setting pyconf_retval.
PYCONF_RETVAL_FUNCS: frozenset[str] = frozenset(
    {
        "check_sizeof",
        "check_alignof",
    }
)

# OPTION.* methods returning 0/1 exit code.
OPTION_BOOL_METHODS: frozenset[str] = frozenset({"is_yes", "is_no"})
OPTION_BOOL_PROPS: frozenset[str] = frozenset({"given"})

# OPTION.* methods returning value via stdout.
OPTION_VALUE_METHODS: frozenset[str] = frozenset({"value_or"})
OPTION_VALUE_PROPS: frozenset[str] = frozenset({"value"})

# OPTION.* methods setting pyconf_retval.
OPTION_RETVAL_METHODS: frozenset[str] = frozenset(
    {"process_bool", "process_value"}
)
