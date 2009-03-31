"""Tests for distutils.command.sdist."""
import os
import unittest
import shutil
import zipfile
from os.path import join
import sys
import tempfile

from distutils.command.sdist import sdist
from distutils.core import Distribution
from distutils.tests.test_config import PyPIRCCommandTestCase
from distutils.errors import DistutilsExecError
from distutils.spawn import find_executable
from distutils.tests import support

SETUP_PY = """
from distutils.core import setup
import somecode

setup(name='fake')
"""

MANIFEST = """\
README
inroot.txt
setup.py
data%(sep)sdata.dt
scripts%(sep)sscript.py
some%(sep)sfile.txt
some%(sep)sother_file.txt
somecode%(sep)s__init__.py
somecode%(sep)sdoc.dat
somecode%(sep)sdoc.txt
"""

class sdistTestCase(PyPIRCCommandTestCase):

    def setUp(self):
        # PyPIRCCommandTestCase creates a temp dir already
        # and put it in self.tmp_dir
        super(sdistTestCase, self).setUp()
        # setting up an environment
        self.old_path = os.getcwd()
        os.mkdir(join(self.tmp_dir, 'somecode'))
        os.mkdir(join(self.tmp_dir, 'dist'))
        # a package, and a README
        self.write_file((self.tmp_dir, 'README'), 'xxx')
        self.write_file((self.tmp_dir, 'somecode', '__init__.py'), '#')
        self.write_file((self.tmp_dir, 'setup.py'), SETUP_PY)
        os.chdir(self.tmp_dir)

    def tearDown(self):
        # back to normal
        os.chdir(self.old_path)
        super(sdistTestCase, self).tearDown()

    def get_cmd(self, metadata=None):
        """Returns a cmd"""
        if metadata is None:
            metadata = {'name': 'fake', 'version': '1.0',
                        'url': 'xxx', 'author': 'xxx',
                        'author_email': 'xxx'}
        dist = Distribution(metadata)
        dist.script_name = 'setup.py'
        dist.packages = ['somecode']
        dist.include_package_data = True
        cmd = sdist(dist)
        cmd.dist_dir = 'dist'
        def _warn(*args):
            pass
        cmd.warn = _warn
        return dist, cmd

    def test_prune_file_list(self):
        # this test creates a package with some vcs dirs in it
        # and launch sdist to make sure they get pruned
        # on all systems

        # creating VCS directories with some files in them
        os.mkdir(join(self.tmp_dir, 'somecode', '.svn'))
        self.write_file((self.tmp_dir, 'somecode', '.svn', 'ok.py'), 'xxx')

        os.mkdir(join(self.tmp_dir, 'somecode', '.hg'))
        self.write_file((self.tmp_dir, 'somecode', '.hg',
                         'ok'), 'xxx')

        os.mkdir(join(self.tmp_dir, 'somecode', '.git'))
        self.write_file((self.tmp_dir, 'somecode', '.git',
                         'ok'), 'xxx')

        # now building a sdist
        dist, cmd = self.get_cmd()

        # zip is available universally
        # (tar might not be installed under win32)
        cmd.formats = ['zip']

        cmd.ensure_finalized()
        cmd.run()

        # now let's check what we have
        dist_folder = join(self.tmp_dir, 'dist')
        files = os.listdir(dist_folder)
        self.assertEquals(files, ['fake-1.0.zip'])

        zip_file = zipfile.ZipFile(join(dist_folder, 'fake-1.0.zip'))
        try:
            content = zip_file.namelist()
        finally:
            zip_file.close()

        # making sure everything has been pruned correctly
        self.assertEquals(len(content), 4)

    def test_make_distribution(self):

        # check if tar and gzip are installed
        if (find_executable('tar') is None or
            find_executable('gzip') is None):
            return

        # now building a sdist
        dist, cmd = self.get_cmd()

        # creating a gztar then a tar
        cmd.formats = ['gztar', 'tar']
        cmd.ensure_finalized()
        cmd.run()

        # making sure we have two files
        dist_folder = join(self.tmp_dir, 'dist')
        result = os.listdir(dist_folder)
        result.sort()
        self.assertEquals(result,
                          ['fake-1.0.tar', 'fake-1.0.tar.gz'] )

        os.remove(join(dist_folder, 'fake-1.0.tar'))
        os.remove(join(dist_folder, 'fake-1.0.tar.gz'))

        # now trying a tar then a gztar
        cmd.formats = ['tar', 'gztar']

        cmd.ensure_finalized()
        cmd.run()

        result = os.listdir(dist_folder)
        result.sort()
        self.assertEquals(result,
                ['fake-1.0.tar', 'fake-1.0.tar.gz'])

    def test_add_defaults(self):

        # http://bugs.python.org/issue2279

        # add_default should also include
        # data_files and package_data
        dist, cmd = self.get_cmd()

        # filling data_files by pointing files
        # in package_data
        dist.package_data = {'': ['*.cfg', '*.dat'],
                             'somecode': ['*.txt']}
        self.write_file((self.tmp_dir, 'somecode', 'doc.txt'), '#')
        self.write_file((self.tmp_dir, 'somecode', 'doc.dat'), '#')

        # adding some data in data_files
        data_dir = join(self.tmp_dir, 'data')
        os.mkdir(data_dir)
        self.write_file((data_dir, 'data.dt'), '#')
        some_dir = join(self.tmp_dir, 'some')
        os.mkdir(some_dir)
        self.write_file((self.tmp_dir, 'inroot.txt'), '#')
        self.write_file((some_dir, 'file.txt'), '#')
        self.write_file((some_dir, 'other_file.txt'), '#')

        dist.data_files = [('data', ['data/data.dt',
                                     'inroot.txt',
                                     'notexisting']),
                           'some/file.txt',
                           'some/other_file.txt']

        # adding a script
        script_dir = join(self.tmp_dir, 'scripts')
        os.mkdir(script_dir)
        self.write_file((script_dir, 'script.py'), '#')
        dist.scripts = [join('scripts', 'script.py')]

        cmd.formats = ['zip']
        cmd.use_defaults = True

        cmd.ensure_finalized()
        cmd.run()

        # now let's check what we have
        dist_folder = join(self.tmp_dir, 'dist')
        files = os.listdir(dist_folder)
        self.assertEquals(files, ['fake-1.0.zip'])

        zip_file = zipfile.ZipFile(join(dist_folder, 'fake-1.0.zip'))
        try:
            content = zip_file.namelist()
        finally:
            zip_file.close()

        # making sure everything was added
        self.assertEquals(len(content), 11)

        # checking the MANIFEST
        manifest = open(join(self.tmp_dir, 'MANIFEST')).read()
        self.assertEquals(manifest, MANIFEST % {'sep': os.sep})

def test_suite():
    return unittest.makeSuite(sdistTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
