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


def test_main():
    run_unittest(TemporaryFileTests)


if __name__ == "__main__":
    test_main()
