# As a test suite for the os module, this is woefully inadequate, but this
# does add tests for a few functions which have been determined to be more
# portable than they had been thought to be.

import os
import unittest
import warnings
from test import test_support

warnings.filterwarnings("ignore", "tempnam", RuntimeWarning, __name__)
warnings.filterwarnings("ignore", "tmpnam", RuntimeWarning, __name__)

class TemporaryFileTests(unittest.TestCase):
    def setUp(self):
        self.files = []
        os.mkdir(test_support.TESTFN)

    def tearDown(self):
        for name in self.files:
            os.unlink(name)
        os.rmdir(test_support.TESTFN)

    def check_tempfile(self, name):
        # make sure it doesn't already exist:
        self.failIf(os.path.exists(name),
                    "file already exists for temporary file")
        # make sure we can create the file
        open(name, "w")
        self.files.append(name)

    def test_tempnam(self):
        if not hasattr(os, "tempnam"):
            return
        warnings.filterwarnings("ignore", "tempnam", RuntimeWarning,
                                r"test_os$")
        self.check_tempfile(os.tempnam())

        name = os.tempnam(test_support.TESTFN)
        self.check_tempfile(name)

        name = os.tempnam(test_support.TESTFN, "pfx")
        self.assert_(os.path.basename(name)[:3] == "pfx")
        self.check_tempfile(name)

    def test_tmpfile(self):
        if not hasattr(os, "tmpfile"):
            return
        fp = os.tmpfile()
        fp.write("foobar")
        fp.seek(0,0)
        s = fp.read()
        fp.close()
        self.assert_(s == "foobar")

    def test_tmpnam(self):
        import sys
        if not hasattr(os, "tmpnam"):
            return
        warnings.filterwarnings("ignore", "tmpnam", RuntimeWarning,
                                r"test_os$")
        name = os.tmpnam()
        if sys.platform in ("win32",):
            # The Windows tmpnam() seems useless.  From the MS docs:
            #
            #     The character string that tmpnam creates consists of
            #     the path prefix, defined by the entry P_tmpdir in the
            #     file STDIO.H, followed by a sequence consisting of the
            #     digit characters '0' through '9'; the numerical value
            #     of this string is in the range 1 - 65,535.  Changing the
            #     definitions of L_tmpnam or P_tmpdir in STDIO.H does not
            #     change the operation of tmpnam.
            #
            # The really bizarre part is that, at least under MSVC6,
            # P_tmpdir is "\\".  That is, the path returned refers to
            # the root of the current drive.  That's a terrible place to
            # put temp files, and, depending on privileges, the user
            # may not even be able to open a file in the root directory.
            self.failIf(os.path.exists(name),
                        "file already exists for temporary file")
        else:
            self.check_tempfile(name)

# Test attributes on return values from os.*stat* family.
class StatAttributeTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(test_support.TESTFN)
        self.fname = os.path.join(test_support.TESTFN, "f1")
        f = open(self.fname, 'wb')
        f.write("ABC")
        f.close()

    def tearDown(self):
        os.unlink(self.fname)
        os.rmdir(test_support.TESTFN)

    def test_stat_attributes(self):
        if not hasattr(os, "stat"):
            return

        import stat
        result = os.stat(self.fname)

        # Make sure direct access works
        self.assertEquals(result[stat.ST_SIZE], 3)
        self.assertEquals(result.st_size, 3)

        import sys

        # Make sure all the attributes are there
        members = dir(result)
        for name in dir(stat):
            if name[:3] == 'ST_':
                attr = name.lower()
                self.assertEquals(getattr(result, attr),
                                  result[getattr(stat, name)])
                self.assert_(attr in members)

        try:
            result[200]
            self.fail("No exception thrown")
        except IndexError:
            pass

        # Make sure that assignment fails
        try:
            result.st_mode = 1
            self.fail("No exception thrown")
        except TypeError:
            pass

        try:
            result.st_rdev = 1
            self.fail("No exception thrown")
        except (AttributeError, TypeError):
            pass

        try:
            result.parrot = 1
            self.fail("No exception thrown")
        except AttributeError:
            pass

        # Use the stat_result constructor with a too-short tuple.
        try:
            result2 = os.stat_result((10,))
            self.fail("No exception thrown")
        except TypeError:
            pass

        # Use the constructr with a too-long tuple.
        try:
            result2 = os.stat_result((0,1,2,3,4,5,6,7,8,9,10,11,12,13,14))
        except TypeError:
            pass


    def test_statvfs_attributes(self):
        if not hasattr(os, "statvfs"):
            return

        import statvfs
        try:
            result = os.statvfs(self.fname)
        except OSError, e:
            # On AtheOS, glibc always returns ENOSYS
            import errno
            if e.errno == errno.ENOSYS:
                return

        # Make sure direct access works
        self.assertEquals(result.f_bfree, result[statvfs.F_BFREE])

        # Make sure all the attributes are there
        members = dir(result)
        for name in dir(statvfs):
            if name[:2] == 'F_':
                attr = name.lower()
                self.assertEquals(getattr(result, attr),
                                  result[getattr(statvfs, name)])
                self.assert_(attr in members)

        # Make sure that assignment really fails
        try:
            result.f_bfree = 1
            self.fail("No exception thrown")
        except TypeError:
            pass

        try:
            result.parrot = 1
            self.fail("No exception thrown")
        except AttributeError:
            pass

        # Use the constructor with a too-short tuple.
        try:
            result2 = os.statvfs_result((10,))
            self.fail("No exception thrown")
        except TypeError:
            pass

        # Use the constructr with a too-long tuple.
        try:
            result2 = os.statvfs_result((0,1,2,3,4,5,6,7,8,9,10,11,12,13,14))
        except TypeError:
            pass

from test_userdict import TestMappingProtocol

class EnvironTests(TestMappingProtocol):
    """check that os.environ object conform to mapping protocol"""
    _tested_class = None
    def _reference(self):
        return {"KEY1":"VALUE1", "KEY2":"VALUE2", "KEY3":"VALUE3"}
    def _empty_mapping(self):
        os.environ.clear()
        return os.environ
    def setUp(self):
        self.__save = dict(os.environ)
        os.environ.clear()
    def tearDown(self):
        os.environ.clear()
        os.environ.update(self.__save)

class WalkTests(unittest.TestCase):
    """Tests for os.walk()."""

    def test_traversal(self):
        import os
        from os.path import join

        # Build:
        #     TESTFN/               a file kid and two directory kids
        #         tmp1
        #         SUB1/             a file kid and a directory kid
        #             tmp2
        #             SUB11/        no kids
        #         SUB2/             just a file kid
        #             tmp3
        sub1_path = join(test_support.TESTFN, "SUB1")
        sub11_path = join(sub1_path, "SUB11")
        sub2_path = join(test_support.TESTFN, "SUB2")
        tmp1_path = join(test_support.TESTFN, "tmp1")
        tmp2_path = join(sub1_path, "tmp2")
        tmp3_path = join(sub2_path, "tmp3")

        # Create stuff.
        os.makedirs(sub11_path)
        os.makedirs(sub2_path)
        for path in tmp1_path, tmp2_path, tmp3_path:
            f = file(path, "w")
            f.write("I'm " + path + " and proud of it.  Blame test_os.\n")
            f.close()

        # Walk top-down.
        all = list(os.walk(test_support.TESTFN))
        self.assertEqual(len(all), 4)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  TESTFN, SUB1, SUB11, SUB2
        #     flipped:  TESTFN, SUB2, SUB1, SUB11
        flipped = all[0][1][0] != "SUB1"
        all[0][1].sort()
        self.assertEqual(all[0], (test_support.TESTFN, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[1 + flipped], (sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 + flipped], (sub11_path, [], []))
        self.assertEqual(all[3 - 2 * flipped], (sub2_path, [], ["tmp3"]))

        # Prune the search.
        all = []
        for root, dirs, files in os.walk(test_support.TESTFN):
            all.append((root, dirs, files))
            # Don't descend into SUB1.
            if 'SUB1' in dirs:
                # Note that this also mutates the dirs we appended to all!
                dirs.remove('SUB1')
        self.assertEqual(len(all), 2)
        self.assertEqual(all[0], (test_support.TESTFN, ["SUB2"], ["tmp1"]))
        self.assertEqual(all[1], (sub2_path, [], ["tmp3"]))

        # Walk bottom-up.
        all = list(os.walk(test_support.TESTFN, topdown=False))
        self.assertEqual(len(all), 4)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  SUB11, SUB1, SUB2, TESTFN
        #     flipped:  SUB2, SUB11, SUB1, TESTFN
        flipped = all[3][1][0] != "SUB1"
        all[3][1].sort()
        self.assertEqual(all[3], (test_support.TESTFN, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[flipped], (sub11_path, [], []))
        self.assertEqual(all[flipped + 1], (sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 - 2 * flipped], (sub2_path, [], ["tmp3"]))

        # Tear everything down.  This is a decent use for bottom-up on
        # Windows, which doesn't have a recursive delete command.  The
        # (not so) subtlety is that rmdir will fail unless the dir's
        # kids are removed first, so bottom up is essential.
        for root, dirs, files in os.walk(test_support.TESTFN, topdown=False):
            for name in files:
                os.remove(join(root, name))
            for name in dirs:
                os.rmdir(join(root, name))
        os.rmdir(test_support.TESTFN)

def test_main():
    test_support.run_unittest(
        TemporaryFileTests,
        StatAttributeTests,
        EnvironTests,
        WalkTests
    )

if __name__ == "__main__":
    test_main()
