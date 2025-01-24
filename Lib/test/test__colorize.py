import contextlib
import sys
import unittest
import unittest.mock
import _colorize
from test.support import force_not_colorized, make_clean_env

ORIGINAL_CAN_COLORIZE = _colorize.can_colorize


def setUpModule():
    _colorize.can_colorize = lambda *args, **kwargs: False


def tearDownModule():
    _colorize.can_colorize = ORIGINAL_CAN_COLORIZE


class TestColorizeFunction(unittest.TestCase):
    def setUp(self):
        # Remove PYTHON* environment variables to isolate from local user
        # settings and simulate running with `-E`. Such variables should be
        # added to test methods later to patched os.environ.
        patcher = unittest.mock.patch("os.environ", new=make_clean_env())
        self.addCleanup(patcher.stop)
        patcher.start()

    @force_not_colorized
    def test_colorized_detection_checks_for_environment_variables(self):
        flags = unittest.mock.MagicMock(ignore_environment=False)
        with (unittest.mock.patch("os.isatty") as isatty_mock,
              unittest.mock.patch("sys.stdout") as stdout_mock,
              unittest.mock.patch("sys.stderr") as stderr_mock,
              unittest.mock.patch("sys.flags", flags),
              unittest.mock.patch("_colorize.can_colorize", ORIGINAL_CAN_COLORIZE),
              (unittest.mock.patch("nt._supports_virtual_terminal", return_value=False)
               if sys.platform == "win32" else
               contextlib.nullcontext()) as vt_mock):

            isatty_mock.return_value = True
            stdout_mock.fileno.return_value = 1
            stdout_mock.isatty.return_value = True
            stderr_mock.fileno.return_value = 2
            stderr_mock.isatty.return_value = True

            for env_vars, expected in [
                ({"TERM": "dumb"}, False),
                ({"TERM": "dumb", "FORCE_COLOR": "1"}, True),
                ({"PYTHON_COLORS": "true"}, True),
                ({"PYTHON_COLORS": "2"}, True),
                ({"PYTHON_COLORS": "1"}, True),
                ({"PYTHON_COLORS": "0"}, False),
                ({"PYTHON_COLORS": ""}, True),
                ({"NO_COLOR": "1"}, False),
                ({"NO_COLOR": "0"}, False),
                ({"NO_COLOR": ""}, True),
                ({"NO_COLOR": "1", "PYTHON_COLORS": "1"}, True),
                ({"FORCE_COLOR": "1"}, True),
                ({"FORCE_COLOR": "1", "NO_COLOR": "1"}, False),
                ({"FORCE_COLOR": "1", "NO_COLOR": "0"}, False),
                ({"FORCE_COLOR": "1", "NO_COLOR": ""}, True),
                ({"FORCE_COLOR": "0", "NO_COLOR": "1"}, False),
                ({"FORCE_COLOR": "", "NO_COLOR": "1"}, False),
                ({"FORCE_COLOR": "1", "PYTHON_COLORS": "0"}, False),
                ({"FORCE_COLOR": "0", "PYTHON_COLORS": "0"}, False),
            ]:
                with self.subTest(env_vars=env_vars, expected=expected):
                    with unittest.mock.patch("os.environ", env_vars):
                        self.assertEqual(_colorize.can_colorize(), expected)

            with unittest.mock.patch("os.environ", {"NO_COLOR": ""}):
                if sys.platform == "win32":
                    vt_mock.return_value = False
                    self.assertEqual(_colorize.can_colorize(), False)

                    vt_mock.return_value = True
                    self.assertEqual(_colorize.can_colorize(), True)
                else:
                    self.assertEqual(_colorize.can_colorize(), True)

            with unittest.mock.patch("os.environ", {}):
                if sys.platform == "win32":
                    vt_mock.return_value = False
                    self.assertEqual(_colorize.can_colorize(), False)

                    vt_mock.return_value = True
                    self.assertEqual(_colorize.can_colorize(), True)
                else:
                    self.assertEqual(_colorize.can_colorize(), True)

                isatty_mock.return_value = False
                stdout_mock.isatty.return_value = False
                stderr_mock.isatty.return_value = False
                self.assertEqual(_colorize.can_colorize(), False)


if __name__ == "__main__":
    unittest.main()
