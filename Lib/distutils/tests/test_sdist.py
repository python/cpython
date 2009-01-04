"""Tests for distutils.command.sdist."""
import os
import unittest
import shutil
import zipfile
from os.path import join

from distutils.command.sdist import sdist
from distutils.core import Distribution
from distutils.tests.test_config import PyPIRCCommandTestCase

CURDIR = os.path.dirname(__file__)
TEMP_PKG = join(CURDIR, 'temppkg')

SETUP_PY = """
from distutils.core import setup
import somecode

setup(name='fake')
"""

MANIFEST_IN = """
recursive-include somecode *
"""

class sdistTestCase(PyPIRCCommandTestCase):

    def setUp(self):
        PyPIRCCommandTestCase.setUp(self)
        self.old_path = os.getcwd()

    def tearDown(self):
        os.chdir(self.old_path)
        if os.path.exists(TEMP_PKG):
            shutil.rmtree(TEMP_PKG)
        PyPIRCCommandTestCase.tearDown(self)

    def _write(self, path, content):
        f = open(path, 'w')
        try:
            f.write(content)
        finally:
            f.close()

    def test_prune_file_list(self):
        # this test creates a package with some vcs dirs in it
        # and launch sdist to make sure they get pruned
        # on all systems
        if not os.path.exists(TEMP_PKG):
            os.mkdir(TEMP_PKG)
        os.mkdir(join(TEMP_PKG, 'somecode'))

        # creating a MANIFEST, a package, and a README
        self._write(join(TEMP_PKG, 'MANIFEST.in'), MANIFEST_IN)
        self._write(join(TEMP_PKG, 'README'), 'xxx')
        self._write(join(TEMP_PKG, 'somecode', '__init__.py'), '#')
        self._write(join(TEMP_PKG, 'setup.py'), SETUP_PY)

        # creating VCS directories with some files in them
        os.mkdir(join(TEMP_PKG, 'somecode', '.svn'))
        self._write(join(TEMP_PKG, 'somecode', '.svn', 'ok.py'), 'xxx')

        os.mkdir(join(TEMP_PKG, 'somecode', '.hg'))
        self._write(join(TEMP_PKG, 'somecode', '.hg',
                         'ok'), 'xxx')

        os.mkdir(join(TEMP_PKG, 'somecode', '.git'))
        self._write(join(TEMP_PKG, 'somecode', '.git',
                         'ok'), 'xxx')

        os.chdir(TEMP_PKG)

        # now building a sdist
        dist = Distribution()
        dist.script_name = 'setup.py'
        dist.metadata.name = 'fake'
        dist.metadata.version = '1.0'
        dist.metadata.url = 'http://xxx'
        dist.metadata.author = dist.metadata.author_email = 'xxx'
        dist.packages = ['somecode']
        dist.include_package_data = True
        cmd = sdist(dist)
        cmd.manifest = 'MANIFEST'
        cmd.template = 'MANIFEST.in'
        cmd.dist_dir = 'dist'

        # zip is available universally
        # (tar might not be installed under win32)
        cmd.formats = ['zip']
        cmd.run()

        # now let's check what we have
        dist_folder = join(TEMP_PKG, 'dist')
        files = os.listdir(dist_folder)
        self.assertEquals(files, ['fake-1.0.zip'])

        zip_file = zipfile.ZipFile(join(dist_folder, 'fake-1.0.zip'))
        try:
            content = zip_file.namelist()
        finally:
            zip_file.close()

        # making sure everything has been pruned correctly
        self.assertEquals(len(content), 4)

def test_suite():
    return unittest.makeSuite(sdistTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
