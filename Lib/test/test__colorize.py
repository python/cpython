import contextlib
import sys
import unittest
import unittest.mock
import _colorize
from test.support import force_not_colorized

ORIGINAL_CAN_COLORIZE = _colorize.can_colorize


def setUpModule():
    _colorize.can_colorize = lambda: False


def tearDownModule():
    _colorize.can_colorize = ORIGINAL_CAN_COLORIZE


class TestColorizeFunction(unittest.TestCase):
    @force_not_colorized
    def test_colorized_detection_checks_for_environment_variables(self):
        flags = unittest.mock.MagicMock(ignore_environment=False)
        with (unittest.mock.patch("os.isatty") as isatty_mock,
              unittest.mock.patch("sys.stderr") as stderr_mock,
              unittest.mock.patch("sys.flags", flags),
              unittest.mock.patch("_colorize.can_colorize", ORIGINAL_CAN_COLORIZE),
              (unittest.mock.patch("nt._supports_virtual_terminal", return_value=False)
               if sys.platform == "win32" else
               contextlib.nullcontext()) as vt_mock):

            isatty_mock.return_value = True
            stderr_mock.fileno.return_value = 2
            stderr_mock.isatty.return_value = True
            with unittest.mock.patch("os.environ", {'TERM': 'dumb'}):
                self.assertEqual(_colorize.can_colorize(), False)
            with unittest.mock.patch("os.environ", {'PYTHON_COLORS': '1'}):
                self.assertEqual(_colorize.can_colorize(), True)
            with unittest.mock.patch("os.environ", {'PYTHON_COLORS': '0'}):
                self.assertEqual(_colorize.can_colorize(), False)
            with unittest.mock.patch("os.environ", {'NO_COLOR': '1'}):
                self.assertEqual(_colorize.can_colorize(), False)
            with unittest.mock.patch("os.environ",
                                     {'NO_COLOR': '1', "PYTHON_COLORS": '1'}):
                self.assertEqual(_colorize.can_colorize(), True)
            with unittest.mock.patch("os.environ", {'FORCE_COLOR': '1'}):
                self.assertEqual(_colorize.can_colorize(), True)
            with unittest.mock.patch("os.environ",
                                     {'FORCE_COLOR': '1', 'NO_COLOR': '1'}):
                self.assertEqual(_colorize.can_colorize(), False)
            with unittest.mock.patch("os.environ",
                                     {'FORCE_COLOR': '1', "PYTHON_COLORS": '0'}):
                self.assertEqual(_colorize.can_colorize(), False)

            with unittest.mock.patch("os.environ", {}):
                if sys.platform == "win32":
                    self.assertEqual(_colorize.can_colorize(), False)

                    vt_mock.return_value = True
                    self.assertEqual(_colorize.can_colorize(), True)
                else:
                    self.assertEqual(_colorize.can_colorize(), True)

                isatty_mock.return_value = False
                stderr_mock.isatty.return_value = False
                self.assertEqual(_colorize.can_colorize(), False)


if __name__ == "__main__":
    unittest.main()
