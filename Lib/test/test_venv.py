"""
Test harness for the venv module.

Copyright (C) 2011-2012 Vinay Sajip.
Licensed to the PSF under a contributor agreement.
"""

import ensurepip
import os
import os.path
import re
import struct
import subprocess
import sys
import tempfile
from test.support import (captured_stdout, captured_stderr, requires_zlib,
                          can_symlink, EnvironmentVarGuard, rmtree)
import unittest
import venv


try:
    import threading
except ImportError:
    threading = None

try:
    import ctypes
except ImportError:
    ctypes = None

skipInVenv = unittest.skipIf(sys.prefix != sys.base_prefix,
                             'Test not appropriate in a venv')

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
        if sys.platform == 'darwin' and '__PYVENV_LAUNCHER__' in os.environ:
            executable = os.environ['__PYVENV_LAUNCHER__']
        else:
            executable = sys.executable
        self.exe = os.path.split(executable)[-1]

    def tearDown(self):
        rmtree(self.env_dir)

    def run_with_capture(self, func, *args, **kwargs):
        with captured_stdout() as output:
            with captured_stderr() as error:
                func(*args, **kwargs)
        return output.getvalue(), error.getvalue()

    def get_env_file(self, *args):
        return os.path.join(self.env_dir, *args)

    def get_text_file_contents(self, *args):
        with open(self.get_env_file(*args), 'r') as f:
            result = f.read()
        return result

class BasicTest(BaseTest):
    """Test venv module functionality."""

    def isdir(self, *args):
        fn = self.get_env_file(*args)
        self.assertTrue(os.path.isdir(fn))

    def test_defaults(self):
        """
        Test the create function with default arguments.
        """
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
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
        if sys.platform == 'darwin' and ('__PYVENV_LAUNCHER__'
                                         in os.environ):
            executable =  os.environ['__PYVENV_LAUNCHER__']
        else:
            executable = sys.executable
        path = os.path.dirname(executable)
        self.assertIn('home = %s' % path, data)
        fn = self.get_env_file(self.bindir, self.exe)
        if not os.path.exists(fn):  # diagnostics for Windows buildbot failures
            bd = self.get_env_file(self.bindir)
            print('Contents of %r:' % bd)
            print('    %r' % os.listdir(bd))
        self.assertTrue(os.path.exists(fn), 'File %r should exist.' % fn)

    def test_prompt(self):
        env_name = os.path.split(self.env_dir)[1]

        builder = venv.EnvBuilder()
        context = builder.ensure_directories(self.env_dir)
        self.assertEqual(context.prompt, '(%s) ' % env_name)

        builder = venv.EnvBuilder(prompt='My prompt')
        context = builder.ensure_directories(self.env_dir)
        self.assertEqual(context.prompt, '(My prompt) ')

    @skipInVenv
    def test_prefixes(self):
        """
        Test that the prefix values are as expected.
        """
        #check our prefixes
        self.assertEqual(sys.base_prefix, sys.prefix)
        self.assertEqual(sys.base_exec_prefix, sys.exec_prefix)

        # check a venv's prefixes
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        envpy = os.path.join(self.env_dir, self.bindir, self.exe)
        cmd = [envpy, '-c', None]
        for prefix, expected in (
            ('prefix', self.env_dir),
            ('prefix', self.env_dir),
            ('base_prefix', sys.prefix),
            ('base_exec_prefix', sys.exec_prefix)):
            cmd[2] = 'import sys; print(sys.%s)' % prefix
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            out, err = p.communicate()
            self.assertEqual(out.strip(), expected.encode())

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
                self.assertTrue(os.path.islink(fn))

    # If a venv is created from a source build and that venv is used to
    # run the test, the pyvenv.cfg in the venv created in the test will
    # point to the venv being used to run the test, and we lose the link
    # to the source build - so Python can't initialise properly.
    @skipInVenv
    def test_executable(self):
        """
        Test that the sys.executable value is as expected.
        """
        rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        envpy = os.path.join(os.path.realpath(self.env_dir), self.bindir, self.exe)
        cmd = [envpy, '-c', 'import sys; print(sys.executable)']
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        self.assertEqual(out.strip(), envpy.encode())

    @unittest.skipUnless(can_symlink(), 'Needs symlinks')
    def test_executable_symlinks(self):
        """
        Test that the sys.executable value is as expected.
        """
        rmtree(self.env_dir)
        builder = venv.EnvBuilder(clear=True, symlinks=True)
        builder.create(self.env_dir)
        envpy = os.path.join(os.path.realpath(self.env_dir), self.bindir, self.exe)
        cmd = [envpy, '-c', 'import sys; print(sys.executable)']
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        self.assertEqual(out.strip(), envpy.encode())

    @unittest.skipUnless(os.name == 'nt', 'only relevant on Windows')
    def test_unicode_in_batch_file(self):
        """
        Test isolation from system site-packages
        """
        rmtree(self.env_dir)
        env_dir = os.path.join(os.path.realpath(self.env_dir), 'ϼўТλФЙ')
        builder = venv.EnvBuilder(clear=True)
        builder.create(env_dir)
        activate = os.path.join(env_dir, self.bindir, 'activate.bat')
        envpy = os.path.join(env_dir, self.bindir, self.exe)
        cmd = [activate, '&', self.exe, '-c', 'print(0)']
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, encoding='oem',
                             shell=True)
        out, err = p.communicate()
        print(err)
        self.assertEqual(out.strip(), '0')

@skipInVenv
class EnsurePipTest(BaseTest):
    """Test venv module installation of pip."""
    def assert_pip_not_installed(self):
        envpy = os.path.join(os.path.realpath(self.env_dir),
                             self.bindir, self.exe)
        try_import = 'try:\n import pip\nexcept ImportError:\n print("OK")'
        cmd = [envpy, '-c', try_import]
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
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

        # Issue #20541: os.path.exists('nul') is False on Windows
        if os.devnull.lower() == 'nul':
            self.assertFalse(os.path.exists(os.devnull))
        else:
            self.assertTrue(os.path.exists(os.devnull))

    def do_test_with_pip(self, system_site_packages):
        rmtree(self.env_dir)
        with EnvironmentVarGuard() as envvars:
            # pip's cross-version compatibility may trigger deprecation
            # warnings in current versions of Python. Ensure related
            # environment settings don't cause venv to fail.
            envvars["PYTHONWARNINGS"] = "e"
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
                try:
                    self.run_with_capture(venv.create, self.env_dir,
                                          system_site_packages=system_site_packages,
                                          with_pip=True)
                except subprocess.CalledProcessError as exc:
                    # The output this produces can be a little hard to read,
                    # but at least it has all the details
                    details = exc.output.decode(errors="replace")
                    msg = "{}\n\n**Subprocess Output**\n{}"
                    self.fail(msg.format(exc, details))
        # Ensure pip is available in the virtual environment
        envpy = os.path.join(os.path.realpath(self.env_dir), self.bindir, self.exe)
        cmd = [envpy, '-Im', 'pip', '--version']
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE)
        out, err = p.communicate()
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
        cmd = [envpy, '-Im', 'ensurepip._uninstall']
        with EnvironmentVarGuard() as envvars:
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE)
            out, err = p.communicate()
        # We force everything to text, so unittest gives the detailed diff
        # if we get unexpected results
        err = err.decode("latin-1") # Force to text, prevent decoding errors
        # Ignore the warning:
        #   "The directory '$HOME/.cache/pip/http' or its parent directory
        #    is not owned by the current user and the cache has been disabled.
        #    Please check the permissions and owner of that directory. If
        #    executing pip with sudo, you may want sudo's -H flag."
        # where $HOME is replaced by the HOME environment variable.
        err = re.sub("^The directory .* or its parent directory is not owned "
                     "by the current user .*$", "", err, flags=re.MULTILINE)
        self.assertEqual(err.rstrip(), "")
        # Being fairly specific regarding the expected behaviour for the
        # initial bundling phase in Python 3.4. If the output changes in
        # future pip versions, this test can likely be relaxed further.
        out = out.decode("latin-1") # Force to text, prevent decoding errors
        self.assertIn("Successfully uninstalled pip", out)
        self.assertIn("Successfully uninstalled setuptools", out)
        # Check pip is now gone from the virtual environment. This only
        # applies in the system_site_packages=False case, because in the
        # other case, pip may still be available in the system site-packages
        if not system_site_packages:
            self.assert_pip_not_installed()

    @unittest.skipUnless(threading, 'some dependencies of pip import threading'
                                    ' module unconditionally')
    # Issue #26610: pip/pep425tags.py requires ctypes
    @unittest.skipUnless(ctypes, 'pip requires ctypes')
    @requires_zlib
    def test_with_pip(self):
        self.do_test_with_pip(False)
        self.do_test_with_pip(True)

if __name__ == "__main__":
    unittest.main()
