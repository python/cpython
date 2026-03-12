"""py_to_pysh.py — Transform 1: Python AST → pysh_ast.

Simple structural walk.  The only non-trivial decision made here is
resolving :class:`~pysh_ast.CallType` for each :class:`~pysh_ast.Call`
node; all semantic lowering is deferred to Transform 2
(:mod:`pysh_to_awk`).

Pipeline position::

    Python AST  --[this module]-->  pysh_ast
                --[pysh_to_awk]-->   awk_ast
                --[awk_emit]-->      awk text
"""

from __future__ import annotations

import ast
import glob
import os

from .pysh_ast import (
    Add,
    Assign,
    Attr,
    AugAssign,
    Block,
    BoolOp,
    Break,
    Call,
    CallType,
    Cmp,
    Const,
    Continue,
    Dict,
    Expr,
    ExprStmt,
    FStr,
    For,
    FuncDef,
    If,
    In,
    List,
    ModuleConst,
    ModuleInfo,
    Not,
    OPTION_BOOL_METHODS,
    OPTION_BOOL_PROPS,
    OPTION_RETVAL_METHODS,
    OPTION_VALUE_METHODS,
    OPTION_VALUE_PROPS,
    OptionInfo,
    Program,
    PYCONF_BOOL_FUNCS,
    PYCONF_RETVAL_FUNCS,
    PYCONF_STDOUT_FUNCS,
    Return,
    Stmt,
    Subscript,
    Ternary,
    Var,
    With,
)

# ---------------------------------------------------------------------------
# Module-level helpers
# ---------------------------------------------------------------------------


def _is_docstring(node: ast.stmt) -> bool:
    match node:
        case ast.Expr(value=ast.Constant(value=str())):
            return True
        case _:
            return False


def _attr_str(node: ast.expr) -> str:
    """Return a dotted-string representation of an attribute chain.

    E.g. ``ast.Attribute(Name("pyconf"), "check_header")``
    → ``"pyconf.check_header"``.
    """
    match node:
        case ast.Name(id=name):
            return name
        case ast.Attribute(value=value, attr=attr):
            return f"{_attr_str(value)}.{attr}"
        case _:
            return "<complex>"


def _cmp_op_str(op: ast.cmpop) -> str:
    """Map a Python AST comparison operator to a string."""
    match op:
        case ast.Eq():
            return "=="
        case ast.NotEq():
            return "!="
        case ast.Lt():
            return "<"
        case ast.LtE():
            return "<="
        case ast.Gt():
            return ">"
        case ast.GtE():
            return ">="
        case ast.Is():
            return "is"
        case ast.IsNot():
            return "is not"
        case _:
            return "=="


# ---------------------------------------------------------------------------
# Main transform class
# ---------------------------------------------------------------------------


class PyToPysh:
    """Transform Python function definitions into :class:`~pysh_ast.FuncDef`.

    Construct once per translation unit; reuse across functions in a module.

    This is Transform 1 in the two-stage pipeline.  The only non-trivial
    work done here is resolving :class:`~pysh_ast.CallType` for each
    :class:`~pysh_ast.Call`.  No shell lowering takes place.
    """

    def __init__(
        self,
        module: ModuleInfo,
        all_option_vars: dict[str, OptionInfo],
    ) -> None:
        self.module = module
        self.option_vars = all_option_vars

    # ------------------------------------------------------------------
    # Top-level entry points
    # ------------------------------------------------------------------

    def transform_func(self, func: ast.FunctionDef) -> FuncDef:
        """Transform a Python function definition into a :class:`~pysh_ast.FuncDef`."""
        params = [arg.arg for arg in func.args.args if arg.arg != "v"]

        # Collect default values for parameters
        defaults: dict[str, Expr] = {}
        n_args = len(func.args.args)
        n_defaults = len(func.args.defaults)
        for i, default in enumerate(func.args.defaults):
            arg_idx = n_args - n_defaults + i
            arg_name = func.args.args[arg_idx].arg
            if arg_name != "v":
                defaults[arg_name] = self.expr(default)

        body_stmts = [s for s in func.body if not _is_docstring(s)]
        return FuncDef(
            name=func.name,
            params=params,
            defaults=defaults,
            body=self.block(body_stmts),
        )

    def block(self, stmts: list[ast.stmt]) -> Block:
        """Transform a list of Python statements into a :class:`~pysh_ast.Block`."""
        result: list[Stmt] = []
        for s in stmts:
            if _is_docstring(s):
                continue
            translated = self.stmt(s)
            if translated is not None:
                result.append(translated)
        return Block(result)

    # ------------------------------------------------------------------
    # Statement dispatch
    # ------------------------------------------------------------------

    def stmt(self, node: ast.stmt) -> Stmt | None:
        """Translate one Python statement to a pysh_ast Stmt (or None to skip)."""
        match node:
            case ast.Expr(value=ast.Constant(value=str())):
                return None  # docstring — already filtered, but just in case
            case ast.Expr(value=val):
                return ExprStmt(self.expr(val))
            case ast.Assign(targets=[target], value=value):
                return Assign(self.expr(target), self.expr(value))
            case ast.Assign(targets=targets, value=value):
                # Multiple assignment targets: take the first
                return Assign(self.expr(targets[0]), self.expr(value))
            case ast.AnnAssign(target=target, value=value) if (
                value is not None
            ):
                return Assign(self.expr(target), self.expr(value))
            case ast.AnnAssign():
                return None  # bare declaration (no value)
            case ast.AugAssign(target=target, op=op, value=value):
                return AugAssign(
                    self.expr(target), type(op).__name__, self.expr(value)
                )
            case ast.If(test=test, body=body, orelse=orelse):
                return If(
                    self.expr(test),
                    self.block(body),
                    self.block(orelse) if orelse else None,
                )
            case ast.For(target=target, iter=iter_, body=body):
                var = target.id if isinstance(target, ast.Name) else "_iter"
                return For(var, self.expr(iter_), self.block(body))
            case ast.With(items=items, body=body):
                ctx = (
                    self.expr(items[0].context_expr) if items else Const(None)
                )
                return With(ctx, self.block(body))
            case ast.Return(value=value):
                return Return(self.expr(value) if value is not None else None)
            case ast.Pass():
                return None
            case ast.Break():
                return Break()
            case ast.Continue():
                return Continue()
            case _:
                # Unsupported statement — emit a no-op placeholder
                return ExprStmt(Const(f"# TODO: {type(node).__name__}"))

    # ------------------------------------------------------------------
    # Expression dispatch
    # ------------------------------------------------------------------

    def expr(self, node: ast.expr | None) -> Expr:
        """Translate a Python expression node to a pysh_ast Expr."""
        if node is None:
            return Const(None)
        match node:
            case ast.Constant(value=value):
                return Const(value)
            case ast.Name(id=name):
                return Var(name)
            case ast.Attribute():
                return self._attr_expr(node)
            case ast.JoinedStr():
                return self._fstr_expr(node)
            case ast.BinOp(left=left, op=ast.Add(), right=right):
                return Add(self.expr(left), self.expr(right))
            case ast.BinOp(left=left, op=ast.USub()) if False:
                # USub handled below
                pass
            case ast.UnaryOp(op=ast.USub(), operand=operand):
                # Negate: -x → Add("-", x)
                return Add(Const("-"), self.expr(operand))
            case ast.BinOp():
                return Const("")  # unsupported binary op
            case ast.BoolOp(op=op, values=values):
                op_str = "and" if isinstance(op, ast.And) else "or"
                return BoolOp(op_str, [self.expr(v) for v in values])
            case ast.UnaryOp(op=ast.Not(), operand=operand):
                return Not(self.expr(operand))
            case ast.Compare():
                return self._compare_expr(node)
            case ast.IfExp(test=test, body=body, orelse=orelse):
                return Ternary(
                    self.expr(test), self.expr(body), self.expr(orelse)
                )
            case ast.Call():
                return self._call_expr(node)
            case ast.Subscript(value=value, slice=slice_):
                return Subscript(self.expr(value), self.expr(slice_))
            case ast.List(elts=elts) | ast.Tuple(elts=elts):
                return List([self.expr(e) for e in elts])
            case ast.Set(elts=elts):
                return List([self.expr(e) for e in elts])
            case ast.Dict(keys=keys, values=vals):
                str_keys = [
                    str(k.value) if isinstance(k, ast.Constant) else None
                    for k in keys
                ]
                return Dict(
                    keys=[k for k in str_keys if k is not None],
                    values=[
                        self.expr(v)
                        for k, v in zip(str_keys, vals)
                        if k is not None
                    ],
                )
            case ast.Starred(value=value):
                return self.expr(value)
            case ast.FormattedValue(value=value):
                return self.expr(value)
            case _:
                return Const("")  # unsupported expression

    def _attr_expr(self, node: ast.Attribute) -> Expr:
        """Translate ``ast.Attribute`` → :class:`~pysh_ast.Attr`."""
        obj_str = _attr_str(node.value)
        return Attr(obj_str, node.attr)

    def _fstr_expr(self, node: ast.JoinedStr) -> FStr:
        """Translate ``ast.JoinedStr`` → :class:`~pysh_ast.FStr`."""
        parts: list = []
        for val in node.values:
            match val:
                case ast.Constant(value=str() as s):
                    parts.append(s)
                case ast.FormattedValue(value=inner):
                    parts.append(self.expr(inner))
                case _:
                    parts.append(self.expr(val))
        return FStr(parts)

    def _compare_expr(self, node: ast.Compare) -> Expr:
        """Translate ``ast.Compare`` → :class:`~pysh_ast.Cmp`, :class:`~pysh_ast.In`, or :class:`~pysh_ast.BoolOp`."""
        if len(node.ops) > 1:
            # Chained comparison: a <= x <= b → BoolOp("and", [a<=x, x<=b])
            parts: list[Expr] = []
            prev: ast.expr = node.left
            for op, right in zip(node.ops, node.comparators):
                match op:
                    case ast.In():
                        parts.append(In(self.expr(prev), self.expr(right)))
                    case ast.NotIn():
                        parts.append(
                            In(self.expr(prev), self.expr(right), negated=True)
                        )
                    case _:
                        parts.append(
                            Cmp(
                                self.expr(prev),
                                _cmp_op_str(op),
                                self.expr(right),
                            )
                        )
                prev = right
            return BoolOp("and", parts)

        op = node.ops[0]
        right = node.comparators[0]
        left = node.left
        match op:
            case ast.In():
                return In(self.expr(left), self.expr(right))
            case ast.NotIn():
                return In(self.expr(left), self.expr(right), negated=True)
            case _:
                return Cmp(self.expr(left), _cmp_op_str(op), self.expr(right))

    def _call_expr(self, node: ast.Call) -> Call:
        """Translate ``ast.Call`` → :class:`~pysh_ast.Call` with resolved :class:`~pysh_ast.CallType`."""
        func_str = _attr_str(node.func)
        call_type = self._resolve_call_type(node.func)

        # Translate positional args.  'v' (the Vars object) is preserved here;
        # Transform 2 decides whether to use or strip it.
        args: list[Expr] = []

        # For chained method calls where the receiver is a complex expression
        # (e.g., x.replace("/", "").lower()), _attr_str can't flatten the chain
        # and returns "<complex>.method".  Inject the lowered receiver as args[0]
        # so pysh_to_awk can recover it.
        if func_str.startswith("<complex>.") and isinstance(
            node.func, ast.Attribute
        ):
            args.append(self.expr(node.func.value))

        for arg in node.args:
            match arg:
                case ast.Starred(value=val):
                    args.append(self.expr(val))
                case _:
                    args.append(self.expr(arg))

        # Translate keyword args.
        kwargs: dict[str, Expr] = {}
        for kw in node.keywords:
            if kw.arg is not None:
                kwargs[kw.arg] = self.expr(kw.value)

        # Bool functions with a value= kwarg also produce stdout output;
        # promote to STDOUT so the transpiler doesn't wrap in if/yes/no.
        if call_type == CallType.BOOL and "value" in kwargs:
            call_type = CallType.STDOUT

        return Call(
            func=func_str, args=args, kwargs=kwargs, call_type=call_type
        )

    # ------------------------------------------------------------------
    # CallType resolution
    # ------------------------------------------------------------------

    def _resolve_call_type(self, func: ast.expr) -> CallType:
        """Determine the :class:`~pysh_ast.CallType` for a call.

        Priority order:

        1. ``pyconf.<name>`` where name in ``PYCONF_BOOL_FUNCS`` → BOOL
        2. ``pyconf.<name>`` where name in ``PYCONF_STDOUT_FUNCS`` → STDOUT
        3. ``pyconf.<name>`` where name in ``PYCONF_RETVAL_FUNCS`` → RETVAL
        4. ``pyconf.pkg_check_modules`` → MULTI
        5. ``OPTION.<method>`` where method in ``OPTION_BOOL_METHODS/PROPS`` → BOOL
        6. ``OPTION.<method>`` where method in ``OPTION_RETVAL_METHODS`` → RETVAL
        7. ``OPTION.<method>`` where method in ``OPTION_VALUE_METHODS/PROPS`` → STDOUT
        8. Everything else → VOID
        """
        match func:
            case ast.Attribute(value=ast.Name(id=obj), attr=attr):
                pass
            case _:
                return CallType.VOID

        if obj == "pyconf":
            if attr == "pkg_check_modules":
                return CallType.MULTI
            if attr in PYCONF_BOOL_FUNCS:
                return CallType.BOOL
            if attr in PYCONF_STDOUT_FUNCS:
                return CallType.STDOUT
            if attr in PYCONF_RETVAL_FUNCS:
                return CallType.RETVAL
            return CallType.VOID

        opt = self.option_vars.get(obj)
        if opt is not None:
            if attr in OPTION_BOOL_METHODS or attr in OPTION_BOOL_PROPS:
                return CallType.BOOL
            if attr in OPTION_RETVAL_METHODS:
                return CallType.RETVAL
            if attr in OPTION_VALUE_METHODS or attr in OPTION_VALUE_PROPS:
                return CallType.STDOUT

        return CallType.VOID


# ---------------------------------------------------------------------------
# Option parsing helpers
# ---------------------------------------------------------------------------


def _normalize_opt_name(name: str) -> str:
    return name.replace("-", "_")


def _extract_string_kwarg(node: ast.Call, name: str) -> str | None:
    for kw in node.keywords:
        if kw.arg == name and isinstance(kw.value, ast.Constant):
            return str(kw.value.value)
    return None


def _extract_kwarg_value(node: ast.Call, name: str) -> object | None:
    for kw in node.keywords:
        if kw.arg == name:
            if isinstance(kw.value, ast.Constant):
                return kw.value.value
            return "<expr>"
    return None


def parse_option_decl(
    python_var: str, call_node: ast.Call
) -> OptionInfo | None:
    """Parse a module-level pyconf.arg_with/arg_enable call."""
    func = call_node.func
    if not (
        isinstance(func, ast.Attribute)
        and isinstance(func.value, ast.Name)
        and func.value.id == "pyconf"
    ):
        return None
    if func.attr not in ("arg_with", "arg_enable"):
        return None

    kind = "with" if func.attr == "arg_with" else "enable"
    if not call_node.args or not isinstance(call_node.args[0], ast.Constant):
        return None
    name = _normalize_opt_name(str(call_node.args[0].value))
    key = f"{kind}_{name}"

    return OptionInfo(
        python_var=python_var,
        kind=kind,
        name=name,
        key=key,
        var=_extract_string_kwarg(call_node, "var"),
        default=_extract_kwarg_value(call_node, "default"),
        help_text=_extract_string_kwarg(call_node, "help") or "",
        metavar=_extract_string_kwarg(call_node, "metavar") or "",
        false_is=_extract_string_kwarg(call_node, "false_is"),
        display=_extract_string_kwarg(call_node, "display"),
    )


def _parse_module_options(filepath: str) -> ModuleInfo:
    """Pass 1: parse a conf_*.py for options and constants (no function transform)."""
    name = os.path.splitext(os.path.basename(filepath))[0]
    with open(filepath) as f:
        source = f.read()
    tree = ast.parse(source, filename=filepath)
    mod = ModuleInfo(name=name, filepath=filepath)

    for child in tree.body:
        match child:
            case ast.Assign(targets=targets, value=value):
                for target in targets:
                    match target:
                        case ast.Name(id=var_name):
                            match value:
                                case ast.Call():
                                    opt = parse_option_decl(var_name, value)
                                    if opt:
                                        mod.options.append(opt)
                                        mod.option_var_map[var_name] = opt
                                case ast.Constant(value=val) if (
                                    var_name not in mod.option_var_map
                                ):
                                    if isinstance(
                                        val, (str, int, float, bool)
                                    ):
                                        mod.constants.append(
                                            ModuleConst(var_name, val)
                                        )
    return mod


def parse_module(
    filepath: str, all_option_vars: dict[str, OptionInfo]
) -> ModuleInfo:
    """Pass 2: parse a conf_*.py file and transform functions with PyToPysh."""
    name = os.path.splitext(os.path.basename(filepath))[0]
    with open(filepath) as f:
        source = f.read()
    tree = ast.parse(source, filename=filepath)
    mod = ModuleInfo(name=name, filepath=filepath)

    for child in tree.body:
        match child:
            case ast.Assign(targets=targets, value=value):
                for target in targets:
                    match target:
                        case ast.Name(id=var_name):
                            match value:
                                case ast.Call():
                                    opt = parse_option_decl(var_name, value)
                                    if opt:
                                        mod.options.append(opt)
                                        mod.option_var_map[var_name] = opt
                                case ast.Constant(value=val) if (
                                    var_name not in mod.option_var_map
                                ):
                                    if isinstance(
                                        val, (str, int, float, bool)
                                    ):
                                        mod.constants.append(
                                            ModuleConst(var_name, val)
                                        )
            case ast.FunctionDef():
                transformer = PyToPysh(mod, all_option_vars)
                mod.functions.append(transformer.transform_func(child))

    return mod


def parse_run_order(configure_py: str) -> list[str]:
    """Parse configure.py's _run() to get the ordered list of calls.

    Returns a list of "module.function" strings.
    """
    with open(configure_py) as f:
        source = f.read()
    tree = ast.parse(source, filename=configure_py)

    for child in tree.body:
        match child:
            case ast.FunctionDef(name="_run", body=body):
                calls = []
                for s in body:
                    match s:
                        case ast.Expr(
                            value=ast.Call(
                                func=ast.Attribute(
                                    value=ast.Name(id=mod_name), attr=func_name
                                )
                            )
                        ):
                            calls.append(f"{mod_name}.{func_name}")
                return calls
    return []


def parse_program(script_dir: str) -> Program:
    """Parse all conf_*.py files and configure.py into a Program.

    Two-pass approach:
      Pass 1: collect all OptionInfo across all modules.
      Pass 2: transform functions (so cross-module option vars are resolved).
    """
    pattern = os.path.join(script_dir, "conf_*.py")
    filepaths = sorted(glob.glob(pattern))

    # Pass 1: collect options from all modules
    all_option_vars: dict[str, OptionInfo] = {}
    for filepath in filepaths:
        mod = _parse_module_options(filepath)
        all_option_vars.update(mod.option_var_map)

    # Pass 2: parse modules with function transformation
    modules = []
    for filepath in filepaths:
        mod = parse_module(filepath, all_option_vars)
        modules.append(mod)

    configure_py = os.path.join(script_dir, "configure.py")
    run_order: list[str] = []
    if os.path.exists(configure_py):
        run_order = parse_run_order(configure_py)

    return Program(
        script_dir=script_dir,
        modules=modules,
        all_option_vars=all_option_vars,
        run_order=run_order,
    )
