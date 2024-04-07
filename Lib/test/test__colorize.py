import contextlib
import sys
import traceback
import unittest
import unittest.mock
import _colorize
from test.support import captured_output


class TestColorizedTraceback(unittest.TestCase):
    def test_colorized_traceback(self):
        def foo(*args):
            x = {'a':{'b': None}}
            y = x['a']['b']['c']

        def baz(*args):
            return foo(1,2,3,4)

        def bar():
            return baz(1,
                    2,3
                    ,4)
        try:
            bar()
        except Exception as e:
            exc = traceback.TracebackException.from_exception(
                e, capture_locals=True
            )
        lines = "".join(exc.format(colorize=True))
        red = _colorize.ANSIColors.RED
        boldr = _colorize.ANSIColors.BOLD_RED
        reset = _colorize.ANSIColors.RESET
        self.assertIn("y = " + red + "x['a']['b']" + reset + boldr + "['c']" + reset, lines)
        self.assertIn("return " + red + "foo" + reset + boldr + "(1,2,3,4)" + reset, lines)
        self.assertIn("return " + red + "baz" + reset + boldr + "(1," + reset, lines)
        self.assertIn(boldr + "2,3" + reset, lines)
        self.assertIn(boldr + ",4)" + reset, lines)
        self.assertIn(red + "bar" + reset + boldr + "()" + reset, lines)

    def test_colorized_syntax_error(self):
        try:
            compile("a $ b", "<string>", "exec")
        except SyntaxError as e:
            exc = traceback.TracebackException.from_exception(
                e, capture_locals=True
            )
        actual = "".join(exc.format(colorize=True))
        magenta = _colorize.ANSIColors.MAGENTA
        boldm = _colorize.ANSIColors.BOLD_MAGENTA
        boldr = _colorize.ANSIColors.BOLD_RED
        reset = _colorize.ANSIColors.RESET
        expected = "".join(
            [
                f'  File {magenta}"<string>"{reset}, line {magenta}1{reset}\n',
                f'    a {boldr}${reset} b\n',
                f'      {boldr}^{reset}\n',
                f'{boldm}SyntaxError{reset}: {magenta}invalid syntax{reset}\n',
            ]
        )
        self.assertIn(expected, actual)

    def test_colorized_traceback_is_the_default(self):
        def foo():
            1/0

        from _testcapi import exception_print

        try:
            foo()
            self.fail("No exception thrown.")
        except Exception as e:
            with captured_output("stderr") as tbstderr:
                with unittest.mock.patch('_colorize.can_colorize', return_value=True):
                    exception_print(e)
            actual = tbstderr.getvalue().splitlines()

        red = _colorize.ANSIColors.RED
        boldr = _colorize.ANSIColors.BOLD_RED
        magenta = _colorize.ANSIColors.MAGENTA
        boldm = _colorize.ANSIColors.BOLD_MAGENTA
        reset = _colorize.ANSIColors.RESET
        lno_foo = foo.__code__.co_firstlineno
        expected = [
            'Traceback (most recent call last):',
            f'  File {magenta}"{__file__}"{reset}, '
            f'line {magenta}{lno_foo+6}{reset}, in {magenta}test_colorized_traceback_is_the_default{reset}',
            f'    {red}foo{reset+boldr}(){reset}',
            f'    {red}~~~{reset+boldr}^^{reset}',
            f'  File {magenta}"{__file__}"{reset}, '
            f'line {magenta}{lno_foo+1}{reset}, in {magenta}foo{reset}',
            f'    {red}1{reset+boldr}/{reset+red}0{reset}',
            f'    {red}~{reset+boldr}^{reset+red}~{reset}',
            f'{boldm}ZeroDivisionError{reset}: {magenta}division by zero{reset}',
        ]
        self.assertEqual(actual, expected)

    def test_colorized_detection_checks_for_environment_variables(self):
        if sys.platform == "win32":
            virtual_patching = unittest.mock.patch(
                "nt._supports_virtual_terminal", return_value=True
            )
        else:
            virtual_patching = contextlib.nullcontext()
        with virtual_patching:
            with unittest.mock.patch("os.isatty") as isatty_mock:
                isatty_mock.return_value = True
                with unittest.mock.patch("os.environ", {'TERM': 'dumb'}):
                    self.assertEqual(_colorize.can_colorize(), False)
                with unittest.mock.patch("os.environ", {'PYTHON_COLORS': '1'}):
                    self.assertEqual(_colorize.can_colorize(), True)
                with unittest.mock.patch("os.environ", {'PYTHON_COLORS': '0'}):
                    self.assertEqual(_colorize.can_colorize(), False)
                with unittest.mock.patch("os.environ", {'NO_COLOR': '1'}):
                    self.assertEqual(_colorize.can_colorize(), False)
                with unittest.mock.patch("os.environ", {'NO_COLOR': '1', "PYTHON_COLORS": '1'}):
                    self.assertEqual(_colorize.can_colorize(), True)
                with unittest.mock.patch("os.environ", {'FORCE_COLOR': '1'}):
                    self.assertEqual(_colorize.can_colorize(), True)
                with unittest.mock.patch("os.environ", {'FORCE_COLOR': '1', 'NO_COLOR': '1'}):
                    self.assertEqual(_colorize.can_colorize(), False)
                with unittest.mock.patch("os.environ", {'FORCE_COLOR': '1', "PYTHON_COLORS": '0'}):
                    self.assertEqual(_colorize.can_colorize(), False)
                isatty_mock.return_value = False
                self.assertEqual(_colorize.can_colorize(), False)


if __name__ == "__main__":
    unittest.main()
