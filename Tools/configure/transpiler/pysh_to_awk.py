"""pysh_to_awk.py — Transform pysh_ast → awk_ast.

Semantic lowering from the Python-shaped AST (with CallType annotations)
to an AWK AST.  Simpler than a shell backend would be because
AWK has native return values, associative arrays, and rich string builtins.

Pipeline position::

    pysh_ast  --[this module]-->  awk_ast
              --[awk_emit]-->     awk text
"""

from __future__ import annotations

from .pysh_ast import (
    Add,
    Assign as PyshAssign,
    Attr,
    AugAssign,
    Block as PyshBlock,
    BoolOp,
    Break as PyshBreak,
    Call,
    CallType,
    Cmp,
    Const,
    Continue as PyshContinue,
    Dict as PyshDict,
    Expr as PyshExpr,
    ExprStmt,
    FStr,
    For as PyshFor,
    FuncDef as PyshFuncDef,
    If as PyshIf,
    In,
    List as PyshList,
    ModuleInfo,
    Not,
    OPTION_BOOL_METHODS,
    OPTION_BOOL_PROPS,
    OPTION_VALUE_METHODS,
    OPTION_VALUE_PROPS,
    OptionInfo,
    Program as PyshProgram,
    Return as PyshReturn,
    Stmt as PyshStmt,
    Subscript,
    Ternary,
    Var,
    With as PyshWith,
)
from . import awk_ast as A

# ---------------------------------------------------------------------------
# Transpile-time precomputation helpers
# ---------------------------------------------------------------------------


def _precompute_func_define(func: str) -> str:
    return "HAVE_" + func.upper()


def _precompute_header_define(header: str) -> str:
    return "HAVE_" + header.upper().replace("/", "_").replace(
        ".", "_"
    ).replace("+", "_").replace("-", "_")


def _precompute_header_cache_key(header: str) -> str:
    return "ac_cv_header_" + header.replace("/", "_").replace(
        ".", "_"
    ).replace("+", "_").replace("-", "_")


def _precompute_member_define(member: str) -> str:
    struct, _, field = member.partition(".")
    return "HAVE_" + struct.upper().replace(" ", "_") + "_" + field.upper()


def _precompute_decl_define(decl: str) -> str:
    return "HAVE_DECL_" + decl.upper()


_KNOWN_HEADER_DIRS = {
    "sys",
    "linux",
    "netinet",
    "netpacket",
    "net",
    "arpa",
    "machine",
    "asm",
    "bluetooth",
    "readline",
    "editline",
    "openssl",
    "db",
    "gdbm",
    "uuid",
    "pthread",
}


def _define_to_header(define: str) -> str:
    raw = define.removeprefix("HAVE_").lower()
    if raw.endswith("_h"):
        raw = raw[:-2]
    parts = raw.split("_", 1)
    if len(parts) == 2 and parts[0] in _KNOWN_HEADER_DIRS:
        return parts[0] + "/" + parts[1] + ".h"
    return raw + ".h"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _has_v_arg(call: Call) -> bool:
    return any(isinstance(a, Var) and a.name == "v" for a in call.args)


def _is_int_expr(e: PyshExpr) -> bool:
    match e:
        case Const(value=int()):
            return True
        case Call(func="int"):
            return True
        case _:
            return False


def _looks_like_full_source(e: PyshExpr) -> bool:
    match e:
        case Const(value=str() as s):
            return s.lstrip().startswith("#") or "main" in s
        case _:
            return False


def _is_complete_program(e: PyshExpr) -> bool:
    match e:
        case Const(value=str() as s):
            return "int main" in s or "return 0;" in s
        case _:
            return False


def _is_pyconf_attr(func: str) -> bool:
    return func.startswith("pyconf.")


# AWK reserved words and builtin function names that cannot be used as
# variable/parameter names.
_AWK_RESERVED = frozenset(
    {
        "BEGIN",
        "END",
        "break",
        "close",
        "continue",
        "delete",
        "do",
        "else",
        "exit",
        "for",
        "func",
        "function",
        "getline",
        "gsub",
        "if",
        "in",
        "length",
        "match",
        "next",
        "print",
        "printf",
        "return",
        "split",
        "sub",
        "substr",
        "system",
        "tolower",
        "toupper",
        "while",
        "sprintf",
        "index",
        "atan2",
        "cos",
        "exp",
        "int",
        "log",
        "rand",
        "sin",
        "sqrt",
        "srand",
    }
)


def _safe_awk_name(name: str) -> str:
    """Rename an identifier if it clashes with an AWK reserved word."""
    if name in _AWK_RESERVED:
        return f"_{name}_"
    return name


# Prefix for all user-defined (transpiled) function names, to avoid
# collisions with AWK builtins and pyconf runtime functions.
_USER_FUNC_PREFIX = "u_"


# ---------------------------------------------------------------------------
# Main transform class
# ---------------------------------------------------------------------------


class PyshToAwk:
    """Transform pysh_ast.FuncDef into awk_ast.FuncDef.

    This is Transform 2 for the AWK backend.  Construct once per module;
    reuse across functions.
    """

    # Class-level counter so array local names are globally unique across
    # all functions.  Avoids gawk 5.2 bug with same-named array locals
    # in caller and callee.
    _global_arr_counter: int = 0

    def __init__(
        self,
        module: ModuleInfo,
        all_option_vars: dict[str, OptionInfo],
        known_funcs: set[str] | None = None,
        global_constants: set[str] | None = None,
    ) -> None:
        self.module = module
        self.option_vars = all_option_vars
        self._known_funcs: set[str] = known_funcs or set()
        self._global_constants: set[str] = global_constants or set()
        self._pkg_var_map: dict[str, str] = {}
        self._locals: set[str] = set()
        self._array_locals: set[str] = set()  # locals used as arrays
        self._pre_stmts: list = []  # statements to emit before current if/while

    # ------------------------------------------------------------------
    # Pre-scan for array variables
    # ------------------------------------------------------------------

    def _pre_scan_arrays(self, block: PyshBlock) -> None:
        """Walk the function body to find variables that are used as arrays.

        This must run before lowering so that assignments like
        ``var = "...".split()`` can be converted to AWK split() into an array
        instead of a scalar assignment.
        """
        for stmt in block.stmts:
            self._pre_scan_stmt(stmt)
        # Promote split-assigned variables to array_locals when they are
        # passed as arguments to varargs functions (e.g. check_funcs).
        self._promote_split_vars_used_as_arrays(block)

    def _pre_scan_stmt(self, node: PyshStmt) -> None:
        match node:
            case ExprStmt(expr=Call(func=f)) if f.endswith(".append"):
                obj = f.rpartition(".")[0]
                if obj and "." not in obj:
                    self._array_locals.add(obj)
            case PyshAssign(target=Var(name=var), value=PyshDict()):
                self._array_locals.add(var)
            case PyshAssign(target=Subscript(obj=Var(name=var))):
                self._array_locals.add(var)
            case PyshAssign(target=Var(name=var), value=Call(func=f)) if (
                f.endswith(".split")
            ):
                self._split_assigned_vars.add(var)
            case PyshAssign(target=Var(name=var), value=PyshList()):
                self._split_assigned_vars.add(var)
            case PyshIf(body=body, orelse=orelse):
                self._pre_scan_arrays(body)
                if orelse:
                    self._pre_scan_arrays(orelse)
            case PyshFor(body=body):
                self._pre_scan_arrays(body)
            case _:
                pass

    def _promote_split_vars_used_as_arrays(self, block: PyshBlock) -> None:
        """Promote split-assigned vars to _array_locals when used as args
        to varargs functions."""
        if not self._split_assigned_vars:
            return
        for stmt in block.stmts:
            self._scan_for_array_usage(stmt)

    def _scan_for_array_usage(self, node: PyshStmt) -> None:
        """Find calls that pass split-assigned vars to varargs functions."""
        calls: list[Call] = []
        match node:
            case ExprStmt(expr=Call() as call):
                calls.append(call)
            case PyshAssign(value=Call() as call):
                calls.append(call)
            case PyshIf(test=test, body=body, orelse=orelse):
                # Also scan the condition expression for varargs calls
                self._collect_calls_from_expr(test, calls)
                for s in body.stmts:
                    self._scan_for_array_usage(s)
                if orelse:
                    for s in orelse.stmts:
                        self._scan_for_array_usage(s)
            case PyshFor(body=body):
                for s in body.stmts:
                    self._scan_for_array_usage(s)
        for call in calls:
            method = call.func.rpartition(".")[2]
            if method in self._VARARGS_CALL_NAMES:
                for arg in call.args:
                    if (
                        isinstance(arg, Var)
                        and arg.name in self._split_assigned_vars
                    ):
                        self._array_locals.add(arg.name)

    def _collect_calls_from_expr(
        self, expr: PyshExpr, calls: list[Call]
    ) -> None:
        """Recursively collect Call nodes from an expression."""
        match expr:
            case Call():
                calls.append(expr)
            case Not(operand=operand):
                self._collect_calls_from_expr(operand, calls)
            case BoolOp(values=values):
                for v in values:
                    self._collect_calls_from_expr(v, calls)
            case _:
                pass

    # ------------------------------------------------------------------
    # Top-level
    # ------------------------------------------------------------------

    def transform_func(self, func: PyshFuncDef) -> A.FuncDef:
        self._locals = set()
        self._array_locals = set()
        self._split_assigned_vars: set[str] = set()
        self._has_multi_return = False
        # Pre-scan to identify variables used as arrays (via .append(),
        # dict literal assignment, subscript assignment, etc.) so that
        # assignments like var = "...".split() can be handled correctly.
        self._pre_scan_arrays(func.body)
        # Build rename map for params that clash with AWK reserved words
        self._rename_map: dict[str, str] = {}
        for p in func.params:
            safe = _safe_awk_name(p)
            if safe != p:
                self._rename_map[p] = safe
        body = self._lower_block(func.body)
        # Remove params from locals (they are already parameters)
        param_set = set(func.params)
        # Scalar locals: exclude params, arrays, and global constants
        scalar_names = sorted(
            self._locals
            - param_set
            - self._array_locals
            - self._global_constants
        )
        # Array locals: exclude params
        array_names = sorted(self._array_locals - param_set)
        # Apply renaming to locals too
        safe_locals = [_safe_awk_name(n) for n in scalar_names]
        safe_params = [_safe_awk_name(p) for p in func.params]
        # If the function has multi-value returns, _result must be an
        # explicit parameter (last) so callers can pass their result
        # array there.
        if self._has_multi_return:
            safe_params.append("_result")
        # Array locals go into the locals list too (AWK needs them
        # declared) but we also emit delete statements at function entry.
        all_locals = safe_locals + array_names
        # Prepend delete statements for each array local
        delete_stmts: list[A.Stmt] = []
        for arr in array_names:
            delete_stmts.append(A.DeleteArray(arr))
        if delete_stmts:
            body = A.Block(delete_stmts + body.stmts)
        return A.FuncDef(
            name=_USER_FUNC_PREFIX + func.name,
            params=safe_params,
            locals=all_locals,
            body=body,
        )

    def _new_array_local(self, prefix: str = "_arr") -> str:
        """Generate a unique array local variable name."""
        PyshToAwk._global_arr_counter += 1
        name = f"{prefix}_{PyshToAwk._global_arr_counter}"
        self._array_locals.add(name)
        return name

    def _var_name(self, name: str) -> str:
        """Map a Python variable name to its AWK equivalent, handling renames."""
        return _safe_awk_name(self._rename_map.get(name, name))

    def _func_name(self, name: str) -> str:
        """Prefix a user-defined function name."""
        return _USER_FUNC_PREFIX + name

    def _lower_block(self, block: PyshBlock) -> A.Block:
        stmts: list[A.Stmt] = []
        for node in block.stmts:
            stmts.extend(self._lower_stmt(node))
        return A.Block(stmts)

    # ------------------------------------------------------------------
    # Statement lowering
    # ------------------------------------------------------------------

    def _lower_stmt(self, node: PyshStmt) -> list[A.Stmt]:
        match node:
            case ExprStmt(expr=Call() as call):
                return self._call_stmt(call)
            case ExprStmt(expr=expr):
                raise ValueError(
                    f"unsupported bare expression: {type(expr).__name__}"
                )
            case PyshAssign():
                return self._stmt_assign(node)
            case AugAssign():
                return self._stmt_aug_assign(node)
            case PyshIf():
                return self._stmt_if(node)
            case PyshFor():
                return self._stmt_for(node)
            case PyshWith():
                return self._stmt_with(node)
            case PyshReturn():
                return self._stmt_return(node)
            case PyshBreak():
                return [A.Break()]
            case PyshContinue():
                return [A.Continue()]
            case _:
                raise ValueError(
                    f"unsupported statement: {type(node).__name__}"
                )

    # ------------------------------------------------------------------
    # Call as statement
    # ------------------------------------------------------------------

    def _call_stmt(self, call: Call) -> list[A.Stmt]:
        func = call.func

        # pyconf.method(...)
        if _is_pyconf_attr(func):
            return self._pyconf_call_stmt(func.removeprefix("pyconf."), call)

        # v.method(...)
        if func.startswith("v."):
            method = func.removeprefix("v.")
            return [
                A.ExprStmt(A.FuncCall(f"v_{method}", self._awk_args(call)))
            ]

        # OPTION.method(...)
        opt = self._option_from_call(call)
        if opt is not None:
            attr = func.rpartition(".")[2]
            return self._option_method_stmt(opt, attr, call)

        # conf_module.function(...)
        if "." in func:
            obj, _, attr = func.rpartition(".")
            if obj.startswith("conf_"):
                return [
                    A.ExprStmt(
                        A.FuncCall(self._func_name(attr), self._awk_args(call))
                    )
                ]
            # warnings.warn → print to stderr
            if obj == "warnings" and attr == "warn":
                if call.args:
                    return [
                        A.Print(
                            [self._expr(call.args[0])],
                            dest=A.StringLit("/dev/stderr"),
                        )
                    ]
                return []
            # list.append(val)
            if attr == "append":
                return self._append_stmt(call)
        # Python print() → AWK print
        if func == "print":
            args = [self._expr(a) for a in call.args] if call.args else []
            return [A.Print(args)]

        # Plain function call
        return [
            A.ExprStmt(A.FuncCall(self._func_name(func), self._awk_args(call)))
        ]

    def _append_stmt(self, call: Call) -> list[A.Stmt]:
        obj_name = call.func.rpartition(".")[0]
        if not call.args:
            raise ValueError("append call with no args")
        arg = call.args[0]
        # Use array-based list: OBJ[++OBJ_len] = val; OBJ[0] = OBJ_len
        len_var = f"{obj_name}_len"
        self._locals.add(len_var)
        self._array_locals.add(obj_name)
        match arg:
            case Call(call_type=CallType.BOOL):
                val_expr = self._bool_call_as_value(arg)
            case _:
                val_expr = self._expr(arg)
        new_len = A.BinOp(A.Var(len_var), "+", A.NumLit(1))
        return [
            A.Assign(A.ArrayRef(obj_name, new_len), val_expr),
            A.Assign(A.Var(len_var), new_len),
            # Keep arr[0] = count for iteration by _any/_all
            # Use Var(len_var) here since len_var was already incremented above
            A.Assign(A.ArrayRef(obj_name, A.NumLit(0)), A.Var(len_var)),
        ]

    def _pyconf_call_stmt(self, method: str, call: Call) -> list[A.Stmt]:
        match method:
            case "define":
                return [A.ExprStmt(self._build_define_call(call))]
            case "run":
                if call.args:
                    return [
                        A.ExprStmt(
                            A.FuncCall(
                                "pyconf_run", [self._expr(call.args[0])]
                            )
                        )
                    ]
                return []
            case "check_headers" | "check_header":
                return self._check_headers_stmts(method, call)
            case "check_sizeof":
                return [A.ExprStmt(self._build_check_sizeof_call(call))]
            case "check_func":
                return self._check_func_stmts(call)
            case "check_decl":
                return self._check_decl_stmts(call)
            case "check_decls":
                return [A.ExprStmt(self._build_check_decls_call(call))]
            case "check_member":
                member = (
                    self._expr(call.args[0]) if call.args else A.StringLit("")
                )
                headers = self._extract_includes_expr(call)
                return [
                    A.ExprStmt(
                        A.FuncCall("pyconf_check_member", [member, headers])
                    )
                ]
            case "check_members":
                return self._check_members_stmts(call)
            case "check_type":
                return self._check_type_stmts(call)
            case "link_check" | "compile_check" | "compile_link_check":
                return [A.ExprStmt(self._build_link_check_call(method, call))]
            case "run_check":
                return [A.ExprStmt(self._build_run_check_call(call))]
            case "stdlib_module":
                return [A.ExprStmt(self._build_stdlib_module_call(call))]
            case "stdlib_module_simple":
                return [
                    A.ExprStmt(self._build_stdlib_module_simple_call(call))
                ]
            case "search_libs":
                return [A.ExprStmt(self._build_search_libs_call(call))]
            case "find_compiler":
                args: list[A.Expr] = []
                cc = call.kwargs.get("cc")
                cpp = call.kwargs.get("cpp")
                args.append(self._expr(cc) if cc else A.StringLit(""))
                args.append(self._expr(cpp) if cpp else A.StringLit(""))
                return [A.ExprStmt(A.FuncCall("pyconf_find_compiler", args))]
            case "ax_c_float_words_bigendian":
                return self._build_ax_float_bigendian(call)
            case "config_files" if "chmod_x" in call.kwargs:
                return [
                    A.ExprStmt(
                        A.FuncCall(
                            "pyconf_config_files_x", self._awk_args(call)
                        )
                    )
                ]
            case _:
                dropped = [
                    k for k in call.kwargs if k not in self._SKIP_KWARGS
                ]
                if dropped:
                    raise ValueError(
                        f"pyconf.{method}() stmt call has unhandled "
                        f"kwargs {dropped} — add a transpiler handler"
                    )
                return [
                    A.ExprStmt(
                        A.FuncCall(f"pyconf_{method}", self._awk_args(call))
                    )
                ]

    def _option_method_stmt(
        self, opt: OptionInfo, method: str, call: Call
    ) -> list[A.Stmt]:
        match method:
            case "process_bool":
                stmts: list[A.Stmt] = []
                result_var = "_opt_result"
                self._locals.add(result_var)
                stmts.append(
                    A.Assign(
                        A.Var(result_var),
                        A.FuncCall(
                            "pyconf_option_process_bool",
                            [A.StringLit(opt.key)],
                        ),
                    )
                )
                if opt.var and _has_v_arg(call):
                    stmts.append(
                        A.Assign(
                            A.ArrayRef("V", A.StringLit(opt.var)),
                            A.Var(result_var),
                        )
                    )
                return stmts
            case "process_value":
                stmts = []
                result_var = "_opt_result"
                self._locals.add(result_var)
                stmts.append(
                    A.Assign(
                        A.Var(result_var),
                        A.FuncCall(
                            "pyconf_option_process_value",
                            [A.StringLit(opt.key)],
                        ),
                    )
                )
                if opt.var and _has_v_arg(call):
                    stmts.append(
                        A.Assign(
                            A.ArrayRef("V", A.StringLit(opt.var)),
                            A.Var(result_var),
                        )
                    )
                return stmts
            case _:
                return [
                    A.ExprStmt(
                        A.FuncCall(
                            f"pyconf_option_{method}",
                            [A.StringLit(opt.key)] + self._awk_args(call),
                        )
                    )
                ]

    # ------------------------------------------------------------------
    # Assignment lowering
    # ------------------------------------------------------------------

    def _stmt_assign(self, node: PyshAssign) -> list[A.Stmt]:
        target = node.target
        value = node.value

        match target:
            # v.NAME = expr → V["NAME"] = expr
            case Attr(obj="v", attr=attr):
                return self._assign_to_var(attr, value, node)

            # plain name = expr
            case Var(name=var):
                self._locals.add(var)
                return self._var_assign(var, value, node)

            # Tuple unpacking
            case PyshList():
                return self._tuple_assign(target, value)

            # x["key"] = val
            case Subscript():
                return self._subscript_assign(target, value)

            # pyconf.attr = val / obj.attr = val
            case Attr(obj=obj, attr=attr):
                rhs = self._expr(value)
                match obj:
                    case "pyconf":
                        return [A.Assign(A.Var(f"pyconf_{attr}"), rhs)]
                    case "os":
                        return [
                            A.Assign(
                                A.ArrayRef("ENV", A.StringLit(attr)), rhs
                            ),
                            A.Assign(
                                A.ArrayRef("MODIFIED_ENV", A.StringLit(attr)),
                                A.NumLit(1),
                            ),
                        ]
                    case _:
                        return [A.Assign(A.Var(f"{obj}_{attr}"), rhs)]

            case _:
                raise ValueError(
                    f"unsupported assign target: {type(target).__name__}"
                )

    def _var_assign(
        self, var: str, value: PyshExpr, node: PyshAssign
    ) -> list[A.Stmt]:
        match value:
            # pyconf.shell() with exports= — capture exported variables
            case Call(func="pyconf.shell") if "exports" in value.kwargs:
                exports_node = value.kwargs["exports"]
                cmd_expr = (
                    self._expr(value.args[0])
                    if value.args
                    else A.StringLit("")
                )
                stmts: list[A.Stmt] = []
                if isinstance(exports_node, PyshList):
                    for elt in exports_node.elts:
                        if isinstance(elt, Const) and isinstance(
                            elt.value, str
                        ):
                            export_name = elt.value
                            local = f"{var}_{export_name}"
                            self._locals.add(local)
                            stmts.append(
                                A.Assign(
                                    A.Var(local),
                                    A.FuncCall(
                                        "pyconf_shell_export",
                                        [cmd_expr, A.StringLit(export_name)],
                                    ),
                                )
                            )
                return stmts if stmts else [A.Assign(A.Var(var), cmd_expr)]

            # pkg_check_modules
            case Call(func="pyconf.pkg_check_modules"):
                match value.args:
                    case [Const(value=prefix), *_]:
                        self._pkg_var_map[var] = str(prefix)
                    case _:
                        pass
                args = self._awk_args(value)
                return [
                    A.Assign(
                        A.Var(var),
                        A.Ternary(
                            A.FuncCall("pyconf_pkg_check_modules", args),
                            A.StringLit("yes"),
                            A.StringLit("no"),
                        ),
                    )
                ]
            # pyconf.run(cmd, capture_output=True)
            case Call(func="pyconf.run"):
                cmd_arg = (
                    self._expr(value.args[0])
                    if value.args
                    else A.StringLit("")
                )
                input_arg = value.kwargs.get("input")
                if input_arg is not None:
                    func_name = "pyconf_run_capture_input"
                    args = [cmd_arg, self._expr(input_arg)]
                else:
                    func_name = "pyconf_run_capture"
                    args = [cmd_arg]
                rc_var = f"{var}_returncode"
                stdout_var = f"{var}_stdout"
                stderr_var = f"{var}_stderr"
                self._locals.update([rc_var, stdout_var, stderr_var])
                return [
                    A.Assign(A.Var(rc_var), A.FuncCall(func_name, args)),
                    A.Assign(
                        A.Var(stdout_var), A.Var("_pyconf_run_cmd_stdout")
                    ),
                    A.Assign(
                        A.Var(stderr_var), A.Var("_pyconf_run_cmd_stderr")
                    ),
                ]
            # .split() assigned to an array variable → split into array
            case Call(func=f) if (
                f.endswith(".split") and var in self._array_locals
            ):
                obj_name = f.rpartition(".")[0]
                if obj_name == "<complex>" and value.args:
                    obj_expr = self._expr(value.args[0])
                    split_args = value.args[1:]
                else:
                    obj_expr = self._expr_from_name(obj_name)
                    split_args = value.args
                match split_args:
                    case [Const(value=sep), *_]:
                        sep_expr = A.StringLit(str(sep))
                    case _:
                        sep_expr = A.StringLit(" ")
                len_var = f"{var}_len"
                self._locals.add(len_var)
                return [
                    A.Assign(
                        A.Var(len_var),
                        A.Split(obj_expr, var, sep_expr),
                    ),
                    A.Assign(A.ArrayRef(var, A.NumLit(0)), A.Var(len_var)),
                ]
            # List literal assigned to array variable → AWK array init
            case PyshList(elts=elts) if elts and var in self._array_locals:
                stmts: list[A.Stmt] = [A.DeleteArray(var)]
                for idx, elt in enumerate(elts, 1):
                    stmts.append(
                        A.Assign(
                            A.ArrayRef(var, A.NumLit(idx)),
                            self._expr(elt),
                        )
                    )
                stmts.append(
                    A.Assign(
                        A.ArrayRef(var, A.NumLit(0)),
                        A.NumLit(len(elts)),
                    )
                )
                return stmts
            # Dict literal
            case PyshDict():
                self._array_locals.add(var)
                stmts2: list[A.Stmt] = []
                for k, v_expr in zip(value.keys, value.values):
                    safe_key = k.replace("-", "_")
                    stmts2.append(
                        A.Assign(
                            A.ArrayRef(var, A.StringLit(safe_key)),
                            self._expr(v_expr),
                        )
                    )
                return stmts2
            # option retval
            case Call(call_type=CallType.RETVAL):
                opt = self._option_from_call(value)
                if opt is not None:
                    return self._assign_option_retval(var, opt, value)
            case _:
                pass
        return self._assign_to_var(var, value, node)

    def _tuple_assign(self, target: PyshList, value: PyshExpr) -> list[A.Stmt]:
        match value:
            case Call(func="pyconf.cmd_status"):
                args = self._awk_args(value)
                names = self._tuple_target_names(target)
                for n in names:
                    self._locals.add(n)
                result_arr = self._new_array_local("_ar")
                return [
                    A.ExprStmt(
                        A.FuncCall(
                            "pyconf_cmd_status", args + [A.Var(result_arr)]
                        )
                    ),
                    A.Assign(
                        A.Var(names[0]), A.ArrayRef(result_arr, A.NumLit(0))
                    ),
                    A.Assign(
                        A.Var(names[1]), A.ArrayRef(result_arr, A.NumLit(1))
                    ),
                ]
            case Call(func=func) if "." not in func:
                result_arr = self._new_array_local("_ar")
                args = self._awk_args(value)
                stmts: list[A.Stmt] = [
                    A.ExprStmt(
                        A.FuncCall(
                            self._func_name(func),
                            args + [A.Var(result_arr)],
                        )
                    )
                ]
                for i, t in enumerate(target.elts):
                    match t:
                        case Attr(obj="v", attr=dest):
                            stmts.append(
                                A.Assign(
                                    A.ArrayRef("V", A.StringLit(dest)),
                                    A.ArrayRef(result_arr, A.NumLit(i)),
                                )
                            )
                        case Var(name=dest):
                            self._locals.add(dest)
                            stmts.append(
                                A.Assign(
                                    A.Var(self._var_name(dest)),
                                    A.ArrayRef(result_arr, A.NumLit(i)),
                                )
                            )
                        case _:
                            raise ValueError(f"complex tuple target: {t}")
                return stmts
            case PyshList():
                stmts = []
                for t, v in zip(target.elts, value.elts):
                    match t:
                        case Var(name=name):
                            self._locals.add(name)
                            stmts.append(
                                A.Assign(
                                    A.Var(self._var_name(name)), self._expr(v)
                                )
                            )
                        case _:
                            raise ValueError(f"complex tuple target: {t}")
                return stmts
            case _:
                raise ValueError(f"unsupported tuple unpacking: {value}")

    @staticmethod
    def _tuple_target_names(target: PyshList) -> list[str]:
        names: list[str] = []
        for t in target.elts:
            match t:
                case Attr(obj="v", attr=dest):
                    names.append(dest)
                case Var(name=dest):
                    names.append(dest)
                case _:
                    raise ValueError(f"complex tuple target: {t}")
        return names

    def _assign_to_var(
        self, var: str, value: PyshExpr, _node: object
    ) -> list[A.Stmt]:
        """Generate assignment for var = <value_expr>."""

        _is_config_var = (
            isinstance(_node, PyshAssign)
            and isinstance(_node.target, Attr)
            and _node.target.obj == "v"
        )

        def _target() -> A.Expr:
            """Build the correct assignment target."""
            if _is_config_var:
                return A.ArrayRef("V", A.StringLit(var))
            return A.Var(var)

        match value:
            case Ternary():
                return self._ternary_assign(var, value, _target)

            case Not(operand=Call(call_type=CallType.BOOL) as operand):
                # Use the raw function call as condition (returns 0/1),
                # then invert: !call ? "yes" : "no"
                fc = self._call_cond(operand)
                return [
                    A.Assign(
                        _target(),
                        A.Ternary(
                            A.UnaryOp("!", fc),
                            A.StringLit("yes"),
                            A.StringLit("no"),
                        ),
                    )
                ]

            # Option value/bool check
            case Attr() | Call() if (
                opt := self._option_from_expr(value)
            ) is not None:
                match value:
                    case Attr(attr=attr):
                        pass
                    case Call(func=func):
                        attr = func.rpartition(".")[2]
                    case _:
                        attr = ""
                if attr in OPTION_VALUE_PROPS or attr in OPTION_VALUE_METHODS:
                    return self._assign_option_value(var, opt, value, _target)
                if attr in OPTION_BOOL_PROPS or attr in OPTION_BOOL_METHODS:
                    return self._assign_option_bool(var, opt, value, _target)
                return self._assign_to_var_general(var, value, _target)

            # Boolean-returning method (startswith/endswith) assigned to
            # config variable — emit ternary to produce "yes"/"no".
            case Call(func=func) if _is_config_var and func.endswith(
                (".startswith", ".endswith")
            ):
                cond = self._cond_expr(value)
                return [
                    A.Assign(
                        _target(),
                        A.Ternary(
                            cond,
                            A.StringLit("yes"),
                            A.StringLit("no"),
                        ),
                    )
                ]

            case _:
                return self._assign_to_var_general(var, value, _target)

    def _assign_to_var_general(
        self, var: str, value: PyshExpr, target_fn: object
    ) -> list[A.Stmt]:
        target = target_fn()
        match value:
            # Bool call: x = pyconf.check_header(...)
            case Call(call_type=CallType.BOOL):
                result = self._bool_call_as_value(value)
                return [A.Assign(target, result)]

            # STDOUT call: x = pyconf.find_prog(...)
            case Call(call_type=CallType.STDOUT):
                return [A.Assign(target, self._pyconf_func_call(value))]

            # RETVAL call: x = pyconf.check_sizeof(...)
            case Call(call_type=CallType.RETVAL):
                return [A.Assign(target, self._pyconf_func_call(value))]

            # List literal
            case PyshList(elts=elts) if elts:
                parts = [self._expr(elt) for elt in elts]
                val: A.Expr = self._concat_with_sep(parts, " ")
                return [A.Assign(target, val)]

            # Empty list: AWK arrays start empty, skip initialization.
            # The _len counter is also a local and defaults to 0.
            case PyshList(elts=[]):
                return []

            case _:
                return [A.Assign(target, self._expr(value))]

    def _bool_call_as_value(self, call: Call) -> A.Expr:
        """Lower a BOOL call to an AWK expression producing "yes"/"no"."""
        success_val = "yes"
        failure_val = "no"
        for k, v in call.kwargs.items():
            match k:
                case "on_success_return" | "success":
                    match v:
                        case Const(value=val):
                            success_val = "yes" if val else "no"
                case "on_failure_return" | "failure":
                    match v:
                        case Const(value=val):
                            failure_val = "yes" if val else "no"
        fc = self._pyconf_func_call(call)
        return A.Ternary(
            fc, A.StringLit(success_val), A.StringLit(failure_val)
        )

    def _assign_option_retval(
        self, var: str, opt: OptionInfo, call: Call
    ) -> list[A.Stmt]:
        method = call.func.rpartition(".")[2]
        result_var = "_opt_result"
        self._locals.add(result_var)
        stmts: list[A.Stmt] = [
            A.Assign(
                A.Var(result_var),
                A.FuncCall(f"pyconf_option_{method}", [A.StringLit(opt.key)]),
            )
        ]
        if opt.var and _has_v_arg(call):
            stmts.append(
                A.Assign(
                    A.ArrayRef("V", A.StringLit(opt.var)),
                    A.Var(result_var),
                )
            )
        self._locals.add(var)
        stmts.append(A.Assign(A.Var(var), A.Var(result_var)))
        return stmts

    def _assign_option_value(
        self, var: str, opt: OptionInfo, value: PyshExpr, target_fn: object
    ) -> list[A.Stmt]:
        target = target_fn()
        match value:
            case Attr():
                return [
                    A.Assign(
                        target,
                        A.FuncCall(
                            "pyconf_option_value", [A.StringLit(opt.key)]
                        ),
                    )
                ]
            case Call(func=func):
                method = func.rpartition(".")[2]
                args = [A.StringLit(opt.key)] + self._awk_args(value)
                return [
                    A.Assign(
                        target,
                        A.FuncCall(f"pyconf_option_{method}", args),
                    )
                ]
            case _:
                raise ValueError(f"unsupported option value assign: {value}")

    def _assign_option_bool(
        self, var: str, opt: OptionInfo, value: PyshExpr, target_fn: object
    ) -> list[A.Stmt]:
        target = target_fn()
        match value:
            case Attr(attr="given"):
                fc = A.FuncCall("pyconf_option_given", [A.StringLit(opt.key)])
            case Call(func=func):
                fc = A.FuncCall(
                    f"pyconf_option_{func.rpartition('.')[2]}",
                    [A.StringLit(opt.key)],
                )
            case _:
                raise ValueError(f"unsupported option bool assign: {value}")
        return [
            A.Assign(
                target,
                A.Ternary(fc, A.StringLit("yes"), A.StringLit("no")),
            )
        ]

    def _ternary_assign(
        self, var: str, node: Ternary, target_fn: object
    ) -> list[A.Stmt]:
        cond = self._cond_expr(node.test)
        body_val = self._expr(node.body)
        else_val = self._expr(node.orelse)
        return [A.Assign(target_fn(), A.Ternary(cond, body_val, else_val))]

    def _subscript_assign(
        self, target: Subscript, value: PyshExpr
    ) -> list[A.Stmt]:
        rhs = self._expr(value)
        obj = target.obj
        key = target.key

        match obj:
            # os.environ["KEY"] / pyconf.env["KEY"]
            case Attr(obj="os" | "pyconf", attr="environ" | "env"):
                key_expr = self._expr(key)
                return [
                    A.Assign(A.ArrayRef("ENV", key_expr), rhs),
                    A.Assign(
                        A.ArrayRef("MODIFIED_ENV", key_expr), A.NumLit(1)
                    ),
                ]
            # pyconf.cache["key"]
            case Attr(obj="pyconf", attr="cache"):
                key_expr = self._expr(key)
                return [A.Assign(A.ArrayRef("CACHE", key_expr), rhs)]
            # x["key"] = val → use x as associative array
            case Var(name=dict_name):
                self._array_locals.add(dict_name)
                key_expr = self._expr(key)
                return [A.Assign(A.ArrayRef(dict_name, key_expr), rhs)]
            case _:
                raise ValueError(f"unsupported subscript assignment: {target}")

    def _stmt_aug_assign(self, node: AugAssign) -> list[A.Stmt]:
        if node.op != "Add":
            raise ValueError(f"unsupported aug-assign op: {node.op}")
        match node.target:
            case Attr(obj="v", attr=var):
                target: A.Expr = A.ArrayRef("V", A.StringLit(var))
                current: A.Expr = A.ArrayRef("V", A.StringLit(var))
            case Var(name=var):
                target = A.Var(var)
                current = A.Var(var)
            case _:
                raise ValueError(
                    f"unsupported aug-assign target: {node.target}"
                )
        rhs = self._expr(node.value)
        return [A.Assign(target, A.Concat([current, rhs]))]

    # ------------------------------------------------------------------
    # If / For / With / Return
    # ------------------------------------------------------------------

    def _stmt_if(self, node: PyshIf) -> list[A.Stmt]:
        result: list[A.Stmt] = []
        self._pre_stmts.clear()
        cond = self._cond_expr(node.test)
        result.extend(self._pre_stmts)
        self._pre_stmts.clear()
        body = self._lower_block(node.body)
        branches = [A.IfBranch(cond=cond, body=body)]
        else_body: A.Block | None = None

        orelse = node.orelse
        while orelse and orelse.stmts:
            if len(orelse.stmts) == 1 and isinstance(orelse.stmts[0], PyshIf):
                elif_node = orelse.stmts[0]
                self._pre_stmts.clear()
                elif_cond = self._cond_expr(elif_node.test)
                # pre_stmts from elif conditions go into a wrapper block
                if self._pre_stmts:
                    # Close the current if chain, emit pre-stmts, start new if
                    else_body_stmts: list[A.Stmt] = list(self._pre_stmts)
                    self._pre_stmts.clear()
                    elif_body = self._lower_block(elif_node.body)
                    elif_branches = [
                        A.IfBranch(cond=elif_cond, body=elif_body)
                    ]
                    # Continue processing remaining elif/else on the inner if
                    inner_orelse = elif_node.orelse
                    while inner_orelse and inner_orelse.stmts:
                        if len(inner_orelse.stmts) == 1 and isinstance(
                            inner_orelse.stmts[0], PyshIf
                        ):
                            inner_elif = inner_orelse.stmts[0]
                            inner_cond = self._cond_expr(inner_elif.test)
                            inner_body = self._lower_block(inner_elif.body)
                            elif_branches.append(
                                A.IfBranch(cond=inner_cond, body=inner_body)
                            )
                            inner_orelse = inner_elif.orelse
                        else:
                            inner_else = self._lower_block(inner_orelse)
                            else_body_stmts.append(
                                A.If(
                                    branches=elif_branches,
                                    else_body=inner_else,
                                )
                            )
                            inner_orelse = None
                            break
                    else:
                        else_body_stmts.append(
                            A.If(branches=elif_branches, else_body=None)
                        )
                    else_body = A.Block(else_body_stmts)
                    orelse = None
                else:
                    elif_body = self._lower_block(elif_node.body)
                    branches.append(A.IfBranch(cond=elif_cond, body=elif_body))
                    orelse = elif_node.orelse
            else:
                else_body = self._lower_block(orelse)
                orelse = None

        result.append(A.If(branches=branches, else_body=else_body))
        return result

    def _stmt_for(self, node: PyshFor) -> list[A.Stmt]:
        var = node.var
        self._locals.add(var)
        body = self._lower_block(node.body)

        match node.iter:
            # .splitlines() → split on newlines
            case Call(func=f, args=[], kwargs={}) if f.endswith(".splitlines"):
                obj_name = f.rpartition(".")[0]
                items = self._expr_from_name(obj_name)
                arr_name = self._new_array_local("_as")
                n_var = f"_n_{var}"
                i_var = f"_i_{var}"
                self._locals.update([n_var, i_var])
                return [
                    A.Assign(
                        A.Var(n_var),
                        A.Split(items, arr_name, A.StringLit("\n")),
                    ),
                    A.For(
                        init=A.Assign(A.Var(i_var), A.NumLit(1)),
                        cond=A.BinOp(A.Var(i_var), "<=", A.Var(n_var)),
                        incr=A.Assign(
                            A.Var(i_var),
                            A.BinOp(A.Var(i_var), "+", A.NumLit(1)),
                        ),
                        body=A.Block(
                            [
                                A.Assign(
                                    A.Var(var),
                                    A.ArrayRef(arr_name, A.Var(i_var)),
                                ),
                                *body.stmts,
                            ]
                        ),
                    ),
                ]

            # List literal
            case PyshList(elts=elts):
                arr_name = self._new_array_local("_al")
                i_var = f"_i_{var}"
                self._locals.update([i_var])
                stmts: list[A.Stmt] = []
                for idx, elt in enumerate(elts):
                    stmts.append(
                        A.Assign(
                            A.ArrayRef(arr_name, A.NumLit(idx + 1)),
                            self._expr(elt),
                        )
                    )
                stmts.append(
                    A.For(
                        init=A.Assign(A.Var(i_var), A.NumLit(1)),
                        cond=A.BinOp(A.Var(i_var), "<=", A.NumLit(len(elts))),
                        incr=A.Assign(
                            A.Var(i_var),
                            A.BinOp(A.Var(i_var), "+", A.NumLit(1)),
                        ),
                        body=A.Block(
                            [
                                A.Assign(
                                    A.Var(var),
                                    A.ArrayRef(arr_name, A.Var(i_var)),
                                ),
                                *body.stmts,
                            ]
                        ),
                    )
                )
                return stmts

            # Iterate over an array local (e.g. list built via .append())
            case Var(name=arr_var) if arr_var in self._array_locals:
                len_var = f"{arr_var}_len"
                i_var = f"_i_{var}"
                self._locals.update([i_var, len_var])
                return [
                    A.For(
                        init=A.Assign(A.Var(i_var), A.NumLit(1)),
                        cond=A.BinOp(A.Var(i_var), "<=", A.Var(len_var)),
                        incr=A.Assign(
                            A.Var(i_var),
                            A.BinOp(A.Var(i_var), "+", A.NumLit(1)),
                        ),
                        body=A.Block(
                            [
                                A.Assign(
                                    A.Var(var),
                                    A.ArrayRef(arr_var, A.Var(i_var)),
                                ),
                                *body.stmts,
                            ]
                        ),
                    ),
                ]

            case _:
                # Generic: split the expression on spaces
                items = self._expr(node.iter)
                arr_name = self._new_array_local("_as")
                n_var = f"_n_{var}"
                i_var = f"_i_{var}"
                self._locals.update([n_var, i_var])
                return [
                    A.Assign(
                        A.Var(n_var),
                        A.Split(items, arr_name, A.StringLit(" ")),
                    ),
                    A.For(
                        init=A.Assign(A.Var(i_var), A.NumLit(1)),
                        cond=A.BinOp(A.Var(i_var), "<=", A.Var(n_var)),
                        incr=A.Assign(
                            A.Var(i_var),
                            A.BinOp(A.Var(i_var), "+", A.NumLit(1)),
                        ),
                        body=A.Block(
                            [
                                A.Assign(
                                    A.Var(var),
                                    A.ArrayRef(arr_name, A.Var(i_var)),
                                ),
                                *body.stmts,
                            ]
                        ),
                    ),
                ]

    def _stmt_with(self, node: PyshWith) -> list[A.Stmt]:
        match node.ctx:
            case Call(func="pyconf.save_env"):
                body = self._lower_block(node.body)
                return [
                    A.ExprStmt(A.FuncCall("pyconf_save_env", [])),
                    *body.stmts,
                    A.ExprStmt(A.FuncCall("pyconf_restore_env", [])),
                ]
            case _:
                raise ValueError(f"unsupported 'with' context: {node.ctx}")

    def _stmt_return(self, node: PyshReturn) -> list[A.Stmt]:
        match node.value:
            case None:
                return [A.Return()]
            case PyshList(elts=elts):
                # Multi-value return: store in _result array param
                self._has_multi_return = True
                stmts: list[A.Stmt] = []
                for i, elt in enumerate(elts):
                    stmts.append(
                        A.Assign(
                            A.ArrayRef("_result", A.NumLit(i)),
                            self._expr(elt),
                        )
                    )
                stmts.append(A.Return())
                return stmts
            case _:
                val = self._expr(node.value)
                return [A.Return(val)]

    # ------------------------------------------------------------------
    # Expression lowering: pysh_ast.Expr → awk_ast.Expr
    # ------------------------------------------------------------------

    def _expr(self, node: PyshExpr) -> A.Expr:
        match node:
            case Const(value=None):
                return A.StringLit("")
            case Const(value=True):
                return A.StringLit("yes")
            case Const(value=False):
                return A.StringLit("no")
            case Const(value=int() as v):
                return A.NumLit(v)
            case Const(value=str() as v):
                return A.StringLit(v)
            case Const(value=v):
                return A.StringLit(str(v))
            case Var(name=name):
                self._locals.add(name)
                return A.Var(self._var_name(name))
            case Attr():
                return self._attr_expr(node)
            case FStr(parts=parts):
                return self._fstr_expr(parts)
            case Add(left=left, right=right):
                return A.Concat([self._expr(left), self._expr(right)])
            case BoolOp(op="or", values=values) if len(values) >= 2:
                # x or y → x != "" ? x : y
                result = self._expr(values[-1])
                for val in reversed(values[:-1]):
                    left_expr = self._expr(val)
                    result = A.Ternary(
                        A.BinOp(left_expr, "!=", A.StringLit("")),
                        left_expr,
                        result,
                    )
                return result
            case BoolOp(op=op, values=values):
                cond = self._boolop_cond(op, values)
                return A.Ternary(cond, A.StringLit("yes"), A.StringLit("no"))
            case Not(operand=operand):
                cond = self._cond_expr(operand)
                return A.Ternary(cond, A.StringLit("no"), A.StringLit("yes"))
            case Call() as call:
                return self._call_as_expr(call)
            case Subscript():
                return self._subscript_expr(node)
            case Cmp() as cmp_node:
                cond = self._cond_expr(cmp_node)
                return A.Ternary(cond, A.StringLit("yes"), A.StringLit("no"))
            case Ternary(test=test, body=body, orelse=orelse):
                cond = self._cond_expr(test)
                return A.Ternary(cond, self._expr(body), self._expr(orelse))
            case PyshList(elts=elts):
                parts = [self._expr(elt) for elt in elts]
                return (
                    self._concat_with_sep(parts, " ")
                    if parts
                    else A.StringLit("")
                )
            case In():
                cond = self._cond_expr(node)
                return A.Ternary(cond, A.StringLit("yes"), A.StringLit("no"))
            case _:
                return A.StringLit("")

    def _attr_expr(self, node: Attr) -> A.Expr:
        obj = node.obj
        attr = node.attr

        # v.NAME → V["NAME"]
        if obj == "v":
            return A.ArrayRef("V", A.StringLit(attr))

        # Option variable access
        opt = self.option_vars.get(obj)
        if opt is not None:
            if attr == "value":
                return A.FuncCall(
                    "pyconf_option_value", [A.StringLit(opt.key)]
                )
            if attr == "given":
                return A.ArrayRef("OPT_GIVEN", A.StringLit(opt.key))

        # pkg_check_modules result
        if obj in self._pkg_var_map:
            prefix = self._pkg_var_map[obj]
            if attr == "cflags":
                return A.ArrayRef("V", A.StringLit(f"{prefix}_CFLAGS"))
            if attr == "libs":
                return A.ArrayRef("V", A.StringLit(f"{prefix}_LIBS"))

        return A.Var(f"{obj}_{attr}")

    def _fstr_expr(self, parts: list) -> A.Expr:
        awk_parts: list[A.Expr] = []
        for p in parts:
            match p:
                case str():
                    awk_parts.append(A.StringLit(p))
                case _:
                    awk_parts.append(self._expr(p))
        if len(awk_parts) == 1:
            return awk_parts[0]
        return A.Concat(awk_parts)

    def _call_as_expr(self, call: Call) -> A.Expr:
        func = call.func

        # Option value methods
        opt = self._option_from_call(call)
        if opt is not None:
            attr = func.rpartition(".")[2]
            if attr in OPTION_VALUE_METHODS:
                args = [A.StringLit(opt.key)] + self._awk_args(call)
                return A.FuncCall(f"pyconf_option_{attr}", args)
            if attr in OPTION_VALUE_PROPS:
                return A.FuncCall(
                    "pyconf_option_value", [A.StringLit(opt.key)]
                )
            if attr in OPTION_BOOL_METHODS or attr in OPTION_BOOL_PROPS:
                return A.Ternary(
                    A.FuncCall(
                        f"pyconf_option_{attr}", [A.StringLit(opt.key)]
                    ),
                    A.StringLit("yes"),
                    A.StringLit("no"),
                )

        # STDOUT / RETVAL: direct function call (AWK has return values)
        if call.call_type in (CallType.STDOUT, CallType.RETVAL):
            return self._pyconf_func_call(call)

        # BOOL as expression
        if call.call_type == CallType.BOOL:
            return self._bool_call_as_value(call)

        if "." not in func:
            fid = func
            if fid in ("str", "int") and call.args:
                return self._expr(call.args[0])
            if fid == "sorted" and call.args:
                return self._expr(call.args[0])
            if fid == "bool" and call.args:
                inner = self._expr(call.args[0])
                return A.Ternary(
                    A.BinOp(inner, "!=", A.StringLit("")),
                    A.StringLit("yes"),
                    A.StringLit("no"),
                )
            if fid == "len" and call.args:
                inner = self._expr(call.args[0])
                return A.Length(inner)
            if self._known_funcs and fid not in self._known_funcs:
                raise ValueError(
                    f"plain function {fid}() used as expression but not "
                    f"handled by the transpiler: {call}"
                )
            args = self._awk_args(call)
            return A.FuncCall(self._func_name(fid), args)

        # String method calls
        obj_str, _, method = func.rpartition(".")
        if obj_str == "<complex>" and call.args:
            obj_expr = self._expr(call.args[0])
            sub_call = Call(
                func=func,
                args=call.args[1:],
                kwargs=call.kwargs,
                call_type=call.call_type,
            )
            return self._string_method_expr("", obj_expr, method, sub_call)
        obj_expr = self._expr_from_name(obj_str)
        return self._string_method_expr(obj_str, obj_expr, method, call)

    def _string_method_expr(
        self, obj_str: str, obj: A.Expr, method: str, call: Call
    ) -> A.Expr:
        match method:
            case "split":
                # Return a space-separated string (AWK split is done at use site)
                match call.args:
                    case [Const(value=sep), *_] if str(sep) != " ":
                        tmp = "_tmp_split"
                        self._locals.add(tmp)
                        # gsub needs in-place: copy, gsub, return
                        return A.FuncCall(
                            "_str_replace",
                            [obj, A.StringLit(str(sep)), A.StringLit(" ")],
                        )
                return obj

            case "strip":
                return A.FuncCall("_str_strip", [obj])

            case "lower":
                return A.Tolower(obj)

            case "upper":
                return A.Toupper(obj)

            case "splitlines":
                return obj

            case "removeprefix" if call.args:
                match call.args[0]:
                    case Const(value=v):
                        prefix = str(v)
                        return A.FuncCall(
                            "_str_removeprefix", [obj, A.StringLit(prefix)]
                        )

            case "removesuffix" | "rstrip" if call.args:
                match call.args[0]:
                    case Const(value=v):
                        suffix = str(v)
                        return A.FuncCall(
                            "_str_removesuffix", [obj, A.StringLit(suffix)]
                        )

            case "rstrip" if not call.args:
                return A.FuncCall("_str_rstrip", [obj])

            case "replace" if len(call.args) >= 2:
                old_arg = call.args[0]
                new_arg = call.args[1]
                old_expr = (
                    self._expr(old_arg)
                    if isinstance(old_arg, Const)
                    else A.StringLit("")
                )
                new_expr = (
                    self._expr(new_arg)
                    if isinstance(new_arg, Const)
                    else A.StringLit("")
                )
                return A.FuncCall("_str_replace", [obj, old_expr, new_expr])

            case "join" if call.args:
                items = self._expr(call.args[0])
                match obj:
                    case A.StringLit(value=sep):
                        pass
                    case _ if obj_str:
                        sep = obj_str
                    case _:
                        sep = ""
                # Use _arr_join for variables that are AWK arrays
                # (built via .append()), _str_join for strings.
                match call.args[0]:
                    case Var(name=vname) if vname in self._array_locals:
                        return A.FuncCall(
                            "_arr_join", [items, A.StringLit(sep)]
                        )
                return A.FuncCall("_str_join", [A.StringLit(sep), items])

            case "get" if call.args:
                return self._dict_get_expr(obj_str, obj, call)

            case "startswith" if call.args:
                match call.args[0]:
                    case Const(value=v):
                        prefix = str(v)
                        return A.FuncCall(
                            "_str_startswith", [obj, A.StringLit(prefix)]
                        )

            case "endswith" if call.args:
                match call.args[0]:
                    case Const(value=v):
                        suffix = str(v)
                        return A.FuncCall(
                            "_str_endswith", [obj, A.StringLit(suffix)]
                        )

            case _:
                pass
        return A.StringLit("")

    def _dict_get_expr(self, obj_str: str, obj: A.Expr, call: Call) -> A.Expr:
        key_expr = self._expr(call.args[0]) if call.args else A.StringLit("")
        default_expr = (
            self._expr(call.args[1]) if len(call.args) > 1 else A.StringLit("")
        )
        match obj_str:
            case "os.environ" | "os_environ":
                return A.Ternary(
                    A.InArray(key_expr, "ENV"),
                    A.ArrayRef("ENV", key_expr),
                    default_expr,
                )
            case "pyconf.cache" | "pyconf_cache":
                return A.Ternary(
                    A.InArray(key_expr, "CACHE"),
                    A.ArrayRef("CACHE", key_expr),
                    default_expr,
                )
            case _:
                pass
        match obj:
            case A.Var(name=name):
                return A.Ternary(
                    A.InArray(key_expr, name),
                    A.ArrayRef(name, key_expr),
                    default_expr,
                )
            case _:
                return default_expr

    def _subscript_expr(self, node: Subscript) -> A.Expr:
        obj = node.obj
        key = node.key

        # x.split("sep")[N]
        match obj, key:
            case (
                Call(func=f),
                Const(value=int() as idx),
            ) if f.endswith(".split"):
                parent_name = f.rpartition(".")[0]
                if parent_name == "<complex>" and obj.args:
                    parent = self._expr(obj.args[0])
                    split_args = obj.args[1:]
                else:
                    parent = self._expr_from_name(parent_name)
                    split_args = obj.args
                arr_name = "_split_tmp"
                self._array_locals.add(arr_name)
                match split_args:
                    case [Const(value=sep), *_]:
                        return A.FuncCall(
                            "_split_index",
                            [parent, A.StringLit(str(sep)), A.NumLit(idx + 1)],
                        )
                    case _:
                        return A.FuncCall(
                            "_split_index",
                            [parent, A.StringLit(" "), A.NumLit(idx + 1)],
                        )

            # Constant int key (index into space-separated list)
            case _, Const(value=int() as iv):
                obj_expr = self._expr(obj)
                arr_name = "_idx_tmp"
                self._array_locals.add(arr_name)
                return A.FuncCall(
                    "_split_index",
                    [obj_expr, A.StringLit(" "), A.NumLit(iv + 1)],
                )

            # Constant string key → array access
            case _, Const(value=kv):
                obj_expr = self._expr(obj)
                match obj_expr:
                    case A.Var(name=name):
                        return A.ArrayRef(name, A.StringLit(str(kv)))
                    case A.ArrayRef():
                        return A.StringLit("")
                    case _:
                        return A.StringLit("")

            # Variable key
            case _, Var(name=key_name):
                obj_expr = self._expr(obj)
                self._locals.add(key_name)
                match obj_expr:
                    case A.Var(name=name):
                        return A.ArrayRef(name, A.Var(key_name))
                    case _:
                        return A.StringLit("")

            case _, _:
                return A.StringLit("")

    def _expr_from_name(self, name: str) -> A.Expr:
        """Recover the object expression from a dotted name."""
        if "." not in name:
            self._locals.add(name)
            return A.Var(name)
        if name.startswith("v.") and "." not in name[2:]:
            return A.ArrayRef("V", A.StringLit(name[2:]))
        return A.Var(name.replace(".", "_"))

    # ------------------------------------------------------------------
    # Condition expression lowering
    # ------------------------------------------------------------------

    def _cond_expr(self, node: PyshExpr) -> A.Expr:
        """Lower a pysh_ast expression to an AWK condition expression (0/1)."""
        match node:
            case BoolOp(op=op, values=values):
                return self._boolop_cond(op, values)
            case Not(operand=operand):
                return A.UnaryOp("!", self._cond_expr(operand))
            case Cmp():
                return self._compare_cond(node)
            case Call() as call:
                return self._call_cond(call)
            case Var(name=name):
                self._locals.add(name)
                vn = self._var_name(name)
                # Truthy: non-empty and != "no"
                return A.BinOp(
                    A.BinOp(A.Var(vn), "!=", A.StringLit("")),
                    "&&",
                    A.BinOp(A.Var(vn), "!=", A.StringLit("no")),
                )
            case Attr() as attr_node:
                return self._attr_cond(attr_node)
            case Const(value=True):
                return A.NumLit(1)
            case Const(value=False | None):
                return A.NumLit(0)
            case Const(value=v):
                return A.BinOp(A.StringLit(str(v)), "!=", A.StringLit(""))
            case In() as in_node:
                return self._in_cond(in_node)
            case _:
                return A.NumLit(1)  # fallback

    def _boolop_cond(self, op: str, values: list) -> A.Expr:
        exprs = [self._cond_expr(val) for val in values]
        awk_op = "&&" if op == "and" else "||"
        result = exprs[0]
        for e in exprs[1:]:
            result = A.BinOp(result, awk_op, e)
        return result

    def _compare_cond(self, node: Cmp) -> A.Expr:
        left = node.left
        op = node.op
        right = node.right

        match right:
            case Const(value=None):
                if op == "is":
                    return A.BinOp(self._expr(left), "==", A.StringLit(""))
                return A.BinOp(self._expr(left), "!=", A.StringLit(""))
            case Const(value=True):
                awk_op = "==" if op == "is" else "!="
                return A.BinOp(self._expr(left), awk_op, A.StringLit("yes"))
            case Const(value=False):
                awk_op = "==" if op == "is" else "!="
                return A.BinOp(self._expr(left), awk_op, A.StringLit("no"))
            case _:
                pass

        match op:
            case "==" | "!=":
                return A.BinOp(self._expr(left), op, self._expr(right))
            case "<" | ">" | "<=" | ">=":
                return A.BinOp(self._expr(left), op, self._expr(right))
            case _:
                return A.NumLit(1)

    def _in_cond(self, node: In) -> A.Expr:
        container = node.container
        item = node.item

        match container, item:
            # x in ("a", "b", "c") — membership test
            case PyshList(elts=elts), _:
                val = self._expr(item)
                # Build OR chain: val == "a" || val == "b" || ...
                parts = []
                for elt in elts:
                    if isinstance(elt, Const):
                        cval = "" if elt.value is None else str(elt.value)
                        parts.append(A.BinOp(val, "==", A.StringLit(cval)))
                if not parts:
                    result: A.Expr = A.NumLit(0)
                elif len(parts) == 1:
                    result = parts[0]
                else:
                    result = parts[0]
                    for p in parts[1:]:
                        result = A.BinOp(result, "||", p)
                if node.negated:
                    result = A.UnaryOp("!", result)
                return result

            # "sub" in string — containment
            case _, Const(value=str() as sub):
                val = self._expr(container)
                result = A.BinOp(
                    A.Index(val, A.StringLit(sub)),
                    ">",
                    A.NumLit(0),
                )
                if node.negated:
                    result = A.UnaryOp("!", result)
                return result

            # key in dict_var — AWK array membership
            case Var(name=name), _ if name in self._array_locals:
                result = A.InArray(self._expr(item), name)
                if node.negated:
                    result = A.UnaryOp("!", result)
                return result

            case _, _:
                val = self._expr(container)
                return A.BinOp(
                    A.Index(val, self._expr(item)),
                    ">",
                    A.NumLit(0),
                )

    def _call_cond(self, call: Call) -> A.Expr:
        func = call.func

        # pyconf.method() as condition
        if _is_pyconf_attr(func):
            fc = self._pyconf_func_call(call)
            if call.call_type == CallType.STDOUT:
                return A.BinOp(fc, "!=", A.StringLit(""))
            # Check for on_success_return=False
            negate = any(
                k in ("on_success_return", "success")
                and isinstance(v, Const)
                and not v.value
                for k, v in call.kwargs.items()
            )
            if negate:
                return A.UnaryOp("!", fc)
            return fc

        # OPTION.method() as boolean
        opt = self._option_from_call(call)
        if opt is not None:
            attr = func.rpartition(".")[2]
            if attr in OPTION_BOOL_METHODS:
                return A.FuncCall(
                    f"pyconf_option_{attr}", [A.StringLit(opt.key)]
                )
            if attr in ("process_bool", "process_value"):
                # process_bool/process_value returns a value; in boolean
                # context test truthy (non-empty and != "no").
                # The assignment is emitted by _stmt_if via _pre_stmts.
                result_var = "_opt_result"
                self._locals.add(result_var)
                fc = A.FuncCall(
                    f"pyconf_option_{attr}", [A.StringLit(opt.key)]
                )
                self._pre_stmts.append(A.Assign(A.Var(result_var), fc))
                if opt.var and _has_v_arg(call):
                    self._pre_stmts.append(
                        A.Assign(
                            A.ArrayRef("V", A.StringLit(opt.var)),
                            A.Var(result_var),
                        )
                    )
                return A.BinOp(
                    A.BinOp(A.Var(result_var), "!=", A.StringLit("")),
                    "&&",
                    A.BinOp(A.Var(result_var), "!=", A.StringLit("no")),
                )

        # v.is_set("name")
        match call:
            case Call(func="v.is_set", args=[Const(value=var_name), *_]):
                return A.InArray(A.StringLit(str(var_name)), "V")
            case _:
                pass

        # .startswith / .endswith
        if "." in func:
            attr = func.rpartition(".")[2]
            obj_str = func.rpartition(".")[0]
            if obj_str == "<complex>" and call.args:
                obj_e = self._expr(call.args[0])
                method_args = call.args[1:]
            else:
                obj_e = self._expr_from_name(obj_str)
                method_args = call.args
            match attr:
                case "startswith" if method_args:
                    arg = method_args[0]
                    match arg:
                        case Const(value=v):
                            return A.FuncCall(
                                "_str_startswith", [obj_e, A.StringLit(str(v))]
                            )
                        case PyshList(elts=elts):
                            parts = []
                            for elt in elts:
                                if isinstance(elt, Const):
                                    parts.append(
                                        A.FuncCall(
                                            "_str_startswith",
                                            [
                                                obj_e,
                                                A.StringLit(str(elt.value)),
                                            ],
                                        )
                                    )
                            if not parts:
                                return A.NumLit(0)
                            result = parts[0]
                            for p in parts[1:]:
                                result = A.BinOp(result, "||", p)
                            return result
                case "endswith" if method_args:
                    arg = method_args[0]
                    match arg:
                        case Const(value=v):
                            return A.FuncCall(
                                "_str_endswith", [obj_e, A.StringLit(str(v))]
                            )
                        case PyshList(elts=elts):
                            parts = []
                            for elt in elts:
                                if isinstance(elt, Const):
                                    parts.append(
                                        A.FuncCall(
                                            "_str_endswith",
                                            [
                                                obj_e,
                                                A.StringLit(str(elt.value)),
                                            ],
                                        )
                                    )
                            if not parts:
                                return A.NumLit(0)
                            result = parts[0]
                            for p in parts[1:]:
                                result = A.BinOp(result, "||", p)
                            return result

        # isinstance → always true
        if func == "isinstance":
            return A.NumLit(1)

        # any(items) / all(items)
        if func == "any" and call.args:
            return A.FuncCall("_any", [self._expr(call.args[0])])
        if func == "all" and call.args:
            return A.FuncCall("_all", [self._expr(call.args[0])])

        # .get() on dicts/environ/cache — use _call_as_expr for truthiness
        if "." in func and func.rpartition(".")[2] == "get":
            val = self._call_as_expr(call)
            return A.BinOp(val, "!=", A.StringLit(""))

        # Generic: use return value as boolean
        fc = self._pyconf_func_call(call)
        return fc

    def _attr_cond(self, node: Attr) -> A.Expr:
        if node.obj == "v":
            v = A.ArrayRef("V", A.StringLit(node.attr))
            return A.BinOp(
                A.BinOp(v, "!=", A.StringLit("")),
                "&&",
                A.BinOp(v, "!=", A.StringLit("no")),
            )
        opt = self.option_vars.get(node.obj)
        if opt is not None:
            if node.attr == "given":
                return A.FuncCall(
                    "pyconf_option_given", [A.StringLit(opt.key)]
                )
            if node.attr == "value":
                return A.BinOp(
                    A.FuncCall("pyconf_option_value", [A.StringLit(opt.key)]),
                    "!=",
                    A.StringLit(""),
                )
        expr = self._attr_expr(node)
        return A.BinOp(
            A.BinOp(expr, "!=", A.StringLit("")),
            "&&",
            A.BinOp(expr, "!=", A.StringLit("no")),
        )

    # ------------------------------------------------------------------
    # Command/call builders
    # ------------------------------------------------------------------

    def _pyconf_func_call(self, call: Call) -> A.FuncCall:
        """Build an AWK function call for a pyconf.method() or user call."""
        func = call.func
        if _is_pyconf_attr(func):
            method = func.removeprefix("pyconf.")
            if method == "config_files" and "chmod_x" in call.kwargs:
                method = "config_files_x"
            match method:
                case "link_check" | "compile_check" | "compile_link_check":
                    return self._build_link_check_call(method, call)
                case "run_check":
                    return self._build_run_check_call(call)
                case "check_func":
                    return self._build_check_func_call(call)
                case "search_libs":
                    return self._build_search_libs_call(call)
                case "define":
                    return self._build_define_call(call)
                case "check_sizeof":
                    return self._build_check_sizeof_call(call)
                case "check_prog":
                    return self._build_check_prog_call(call)
                case "check_decl":
                    return self._build_check_decl_expr(call)
                case "check_member":
                    return self._build_check_member_expr(call)
                case "check_type":
                    return self._build_check_type_expr(call)
                case "check_lib":
                    return self._build_check_lib_expr(call)
                case "check_header":
                    return self._build_check_header_expr(call)
                case "try_link":
                    return self._build_try_link_expr(call)
                case "check_linker_flag":
                    return self._build_check_linker_flag_expr(call)
                case "check_compile_flag":
                    return self._build_check_compile_flag_expr(call)
                case _:
                    dropped = [
                        k for k in call.kwargs if k not in self._SKIP_KWARGS
                    ]
                    if dropped:
                        raise ValueError(
                            f"pyconf.{method}() expr call has unhandled "
                            f"kwargs {dropped} — add a transpiler handler"
                        )
                    return A.FuncCall(f"pyconf_{method}", self._awk_args(call))
        # Option method call (e.g. ENABLE_SHARED.is_no())
        if "." in func:
            obj, _, attr = func.rpartition(".")
            opt = self.option_vars.get(obj)
            if opt is not None:
                return A.FuncCall(
                    f"pyconf_option_{attr}", [A.StringLit(opt.key)]
                )

        # User-defined or plain function
        if "." not in func:
            return A.FuncCall(self._func_name(func), self._awk_args(call))
        attr = func.rpartition(".")[2]
        return A.FuncCall(self._func_name(attr), self._awk_args(call))

    def _build_ax_float_bigendian(self, call: Call) -> list[A.Stmt]:
        """Emit ax_c_float_words_bigendian with inline callback dispatch."""
        stmts: list[A.Stmt] = [
            A.ExprStmt(A.FuncCall("pyconf_ax_c_float_words_bigendian", []))
        ]
        # Extract callback names from kwargs
        callbacks: dict[str, str] = {}
        for k, v in call.kwargs.items():
            if k.startswith("on_") and isinstance(v, Var):
                callbacks[k] = v.name
        # Build if/elif chain to dispatch
        branches: list[A.IfBranch] = []
        for check, kwname in [
            ("big", "on_big"),
            ("little", "on_little"),
            ("unknown", "on_unknown"),
        ]:
            if kwname in callbacks:
                fname = callbacks[kwname]
                branches.append(
                    A.IfBranch(
                        A.BinOp(
                            A.Var("_pyconf_retval"),
                            "==",
                            A.StringLit(check),
                        ),
                        A.Block(
                            [
                                A.ExprStmt(
                                    A.FuncCall(self._func_name(fname), [])
                                )
                            ]
                        ),
                    )
                )
        if branches:
            stmts.append(A.If(branches=branches))
        return stmts

    def _build_check_prog_call(self, call: Call) -> A.FuncCall:
        """Build pyconf_check_prog(prog, search_path, default_val)."""
        args = self._awk_args(call)
        # prog is first positional arg
        prog = args[0] if args else A.StringLit("")
        # path is second positional arg (kwargs 'path' handled by _awk_args)
        search_path = args[1] if len(args) > 1 else A.StringLit("")
        # default kwarg (skipped by _awk_args, handle explicitly)
        default_val = A.StringLit("")
        if "default" in call.kwargs:
            default_val = self._expr(call.kwargs["default"])
        return A.FuncCall(
            "pyconf_check_prog", [prog, search_path, default_val]
        )

    def _build_define_call(self, call: Call) -> A.FuncCall:
        args = call.args
        name = self._expr(args[0]) if args else A.StringLit("")
        value = self._expr(args[1]) if len(args) > 1 else A.NumLit(1)
        desc = self._expr(args[2]) if len(args) > 2 else A.StringLit("")
        if len(args) <= 2:
            desc_kw = call.kwargs.get("description")
            if desc_kw is not None:
                desc = self._expr(desc_kw)
        quoted = 0
        if len(args) > 1 and not _is_int_expr(args[1]):
            quoted = 1
        return A.FuncCall(
            "pyconf_define", [name, value, A.NumLit(quoted), desc]
        )

    def _check_headers_stmts(self, method: str, call: Call) -> list[A.Stmt]:
        prologue_kw = call.kwargs.get("prologue")
        prologue = self._expr(prologue_kw) if prologue_kw is not None else None
        header_args = list(call.args)
        if prologue is not None:
            return [
                A.ExprStmt(
                    A.FuncCall(
                        "pyconf_check_header",
                        [self._expr(h), A.StringLit(""), prologue],
                    )
                )
                for h in header_args
            ]
        result: list[A.Stmt] = []
        for arg in header_args:
            h = self._expr(arg)
            if isinstance(arg, Const) and isinstance(arg.value, str):
                define = A.StringLit(_precompute_header_define(arg.value))
                ckey = A.StringLit(_precompute_header_cache_key(arg.value))
                result.append(
                    A.ExprStmt(
                        A.FuncCall(
                            "pyconf_check_header",
                            [
                                h,
                                A.StringLit(""),
                                A.StringLit(""),
                                define,
                                ckey,
                            ],
                        )
                    )
                )
            else:
                result.append(A.ExprStmt(A.FuncCall(f"pyconf_{method}", [h])))
        return result

    def _build_check_sizeof_call(self, call: Call) -> A.FuncCall:
        type_arg = self._expr(call.args[0]) if call.args else A.StringLit("")
        default: A.Expr = A.StringLit("")
        headers: A.Expr = A.StringLit("")
        for k, v in call.kwargs.items():
            if k == "default":
                default = self._expr(v)
            elif k in ("headers", "includes"):
                if isinstance(v, PyshList):
                    joined = " ".join(
                        str(elt.value) if isinstance(elt, Const) else "..."
                        for elt in v.elts
                    )
                    headers = A.StringLit(joined)
                else:
                    headers = self._expr(v)
        return A.FuncCall("pyconf_check_sizeof", [type_arg, default, headers])

    def _build_check_func_call(self, call: Call) -> A.FuncCall:
        func_arg = call.args[0] if call.args else Const("")
        func_name = self._expr(func_arg)
        headers_parts: list[str] = []
        define: A.Expr = A.StringLit("")
        cond_headers: list[str] = []
        raw_headers: list[str] = []
        for k, v in call.kwargs.items():
            if k in ("headers", "includes"):
                if isinstance(v, PyshList):
                    for elt in v.elts:
                        h = str(elt.value) if isinstance(elt, Const) else "..."
                        headers_parts.append(h)
                        raw_headers.append(h)
            elif k == "define":
                define = self._expr(v)
            elif k == "conditional_headers":
                if isinstance(v, PyshList):
                    for elt in v.elts:
                        if isinstance(elt, Const) and isinstance(elt.value, str):
                            cond_headers.append(elt.value)
        if (
            isinstance(define, A.StringLit)
            and not define.value
            and isinstance(func_arg, Const)
            and isinstance(func_arg.value, str)
        ):
            define = A.StringLit(_precompute_func_define(func_arg.value))
        # Build headers expression; for conditional_headers, emit inline ternary
        # (CACHE["ac_cv_header_X"] == "yes" ? "header.h" : "") to avoid needing
        # setup statements (which are not available in expression context).
        define_to_header: dict[str, str] = {}
        for h in raw_headers:
            define_to_header[_precompute_header_define(h)] = h
        header_parts_expr: list[A.Expr] = []
        for cond_define in cond_headers:
            header = define_to_header.get(
                cond_define, _define_to_header(cond_define)
            )
            if header in headers_parts:
                headers_parts.remove(header)
            cache_var = _precompute_header_cache_key(header)
            header_parts_expr.append(
                A.Concat(
                    [
                        A.StringLit(" "),
                        A.Ternary(
                            A.BinOp(
                                A.ArrayRef("CACHE", A.StringLit(cache_var)),
                                "==",
                                A.StringLit("yes"),
                            ),
                            A.StringLit(header),
                            A.StringLit(""),
                        ),
                    ]
                )
            )
        if headers_parts:
            header_parts_expr.insert(0, A.StringLit(" ".join(headers_parts)))
        headers: A.Expr
        if not header_parts_expr:
            headers = A.StringLit("")
        elif len(header_parts_expr) == 1:
            headers = header_parts_expr[0]
        else:
            headers = A.Concat(header_parts_expr)
        return A.FuncCall("pyconf_check_func", [func_name, headers, define])

    def _check_func_stmts(self, call: Call) -> list[A.Stmt]:
        func_arg = call.args[0] if call.args else Const("")
        func_name = self._expr(func_arg)
        headers_parts: list[str] = []
        define: A.Expr = A.StringLit("")
        cond_headers: list[str] = []
        raw_headers: list[str] = []
        for k, v in call.kwargs.items():
            if k in ("headers", "includes"):
                if isinstance(v, PyshList):
                    for elt in v.elts:
                        h = str(elt.value) if isinstance(elt, Const) else "..."
                        headers_parts.append(h)
                        raw_headers.append(h)
            elif k == "define":
                define = self._expr(v)
            elif k == "conditional_headers":
                if isinstance(v, PyshList):
                    for elt in v.elts:
                        if isinstance(elt, Const) and isinstance(
                            elt.value, str
                        ):
                            cond_headers.append(elt.value)

        define_to_header: dict[str, str] = {}
        for h in raw_headers:
            define_to_header[_precompute_header_define(h)] = h

        cond_vars: list[str] = []
        stmts: list[A.Stmt] = []
        for cond_define in cond_headers:
            header = define_to_header.get(
                cond_define, _define_to_header(cond_define)
            )
            if header in headers_parts:
                headers_parts.remove(header)
            cache_var = _precompute_header_cache_key(header)
            norm = header.replace("/", "_").replace(".", "_").replace("-", "_")
            var = f"_pyconf_cond_{norm}"
            cond_vars.append(var)
            self._locals.add(var)
            stmts.append(A.Assign(A.Var(var), A.StringLit("")))
            stmts.append(
                A.If(
                    branches=[
                        A.IfBranch(
                            A.BinOp(
                                A.ArrayRef("CACHE", A.StringLit(cache_var)),
                                "==",
                                A.StringLit("yes"),
                            ),
                            A.Block(
                                [A.Assign(A.Var(var), A.StringLit(header))]
                            ),
                        )
                    ],
                )
            )

        if (
            isinstance(define, A.StringLit)
            and not define.value
            and isinstance(func_arg, Const)
            and isinstance(func_arg.value, str)
        ):
            define = A.StringLit(_precompute_func_define(func_arg.value))

        header_parts_expr: list[A.Expr] = []
        if headers_parts:
            header_parts_expr.append(A.StringLit(" ".join(headers_parts)))
        for cv in cond_vars:
            header_parts_expr.append(A.Concat([A.StringLit(" "), A.Var(cv)]))
        if not header_parts_expr:
            headers: A.Expr = A.StringLit("")
        elif len(header_parts_expr) == 1:
            headers = header_parts_expr[0]
        else:
            headers = A.Concat(header_parts_expr)

        stmts.append(
            A.ExprStmt(
                A.FuncCall("pyconf_check_func", [func_name, headers, define])
            )
        )
        return stmts

    def _check_decl_stmts(self, call: Call) -> list[A.Stmt]:
        decl_arg = call.args[0] if call.args else Const("")
        decl = self._expr(decl_arg)
        includes_parts: list[str] = []
        includes_var_expr: A.Expr | None = None
        on_found = None
        on_notfound = None
        define_name_lit: str | None = None
        for k, v in call.kwargs.items():
            if k in ("includes", "extra_includes"):
                if isinstance(v, PyshList):
                    includes_parts.extend(
                        str(elt.value) if isinstance(elt, Const) else "..."
                        for elt in v.elts
                    )
                else:
                    includes_var_expr = self._expr(v)
            elif k == "on_found" and isinstance(v, Var):
                on_found = v.name
            elif k == "on_notfound" and isinstance(v, Var):
                on_notfound = v.name
            elif k == "define_name" and isinstance(v, Const):
                define_name_lit = str(v.value)
        includes: A.Expr
        if includes_parts:
            includes = A.StringLit(" ".join(includes_parts))
        elif includes_var_expr is not None:
            includes = includes_var_expr
        else:
            includes = A.StringLit("")
        define_args: list[A.Expr] = []
        if define_name_lit is not None:
            define_args = [A.StringLit(define_name_lit)]
        elif isinstance(decl_arg, Const) and isinstance(decl_arg.value, str):
            define_args = [
                A.StringLit(_precompute_decl_define(decl_arg.value))
            ]
        fc = A.FuncCall("pyconf_check_decl", [decl, includes] + define_args)
        if on_found or on_notfound:
            return [
                A.If(
                    branches=[
                        A.IfBranch(
                            fc,
                            A.Block(
                                [
                                    A.ExprStmt(
                                        A.FuncCall(
                                            self._func_name(on_found), []
                                        )
                                    )
                                ]
                                if on_found
                                else []
                            ),
                        )
                    ],
                    else_body=A.Block(
                        [
                            A.ExprStmt(
                                A.FuncCall(self._func_name(on_notfound), [])
                            )
                        ]
                    )
                    if on_notfound
                    else None,
                )
            ]
        return [A.ExprStmt(fc)]

    def _build_check_decl_expr(self, call: Call) -> A.FuncCall:
        """Build pyconf_check_decl() call for use as an expression."""
        decl_arg = call.args[0] if call.args else Const("")
        decl = self._expr(decl_arg)
        includes = self._extract_includes_expr(call)
        define_args: list[A.Expr] = []
        define_name_kw = call.kwargs.get("define_name")
        if define_name_kw is not None and isinstance(define_name_kw, Const):
            define_args = [A.StringLit(str(define_name_kw.value))]
        elif isinstance(decl_arg, Const) and isinstance(decl_arg.value, str):
            define_args = [
                A.StringLit(_precompute_decl_define(decl_arg.value))
            ]
        return A.FuncCall("pyconf_check_decl", [decl, includes] + define_args)

    def _build_check_member_expr(self, call: Call) -> A.FuncCall:
        """Build pyconf_check_member() call for use as an expression."""
        member = self._expr(call.args[0]) if call.args else A.StringLit("")
        headers = self._extract_includes_expr(call)
        return A.FuncCall("pyconf_check_member", [member, headers])

    def _build_check_type_expr(self, call: Call) -> A.FuncCall:
        """Build pyconf_check_type() call for use as an expression."""
        name = self._expr(call.args[0]) if call.args else A.StringLit("")
        headers = self._extract_includes_expr(call)
        return A.FuncCall("pyconf_check_type", [name, headers])

    def _extract_includes_expr(self, call: Call) -> A.Expr:
        """Extract includes/extra_includes/headers kwarg as an AWK expr."""
        for k, v in call.kwargs.items():
            if k in ("includes", "extra_includes", "headers"):
                if isinstance(v, PyshList):
                    parts = [
                        str(elt.value) if isinstance(elt, Const) else "..."
                        for elt in v.elts
                    ]
                    return A.StringLit(" ".join(parts))
                return self._expr(v)
        return A.StringLit("")

    def _extract_kwarg(self, call: Call, name: str) -> A.Expr:
        """Extract a kwarg value as an AWK expression, or empty string."""
        v = call.kwargs.get(name)
        if v is None:
            return A.StringLit("")
        return self._expr(v)

    def _build_check_lib_expr(self, call: Call) -> A.FuncCall:
        """Build pyconf_check_lib(lib, func, extra_cflags, extra_libs)."""
        args: list[A.Expr] = [self._expr(a) for a in call.args]
        # Pad to at least 2 positional args (lib, func)
        while len(args) < 2:
            args.append(A.StringLit(""))
        extra_cflags = self._extract_kwarg(call, "extra_cflags")
        extra_libs = self._extract_kwarg(call, "extra_libs")
        return A.FuncCall(
            "pyconf_check_lib", args + [extra_cflags, extra_libs]
        )

    def _build_check_header_expr(self, call: Call) -> A.FuncCall:
        """Build pyconf_check_header(header, prologue, default_inc, define,
        cache_key, extra_cflags)."""
        args: list[A.Expr] = [self._expr(a) for a in call.args]
        extra_cflags = call.kwargs.get("extra_cflags")
        if extra_cflags is not None:
            # Pad to position 6: header, prologue, default_inc, define,
            # cache_key, extra_cflags
            while len(args) < 5:
                args.append(A.StringLit(""))
            args.append(self._expr(extra_cflags))
        return A.FuncCall("pyconf_check_header", args)

    def _build_try_link_expr(self, call: Call) -> A.FuncCall:
        """Build pyconf_try_link(desc, source, extra_flags)."""
        args: list[A.Expr] = [self._expr(a) for a in call.args]
        extra_flags = call.kwargs.get("extra_flags")
        if extra_flags is not None:
            while len(args) < 2:
                args.append(A.StringLit(""))
            args.append(self._expr(extra_flags))
        return A.FuncCall("pyconf_try_link", args)

    def _build_check_linker_flag_expr(self, call: Call) -> A.FuncCall:
        """Build pyconf_check_linker_flag(flag, value)."""
        args: list[A.Expr] = [self._expr(a) for a in call.args]
        value = call.kwargs.get("value")
        if value is not None:
            args.append(self._expr(value))
        return A.FuncCall("pyconf_check_linker_flag", args)

    def _build_check_compile_flag_expr(self, call: Call) -> A.FuncCall:
        """Build pyconf_check_compile_flag(flag, extra_flags_array).

        extra_flags (list) and extra_cflags (str) are emitted as variadic
        args after the flag; the emitter packs them into an AWK array and
        the runtime shell-quotes each element via _arr_join_quoted().
        """
        args: list[A.Expr] = [self._expr(a) for a in call.args]
        extra_flags = call.kwargs.get("extra_flags")
        if extra_flags is not None:
            if isinstance(extra_flags, PyshList):
                for elt in extra_flags.elts:
                    args.append(self._expr(elt))
            else:
                args.append(self._expr(extra_flags))
        extra_cflags = call.kwargs.get("extra_cflags")
        if extra_cflags is not None:
            args.append(self._expr(extra_cflags))
        return A.FuncCall("pyconf_check_compile_flag", args)

    def _build_check_decls_call(self, call: Call) -> A.FuncCall:
        decl_args: list[A.Expr] = []
        if call.args:
            arg = call.args[0]
            if isinstance(arg, PyshList):
                decl_args = [self._expr(elt) for elt in arg.elts]
            else:
                decl_args = [self._expr(arg)]
        includes_parts: list[str] = []
        for k, v in call.kwargs.items():
            if k in ("includes", "extra_includes"):
                if isinstance(v, PyshList):
                    includes_parts.extend(
                        str(elt.value) if isinstance(elt, Const) else "..."
                        for elt in v.elts
                    )
        if includes_parts:
            decl_args.append(A.StringLit("--"))
            decl_args.extend(A.StringLit(h) for h in includes_parts)
        return A.FuncCall("pyconf_check_decls", decl_args)

    def _check_members_stmts(self, call: Call) -> list[A.Stmt]:
        members: list[PyshExpr] = []
        includes_parts: list[str] = []
        if call.args:
            arg = call.args[0]
            if isinstance(arg, PyshList):
                members = arg.elts
            else:
                members = [arg]
        for k, v in call.kwargs.items():
            if k in ("includes", "headers"):
                if isinstance(v, PyshList):
                    includes_parts.extend(
                        str(elt.value) if isinstance(elt, Const) else "..."
                        for elt in v.elts
                    )
        includes: A.Expr | None = (
            A.StringLit(" ".join(includes_parts)) if includes_parts else None
        )
        stmts: list[A.Stmt] = []
        for m_expr in members:
            m = self._expr(m_expr)
            if isinstance(m_expr, Const) and isinstance(m_expr.value, str):
                define = A.StringLit(_precompute_member_define(m_expr.value))
                stmts.append(
                    A.ExprStmt(
                        A.FuncCall(
                            "pyconf_check_member",
                            [m, includes, define]
                            if includes
                            else [m, A.StringLit(""), define],
                        )
                    )
                )
            elif includes:
                stmts.append(
                    A.ExprStmt(
                        A.FuncCall("pyconf_check_member", [m, includes])
                    )
                )
            else:
                stmts.append(
                    A.ExprStmt(A.FuncCall("pyconf_check_member", [m]))
                )
        return stmts

    def _check_type_stmts(self, call: Call) -> list[A.Stmt]:
        name = self._expr(call.args[0]) if call.args else A.StringLit("")
        headers_parts: list[str] = []
        headers_var_expr: A.Expr | None = None
        fallback = None
        for k, v in call.kwargs.items():
            if k in ("headers", "includes", "extra_includes"):
                if isinstance(v, PyshList):
                    headers_parts.extend(
                        str(elt.value) if isinstance(elt, Const) else "..."
                        for elt in v.elts
                    )
                else:
                    headers_var_expr = self._expr(v)
            elif k == "fallback_define":
                if isinstance(v, PyshList) and len(v.elts) >= 3:
                    fallback = (
                        self._expr(v.elts[0]),
                        self._expr(v.elts[1]),
                        self._expr(v.elts[2]),
                    )
        headers_args: list[A.Expr] = []
        if headers_parts:
            headers_args = [A.StringLit(" ".join(headers_parts))]
        elif headers_var_expr is not None:
            headers_args = [headers_var_expr]
        fc = A.FuncCall("pyconf_check_type", [name] + headers_args)
        if fallback:
            alias, base_type, desc = fallback
            return [
                A.If(
                    branches=[
                        A.IfBranch(
                            A.UnaryOp("!", fc),
                            A.Block(
                                [
                                    A.ExprStmt(
                                        A.FuncCall(
                                            "pyconf_define",
                                            [
                                                alias,
                                                base_type,
                                                A.NumLit(1),
                                                desc,
                                            ],
                                        )
                                    )
                                ]
                            ),
                        )
                    ],
                )
            ]
        return [A.ExprStmt(fc)]

    def _build_link_check_call(self, method: str, call: Call) -> A.FuncCall:
        extra_cflags: A.Expr | None = None
        extra_libs: A.Expr | None = None
        checking_kw: A.Expr | None = None
        preamble_kw: PyshExpr | None = None
        body_kw: PyshExpr | None = None
        includes_parts: list[str] = []
        includes_expr: A.Expr | None = None  # for variable includes
        program_kw: PyshExpr | None = None
        compiler_kw: A.Expr | None = None

        for k, v in call.kwargs.items():
            if k == "extra_cflags":
                extra_cflags = self._expr(v)
            elif k == "extra_libs":
                extra_libs = self._expr(v)
            elif k == "checking":
                checking_kw = self._expr(v)
            elif k == "preamble":
                preamble_kw = v
            elif k == "body":
                body_kw = v
            elif k in ("program", "source"):
                program_kw = v
            elif k == "compiler":
                compiler_kw = self._expr(v)
            elif k in ("includes", "headers"):
                if isinstance(v, PyshList):
                    includes_parts.extend(
                        str(elt.value) if isinstance(elt, Const) else "..."
                        for elt in v.elts
                    )
                else:
                    # Variable reference — generate runtime includes
                    includes_expr = A.FuncCall(
                        "_pyconf_includes_from_list", [self._expr(v)]
                    )

        inc_lines = "".join(f"#include <{h}>\n" for h in includes_parts)
        positional = call.args
        desc: A.Expr = checking_kw or A.StringLit("")
        source: A.Expr = A.StringLit("")

        if program_kw is not None:
            source = self._expr(program_kw)
            # When source/program is a kwarg, the first positional is the desc
            if len(positional) >= 1 and not checking_kw:
                desc = self._expr(positional[0])
        elif preamble_kw is not None or body_kw is not None:
            source = self._build_check_source(
                inc_lines,
                self._expr(preamble_kw) if preamble_kw else None,
                self._expr(body_kw) if body_kw else None,
            )
        elif len(positional) == 2:
            if _looks_like_full_source(positional[1]):
                desc = self._expr(positional[0])
                source = self._expr(positional[1])
            else:
                source = self._build_check_source(
                    inc_lines,
                    self._expr(positional[0]),
                    self._expr(positional[1]),
                )
                if checking_kw:
                    desc = checking_kw
        elif len(positional) == 1:
            if _is_complete_program(positional[0]):
                source = self._expr(positional[0])
            elif isinstance(positional[0], Const):
                source = self._build_check_source(
                    inc_lines, self._expr(positional[0]), None
                )
            else:
                # Variable — might be a complete program at runtime.
                # Emit a runtime check mirroring Python link_check's
                # prologue_is_source detection.
                var_expr = self._expr(positional[0])
                wrapped = self._build_check_source(inc_lines, var_expr, None)
                source = A.Ternary(
                    A.FuncCall("index", [var_expr, A.StringLit("int main")]),
                    var_expr,
                    wrapped,
                )

        # If includes was a variable, prepend runtime-generated #include lines
        if includes_expr is not None:
            source = A.Concat([includes_expr, source])

        args: list[A.Expr] = [desc, source]
        extra_parts = [e for e in (extra_cflags, extra_libs) if e is not None]
        if extra_parts:
            if len(extra_parts) == 1:
                args.append(extra_parts[0])
            else:
                # Insert a literal space between cflags and libs
                args.append(
                    A.Concat(
                        [extra_parts[0], A.StringLit(" "), extra_parts[1]]
                    )
                )
        if compiler_kw is not None:
            # Pad extra_flags if not already present
            if not extra_parts:
                args.append(A.StringLit(""))
            args.append(compiler_kw)
        return A.FuncCall(f"pyconf_{method}", args)

    def _build_check_source(
        self,
        inc_lines: str,
        pre_expr: A.Expr | None,
        body_expr: A.Expr | None,
    ) -> A.Expr:
        if body_expr is not None and isinstance(body_expr, A.StringLit):
            body_s = body_expr.value.rstrip()
            if (
                body_s
                and not body_s.endswith(";")
                and not body_s.endswith("}")
            ):
                body_s += ";"
            body_expr = None
        else:
            body_s = None

        if body_s is not None or body_expr is None:
            bs = body_s or ""
            suffix = f"\nint main(void) {{ {bs} return 0; }}"
            if pre_expr is None or isinstance(pre_expr, A.StringLit):
                pre = (
                    pre_expr.value if isinstance(pre_expr, A.StringLit) else ""
                )
                return A.StringLit(f"{inc_lines}{pre}{suffix}")
            return A.Concat(
                [
                    A.StringLit(inc_lines) if inc_lines else pre_expr,
                    *([] if not inc_lines else [pre_expr]),
                    A.StringLit(suffix),
                ]
            )

        parts: list[A.Expr] = []
        prefix = inc_lines
        if pre_expr is not None:
            if isinstance(pre_expr, A.StringLit):
                prefix += pre_expr.value
            else:
                if prefix:
                    parts.append(A.StringLit(prefix))
                    prefix = ""
                parts.append(pre_expr)
        if prefix:
            parts.append(A.StringLit(prefix))
        parts.append(A.StringLit("\\nint main(void) { "))
        parts.append(body_expr)
        parts.append(A.StringLit(" return 0; }"))
        return A.Concat(parts)

    def _build_run_check_call(self, call: Call) -> A.FuncCall:
        checking_kw: A.Expr | None = None
        extra_cflags: A.Expr | None = None
        extra_libs: A.Expr | None = None
        for k, v in call.kwargs.items():
            if k == "checking":
                checking_kw = self._expr(v)
            elif k == "extra_cflags":
                extra_cflags = self._expr(v)
            elif k == "extra_libs":
                extra_libs = self._expr(v)
        positional = call.args
        if len(positional) >= 2:
            desc = self._expr(positional[0])
            source = self._expr(positional[1])
        elif len(positional) == 1:
            desc = checking_kw or A.StringLit("")
            source = self._expr(positional[0])
        else:
            desc = checking_kw or A.StringLit("")
            source = A.StringLit("")
        # Compute cross_val: the value pyconf_run_check returns when
        # cross-compiling.  Must account for callers that invert the
        # boolean via on_success_return=False (ternary ? "no" : "yes").
        cross_val = self._run_check_cross_val(call)
        # Always pass all 5 positional args (desc, source, extra_cflags,
        # extra_libs, cross_val) so cross_val lands in the right slot.
        args: list[A.Expr] = [
            desc,
            source,
            extra_cflags or A.StringLit(""),
            extra_libs or A.StringLit(""),
            A.NumLit(cross_val),
        ]
        return A.FuncCall("pyconf_run_check", args)

    def _run_check_cross_val(self, call: Call) -> int:
        """Compute the cross-compiling return value for pyconf_run_check.

        When cross-compiling, run_check cannot execute the test binary.
        The Python version uses cross_compiling_default (or cross_default,
        default) to supply a fallback value.  Callers that invert the
        boolean with on_success_return=False generate a ternary like
        ``(run_check(...) ? "no" : "yes")``, so the cross_val we pass
        must be chosen so the ternary produces the correct result.

        Formula: cross_val = 1 iff cross_mapped == success_val, else 0.

        If the cross default is a non-constant expression (runtime value),
        we cannot compute cross_val at transpile time and return 0 to
        preserve the previous behavior.
        """
        # Find a constant cross-compiling default value.  The Python
        # run_check resolves these in order:
        #   default > cross_compiling_result > cross_compiling_default > cross_default
        # We look for the last constant one (later keys override earlier).
        cross_default: bool | None = None
        for k in (
            "default",
            "cross_compiling_result",
            "cross_compiling_default",
            "cross_default",
        ):
            if k in call.kwargs:
                match call.kwargs[k]:
                    case Const(value=bool() as val):
                        cross_default = val
                    case Const(value=str()):
                        # String sentinel (e.g. "cross", "undefined") —
                        # these are neither True nor False in Python;
                        # callers compare with == or `is True`.  Treat
                        # as False so the AWK ternary produces the
                        # non-triggering ("no") result.
                        cross_default = False
                    case _:
                        # Non-constant: can't resolve at transpile time.
                        cross_default = None

        if cross_default is None:
            # No constant cross default found; return 0 (old behavior).
            return 0

        # Determine whether the caller inverts the boolean.
        inverted = False
        for k, v in call.kwargs.items():
            match k:
                case "on_success_return" | "success":
                    match v:
                        case Const(value=val) if not val:
                            inverted = True
        success_val = "no" if inverted else "yes"
        cross_mapped = "yes" if cross_default else "no"
        return 1 if cross_mapped == success_val else 0

    def _build_stdlib_module_call(self, call: Call) -> A.FuncCall:
        name = self._expr(call.args[0]) if call.args else A.StringLit("")
        kw_map: dict[str, A.Expr] = {}
        for k, v in call.kwargs.items():
            if k in ("supported", "enabled", "cflags", "ldflags"):
                kw_map[k] = self._expr(v)
        return A.FuncCall(
            "pyconf_stdlib_module",
            [
                name,
                kw_map.get("supported", A.StringLit("yes")),
                kw_map.get("enabled", A.StringLit("yes")),
                kw_map.get("cflags", A.StringLit("")),
                kw_map.get("ldflags", A.StringLit("")),
                A.StringLit("yes" if "cflags" in kw_map else "no"),
                A.StringLit("yes" if "ldflags" in kw_map else "no"),
            ],
        )

    def _build_stdlib_module_simple_call(self, call: Call) -> A.FuncCall:
        name = self._expr(call.args[0]) if call.args else A.StringLit("")
        kw_map: dict[str, A.Expr] = {}
        for k, v in call.kwargs.items():
            if k in ("cflags", "ldflags"):
                kw_map[k] = self._expr(v)
        if kw_map:
            args = [
                name,
                kw_map.get("cflags", A.StringLit("")),
                kw_map.get("ldflags", A.StringLit("")),
                A.StringLit("yes" if "cflags" in kw_map else "no"),
                A.StringLit("yes" if "ldflags" in kw_map else "no"),
            ]
        else:
            args = [name]
        return A.FuncCall("pyconf_stdlib_module_simple", args)

    def _build_search_libs_call(self, call: Call) -> A.FuncCall:
        func_arg = self._expr(call.args[0]) if call.args else A.StringLit("")
        if len(call.args) >= 2 and isinstance(call.args[1], PyshList):
            libs = " ".join(
                str(elt.value) if isinstance(elt, Const) else "..."
                for elt in call.args[1].elts
            )
            lib_arg: A.Expr = A.StringLit(libs)
        elif len(call.args) >= 2:
            lib_arg = self._expr(call.args[1])
        else:
            lib_arg = A.StringLit("")
        return A.FuncCall("pyconf_search_libs", [func_arg, lib_arg])

    # ------------------------------------------------------------------
    # AWK argument list builder
    # ------------------------------------------------------------------

    _SKIP_KWARGS = frozenset(
        {
            "v",
            "vars",
            "capture_output",
            "text",
            "input",
            "prologue",
            "defines",
            "required",
            "cross_default",
            "default",
            "cross_compiling_default",
            "cross_compiling_result",
            "success",
            "failure",
            "on_success_return",
            "on_failure_return",
            "cache_var",
            "preamble",
            "body",
            "chmod_x",
            "fallback_define",
            "extra_includes",
            "conditional_headers",
            "autodefine",
            "define",
            "checking",
            "extra_cflags",
            "extra_libs",
            "on_found",
            "on_notfound",
            "program",
            "exports",
            # compiler and source are handled by _build_link_check_call
        }
    )

    # Functions whose AWK counterparts accept varargs packed into an array.
    # Split-assigned variables passed to these functions should be emitted as
    # ArrayVar so the emitter passes the array directly.
    _VARARGS_CALL_NAMES = frozenset(
        {
            "check_progs",
            "check_tools",
            "cmd",
            "cmd_output",
            "cmd_status",
            "fnmatch_any",
            "stdlib_module_set_na",
            "path_join",
            "check_funcs",
            "check_headers",
            "replace_funcs",
            "check_decls",
            "config_files",
            "config_files_x",
            "shell",
        }
    )

    def _awk_args(self, call: Call) -> list[A.Expr]:
        parts: list[A.Expr] = []
        # Check whether the target function is a varargs function so we know
        # to pass split-assigned variables as arrays rather than scalars.
        method = call.func.rpartition(".")[2]
        _is_varargs = method in self._VARARGS_CALL_NAMES

        for arg in call.args:
            if isinstance(arg, Var) and arg.name == "v":
                continue
            if isinstance(arg, PyshList):
                for elt in arg.elts:
                    parts.append(self._expr(elt))
            elif isinstance(arg, Add) and self._is_list_concat(arg):
                # List concatenation: [a, b] + q.split() → expand both sides
                self._expand_add_arg(arg, parts)
            elif (
                _is_varargs
                and isinstance(arg, Var)
                and arg.name
                in (self._array_locals | self._split_assigned_vars)
            ):
                parts.append(A.ArrayVar(self._var_name(arg.name)))
            else:
                parts.append(self._expr(arg))

        for k, v in call.kwargs.items():
            if k in self._SKIP_KWARGS:
                continue
            if k in ("headers", "includes"):
                if isinstance(v, PyshList):
                    joined = " ".join(
                        str(elt.value) if isinstance(elt, Const) else "..."
                        for elt in v.elts
                    )
                    parts.append(A.StringLit(joined))
                else:
                    parts.append(self._expr(v))
                continue
            if isinstance(v, Var) and k.startswith("on_"):
                parts.append(A.StringLit(v.name))
                continue
            parts.append(self._expr(v))

        return parts

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _is_list_concat(node: Add) -> bool:
        """Return True if the Add represents list concatenation (has a PyshList on either side)."""
        if isinstance(node.left, PyshList) or isinstance(node.right, PyshList):
            return True
        if isinstance(node.left, Add) and PyshToAwk._is_list_concat(node.left):
            return True
        if isinstance(node.right, Add) and PyshToAwk._is_list_concat(
            node.right
        ):
            return True
        return False

    def _expand_add_arg(self, node: Add, parts: list[A.Expr]) -> None:
        """Expand Add (list concatenation) into separate varargs elements.

        Handles patterns like [a, b] + q.split() by expanding the list
        elements and the split result as separate parts.
        """
        match node.left:
            case PyshList(elts=elts):
                for elt in elts:
                    parts.append(self._expr(elt))
            case Add():
                self._expand_add_arg(node.left, parts)
            case _:
                parts.append(self._expr(node.left))
        match node.right:
            case PyshList(elts=elts):
                for elt in elts:
                    parts.append(self._expr(elt))
            case Add():
                self._expand_add_arg(node.right, parts)
            case _:
                parts.append(self._expr(node.right))

    def _concat_with_sep(self, parts: list[A.Expr], sep: str) -> A.Expr:
        """Concatenate expressions with a separator."""
        if not parts:
            return A.StringLit("")
        if len(parts) == 1:
            return parts[0]
        result: list[A.Expr] = [parts[0]]
        for p in parts[1:]:
            result.append(A.StringLit(sep))
            result.append(p)
        return A.Concat(result)

    def _option_from_call(self, call: Call) -> OptionInfo | None:
        if "." not in call.func:
            return None
        obj = call.func.rpartition(".")[0]
        return self.option_vars.get(obj)

    def _option_from_expr(self, e: PyshExpr) -> OptionInfo | None:
        match e:
            case Attr(obj=obj):
                return self.option_vars.get(obj)
            case Call():
                return self._option_from_call(e)
            case _:
                return None


# string helper functions
_STRING_HELPERS = r"""

function _str_strip(s,    tmp) {
    tmp = s
    gsub(/^[ \t\n]+/, "", tmp)
    gsub(/[ \t\n]+$/, "", tmp)
    return tmp
}

function _str_rstrip(s,    tmp) {
    tmp = s
    gsub(/[ \t\n]+$/, "", tmp)
    return tmp
}

function _regex_escape(s,    tmp) {
    tmp = s
    gsub(/[\\.*+?[\](){}|^$]/, "\\\\&", tmp)
    return tmp
}

function _gsub_escape_replacement(s,    tmp) {
    tmp = s
    gsub(/\\/, "\\\\", tmp)
    gsub(/&/, "\\&", tmp)
    return tmp
}

function _str_replace(s, old, new,    tmp, escaped, safe_new) {
    tmp = s
    escaped = _regex_escape(old)
    safe_new = _gsub_escape_replacement(new)
    gsub(escaped, safe_new, tmp)
    return tmp
}

function _str_removeprefix(s, prefix,    plen) {
    plen = length(prefix)
    if (substr(s, 1, plen) == prefix)
        return substr(s, plen + 1)
    return s
}

function _str_removesuffix(s, suffix,    slen, total) {
    slen = length(suffix)
    total = length(s)
    if (total >= slen && substr(s, total - slen + 1) == suffix)
        return substr(s, 1, total - slen)
    return s
}

function _str_startswith(s, prefix) {
    return (substr(s, 1, length(prefix)) == prefix)
}

function _str_endswith(s, suffix,    slen, total) {
    slen = length(suffix)
    total = length(s)
    return (total >= slen && substr(s, total - slen + 1) == suffix)
}

function _str_join(sep, s,    n, arr, i, result) {
    n = split(s, arr, "\n")
    result = ""
    for (i = 1; i <= n; i++) {
        if (i > 1) result = result sep
        result = result arr[i]
    }
    return result
}

function _split_index(s, sep, idx,    n, arr) {
    n = split(s, arr, sep)
    if (idx >= 1 && idx <= n)
        return arr[idx]
    return ""
}

function _any(arr,    i) {
    for (i = 1; i <= arr[0]; i++)
        if (arr[i] == "yes") return 1
    return 0
}

function _all(arr,    i) {
    for (i = 1; i <= arr[0]; i++)
        if (arr[i] == "no") return 0
    return 1
}
"""


# ---------------------------------------------------------------------------
# Program-level transform
# ---------------------------------------------------------------------------


def transform_program(program: PyshProgram) -> A.Program:
    """Transform a pysh_ast.Program into an awk_ast.Program."""
    import os

    parts: list[A.FuncDef | A.BeginBlock | A.VerbatimBlock | A.Comment] = []

    # Embed pyconf.awk runtime
    parts.append(A.Comment("=== pyconf.awk runtime ==="))
    runtime_path = os.path.join(os.path.dirname(__file__), "pyconf.awk")
    if not os.path.exists(runtime_path):
        raise RuntimeError("cannot find pyconf.awk")
    with open(runtime_path) as f:
        content = f.read()
    parts.append(A.VerbatimBlock(content))
    parts.append(A.Comment("=== end pyconf.awk runtime ==="))
    parts.append(A.VerbatimBlock(_STRING_HELPERS))

    # Option registration function
    parts.append(A.Comment("=== Option declarations ==="))
    reg_body: list[A.Stmt] = []
    for mod in program.modules:
        for opt in mod.options:
            func = f"pyconf_arg_{opt.kind}"
            if opt.default is None:
                default = ""
            elif opt.default is True:
                default = "yes"
            elif opt.default is False:
                default = "no"
            else:
                default = str(opt.default)
            reg_body.append(
                A.ExprStmt(
                    A.FuncCall(
                        func,
                        [
                            A.StringLit(opt.name),
                            A.StringLit(default),
                            A.StringLit(opt.help_text),
                            A.StringLit(opt.metavar or ""),
                            A.StringLit(opt.var or ""),
                            A.StringLit(opt.false_is or ""),
                        ],
                    )
                )
            )
    parts.append(
        A.FuncDef(
            name="_pyconf_register_options",
            params=[],
            locals=[],
            body=A.Block(reg_body),
        )
    )

    # Module-level constants (function that sets them)
    parts.append(A.Comment("=== Module-level constants ==="))
    const_stmts: list[A.Stmt] = []
    for mod in program.modules:
        for const in mod.constants:
            match const.value:
                case str() as val:
                    const_stmts.append(
                        A.Assign(A.Var(const.name), A.StringLit(val))
                    )
                case bool() as val:
                    const_stmts.append(
                        A.Assign(
                            A.Var(const.name),
                            A.StringLit("yes" if val else "no"),
                        )
                    )
                case int() | float() as val:
                    const_stmts.append(
                        A.Assign(A.Var(const.name), A.StringLit(str(val)))
                    )
    if const_stmts:
        parts.append(
            A.FuncDef(
                name="_pyconf_init_constants",
                params=[],
                locals=[],
                body=A.Block(const_stmts),
            )
        )

    # Transpiled functions
    parts.append(A.Comment("=== Configuration functions ==="))
    all_func_names = {
        func.name for mod in program.modules for func in mod.functions
    }
    all_global_constants = {
        const.name for mod in program.modules for const in mod.constants
    }
    for mod in program.modules:
        if mod.functions:
            parts.append(A.Comment(f"--- {mod.name} ---"))
            for func in mod.functions:
                transformer = PyshToAwk(
                    mod,
                    program.all_option_vars,
                    all_func_names,
                    all_global_constants,
                )
                parts.append(transformer.transform_func(func))

    # BEGIN block: init → register → parse args → run → output
    parts.append(A.Comment("=== Main ==="))
    begin_stmts: list[A.Stmt] = [
        A.ExprStmt(A.FuncCall("pyconf_init", [])),
        A.ExprStmt(A.FuncCall("_pyconf_register_options", [])),
    ]
    if const_stmts:
        begin_stmts.append(
            A.ExprStmt(A.FuncCall("_pyconf_init_constants", []))
        )
    # Parse args
    begin_stmts.append(A.ExprStmt(A.FuncCall("pyconf_parse_args", [])))
    # Load CONFIG_SITE (after arg parsing so VAR=VALUE overrides take precedence)
    begin_stmts.append(A.ExprStmt(A.FuncCall("pyconf_load_config_site", [])))
    # Check --help
    begin_stmts.append(
        A.If(
            branches=[
                A.IfBranch(
                    A.FuncCall("pyconf_check_help", []),
                    A.Block([A.Exit(A.NumLit(0))]),
                )
            ],
        )
    )
    # Run functions in order
    for call in program.run_order:
        func_parts = call.split(".")
        func_name = func_parts[-1] if len(func_parts) > 1 else call
        begin_stmts.append(
            A.ExprStmt(A.FuncCall(_USER_FUNC_PREFIX + func_name, []))
        )
    # Note: pyconf_output() is NOT appended here — it is already called
    # inside u_generate_output() (transpiled from conf_output.generate_output
    # which calls pyconf.output()).
    begin_stmts.append(A.Exit(A.NumLit(0)))
    parts.append(A.BeginBlock(A.Block(begin_stmts)))

    return A.Program(parts=parts)
