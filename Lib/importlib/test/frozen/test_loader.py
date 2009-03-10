from importlib import machinery
from .. import abc
from .. import util


class LoaderTests(abc.LoaderTests):

    def test_module(self):
        with util.uncache('__hello__'):
            module = machinery.FrozenImporter.load_module('__hello__')
            check = {'__name__': '__hello__', '__file__': '<frozen>',
                    '__package__': '', '__loader__': machinery.FrozenImporter}
            for attr, value in check.items():
                self.assertEqual(getattr(module, attr), value)

    def test_package(self):
        with util.uncache('__phello__'):
            module = machinery.FrozenImporter.load_module('__phello__')
            check = {'__name__': '__phello__', '__file__': '<frozen>',
                     '__package__': '__phello__', '__path__': ['__phello__'],
                     '__loader__': machinery.FrozenImporter}
            for attr, value in check.items():
                attr_value = getattr(module, attr)
                self.assertEqual(attr_value, value,
                                 "for __phello__.%s, %r != %r" %
                                 (attr, attr_value, value))

    def test_lacking_parent(self):
        with util.uncache('__phello__', '__phello__.spam'):
            module = machinery.FrozenImporter.load_module('__phello__.spam')
            check = {'__name__': '__phello__.spam', '__file__': '<frozen>',
                    '__package__': '__phello__',
                    '__loader__': machinery.FrozenImporter}
            for attr, value in check.items():
                attr_value = getattr(module, attr)
                self.assertEqual(attr_value, value,
                                 "for __phello__.spam.%s, %r != %r" %
                                 (attr, attr_value, value))

    def test_module_reuse(self):
        with util.uncache('__hello__'):
            module1 = machinery.FrozenImporter.load_module('__hello__')
            module2 = machinery.FrozenImporter.load_module('__hello__')
            self.assert_(module1 is module2)

    def test_state_after_failure(self):
        # No way to trigger an error in a frozen module.
        pass

    def test_unloadable(self):
        assert machinery.FrozenImporter.find_module('_not_real') is None
        self.assertRaises(ImportError, machinery.FrozenImporter.load_module,
                            '_not_real')


def test_main():
    from test.support import run_unittest
    run_unittest(LoaderTests)


if __name__ == '__main__':
    test_main()
