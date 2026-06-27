import importlib
from importlib import abc
from importlib import util
import sys
import time
import threading
import types
import unittest

from test.support import threading_helper
from test.test_importlib import util as test_util


class CollectInit:

    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs

    def exec_module(self, module):
        return self


class LazyLoaderFactoryTests(unittest.TestCase):

    def test_init(self):
        factory = util.LazyLoader.factory(CollectInit)
        # E.g. what importlib.machinery.FileFinder instantiates loaders with
        # plus keyword arguments.
        lazy_loader = factory('module name', 'module path', kw='kw')
        loader = lazy_loader.loader
        self.assertEqual(('module name', 'module path'), loader.args)
        self.assertEqual({'kw': 'kw'}, loader.kwargs)

    def test_validation(self):
        # No exec_module(), no lazy loading.
        with self.assertRaises(TypeError):
            util.LazyLoader.factory(object)


class TestingImporter(abc.MetaPathFinder, abc.Loader):

    module_name = 'lazy_loader_test'
    mutated_name = 'changed'
    loaded = None
    load_count = 0
    source_code = 'attr = 42; __name__ = {!r}'.format(mutated_name)

    def find_spec(self, name, path, target=None):
        if name != self.module_name:
            return None
        return util.spec_from_loader(name, util.LazyLoader(self))

    def exec_module(self, module):
        time.sleep(0.01)  # Simulate a slow load.
        exec(self.source_code, module.__dict__)
        self.loaded = module
        self.load_count += 1


class LazyLoaderTests(unittest.TestCase):

    def test_init(self):
        with self.assertRaises(TypeError):
            # Classes that don't define exec_module() trigger TypeError.
            util.LazyLoader(object)

    def new_module(self, source_code=None, loader=None):
        if loader is None:
            loader = TestingImporter()
        if source_code is not None:
            loader.source_code = source_code
        spec = util.spec_from_loader(TestingImporter.module_name,
                                     util.LazyLoader(loader))
        module = spec.loader.create_module(spec)
        if module is None:
            module = types.ModuleType(TestingImporter.module_name)
        module.__spec__ = spec
        module.__loader__ = spec.loader
        spec.loader.exec_module(module)
        # Module is now lazy.
        self.assertIsNone(loader.loaded)
        return module

    def test_e2e(self):
        # End-to-end test to verify the load is in fact lazy.
        importer = TestingImporter()
        assert importer.loaded is None
        with test_util.uncache(importer.module_name):
            with test_util.import_state(meta_path=[importer]):
                module = importlib.import_module(importer.module_name)
        self.assertIsNone(importer.loaded)
        # Trigger load.
        self.assertEqual(module.__loader__, importer)
        self.assertIsNotNone(importer.loaded)
        self.assertEqual(module, importer.loaded)

    def test_attr_unchanged(self):
        # An attribute only mutated as a side-effect of import should not be
        # changed needlessly.
        module = self.new_module()
        self.assertEqual(TestingImporter.mutated_name, module.__name__)

    def test_new_attr(self):
        # A new attribute should persist.
        module = self.new_module()
        module.new_attr = 42
        self.assertEqual(42, module.new_attr)

    def test_mutated_preexisting_attr(self):
        # Changing an attribute that already existed on the module --
        # e.g. __name__ -- should persist.
        module = self.new_module()
        module.__name__ = 'bogus'
        self.assertEqual('bogus', module.__name__)

    def test_mutated_attr(self):
        # Changing an attribute that comes into existence after an import
        # should persist.
        module = self.new_module()
        module.attr = 6
        self.assertEqual(6, module.attr)

    def test_delete_eventual_attr(self):
        # Deleting an attribute should stay deleted.
        module = self.new_module()
        del module.attr
        self.assertNotHasAttr(module, 'attr')

    def test_delete_preexisting_attr(self):
        module = self.new_module()
        del module.__name__
        self.assertNotHasAttr(module, '__name__')

    def test_module_substitution_error(self):
        with test_util.uncache(TestingImporter.module_name):
            fresh_module = types.ModuleType(TestingImporter.module_name)
            sys.modules[TestingImporter.module_name] = fresh_module
            module = self.new_module()
            with self.assertRaisesRegex(ValueError, "substituted"):
                module.__name__

    def test_module_already_in_sys(self):
        with test_util.uncache(TestingImporter.module_name):
            module = self.new_module()
            sys.modules[TestingImporter.module_name] = module
            # Force the load; just care that no exception is raised.
            module.__name__

    @threading_helper.requires_working_threading()
    def test_module_load_race(self):
        with test_util.uncache(TestingImporter.module_name):
            loader = TestingImporter()
            module = self.new_module(loader=loader)
            self.assertEqual(loader.load_count, 0)

            class RaisingThread(threading.Thread):
                exc = None
                def run(self):
                    try:
                        super().run()
                    except Exception as exc:
                        self.exc = exc

            def access_module():
                return module.attr

            threads = []
            for _ in range(2):
                threads.append(thread := RaisingThread(target=access_module))
                thread.start()

            # Races could cause errors
            for thread in threads:
                thread.join()
                self.assertIsNone(thread.exc)

            # Or multiple load attempts
            self.assertEqual(loader.load_count, 1)

    def test_lazy_self_referential_modules(self):
        # Directory modules with submodules that reference the parent can attempt to access
        # the parent module during a load. Verify that this common pattern works with lazy loading.
        # json is a good example in the stdlib.
        json_modules = [name for name in sys.modules if name.startswith('json')]
        with test_util.uncache(*json_modules):
            # Standard lazy loading, unwrapped
            spec = util.find_spec('json')
            loader = util.LazyLoader(spec.loader)
            spec.loader = loader
            module = util.module_from_spec(spec)
            sys.modules['json'] = module
            loader.exec_module(module)

            # Trigger load with attribute lookup, ensure expected behavior
            test_load = module.loads('{}')
            self.assertEqual(test_load, {})

    def test_lazy_module_type_override(self):
        # Verify that lazy loading works with a module that modifies
        # its __class__ to be a custom type.

        # Example module from PEP 726
        module = self.new_module(source_code="""\
import sys
from types import ModuleType

CONSTANT = 3.14

class ImmutableModule(ModuleType):
    def __setattr__(self, name, value):
        raise AttributeError('Read-only attribute!')

    def __delattr__(self, name):
        raise AttributeError('Read-only attribute!')

sys.modules[__name__].__class__ = ImmutableModule
""")
        sys.modules[TestingImporter.module_name] = module
        self.assertIsInstance(module, util._LazyModule)
        self.assertEqual(module.CONSTANT, 3.14)
        with self.assertRaises(AttributeError):
            module.CONSTANT = 2.71
        with self.assertRaises(AttributeError):
            del module.CONSTANT


if __name__ == '__main__':
    unittest.main()
