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
                          can_symlink)
import unittest
import venv

class BaseTest(unittest.TestCase):
    """Base class for venv tests."""

    def setUp(self):
        self.env_dir = os.path.realpath(tempfile.mkdtemp())
        if os.name == 'nt':
            self.bindir = 'Scripts'
            self.pydocname = 'pydoc.py'
            self.lib = ('Lib',)
            self.include = 'Include'
        else:
            self.bindir = 'bin'
            self.pydocname = 'pydoc'
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
        data = self.get_text_file_contents(self.bindir, self.pydocname)
        self.assertTrue(data.startswith('#!%s%s' % (self.env_dir, os.sep)))
        fn = self.get_env_file(self.bindir, self.exe)
        if not os.path.exists(fn):  # diagnostics for Windows buildbot failures
            bd = self.get_env_file(self.bindir)
            print('Contents of %r:' % bd)
            print('    %r' % os.listdir(bd))
        self.assertTrue(os.path.exists(fn), 'File %r should exist.' % fn)

    @unittest.skipIf(sys.prefix != sys.base_prefix, 'Test not appropriate '
                     'in a venv')
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

    def test_overwrite_existing(self):
        """
        Test control of overwriting an existing environment directory.
        """
        self.assertRaises(ValueError, venv.create, self.env_dir)
        builder = venv.EnvBuilder(clear=True)
        builder.create(self.env_dir)

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
    @unittest.skipIf(sys.prefix != sys.base_prefix, 'Test not appropriate '
                     'in a venv')
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

def test_main():
    run_unittest(BasicTest)

if __name__ == "__main__":
    test_main()
