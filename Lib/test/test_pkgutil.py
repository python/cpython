from pathlib import Path
from test.support.import_helper import unload, CleanImport
from test.support.warnings_helper import check_warnings, ignore_warnings
import unittest
import sys
import importlib
from importlib.util import spec_from_file_location
import pkgutil
import os
import os.path
import tempfile
import shutil
import zipfile

from test.support.import_helper import DirsOnSysPath
from test.support.os_helper import FakePath
from test.test_importlib.util import uncache

# Note: pkgutil.walk_packages is currently tested in test_runpy. This is
# a hack to get a major issue resolved for 3.3b2. Longer term, it should
# be moved back here, perhaps by factoring out the helper code for
# creating interesting package layouts to a separate module.
# Issue #15348 declares this is indeed a dodgy hack ;)

class PkgutilTests(unittest.TestCase):

    def setUp(self):
        self.dirname = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.dirname)
        sys.path.insert(0, self.dirname)

    def tearDown(self):
        del sys.path[0]

    def test_getdata_filesys(self):
        pkg = 'test_getdata_filesys'

        # Include a LF and a CRLF, to test that binary data is read back
        RESOURCE_DATA = b'Hello, world!\nSecond line\r\nThird line'

        # Make a package with some resources
        package_dir = os.path.join(self.dirname, pkg)
        os.mkdir(package_dir)
        # Empty init.py
        f = open(os.path.join(package_dir, '__init__.py'), "wb")
        f.close()
        # Resource files, res.txt, sub/res.txt
        f = open(os.path.join(package_dir, 'res.txt'), "wb")
        f.write(RESOURCE_DATA)
        f.close()
        os.mkdir(os.path.join(package_dir, 'sub'))
        f = open(os.path.join(package_dir, 'sub', 'res.txt'), "wb")
        f.write(RESOURCE_DATA)
        f.close()

        # Check we can read the resources
        res1 = pkgutil.get_data(pkg, 'res.txt')
        self.assertEqual(res1, RESOURCE_DATA)
        res2 = pkgutil.get_data(pkg, 'sub/res.txt')
        self.assertEqual(res2, RESOURCE_DATA)

        del sys.modules[pkg]

    def test_getdata_zipfile(self):
        zip = 'test_getdata_zipfile.zip'
        pkg = 'test_getdata_zipfile'

        # Include a LF and a CRLF, to test that binary data is read back
        RESOURCE_DATA = b'Hello, world!\nSecond line\r\nThird line'

        # Make a package with some resources
        zip_file = os.path.join(self.dirname, zip)
        z = zipfile.ZipFile(zip_file, 'w')

        # Empty init.py
        z.writestr(pkg + '/__init__.py', "")
        # Resource files, res.txt, sub/res.txt
        z.writestr(pkg + '/res.txt', RESOURCE_DATA)
        z.writestr(pkg + '/sub/res.txt', RESOURCE_DATA)
        z.close()

        # Check we can read the resources
        sys.path.insert(0, zip_file)
        res1 = pkgutil.get_data(pkg, 'res.txt')
        self.assertEqual(res1, RESOURCE_DATA)
        res2 = pkgutil.get_data(pkg, 'sub/res.txt')
        self.assertEqual(res2, RESOURCE_DATA)

        names = []
        for moduleinfo in pkgutil.iter_modules([zip_file]):
            self.assertIsInstance(moduleinfo, pkgutil.ModuleInfo)
            names.append(moduleinfo.name)
        self.assertEqual(names, ['test_getdata_zipfile'])

        del sys.path[0]

        del sys.modules[pkg]

    def test_issue44061_iter_modules(self):
        #see: issue44061
        zip = 'test_getdata_zipfile.zip'
        pkg = 'test_getdata_zipfile'

        # Include a LF and a CRLF, to test that binary data is read back
        RESOURCE_DATA = b'Hello, world!\nSecond line\r\nThird line'

        # Make a package with some resources
        zip_file = os.path.join(self.dirname, zip)
        z = zipfile.ZipFile(zip_file, 'w')

        # Empty init.py
        z.writestr(pkg + '/__init__.py', "")
        # Resource files, res.txt
        z.writestr(pkg + '/res.txt', RESOURCE_DATA)
        z.close()

        # Check we can read the resources
        sys.path.insert(0, zip_file)
        try:
            res = pkgutil.get_data(pkg, 'res.txt')
            self.assertEqual(res, RESOURCE_DATA)

            # make sure iter_modules accepts Path objects
            names = []
            for moduleinfo in pkgutil.iter_modules([FakePath(zip_file)]):
                self.assertIsInstance(moduleinfo, pkgutil.ModuleInfo)
                names.append(moduleinfo.name)
            self.assertEqual(names, [pkg])
        finally:
            del sys.path[0]
            sys.modules.pop(pkg, None)

        # assert path must be None or list of paths
        expected_msg = "path must be None or list of paths to look for modules in"
        with self.assertRaisesRegex(ValueError, expected_msg):
            list(pkgutil.iter_modules("invalid_path"))

    def test_unreadable_dir_on_syspath(self):
        # issue7367 - walk_packages failed if unreadable dir on sys.path
        package_name = "unreadable_package"
        d = os.path.join(self.dirname, package_name)
        # this does not appear to create an unreadable dir on Windows
        #   but the test should not fail anyway
        os.mkdir(d, 0)
        self.addCleanup(os.rmdir, d)
        for t in pkgutil.walk_packages(path=[self.dirname]):
            self.fail("unexpected package found")

    def test_walkpackages_filesys(self):
        pkg1 = 'test_walkpackages_filesys'
        pkg1_dir = os.path.join(self.dirname, pkg1)
        os.mkdir(pkg1_dir)
        f = open(os.path.join(pkg1_dir, '__init__.py'), "wb")
        f.close()
        os.mkdir(os.path.join(pkg1_dir, 'sub'))
        f = open(os.path.join(pkg1_dir, 'sub', '__init__.py'), "wb")
        f.close()
        f = open(os.path.join(pkg1_dir, 'sub', 'mod.py'), "wb")
        f.close()

        # Now, to juice it up, let's add the opposite packages, too.
        pkg2 = 'sub'
        pkg2_dir = os.path.join(self.dirname, pkg2)
        os.mkdir(pkg2_dir)
        f = open(os.path.join(pkg2_dir, '__init__.py'), "wb")
        f.close()
        os.mkdir(os.path.join(pkg2_dir, 'test_walkpackages_filesys'))
        f = open(os.path.join(pkg2_dir, 'test_walkpackages_filesys', '__init__.py'), "wb")
        f.close()
        f = open(os.path.join(pkg2_dir, 'test_walkpackages_filesys', 'mod.py'), "wb")
        f.close()

        expected = [
            'sub',
            'sub.test_walkpackages_filesys',
            'sub.test_walkpackages_filesys.mod',
            'test_walkpackages_filesys',
            'test_walkpackages_filesys.sub',
            'test_walkpackages_filesys.sub.mod',
        ]
        actual= [e[1] for e in pkgutil.walk_packages([self.dirname])]
        self.assertEqual(actual, expected)

        for pkg in expected:
            if pkg.endswith('mod'):
                continue
            del sys.modules[pkg]

    def test_walkpackages_zipfile(self):
        """Tests the same as test_walkpackages_filesys, only with a zip file."""

        zip = 'test_walkpackages_zipfile.zip'
        pkg1 = 'test_walkpackages_zipfile'
        pkg2 = 'sub'

        zip_file = os.path.join(self.dirname, zip)
        z = zipfile.ZipFile(zip_file, 'w')
        z.writestr(pkg2 + '/__init__.py', "")
        z.writestr(pkg2 + '/' + pkg1 + '/__init__.py', "")
        z.writestr(pkg2 + '/' + pkg1 + '/mod.py', "")
        z.writestr(pkg1 + '/__init__.py', "")
        z.writestr(pkg1 + '/' + pkg2 + '/__init__.py', "")
        z.writestr(pkg1 + '/' + pkg2 + '/mod.py', "")
        z.close()

        sys.path.insert(0, zip_file)
        expected = [
            'sub',
            'sub.test_walkpackages_zipfile',
            'sub.test_walkpackages_zipfile.mod',
            'test_walkpackages_zipfile',
            'test_walkpackages_zipfile.sub',
            'test_walkpackages_zipfile.sub.mod',
        ]
        actual= [e[1] for e in pkgutil.walk_packages([zip_file])]
        self.assertEqual(actual, expected)
        del sys.path[0]

        for pkg in expected:
            if pkg.endswith('mod'):
                continue
            del sys.modules[pkg]

    def test_walk_packages_raises_on_string_or_bytes_input(self):

        str_input = 'test_dir'
        with self.assertRaises((TypeError, ValueError)):
            list(pkgutil.walk_packages(str_input))

        bytes_input = b'test_dir'
        with self.assertRaises((TypeError, ValueError)):
            list(pkgutil.walk_packages(bytes_input))

    def test_name_resolution(self):
        import logging
        import logging.handlers

        success_cases = (
            ('os', os),
            ('os.path', os.path),
            ('os.path:pathsep', os.path.pathsep),
            ('logging', logging),
            ('logging:', logging),
            ('logging.handlers', logging.handlers),
            ('logging.handlers:', logging.handlers),
            ('logging.handlers:SysLogHandler', logging.handlers.SysLogHandler),
            ('logging.handlers.SysLogHandler', logging.handlers.SysLogHandler),
            ('logging.handlers:SysLogHandler.LOG_ALERT',
                logging.handlers.SysLogHandler.LOG_ALERT),
            ('logging.handlers.SysLogHandler.LOG_ALERT',
                logging.handlers.SysLogHandler.LOG_ALERT),
            ('builtins.int', int),
            ('builtins:int', int),
            ('builtins.int.from_bytes', int.from_bytes),
            ('builtins:int.from_bytes', int.from_bytes),
            ('builtins.ZeroDivisionError', ZeroDivisionError),
            ('builtins:ZeroDivisionError', ZeroDivisionError),
            ('os:path', os.path),
        )

        failure_cases = (
            (None, TypeError),
            (1, TypeError),
            (2.0, TypeError),
            (True, TypeError),
            ('', ValueError),
            ('?abc', ValueError),
            ('abc/foo', ValueError),
            ('foo', ImportError),
            ('os.foo', AttributeError),
            ('os.foo:', ImportError),
            ('os.pth:pathsep', ImportError),
            ('logging.handlers:NoSuchHandler', AttributeError),
            ('logging.handlers:SysLogHandler.NO_SUCH_VALUE', AttributeError),
            ('logging.handlers.SysLogHandler.NO_SUCH_VALUE', AttributeError),
            ('ZeroDivisionError', ImportError),
            ('os.path.9abc', ValueError),
            ('9abc', ValueError),
        )

        # add some Unicode package names to the mix.

        unicode_words = ('\u0935\u092e\u0938',
                         '\xe9', '\xc8',
                         '\uc548\ub155\ud558\uc138\uc694',
                         '\u3055\u3088\u306a\u3089',
                         '\u3042\u308a\u304c\u3068\u3046',
                         '\u0425\u043e\u0440\u043e\u0448\u043e',
                         '\u0441\u043f\u0430\u0441\u0438\u0431\u043e',
                         '\u73b0\u4ee3\u6c49\u8bed\u5e38\u7528\u5b57\u8868')

        for uw in unicode_words:
            d = os.path.join(self.dirname, uw)
            try:
                os.makedirs(d, exist_ok=True)
            except  UnicodeEncodeError:
                # When filesystem encoding cannot encode uw: skip this test
                continue
            # make an empty __init__.py file
            f = os.path.join(d, '__init__.py')
            with open(f, 'w') as f:
                f.write('')
                f.flush()
            # now import the package we just created; clearing the caches is
            # needed, otherwise the newly created package isn't found
            importlib.invalidate_caches()
            mod = importlib.import_module(uw)
            success_cases += (uw, mod),
            if len(uw) > 1:
                failure_cases += (uw[:-1], ImportError),

        # add an example with a Unicode digit at the start
        failure_cases += ('\u0966\u0935\u092e\u0938', ValueError),

        for s, expected in success_cases:
            with self.subTest(s=s):
                o = pkgutil.resolve_name(s)
                self.assertEqual(o, expected)

        for s, exc in failure_cases:
            with self.subTest(s=s):
                with self.assertRaises(exc):
                    pkgutil.resolve_name(s)

    def test_name_resolution_import_rebinding(self):
        # The same data is also used for testing import in test_import and
        # mock.patch in test_unittest.
        path = os.path.join(os.path.dirname(__file__), 'test_import', 'data')
        with uncache('package3', 'package3.submodule'), DirsOnSysPath(path):
            self.assertEqual(pkgutil.resolve_name('package3.submodule.attr'), 'submodule')
        with uncache('package3', 'package3.submodule'), DirsOnSysPath(path):
            self.assertEqual(pkgutil.resolve_name('package3.submodule:attr'), 'submodule')
        with uncache('package3', 'package3.submodule'), DirsOnSysPath(path):
            self.assertEqual(pkgutil.resolve_name('package3:submodule.attr'), 'rebound')
            self.assertEqual(pkgutil.resolve_name('package3.submodule.attr'), 'submodule')
            self.assertEqual(pkgutil.resolve_name('package3:submodule.attr'), 'rebound')
        with uncache('package3', 'package3.submodule'), DirsOnSysPath(path):
            self.assertEqual(pkgutil.resolve_name('package3:submodule.attr'), 'rebound')
            self.assertEqual(pkgutil.resolve_name('package3.submodule:attr'), 'submodule')
            self.assertEqual(pkgutil.resolve_name('package3:submodule.attr'), 'rebound')

    def test_name_resolution_import_rebinding2(self):
        path = os.path.join(os.path.dirname(__file__), 'test_import', 'data')
        with uncache('package4', 'package4.submodule'), DirsOnSysPath(path):
            self.assertEqual(pkgutil.resolve_name('package4.submodule.attr'), 'submodule')
        with uncache('package4', 'package4.submodule'), DirsOnSysPath(path):
            self.assertEqual(pkgutil.resolve_name('package4.submodule:attr'), 'submodule')
        with uncache('package4', 'package4.submodule'), DirsOnSysPath(path):
            self.assertEqual(pkgutil.resolve_name('package4:submodule.attr'), 'origin')
            self.assertEqual(pkgutil.resolve_name('package4.submodule.attr'), 'submodule')
            self.assertEqual(pkgutil.resolve_name('package4:submodule.attr'), 'submodule')
        with uncache('package4', 'package4.submodule'), DirsOnSysPath(path):
            self.assertEqual(pkgutil.resolve_name('package4:submodule.attr'), 'origin')
            self.assertEqual(pkgutil.resolve_name('package4.submodule:attr'), 'submodule')
            self.assertEqual(pkgutil.resolve_name('package4:submodule.attr'), 'submodule')


class PkgutilPEP302Tests(unittest.TestCase):

    class MyTestLoader(object):
        def create_module(self, spec):
            return None

        def exec_module(self, mod):
            # Count how many times the module is reloaded
            mod.__dict__['loads'] = mod.__dict__.get('loads', 0) + 1

        def get_data(self, path):
            return "Hello, world!"

    class MyTestImporter(object):
        def find_spec(self, fullname, path=None, target=None):
            loader = PkgutilPEP302Tests.MyTestLoader()
            return spec_from_file_location(fullname,
                                           '<%s>' % loader.__class__.__name__,
                                           loader=loader,
                                           submodule_search_locations=[])

    def setUp(self):
        sys.meta_path.insert(0, self.MyTestImporter())

    def tearDown(self):
        del sys.meta_path[0]

    def test_getdata_pep302(self):
        # Use a dummy finder/loader
        self.assertEqual(pkgutil.get_data('foo', 'dummy'), "Hello, world!")
        del sys.modules['foo']

    def test_alreadyloaded(self):
        # Ensure that get_data works without reloading - the "loads" module
        # variable in the example loader should count how many times a reload
        # occurs.
        import foo
        self.assertEqual(foo.loads, 1)
        self.assertEqual(pkgutil.get_data('foo', 'dummy'), "Hello, world!")
        self.assertEqual(foo.loads, 1)
        del sys.modules['foo']


# These tests, especially the setup and cleanup, are hideous. They
# need to be cleaned up once issue 14715 is addressed.
class ExtendPathTests(unittest.TestCase):
    def create_init(self, pkgname):
        dirname = tempfile.mkdtemp()
        sys.path.insert(0, dirname)

        pkgdir = os.path.join(dirname, pkgname)
        os.mkdir(pkgdir)
        with open(os.path.join(pkgdir, '__init__.py'), 'w') as fl:
            fl.write('from pkgutil import extend_path\n__path__ = extend_path(__path__, __name__)\n')

        return dirname

    def create_submodule(self, dirname, pkgname, submodule_name, value):
        module_name = os.path.join(dirname, pkgname, submodule_name + '.py')
        with open(module_name, 'w') as fl:
            print('value={}'.format(value), file=fl)

    def test_simple(self):
        pkgname = 'foo'
        dirname_0 = self.create_init(pkgname)
        dirname_1 = self.create_init(pkgname)
        self.create_submodule(dirname_0, pkgname, 'bar', 0)
        self.create_submodule(dirname_1, pkgname, 'baz', 1)
        import foo.bar
        import foo.baz
        # Ensure we read the expected values
        self.assertEqual(foo.bar.value, 0)
        self.assertEqual(foo.baz.value, 1)

        # Ensure the path is set up correctly
        self.assertEqual(sorted(foo.__path__),
                         sorted([os.path.join(dirname_0, pkgname),
                                 os.path.join(dirname_1, pkgname)]))

        # Cleanup
        shutil.rmtree(dirname_0)
        shutil.rmtree(dirname_1)
        del sys.path[0]
        del sys.path[0]
        del sys.modules['foo']
        del sys.modules['foo.bar']
        del sys.modules['foo.baz']


    # Another awful testing hack to be cleaned up once the test_runpy
    # helpers are factored out to a common location
    def test_iter_importers(self):
        iter_importers = pkgutil.iter_importers
        get_importer = pkgutil.get_importer

        pkgname = 'spam'
        modname = 'eggs'
        dirname = self.create_init(pkgname)
        pathitem = os.path.join(dirname, pkgname)
        fullname = '{}.{}'.format(pkgname, modname)
        sys.modules.pop(fullname, None)
        sys.modules.pop(pkgname, None)
        try:
            self.create_submodule(dirname, pkgname, modname, 0)

            importlib.import_module(fullname)

            importers = list(iter_importers(fullname))
            expected_importer = get_importer(pathitem)
            for finder in importers:
                spec = finder.find_spec(fullname)
                loader = spec.loader
                try:
                    loader = loader.loader
                except AttributeError:
                    # For now we still allow raw loaders from
                    # find_module().
                    pass
                self.assertIsInstance(finder, importlib.machinery.FileFinder)
                self.assertEqual(finder, expected_importer)
                self.assertIsInstance(loader,
                                      importlib.machinery.SourceFileLoader)
                self.assertIsNone(finder.find_spec(pkgname))

            with self.assertRaises(ImportError):
                list(iter_importers('invalid.module'))

            with self.assertRaises(ImportError):
                list(iter_importers('.spam'))
        finally:
            shutil.rmtree(dirname)
            del sys.path[0]
            try:
                del sys.modules['spam']
                del sys.modules['spam.eggs']
            except KeyError:
                pass


    def test_mixed_namespace(self):
        pkgname = 'foo'
        dirname_0 = self.create_init(pkgname)
        dirname_1 = self.create_init(pkgname)
        self.create_submodule(dirname_0, pkgname, 'bar', 0)
        # Turn this into a PEP 420 namespace package
        os.unlink(os.path.join(dirname_0, pkgname, '__init__.py'))
        self.create_submodule(dirname_1, pkgname, 'baz', 1)
        import foo.bar
        import foo.baz
        # Ensure we read the expected values
        self.assertEqual(foo.bar.value, 0)
        self.assertEqual(foo.baz.value, 1)

        # Ensure the path is set up correctly
        self.assertEqual(sorted(foo.__path__),
                         sorted([os.path.join(dirname_0, pkgname),
                                 os.path.join(dirname_1, pkgname)]))

        # Cleanup
        shutil.rmtree(dirname_0)
        shutil.rmtree(dirname_1)
        del sys.path[0]
        del sys.path[0]
        del sys.modules['foo']
        del sys.modules['foo.bar']
        del sys.modules['foo.baz']

    # XXX: test .pkg files


class NestedNamespacePackageTest(unittest.TestCase):

    def setUp(self):
        self.basedir = tempfile.mkdtemp()
        self.old_path = sys.path[:]

    def tearDown(self):
        sys.path[:] = self.old_path
        shutil.rmtree(self.basedir)

    def create_module(self, name, contents):
        base, final = name.rsplit('.', 1)
        base_path = os.path.join(self.basedir, base.replace('.', os.path.sep))
        os.makedirs(base_path, exist_ok=True)
        with open(os.path.join(base_path, final + ".py"), 'w') as f:
            f.write(contents)

    def test_nested(self):
        pkgutil_boilerplate = (
            'import pkgutil; '
            '__path__ = pkgutil.extend_path(__path__, __name__)')
        self.create_module('a.pkg.__init__', pkgutil_boilerplate)
        self.create_module('b.pkg.__init__', pkgutil_boilerplate)
        self.create_module('a.pkg.subpkg.__init__', pkgutil_boilerplate)
        self.create_module('b.pkg.subpkg.__init__', pkgutil_boilerplate)
        self.create_module('a.pkg.subpkg.c', 'c = 1')
        self.create_module('b.pkg.subpkg.d', 'd = 2')
        sys.path.insert(0, os.path.join(self.basedir, 'a'))
        sys.path.insert(0, os.path.join(self.basedir, 'b'))
        import pkg
        self.addCleanup(unload, 'pkg')
        self.assertEqual(len(pkg.__path__), 2)
        import pkg.subpkg
        self.addCleanup(unload, 'pkg.subpkg')
        self.assertEqual(len(pkg.subpkg.__path__), 2)
        from pkg.subpkg.c import c
        from pkg.subpkg.d import d
        self.assertEqual(c, 1)
        self.assertEqual(d, 2)


class ImportlibMigrationTests(unittest.TestCase):
    # With full PEP 302 support in the standard import machinery, the
    # PEP 302 emulation in this module is in the process of being
    # deprecated in favour of importlib proper

    @unittest.skipIf(__name__ == '__main__', 'not compatible with __main__')
    @ignore_warnings(category=DeprecationWarning)
    def test_get_loader_handles_missing_loader_attribute(self):
        global __loader__
        this_loader = __loader__
        del __loader__
        try:
            self.assertIsNotNone(pkgutil.get_loader(__name__))
        finally:
            __loader__ = this_loader

    @ignore_warnings(category=DeprecationWarning)
    def test_get_loader_handles_missing_spec_attribute(self):
        name = 'spam'
        mod = type(sys)(name)
        del mod.__spec__
        with CleanImport(name):
            sys.modules[name] = mod
            loader = pkgutil.get_loader(name)
        self.assertIsNone(loader)

    @ignore_warnings(category=DeprecationWarning)
    def test_get_loader_handles_spec_attribute_none(self):
        name = 'spam'
        mod = type(sys)(name)
        mod.__spec__ = None
        with CleanImport(name):
            sys.modules[name] = mod
            loader = pkgutil.get_loader(name)
        self.assertIsNone(loader)

    @ignore_warnings(category=DeprecationWarning)
    def test_get_loader_None_in_sys_modules(self):
        name = 'totally bogus'
        sys.modules[name] = None
        try:
            loader = pkgutil.get_loader(name)
        finally:
            del sys.modules[name]
        self.assertIsNone(loader)

    def test_get_loader_is_deprecated(self):
        with check_warnings(
            (r".*\bpkgutil.get_loader\b.*", DeprecationWarning),
        ):
            res = pkgutil.get_loader("sys")
        self.assertIsNotNone(res)

    def test_find_loader_is_deprecated(self):
        with check_warnings(
            (r".*\bpkgutil.find_loader\b.*", DeprecationWarning),
        ):
            res = pkgutil.find_loader("sys")
        self.assertIsNotNone(res)

    @ignore_warnings(category=DeprecationWarning)
    def test_find_loader_missing_module(self):
        name = 'totally bogus'
        loader = pkgutil.find_loader(name)
        self.assertIsNone(loader)

    def test_get_importer_avoids_emulation(self):
        # We use an illegal path so *none* of the path hooks should fire
        with check_warnings() as w:
            self.assertIsNone(pkgutil.get_importer("*??"))
            self.assertEqual(len(w.warnings), 0)

    def test_issue44061(self):
        try:
            pkgutil.get_importer(Path("/home"))
        except AttributeError:
            self.fail("Unexpected AttributeError when calling get_importer")

    def test_iter_importers_avoids_emulation(self):
        with check_warnings() as w:
            for importer in pkgutil.iter_importers(): pass
            self.assertEqual(len(w.warnings), 0)


def tearDownModule():
    # this is necessary if test is run repeated (like when finding leaks)
    import zipimport
    import importlib
    zipimport._zip_directory_cache.clear()
    importlib.invalidate_caches()


if __name__ == '__main__':
    unittest.main()
