"""Test reload support.

Reload support requires two things. One is that if module is loaded that
already exists in sys.modules then it is reused. And two, if a reload fails the
pre-existing module is left in a sane state.

"""
import imp
import sys
import types
import unittest
import importlib
from .. import support


class ReloadTests(unittest.TestCase):

    name = '_temp'

    def load_module(self, mapping):
        return importlib._PyFileLoader(self.name, mapping[self.name], False)

    def fake_mtime(self, fxn):
        """Fake mtime to always be higher than expected."""
        return lambda name: fxn(name) + 1

    def test_module_reuse(self):
        with support.create_modules(self.name) as mapping:
            loader = self.load_module(mapping)
            module = loader.load_module(self.name)
            module_id = id(module)
            module_dict_id = id(module.__dict__)
            with open(mapping[self.name], 'w') as file:
                file.write("testing_var = 42\n")
            # For filesystems where the mtime is only to a second granularity,
            # everything that has happened above can be too fast;
            # force an mtime on the source that is guaranteed to be different
            # than the original mtime.
            loader.source_mtime = self.fake_mtime(loader.source_mtime)
            module = loader.load_module(self.name)
            self.assert_('testing_var' in module.__dict__,
                         "'testing_var' not in "
                            "{0}".format(list(module.__dict__.keys())))
            self.assertEqual(module, sys.modules[self.name])
            self.assertEqual(id(module), module_id)
            self.assertEqual(id(module.__dict__), module_dict_id)

    def test_bad_reload(self):
        # A failed reload should leave the original module intact.
        attributes = ('__file__', '__path__', '__package__')
        value = '<test>'
        with support.create_modules(self.name) as mapping:
            orig_module = imp.new_module(self.name)
            for attr in attributes:
                setattr(orig_module, attr, value)
            with open(mapping[self.name], 'w') as file:
                file.write('+++ bad syntax +++')
            loader = self.load_module(mapping)
            self.assertRaises(SyntaxError, loader.load_module, self.name)
            for attr in attributes:
                self.assertEqual(getattr(orig_module, attr), value)



def test_main():
    from test.support import run_unittest
    run_unittest(ReloadTests)


if __name__ == '__main__':
    test_main()
