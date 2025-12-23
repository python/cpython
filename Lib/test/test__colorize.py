import contextlib
import dataclasses
import io
import sys
import unittest
import unittest.mock
import _colorize
from test.support.os_helper import EnvironmentVarGuard


@contextlib.contextmanager
def clear_env():
    with EnvironmentVarGuard() as mock_env:
        mock_env.unset("FORCE_COLOR", "NO_COLOR", "PYTHON_COLORS", "TERM")
        yield mock_env


def supports_virtual_terminal():
    if sys.platform == "win32":
        return unittest.mock.patch("nt._supports_virtual_terminal", return_value=True)
    else:
        return contextlib.nullcontext()


class TestTheme(unittest.TestCase):

    def test_attributes(self):
        # only theme configurations attributes by default
        for field in dataclasses.fields(_colorize.Theme):
            with self.subTest(field.name):
                self.assertIsSubclass(field.type, _colorize.ThemeSection)
                self.assertIsNotNone(field.default_factory)

    def test_copy_with(self):
        theme = _colorize.Theme()

        copy = theme.copy_with()
        self.assertEqual(theme, copy)

        unittest_no_colors = _colorize.Unittest.no_colors()
        copy = theme.copy_with(unittest=unittest_no_colors)
        self.assertEqual(copy.argparse, theme.argparse)
        self.assertEqual(copy.difflib, theme.difflib)
        self.assertEqual(copy.syntax, theme.syntax)
        self.assertEqual(copy.traceback, theme.traceback)
        self.assertEqual(copy.unittest, unittest_no_colors)

    def test_no_colors(self):
        # idempotence test
        theme_no_colors = _colorize.Theme().no_colors()
        theme_no_colors_no_colors = theme_no_colors.no_colors()
        self.assertEqual(theme_no_colors, theme_no_colors_no_colors)

        # attributes check
        for section in dataclasses.fields(_colorize.Theme):
            with self.subTest(section.name):
                section_theme = getattr(theme_no_colors, section.name)
                self.assertEqual(section_theme, section.type.no_colors())


class TestColorizeFunction(unittest.TestCase):
    def test_colorized_detection_checks_for_environment_variables(self):
        def check(env, fallback, expected):
            with (self.subTest(env=env, fallback=fallback),
                  clear_env() as mock_env):
                mock_env.update(env)
                isatty_mock.return_value = fallback
                stdout_mock.isatty.return_value = fallback
                self.assertEqual(_colorize.can_colorize(), expected)

        with (unittest.mock.patch("os.isatty") as isatty_mock,
              unittest.mock.patch("sys.stdout") as stdout_mock,
              supports_virtual_terminal()):
            stdout_mock.fileno.return_value = 1

            for fallback in False, True:
                check({}, fallback, fallback)
                check({'TERM': 'dumb'}, fallback, False)
                check({'TERM': 'xterm'}, fallback, fallback)
                check({'TERM': ''}, fallback, fallback)
                check({'FORCE_COLOR': '1'}, fallback, True)
                check({'FORCE_COLOR': '0'}, fallback, True)
                check({'FORCE_COLOR': ''}, fallback, fallback)
                check({'NO_COLOR': '1'}, fallback, False)
                check({'NO_COLOR': '0'}, fallback, False)
                check({'NO_COLOR': ''}, fallback, fallback)

            check({'TERM': 'dumb', 'FORCE_COLOR': '1'}, False, True)
            check({'FORCE_COLOR': '1', 'NO_COLOR': '1'}, True, False)

            for ignore_environment in False, True:
                # Simulate running with or without `-E`.
                flags = unittest.mock.MagicMock(ignore_environment=ignore_environment)
                with unittest.mock.patch("sys.flags", flags):
                    check({'PYTHON_COLORS': '1'}, True, True)
                    check({'PYTHON_COLORS': '1'}, False, not ignore_environment)
                    check({'PYTHON_COLORS': '0'}, True, ignore_environment)
                    check({'PYTHON_COLORS': '0'}, False, False)
                    for fallback in False, True:
                        check({'PYTHON_COLORS': 'x'}, fallback, fallback)
                        check({'PYTHON_COLORS': ''}, fallback, fallback)

                    check({'TERM': 'dumb', 'PYTHON_COLORS': '1'}, False, not ignore_environment)
                    check({'NO_COLOR': '1', 'PYTHON_COLORS': '1'}, False, not ignore_environment)
                    check({'FORCE_COLOR': '1', 'PYTHON_COLORS': '0'}, True, ignore_environment)

    @unittest.skipUnless(sys.platform == "win32", "requires Windows")
    def test_colorized_detection_checks_on_windows(self):
        with (clear_env(),
              unittest.mock.patch("os.isatty") as isatty_mock,
              unittest.mock.patch("sys.stdout") as stdout_mock,
              supports_virtual_terminal() as vt_mock):
            stdout_mock.fileno.return_value = 1
            isatty_mock.return_value = True
            stdout_mock.isatty.return_value = True

            vt_mock.return_value = True
            self.assertEqual(_colorize.can_colorize(), True)
            vt_mock.return_value = False
            self.assertEqual(_colorize.can_colorize(), False)
            import nt
            del nt._supports_virtual_terminal
            self.assertEqual(_colorize.can_colorize(), False)

    def test_colorized_detection_checks_for_std_streams(self):
        with (clear_env(),
              unittest.mock.patch("os.isatty") as isatty_mock,
              unittest.mock.patch("sys.stdout") as stdout_mock,
              unittest.mock.patch("sys.stderr") as stderr_mock,
              supports_virtual_terminal()):
            stdout_mock.fileno.return_value = 1
            stderr_mock.fileno.side_effect = ZeroDivisionError
            stderr_mock.isatty.side_effect = ZeroDivisionError

            isatty_mock.return_value = True
            stdout_mock.isatty.return_value = True
            self.assertEqual(_colorize.can_colorize(), True)

            isatty_mock.return_value = False
            stdout_mock.isatty.return_value = False
            self.assertEqual(_colorize.can_colorize(), False)

    def test_colorized_detection_checks_for_file(self):
        with clear_env(), supports_virtual_terminal():

            with unittest.mock.patch("os.isatty") as isatty_mock:
                file = unittest.mock.MagicMock()
                file.fileno.return_value = 1
                isatty_mock.return_value = True
                self.assertEqual(_colorize.can_colorize(file=file), True)
                isatty_mock.return_value = False
                self.assertEqual(_colorize.can_colorize(file=file), False)

            # No file.fileno.
            with unittest.mock.patch("os.isatty", side_effect=ZeroDivisionError):
                file = unittest.mock.MagicMock(spec=['isatty'])
                file.isatty.return_value = True
                self.assertEqual(_colorize.can_colorize(file=file), False)

            # file.fileno() raises io.UnsupportedOperation.
            with unittest.mock.patch("os.isatty", side_effect=ZeroDivisionError):
                file = unittest.mock.MagicMock()
                file.fileno.side_effect = io.UnsupportedOperation
                file.isatty.return_value = True
                self.assertEqual(_colorize.can_colorize(file=file), True)
                file.isatty.return_value = False
                self.assertEqual(_colorize.can_colorize(file=file), False)

            # The documentation for file.fileno says:
            # > An OSError is raised if the IO object does not use a file descriptor.
            # gh-141570: Check OSError is caught and handled
            with unittest.mock.patch("os.isatty", side_effect=ZeroDivisionError):
                file = unittest.mock.MagicMock()
                file.fileno.side_effect = OSError
                file.isatty.return_value = True
                self.assertEqual(_colorize.can_colorize(file=file), True)
                file.isatty.return_value = False
                self.assertEqual(_colorize.can_colorize(file=file), False)


if __name__ == "__main__":
    unittest.main()
