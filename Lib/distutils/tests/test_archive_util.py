"""Tests for distutils.archive_util."""
__revision__ = "$Id:$"

import unittest
import os

from distutils.archive_util import (check_archive_formats, make_tarball,
                                    make_zipfile, make_archive)
from distutils.spawn import find_executable
from distutils.tests import support

try:
    import zipfile
    ZIP_SUPPORT = True
except ImportError:
    ZIP_SUPPORT = find_executable('zip')

class ArchiveUtilTestCase(support.TempdirManager,
                          unittest.TestCase):

    @unittest.skipUnless(find_executable('tar'), 'Need the tar command to run')
    def test_make_tarball(self):
        # creating something to tar
        tmpdir = self.mkdtemp()
        self.write_file([tmpdir, 'file1'], 'xxx')
        self.write_file([tmpdir, 'file2'], 'xxx')

        tmpdir2 = self.mkdtemp()
        base_name = os.path.join(tmpdir2, 'archive')
        make_tarball(base_name, tmpdir)

        # check if the compressed tarball was created
        tarball = base_name + '.tar.gz'
        self.assert_(os.path.exists(tarball))

        # trying an uncompressed one
        base_name = os.path.join(tmpdir2, 'archive')
        make_tarball(base_name, tmpdir, compress=None)
        tarball = base_name + '.tar'
        self.assert_(os.path.exists(tarball))

    @unittest.skipUnless(ZIP_SUPPORT, 'Need zip support to run')
    def test_make_tarball(self):
        # creating something to tar
        tmpdir = self.mkdtemp()
        self.write_file([tmpdir, 'file1'], 'xxx')
        self.write_file([tmpdir, 'file2'], 'xxx')

        tmpdir2 = self.mkdtemp()
        base_name = os.path.join(tmpdir2, 'archive')
        make_zipfile(base_name, tmpdir)

        # check if the compressed tarball was created
        tarball = base_name + '.zip'

    def test_check_archive_formats(self):
        self.assertEquals(check_archive_formats(['gztar', 'xxx', 'zip']),
                          'xxx')
        self.assertEquals(check_archive_formats(['gztar', 'zip']), None)

    def test_make_archive(self):
        tmpdir = self.mkdtemp()
        base_name = os.path.join(tmpdir, 'archive')
        self.assertRaises(ValueError, make_archive, base_name, 'xxx')

def test_suite():
    return unittest.makeSuite(ArchiveUtilTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
