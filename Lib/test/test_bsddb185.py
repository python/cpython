"""Tests for the bsddb185 module.

The file 185test.db found in Lib/test/ is for testing purposes with this
testing suite.

"""
from test.test_support import verbose, run_unittest, findfile
import unittest
import bsddb185
import anydbm
import whichdb
import os
import tempfile
import shutil

class Bsddb185Tests(unittest.TestCase):

    def test_open_existing_hash(self):
        # Verify we can open a file known to be a hash v2 file
        db = bsddb185.hashopen(findfile("185test.db"))
        self.assertEqual(db["1"], "1")
        db.close()

    def test_whichdb(self):
        # Verify that whichdb correctly sniffs the known hash v2 file
        self.assertEqual(whichdb.whichdb(findfile("185test.db")), "bsddb185")

    def test_anydbm_create(self):
        # Verify that anydbm.open does *not* create a bsddb185 file
        tmpdir = tempfile.mkdtemp()
        try:
            dbfile = os.path.join(tmpdir, "foo.db")
            anydbm.open(dbfile, "c").close()
            ftype = whichdb.whichdb(dbfile)
            self.assertNotEqual(ftype, "bsddb185")
        finally:
            shutil.rmtree(tmpdir)

def test_main():
    run_unittest(Bsddb185Tests)

if __name__ == "__main__":
    test_main()
