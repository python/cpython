import importlib
from .. import support

import sys
import types
import unittest


class LoaderTests(unittest.TestCase):

    """Test load_module() for built-in modules."""

    assert 'errno' in sys.builtin_module_names
    name = 'errno'

    verification = {'__name__': 'errno', '__package__': None}

    def verify(self, module):
        """Verify that the module matches against what it should have."""
        self.assert_(isinstance(module, types.ModuleType))
        for attr, value in self.verification.items():
            self.assertEqual(getattr(module, attr), value)
        self.assert_(module.__name__ in sys.modules)

    load_module = staticmethod(lambda name:
                                importlib.BuiltinImporter().load_module(name))

    def test_load_module(self):
        # Common case.
        with support.uncache(self.name):
            module = self.load_module(self.name)
            self.verify(module)

    def test_nonexistent(self):
        name = 'dssdsdfff'
        assert name not in sys.builtin_module_names
        self.assertRaises(ImportError, self.load_module, name)

    def test_already_imported(self):
        # Using the name of a module already imported but not a built-in should
        # still fail.
        assert hasattr(importlib, '__file__')  # Not a built-in.
        self.assertRaises(ImportError, self.load_module, 'importlib')


def test_main():
    from test.support import run_unittest
    run_unittest(LoaderTests)


if __name__ == '__main__':
    test_main()
