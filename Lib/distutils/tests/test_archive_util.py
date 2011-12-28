# -*- coding: utf-8 -*-
"""Tests for distutils.archive_util."""
__revision__ = "$Id$"

import unittest
import os
import sys
import tarfile
from os.path import splitdrive
import warnings

from distutils.archive_util import (check_archive_formats, make_tarball,
                                    make_zipfile, make_archive,
                                    ARCHIVE_FORMATS)
from distutils.spawn import find_executable, spawn
from distutils.tests import support
from test.test_support import check_warnings, run_unittest

try:
    import grp
    import pwd
    UID_GID_SUPPORT = True
except ImportError:
    UID_GID_SUPPORT = False

try:
    import zipfile
    ZIP_SUPPORT = True
except ImportError:
    ZIP_SUPPORT = find_executable('zip')

# some tests will fail if zlib is not available
try:
    import zlib
except ImportError:
    zlib = None

def can_fs_encode(filename):
    """
    Return True if the filename can be saved in the file system.
    """
    if os.path.supports_unicode_filenames:
        return True
    try:
        filename.encode(sys.getfilesystemencoding())
    except UnicodeEncodeError:
        return False
    return True


class ArchiveUtilTestCase(support.TempdirManager,
                          support.LoggingSilencer,
                          unittest.TestCase):

    @unittest.skipUnless(zlib, "requires zlib")
    def test_make_tarball(self):
        self._make_tarball('archive')

    def _make_tarball(self, target_name):
        # creating something to tar
        tmpdir = self.mkdtemp()
        self.write_file([tmpdir, 'file1'], 'xxx')
        self.write_file([tmpdir, 'file2'], 'xxx')
        os.mkdir(os.path.join(tmpdir, 'sub'))
        self.write_file([tmpdir, 'sub', 'file3'], 'xxx')

        tmpdir2 = self.mkdtemp()
        unittest.skipUnless(splitdrive(tmpdir)[0] == splitdrive(tmpdir2)[0],
                            "source and target should be on same drive")

        base_name = os.path.join(tmpdir2, target_name)

        # working with relative paths to avoid tar warnings
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            make_tarball(splitdrive(base_name)[1], '.')
        finally:
            os.chdir(old_dir)

        # check if the compressed tarball was created
        tarball = base_name + '.tar.gz'
        self.assertTrue(os.path.exists(tarball))

        # trying an uncompressed one
        base_name = os.path.join(tmpdir2, target_name)
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            make_tarball(splitdrive(base_name)[1], '.', compress=None)
        finally:
            os.chdir(old_dir)
        tarball = base_name + '.tar'
        self.assertTrue(os.path.exists(tarball))

    def _tarinfo(self, path):
        tar = tarfile.open(path)
        try:
            names = tar.getnames()
            names.sort()
            return tuple(names)
        finally:
            tar.close()

    def _create_files(self):
        # creating something to tar
        tmpdir = self.mkdtemp()
        dist = os.path.join(tmpdir, 'dist')
        os.mkdir(dist)
        self.write_file([dist, 'file1'], 'xxx')
        self.write_file([dist, 'file2'], 'xxx')
        os.mkdir(os.path.join(dist, 'sub'))
        self.write_file([dist, 'sub', 'file3'], 'xxx')
        os.mkdir(os.path.join(dist, 'sub2'))
        tmpdir2 = self.mkdtemp()
        base_name = os.path.join(tmpdir2, 'archive')
        return tmpdir, tmpdir2, base_name

    @unittest.skipUnless(zlib, "Requires zlib")
    @unittest.skipUnless(find_executable('tar') and find_executable('gzip'),
                         'Need the tar command to run')
    def test_tarfile_vs_tar(self):
        tmpdir, tmpdir2, base_name =  self._create_files()
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            make_tarball(base_name, 'dist')
        finally:
            os.chdir(old_dir)

        # check if the compressed tarball was created
        tarball = base_name + '.tar.gz'
        self.assertTrue(os.path.exists(tarball))

        # now create another tarball using `tar`
        tarball2 = os.path.join(tmpdir, 'archive2.tar.gz')
        tar_cmd = ['tar', '-cf', 'archive2.tar', 'dist']
        gzip_cmd = ['gzip', '-f9', 'archive2.tar']
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            spawn(tar_cmd)
            spawn(gzip_cmd)
        finally:
            os.chdir(old_dir)

        self.assertTrue(os.path.exists(tarball2))
        # let's compare both tarballs
        self.assertEqual(self._tarinfo(tarball), self._tarinfo(tarball2))

        # trying an uncompressed one
        base_name = os.path.join(tmpdir2, 'archive')
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            make_tarball(base_name, 'dist', compress=None)
        finally:
            os.chdir(old_dir)
        tarball = base_name + '.tar'
        self.assertTrue(os.path.exists(tarball))

        # now for a dry_run
        base_name = os.path.join(tmpdir2, 'archive')
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            make_tarball(base_name, 'dist', compress=None, dry_run=True)
        finally:
            os.chdir(old_dir)
        tarball = base_name + '.tar'
        self.assertTrue(os.path.exists(tarball))

    @unittest.skipUnless(find_executable('compress'),
                         'The compress program is required')
    def test_compress_deprecated(self):
        tmpdir, tmpdir2, base_name =  self._create_files()

        # using compress and testing the PendingDeprecationWarning
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            with check_warnings() as w:
                warnings.simplefilter("always")
                make_tarball(base_name, 'dist', compress='compress')
        finally:
            os.chdir(old_dir)
        tarball = base_name + '.tar.Z'
        self.assertTrue(os.path.exists(tarball))
        self.assertEqual(len(w.warnings), 1)

        # same test with dry_run
        os.remove(tarball)
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        try:
            with check_warnings() as w:
                warnings.simplefilter("always")
                make_tarball(base_name, 'dist', compress='compress',
                             dry_run=True)
        finally:
            os.chdir(old_dir)
        self.assertTrue(not os.path.exists(tarball))
        self.assertEqual(len(w.warnings), 1)

    @unittest.skipUnless(zlib, "Requires zlib")
    @unittest.skipUnless(ZIP_SUPPORT, 'Need zip support to run')
    def test_make_zipfile(self):
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
        self.assertEqual(check_archive_formats(['gztar', 'xxx', 'zip']),
                         'xxx')
        self.assertEqual(check_archive_formats(['gztar', 'zip']), None)

    def test_make_archive(self):
        tmpdir = self.mkdtemp()
        base_name = os.path.join(tmpdir, 'archive')
        self.assertRaises(ValueError, make_archive, base_name, 'xxx')

    @unittest.skipUnless(zlib, "Requires zlib")
    def test_make_archive_owner_group(self):
        # testing make_archive with owner and group, with various combinations
        # this works even if there's not gid/uid support
        if UID_GID_SUPPORT:
            group = grp.getgrgid(0)[0]
            owner = pwd.getpwuid(0)[0]
        else:
            group = owner = 'root'

        base_dir, root_dir, base_name =  self._create_files()
        base_name = os.path.join(self.mkdtemp() , 'archive')
        res = make_archive(base_name, 'zip', root_dir, base_dir, owner=owner,
                           group=group)
        self.assertTrue(os.path.exists(res))

        res = make_archive(base_name, 'zip', root_dir, base_dir)
        self.assertTrue(os.path.exists(res))

        res = make_archive(base_name, 'tar', root_dir, base_dir,
                           owner=owner, group=group)
        self.assertTrue(os.path.exists(res))

        res = make_archive(base_name, 'tar', root_dir, base_dir,
                           owner='kjhkjhkjg', group='oihohoh')
        self.assertTrue(os.path.exists(res))

    @unittest.skipUnless(zlib, "Requires zlib")
    @unittest.skipUnless(UID_GID_SUPPORT, "Requires grp and pwd support")
    def test_tarfile_root_owner(self):
        tmpdir, tmpdir2, base_name =  self._create_files()
        old_dir = os.getcwd()
        os.chdir(tmpdir)
        group = grp.getgrgid(0)[0]
        owner = pwd.getpwuid(0)[0]
        try:
            archive_name = make_tarball(base_name, 'dist', compress=None,
                                        owner=owner, group=group)
        finally:
            os.chdir(old_dir)

        # check if the compressed tarball was created
        self.assertTrue(os.path.exists(archive_name))

        # now checks the rights
        archive = tarfile.open(archive_name)
        try:
            for member in archive.getmembers():
                self.assertEqual(member.uid, 0)
                self.assertEqual(member.gid, 0)
        finally:
            archive.close()

    def test_make_archive_cwd(self):
        current_dir = os.getcwd()
        def _breaks(*args, **kw):
            raise RuntimeError()
        ARCHIVE_FORMATS['xxx'] = (_breaks, [], 'xxx file')
        try:
            try:
                make_archive('xxx', 'xxx', root_dir=self.mkdtemp())
            except:
                pass
            self.assertEqual(os.getcwd(), current_dir)
        finally:
            del ARCHIVE_FORMATS['xxx']

    @unittest.skipUnless(zlib, "requires zlib")
    def test_make_tarball_unicode(self):
        """
        Mirror test_make_tarball, except filename is unicode.
        """
        self._make_tarball(u'archive')

    @unittest.skipUnless(zlib, "requires zlib")
    @unittest.skipUnless(can_fs_encode(u'årchiv'),
        'File system cannot handle this filename')
    def test_make_tarball_unicode_latin1(self):
        """
        Mirror test_make_tarball, except filename is unicode and contains
        latin characters.
        """
        self._make_tarball(u'årchiv') # note this isn't a real word

    @unittest.skipUnless(zlib, "requires zlib")
    @unittest.skipUnless(can_fs_encode(u'のアーカイブ'),
        'File system cannot handle this filename')
    def test_make_tarball_unicode_extended(self):
        """
        Mirror test_make_tarball, except filename is unicode and contains
        characters outside the latin charset.
        """
        self._make_tarball(u'のアーカイブ') # japanese for archive

def test_suite():
    return unittest.makeSuite(ArchiveUtilTestCase)

if __name__ == "__main__":
    run_unittest(test_suite())
