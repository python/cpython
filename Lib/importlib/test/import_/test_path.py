from ..support import (mock_modules, import_state, import_, mock_path_hook,
                        importlib_only, uncache)

from contextlib import nested
from imp import new_module
import sys
from types import MethodType
import unittest


class BaseTests(unittest.TestCase):

    """When sys.meta_path cannot find the desired module, sys.path is
    consulted. For each entry on the sequence [order], sys.path_importer_cache
    is checked to see if it contains a key for the entry [cache check]. If an
    importer is found then it is consulted before trying the next entry in
    sys.path [cache use]. The 'path' argument to find_module() is never used
    when trying to find a module [path not used].

    If an entry from sys.path is not in sys.path_importer_cache, sys.path_hooks
    is called in turn [hooks order]. If a path hook cannot handle an entry,
    ImportError is raised [hook failure]. Otherwise the resulting object is
    cached in sys.path_importer_cache and then consulted [hook success]. If no
    hook is found, None is set in sys.path_importer_cache and the default
    importer is tried [no hook].

    For use of __path__ in a package, the above is all true, just substitute
    "sys.path" for "__path__".

    """

    def order_test(self, to_import, entry, search_path, path=[]):
        # [order]
        log = []
        class LogFindModule(mock_modules):
            def find_module(self, fullname):
                log.append(self)
                return super().find_module(fullname)

        assert len(search_path) == 2
        misser = LogFindModule(search_path[0])
        hitter = LogFindModule(to_import)
        with nested(misser, hitter):
            cache = dict(zip(search_path, (misser, hitter)))
            with import_state(path=path, path_importer_cache=cache):
                import_(to_import)
        self.assertEquals(log[0], misser)
        self.assertEquals(log[1], hitter)

    @importlib_only  # __import__ uses PyDict_GetItem(), bypassing log.
    def cache_use_test(self, to_import, entry, path=[]):
        # [cache check], [cache use]
        log = []
        class LoggingDict(dict):
            def __getitem__(self, item):
                log.append(item)
                return super(LoggingDict, self).__getitem__(item)

        with mock_modules(to_import) as importer:
            cache = LoggingDict()
            cache[entry] = importer
            with import_state(path=[entry], path_importer_cache=cache):
                module = import_(to_import, fromlist=['a'])
            self.assert_(module is importer[to_import])
        self.assertEquals(len(cache), 1)
        self.assertEquals([entry], log)

    def hooks_order_test(self, to_import, entry, path=[]):
        # [hooks order], [hooks failure], [hook success]
        log = []
        def logging_hook(entry):
            log.append(entry)
            raise ImportError
        with mock_modules(to_import) as importer:
            hitter = mock_path_hook(entry, importer=importer)
            path_hooks = [logging_hook, logging_hook, hitter]
            with import_state(path_hooks=path_hooks, path=path):
                import_(to_import)
                self.assertEquals(sys.path_importer_cache[entry], importer)
        self.assertEquals(len(log), 2)

    # [no hook] XXX Worry about after deciding how to handle the default hook.

    def path_argument_test(self, to_import):
        # [path not used]
        class BadImporter:
            """Class to help detect TypeError from calling find_module() with
            an improper number of arguments."""
            def find_module(name):
                raise ImportError

        try:
            import_(to_import)
        except ImportError:
            pass


class PathTests(BaseTests):

    """Tests for sys.path."""

    def test_order(self):
        self.order_test('hit', 'second', ['first', 'second'],
                        ['first', 'second'])

    def test_cache_use(self):
        entry = "found!"
        self.cache_use_test('hit', entry, [entry])

    def test_hooks_order(self):
        entry = "found!"
        self.hooks_order_test('hit', entry, [entry])

    def test_path_argument(self):
        name = 'total junk'
        with uncache(name):
            self.path_argument_test(name)


class __path__Tests(BaseTests):

    """Tests for __path__."""

    def run_test(self, test, entry, path, *args):
        with mock_modules('pkg.__init__') as importer:
            importer['pkg'].__path__ = path
            importer.load_module('pkg')
            test('pkg.hit', entry, *args)


    @importlib_only  # XXX Unknown reason why this fails.
    def test_order(self):
        self.run_test(self.order_test, 'second', ('first', 'second'), ['first',
            'second'])

    def test_cache_use(self):
        location = "I'm here!"
        self.run_test(self.cache_use_test, location, [location])

    def test_hooks_order(self):
        location = "I'm here!"
        self.run_test(self.hooks_order_test, location, [location])

    def test_path_argument(self):
        module = new_module('pkg')
        module.__path__ = ['random __path__']
        name = 'pkg.whatever'
        sys.modules['pkg'] = module
        with uncache('pkg', name):
            self.path_argument_test(name)


def test_main():
    from test.support import run_unittest
    run_unittest(PathTests, __path__Tests)

if __name__ == '__main__':
    test_main()
