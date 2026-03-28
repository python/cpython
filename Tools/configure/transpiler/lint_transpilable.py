"""lint_transpilable — AST-based checker for the transpilable Python subset.

Ensures conf_*.py files stay within the subset of Python that can be
mechanically translated to POSIX shell by the transpiler.  Run in CI
to prevent regressions.

Usage:
    python lint_transpilable.py [conf_*.py ...]

If no files are given, lints all conf_*.py files in the same directory.
"""

from __future__ import annotations

import ast
import glob
import os
import sys
from dataclasses import dataclass, field

# Imports allowed in conf_*.py files.
# 'pyconf' and 'conf_*' are always allowed.
# Others will be flagged as needing replacement with pyconf wrappers.
ALLOWED_IMPORTS = frozenset({"pyconf", "os", "warnings"})

# os attributes that are allowed (will have pyconf equivalents).
# os.path.*, os.environ.*, etc. are flagged separately.
ALLOWED_OS_ATTRS = frozenset(
    {
        "environ",
        "pathsep",
    }
)

# Imports that must be replaced with pyconf wrappers.
DISALLOWED_IMPORTS = frozenset(
    {
        "re",
        "subprocess",
        "pathlib",
        "shutil",
        "glob",
        "platform",
    }
)

# Allowed pyconf method calls (pyconf.foo()).
ALLOWED_PYCONF_CALLS = frozenset(
    {
        # Core checking/result
        "checking",
        "result",
        "warn",
        "notice",
        "error",
        "fatal",
        # Definitions and exports
        "define",
        "define_unquoted",
        "define_template",
        # Compilation checks
        "compile_check",
        "link_check",
        "run_check",
        "compile_link_check",
        "try_link",
        "check_compile_flag",
        "check_linker_flag",
        "run_check_with_cc_flag",
        # Function/header/type checks
        "check_func",
        "check_funcs",
        "check_headers",
        "check_header",
        "check_sizeof",
        "check_alignof",
        "sizeof",
        "check_type",
        "check_member",
        "check_members",
        "check_decl",
        "check_decls",
        "check_define",
        "check_c_const",
        "check_c_restrict",
        "check_c_bigendian",
        "ax_c_float_words_bigendian",
        "check_struct_tm",
        "check_struct_timezone",
        "check_lib",
        "search_libs",
        "replace_funcs",
        # Program checks
        "check_prog",
        "check_progs",
        "check_tools",
        "find_prog",
        "find_install",
        "find_mkdir_p",
        # Compiler
        "find_compiler",
        "use_system_extensions",
        # Option handling
        "arg_with",
        "arg_enable",
        "env_var",
        "get_dir_arg",
        # Host/build
        "canonical_host",
        # File operations (pyconf wrappers)
        "abspath",
        "basename",
        "getcwd",
        "readlink",
        "relpath",
        "path_exists",
        "path_is_dir",
        "path_is_file",
        "path_is_symlink",
        "path_parent",
        "path_join",
        "rm_f",
        "rmdir",
        "mkdir_p",
        "rename_file",
        "write_file",
        "read_file",
        "is_executable",
        "glob_files",
        "platform_machine",
        "platform_system",
        # Command execution (pyconf wrappers)
        "cmd",
        "cmd_output",
        "cmd_status",
        "run_program_output",
        "shell",
        # Pattern matching (pyconf wrapper)
        "fnmatch",
        "fnmatch_any",
        "sed",
        # Environment save/restore
        "save_env",
        # Init/output
        "init_args",
        "check_help",
        "output",
        "config_files",
        # pkg-config
        "pkg_config",
        "pkg_config_check",
        "pkg_check_modules",
        # Emscripten
        "check_emscripten_port",
        # Module support
        "stdlib_module",
        "stdlib_module_simple",
        "stdlib_module_set_na",
        # Misc
        "cache_check",
        "cache_store",
        "is_defined",
        "macro",
        "run",
    }
)

# Allowed OptionSpec method calls (WITH_FOO.method()).
ALLOWED_OPTION_METHODS = frozenset(
    {
        "value",
        "given",
        "is_yes",
        "is_no",
        "process_bool",
        "process_value",
        "value_or",
    }
)

# Allowed Vars (v) method calls.  The Vars object uses __getattr__ to
# return env/empty-string for unknown attributes, so calling anything
# other than these real methods (e.g. v.get()) silently returns a string
# and then fails at runtime with "TypeError: 'str' object is not callable".
ALLOWED_VARS_METHODS = frozenset(
    {
        "export",
        "is_set",
    }
)

# Allowed string methods that have shell equivalents.
ALLOWED_STR_METHODS = frozenset(
    {
        "startswith",
        "endswith",
        "strip",
        "lstrip",
        "rstrip",
        "replace",
        "split",
        "splitlines",
        "join",
        "lower",
        "upper",
        "format",
    }
)

# Allowed builtin calls.
ALLOWED_BUILTINS = frozenset(
    {
        "bool",
        "int",
        "str",
        "isinstance",
        "len",
        "print",
        "list",
        "tuple",
        "range",
        "sorted",
        "reversed",
        "enumerate",
        "min",
        "max",
        "any",
        "all",
        "zip",
        "map",
        "filter",
    }
)


@dataclass
class Violation:
    file: str
    line: int
    col: int
    msg: str

    def __str__(self):
        return f"{self.file}:{self.line}:{self.col}: {self.msg}"


@dataclass
class LintResult:
    violations: list[Violation] = field(default_factory=list)

    def add(self, file, node, msg):
        self.violations.append(
            Violation(
                file=file,
                line=getattr(node, "lineno", 0),
                col=getattr(node, "col_offset", 0),
                msg=msg,
            )
        )

    @property
    def ok(self):
        return len(self.violations) == 0


class TranspilableLinter(ast.NodeVisitor):
    """Walk a conf_*.py AST and flag constructs outside the transpilable subset."""

    def __init__(self, filename: str, result: LintResult):
        self.filename = filename
        self.result = result
        self._in_function = False
        # Track module-level option variables for OptionSpec method validation
        self._option_vars: set[str] = set()

    def _warn(self, node, msg):
        self.result.add(self.filename, node, msg)

    # ── Module level ──────────────────────────────────────────────────

    def visit_Module(self, node):
        for child in node.body:
            if isinstance(child, ast.Import):
                self._check_import(child)
            elif isinstance(child, ast.ImportFrom):
                self._check_import_from(child)
            elif isinstance(child, ast.FunctionDef):
                self._check_function_def(child)
            elif isinstance(child, ast.Assign):
                self._check_module_assign(child)
            elif isinstance(child, ast.AnnAssign):
                # Type annotations at module level are fine (constants)
                pass
            elif isinstance(child, ast.Expr):
                # Module-level expressions (docstrings, pyconf calls)
                if isinstance(child.value, ast.Constant) and isinstance(
                    child.value.value, str
                ):
                    pass  # docstring
                else:
                    self._check_expr_value(child.value)
            elif isinstance(child, ast.ClassDef):
                self._warn(child, "class definitions are not transpilable")
            else:
                self._warn(
                    child,
                    f"unexpected module-level statement: {type(child).__name__}",
                )

    # ── Imports ───────────────────────────────────────────────────────

    def _check_import(self, node):
        for alias in node.names:
            name = alias.name
            if name in DISALLOWED_IMPORTS:
                self._warn(
                    node,
                    f"import {name}: must be replaced with pyconf wrapper",
                )
            elif name.startswith("conf_") or name in ALLOWED_IMPORTS:
                pass  # ok
            elif name == "pyconf":
                pass  # ok
            else:
                self._warn(
                    node,
                    f"import {name}: not in allowed imports",
                )

    def _check_import_from(self, node):
        mod = node.module or ""
        if mod == "__future__":
            pass  # always allowed (e.g. from __future__ import annotations)
        elif (
            mod in DISALLOWED_IMPORTS
            or mod.split(".")[0] in DISALLOWED_IMPORTS
        ):
            self._warn(
                node,
                f"from {mod} import ...: must be replaced with pyconf wrapper",
            )
        elif (
            mod.startswith("conf_")
            or mod in ALLOWED_IMPORTS
            or mod == "pyconf"
        ):
            pass  # ok
        else:
            self._warn(node, f"from {mod} import ...: not in allowed imports")

    # ── Function definitions ──────────────────────────────────────────

    def _check_function_def(self, node):
        if node.name.startswith("_"):
            # Private helper functions are allowed but still linted
            pass

        # Check for *args and **kwargs — not transpilable
        if node.args.vararg:
            self._warn(
                node,
                f"*{node.args.vararg.arg}: star-args are not transpilable"
                " (pass a list instead)",
            )
        if node.args.kwarg:
            self._warn(
                node,
                f"**{node.args.kwarg.arg}: keyword star-args are not"
                " transpilable",
            )

        # Check decorators
        for dec in node.decorator_list:
            self._warn(dec, "decorators are not transpilable")

        # Check for nested functions
        old_in_function = self._in_function
        self._in_function = True
        for child in node.body:
            self._check_stmt(child)
        self._in_function = old_in_function

    # ── Statements (inside functions) ─────────────────────────────────

    def _check_stmt(self, node):
        if isinstance(node, ast.Expr):
            if isinstance(node.value, ast.Constant) and isinstance(
                node.value.value, str
            ):
                pass  # docstring
            else:
                self._check_expr_value(node.value)

        elif isinstance(node, ast.Assign):
            self._check_assign(node)

        elif isinstance(node, ast.AugAssign):
            self._check_expr(node.value)

        elif isinstance(node, ast.AnnAssign):
            if node.value:
                self._check_expr(node.value)

        elif isinstance(node, ast.If):
            self._check_expr(node.test)
            for child in node.body:
                self._check_stmt(child)
            for child in node.orelse:
                self._check_stmt(child)

        elif isinstance(node, ast.For):
            self._check_expr(node.iter)
            for child in node.body:
                self._check_stmt(child)
            for child in node.orelse:
                self._check_stmt(child)

        elif isinstance(node, ast.While):
            self._warn(node, "while loops are not transpilable")

        elif isinstance(node, ast.Return):
            if node.value:
                self._check_expr(node.value)

        elif isinstance(node, ast.With):
            self._check_with(node)

        elif isinstance(node, (ast.Try, ast.TryStar)):
            self._warn(node, "try/except is not transpilable")

        elif isinstance(node, ast.FunctionDef):
            self._warn(
                node, "nested function definitions are not transpilable"
            )

        elif isinstance(node, ast.ClassDef):
            self._warn(node, "class definitions are not transpilable")

        elif isinstance(node, ast.Delete):
            self._warn(node, "del statements are not transpilable")

        elif isinstance(node, ast.Global):
            self._warn(node, "global statements are not transpilable")

        elif isinstance(node, ast.Nonlocal):
            self._warn(node, "nonlocal statements are not transpilable")

        elif isinstance(node, ast.Assert):
            self._warn(node, "assert statements are not transpilable")

        elif isinstance(node, ast.Raise):
            self._warn(node, "raise statements are not transpilable")

        elif isinstance(node, ast.Pass):
            pass  # ok

        elif isinstance(node, ast.Break):
            pass  # ok

        elif isinstance(node, ast.Continue):
            pass  # ok

        elif isinstance(node, ast.Match):
            self._warn(node, "match statements are not transpilable")

        elif isinstance(node, ast.Import):
            self._warn(node, "imports inside functions are not transpilable")

        elif isinstance(node, ast.ImportFrom):
            self._warn(node, "imports inside functions are not transpilable")

        elif isinstance(node, ast.AsyncFunctionDef):
            self._warn(node, "async functions are not transpilable")

        elif isinstance(node, ast.AsyncFor):
            self._warn(node, "async for is not transpilable")

        elif isinstance(node, ast.AsyncWith):
            self._warn(node, "async with is not transpilable")

        else:
            self._warn(
                node,
                f"unexpected statement type: {type(node).__name__}",
            )

    # ── With statement ────────────────────────────────────────────────

    def _check_with(self, node):
        """Only `with pyconf.save_env():` is allowed."""
        allowed = False
        for item in node.items:
            ctx = item.context_expr
            if (
                isinstance(ctx, ast.Call)
                and isinstance(ctx.func, ast.Attribute)
                and isinstance(ctx.func.value, ast.Name)
                and ctx.func.value.id == "pyconf"
                and ctx.func.attr == "save_env"
            ):
                allowed = True
            else:
                self._warn(
                    node,
                    "with statements are not transpilable "
                    "(only `with pyconf.save_env()` is allowed)",
                )
                return

        if allowed:
            for child in node.body:
                self._check_stmt(child)

    # ── Assignments ───────────────────────────────────────────────────

    def _check_module_assign(self, node):
        """Check module-level assignments (constants, option declarations)."""
        for target in node.targets:
            if isinstance(target, ast.Name):
                # Check if RHS is a pyconf.arg_with/arg_enable/env_var call
                if isinstance(
                    node.value, ast.Call
                ) and self._is_pyconf_option_call(node.value):
                    self._option_vars.add(target.id)
        self._check_expr(node.value)

    def _check_assign(self, node):
        self._check_expr(node.value)
        # Warn if a list with multi-word string elements is assigned to a name.
        # Such variables cannot be safely iterated with `for x in var:` because
        # shell word-splits on spaces, breaking multi-word elements.
        # Fix: use an inline literal list in the for loop instead.
        if (
            isinstance(node.value, (ast.List, ast.Tuple))
            and len(node.targets) == 1
            and isinstance(node.targets[0], ast.Name)
        ):
            for elt in node.value.elts:
                if (
                    isinstance(elt, ast.Constant)
                    and isinstance(elt.value, str)
                    and " " in elt.value
                ):
                    self._warn(
                        node,
                        "list assigned to variable contains a multi-word string element "
                        f"({elt.value!r}); use an inline literal list in the for loop "
                        "instead of a named variable to avoid shell word-splitting",
                    )
                    break

    def _is_pyconf_option_call(self, node):
        """Check if a call is pyconf.arg_with/arg_enable/env_var."""
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute):
            if (
                isinstance(node.func.value, ast.Name)
                and node.func.value.id == "pyconf"
                and node.func.attr in ("arg_with", "arg_enable", "env_var")
            ):
                return True
        return False

    # ── Expressions ───────────────────────────────────────────────────

    def _check_expr_value(self, node):
        """Check an expression used as a statement (bare call, etc.)."""
        self._check_expr(node)

    def _check_expr(self, node):
        """Recursively check an expression is within the transpilable subset."""
        if node is None:
            return

        if isinstance(node, ast.Constant):
            pass  # ok: strings, ints, bools, None

        elif isinstance(node, ast.Name):
            pass  # ok: variable references

        elif isinstance(node, ast.Attribute):
            self._check_attribute(node)

        elif isinstance(node, ast.Call):
            self._check_call(node)

        elif isinstance(node, ast.BoolOp):
            for val in node.values:
                self._check_expr(val)

        elif isinstance(node, ast.BinOp):
            self._check_expr(node.left)
            self._check_expr(node.right)

        elif isinstance(node, ast.UnaryOp):
            self._check_expr(node.operand)

        elif isinstance(node, ast.Compare):
            self._check_expr(node.left)
            for comp in node.comparators:
                self._check_expr(comp)

        elif isinstance(node, ast.IfExp):
            # Ternary: x if cond else y
            self._check_expr(node.test)
            self._check_expr(node.body)
            self._check_expr(node.orelse)

        elif isinstance(node, ast.JoinedStr):
            # f-strings
            for val in node.values:
                self._check_expr(val)

        elif isinstance(node, ast.FormattedValue):
            self._check_expr(node.value)

        elif isinstance(node, ast.Subscript):
            self._check_expr(node.value)
            self._check_expr(node.slice)

        elif isinstance(node, ast.Starred):
            self._warn(
                node,
                "*-unpacking in calls is not transpilable"
                " (pass a list instead)",
            )
            self._check_expr(node.value)

        elif isinstance(node, ast.List):
            for elt in node.elts:
                self._check_expr(elt)

        elif isinstance(node, ast.Tuple):
            for elt in node.elts:
                self._check_expr(elt)

        elif isinstance(node, ast.Dict):
            for k in node.keys:
                if k:
                    self._check_expr(k)
            for val in node.values:
                self._check_expr(val)

        elif isinstance(node, ast.Set):
            for elt in node.elts:
                self._check_expr(elt)

        elif isinstance(node, ast.Slice):
            if node.lower:
                self._check_expr(node.lower)
            if node.upper:
                self._check_expr(node.upper)
            if node.step:
                self._check_expr(node.step)

        elif isinstance(node, ast.ListComp):
            self._warn(node, "list comprehensions are not transpilable")

        elif isinstance(node, ast.SetComp):
            self._warn(node, "set comprehensions are not transpilable")

        elif isinstance(node, ast.DictComp):
            self._warn(node, "dict comprehensions are not transpilable")

        elif isinstance(node, ast.GeneratorExp):
            self._warn(node, "generator expressions are not transpilable")

        elif isinstance(node, ast.Lambda):
            self._warn(node, "lambda expressions are not transpilable")

        elif isinstance(node, ast.NamedExpr):
            self._warn(node, "walrus operator (:=) is not transpilable")

        elif isinstance(node, ast.Await):
            self._warn(node, "await expressions are not transpilable")

        elif isinstance(node, ast.Yield):
            self._warn(node, "yield expressions are not transpilable")

        elif isinstance(node, ast.YieldFrom):
            self._warn(node, "yield from expressions are not transpilable")

        else:
            self._warn(
                node,
                f"unexpected expression type: {type(node).__name__}",
            )

    # ── Attribute access ──────────────────────────────────────────────

    def _check_attribute(self, node):
        """Check attribute access patterns."""
        # pyconf.attr — allowed for known attributes
        if isinstance(node.value, ast.Name) and node.value.id == "pyconf":
            # pyconf.something — we'll validate calls separately
            return

        # os.path.* — not transpilable
        if isinstance(node.value, ast.Attribute):
            if (
                isinstance(node.value.value, ast.Name)
                and node.value.value.id == "os"
                and node.value.attr == "path"
            ):
                self._warn(
                    node,
                    f"os.path.{node.attr}(): must be replaced with pyconf wrapper",
                )
                return

        # os.environ — allowed (limited)
        if isinstance(node.value, ast.Name) and node.value.id == "os":
            if node.attr not in ALLOWED_OS_ATTRS:
                self._warn(
                    node,
                    f"os.{node.attr}: must be replaced with pyconf wrapper",
                )
            return

        # v.FOO — always allowed (Vars object access)
        # string.method — checked at call site
        # other — recurse into value
        self._check_expr(node.value)

    # ── Function calls ────────────────────────────────────────────────

    def _check_call(self, node):
        """Check function/method calls."""
        # Check for **kwargs unpacking in calls
        for kw in node.keywords:
            if kw.arg is None:
                self._warn(
                    node,
                    "**-unpacking in calls is not transpilable",
                )
        func = node.func

        # pyconf.method(...)
        if isinstance(func, ast.Attribute) and isinstance(
            func.value, ast.Name
        ):
            if func.value.id == "pyconf":
                if func.attr not in ALLOWED_PYCONF_CALLS:
                    self._warn(
                        node,
                        f"pyconf.{func.attr}(): not in allowed pyconf calls",
                    )
                # Check arguments
                for arg in node.args:
                    self._check_expr(arg)
                for kw in node.keywords:
                    self._check_expr(kw.value)
                return

        # OptionSpec method calls: WITH_FOO.method(...)
        if isinstance(func, ast.Attribute) and isinstance(
            func.value, ast.Name
        ):
            if func.value.id in self._option_vars:
                if func.attr not in ALLOWED_OPTION_METHODS:
                    self._warn(
                        node,
                        f"{func.value.id}.{func.attr}(): "
                        f"not in allowed OptionSpec methods",
                    )
                for arg in node.args:
                    self._check_expr(arg)
                for kw in node.keywords:
                    self._check_expr(kw.value)
                return

        # String method calls: x.startswith(...), etc.
        if isinstance(func, ast.Attribute):
            if func.attr in ALLOWED_STR_METHODS:
                self._check_expr(func.value)
                for arg in node.args:
                    self._check_expr(arg)
                for kw in node.keywords:
                    self._check_expr(kw.value)
                return

        # Builtin calls
        if isinstance(func, ast.Name):
            if func.id in ALLOWED_BUILTINS:
                for arg in node.args:
                    self._check_expr(arg)
                for kw in node.keywords:
                    self._check_expr(kw.value)
                return
            elif func.id == "open":
                self._warn(
                    node, "open(): must be replaced with pyconf wrapper"
                )
                return

        # os.path.method(...)
        if isinstance(func, ast.Attribute) and isinstance(
            func.value, ast.Attribute
        ):
            if (
                isinstance(func.value.value, ast.Name)
                and func.value.value.id == "os"
                and func.value.attr == "path"
            ):
                self._warn(
                    node,
                    f"os.path.{func.attr}(): must be replaced with pyconf wrapper",
                )
                for arg in node.args:
                    self._check_expr(arg)
                for kw in node.keywords:
                    self._check_expr(kw.value)
                return

        # os.method(...)
        if isinstance(func, ast.Attribute) and isinstance(
            func.value, ast.Name
        ):
            if func.value.id == "os":
                if func.attr not in ALLOWED_OS_ATTRS:
                    self._warn(
                        node,
                        f"os.{func.attr}(): must be replaced with pyconf wrapper",
                    )
                for arg in node.args:
                    self._check_expr(arg)
                for kw in node.keywords:
                    self._check_expr(kw.value)
                return

        # v.export(...) / v.is_set(...) — allowed Vars methods
        if isinstance(func, ast.Attribute) and isinstance(
            func.value, ast.Name
        ):
            if func.value.id == "v":
                if func.attr not in ALLOWED_VARS_METHODS:
                    self._warn(
                        node,
                        f"v.{func.attr}(): not in allowed Vars methods"
                        f" ({', '.join(sorted(ALLOWED_VARS_METHODS))})"
                        " — use v.ATTR for attribute access instead",
                    )
                for arg in node.args:
                    self._check_expr(arg)
                for kw in node.keywords:
                    self._check_expr(kw.value)
                return

        # conf_module.function(...) — allowed
        if isinstance(func, ast.Attribute) and isinstance(
            func.value, ast.Name
        ):
            if func.value.id.startswith("conf_"):
                for arg in node.args:
                    self._check_expr(arg)
                for kw in node.keywords:
                    self._check_expr(kw.value)
                return

        # Generic: check func expression and args
        self._check_expr(func)
        for arg in node.args:
            self._check_expr(arg)
        for kw in node.keywords:
            self._check_expr(kw.value)


def lint_file(filepath: str) -> LintResult:
    """Lint a single file and return the result."""
    result = LintResult()
    with open(filepath) as f:
        source = f.read()
    try:
        tree = ast.parse(source, filename=filepath)
    except SyntaxError as e:
        result.add(
            filepath,
            type("node", (), {"lineno": e.lineno or 0, "col_offset": 0})(),
            f"syntax error: {e.msg}",
        )
        return result

    linter = TranspilableLinter(filepath, result)
    linter.visit(tree)
    return result


def main():
    if len(sys.argv) > 1:
        files = sys.argv[1:]
    else:
        # Find all conf_*.py in the same directory as this script
        script_dir = os.path.dirname(os.path.abspath(__file__))
        files = sorted(glob.glob(os.path.join(script_dir, "conf_*.py")))

    total_violations = 0
    for filepath in files:
        result = lint_file(filepath)
        for v in result.violations:
            print(v)
        total_violations += len(result.violations)

    if total_violations:
        print(f"\n{total_violations} transpilability violation(s) found.")
        sys.exit(1)
    else:
        print("All files pass transpilability checks.")
        sys.exit(0)


if __name__ == "__main__":
    main()
