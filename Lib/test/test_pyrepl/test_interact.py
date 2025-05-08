import contextlib
import io
import unittest
import warnings
from unittest.mock import patch
from textwrap import dedent

from test.support import force_not_colorized

from _pyrepl.console import InteractiveColoredConsole
from _pyrepl.simple_interact import _more_lines

class TestSimpleInteract(unittest.TestCase):
    def test_multiple_statements(self):
        namespace = {}
        code = dedent("""\
        class A:
            def foo(self):


                pass

        class B:
            def bar(self):
                pass

        a = 1
        a
        """)
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        f = io.StringIO()
        with (
            patch.object(InteractiveColoredConsole, "showsyntaxerror") as showsyntaxerror,
            patch.object(InteractiveColoredConsole, "runsource", wraps=console.runsource) as runsource,
            contextlib.redirect_stdout(f),
        ):
            more = console.push(code, filename="<stdin>", _symbol="single")  # type: ignore[call-arg]
        self.assertFalse(more)
        showsyntaxerror.assert_not_called()


    def test_multiple_statements_output(self):
        namespace = {}
        code = dedent("""\
        b = 1
        b
        a = 1
        a
        """)
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            more = console.push(code, filename="<stdin>", _symbol="single")  # type: ignore[call-arg]
        self.assertFalse(more)
        self.assertEqual(f.getvalue(), "1\n")

    @force_not_colorized
    def test_multiple_statements_fail_early(self):
        console = InteractiveColoredConsole()
        code = dedent("""\
        raise Exception('foobar')
        print('spam', 'eggs', sep='&')
        """)
        f = io.StringIO()
        with contextlib.redirect_stderr(f):
            console.runsource(code)
        self.assertIn('Exception: foobar', f.getvalue())
        self.assertNotIn('spam&eggs', f.getvalue())

    def test_empty(self):
        namespace = {}
        code = ""
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            more = console.push(code, filename="<stdin>", _symbol="single")  # type: ignore[call-arg]
        self.assertFalse(more)
        self.assertEqual(f.getvalue(), "")

    def test_runsource_compiles_and_runs_code(self):
        console = InteractiveColoredConsole()
        source = "print('Hello, world!')"
        with patch.object(console, "runcode") as mock_runcode:
            console.runsource(source)
            mock_runcode.assert_called_once()

    def test_runsource_returns_false_for_successful_compilation(self):
        console = InteractiveColoredConsole()
        source = "print('Hello, world!')"
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            result = console.runsource(source)
        self.assertFalse(result)

    @force_not_colorized
    def test_runsource_returns_false_for_failed_compilation(self):
        console = InteractiveColoredConsole()
        source = "print('Hello, world!'"
        f = io.StringIO()
        with contextlib.redirect_stderr(f):
            result = console.runsource(source)
        self.assertFalse(result)
        self.assertIn('SyntaxError', f.getvalue())

    @force_not_colorized
    def test_runsource_show_syntax_error_location(self):
        console = InteractiveColoredConsole()
        source = "def f(x, x): ..."
        f = io.StringIO()
        with contextlib.redirect_stderr(f):
            result = console.runsource(source)
        self.assertFalse(result)
        r = """
    def f(x, x): ...
             ^
SyntaxError: duplicate argument 'x' in function definition"""
        self.assertIn(r, f.getvalue())

    def test_runsource_shows_syntax_error_for_failed_compilation(self):
        console = InteractiveColoredConsole()
        source = "print('Hello, world!'"
        with patch.object(console, "showsyntaxerror") as mock_showsyntaxerror:
            console.runsource(source)
            mock_showsyntaxerror.assert_called_once()
        source = dedent("""\
        match 1:
            case {0: _, 0j: _}:
                pass
        """)
        with patch.object(console, "showsyntaxerror") as mock_showsyntaxerror:
            console.runsource(source)
            mock_showsyntaxerror.assert_called_once()

    def test_runsource_survives_null_bytes(self):
        console = InteractiveColoredConsole()
        source = "\x00\n"
        f = io.StringIO()
        with contextlib.redirect_stdout(f), contextlib.redirect_stderr(f):
            result = console.runsource(source)
        self.assertFalse(result)
        self.assertIn("source code string cannot contain null bytes", f.getvalue())

    def test_no_active_future(self):
        console = InteractiveColoredConsole()
        source = dedent("""\
        x: int = 1
        print(__annotate__(1))
        """)
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            result = console.runsource(source)
        self.assertFalse(result)
        self.assertEqual(f.getvalue(), "{'x': <class 'int'>}\n")

    def test_future_annotations(self):
        console = InteractiveColoredConsole()
        source = dedent("""\
        from __future__ import annotations
        def g(x: int): ...
        print(g.__annotations__)
        """)
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            result = console.runsource(source)
        self.assertFalse(result)
        self.assertEqual(f.getvalue(), "{'x': 'int'}\n")

    def test_future_barry_as_flufl(self):
        console = InteractiveColoredConsole()
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            result = console.runsource("from __future__ import barry_as_FLUFL\n")
            result = console.runsource("""print("black" <> 'blue')\n""")
        self.assertFalse(result)
        self.assertEqual(f.getvalue(), "True\n")


class TestMoreLines(unittest.TestCase):
    def test_invalid_syntax_single_line(self):
        namespace = {}
        code = "if foo"
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertFalse(_more_lines(console, code))

    def test_empty_line(self):
        namespace = {}
        code = ""
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertFalse(_more_lines(console, code))

    def test_valid_single_statement(self):
        namespace = {}
        code = "foo = 1"
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertFalse(_more_lines(console, code))

    def test_multiline_single_assignment(self):
        namespace = {}
        code = dedent("""\
        foo = [
            1,
            2,
            3,
        ]""")
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertFalse(_more_lines(console, code))

    def test_multiline_single_block(self):
        namespace = {}
        code = dedent("""\
        def foo():
            '''docs'''

            return 1""")
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertTrue(_more_lines(console, code))

    def test_multiple_statements_single_line(self):
        namespace = {}
        code = "foo = 1;bar = 2"
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertFalse(_more_lines(console, code))

    def test_multiple_statements(self):
        namespace = {}
        code = dedent("""\
        import time

        foo = 1""")
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertTrue(_more_lines(console, code))

    def test_multiple_blocks(self):
        namespace = {}
        code = dedent("""\
        from dataclasses import dataclass

        @dataclass
        class Point:
            x: float
            y: float""")
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertTrue(_more_lines(console, code))

    def test_multiple_blocks_empty_newline(self):
        namespace = {}
        code = dedent("""\
        from dataclasses import dataclass

        @dataclass
        class Point:
            x: float
            y: float
        """)
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertFalse(_more_lines(console, code))

    def test_multiple_blocks_indented_newline(self):
        namespace = {}
        code = (
            "from dataclasses import dataclass\n"
            "\n"
            "@dataclass\n"
            "class Point:\n"
            "    x: float\n"
            "    y: float\n"
            "    "
        )
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertFalse(_more_lines(console, code))

    def test_incomplete_statement(self):
        namespace = {}
        code = "if foo:"
        console = InteractiveColoredConsole(namespace, filename="<stdin>")
        self.assertTrue(_more_lines(console, code))


class TestWarnings(unittest.TestCase):
    def test_pep_765_warning(self):
        """
        Test that a SyntaxWarning emitted from the
        AST optimizer is only shown once in the REPL.
        """
        # gh-131927
        console = InteractiveColoredConsole()
        code = dedent("""\
        def f():
            try:
                return 1
            finally:
                return 2
        """)

        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("default")
            console.runsource(code)

        count = sum("'return' in a 'finally' block" in str(w.message)
                    for w in caught)
        self.assertEqual(count, 1)
