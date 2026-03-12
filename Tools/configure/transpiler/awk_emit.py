"""awk_emit.py — AWK AST → POSIX AWK text emitter.

Walks AWK AST nodes (see awk_ast.py) and produces AWK source code.

Pipeline position::

    pysh_ast  --[pysh_to_awk]-->  awk_ast  --[this module]-->  awk text
"""

from __future__ import annotations

from .awk_ast import (
    ArrayRef,
    ArrayVar,
    Assign,
    BeginBlock,
    BinOp,
    Block,
    Break,
    Comment,
    Concat,
    Continue,
    Delete,
    DeleteArray,
    Exit,
    Expr,
    ExprStmt,
    For,
    ForIn,
    FuncCall,
    FuncDef,
    Getline,
    If,
    InArray,
    Index,
    Length,
    Match,
    NumLit,
    Print,
    Printf,
    Program,
    Regex,
    Return,
    Split,
    Sprintf,
    StringLit,
    Stmt,
    Sub,
    Substr,
    System,
    Ternary,
    Tolower,
    Toupper,
    UnaryOp,
    Var,
    VerbatimBlock,
    While,
)


# ---------------------------------------------------------------------------
# AWK string escaping
# ---------------------------------------------------------------------------


def _escape_awk_string(s: str) -> str:
    """Escape a string for use inside AWK double quotes."""
    s = s.replace("\\", "\\\\")
    s = s.replace('"', '\\"')
    s = s.replace("\n", "\\n")
    s = s.replace("\t", "\\t")
    s = s.replace("\r", "\\r")
    return s


def _escape_regex(s: str) -> str:
    """Escape a string for use inside AWK /regex/."""
    s = s.replace("\\", "\\\\")
    s = s.replace("/", "\\/")
    return s


# ---------------------------------------------------------------------------
# Writer helper
# ---------------------------------------------------------------------------


class AwkWriter:
    """Manages AWK code output with proper indentation."""

    def __init__(self) -> None:
        self.lines: list[str] = []
        self.indent_level = 0

    def emit(self, line: str = "") -> None:
        if not line:
            self.lines.append("")
        else:
            self.lines.append("\t" * self.indent_level + line)

    def emit_raw(self, text: str) -> None:
        for ln in text.split("\n"):
            self.lines.append(ln)

    def indent(self) -> None:
        self.indent_level += 1

    def dedent(self) -> None:
        self.indent_level -= 1

    def get_output(self) -> str:
        return "\n".join(self.lines) + "\n"


# ---------------------------------------------------------------------------
# Emitter
# ---------------------------------------------------------------------------


class AwkEmitter:
    """Emit AWK AST nodes as POSIX AWK text."""

    def __init__(self, writer: AwkWriter) -> None:
        self.w = writer
        self._hoisted: list[str] = []  # lines to emit before current stmt
        self._va_counter = 0  # unique temp array counter

    # ------------------------------------------------------------------
    # Top-level
    # ------------------------------------------------------------------

    def emit_program(self, program: Program) -> None:
        for part in program.parts:
            match part:
                case FuncDef():
                    self.emit_func(part)
                    self.w.emit()
                case BeginBlock():
                    self.emit_begin(part)
                    self.w.emit()
                case VerbatimBlock(code=code):
                    self.w.emit_raw(code)
                    self.w.emit()
                case Comment(text=text):
                    self.w.emit(f"# {text}")

    def emit_func(self, func: FuncDef) -> None:
        params_str = ", ".join(func.params)
        if func.locals:
            locals_str = ", ".join(func.locals)
            if params_str:
                sig = f"{params_str},    {locals_str}"
            else:
                sig = f"    {locals_str}"
        else:
            sig = params_str
        self.w.emit(f"function {func.name}({sig}) {{")
        self.w.indent()
        self.emit_block(func.body)
        self.w.dedent()
        self.w.emit("}")

    def emit_begin(self, begin: BeginBlock) -> None:
        self.w.emit("BEGIN {")
        self.w.indent()
        self.emit_block(begin.body)
        self.w.dedent()
        self.w.emit("}")

    # ------------------------------------------------------------------
    # Block and statements
    # ------------------------------------------------------------------

    def emit_block(self, block: Block) -> None:
        if not block.stmts:
            return
        for stmt in block.stmts:
            self.emit_stmt(stmt)

    def _flush_hoisted(self) -> None:
        """Emit any hoisted setup lines (e.g., array population for varargs)."""
        for line in self._hoisted:
            self.w.emit(line)
        self._hoisted.clear()

    def emit_stmt(self, node: Stmt) -> None:
        match node:
            case Assign(target=target, value=value):
                tgt_s = self.expr(target)
                val_s = self.expr(value)
                self._flush_hoisted()
                self.w.emit(f"{tgt_s} = {val_s}")

            case If():
                self._emit_if(node)

            case For(init=init, cond=cond, incr=incr, body=body):
                init_s = self._stmt_inline(init) if init else ""
                cond_s = self.expr(cond) if cond else ""
                incr_s = self._stmt_inline(incr) if incr else ""
                self._flush_hoisted()
                self.w.emit(f"for ({init_s}; {cond_s}; {incr_s}) {{")
                self.w.indent()
                self.emit_block(body)
                self.w.dedent()
                self.w.emit("}")

            case ForIn(var=var, array=array, body=body):
                self.w.emit(f"for ({var} in {array}) {{")
                self.w.indent()
                self.emit_block(body)
                self.w.dedent()
                self.w.emit("}")

            case While(cond=cond, body=body):
                cond_s = self.expr(cond)
                self._flush_hoisted()
                self.w.emit(f"while ({cond_s}) {{")
                self.w.indent()
                self.emit_block(body)
                self.w.dedent()
                self.w.emit("}")

            case Return(value=None):
                self.w.emit("return")

            case Return(value=value):
                val_s = self.expr(value)
                self._flush_hoisted()
                self.w.emit(f"return {val_s}")

            case Print(args=args, dest=dest, append=append):
                args_s = ", ".join(self.expr(a) for a in args) if args else ""
                line = f"print {args_s}" if args_s else "print"
                if dest:
                    op = ">>" if append else ">"
                    line += f" {op} {self.expr(dest)}"
                self._flush_hoisted()
                self.w.emit(line)

            case Printf(fmt=fmt, args=args, dest=dest, append=append):
                parts = [self.expr(fmt)]
                parts.extend(self.expr(a) for a in args)
                line = "printf " + ", ".join(parts)
                if dest:
                    op = ">>" if append else ">"
                    line += f" {op} {self.expr(dest)}"
                self._flush_hoisted()
                self.w.emit(line)

            case ExprStmt(expr=FuncCall(name="v_export", args=args)) if (
                len(args) >= 2
            ):
                # v_export("NAME", value) → V["NAME"] = value; v_export("NAME")
                name_s = self.expr(args[0])
                val_s = self.expr(args[1])
                self._flush_hoisted()
                self.w.emit(f"V[{name_s}] = {val_s}")
                self.w.emit(f"v_export({name_s})")

            case ExprStmt(expr=e):
                line = self.expr(e)
                self._flush_hoisted()
                self.w.emit(line)

            case Delete(array=array, key=key):
                self.w.emit(f"delete {array}[{self.expr(key)}]")

            case DeleteArray(array=array):
                self.w.emit(f"delete {array}")

            case Break():
                self.w.emit("break")

            case Continue():
                self.w.emit("continue")

            case Exit(code=None):
                self.w.emit("exit")

            case Exit(code=code):
                self.w.emit(f"exit {self.expr(code)}")

            case Block():
                self.emit_block(node)

            case Comment(text=text):
                self.w.emit(f"# {text}")

            case _:
                raise ValueError(
                    f"unsupported AWK statement: {type(node).__name__}"
                )

    def _emit_if(self, node: If) -> None:
        # Track how many extra else blocks we open for hoisted vararg setup.
        extra_nesting = 0
        for i, branch in enumerate(node.branches):
            cond_s = self.expr(branch.cond)
            hoisted = list(self._hoisted)
            self._hoisted.clear()
            if i == 0:
                for h in hoisted:
                    self.w.emit(h)
                self.w.emit(f"if ({cond_s}) {{")
            elif hoisted:
                # Hoisted setup needed: close previous block with else,
                # nest the hoisted lines and new if inside it.
                self.w.emit("} else {")
                self.w.indent()
                extra_nesting += 1
                for h in hoisted:
                    self.w.emit(h)
                self.w.emit(f"if ({cond_s}) {{")
            else:
                self.w.emit(f"}} else if ({cond_s}) {{")
            self.w.indent()
            self.emit_block(branch.body)
            self.w.dedent()
        if node.else_body and node.else_body.stmts:
            self.w.emit("} else {")
            self.w.indent()
            self.emit_block(node.else_body)
            self.w.dedent()
        self.w.emit("}")
        # Close any extra nesting from hoisted else blocks
        for _ in range(extra_nesting):
            self.w.dedent()
            self.w.emit("}")

    def _stmt_inline(self, node: Stmt) -> str:
        """Render a statement inline (for for-loop init/incr)."""
        match node:
            case Assign(target=target, value=value):
                return f"{self.expr(target)} = {self.expr(value)}"
            case ExprStmt(expr=e):
                return self.expr(e)
            case _:
                raise ValueError(
                    f"cannot inline statement: {type(node).__name__}"
                )

    # ------------------------------------------------------------------
    # Expressions
    # ------------------------------------------------------------------

    def expr(self, node: Expr) -> str:
        match node:
            case StringLit(value=v):
                return f'"{_escape_awk_string(v)}"'

            case NumLit(value=v):
                return str(v)

            case Var(name=name):
                return name

            case ArrayVar(name=name):
                return name

            case ArrayRef(name=name, key=key):
                return f"{name}[{self.expr(key)}]"

            case BinOp(left=left, op=op, right=right):
                return f"({self.expr(left)} {op} {self.expr(right)})"

            case UnaryOp(op=op, expr=e):
                return f"({op}{self.expr(e)})"

            case Ternary(test=test, body=body, orelse=orelse):
                return (
                    f"({self.expr(test)} ? {self.expr(body)} : "
                    f"{self.expr(orelse)})"
                )

            case FuncCall(name=name, args=args):
                return self._emit_func_call(name, args)

            case Regex(pattern=pattern):
                return f"/{_escape_regex(pattern)}/"

            case Match(expr=e, pattern=pat, negated=neg):
                op = "!~" if neg else "~"
                return f"({self.expr(e)} {op} {self.expr(pat)})"

            case Concat(parts=parts):
                return " ".join(self.expr(p) for p in parts)

            case Sprintf(fmt=fmt, args=args):
                all_args = [f'"{_escape_awk_string(fmt)}"']
                all_args.extend(self.expr(a) for a in args)
                return f"sprintf({', '.join(all_args)})"

            case InArray(key=key, array=array):
                return f"({self.expr(key)} in {array})"

            case Getline(cmd=cmd, var=var):
                return f"({self.expr(cmd)} | getline {var})"

            case Substr(string=s, start=start, length=None):
                return f"substr({self.expr(s)}, {self.expr(start)})"

            case Substr(string=s, start=start, length=length):
                return (
                    f"substr({self.expr(s)}, {self.expr(start)}, "
                    f"{self.expr(length)})"
                )

            case Length(expr=e):
                return f"length({self.expr(e)})"

            case Index(haystack=h, needle=n):
                return f"index({self.expr(h)}, {self.expr(n)})"

            case Split(string=s, array=array, sep=None):
                return f"split({self.expr(s)}, {array})"

            case Split(string=s, array=array, sep=sep):
                return f"split({self.expr(s)}, {array}, {self.expr(sep)})"

            case Sub(pattern=pat, replacement=repl, target=target, global_=g):
                func = "gsub" if g else "sub"
                return (
                    f"{func}({self.expr(pat)}, {self.expr(repl)}, "
                    f"{self.expr(target)})"
                )

            case Tolower(expr=e):
                return f"tolower({self.expr(e)})"

            case Toupper(expr=e):
                return f"toupper({self.expr(e)})"

            case System(cmd=cmd):
                return f"system({self.expr(cmd)})"

            case _:
                raise ValueError(
                    f"unsupported AWK expression: {type(node).__name__}"
                )

    # ------------------------------------------------------------------
    # Varargs function call handling
    # ------------------------------------------------------------------

    # Maps function name → (fixed_leading, fixed_trailing).
    # Variable args between fixed_leading and len-fixed_trailing are packed
    # into a temp AWK array and passed as one array argument.
    _VARARGS_FUNCS: dict[str, tuple[int, int]] = {
        "pyconf_check_compile_flag": (1, 0),
        "pyconf_check_progs": (0, 0),
        "pyconf_check_tools": (0, 0),
        "pyconf_cmd": (0, 0),
        "pyconf_cmd_output": (0, 0),
        "pyconf_cmd_status": (0, 1),  # last arg is result array
        "pyconf_fnmatch_any": (1, 0),  # first arg is string, rest are patterns
        "pyconf_stdlib_module_set_na": (0, 0),
        "pyconf_path_join": (0, 0),
        "pyconf_check_funcs": (0, 0),
        "pyconf_check_headers": (0, 0),
        "pyconf_replace_funcs": (0, 0),
        "pyconf_check_decls": (0, 0),
        "pyconf_config_files": (0, 0),
        "pyconf_config_files_x": (0, 0),
        "pyconf_shell": (0, 0),
    }

    def _next_va_name(self) -> str:
        self._va_counter += 1
        return f"_va{self._va_counter}"

    def _emit_func_call(self, name: str, args: list) -> str:
        spec = self._VARARGS_FUNCS.get(name)
        if spec is not None:
            lead, trail = spec
            total_fixed = lead + trail
            if len(args) > total_fixed:
                end = len(args) - trail if trail else len(args)
                va_args = args[lead:end]
                # If the sole variadic arg is an ArrayVar, pass it directly
                # instead of wrapping in a temporary array — it is already
                # a properly-formatted array (element 0 = count).
                if len(va_args) == 1 and isinstance(va_args[0], ArrayVar):
                    call_args: list[str] = []
                    call_args.extend(self.expr(a) for a in args[:lead])
                    call_args.append(va_args[0].name)
                    call_args.extend(self.expr(a) for a in args[end:])
                    return f"{name}({', '.join(call_args)})"
                # Pack variable args into a temp array via hoisted lines.
                # Lines are stored WITHOUT indentation — _flush_hoisted()
                # emits them via self.w.emit() which adds the current indent.
                va_name = self._next_va_name()
                self._hoisted.append(f"delete {va_name}")
                idx = 1
                for arg in va_args:
                    self._hoisted.append(
                        f"{va_name}[{idx}] = {self.expr(arg)}"
                    )
                    idx += 1
                self._hoisted.append(f"{va_name}[0] = {idx - 1}")
                call_args = []
                call_args.extend(self.expr(a) for a in args[:lead])
                call_args.append(va_name)
                call_args.extend(self.expr(a) for a in args[end:])
                return f"{name}({', '.join(call_args)})"
        args_s = ", ".join(self.expr(a) for a in args)
        return f"{name}({args_s})"


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------


def generate_code(program: Program) -> str:
    """Generate AWK source code from an AWK AST Program."""
    w = AwkWriter()
    emitter = AwkEmitter(w)
    emitter.emit_program(program)
    return w.get_output()
