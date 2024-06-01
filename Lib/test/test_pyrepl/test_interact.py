import contextlib
import io
import unittest
from unittest.mock import patch
from textwrap import dedent

from test.support import force_not_colorized

from _pyrepl.console import InteractiveColoredConsole


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

    def test_no_active_future(self):
        console = InteractiveColoredConsole()
        source = "x: int = 1; print(__annotations__)"
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            result = console.runsource(source)
        self.assertFalse(result)
        self.assertEqual(f.getvalue(), "{'x': <class 'int'>}\n")
