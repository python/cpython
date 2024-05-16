"""
Test harness for the venv module.

Copyright (C) 2011-2012 Vinay Sajip.
Licensed to the PSF under a contributor agreement.
"""

import contextlib
import ensurepip
import os
import os.path
import pathlib
import re
import shutil
import struct
import subprocess
import sys
import sysconfig
import tempfile
from test.support import (captured_stdout, captured_stderr,
                          skip_if_broken_multiprocessing_synchronize, verbose,
                          requires_subprocess, is_emscripten, is_wasi,
                          requires_venv_with_pip, TEST_HOME_DIR,
                          requires_resource, copy_python_src_ignore)
from test.support.os_helper import (can_symlink, EnvironmentVarGuard, rmtree,
                                    TESTFN, FakePath)
import unittest
import venv
from unittest.mock import patch, Mock

try:
    import ctypes
except ImportError:
    ctypes = None

# Platforms that set sys._base_executable can create venvs from within
# another venv, so no need to skip tests that require venv.create().
requireVenvCreate = unittest.skipUnless(
    sys.prefix == sys.base_prefix
    or sys._base_executable != sys.executable,
    'cannot run venv.create from within a venv on this platform')

if is_emscripten or is_wasi:
    raise unittest.SkipTest("venv is not available on Emscripten/WASI.")

@requires_subprocess()
def check_output(cmd, encoding=None):
    p = subprocess.Popen(cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding=encoding)
    out, err = p.communicate()
    if p.returncode:
        if verbose and err:
            print(err.decode('utf-8', 'backslashreplace'))
        raise subprocess.CalledProcessError(
            p.returncode, cmd, out, err)
    return out, err

class BaseTest(unittest.TestCase):
    """Base class for venv tests."""
    maxDiff = 80 * 50

    def setUp(self):
        self.env_dir = os.path.realpath(tempfile.mkdtemp())
        if os.name == 'nt':
            self.bindir = 'Scripts'
            self.lib = ('Lib',)
            self.include = 'Include'
        else:
            self.bindir = 'bin'
            self.lib = ('lib', 'python%d.%d' % sys.version_info[:2])
            self.include = 'include'
        executable = sys._base_executable
        self.exe = os.path.split(executable)[-1]
        if (sys.platform == 'win32'
            and os.path.lexists(executable)
            and not os.path.exists(executable)):
            self.cannot_link_exe = True
        else:
            self.cannot_link_exe = False

    def tearDown(self):
        rmtree(self.env_dir)

    def run_with_capture(self, func, *args, **kwargs):
        with captured_stdout() as output:
            with captured_stderr() as error:
                func(*args, **kwargs)
        return output.getvalue(), error.getvalue()

    def get_env_file(self, *args):
        return os.path.join(self.env_dir, *args)

    def get_text_file_contents(self, *args, encoding='utf-8'):
        with open(self.get_env_file(*args), 'r', encoding=encoding) as f:
            result = f.read()
        return result

class BasicTest(BaseTest):
    """Test venv module functionality."""

    def isdir(self, *args):
        fn = self.get_env_file(*args)
        self.assertTrue(os.path.isdir(fn))

    def test_defaults_with_str_path(self):
        """
        Test the create function with default arguments and a str path.
        """
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        self._check_output_of_default_create()

    def test_defaults_with_pathlike(self):
        """
        Test the create function with default arguments and a path-like path.
        """
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, FakePath(self.env_dir))
        self._check_output_of_default_create()

    def _check_output_of_default_create(self):
        self.isdir(self.bindir)
        self.isdir(self.include)
        self.isdir(*self.lib)
        # Issue 21197
        p = self.get_env_file('lib64')
        conditions = ((struct.calcsize('P') == 8) and (os.name == 'posix') and
                      (sys.platform != 'darwin'))
        if conditions:
            self.assertTrue(os.path.islink(p))
        else:
            self.assertFalse(os.path.exists(p))
        data = self.get_text_file_contents('pyvenv.cfg')
        executable = sys._base_executable
        path = os.path.dirname(executable)
        self.assertIn('home = %s' % path, data)
        self.assertIn('executable = %s' %
                      os.path.realpath(sys.executable), data)
        copies = '' if os.name=='nt' else ' --copies'
        cmd = f'command = {sys.executable} -m venv{copies} --without-pip {self.env_dir}'
        self.assertIn(cmd, data)
        fn = self.get_env_file(self.bindir, self.exe)
        if not os.path.exists(fn):  # diagnostics for Windows buildbot failures
            bd = self.get_env_file(self.bindir)
            print('Contents of %r:' % bd)
            print('    %r' % os.listdir(bd))
        self.assertTrue(os.path.exists(fn), 'File %r should exist.' % fn)

    def test_config_file_command_key(self):
        attrs = [
            (None, None),
            ('symlinks', '--copies'),
            ('with_pip', '--without-pip'),
            ('system_site_packages', '--system-site-packages'),
            ('clear', '--clear'),
            ('upgrade', '--upgrade'),
            ('upgrade_deps', '--upgrade-deps'),
            ('prompt', '--prompt'),
        ]
        for attr, opt in attrs:
            rmtree(self.env_dir)
            if not attr:
                b = venv.EnvBuilder()
            else:
                b = venv.EnvBuilder(
                    **{attr: False if attr in ('with_pip', 'symlinks') else True})
            b.upgrade_dependencies = Mock() # avoid pip command to upgrade deps
            b._setup_pip = Mock() # avoid pip setup
            self.run_with_capture(b.create, self.env_dir)
            data = self.get_text_file_contents('pyvenv.cfg')
            if not attr:
                for opt in ('--system-site-packages', '--clear', '--upgrade',
                        '--upgrade-deps', '--prompt'):
                    self.assertNotRegex(data, rf'command = .* {opt}')
            elif os.name=='nt' and attr=='symlinks':
                pass
            else:
                self.assertRegex(data, rf'command = .* {opt}')

    def test_prompt(self):
        env_name = os.path.split(self.env_dir)[1]

        rmtree(self.env_dir)
        builder = venv.EnvBuilder()
        self.run_with_capture(builder.create, self.env_dir)
        context = builder.ensure_directories(self.env_dir)
        data = self.get_text_file_contents('pyvenv.cfg')
        self.assertEqual(context.prompt, '(%s) ' % env_name)
        self.assertNotIn("prompt = ", data)

        rmtree(self.env_dir)
        builder = venv.EnvBuilder(prompt='My prompt')
        self.run_with_capture(builder.create, self.env_dir)
        context = builder.ensure_directories(self.env_dir)
        data = self.get_text_file_contents('pyvenv.cfg')
        self.assertEqual(context.prompt, '(My prompt) ')
        self.assertIn("prompt = 'My prompt'\n", data)

        rmtree(self.env_dir)
        builder = venv.EnvBuilder(prompt='.')
        cwd = os.path.basename(os.getcwd())
        self.run_with_capture(builder.create, self.env_dir)
        context = builder.ensure_directories(self.env_dir)
        data = self.get_text_file_contents('pyvenv.cfg')
        self.assertEqual(context.prompt, '(%s) ' % cwd)
        self.assertIn("prompt = '%s'\n" % cwd, data)

    def test_upgrade_dependencies(self):
        builder = venv.EnvBuilder()
        bin_path = 'Scripts' if sys.platform == 'win32' else 'bin'
        python_exe = os.path.split(sys.executable)[1]
        with tempfile.TemporaryDirectory() as fake_env_dir:
            expect_exe = os.path.normcase(
                os.path.join(fake_env_dir, bin_path, python_exe)
            )
            if sys.platform == 'win32':
                expect_exe = os.path.normcase(os.path.realpath(expect_exe))

            def pip_cmd_checker(cmd, **kwargs):
                cmd[0] = os.path.normcase(cmd[0])
                self.assertEqual(
                    cmd,
                    [
                        expect_exe,
                        '-m',
                        'pip',
                        'install',
                        '--upgrade',
                        'pip',
                    ]
                )

            fake_context = builder.ensure_directories(fake_env_dir)
            with patch('venv.subprocess.check_output', pip_cmd_checker):
                builder.upgrade_dependencies(fake_context)

    @requireVenvCreate
    def test_prefixes(self):
        """
        Test that the prefix values are as expected.
        """
        # check a venv's prefixes
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        envpy = os.path.join(self.env_dir, self.bindir, self.exe)
        cmd = [envpy, '-c', None]
        for prefix, expected in (
            ('prefix', self.env_dir),
            ('exec_prefix', self.env_dir),
            ('base_prefix', sys.base_prefix),
            ('base_exec_prefix', sys.base_exec_prefix)):
            cmd[2] = 'import sys; print(sys.%s)' % prefix
            out, err = check_output(cmd)
            self.assertEqual(pathlib.Path(out.strip().decode()),
                             pathlib.Path(expected), prefix)

    @requireVenvCreate
    def test_sysconfig(self):
        """
        Test that the sysconfig functions work in a virtual environment.
        """
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir, symlinks=False)
        envpy = os.path.join(self.env_dir, self.bindir, self.exe)
        cmd = [envpy, '-c', None]
        for call, expected in (
            # installation scheme
            ('get_preferred_scheme("prefix")', 'venv'),
            ('get_default_scheme()', 'venv'),
            # build environment
            ('is_python_build()', str(sysconfig.is_python_build())),
            ('get_makefile_filename()', sysconfig.get_makefile_filename()),
            ('get_config_h_filename()', sysconfig.get_config_h_filename())):
            with self.subTest(call):
                cmd[2] = 'import sysconfig; print(sysconfig.%s)' % call
                out, err = check_output(cmd)
                self.assertEqual(out.strip(), expected.encode(), err)

    @requireVenvCreate
    @unittest.skipUnless(can_symlink(), 'Needs symlinks')
    def test_sysconfig_symlinks(self):
        """
        Test that the sysconfig functions work in a virtual environment.
        """
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir, symlinks=True)
        envpy = os.path.join(self.env_dir, self.bindir, self.exe)
        cmd = [envpy, '-c', None]
        for call, expected in (
            # installation scheme
            ('get_preferred_scheme("prefix")', 'venv'),
            ('get_default_scheme()', 'venv'),
            # build environment
            ('is_python_build()', str(sysconfig.is_python_build())),
            ('get_makefile_filename()', sysconfig.get_makefile_filename()),
            ('get_config_h_filename()', sysconfig.get_config_h_filename())):
            with self.subTest(call):
                cmd[2] = 'import sysconfig; print(sysconfig.%s)' % call
                out, err = check_output(cmd)
                self.assertEqual(out.strip(), expected.encode(), err)

    if sys.platform == 'win32':
        ENV_SUBDIRS = (
            ('Scripts',),
            ('Include',),
            ('Lib',),
            ('Lib', 'site-packages'),
        )
    else:
        ENV_SUBDIRS = (
            ('bin',),
            ('include',),
            ('lib',),
            ('lib', 'python%d.%d' % sys.version_info[:2]),
            ('lib', 'python%d.%d' % sys.version_info[:2], 'site-packages'),
        )

    def create_contents(self, paths, filename):
        """
        Create some files in the environment which are unrelated
        to the virtual environment.
        """
        for subdirs in paths:
            d = os.path.join(self.env_dir, *subdirs)
            os.mkdir(d)
            fn = os.path.join(d, filename)
            with open(fn, 'wb') as f:
                f.write(b'Still here?')

    def test_overwrite_existing(self):
        """
        Test creating environment in an existing directory.
        """
        self.create_contents(self.ENV_SUBDIRS, 'foo')
        venv.create(self.env_dir)
        for subdirs in self.ENV_SUBDIRS:
            fn = os.path.join(self.env_dir, *(subdirs + ('foo',)))
            self.assertTrue(os.path.exists(fn))
            with open(fn, 'rb') as f:
                self.assertEqual(f.read(), b'Still here?')

        builder = venv.EnvBuilder(clear=True)
        builder.create(self.env_dir)
        for subdirs in self.ENV_SUBDIRS:
            fn = os.path.join(self.env_dir, *(subdirs + ('foo',)))
            self.assertFalse(os.path.exists(fn))

    def clear_directory(self, path):
        for fn in os.listdir(path):
            fn = os.path.join(path, fn)
            if os.path.islink(fn) or os.path.isfile(fn):
                os.remove(fn)
            elif os.path.isdir(fn):
                rmtree(fn)

    def test_unoverwritable_fails(self):
        #create a file clashing with directories in the env dir
        for paths in self.ENV_SUBDIRS[:3]:
            fn = os.path.join(self.env_dir, *paths)
            with open(fn, 'wb') as f:
                f.write(b'')
            self.assertRaises((ValueError, OSError), venv.create, self.env_dir)
            self.clear_directory(self.env_dir)

    def test_upgrade(self):
        """
        Test upgrading an existing environment directory.
        """
        # See Issue #21643: the loop needs to run twice to ensure
        # that everything works on the upgrade (the first run just creates
        # the venv).
        for upgrade in (False, True):
            builder = venv.EnvBuilder(upgrade=upgrade)
            self.run_with_capture(builder.create, self.env_dir)
            self.isdir(self.bindir)
            self.isdir(self.include)
            self.isdir(*self.lib)
            fn = self.get_env_file(self.bindir, self.exe)
            if not os.path.exists(fn):
                # diagnostics for Windows buildbot failures
                bd = self.get_env_file(self.bindir)
                print('Contents of %r:' % bd)
                print('    %r' % os.listdir(bd))
            self.assertTrue(os.path.exists(fn), 'File %r should exist.' % fn)

    def test_isolation(self):
        """
        Test isolation from system site-packages
        """
        for ssp, s in ((True, 'true'), (False, 'false')):
            builder = venv.EnvBuilder(clear=True, system_site_packages=ssp)
            builder.create(self.env_dir)
            data = self.get_text_file_contents('pyvenv.cfg')
            self.assertIn('include-system-site-packages = %s\n' % s, data)

    @unittest.skipUnless(can_symlink(), 'Needs symlinks')
    def test_symlinking(self):
        """
        Test symlinking works as expected
        """
        for usl in (False, True):
            builder = venv.EnvBuilder(clear=True, symlinks=usl)
            builder.create(self.env_dir)
            fn = self.get_env_file(self.bindir, self.exe)
            # Don't test when False, because e.g. 'python' is always
            # symlinked to 'python3.3' in the env, even when symlinking in
            # general isn't wanted.
            if usl:
                if self.cannot_link_exe:
                    # Symlinking is skipped when our executable is already a
                    # special app symlink
                    self.assertFalse(os.path.islink(fn))
                else:
                    self.assertTrue(os.path.islink(fn))

    # If a venv is created from a source build and that venv is used to
    # run the test, the pyvenv.cfg in the venv created in the test will
    # point to the venv being used to run the test, and we lose the link
    # to the source build - so Python can't initialise properly.
    @requireVenvCreate
    def test_executable(self):
        """
        Test that the sys.executable value is as expected.
        """
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        envpy = os.path.join(os.path.realpath(self.env_dir),
                             self.bindir, self.exe)
        out, err = check_output([envpy, '-c',
            'import sys; print(sys.executable)'])
        self.assertEqual(out.strip(), envpy.encode())

    @unittest.skipUnless(can_symlink(), 'Needs symlinks')
    def test_executable_symlinks(self):
        """
        Test that the sys.executable value is as expected.
        """
        rmtree(self.env_dir)
        builder = venv.EnvBuilder(clear=True, symlinks=True)
        builder.create(self.env_dir)
        envpy = os.path.join(os.path.realpath(self.env_dir),
                             self.bindir, self.exe)
        out, err = check_output([envpy, '-c',
            'import sys; print(sys.executable)'])
        self.assertEqual(out.strip(), envpy.encode())

    @unittest.skipUnless(os.name == 'nt', 'only relevant on Windows')
    def test_unicode_in_batch_file(self):
        """
        Test handling of Unicode paths
        """
        rmtree(self.env_dir)
        env_dir = os.path.join(os.path.realpath(self.env_dir), 'ϼўТλФЙ')
        builder = venv.EnvBuilder(clear=True)
        builder.create(env_dir)
        activate = os.path.join(env_dir, self.bindir, 'activate.bat')
        envpy = os.path.join(env_dir, self.bindir, self.exe)
        out, err = check_output(
            [activate, '&', self.exe, '-c', 'print(0)'],
            encoding='oem',
        )
        self.assertEqual(out.strip(), '0')

    @requireVenvCreate
    def test_multiprocessing(self):
        """
        Test that the multiprocessing is able to spawn.
        """
        # bpo-36342: Instantiation of a Pool object imports the
        # multiprocessing.synchronize module. Skip the test if this module
        # cannot be imported.
        skip_if_broken_multiprocessing_synchronize()

        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        envpy = os.path.join(os.path.realpath(self.env_dir),
                             self.bindir, self.exe)
        out, err = check_output([envpy, '-c',
            'from multiprocessing import Pool; '
            'pool = Pool(1); '
            'print(pool.apply_async("Python".lower).get(3)); '
            'pool.terminate()'])
        self.assertEqual(out.strip(), "python".encode())

    @requireVenvCreate
    def test_multiprocessing_recursion(self):
        """
        Test that the multiprocessing is able to spawn itself
        """
        skip_if_broken_multiprocessing_synchronize()

        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        envpy = os.path.join(os.path.realpath(self.env_dir),
                             self.bindir, self.exe)
        script = os.path.join(TEST_HOME_DIR, '_test_venv_multiprocessing.py')
        subprocess.check_call([envpy, "-I", script])

    @unittest.skipIf(os.name == 'nt', 'not relevant on Windows')
    def test_deactivate_with_strict_bash_opts(self):
        bash = shutil.which("bash")
        if bash is None:
            self.skipTest("bash required for this test")
        rmtree(self.env_dir)
        builder = venv.EnvBuilder(clear=True)
        builder.create(self.env_dir)
        activate = os.path.join(self.env_dir, self.bindir, "activate")
        test_script = os.path.join(self.env_dir, "test_strict.sh")
        with open(test_script, "w") as f:
            f.write("set -euo pipefail\n"
                    f"source {activate}\n"
                    "deactivate\n")
        out, err = check_output([bash, test_script])
        self.assertEqual(out, "".encode())
        self.assertEqual(err, "".encode())


    @unittest.skipUnless(sys.platform == 'darwin', 'only relevant on macOS')
    def test_macos_env(self):
        rmtree(self.env_dir)
        builder = venv.EnvBuilder()
        builder.create(self.env_dir)

        envpy = os.path.join(os.path.realpath(self.env_dir),
                             self.bindir, self.exe)
        out, err = check_output([envpy, '-c',
            'import os; print("__PYVENV_LAUNCHER__" in os.environ)'])
        self.assertEqual(out.strip(), 'False'.encode())

    def test_pathsep_error(self):
        """
        Test that venv creation fails when the target directory contains
        the path separator.
        """
        rmtree(self.env_dir)
        bad_itempath = self.env_dir + os.pathsep
        self.assertRaises(ValueError, venv.create, bad_itempath)
        self.assertRaises(ValueError, venv.create, FakePath(bad_itempath))

    @unittest.skipIf(os.name == 'nt', 'not relevant on Windows')
    @requireVenvCreate
    def test_zippath_from_non_installed_posix(self):
        """
        Test that when create venv from non-installed python, the zip path
        value is as expected.
        """
        rmtree(self.env_dir)
        # First try to create a non-installed python. It's not a real full
        # functional non-installed python, but enough for this test.
        platlibdir = sys.platlibdir
        non_installed_dir = os.path.realpath(tempfile.mkdtemp())
        self.addCleanup(rmtree, non_installed_dir)
        bindir = os.path.join(non_installed_dir, self.bindir)
        os.mkdir(bindir)
        shutil.copy2(sys.executable, bindir)
        libdir = os.path.join(non_installed_dir, platlibdir, self.lib[1])
        os.makedirs(libdir)
        landmark = os.path.join(libdir, "os.py")
        stdlib_zip = "python%d%d.zip" % sys.version_info[:2]
        zip_landmark = os.path.join(non_installed_dir,
                                    platlibdir,
                                    stdlib_zip)
        additional_pythonpath_for_non_installed = []

        # Copy stdlib files to the non-installed python so venv can
        # correctly calculate the prefix.
        for eachpath in sys.path:
            if eachpath.endswith(".zip"):
                if os.path.isfile(eachpath):
                    shutil.copyfile(
                        eachpath,
                        os.path.join(non_installed_dir, platlibdir))
            elif os.path.isfile(os.path.join(eachpath, "os.py")):
                names = os.listdir(eachpath)
                ignored_names = copy_python_src_ignore(eachpath, names)
                for name in names:
                    if name in ignored_names:
                        continue
                    if name == "site-packages":
                        continue
                    fn = os.path.join(eachpath, name)
                    if os.path.isfile(fn):
                        shutil.copy(fn, libdir)
                    elif os.path.isdir(fn):
                        shutil.copytree(fn, os.path.join(libdir, name),
                                        ignore=copy_python_src_ignore)
            else:
                additional_pythonpath_for_non_installed.append(
                    eachpath)
        cmd = [os.path.join(non_installed_dir, self.bindir, self.exe),
               "-m",
               "venv",
               "--without-pip",
               self.env_dir]
        # Our fake non-installed python is not fully functional because
        # it cannot find the extensions. Set PYTHONPATH so it can run the
        # venv module correctly.
        pythonpath = os.pathsep.join(
            additional_pythonpath_for_non_installed)
        # For python built with shared enabled. We need to set
        # LD_LIBRARY_PATH so the non-installed python can find and link
        # libpython.so
        ld_library_path = sysconfig.get_config_var("LIBDIR")
        if not ld_library_path or sysconfig.is_python_build():
            ld_library_path = os.path.abspath(os.path.dirname(sys.executable))
        if sys.platform == 'darwin':
            ld_library_path_env = "DYLD_LIBRARY_PATH"
        else:
            ld_library_path_env = "LD_LIBRARY_PATH"
        child_env = {
                "PYTHONPATH": pythonpath,
                ld_library_path_env: ld_library_path,
        }
        if asan_options := os.environ.get("ASAN_OPTIONS"):
            # prevent https://github.com/python/cpython/issues/104839
            child_env["ASAN_OPTIONS"] = asan_options
        subprocess.check_call(cmd, env=child_env)
        envpy = os.path.join(self.env_dir, self.bindir, self.exe)
        # Now check the venv created from the non-installed python has
        # correct zip path in pythonpath.
        cmd = [envpy, '-S', '-c', 'import sys; print(sys.path)']
        out, err = check_output(cmd)
        self.assertTrue(zip_landmark.encode() in out)

    def test_activate_shell_script_has_no_dos_newlines(self):
        """
        Test that the `activate` shell script contains no CR LF.
        This is relevant for Cygwin, as the Windows build might have
        converted line endings accidentally.
        """
        venv_dir = pathlib.Path(self.env_dir)
        rmtree(venv_dir)
        [[scripts_dir], *_] = self.ENV_SUBDIRS
        script_path = venv_dir / scripts_dir / "activate"
        venv.create(venv_dir)
        with open(script_path, 'rb') as script:
            for i, line in enumerate(script, 1):
                error_message = f"CR LF found in line {i}"
                self.assertFalse(line.endswith(b'\r\n'), error_message)

    def test_venv_same_path(self):
        same_path = venv.EnvBuilder._same_path
        if sys.platform == 'win32':
            # Case-insensitive, and handles short/long names
            tests = [
                (True, TESTFN, TESTFN),
                (True, TESTFN.lower(), TESTFN.upper()),
            ]
            import _winapi
            # ProgramFiles is the most reliable path that will have short/long
            progfiles = os.getenv('ProgramFiles')
            if progfiles:
                tests = [
                    *tests,
                    (True, progfiles, progfiles),
                    (True, _winapi.GetShortPathName(progfiles), _winapi.GetLongPathName(progfiles)),
                ]
        else:
            # Just a simple case-sensitive comparison
            tests = [
                (True, TESTFN, TESTFN),
                (False, TESTFN.lower(), TESTFN.upper()),
            ]
        for r, path1, path2 in tests:
            with self.subTest(f"{path1}-{path2}"):
                if r:
                    self.assertTrue(same_path(path1, path2))
                else:
                    self.assertFalse(same_path(path1, path2))

@requireVenvCreate
class EnsurePipTest(BaseTest):
    """Test venv module installation of pip."""
    def assert_pip_not_installed(self):
        envpy = os.path.join(os.path.realpath(self.env_dir),
                             self.bindir, self.exe)
        out, err = check_output([envpy, '-c',
            'try:\n import pip\nexcept ImportError:\n print("OK")'])
        # We force everything to text, so unittest gives the detailed diff
        # if we get unexpected results
        err = err.decode("latin-1") # Force to text, prevent decoding errors
        self.assertEqual(err, "")
        out = out.decode("latin-1") # Force to text, prevent decoding errors
        self.assertEqual(out.strip(), "OK")


    def test_no_pip_by_default(self):
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        self.assert_pip_not_installed()

    def test_explicit_no_pip(self):
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir, with_pip=False)
        self.assert_pip_not_installed()

    def test_devnull(self):
        # Fix for issue #20053 uses os.devnull to force a config file to
        # appear empty. However http://bugs.python.org/issue20541 means
        # that doesn't currently work properly on Windows. Once that is
        # fixed, the "win_location" part of test_with_pip should be restored
        with open(os.devnull, "rb") as f:
            self.assertEqual(f.read(), b"")

        self.assertTrue(os.path.exists(os.devnull))

    def do_test_with_pip(self, system_site_packages):
        rmtree(self.env_dir)
        with EnvironmentVarGuard() as envvars:
            # pip's cross-version compatibility may trigger deprecation
            # warnings in current versions of Python. Ensure related
            # environment settings don't cause venv to fail.
            envvars["PYTHONWARNINGS"] = "ignore"
            # ensurepip is different enough from a normal pip invocation
            # that we want to ensure it ignores the normal pip environment
            # variable settings. We set PIP_NO_INSTALL here specifically
            # to check that ensurepip (and hence venv) ignores it.
            # See http://bugs.python.org/issue19734
            envvars["PIP_NO_INSTALL"] = "1"
            # Also check that we ignore the pip configuration file
            # See http://bugs.python.org/issue20053
            with tempfile.TemporaryDirectory() as home_dir:
                envvars["HOME"] = home_dir
                bad_config = "[global]\nno-install=1"
                # Write to both config file names on all platforms to reduce
                # cross-platform variation in test code behaviour
                win_location = ("pip", "pip.ini")
                posix_location = (".pip", "pip.conf")
                # Skips win_location due to http://bugs.python.org/issue20541
                for dirname, fname in (posix_location,):
                    dirpath = os.path.join(home_dir, dirname)
                    os.mkdir(dirpath)
                    fpath = os.path.join(dirpath, fname)
                    with open(fpath, 'w') as f:
                        f.write(bad_config)

                # Actually run the create command with all that unhelpful
                # config in place to ensure we ignore it
                with self.nicer_error():
                    self.run_with_capture(venv.create, self.env_dir,
                                          system_site_packages=system_site_packages,
                                          with_pip=True)
        # Ensure pip is available in the virtual environment
        envpy = os.path.join(os.path.realpath(self.env_dir), self.bindir, self.exe)
        # Ignore DeprecationWarning since pip code is not part of Python
        out, err = check_output([envpy, '-W', 'ignore::DeprecationWarning',
               '-W', 'ignore::ImportWarning', '-I',
               '-m', 'pip', '--version'])
        # We force everything to text, so unittest gives the detailed diff
        # if we get unexpected results
        err = err.decode("latin-1") # Force to text, prevent decoding errors
        self.assertEqual(err, "")
        out = out.decode("latin-1") # Force to text, prevent decoding errors
        expected_version = "pip {}".format(ensurepip.version())
        self.assertEqual(out[:len(expected_version)], expected_version)
        env_dir = os.fsencode(self.env_dir).decode("latin-1")
        self.assertIn(env_dir, out)

        # http://bugs.python.org/issue19728
        # Check the private uninstall command provided for the Windows
        # installers works (at least in a virtual environment)
        with EnvironmentVarGuard() as envvars:
            with self.nicer_error():
                # It seems ensurepip._uninstall calls subprocesses which do not
                # inherit the interpreter settings.
                envvars["PYTHONWARNINGS"] = "ignore"
                out, err = check_output([envpy,
                    '-W', 'ignore::DeprecationWarning',
                    '-W', 'ignore::ImportWarning', '-I',
                    '-m', 'ensurepip._uninstall'])
        # We force everything to text, so unittest gives the detailed diff
        # if we get unexpected results
        err = err.decode("latin-1") # Force to text, prevent decoding errors
        # Ignore the warning:
        #   "The directory '$HOME/.cache/pip/http' or its parent directory
        #    is not owned by the current user and the cache has been disabled.
        #    Please check the permissions and owner of that directory. If
        #    executing pip with sudo, you may want sudo's -H flag."
        # where $HOME is replaced by the HOME environment variable.
        err = re.sub("^(WARNING: )?The directory .* or its parent directory "
                     "is not owned or is not writable by the current user.*$", "",
                     err, flags=re.MULTILINE)
        self.assertEqual(err.rstrip(), "")
        # Being fairly specific regarding the expected behaviour for the
        # initial bundling phase in Python 3.4. If the output changes in
        # future pip versions, this test can likely be relaxed further.
        out = out.decode("latin-1") # Force to text, prevent decoding errors
        self.assertIn("Successfully uninstalled pip", out)
        # Check pip is now gone from the virtual environment. This only
        # applies in the system_site_packages=False case, because in the
        # other case, pip may still be available in the system site-packages
        if not system_site_packages:
            self.assert_pip_not_installed()

    @contextlib.contextmanager
    def nicer_error(self):
        """
        Capture output from a failed subprocess for easier debugging.

        The output this handler produces can be a little hard to read,
        but at least it has all the details.
        """
        try:
            yield
        except subprocess.CalledProcessError as exc:
            out = (exc.output or b'').decode(errors="replace")
            err = (exc.stderr or b'').decode(errors="replace")
            self.fail(
                f"{exc}\n\n"
                f"**Subprocess Output**\n{out}\n\n"
                f"**Subprocess Error**\n{err}"
            )

    @requires_venv_with_pip()
    @requires_resource('cpu')
    def test_with_pip(self):
        self.do_test_with_pip(False)
        self.do_test_with_pip(True)


if __name__ == "__main__":
    unittest.main()
