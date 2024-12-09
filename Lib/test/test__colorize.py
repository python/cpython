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
        if sys.platform == "win32":
            virtual_patching = unittest.mock.patch("nt._supports_virtual_terminal",
                                                   return_value=True)
        else:
            virtual_patching = contextlib.nullcontext()
        with virtual_patching:

            flags = unittest.mock.MagicMock(ignore_environment=False)
            with (unittest.mock.patch("os.isatty") as isatty_mock,
                  unittest.mock.patch("sys.flags", flags),
                  unittest.mock.patch("_colorize.can_colorize", ORIGINAL_CAN_COLORIZE)):
                isatty_mock.return_value = True
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
                isatty_mock.return_value = False
                with unittest.mock.patch("os.environ", {}):
                    self.assertEqual(_colorize.can_colorize(), False)


if __name__ == "__main__":
    unittest.main()
