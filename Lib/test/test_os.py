# As a test suite for the os module, this is woefully inadequate, but this
# does add tests for a few functions which have been determined to be more
# more portable than they had been thought to be.

import os
import unittest
import warnings

warnings.filterwarnings("ignore", "tempnam", RuntimeWarning, __name__)
warnings.filterwarnings("ignore", "tmpnam", RuntimeWarning, __name__)

from test_support import TESTFN, run_unittest

class TemporaryFileTests(unittest.TestCase):
    def setUp(self):
        self.files = []
        os.mkdir(TESTFN)

    def tearDown(self):
        for name in self.files:
            os.unlink(name)
        os.rmdir(TESTFN)

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
                                "test_os")
        self.check_tempfile(os.tempnam())

        name = os.tempnam(TESTFN)
        self.check_tempfile(name)

        name = os.tempnam(TESTFN, "pfx")
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
        if not hasattr(os, "tmpnam"):
            return
        warnings.filterwarnings("ignore", "tmpnam", RuntimeWarning,
                                "test_os")
        self.check_tempfile(os.tmpnam())

# Test attributes on return values from os.*stat* family.
class StatAttributeTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(TESTFN)
        self.fname = os.path.join(TESTFN, "f1")
        f = open(self.fname, 'wb')
        f.write("ABC")
        f.close()

    def tearDown(self):
        os.unlink(self.fname)
        os.rmdir(TESTFN)

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
        result = os.statvfs(self.fname)

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

def test_main():
    run_unittest(TemporaryFileTests)
    run_unittest(StatAttributeTests)

if __name__ == "__main__":
    test_main()
