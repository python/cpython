from unittest.mock import Mock, patch

from test.test_importlib import util

importlib = util.import_importlib('importlib')
machinery = util.import_importlib('importlib.machinery')


class DiscoverableFinder:
    def __init__(self, discover=[]):
        self._discovered_values = discover

    def find_spec(self, fullname, path=None, target=None):
        raise NotImplementedError

    def discover(self, parent=None):
        yield from self._discovered_values


class TestPathFinder:
    """PathFinder implements MetaPathFinder, which uses the PathEntryFinder(s)
    registered in sys.path_hooks (and sys.path_importer_cache) to search
    sys.path or the parent's __path__.

    PathFinder.discover() should redirect to the .discover() method of the
    PathEntryFinder for each path entry.
    """

    def test_search_path_hooks_top_level(self):
        modules = [
            self.machinery.ModuleSpec(name='example1', loader=None),
            self.machinery.ModuleSpec(name='example2', loader=None),
            self.machinery.ModuleSpec(name='example3', loader=None),
        ]

        with util.import_state(
            path_importer_cache={
                'discoverable': DiscoverableFinder(discover=modules),
            },
            path=['discoverable'],
        ):
            discovered = list(self.machinery.PathFinder.discover())

        self.assertEqual(discovered, modules)


    def test_search_path_hooks_parent(self):
        parent = self.machinery.ModuleSpec(name='example', loader=None, is_package=True)
        parent.submodule_search_locations.append('discoverable')

        children = [
            self.machinery.ModuleSpec(name='example.child1', loader=None),
            self.machinery.ModuleSpec(name='example.child2', loader=None),
            self.machinery.ModuleSpec(name='example.child3', loader=None),
        ]

        with util.import_state(
            path_importer_cache={
                'discoverable': DiscoverableFinder(discover=children)
            },
            path=[],
        ):
            discovered = list(self.machinery.PathFinder.discover(parent))

        self.assertEqual(discovered, children)

    def test_invalid_parent(self):
        parent = self.machinery.ModuleSpec(name='example', loader=None)
        with self.assertRaises(ValueError):
            list(self.machinery.PathFinder.discover(parent))


(
    Frozen_TestPathFinder,
    Source_TestPathFinder,
) = util.test_both(TestPathFinder, importlib=importlib, machinery=machinery)


class TestFileFinder:
    """FileFinder implements PathEntryFinder and provides the base finder
    implementation to search the file system.
    """

    def get_finder(self, path):
        loader_details = [
            (self.machinery.SourceFileLoader, self.machinery.SOURCE_SUFFIXES),
            (self.machinery.SourcelessFileLoader, self.machinery.BYTECODE_SUFFIXES),
        ]
        return self.machinery.FileFinder(path, *loader_details)

    def test_discover_top_level(self):
        modules = {'example1', 'example2', 'example3'}
        with util.create_modules(*modules) as mapping:
            finder = self.get_finder(mapping['.root'])
            discovered = list(finder.discover())
        self.assertEqual({spec.name for spec in discovered}, modules)

    def test_discover_parent(self):
        modules = {
            'example.child1',
            'example.child2',
            'example.child3',
        }
        with util.create_modules(*modules) as mapping:
            example = self.get_finder(mapping['.root']).find_spec('example')
            finder = self.get_finder(example.submodule_search_locations[0])
            discovered = list(finder.discover(example))
        self.assertEqual({spec.name for spec in discovered}, modules)

    def test_invalid_parent(self):
        with util.create_modules('example') as mapping:
            finder = self.get_finder(mapping['.root'])
            example = finder.find_spec('example')
            with self.assertRaises(ValueError):
                list(finder.discover(example))

    def _patch_scandir(self, scandir):
        module_os = self.machinery.FileFinder._fill_cache.__globals__['_os']
        return patch.object(module_os, 'scandir', scandir)

    def test_discover_persistently_failing_scan(self):
        # gh-155935: an iterator that raises OSError on every next() call
        # must end the listing instead of looping forever.
        class FailingScandirIterator:
            calls = 0

            def __enter__(self):
                return self

            def __exit__(self, *args):
                return False

            def __next__(self):
                self.calls += 1
                if self.calls > 100:
                    # Safety net so regressed code fails fast on the call
                    # count below instead of hanging the test forever.
                    raise StopIteration
                raise OSError('persistently failing directory scan')

        scan_iterator = FailingScandirIterator()
        with self._patch_scandir(lambda path: scan_iterator):
            finder = self.get_finder('dummy')
            discovered = list(finder.discover())
        self.assertEqual(discovered, [])
        # A failed scan must not be retried.
        self.assertEqual(scan_iterator.calls, 1)

    def test_find_children_failing_direntry(self):
        # An entry whose DirEntry methods raise OSError is skipped; the
        # remaining entries are still listed.
        failing = Mock()
        failing.name = 'failing'
        failing.is_dir.side_effect = OSError('stat failed')
        good = Mock()
        good.name = 'example.py'
        good.is_dir.return_value = False
        good.is_file.return_value = True

        class FakeScandirIterator:
            def __init__(self, entries):
                self._iterator = iter(entries)

            def __enter__(self):
                return self

            def __exit__(self, *args):
                return False

            def __next__(self):
                return next(self._iterator)

        with self._patch_scandir(
                lambda path: FakeScandirIterator([failing, good])):
            finder = self.get_finder('dummy')
            children = list(finder._find_children())
        self.assertEqual(children, ['example'])


(
    Frozen_TestFileFinder,
    Source_TestFileFinder,
) = util.test_both(TestFileFinder, importlib=importlib, machinery=machinery)
