from importlib import machinery
import imp
import unittest
from .. import abc
from .. import util
from test.support import captured_stdout

class LoaderTests(abc.LoaderTests):

    def test_module(self):
        with util.uncache('__hello__'), captured_stdout() as stdout:
            module = machinery.FrozenImporter.load_module('__hello__')
            check = {'__name__': '__hello__',
                    '__package__': '',
                    '__loader__': machinery.FrozenImporter,
                    }
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)
            self.assertEqual(stdout.getvalue(), 'Hello world!\n')
            self.assertFalse(hasattr(module, '__file__'))

    def test_package(self):
        with util.uncache('__phello__'),  captured_stdout() as stdout:
            module = machinery.FrozenImporter.load_module('__phello__')
            check = {'__name__': '__phello__',
                     '__package__': '__phello__',
                     '__path__': ['__phello__'],
                     '__loader__': machinery.FrozenImporter,
                     }
            for attr, value in check.items():
                attr_value = getattr(module, attr)
                self.assertEqual(attr_value, value,
                                 "for __phello__.%s, %r != %r" %
                                 (attr, attr_value, value))
            self.assertEqual(stdout.getvalue(), 'Hello world!\n')
            self.assertFalse(hasattr(module, '__file__'))

    def test_lacking_parent(self):
        with util.uncache('__phello__', '__phello__.spam'), \
             captured_stdout() as stdout:
            module = machinery.FrozenImporter.load_module('__phello__.spam')
            check = {'__name__': '__phello__.spam',
                    '__package__': '__phello__',
                    '__loader__': machinery.FrozenImporter,
                    }
            for attr, value in check.items():
                attr_value = getattr(module, attr)
                self.assertEqual(attr_value, value,
                                 "for __phello__.spam.%s, %r != %r" %
                                 (attr, attr_value, value))
            self.assertEqual(stdout.getvalue(), 'Hello world!\n')
            self.assertFalse(hasattr(module, '__file__'))

    def test_module_reuse(self):
        with util.uncache('__hello__'), captured_stdout() as stdout:
            module1 = machinery.FrozenImporter.load_module('__hello__')
            module2 = machinery.FrozenImporter.load_module('__hello__')
            self.assertIs(module1, module2)
            self.assertEqual(stdout.getvalue(),
                             'Hello world!\nHello world!\n')

    def test_module_repr(self):
        with util.uncache('__hello__'), captured_stdout():
            module = machinery.FrozenImporter.load_module('__hello__')
            self.assertEqual(repr(module),
                             "<module '__hello__' (frozen)>")

    def test_state_after_failure(self):
        # No way to trigger an error in a frozen module.
        pass

    def test_unloadable(self):
        assert machinery.FrozenImporter.find_module('_not_real') is None
        with self.assertRaises(ImportError) as cm:
            machinery.FrozenImporter.load_module('_not_real')
        self.assertEqual(cm.exception.name, '_not_real')


class InspectLoaderTests(unittest.TestCase):

    """Tests for the InspectLoader methods for FrozenImporter."""

    def test_get_code(self):
        # Make sure that the code object is good.
        name = '__hello__'
        with captured_stdout() as stdout:
            code = machinery.FrozenImporter.get_code(name)
            mod = imp.new_module(name)
            exec(code, mod.__dict__)
            self.assertTrue(hasattr(mod, 'initialized'))
            self.assertEqual(stdout.getvalue(), 'Hello world!\n')

    def test_get_source(self):
        # Should always return None.
        result = machinery.FrozenImporter.get_source('__hello__')
        self.assertIs(result, None)

    def test_is_package(self):
        # Should be able to tell what is a package.
        test_for = (('__hello__', False), ('__phello__', True),
                    ('__phello__.spam', False))
        for name, is_package in test_for:
            result = machinery.FrozenImporter.is_package(name)
            self.assertEqual(bool(result), is_package)

    def test_failure(self):
        # Raise ImportError for modules that are not frozen.
        for meth_name in ('get_code', 'get_source', 'is_package'):
            method = getattr(machinery.FrozenImporter, meth_name)
            with self.assertRaises(ImportError) as cm:
                method('importlib')
            self.assertEqual(cm.exception.name, 'importlib')


def test_main():
    from test.support import run_unittest
    run_unittest(LoaderTests, InspectLoaderTests)


if __name__ == '__main__':
    test_main()
