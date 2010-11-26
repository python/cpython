import sys
import compileall
import imp
import os
import py_compile
import shutil
import struct
import subprocess
import tempfile
import time
import unittest
import io

from test import support

class CompileallTests(unittest.TestCase):

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.source_path = os.path.join(self.directory, '_test.py')
        self.bc_path = imp.cache_from_source(self.source_path)
        with open(self.source_path, 'w') as file:
            file.write('x = 123\n')
        self.source_path2 = os.path.join(self.directory, '_test2.py')
        self.bc_path2 = imp.cache_from_source(self.source_path2)
        shutil.copyfile(self.source_path, self.source_path2)

    def tearDown(self):
        shutil.rmtree(self.directory)

    def data(self):
        with open(self.bc_path, 'rb') as file:
            data = file.read(8)
        mtime = int(os.stat(self.source_path).st_mtime)
        compare = struct.pack('<4sl', imp.get_magic(), mtime)
        return data, compare

    def recreation_check(self, metadata):
        """Check that compileall recreates bytecode when the new metadata is
        used."""
        if not hasattr(os, 'stat'):
            return
        py_compile.compile(self.source_path)
        self.assertEqual(*self.data())
        with open(self.bc_path, 'rb') as file:
            bc = file.read()[len(metadata):]
        with open(self.bc_path, 'wb') as file:
            file.write(metadata)
            file.write(bc)
        self.assertNotEqual(*self.data())
        compileall.compile_dir(self.directory, force=False, quiet=True)
        self.assertTrue(*self.data())

    def test_mtime(self):
        # Test a change in mtime leads to a new .pyc.
        self.recreation_check(struct.pack('<4sl', imp.get_magic(), 1))

    def test_magic_number(self):
        # Test a change in mtime leads to a new .pyc.
        self.recreation_check(b'\0\0\0\0')

    def test_compile_files(self):
        # Test compiling a single file, and complete directory
        for fn in (self.bc_path, self.bc_path2):
            try:
                os.unlink(fn)
            except:
                pass
        compileall.compile_file(self.source_path, force=False, quiet=True)
        self.assertTrue(os.path.isfile(self.bc_path) and
                        not os.path.isfile(self.bc_path2))
        os.unlink(self.bc_path)
        compileall.compile_dir(self.directory, force=False, quiet=True)
        self.assertTrue(os.path.isfile(self.bc_path) and
                        os.path.isfile(self.bc_path2))
        os.unlink(self.bc_path)
        os.unlink(self.bc_path2)

    def test_no_pycache_in_non_package(self):
        # Bug 8563 reported that __pycache__ directories got created by
        # compile_file() for non-.py files.
        data_dir = os.path.join(self.directory, 'data')
        data_file = os.path.join(data_dir, 'file')
        os.mkdir(data_dir)
        # touch data/file
        with open(data_file, 'w'):
            pass
        compileall.compile_file(data_file)
        self.assertFalse(os.path.exists(os.path.join(data_dir, '__pycache__')))


class EncodingTest(unittest.TestCase):
    """Issue 6716: compileall should escape source code when printing errors
    to stdout."""

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.source_path = os.path.join(self.directory, '_test.py')
        with open(self.source_path, 'w', encoding='utf-8') as file:
            file.write('# -*- coding: utf-8 -*-\n')
            file.write('print u"\u20ac"\n')

    def tearDown(self):
        shutil.rmtree(self.directory)

    def test_error(self):
        try:
            orig_stdout = sys.stdout
            sys.stdout = io.TextIOWrapper(io.BytesIO(),encoding='ascii')
            compileall.compile_dir(self.directory)
        finally:
            sys.stdout = orig_stdout


class CommandLineTests(unittest.TestCase):
    """Test compileall's CLI."""

    def setUp(self):
        self.addCleanup(self._cleanup)
        self.directory = tempfile.mkdtemp()
        self.pkgdir = os.path.join(self.directory, 'foo')
        os.mkdir(self.pkgdir)
        # Touch the __init__.py and a package module.
        with open(os.path.join(self.pkgdir, '__init__.py'), 'w'):
            pass
        with open(os.path.join(self.pkgdir, 'bar.py'), 'w'):
            pass
        sys.path.insert(0, self.directory)

    def _cleanup(self):
        support.rmtree(self.directory)
        assert sys.path[0] == self.directory, 'Missing path'
        del sys.path[0]

    # Ensure that the default behavior of compileall's CLI is to create
    # PEP 3147 pyc/pyo files.
    for name, ext, switch in [
        ('normal', 'pyc', []),
        ('optimize', 'pyo', ['-O']),
        ('doubleoptimize', 'pyo', ['-OO']),
    ]:
        def f(self, ext=ext, switch=switch):
            retcode = subprocess.call(
                [sys.executable] + switch +
                ['-m', 'compileall', '-q', self.pkgdir])
            self.assertEqual(retcode, 0)
            # Verify the __pycache__ directory contents.
            cachedir = os.path.join(self.pkgdir, '__pycache__')
            self.assertTrue(os.path.exists(cachedir))
            expected = sorted(base.format(imp.get_tag(), ext) for base in
                              ('__init__.{}.{}', 'bar.{}.{}'))
            self.assertEqual(sorted(os.listdir(cachedir)), expected)
            # Make sure there are no .pyc files in the source directory.
            self.assertFalse([pyc_file for pyc_file in os.listdir(self.pkgdir)
                              if pyc_file.endswith(ext)])
        locals()['test_pep3147_paths_' + name] = f

    def test_legacy_paths(self):
        # Ensure that with the proper switch, compileall leaves legacy
        # pyc/pyo files, and no __pycache__ directory.
        retcode = subprocess.call(
            (sys.executable, '-m', 'compileall', '-b', '-q', self.pkgdir))
        self.assertEqual(retcode, 0)
        # Verify the __pycache__ directory contents.
        cachedir = os.path.join(self.pkgdir, '__pycache__')
        self.assertFalse(os.path.exists(cachedir))
        expected = sorted(['__init__.py', '__init__.pyc', 'bar.py', 'bar.pyc'])
        self.assertEqual(sorted(os.listdir(self.pkgdir)), expected)

    def test_multiple_runs(self):
        # Bug 8527 reported that multiple calls produced empty
        # __pycache__/__pycache__ directories.
        retcode = subprocess.call(
            (sys.executable, '-m', 'compileall', '-q', self.pkgdir))
        self.assertEqual(retcode, 0)
        # Verify the __pycache__ directory contents.
        cachedir = os.path.join(self.pkgdir, '__pycache__')
        self.assertTrue(os.path.exists(cachedir))
        cachecachedir = os.path.join(cachedir, '__pycache__')
        self.assertFalse(os.path.exists(cachecachedir))
        # Call compileall again.
        retcode = subprocess.call(
            (sys.executable, '-m', 'compileall', '-q', self.pkgdir))
        self.assertEqual(retcode, 0)
        self.assertTrue(os.path.exists(cachedir))
        self.assertFalse(os.path.exists(cachecachedir))

    def test_force(self):
        retcode = subprocess.call(
            (sys.executable, '-m', 'compileall', '-q', self.pkgdir))
        self.assertEqual(retcode, 0)
        pycpath = imp.cache_from_source(os.path.join(self.pkgdir, 'bar.py'))
        # set atime/mtime backward to avoid file timestamp resolution issues
        os.utime(pycpath, (time.time()-60,)*2)
        access = os.stat(pycpath).st_mtime
        retcode = subprocess.call(
            (sys.executable, '-m', 'compileall', '-q', '-f', self.pkgdir))
        self.assertEqual(retcode, 0)
        access2 = os.stat(pycpath).st_mtime
        self.assertNotEqual(access, access2)

    def test_legacy(self):
        # create a new module  XXX could rewrite using self.pkgdir
        newpackage = os.path.join(self.pkgdir, 'spam')
        os.mkdir(newpackage)
        with open(os.path.join(newpackage, '__init__.py'), 'w'):
            pass
        with open(os.path.join(newpackage, 'ham.py'), 'w'):
            pass
        sourcefile = os.path.join(newpackage, 'ham.py')

        retcode = subprocess.call(
                (sys.executable, '-m', 'compileall',  '-q', '-l', self.pkgdir))
        self.assertEqual(retcode, 0)
        self.assertFalse(os.path.exists(imp.cache_from_source(sourcefile)))

        retcode = subprocess.call(
                (sys.executable, '-m', 'compileall', '-q', self.pkgdir))
        self.assertEqual(retcode, 0)
        self.assertTrue(os.path.exists(imp.cache_from_source(sourcefile)))

    def test_quiet(self):
        noise = subprocess.check_output(
             [sys.executable, '-m', 'compileall', self.pkgdir],
             stderr=subprocess.STDOUT)
        quiet = subprocess.check_output(
            [sys.executable, '-m', 'compileall', '-f', '-q', self.pkgdir],
            stderr=subprocess.STDOUT)
        self.assertGreater(len(noise), len(quiet))

    def test_regexp(self):
        retcode = subprocess.call(
            (sys.executable, '-m', 'compileall', '-q', '-x', 'bar.*', self.pkgdir))
        self.assertEqual(retcode, 0)

        sourcefile = os.path.join(self.pkgdir, 'bar.py')
        self.assertFalse(os.path.exists(imp.cache_from_source(sourcefile)))
        sourcefile = os.path.join(self.pkgdir, '__init__.py')
        self.assertTrue(os.path.exists(imp.cache_from_source(sourcefile)))


def test_main():
    support.run_unittest(
        CommandLineTests,
        CompileallTests,
        EncodingTest,
        )


if __name__ == "__main__":
    test_main()
