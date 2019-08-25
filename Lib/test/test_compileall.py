import sys
import compileall
import importlib.util
import test.test_importlib.util
import os
import pathlib
import py_compile
import shutil
import struct
import tempfile
import time
import unittest
import io

from unittest import mock, skipUnless
try:
    from concurrent.futures import ProcessPoolExecutor
    _have_multiprocessing = True
except ImportError:
    _have_multiprocessing = False

from test import support
from test.support import script_helper

from .test_py_compile import without_source_date_epoch
from .test_py_compile import SourceDateEpochTestMeta


class CompileallTestsBase:

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.source_path = os.path.join(self.directory, '_test.py')
        self.bc_path = importlib.util.cache_from_source(self.source_path)
        with open(self.source_path, 'w') as file:
            file.write('x = 123\n')
        self.source_path2 = os.path.join(self.directory, '_test2.py')
        self.bc_path2 = importlib.util.cache_from_source(self.source_path2)
        shutil.copyfile(self.source_path, self.source_path2)
        self.subdirectory = os.path.join(self.directory, '_subdir')
        os.mkdir(self.subdirectory)
        self.source_path3 = os.path.join(self.subdirectory, '_test3.py')
        shutil.copyfile(self.source_path, self.source_path3)

    def tearDown(self):
        shutil.rmtree(self.directory)

    def add_bad_source_file(self):
        self.bad_source_path = os.path.join(self.directory, '_test_bad.py')
        with open(self.bad_source_path, 'w') as file:
            file.write('x (\n')

    def timestamp_metadata(self):
        with open(self.bc_path, 'rb') as file:
            data = file.read(12)
        mtime = int(os.stat(self.source_path).st_mtime)
        compare = struct.pack('<4sll', importlib.util.MAGIC_NUMBER, 0, mtime)
        return data, compare

    def recreation_check(self, metadata):
        """Check that compileall recreates bytecode when the new metadata is
        used."""
        if os.environ.get('SOURCE_DATE_EPOCH'):
            raise unittest.SkipTest('SOURCE_DATE_EPOCH is set')
        py_compile.compile(self.source_path)
        self.assertEqual(*self.timestamp_metadata())
        with open(self.bc_path, 'rb') as file:
            bc = file.read()[len(metadata):]
        with open(self.bc_path, 'wb') as file:
            file.write(metadata)
            file.write(bc)
        self.assertNotEqual(*self.timestamp_metadata())
        compileall.compile_dir(self.directory, force=False, quiet=True)
        self.assertTrue(*self.timestamp_metadata())

    def test_mtime(self):
        # Test a change in mtime leads to a new .pyc.
        self.recreation_check(struct.pack('<4sll', importlib.util.MAGIC_NUMBER,
                                          0, 1))

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
        self.assertTrue(compileall.compile_file(self.source_path,
                                                force=False, quiet=True))
        self.assertTrue(os.path.isfile(self.bc_path) and
                        not os.path.isfile(self.bc_path2))
        os.unlink(self.bc_path)
        self.assertTrue(compileall.compile_dir(self.directory, force=False,
                                               quiet=True))
        self.assertTrue(os.path.isfile(self.bc_path) and
                        os.path.isfile(self.bc_path2))
        os.unlink(self.bc_path)
        os.unlink(self.bc_path2)
        # Test against bad files
        self.add_bad_source_file()
        self.assertFalse(compileall.compile_file(self.bad_source_path,
                                                 force=False, quiet=2))
        self.assertFalse(compileall.compile_dir(self.directory,
                                                force=False, quiet=2))

    def test_compile_file_pathlike(self):
        self.assertFalse(os.path.isfile(self.bc_path))
        # we should also test the output
        with support.captured_stdout() as stdout:
            self.assertTrue(compileall.compile_file(pathlib.Path(self.source_path)))
        self.assertRegex(stdout.getvalue(), r'Compiling ([^WindowsPath|PosixPath].*)')
        self.assertTrue(os.path.isfile(self.bc_path))

    def test_compile_file_pathlike_ddir(self):
        self.assertFalse(os.path.isfile(self.bc_path))
        self.assertTrue(compileall.compile_file(pathlib.Path(self.source_path),
                                                ddir=pathlib.Path('ddir_path'),
                                                quiet=2))
        self.assertTrue(os.path.isfile(self.bc_path))

    def test_compile_path(self):
        with test.test_importlib.util.import_state(path=[self.directory]):
            self.assertTrue(compileall.compile_path(quiet=2))

        with test.test_importlib.util.import_state(path=[self.directory]):
            self.add_bad_source_file()
            self.assertFalse(compileall.compile_path(skip_curdir=False,
                                                     force=True, quiet=2))

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

    def test_optimize(self):
        # make sure compiling with different optimization settings than the
        # interpreter's creates the correct file names
        optimize, opt = (1, 1) if __debug__ else (0, '')
        compileall.compile_dir(self.directory, quiet=True, optimize=optimize)
        cached = importlib.util.cache_from_source(self.source_path,
                                                  optimization=opt)
        self.assertTrue(os.path.isfile(cached))
        cached2 = importlib.util.cache_from_source(self.source_path2,
                                                   optimization=opt)
        self.assertTrue(os.path.isfile(cached2))
        cached3 = importlib.util.cache_from_source(self.source_path3,
                                                   optimization=opt)
        self.assertTrue(os.path.isfile(cached3))

    def test_compile_dir_pathlike(self):
        self.assertFalse(os.path.isfile(self.bc_path))
        with support.captured_stdout() as stdout:
            compileall.compile_dir(pathlib.Path(self.directory))
        line = stdout.getvalue().splitlines()[0]
        self.assertRegex(line, r'Listing ([^WindowsPath|PosixPath].*)')
        self.assertTrue(os.path.isfile(self.bc_path))

    @mock.patch('concurrent.futures.ProcessPoolExecutor')
    def test_compile_pool_called(self, pool_mock):
        compileall.compile_dir(self.directory, quiet=True, workers=5)
        self.assertTrue(pool_mock.called)

    def test_compile_workers_non_positive(self):
        with self.assertRaisesRegex(ValueError,
                                    "workers must be greater or equal to 0"):
            compileall.compile_dir(self.directory, workers=-1)

    @mock.patch('concurrent.futures.ProcessPoolExecutor')
    def test_compile_workers_cpu_count(self, pool_mock):
        compileall.compile_dir(self.directory, quiet=True, workers=0)
        self.assertEqual(pool_mock.call_args[1]['max_workers'], None)

    @mock.patch('concurrent.futures.ProcessPoolExecutor')
    @mock.patch('compileall.compile_file')
    def test_compile_one_worker(self, compile_file_mock, pool_mock):
        compileall.compile_dir(self.directory, quiet=True)
        self.assertFalse(pool_mock.called)
        self.assertTrue(compile_file_mock.called)

    @mock.patch('concurrent.futures.ProcessPoolExecutor', new=None)
    @mock.patch('compileall.compile_file')
    def test_compile_missing_multiprocessing(self, compile_file_mock):
        compileall.compile_dir(self.directory, quiet=True, workers=5)
        self.assertTrue(compile_file_mock.called)


class CompileallTestsWithSourceEpoch(CompileallTestsBase,
                                     unittest.TestCase,
                                     metaclass=SourceDateEpochTestMeta,
                                     source_date_epoch=True):
    pass


class CompileallTestsWithoutSourceEpoch(CompileallTestsBase,
                                        unittest.TestCase,
                                        metaclass=SourceDateEpochTestMeta,
                                        source_date_epoch=False):
    pass


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


class CommandLineTestsBase:
    """Test compileall's CLI."""

    @classmethod
    def setUpClass(cls):
        for path in filter(os.path.isdir, sys.path):
            directory_created = False
            directory = pathlib.Path(path) / '__pycache__'
            path = directory / 'test.try'
            try:
                if not directory.is_dir():
                    directory.mkdir()
                    directory_created = True
                with path.open('w') as file:
                    file.write('# for test_compileall')
            except OSError:
                sys_path_writable = False
                break
            finally:
                support.unlink(str(path))
                if directory_created:
                    directory.rmdir()
        else:
            sys_path_writable = True
        cls._sys_path_writable = sys_path_writable

    def _skip_if_sys_path_not_writable(self):
        if not self._sys_path_writable:
            raise unittest.SkipTest('not all entries on sys.path are writable')

    def _get_run_args(self, args):
        return [*support.optim_args_from_interpreter_flags(),
                '-S', '-m', 'compileall',
                *args]

    def assertRunOK(self, *args, **env_vars):
        rc, out, err = script_helper.assert_python_ok(
                         *self._get_run_args(args), **env_vars)
        self.assertEqual(b'', err)
        return out

    def assertRunNotOK(self, *args, **env_vars):
        rc, out, err = script_helper.assert_python_failure(
                        *self._get_run_args(args), **env_vars)
        return rc, out, err

    def assertCompiled(self, fn):
        path = importlib.util.cache_from_source(fn)
        self.assertTrue(os.path.exists(path))

    def assertNotCompiled(self, fn):
        path = importlib.util.cache_from_source(fn)
        self.assertFalse(os.path.exists(path))

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.addCleanup(support.rmtree, self.directory)
        self.pkgdir = os.path.join(self.directory, 'foo')
        os.mkdir(self.pkgdir)
        self.pkgdir_cachedir = os.path.join(self.pkgdir, '__pycache__')
        # Create the __init__.py and a package module.
        self.initfn = script_helper.make_script(self.pkgdir, '__init__', '')
        self.barfn = script_helper.make_script(self.pkgdir, 'bar', '')

    def test_no_args_compiles_path(self):
        # Note that -l is implied for the no args case.
        self._skip_if_sys_path_not_writable()
        bazfn = script_helper.make_script(self.directory, 'baz', '')
        self.assertRunOK(PYTHONPATH=self.directory)
        self.assertCompiled(bazfn)
        self.assertNotCompiled(self.initfn)
        self.assertNotCompiled(self.barfn)

    @without_source_date_epoch  # timestamp invalidation test
    def test_no_args_respects_force_flag(self):
        self._skip_if_sys_path_not_writable()
        bazfn = script_helper.make_script(self.directory, 'baz', '')
        self.assertRunOK(PYTHONPATH=self.directory)
        pycpath = importlib.util.cache_from_source(bazfn)
        # Set atime/mtime backward to avoid file timestamp resolution issues
        os.utime(pycpath, (time.time()-60,)*2)
        mtime = os.stat(pycpath).st_mtime
        # Without force, no recompilation
        self.assertRunOK(PYTHONPATH=self.directory)
        mtime2 = os.stat(pycpath).st_mtime
        self.assertEqual(mtime, mtime2)
        # Now force it.
        self.assertRunOK('-f', PYTHONPATH=self.directory)
        mtime2 = os.stat(pycpath).st_mtime
        self.assertNotEqual(mtime, mtime2)

    def test_no_args_respects_quiet_flag(self):
        self._skip_if_sys_path_not_writable()
        script_helper.make_script(self.directory, 'baz', '')
        noisy = self.assertRunOK(PYTHONPATH=self.directory)
        self.assertIn(b'Listing ', noisy)
        quiet = self.assertRunOK('-q', PYTHONPATH=self.directory)
        self.assertNotIn(b'Listing ', quiet)

    # Ensure that the default behavior of compileall's CLI is to create
    # PEP 3147/PEP 488 pyc files.
    for name, ext, switch in [
        ('normal', 'pyc', []),
        ('optimize', 'opt-1.pyc', ['-O']),
        ('doubleoptimize', 'opt-2.pyc', ['-OO']),
    ]:
        def f(self, ext=ext, switch=switch):
            script_helper.assert_python_ok(*(switch +
                ['-m', 'compileall', '-q', self.pkgdir]))
            # Verify the __pycache__ directory contents.
            self.assertTrue(os.path.exists(self.pkgdir_cachedir))
            expected = sorted(base.format(sys.implementation.cache_tag, ext)
                              for base in ('__init__.{}.{}', 'bar.{}.{}'))
            self.assertEqual(sorted(os.listdir(self.pkgdir_cachedir)), expected)
            # Make sure there are no .pyc files in the source directory.
            self.assertFalse([fn for fn in os.listdir(self.pkgdir)
                              if fn.endswith(ext)])
        locals()['test_pep3147_paths_' + name] = f

    def test_legacy_paths(self):
        # Ensure that with the proper switch, compileall leaves legacy
        # pyc files, and no __pycache__ directory.
        self.assertRunOK('-b', '-q', self.pkgdir)
        # Verify the __pycache__ directory contents.
        self.assertFalse(os.path.exists(self.pkgdir_cachedir))
        expected = sorted(['__init__.py', '__init__.pyc', 'bar.py',
                           'bar.pyc'])
        self.assertEqual(sorted(os.listdir(self.pkgdir)), expected)

    def test_multiple_runs(self):
        # Bug 8527 reported that multiple calls produced empty
        # __pycache__/__pycache__ directories.
        self.assertRunOK('-q', self.pkgdir)
        # Verify the __pycache__ directory contents.
        self.assertTrue(os.path.exists(self.pkgdir_cachedir))
        cachecachedir = os.path.join(self.pkgdir_cachedir, '__pycache__')
        self.assertFalse(os.path.exists(cachecachedir))
        # Call compileall again.
        self.assertRunOK('-q', self.pkgdir)
        self.assertTrue(os.path.exists(self.pkgdir_cachedir))
        self.assertFalse(os.path.exists(cachecachedir))

    @without_source_date_epoch  # timestamp invalidation test
    def test_force(self):
        self.assertRunOK('-q', self.pkgdir)
        pycpath = importlib.util.cache_from_source(self.barfn)
        # set atime/mtime backward to avoid file timestamp resolution issues
        os.utime(pycpath, (time.time()-60,)*2)
        mtime = os.stat(pycpath).st_mtime
        # without force, no recompilation
        self.assertRunOK('-q', self.pkgdir)
        mtime2 = os.stat(pycpath).st_mtime
        self.assertEqual(mtime, mtime2)
        # now force it.
        self.assertRunOK('-q', '-f', self.pkgdir)
        mtime2 = os.stat(pycpath).st_mtime
        self.assertNotEqual(mtime, mtime2)

    def test_recursion_control(self):
        subpackage = os.path.join(self.pkgdir, 'spam')
        os.mkdir(subpackage)
        subinitfn = script_helper.make_script(subpackage, '__init__', '')
        hamfn = script_helper.make_script(subpackage, 'ham', '')
        self.assertRunOK('-q', '-l', self.pkgdir)
        self.assertNotCompiled(subinitfn)
        self.assertFalse(os.path.exists(os.path.join(subpackage, '__pycache__')))
        self.assertRunOK('-q', self.pkgdir)
        self.assertCompiled(subinitfn)
        self.assertCompiled(hamfn)

    def test_recursion_limit(self):
        subpackage = os.path.join(self.pkgdir, 'spam')
        subpackage2 = os.path.join(subpackage, 'ham')
        subpackage3 = os.path.join(subpackage2, 'eggs')
        for pkg in (subpackage, subpackage2, subpackage3):
            script_helper.make_pkg(pkg)

        subinitfn = os.path.join(subpackage, '__init__.py')
        hamfn = script_helper.make_script(subpackage, 'ham', '')
        spamfn = script_helper.make_script(subpackage2, 'spam', '')
        eggfn = script_helper.make_script(subpackage3, 'egg', '')

        self.assertRunOK('-q', '-r 0', self.pkgdir)
        self.assertNotCompiled(subinitfn)
        self.assertFalse(
            os.path.exists(os.path.join(subpackage, '__pycache__')))

        self.assertRunOK('-q', '-r 1', self.pkgdir)
        self.assertCompiled(subinitfn)
        self.assertCompiled(hamfn)
        self.assertNotCompiled(spamfn)

        self.assertRunOK('-q', '-r 2', self.pkgdir)
        self.assertCompiled(subinitfn)
        self.assertCompiled(hamfn)
        self.assertCompiled(spamfn)
        self.assertNotCompiled(eggfn)

        self.assertRunOK('-q', '-r 5', self.pkgdir)
        self.assertCompiled(subinitfn)
        self.assertCompiled(hamfn)
        self.assertCompiled(spamfn)
        self.assertCompiled(eggfn)

    def test_quiet(self):
        noisy = self.assertRunOK(self.pkgdir)
        quiet = self.assertRunOK('-q', self.pkgdir)
        self.assertNotEqual(b'', noisy)
        self.assertEqual(b'', quiet)

    def test_silent(self):
        script_helper.make_script(self.pkgdir, 'crunchyfrog', 'bad(syntax')
        _, quiet, _ = self.assertRunNotOK('-q', self.pkgdir)
        _, silent, _ = self.assertRunNotOK('-qq', self.pkgdir)
        self.assertNotEqual(b'', quiet)
        self.assertEqual(b'', silent)

    def test_regexp(self):
        self.assertRunOK('-q', '-x', r'ba[^\\/]*$', self.pkgdir)
        self.assertNotCompiled(self.barfn)
        self.assertCompiled(self.initfn)

    def test_multiple_dirs(self):
        pkgdir2 = os.path.join(self.directory, 'foo2')
        os.mkdir(pkgdir2)
        init2fn = script_helper.make_script(pkgdir2, '__init__', '')
        bar2fn = script_helper.make_script(pkgdir2, 'bar2', '')
        self.assertRunOK('-q', self.pkgdir, pkgdir2)
        self.assertCompiled(self.initfn)
        self.assertCompiled(self.barfn)
        self.assertCompiled(init2fn)
        self.assertCompiled(bar2fn)

    def test_d_compile_error(self):
        script_helper.make_script(self.pkgdir, 'crunchyfrog', 'bad(syntax')
        rc, out, err = self.assertRunNotOK('-q', '-d', 'dinsdale', self.pkgdir)
        self.assertRegex(out, b'File "dinsdale')

    def test_d_runtime_error(self):
        bazfn = script_helper.make_script(self.pkgdir, 'baz', 'raise Exception')
        self.assertRunOK('-q', '-d', 'dinsdale', self.pkgdir)
        fn = script_helper.make_script(self.pkgdir, 'bing', 'import baz')
        pyc = importlib.util.cache_from_source(bazfn)
        os.rename(pyc, os.path.join(self.pkgdir, 'baz.pyc'))
        os.remove(bazfn)
        rc, out, err = script_helper.assert_python_failure(fn, __isolated=False)
        self.assertRegex(err, b'File "dinsdale')

    def test_include_bad_file(self):
        rc, out, err = self.assertRunNotOK(
            '-i', os.path.join(self.directory, 'nosuchfile'), self.pkgdir)
        self.assertRegex(out, b'rror.*nosuchfile')
        self.assertNotRegex(err, b'Traceback')
        self.assertFalse(os.path.exists(importlib.util.cache_from_source(
                                            self.pkgdir_cachedir)))

    def test_include_file_with_arg(self):
        f1 = script_helper.make_script(self.pkgdir, 'f1', '')
        f2 = script_helper.make_script(self.pkgdir, 'f2', '')
        f3 = script_helper.make_script(self.pkgdir, 'f3', '')
        f4 = script_helper.make_script(self.pkgdir, 'f4', '')
        with open(os.path.join(self.directory, 'l1'), 'w') as l1:
            l1.write(os.path.join(self.pkgdir, 'f1.py')+os.linesep)
            l1.write(os.path.join(self.pkgdir, 'f2.py')+os.linesep)
        self.assertRunOK('-i', os.path.join(self.directory, 'l1'), f4)
        self.assertCompiled(f1)
        self.assertCompiled(f2)
        self.assertNotCompiled(f3)
        self.assertCompiled(f4)

    def test_include_file_no_arg(self):
        f1 = script_helper.make_script(self.pkgdir, 'f1', '')
        f2 = script_helper.make_script(self.pkgdir, 'f2', '')
        f3 = script_helper.make_script(self.pkgdir, 'f3', '')
        f4 = script_helper.make_script(self.pkgdir, 'f4', '')
        with open(os.path.join(self.directory, 'l1'), 'w') as l1:
            l1.write(os.path.join(self.pkgdir, 'f2.py')+os.linesep)
        self.assertRunOK('-i', os.path.join(self.directory, 'l1'))
        self.assertNotCompiled(f1)
        self.assertCompiled(f2)
        self.assertNotCompiled(f3)
        self.assertNotCompiled(f4)

    def test_include_on_stdin(self):
        f1 = script_helper.make_script(self.pkgdir, 'f1', '')
        f2 = script_helper.make_script(self.pkgdir, 'f2', '')
        f3 = script_helper.make_script(self.pkgdir, 'f3', '')
        f4 = script_helper.make_script(self.pkgdir, 'f4', '')
        p = script_helper.spawn_python(*(self._get_run_args(()) + ['-i', '-']))
        p.stdin.write((f3+os.linesep).encode('ascii'))
        script_helper.kill_python(p)
        self.assertNotCompiled(f1)
        self.assertNotCompiled(f2)
        self.assertCompiled(f3)
        self.assertNotCompiled(f4)

    def test_compiles_as_much_as_possible(self):
        bingfn = script_helper.make_script(self.pkgdir, 'bing', 'syntax(error')
        rc, out, err = self.assertRunNotOK('nosuchfile', self.initfn,
                                           bingfn, self.barfn)
        self.assertRegex(out, b'rror')
        self.assertNotCompiled(bingfn)
        self.assertCompiled(self.initfn)
        self.assertCompiled(self.barfn)

    def test_invalid_arg_produces_message(self):
        out = self.assertRunOK('badfilename')
        self.assertRegex(out, b"Can't list 'badfilename'")

    def test_pyc_invalidation_mode(self):
        script_helper.make_script(self.pkgdir, 'f1', '')
        pyc = importlib.util.cache_from_source(
            os.path.join(self.pkgdir, 'f1.py'))
        self.assertRunOK('--invalidation-mode=checked-hash', self.pkgdir)
        with open(pyc, 'rb') as fp:
            data = fp.read()
        self.assertEqual(int.from_bytes(data[4:8], 'little'), 0b11)
        self.assertRunOK('--invalidation-mode=unchecked-hash', self.pkgdir)
        with open(pyc, 'rb') as fp:
            data = fp.read()
        self.assertEqual(int.from_bytes(data[4:8], 'little'), 0b01)

    @skipUnless(_have_multiprocessing, "requires multiprocessing")
    def test_workers(self):
        bar2fn = script_helper.make_script(self.directory, 'bar2', '')
        files = []
        for suffix in range(5):
            pkgdir = os.path.join(self.directory, 'foo{}'.format(suffix))
            os.mkdir(pkgdir)
            fn = script_helper.make_script(pkgdir, '__init__', '')
            files.append(script_helper.make_script(pkgdir, 'bar2', ''))

        self.assertRunOK(self.directory, '-j', '0')
        self.assertCompiled(bar2fn)
        for file in files:
            self.assertCompiled(file)

    @mock.patch('compileall.compile_dir')
    def test_workers_available_cores(self, compile_dir):
        with mock.patch("sys.argv",
                        new=[sys.executable, self.directory, "-j0"]):
            compileall.main()
            self.assertTrue(compile_dir.called)
            self.assertEqual(compile_dir.call_args[-1]['workers'], 0)


class CommandLineTestsWithSourceEpoch(CommandLineTestsBase,
                                       unittest.TestCase,
                                       metaclass=SourceDateEpochTestMeta,
                                       source_date_epoch=True):
    pass


class CommandLineTestsNoSourceEpoch(CommandLineTestsBase,
                                     unittest.TestCase,
                                     metaclass=SourceDateEpochTestMeta,
                                     source_date_epoch=False):
    pass



if __name__ == "__main__":
    unittest.main()
