import contextlib
import sys
import traceback
import unittest
import unittest.mock
import _colorize
from test.support import captured_output


class TestColorizedTraceback(unittest.TestCase):
    def test_colorized_detection_checks_for_environment_variables(self):
        if sys.platform == "win32":
            virtual_patching = unittest.mock.patch(
                "nt._supports_virtual_terminal", return_value=True
            )
        else:
            virtual_patching = contextlib.nullcontext()

        env_vars_expected = [
            ({'TERM': 'dumb'}, False),
            ({'PYTHON_COLORS': '1'}, True),
            ({'PYTHON_COLORS': '0'}, False),
            ({'NO_COLOR': '1'}, False),
            ({'NO_COLOR': '1', "PYTHON_COLORS": '1'}, True),
            ({'FORCE_COLOR': '1'}, True),
            ({'FORCE_COLOR': '1', 'NO_COLOR': '1'}, False),
            ({'FORCE_COLOR': '1', "PYTHON_COLORS": '0'}, False),
        ]

        with virtual_patching:
            with unittest.mock.patch("os.isatty") as isatty_mock:
                isatty_mock.return_value = True

                for env_vars, expected in env_vars_expected:
                    with unittest.mock.patch("os.environ", env_vars):
                        self.assertEqual(_colorize.can_colorize(), expected)

                isatty_mock.return_value = False
                self.assertEqual(_colorize.can_colorize(), False)


if __name__ == "__main__":
    unittest.main()
