"""
Test harness for the venv module.

Copyright (C) 2011-2012 Vinay Sajip.
"""

import os
import os.path
import shutil
import sys
import tempfile
from test.support import (captured_stdout, captured_stderr, run_unittest,
                          can_symlink)
import unittest
import venv

class BaseTest(unittest.TestCase):
    """Base class for venv tests."""

    def setUp(self):
        self.env_dir = tempfile.mkdtemp()
        if os.name == 'nt':
            self.bindir = 'Scripts'
            self.ps3name = 'pysetup3.py'
            self.lib = ('Lib',)
            self.include = 'Include'
        else:
            self.bindir = 'bin'
            self.ps3name = 'pysetup3'
            self.lib = ('lib', 'python%s' % sys.version[:3])
            self.include = 'include'
        if sys.platform == 'darwin' and '__PYTHONV_LAUNCHER__' in os.environ:
            executable = os.environ['__PYTHONV_LAUNCHER__']
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
        if sys.platform == 'darwin' and ('__PYTHONV_LAUNCHER__'
                                         in os.environ):
            executable =  os.environ['__PYTHONV_LAUNCHER__']
        else:
            executable = sys.executable
        path = os.path.dirname(executable)
        self.assertIn('home = %s' % path, data)
        data = self.get_text_file_contents(self.bindir, self.ps3name)
        self.assertTrue(data.startswith('#!%s%s' % (self.env_dir, os.sep)))
        fn = self.get_env_file(self.bindir, self.exe)
        if not os.path.exists(fn):  # diagnostics for Windows buildbot failures
            bd = self.get_env_file(self.bindir)
            print('Contents of %r:' % bd)
            print('    %r' % os.listdir(bd))
        self.assertTrue(os.path.exists(fn), 'File %r should exist.' % fn)

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
            if (usl and sys.platform == 'darwin' and
                '__PYTHONV_LAUNCHER__' in os.environ):
                self.assertRaises(ValueError, builder.create, self.env_dir)
            else:
                builder.create(self.env_dir)
                fn = self.get_env_file(self.bindir, self.exe)
                # Don't test when False, because e.g. 'python' is always
                # symlinked to 'python3.3' in the env, even when symlinking in
                # general isn't wanted.
                if usl:
                    self.assertTrue(os.path.islink(fn))

def test_main():
    run_unittest(BasicTest)

if __name__ == "__main__":
    test_main()
