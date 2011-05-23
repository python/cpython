"""Tests for the uninstall command."""
import os
import sys

from packaging.database import disable_cache, enable_cache
from packaging.run import main
from packaging.errors import PackagingError
from packaging.install import remove
from packaging.command.install_dist import install_dist

from packaging.tests import unittest, support

SETUP_CFG = """
[metadata]
name = %(name)s
version = %(version)s

[files]
packages =
    %(pkg)s
    %(pkg)s.sub
"""


class UninstallTestCase(support.TempdirManager,
                        support.LoggingCatcher,
                        support.EnvironRestorer,
                        unittest.TestCase):

    restore_environ = ['PLAT']

    def setUp(self):
        super(UninstallTestCase, self).setUp()
        self.addCleanup(setattr, sys, 'stdout', sys.stdout)
        self.addCleanup(setattr, sys, 'stderr', sys.stderr)
        self.addCleanup(os.chdir, os.getcwd())
        self.addCleanup(enable_cache)
        self.root_dir = self.mkdtemp()
        self.cwd = os.getcwd()
        disable_cache()

    def tearDown(self):
        os.chdir(self.cwd)
        super(UninstallTestCase, self).tearDown()

    def run_setup(self, *args):
        # run setup with args
        args = ['run'] + list(args)
        dist = main(args)
        return dist

    def get_path(self, dist, name):
        cmd = install_dist(dist)
        cmd.prefix = self.root_dir
        cmd.finalize_options()
        return getattr(cmd, 'install_' + name)

    def make_dist(self, name='Foo', **kw):
        kw['name'] = name
        pkg = name.lower()
        if 'version' not in kw:
            kw['version'] = '0.1'
        project_dir, dist = self.create_dist(**kw)
        kw['pkg'] = pkg

        pkg_dir = os.path.join(project_dir, pkg)
        os.mkdir(pkg_dir)
        os.mkdir(os.path.join(pkg_dir, 'sub'))

        self.write_file((project_dir, 'setup.cfg'), SETUP_CFG % kw)
        self.write_file((pkg_dir, '__init__.py'), '#')
        self.write_file((pkg_dir, pkg + '_utils.py'), '#')
        self.write_file((pkg_dir, 'sub', '__init__.py'), '#')
        self.write_file((pkg_dir, 'sub', pkg + '_utils.py'), '#')

        return project_dir

    def install_dist(self, name='Foo', dirname=None, **kw):
        if not dirname:
            dirname = self.make_dist(name, **kw)
        os.chdir(dirname)
        dist = self.run_setup('install_dist', '--prefix=' + self.root_dir)
        install_lib = self.get_path(dist, 'purelib')
        return dist, install_lib

    def test_uninstall_unknow_distribution(self):
        self.assertRaises(PackagingError, remove, 'Foo',
                          paths=[self.root_dir])

    @unittest.skipIf(sys.platform == 'win32', 'deactivated for now')
    def test_uninstall(self):
        dist, install_lib = self.install_dist()
        self.assertIsFile(install_lib, 'foo', '__init__.py')
        self.assertIsFile(install_lib, 'foo', 'sub', '__init__.py')
        self.assertIsFile(install_lib, 'Foo-0.1.dist-info', 'RECORD')
        remove('Foo', paths=[install_lib])
        self.assertIsNotFile(install_lib, 'foo', 'sub', '__init__.py')
        self.assertIsNotFile(install_lib, 'Foo-0.1.dist-info', 'RECORD')


def test_suite():
    return unittest.makeSuite(UninstallTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
