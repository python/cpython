"""
  Test cases for the dircache module
  Nick Mathewson
"""

import unittest
from test.test_support import run_unittest, import_module
dircache = import_module('dircache', deprecated=True)
import os, time, sys, tempfile


class DircacheTests(unittest.TestCase):
    def setUp(self):
        self.tempdir = tempfile.mkdtemp()

    def tearDown(self):
        for fname in os.listdir(self.tempdir):
            self.delTemp(fname)
        os.rmdir(self.tempdir)

    def writeTemp(self, fname):
        f = open(os.path.join(self.tempdir, fname), 'w')
        f.close()

    def mkdirTemp(self, fname):
        os.mkdir(os.path.join(self.tempdir, fname))

    def delTemp(self, fname):
        fname = os.path.join(self.tempdir, fname)
        if os.path.isdir(fname):
            os.rmdir(fname)
        else:
            os.unlink(fname)

    def test_listdir(self):
        ## SUCCESSFUL CASES
        entries = dircache.listdir(self.tempdir)
        self.assertEqual(entries, [])

        # Check that cache is actually caching, not just passing through.
        self.assertTrue(dircache.listdir(self.tempdir) is entries)

        # Directories aren't "files" on Windows, and directory mtime has
        # nothing to do with when files under a directory get created.
        # That is, this test can't possibly work under Windows -- dircache
        # is only good for capturing a one-shot snapshot there.

        if sys.platform[:3] not in ('win', 'os2'):
            # Sadly, dircache has the same granularity as stat.mtime, and so
            # can't notice any changes that occurred within 1 sec of the last
            # time it examined a directory.
            time.sleep(1)
            self.writeTemp("test1")
            entries = dircache.listdir(self.tempdir)
            self.assertEqual(entries, ['test1'])
            self.assertTrue(dircache.listdir(self.tempdir) is entries)

        ## UNSUCCESSFUL CASES
        self.assertRaises(OSError, dircache.listdir, self.tempdir+"_nonexistent")

    def test_annotate(self):
        self.writeTemp("test2")
        self.mkdirTemp("A")
        lst = ['A', 'test2', 'test_nonexistent']
        dircache.annotate(self.tempdir, lst)
        self.assertEqual(lst, ['A/', 'test2', 'test_nonexistent'])


def test_main():
    try:
        run_unittest(DircacheTests)
    finally:
        dircache.reset()


if __name__ == "__main__":
    test_main()
