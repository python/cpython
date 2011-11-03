"""Tests for packaging.command.install_data."""
import os
import sys
import imp

from packaging.tests import unittest, support
from packaging.command.install_lib import install_lib
from packaging.compiler.extension import Extension
from packaging.errors import PackagingOptionError


class InstallLibTestCase(support.TempdirManager,
                         support.LoggingCatcher,
                         support.EnvironRestorer,
                         unittest.TestCase):

    restore_environ = ['PYTHONPATH']

    def test_finalize_options(self):
        dist = self.create_dist()[1]
        cmd = install_lib(dist)

        cmd.finalize_options()
        self.assertTrue(cmd.compile)
        self.assertEqual(cmd.optimize, 0)

        # optimize must be 0, 1, or 2
        cmd.optimize = 'foo'
        self.assertRaises(PackagingOptionError, cmd.finalize_options)
        cmd.optimize = '4'
        self.assertRaises(PackagingOptionError, cmd.finalize_options)

        cmd.optimize = '2'
        cmd.finalize_options()
        self.assertEqual(cmd.optimize, 2)

    def test_byte_compile(self):
        project_dir, dist = self.create_dist()
        os.chdir(project_dir)
        cmd = install_lib(dist)
        cmd.compile = True
        cmd.optimize = 1

        f = os.path.join(project_dir, 'foo.py')
        self.write_file(f, '# python file')
        cmd.byte_compile([f])
        pyc_file = imp.cache_from_source('foo.py', debug_override=True)
        pyo_file = imp.cache_from_source('foo.py', debug_override=False)
        self.assertTrue(os.path.exists(pyc_file))
        self.assertTrue(os.path.exists(pyo_file))

    def test_byte_compile_under_B(self):
        # make sure byte compilation works under -B (dont_write_bytecode)
        self.addCleanup(setattr, sys, 'dont_write_bytecode',
                        sys.dont_write_bytecode)
        sys.dont_write_bytecode = True
        self.test_byte_compile()

    def test_get_outputs(self):
        project_dir, dist = self.create_dist()
        os.chdir(project_dir)
        os.mkdir('spam')
        cmd = install_lib(dist)

        # setting up a dist environment
        cmd.compile = True
        cmd.optimize = 1
        cmd.install_dir = self.mkdtemp()
        f = os.path.join(project_dir, 'spam', '__init__.py')
        self.write_file(f, '# python package')
        cmd.distribution.ext_modules = [Extension('foo', ['xxx'])]
        cmd.distribution.packages = ['spam']

        # make sure the build_lib is set the temp dir  # XXX what?  this is not
        # needed in the same distutils test and should work without manual
        # intervention
        build_dir = os.path.split(project_dir)[0]
        cmd.get_finalized_command('build_py').build_lib = build_dir

        # get_outputs should return 4 elements: spam/__init__.py, .pyc and
        # .pyo, foo.import-tag-abiflags.so / foo.pyd
        outputs = cmd.get_outputs()
        self.assertEqual(len(outputs), 4, outputs)

    def test_get_inputs(self):
        project_dir, dist = self.create_dist()
        os.chdir(project_dir)
        os.mkdir('spam')
        cmd = install_lib(dist)

        # setting up a dist environment
        cmd.compile = True
        cmd.optimize = 1
        cmd.install_dir = self.mkdtemp()
        f = os.path.join(project_dir, 'spam', '__init__.py')
        self.write_file(f, '# python package')
        cmd.distribution.ext_modules = [Extension('foo', ['xxx'])]
        cmd.distribution.packages = ['spam']

        # get_inputs should return 2 elements: spam/__init__.py and
        # foo.import-tag-abiflags.so / foo.pyd
        inputs = cmd.get_inputs()
        self.assertEqual(len(inputs), 2, inputs)


def test_suite():
    return unittest.makeSuite(InstallLibTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
