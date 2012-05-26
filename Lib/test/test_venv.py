#!/usr/bin/env python
#
# Copyright 2011 by Vinay Sajip. All Rights Reserved.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appear in all copies and that
# both that copyright notice and this permission notice appear in
# supporting documentation, and that the name of Vinay Sajip
# not be used in advertising or publicity pertaining to distribution
# of the software without specific, written prior permission.
# VINAY SAJIP DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
# ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
# VINAY SAJIP BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
# ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
# IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
# OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

"""Test harness for the venv module. Run all tests.

Copyright (C) 2011 Vinay Sajip. All Rights Reserved.
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
            self.ps3name = 'pysetup3-script.py'
            self.lib = ('Lib',)
            self.include = 'Include'
            self.exe = 'python.exe'
        else:
            self.bindir = 'bin'
            self.ps3name = 'pysetup3'
            self.lib = ('lib', 'python%s' % sys.version[:3])
            self.include = 'include'
            self.exe = 'python'

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

    def test_defaults(self):
        """
        Test the create function with default arguments.
        """
        def isdir(*args):
            fn = self.get_env_file(*args)
            self.assertTrue(os.path.isdir(fn))

        shutil.rmtree(self.env_dir)
        self.run_with_capture(venv.create, self.env_dir)
        isdir(self.bindir)
        isdir(self.include)
        isdir(*self.lib)
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
        self.assertTrue(os.path.exists(fn))

    def test_overwrite_existing(self):
        """
        Test control of overwriting an existing environment directory.
        """
        self.assertRaises(ValueError, venv.create, self.env_dir)
        builder = venv.EnvBuilder(clear=True)
        builder.create(self.env_dir)

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
