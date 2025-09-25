"""
Test symbolic and hard links: os.link(), os.symlink(), etc.
"""

import os
import posix
import shutil
import sys
import unittest
from test.support import os_helper
from test.support.os_helper import FakePath
from .utils import create_file


@unittest.skipUnless(hasattr(os, 'link'), 'requires os.link')
class LinkTests(unittest.TestCase):
    def setUp(self):
        self.file1 = os_helper.TESTFN
        self.file2 = os.path.join(os_helper.TESTFN + "2")

    def tearDown(self):
        for file in (self.file1, self.file2):
            if os.path.exists(file):
                os.unlink(file)

    def _test_link(self, file1, file2):
        create_file(file1)

        try:
            os.link(file1, file2)
        except PermissionError as e:
            self.skipTest('os.link(): %s' % e)
        with open(file1, "rb") as f1, open(file2, "rb") as f2:
            self.assertTrue(os.path.sameopenfile(f1.fileno(), f2.fileno()))

    def test_link(self):
        self._test_link(self.file1, self.file2)

    def test_link_bytes(self):
        self._test_link(bytes(self.file1, sys.getfilesystemencoding()),
                        bytes(self.file2, sys.getfilesystemencoding()))

    def test_unicode_name(self):
        try:
            os.fsencode("\xf1")
        except UnicodeError:
            raise unittest.SkipTest("Unable to encode for this platform.")

        self.file1 += "\xf1"
        self.file2 = self.file1 + "2"
        self._test_link(self.file1, self.file2)


@os_helper.skip_unless_symlink
class NonLocalSymlinkTests(unittest.TestCase):

    def setUp(self):
        r"""
        Create this structure:

        base
         \___ some_dir
        """
        os.makedirs('base/some_dir')

    def tearDown(self):
        shutil.rmtree('base')

    def test_directory_link_nonlocal(self):
        """
        The symlink target should resolve relative to the link, not relative
        to the current directory.

        Then, link base/some_link -> base/some_dir and ensure that some_link
        is resolved as a directory.

        In issue13772, it was discovered that directory detection failed if
        the symlink target was not specified relative to the current
        directory, which was a defect in the implementation.
        """
        src = os.path.join('base', 'some_link')
        os.symlink('some_dir', src)
        assert os.path.isdir(src)


@unittest.skipUnless(hasattr(os, 'readlink'), 'needs os.readlink()')
class ReadlinkTests(unittest.TestCase):
    filelink = 'readlinktest'
    filelink_target = os.path.abspath(__file__)
    filelinkb = os.fsencode(filelink)
    filelinkb_target = os.fsencode(filelink_target)

    def assertPathEqual(self, left, right):
        left = os.path.normcase(left)
        right = os.path.normcase(right)
        if sys.platform == 'win32':
            # Bad practice to blindly strip the prefix as it may be required to
            # correctly refer to the file, but we're only comparing paths here.
            has_prefix = lambda p: p.startswith(
                b'\\\\?\\' if isinstance(p, bytes) else '\\\\?\\')
            if has_prefix(left):
                left = left[4:]
            if has_prefix(right):
                right = right[4:]
        self.assertEqual(left, right)

    def setUp(self):
        self.assertTrue(os.path.exists(self.filelink_target))
        self.assertTrue(os.path.exists(self.filelinkb_target))
        self.assertFalse(os.path.exists(self.filelink))
        self.assertFalse(os.path.exists(self.filelinkb))

    def test_not_symlink(self):
        filelink_target = FakePath(self.filelink_target)
        self.assertRaises(OSError, os.readlink, self.filelink_target)
        self.assertRaises(OSError, os.readlink, filelink_target)

    def test_missing_link(self):
        self.assertRaises(FileNotFoundError, os.readlink, 'missing-link')
        self.assertRaises(FileNotFoundError, os.readlink,
                          FakePath('missing-link'))

    @os_helper.skip_unless_symlink
    def test_pathlike(self):
        os.symlink(self.filelink_target, self.filelink)
        self.addCleanup(os_helper.unlink, self.filelink)
        filelink = FakePath(self.filelink)
        self.assertPathEqual(os.readlink(filelink), self.filelink_target)

    @os_helper.skip_unless_symlink
    def test_pathlike_bytes(self):
        os.symlink(self.filelinkb_target, self.filelinkb)
        self.addCleanup(os_helper.unlink, self.filelinkb)
        path = os.readlink(FakePath(self.filelinkb))
        self.assertPathEqual(path, self.filelinkb_target)
        self.assertIsInstance(path, bytes)

    @os_helper.skip_unless_symlink
    def test_bytes(self):
        os.symlink(self.filelinkb_target, self.filelinkb)
        self.addCleanup(os_helper.unlink, self.filelinkb)
        path = os.readlink(self.filelinkb)
        self.assertPathEqual(path, self.filelinkb_target)
        self.assertIsInstance(path, bytes)


class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b'')

    @os_helper.skip_unless_hardlink
    @os_helper.skip_unless_symlink
    def test_link_follow_symlinks(self):
        default_follow = sys.platform.startswith(
            ('darwin', 'freebsd', 'netbsd', 'openbsd', 'dragonfly', 'sunos5'))
        default_no_follow = sys.platform.startswith(('win32', 'linux'))
        orig = os_helper.TESTFN
        symlink = orig + 'symlink'
        posix.symlink(orig, symlink)
        self.addCleanup(os_helper.unlink, symlink)

        with self.subTest('no follow_symlinks'):
            # no follow_symlinks -> platform depending
            link = orig + 'link'
            posix.link(symlink, link)
            self.addCleanup(os_helper.unlink, link)
            if os.link in os.supports_follow_symlinks or default_follow:
                self.assertEqual(posix.lstat(link), posix.lstat(orig))
            elif default_no_follow:
                self.assertEqual(posix.lstat(link), posix.lstat(symlink))

        with self.subTest('follow_symlinks=False'):
            # follow_symlinks=False -> duplicate the symlink itself
            link = orig + 'link_nofollow'
            try:
                posix.link(symlink, link, follow_symlinks=False)
            except NotImplementedError:
                if os.link in os.supports_follow_symlinks or default_no_follow:
                    raise
            else:
                self.addCleanup(os_helper.unlink, link)
                self.assertEqual(posix.lstat(link), posix.lstat(symlink))

        with self.subTest('follow_symlinks=True'):
            # follow_symlinks=True -> duplicate the target file
            link = orig + 'link_following'
            try:
                posix.link(symlink, link, follow_symlinks=True)
            except NotImplementedError:
                if os.link in os.supports_follow_symlinks or default_follow:
                    raise
            else:
                self.addCleanup(os_helper.unlink, link)
                self.assertEqual(posix.lstat(link), posix.lstat(orig))


if __name__ == "__main__":
    unittest.main()
