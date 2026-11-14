import os
import unittest
from test import support
from test.support import import_helper


# skip tests if the _ctypes extension was not built
import_helper.import_module('ctypes')


class TestModule(unittest.TestCase):
    def test_deprecated_module_attributes(self):
        import ctypes
        import _ctypes

        for mod in (ctypes, _ctypes):
            with self.subTest(mod=mod):
                support.check_deprecated_module_attributes(
                    self, mod, "__version__", remove=(3, 20),
                )


def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
