import os
import unittest
from test import support
from test.support import import_helper


# skip tests if the _ctypes extension was not built
import_helper.import_module('ctypes')


class TestModule(unittest.TestCase):
    def test_deprecated__version__(self):
        import ctypes
        import _ctypes

        for mod in (ctypes, _ctypes):
            with self.subTest(mod=mod):
                with self.assertWarnsRegex(
                    DeprecationWarning,
                    "'__version__' is deprecated and slated for removal in Python 3.20",
                ) as cm:
                    getattr(mod, "__version__")
                self.assertEqual(cm.filename, __file__)


def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
