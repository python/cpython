import sys
import unittest
from contextlib import contextmanager
from unittest.mock import patch

from test.support import warnings_helper


class WarningsHelperTests(unittest.TestCase):
    def test_check_warnings_when_module_is_not_in_sys_modules(self):
        warnings_module = sys.modules.pop("warnings", None)
        try:
            @contextmanager
            def catch_warnings(*, record):
                self.assertTrue(record)
                yield []

            with patch.object(
                warnings_helper.warnings, "catch_warnings", catch_warnings
            ):
                with warnings_helper.check_warnings(quiet=True):
                    pass
        finally:
            if warnings_module is not None:
                sys.modules["warnings"] = warnings_module


if __name__ == "__main__":
    unittest.main()
