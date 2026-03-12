"""awk_ast.py — AWK AST intermediate representation for the transpiler.

Defines dataclass nodes representing POSIX AWK constructs.  The pipeline is:

    pysh_ast  --[pysh_to_awk]-->  awk_ast  (this module)
              --[awk_emit]-->     awk text

Key simplification: no separate BoolExpr hierarchy.
In AWK, comparisons and logical operators are regular expressions that
produce 0/1, usable in if conditions or as values.
"""

from __future__ import annotations

from dataclasses import dataclass, field


# ---------------------------------------------------------------------------
# Abstract bases
# ---------------------------------------------------------------------------


@dataclass
class AwkNode:
    """Base class for all AWK AST nodes."""


@dataclass
class Expr(AwkNode):
    """Produces an AWK value (string or number)."""


@dataclass
class Stmt(AwkNode):
    """An executable AWK statement."""


# ---------------------------------------------------------------------------
# Expressions
# ---------------------------------------------------------------------------


@dataclass
class StringLit(Expr):
    """A literal string value.

    AWK output: ``"value"``
    """

    value: str


@dataclass
class NumLit(Expr):
    """A literal number.

    AWK output: ``42``
    """

    value: int | float


@dataclass
class Var(Expr):
    """A variable reference.

    AWK output: ``name`` (no sigils)
    """

    name: str


@dataclass
class ArrayVar(Expr):
    """An array variable passed by name (not subscripted).

    Used when an entire array is passed to a varargs function, so that the
    emitter knows not to wrap it in a temporary array.
    """

    name: str


@dataclass
class ArrayRef(Expr):
    """Array element access.

    AWK output: ``name[key]``
    """

    name: str
    key: Expr


@dataclass
class BinOp(Expr):
    """Binary operator expression.

    AWK output: ``left op right``

    Covers arithmetic (+, -, *, /, %), string comparison (==, !=, <, >, <=, >=),
    logical (&&, ||), string match (~, !~), and string concatenation (" ").
    """

    left: Expr
    op: str
    right: Expr


@dataclass
class UnaryOp(Expr):
    """Unary operator expression.

    AWK output: ``!expr`` or ``-expr``
    """

    op: str
    expr: Expr


@dataclass
class Ternary(Expr):
    """Ternary conditional expression.

    AWK output: ``(test ? body : orelse)``
    """

    test: Expr
    body: Expr
    orelse: Expr


@dataclass
class FuncCall(Expr):
    """Function call expression.

    AWK output: ``name(args)``
    """

    name: str
    args: list[Expr] = field(default_factory=list)


@dataclass
class Regex(Expr):
    """A regex literal.

    AWK output: ``/pattern/``
    """

    pattern: str


@dataclass
class Match(Expr):
    """Regex match expression.

    AWK output: ``expr ~ /pattern/`` or ``expr !~ /pattern/``
    """

    expr: Expr
    pattern: Expr  # Regex or dynamic
    negated: bool = False


@dataclass
class Concat(Expr):
    """String concatenation via juxtaposition.

    AWK output: ``a b c`` (space-separated in source = concatenation)
    """

    parts: list[Expr] = field(default_factory=list)


@dataclass
class Sprintf(Expr):
    """sprintf() call.

    AWK output: ``sprintf(fmt, args...)``
    """

    fmt: str
    args: list[Expr] = field(default_factory=list)


@dataclass
class InArray(Expr):
    """Array membership test.

    AWK output: ``(key in array)``
    """

    key: Expr
    array: str


@dataclass
class Getline(Expr):
    """Command getline for capturing output.

    AWK output: ``cmd | getline var``
    This is typically used as a statement but produces a value (0/1).
    """

    cmd: Expr
    var: str = "_getline_tmp"


@dataclass
class Substr(Expr):
    """substr() call.

    AWK output: ``substr(str, start[, len])``
    """

    string: Expr
    start: Expr
    length: Expr | None = None


@dataclass
class Length(Expr):
    """length() call.

    AWK output: ``length(expr)``
    """

    expr: Expr


@dataclass
class Index(Expr):
    """index() call.

    AWK output: ``index(haystack, needle)``
    """

    haystack: Expr
    needle: Expr


@dataclass
class Split(Expr):
    """split() call.

    AWK output: ``split(str, arr, sep)``
    """

    string: Expr
    array: str
    sep: Expr | None = None


@dataclass
class Sub(Expr):
    """sub() or gsub() call.

    AWK output: ``gsub(pattern, replacement, target)``
    """

    pattern: Expr
    replacement: Expr
    target: Expr
    global_: bool = True  # gsub vs sub


@dataclass
class Tolower(Expr):
    """tolower() call.

    AWK output: ``tolower(expr)``
    """

    expr: Expr


@dataclass
class Toupper(Expr):
    """toupper() call.

    AWK output: ``toupper(expr)``
    """

    expr: Expr


@dataclass
class System(Expr):
    """system() call — execute a shell command.

    AWK output: ``system(cmd)``
    Returns the exit status.
    """

    cmd: Expr


# ---------------------------------------------------------------------------
# Statements
# ---------------------------------------------------------------------------


@dataclass
class Assign(Stmt):
    """Variable or array assignment.

    AWK output: ``target = value``
    Target is a Var or ArrayRef.
    """

    target: Expr
    value: Expr


@dataclass
class If(Stmt):
    """if/else if/else construct.

    AWK output: ``if (cond) { ... } else if (...) { ... } else { ... }``
    """

    branches: list[IfBranch] = field(default_factory=list)
    else_body: Block | None = None


@dataclass
class IfBranch(AwkNode):
    """A single if or else-if branch."""

    cond: Expr
    body: Block


@dataclass
class For(Stmt):
    """C-style for loop.

    AWK output: ``for (init; cond; incr) { ... }``
    """

    init: Stmt | None
    cond: Expr | None
    incr: Stmt | None
    body: Block


@dataclass
class ForIn(Stmt):
    """For-in loop over array keys.

    AWK output: ``for (var in array) { ... }``
    """

    var: str
    array: str
    body: Block


@dataclass
class While(Stmt):
    """While loop.

    AWK output: ``while (cond) { ... }``
    """

    cond: Expr
    body: Block


@dataclass
class Return(Stmt):
    """Return statement.

    AWK output: ``return value``
    """

    value: Expr | None = None


@dataclass
class Print(Stmt):
    """Print statement.

    AWK output: ``print args [> dest]``
    """

    args: list[Expr] = field(default_factory=list)
    dest: Expr | None = None
    append: bool = False


@dataclass
class Printf(Stmt):
    """Printf statement.

    AWK output: ``printf fmt, args [> dest]``
    """

    fmt: Expr
    args: list[Expr] = field(default_factory=list)
    dest: Expr | None = None
    append: bool = False


@dataclass
class ExprStmt(Stmt):
    """Expression used as statement (e.g., function call with no return).

    AWK output: ``expr``
    """

    expr: Expr


@dataclass
class Delete(Stmt):
    """Delete array element.

    AWK output: ``delete array[key]``
    """

    array: str
    key: Expr


@dataclass
class DeleteArray(Stmt):
    """Delete entire array.

    AWK output: ``delete array``
    """

    array: str


@dataclass
class Break(Stmt):
    """Break statement."""


@dataclass
class Continue(Stmt):
    """Continue statement."""


@dataclass
class Exit(Stmt):
    """Exit statement.

    AWK output: ``exit [code]``
    """

    code: Expr | None = None


@dataclass
class Block(Stmt):
    """An ordered sequence of statements."""

    stmts: list[Stmt] = field(default_factory=list)


@dataclass
class Comment(Stmt):
    """A comment line.

    AWK output: ``# text``
    """

    text: str


# ---------------------------------------------------------------------------
# Top-level nodes
# ---------------------------------------------------------------------------


@dataclass
class FuncDef(AwkNode):
    """AWK function definition.

    Locals are declared as extra parameters separated by extra spaces.

    AWK output::

        function name(params,    loc1, loc2) {
            ...
        }
    """

    name: str
    params: list[str] = field(default_factory=list)
    locals: list[str] = field(default_factory=list)
    body: Block = field(default_factory=Block)


@dataclass
class BeginBlock(AwkNode):
    """AWK BEGIN block.

    AWK output: ``BEGIN { ... }``
    """

    body: Block = field(default_factory=Block)


@dataclass
class VerbatimBlock(AwkNode):
    """A block of verbatim AWK code included as-is.

    Used for embedding the pyconf.awk runtime library.
    """

    code: str


@dataclass
class Program(AwkNode):
    """Complete AWK program.

    Contains function definitions, verbatim blocks, and a BEGIN block.
    """

    parts: list[FuncDef | BeginBlock | VerbatimBlock | Comment] = field(
        default_factory=list
    )
