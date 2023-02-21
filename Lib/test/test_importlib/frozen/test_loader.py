from test.test_importlib import abc, util

machinery = util.import_importlib('importlib.machinery')

from test.support import captured_stdout, import_helper, STDLIB_DIR
import _imp
import contextlib
import marshal
import os.path
import types
import unittest
import warnings


@contextlib.contextmanager
def deprecated():
    with warnings.catch_warnings():
        warnings.simplefilter('ignore', DeprecationWarning)
        yield


@contextlib.contextmanager
def fresh(name, *, oldapi=False):
    with util.uncache(name):
        with import_helper.frozen_modules():
            if oldapi:
                with deprecated():
                    yield
            else:
                yield


def resolve_stdlib_file(name, ispkg=False):
    assert name
    if ispkg:
        return os.path.join(STDLIB_DIR, *name.split('.'), '__init__.py')
    else:
        return os.path.join(STDLIB_DIR, *name.split('.')) + '.py'


class ExecModuleTests(abc.LoaderTests):

    def exec_module(self, name, origname=None):
        with import_helper.frozen_modules():
            is_package = self.machinery.FrozenImporter.is_package(name)
        spec = self.machinery.ModuleSpec(
            name,
            self.machinery.FrozenImporter,
            origin='frozen',
            is_package=is_package,
            loader_state=types.SimpleNamespace(
                origname=origname or name,
                filename=resolve_stdlib_file(origname or name, is_package),
            ),
        )
        module = types.ModuleType(name)
        module.__spec__ = spec
        assert not hasattr(module, 'initialized')

        with fresh(name):
            self.machinery.FrozenImporter.exec_module(module)
        with captured_stdout() as stdout:
            module.main()

        self.assertTrue(module.initialized)
        self.assertTrue(hasattr(module, '__spec__'))
        self.assertEqual(module.__spec__.origin, 'frozen')
        return module, stdout.getvalue()

    def test_module(self):
        name = '__hello__'
        module, output = self.exec_module(name)
        check = {'__name__': name}
        for attr, value in check.items():
            self.assertEqual(getattr(module, attr), value)
        self.assertEqual(output, 'Hello world!\n')
        self.assertTrue(hasattr(module, '__spec__'))
        self.assertEqual(module.__spec__.loader_state.origname, name)

    def test_package(self):
        name = '__phello__'
        module, output = self.exec_module(name)
        check = {'__name__': name}
        for attr, value in check.items():
            attr_value = getattr(module, attr)
            self.assertEqual(attr_value, value,
                        'for {name}.{attr}, {given!r} != {expected!r}'.format(
                                 name=name, attr=attr, given=attr_value,
                                 expected=value))
        self.assertEqual(output, 'Hello world!\n')
        self.assertEqual(module.__spec__.loader_state.origname, name)

    def test_lacking_parent(self):
        name = '__phello__.spam'
        with util.uncache('__phello__'):
            module, output = self.exec_module(name)
        check = {'__name__': name}
        for attr, value in check.items():
            attr_value = getattr(module, attr)
            self.assertEqual(attr_value, value,
                    'for {name}.{attr}, {given} != {expected!r}'.format(
                             name=name, attr=attr, given=attr_value,
                             expected=value))
        self.assertEqual(output, 'Hello world!\n')

    def test_module_repr_indirect_through_spec(self):
        name = '__hello__'
        module, output = self.exec_module(name)
        self.assertEqual(repr(module),
                         "<module '__hello__' (frozen)>")

    # No way to trigger an error in a frozen module.
    test_state_after_failure = None

    def test_unloadable(self):
        with import_helper.frozen_modules():
            assert self.machinery.FrozenImporter.find_spec('_not_real') is None
        with self.assertRaises(ImportError) as cm:
            self.exec_module('_not_real')
        self.assertEqual(cm.exception.name, '_not_real')


(Frozen_ExecModuleTests,
 Source_ExecModuleTests
 ) = util.test_both(ExecModuleTests, machinery=machinery)


class LoaderTests(abc.LoaderTests):

    def load_module(self, name):
        with fresh(name, oldapi=True):
            module = self.machinery.FrozenImporter.load_module(name)
        with captured_stdout() as stdout:
            module.main()
        return module, stdout

    def test_module(self):
        module, stdout = self.load_module('__hello__')
        filename = resolve_stdlib_file('__hello__')
        check = {'__name__': '__hello__',
                '__package__': '',
                '__loader__': self.machinery.FrozenImporter,
                '__file__': filename,
                }
        for attr, value in check.items():
            self.assertEqual(getattr(module, attr, None), value)
        self.assertEqual(stdout.getvalue(), 'Hello world!\n')

    def test_package(self):
        module, stdout = self.load_module('__phello__')
        filename = resolve_stdlib_file('__phello__', ispkg=True)
        pkgdir = os.path.dirname(filename)
        check = {'__name__': '__phello__',
                 '__package__': '__phello__',
                 '__path__': [pkgdir],
                 '__loader__': self.machinery.FrozenImporter,
                 '__file__': filename,
                 }
        for attr, value in check.items():
            attr_value = getattr(module, attr, None)
            self.assertEqual(attr_value, value,
                             "for __phello__.%s, %r != %r" %
                             (attr, attr_value, value))
        self.assertEqual(stdout.getvalue(), 'Hello world!\n')

    def test_lacking_parent(self):
        with util.uncache('__phello__'):
            module, stdout = self.load_module('__phello__.spam')
        filename = resolve_stdlib_file('__phello__.spam')
        check = {'__name__': '__phello__.spam',
                '__package__': '__phello__',
                '__loader__': self.machinery.FrozenImporter,
                '__file__': filename,
                }
        for attr, value in check.items():
            attr_value = getattr(module, attr)
            self.assertEqual(attr_value, value,
                             "for __phello__.spam.%s, %r != %r" %
                             (attr, attr_value, value))
        self.assertEqual(stdout.getvalue(), 'Hello world!\n')

    def test_module_reuse(self):
        with fresh('__hello__', oldapi=True):
            module1 = self.machinery.FrozenImporter.load_module('__hello__')
            module2 = self.machinery.FrozenImporter.load_module('__hello__')
        with captured_stdout() as stdout:
            module1.main()
            module2.main()
        self.assertIs(module1, module2)
        self.assertEqual(stdout.getvalue(),
                         'Hello world!\nHello world!\n')

    # No way to trigger an error in a frozen module.
    test_state_after_failure = None

    def test_unloadable(self):
        with import_helper.frozen_modules():
            with deprecated():
                assert self.machinery.FrozenImporter.find_module('_not_real') is None
            with self.assertRaises(ImportError) as cm:
                self.load_module('_not_real')
            self.assertEqual(cm.exception.name, '_not_real')


(Frozen_LoaderTests,
 Source_LoaderTests
 ) = util.test_both(LoaderTests, machinery=machinery)


class InspectLoaderTests:

    """Tests for the InspectLoader methods for FrozenImporter."""

    def test_get_code(self):
        # Make sure that the code object is good.
        name = '__hello__'
        with import_helper.frozen_modules():
            code = self.machinery.FrozenImporter.get_code(name)
            mod = types.ModuleType(name)
            exec(code, mod.__dict__)
        with captured_stdout() as stdout:
            mod.main()
        self.assertTrue(hasattr(mod, 'initialized'))
        self.assertEqual(stdout.getvalue(), 'Hello world!\n')

    def test_get_source(self):
        # Should always return None.
        with import_helper.frozen_modules():
            result = self.machinery.FrozenImporter.get_source('__hello__')
        self.assertIsNone(result)

    def test_is_package(self):
        # Should be able to tell what is a package.
        test_for = (('__hello__', False), ('__phello__', True),
                    ('__phello__.spam', False))
        for name, is_package in test_for:
            with import_helper.frozen_modules():
                result = self.machinery.FrozenImporter.is_package(name)
            self.assertEqual(bool(result), is_package)

    def test_failure(self):
        # Raise ImportError for modules that are not frozen.
        for meth_name in ('get_code', 'get_source', 'is_package'):
            method = getattr(self.machinery.FrozenImporter, meth_name)
            with self.assertRaises(ImportError) as cm:
                with import_helper.frozen_modules():
                    method('importlib')
            self.assertEqual(cm.exception.name, 'importlib')

(Frozen_ILTests,
 Source_ILTests
 ) = util.test_both(InspectLoaderTests, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
