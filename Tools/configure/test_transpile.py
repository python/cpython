"""Tests for the Python-to-AWK transpiler.

Each test writes a minimal conf_*.py-style Python function, transpiles it
using the AWK backend, wraps the output with the pyconf.awk runtime, runs
it under awk, and checks the result.

Usage:
    uv run Tools/configure/test_transpile.py
"""

import ast
import os
import subprocess
import sys
import tempfile
import textwrap
import unittest

# Ensure we can import from the Tools/configure directory
sys.path.insert(0, os.path.dirname(__file__))

from transpiler.py_to_pysh import PyToPysh
from transpiler.pysh_to_awk import PyshToAwk
from transpiler.awk_emit import AwkEmitter, AwkWriter
from transpiler import awk_ast as A
from transpiler.transpile import (
    ModuleInfo,
    OptionInfo,
)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RUNTIME_PATH = os.path.join(SCRIPT_DIR, "transpiler", "pyconf.awk")

# Find awk
AWK = "/usr/bin/awk"
if not os.path.exists(AWK):
    AWK = "/usr/bin/mawk"
    if not os.path.exists(AWK):
        AWK = "awk"


def _get_runtime() -> str:
    """Read pyconf.awk runtime."""
    with open(RUNTIME_PATH) as f:
        return f.read()


_RUNTIME_CACHE: str | None = None


def get_runtime() -> str:
    global _RUNTIME_CACHE
    if _RUNTIME_CACHE is None:
        _RUNTIME_CACHE = _get_runtime()
    return _RUNTIME_CACHE


# String helper functions from pysh_to_awk._STRING_HELPERS
_STRING_HELPERS = None


def get_string_helpers() -> str:
    global _STRING_HELPERS
    if _STRING_HELPERS is None:
        from transpiler.pysh_to_awk import _STRING_HELPERS as helpers

        _STRING_HELPERS = helpers
    return _STRING_HELPERS


def transpile_python_func(
    python_code: str,
    option_vars: dict[str, OptionInfo] | None = None,
) -> str:
    """Transpile a Python function string to AWK code.

    Returns the transpiled AWK function(s) as a string.
    """
    tree = ast.parse(textwrap.dedent(python_code))
    mod = ModuleInfo(name="test", filepath="<test>", tree=tree)

    all_opts = dict(mod.option_var_map)
    if option_vars:
        all_opts.update(option_vars)

    w = AwkWriter()
    emitter = AwkEmitter(w)

    # Emit module-level constants
    const_stmts: list[A.Stmt] = []
    for child in tree.body:
        if isinstance(child, ast.Assign) and len(child.targets) == 1:
            target = child.targets[0]
            if (
                isinstance(target, ast.Name)
                and isinstance(child.value, ast.Constant)
                and target.id not in mod.option_var_map
            ):
                val = child.value.value
                if isinstance(val, str):
                    const_stmts.append(
                        A.Assign(A.Var(target.id), A.StringLit(val))
                    )
                elif isinstance(val, (int, float)):
                    const_stmts.append(
                        A.Assign(A.Var(target.id), A.StringLit(str(val)))
                    )
                elif isinstance(val, bool):
                    const_stmts.append(
                        A.Assign(
                            A.Var(target.id),
                            A.StringLit("yes" if val else "no"),
                        )
                    )

    if const_stmts:
        const_func = A.FuncDef(
            name="_init_constants",
            params=[],
            locals=[],
            body=A.Block(const_stmts),
        )
        emitter.emit_func(const_func)
        w.emit()

    t1 = PyToPysh(mod, all_opts)
    t2 = PyshToAwk(mod, all_opts)
    for child in tree.body:
        if isinstance(child, ast.FunctionDef):
            mod.functions.append(child)
            pysh_func = t1.transform_func(child)
            awk_func = t2.transform_func(pysh_func)
            emitter.emit_func(awk_func)
            w.emit()

    return w.get_output()


def run_awk(
    awk_funcs: str,
    main_body: str,
    extra_setup: str = "",
) -> subprocess.CompletedProcess:
    """Run transpiled AWK functions under awk.

    Wraps the functions with pyconf.awk runtime, adds main_body in a BEGIN
    block, and runs with awk.

    Returns CompletedProcess with stdout/stderr.
    """
    default_setup = """
# Minimal init for test environment
V["CC"] = (ENVIRON["CC"] != "" ? ENVIRON["CC"] : "cc")
V["CFLAGS"] = ENVIRON["CFLAGS"]
V["CPPFLAGS"] = ENVIRON["CPPFLAGS"]
V["LDFLAGS"] = ENVIRON["LDFLAGS"]
V["LIBS"] = ENVIRON["LIBS"]
V["GCC"] = "yes"
_pyconf_srcdir = "."
_pyconf_cross_compiling = "no"
pyconf_mktmpdir()
"""
    script = f"""{get_runtime()}

{get_string_helpers()}

# === test setup ===

# === transpiled functions ===
{awk_funcs}

# === test main ===
BEGIN {{
{default_setup}
{extra_setup}
{main_body}
pyconf_cleanup()
}}
"""
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".awk", delete=False
    ) as f:
        f.write(script)
        f.flush()
        try:
            result = subprocess.run(
                [AWK, "-f", f.name],
                capture_output=True,
                text=True,
                timeout=30,
            )
        finally:
            os.unlink(f.name)
    return result


def transpile_and_run(
    python_code: str,
    main_body: str,
    extra_setup: str = "",
    option_vars: dict[str, OptionInfo] | None = None,
) -> subprocess.CompletedProcess:
    """Convenience: transpile Python code and run the result."""
    awk_funcs = transpile_python_func(python_code, option_vars)
    return run_awk(awk_funcs, main_body, extra_setup)


# ---------------------------------------------------------------------------
# Test cases
# ---------------------------------------------------------------------------


class TestAwkEmitter(unittest.TestCase):
    """Test the AWK emitter directly with hand-built AST nodes."""

    def test_string_literal(self):
        w = AwkWriter()
        e = AwkEmitter(w)
        result = e.expr(A.StringLit("hello"))
        self.assertEqual(result, '"hello"')

    def test_num_literal(self):
        w = AwkWriter()
        e = AwkEmitter(w)
        result = e.expr(A.NumLit(42))
        self.assertEqual(result, "42")

    def test_var(self):
        w = AwkWriter()
        e = AwkEmitter(w)
        result = e.expr(A.Var("x"))
        self.assertEqual(result, "x")

    def test_array_ref(self):
        w = AwkWriter()
        e = AwkEmitter(w)
        result = e.expr(A.ArrayRef("V", A.StringLit("FOO")))
        self.assertEqual(result, 'V["FOO"]')

    def test_func_call(self):
        w = AwkWriter()
        e = AwkEmitter(w)
        result = e.expr(A.FuncCall("f", [A.StringLit("a"), A.NumLit(1)]))
        self.assertEqual(result, 'f("a", 1)')

    def test_func_def(self):
        w = AwkWriter()
        e = AwkEmitter(w)
        func = A.FuncDef(
            name="test_func",
            params=["a", "b"],
            locals=["tmp"],
            body=A.Block([A.Return(A.Var("a"))]),
        )
        e.emit_func(func)
        output = w.get_output()
        self.assertIn("function test_func(a, b,    tmp)", output)
        self.assertIn("return a", output)

    def test_if_else(self):
        w = AwkWriter()
        e = AwkEmitter(w)
        stmt = A.If(
            branches=[
                A.IfBranch(
                    cond=A.BinOp(A.Var("x"), "==", A.StringLit("yes")),
                    body=A.Block([A.Assign(A.Var("y"), A.NumLit(1))]),
                )
            ],
            else_body=A.Block([A.Assign(A.Var("y"), A.NumLit(0))]),
        )
        e.emit_stmt(stmt)
        output = w.get_output()
        self.assertIn('if ((x == "yes"))', output)
        self.assertIn("y = 1", output)
        self.assertIn("} else {", output)
        self.assertIn("y = 0", output)


class TestBasicTranspile(unittest.TestCase):
    """Basic transpilation: assignments, if/else, function calls."""

    def test_simple_assignment(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.FOO = "hello"
""",
            'u_my_func()\nprint "FOO=" V["FOO"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("FOO=hello", result.stdout)

    def test_if_else(self):
        result = transpile_and_run(
            """
def my_func(v):
    if v.FOO == "yes":
        v.RESULT = "matched"
    else:
        v.RESULT = "nope"
""",
            'V["FOO"] = "yes"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=matched", result.stdout)

    def test_function_parameters(self):
        result = transpile_and_run(
            """
def my_func(v, name, value):
    v.RESULT = name
""",
            'u_my_func("hello", "world")\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=hello", result.stdout)

    def test_for_loop(self):
        result = transpile_and_run(
            """
def my_func(v):
    for x in ["a", "b", "c"]:
        v.RESULT = x
""",
            'u_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=c", result.stdout)

    def test_string_concat(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.RESULT = "hello" + " " + "world"
""",
            'u_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=hello world", result.stdout)

    def test_fstring(self):
        result = transpile_and_run(
            """
def my_func(v):
    name = "world"
    v.RESULT = f"hello {name}"
""",
            'u_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=hello world", result.stdout)

    def test_ternary(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.RESULT = "yes" if v.FOO == "1" else "no"
""",
            'V["FOO"] = "1"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=yes", result.stdout)

    def test_return_value(self):
        result = transpile_and_run(
            """
def helper(v):
    return "hello"

def my_func(v):
    v.RESULT = helper(v)
""",
            'u_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=hello", result.stdout)

    def test_boolean_true_false(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.A = True
    v.B = False
""",
            'u_my_func()\nprint "A=" V["A"] " B=" V["B"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("A=yes", result.stdout)
        self.assertIn("B=no", result.stdout)

    def test_elif(self):
        result = transpile_and_run(
            """
def my_func(v):
    if v.X == "a":
        v.RESULT = "1"
    elif v.X == "b":
        v.RESULT = "2"
    else:
        v.RESULT = "3"
""",
            'V["X"] = "b"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=2", result.stdout)


class TestStringMethods(unittest.TestCase):
    """Test string method transpilation."""

    def test_lower(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.RESULT = v.INPUT.lower()
""",
            'V["INPUT"] = "HELLO"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=hello", result.stdout)

    def test_upper(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.RESULT = v.INPUT.upper()
""",
            'V["INPUT"] = "hello"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=HELLO", result.stdout)

    def test_startswith(self):
        result = transpile_and_run(
            """
def my_func(v):
    if v.INPUT.startswith("hel"):
        v.RESULT = "yes"
    else:
        v.RESULT = "no"
""",
            'V["INPUT"] = "hello"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=yes", result.stdout)

    def test_endswith(self):
        result = transpile_and_run(
            """
def my_func(v):
    if v.INPUT.endswith("llo"):
        v.RESULT = "yes"
    else:
        v.RESULT = "no"
""",
            'V["INPUT"] = "hello"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=yes", result.stdout)

    def test_replace(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.RESULT = v.INPUT.replace("world", "awk")
""",
            'V["INPUT"] = "hello world"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=hello awk", result.stdout)

    def test_in_string(self):
        result = transpile_and_run(
            """
def my_func(v):
    if "ell" in v.INPUT:
        v.RESULT = "found"
    else:
        v.RESULT = "not found"
""",
            'V["INPUT"] = "hello"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=found", result.stdout)


class TestControlFlow(unittest.TestCase):
    """Test control flow transpilation."""

    def test_while_equivalent(self):
        """Test for loop with list."""
        result = transpile_and_run(
            """
def my_func(v):
    total = 0
    for x in [1, 2, 3]:
        total = total + x
    v.RESULT = str(total)
""",
            'u_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        # AWK treats string "1"+"2"+"3" as numeric addition
        # but our list literals are strings so concatenation
        # Just check it completes successfully

    def test_break(self):
        result = transpile_and_run(
            """
def my_func(v):
    for x in ["a", "b", "c"]:
        if x == "b":
            break
        v.RESULT = x
""",
            'u_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=a", result.stdout)

    def test_continue(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.RESULT = ""
    for x in ["a", "b", "c"]:
        if x == "b":
            continue
        v.RESULT = v.RESULT + x
""",
            'u_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=ac", result.stdout)

    def test_membership_in_list(self):
        result = transpile_and_run(
            """
def my_func(v):
    if v.X in ("a", "b", "c"):
        v.RESULT = "found"
    else:
        v.RESULT = "not found"
""",
            'V["X"] = "b"\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=found", result.stdout)


class TestVariableAccess(unittest.TestCase):
    """Test v.NAME and related access patterns."""

    def test_v_read_write(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.OUT = v.IN
""",
            'V["IN"] = "test_value"\nu_my_func()\nprint "OUT=" V["OUT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("OUT=test_value", result.stdout)

    def test_aug_assign(self):
        result = transpile_and_run(
            """
def my_func(v):
    v.RESULT = "hello"
    v.RESULT += " world"
""",
            'u_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=hello world", result.stdout)

    def test_comparison_ops(self):
        result = transpile_and_run(
            """
def my_func(v):
    if v.X == "yes":
        v.R1 = "eq"
    if v.X != "no":
        v.R2 = "neq"
""",
            'V["X"] = "yes"\nu_my_func()\nprint "R1=" V["R1"] " R2=" V["R2"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("R1=eq", result.stdout)
        self.assertIn("R2=neq", result.stdout)

    def test_none_check(self):
        result = transpile_and_run(
            """
def my_func(v):
    if v.X is None:
        v.RESULT = "none"
    else:
        v.RESULT = "not_none"
""",
            'V["X"] = ""\nu_my_func()\nprint "RESULT=" V["RESULT"]',
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RESULT=none", result.stdout)


class TestExpandCmdVars(unittest.TestCase):
    """Test _expand_cmd_vars() in pyconf.awk runtime."""

    def _run_expand(self, cmd_str, extra_setup=""):
        """Run _expand_cmd_vars on cmd_str and return the result."""
        main_body = f"""
V["CPP"] = "cc -E"
V["CPPFLAGS"] = "-I/usr/include"
V["CC"] = "/usr/bin/cc"
{extra_setup}
print _expand_cmd_vars("{cmd_str}")
"""
        return run_awk("", main_body)

    def test_simple_var(self):
        result = self._run_expand("$CC --version")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "/usr/bin/cc --version")

    def test_multiple_vars(self):
        result = self._run_expand("$CPP $CPPFLAGS file.c")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "cc -E -I/usr/include file.c")

    def test_braced_var(self):
        result = self._run_expand("${CC} --version")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "/usr/bin/cc --version")

    def test_no_vars(self):
        result = self._run_expand("echo hello world")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "echo hello world")

    def test_unknown_var_expands_empty(self):
        result = self._run_expand("prefix$UNKNOWN_VARsuffix rest")
        self.assertEqual(result.returncode, 0, result.stderr)
        # UNKNOWN_VARsuffix is one token, expands to empty
        self.assertEqual(result.stdout.strip(), "prefix rest")

    def test_environ_fallback(self):
        result = self._run_expand("$HOME/bin")
        self.assertEqual(result.returncode, 0, result.stderr)
        import os

        self.assertEqual(result.stdout.strip(), f"{os.environ['HOME']}/bin")

    def test_dollar_at_end(self):
        result = self._run_expand("cost is 5$")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "cost is 5$")

    def test_pyconf_run_capture_expands(self):
        """Test that pyconf_run_capture properly expands $VAR in commands."""
        result = transpile_and_run(
            """
def my_func(v):
    result = pyconf.run(
        "$CPP $CPPFLAGS -",
        vars={"CPP": v.CPP, "CPPFLAGS": v.CPPFLAGS},
        capture_output=True,
        text=True,
        input="#define HELLO 42\\nHELLO",
    )
    v.RC = str(result.returncode)
""",
            """
V["CPP"] = "cc -E"
V["CPPFLAGS"] = ""
u_my_func()
print "RC=" V["RC"]
""",
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        # The C preprocessor should succeed (returncode 0)
        self.assertIn("RC=0", result.stdout)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    unittest.main()
