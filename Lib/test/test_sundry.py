"""Do a minimal test of all the modules that aren't otherwise tested."""
import importlib
from test import support
from test.support import import_helper
from test.support import warnings_helper
import unittest

class TestUntestedModules(unittest.TestCase):
    def test_untested_modules_can_be_imported(self):
        untested = ('encodings',)
        with warnings_helper.check_warnings(quiet=True):
            for name in untested:
                try:
                    import_helper.import_module('test.test_{}'.format(name))
                except unittest.SkipTest:
                    importlib.import_module(name)
                else:
                    self.fail('{} has tests even though test_sundry claims '
                              'otherwise'.format(name))

            import html.entities  # noqa: F401

            try:
                # Not available on Windows
                import tty  # noqa: F401
            except ImportError:
                if support.verbose:
                    print("skipping tty")


if __name__ == "__main__":
    unittest.main()
