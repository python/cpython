"""
Test harness for the venv module.

Copyright (C) 2011-2012 Vinay Sajip.
Licensed to the PSF under a contributor agreement.
"""

import os
import os.path
import shutil
import subprocess
import sys
import tempfile
from test.support import (captured_stdout, captured_stderr, run_unittest,
                          can_symlink, EnvironmentVarGuard)
import unittest
import venv

skipInVenv = unittest.skipIf(sys.prefix != sys.base_prefix,
                             'Test not appropriate in a venv')


class BaseTest(unittest.TestCase):
    """Base class for venv tests."""

    def setUp(self):
        self.env_dir = os.path.realpath(tempfile.mkdtemp())
        if os.name == 'nt':
            self.bindir = 'Scripts'
            self.lib = ('Lib',)
            self.include = 'Include'
        else:
            self.bindir = 'bin'
            self.lib = ('lib', 'python%s' % sys.version[:3])
            self.include = 'include'
        if sys.platform == 'darwin' and '__PYVENV_LAUNCHER__' in os.environ:
            executable = os.environ['__PYVENV_LAUNCHER__']
        else:
            executable = sys.executable
        self.exe = os.path.split(executable)[-1]

    def tearDown(self):
        shutil.rmtree(self.env_dir)

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
        shutil.rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        self.isdir(self.bindir)
        self.isdir(self.include)
        self.isdir(*self.lib)
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

    @skipInVenv
    def test_prefixes(self):
        """
        Test that the prefix values are as expected.
        """
        #check our prefixes
        self.assertEqual(sys.base_prefix, sys.prefix)
        self.assertEqual(sys.base_exec_prefix, sys.exec_prefix)

        # check a venv's prefixes
        shutil.rmtree(self.env_dir)
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
                shutil.rmtree(fn)

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
        builder = venv.EnvBuilder(upgrade=True)
        self.run_with_capture(builder.create, self.env_dir)
        self.isdir(self.bindir)
        self.isdir(self.include)
        self.isdir(*self.lib)
        fn = self.get_env_file(self.bindir, self.exe)
        if not os.path.exists(fn):  # diagnostics for Windows buildbot failures
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
        shutil.rmtree(self.env_dir)
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
        shutil.rmtree(self.env_dir)
        builder = venv.EnvBuilder(clear=True, symlinks=True)
        builder.create(self.env_dir)
        envpy = os.path.join(os.path.realpath(self.env_dir), self.bindir, self.exe)
        cmd = [envpy, '-c', 'import sys; print(sys.executable)']
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        self.assertEqual(out.strip(), envpy.encode())


@skipInVenv
class EnsurePipTest(BaseTest):
    """Test venv module installation of pip."""

    def test_no_pip_by_default(self):
        shutil.rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        envpy = os.path.join(os.path.realpath(self.env_dir), self.bindir, self.exe)
        try_import = 'try:\n import pip\nexcept ImportError:\n print("OK")'
        cmd = [envpy, '-c', try_import]
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        self.assertEqual(err, b"")
        self.assertEqual(out.strip(), b"OK")

    def test_explicit_no_pip(self):
        shutil.rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir, with_pip=False)
        envpy = os.path.join(os.path.realpath(self.env_dir), self.bindir, self.exe)
        try_import = 'try:\n import pip\nexcept ImportError:\n print("OK")'
        cmd = [envpy, '-c', try_import]
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        self.assertEqual(err, b"")
        self.assertEqual(out.strip(), b"OK")

    def test_with_pip(self):
        shutil.rmtree(self.env_dir)
        with EnvironmentVarGuard() as envvars:
            # pip's cross-version compatibility may trigger deprecation
            # warnings in current versions of Python. Ensure related
            # environment settings don't cause venv to fail.
            envvars["PYTHONWARNINGS"] = "e"
            try:
                self.run_with_capture(venv.create, self.env_dir, with_pip=True)
            except subprocess.CalledProcessError as exc:
                # The output this produces can be a little hard to read, but
                # least it has all the details
                details = exc.output.decode(errors="replace")
                msg = "{}\n\n**Subprocess Output**\n{}".format(exc, details)
                self.fail(msg)
        envpy = os.path.join(os.path.realpath(self.env_dir), self.bindir, self.exe)
        cmd = [envpy, '-m', 'pip', '--version']
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = p.communicate()
        # We force everything to text, so unittest gives the detailed diff
        # if we get unexpected results
        err = err.decode("latin-1") # Force to text, prevent decoding errors
        self.assertEqual(err, "")
        out = out.decode("latin-1") # Force to text, prevent decoding errors
        env_dir = os.fsencode(self.env_dir).decode("latin-1")
        self.assertTrue(out.startswith("pip"))
        self.assertIn(env_dir, out)


def test_main():
    run_unittest(BasicTest, EnsurePipTest)

if __name__ == "__main__":
    test_main()
