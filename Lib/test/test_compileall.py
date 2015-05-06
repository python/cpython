import sys
import compileall
import importlib.util
import os
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

class CompileallTests(unittest.TestCase):

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

    def data(self):
        with open(self.bc_path, 'rb') as file:
            data = file.read(8)
        mtime = int(os.stat(self.source_path).st_mtime)
        compare = struct.pack('<4sl', importlib.util.MAGIC_NUMBER, mtime)
        return data, compare

    @unittest.skipUnless(hasattr(os, 'stat'), 'test needs os.stat()')
    def recreation_check(self, metadata):
        """Check that compileall recreates bytecode when the new metadata is
        used."""
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
        self.recreation_check(struct.pack('<4sl', importlib.util.MAGIC_NUMBER,
                                          1))

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

    @mock.patch('compileall.ProcessPoolExecutor')
    def test_compile_pool_called(self, pool_mock):
        compileall.compile_dir(self.directory, quiet=True, workers=5)
        self.assertTrue(pool_mock.called)

    def test_compile_workers_non_positive(self):
        with self.assertRaisesRegex(ValueError,
                                    "workers must be greater or equal to 0"):
            compileall.compile_dir(self.directory, workers=-1)

    @mock.patch('compileall.ProcessPoolExecutor')
    def test_compile_workers_cpu_count(self, pool_mock):
        compileall.compile_dir(self.directory, quiet=True, workers=0)
        self.assertEqual(pool_mock.call_args[1]['max_workers'], None)

    @mock.patch('compileall.ProcessPoolExecutor')
    @mock.patch('compileall.compile_file')
    def test_compile_one_worker(self, compile_file_mock, pool_mock):
        compileall.compile_dir(self.directory, quiet=True)
        self.assertFalse(pool_mock.called)
        self.assertTrue(compile_file_mock.called)

    @mock.patch('compileall.ProcessPoolExecutor', new=None)
    @mock.patch('compileall.compile_file')
    def test_compile_missing_multiprocessing(self, compile_file_mock):
        compileall.compile_dir(self.directory, quiet=True, workers=5)
        self.assertTrue(compile_file_mock.called)

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

    def _get_run_args(self, args):
        interp_args = ['-S']
        if sys.flags.optimize:
            interp_args.append({1 : '-O', 2 : '-OO'}[sys.flags.optimize])
        return interp_args + ['-m', 'compileall'] + list(args)

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
        self.addCleanup(self._cleanup)
        self.directory = tempfile.mkdtemp()
        self.pkgdir = os.path.join(self.directory, 'foo')
        os.mkdir(self.pkgdir)
        self.pkgdir_cachedir = os.path.join(self.pkgdir, '__pycache__')
        # Create the __init__.py and a package module.
        self.initfn = script_helper.make_script(self.pkgdir, '__init__', '')
        self.barfn = script_helper.make_script(self.pkgdir, 'bar', '')

    def _cleanup(self):
        support.rmtree(self.directory)

    def test_no_args_compiles_path(self):
        # Note that -l is implied for the no args case.
        bazfn = script_helper.make_script(self.directory, 'baz', '')
        self.assertRunOK(PYTHONPATH=self.directory)
        self.assertCompiled(bazfn)
        self.assertNotCompiled(self.initfn)
        self.assertNotCompiled(self.barfn)

    def test_no_args_respects_force_flag(self):
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

    def test_d_takes_exactly_one_dir(self):
        rc, out, err = self.assertRunNotOK('-d', 'foo')
        self.assertEqual(out, b'')
        self.assertRegex(err, b'-d')
        rc, out, err = self.assertRunNotOK('-d', 'foo', 'bar')
        self.assertEqual(out, b'')
        self.assertRegex(err, b'-d')

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
            self.assertEqual(compile_dir.call_args[-1]['workers'], None)


if __name__ == "__main__":
    unittest.main()
