import sys
import unittest
import unittest.mock
import _colorize


class TestColorizeFunction(unittest.TestCase):
    def test_colorized_detection_checks_for_environment_variables(self):
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

        if sys.platform == "win32":
            self.enterContext(
                unittest.mock.patch(
                    "nt._supports_virtual_terminal", return_value=True
                )
            )

        isatty_mock = self.enterContext(unittest.mock.patch("os.isatty"))
        isatty_mock.return_value = True

        for env_vars, expected in env_vars_expected:
            with self.subTest(env_vars=env_vars, expected_color=expected):
                with unittest.mock.patch("os.environ", env_vars):
                    self.assertEqual(_colorize.can_colorize(), expected)

        isatty_mock.return_value = False
        self.assertEqual(_colorize.can_colorize(), False)


if __name__ == "__main__":
    unittest.main()
