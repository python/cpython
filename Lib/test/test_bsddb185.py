
from test.test_support import verbose, run_unittest, findfile
import unittest
import bsddb185
import anydbm
import whichdb
import os
import tempfile

class Bsddb185Tests(unittest.TestCase):
    def test_open_existing_hash(self):
        "verify we can open a file known to be a hash v2 file"
        # do we need to worry about big vs little endian?
        db = bsddb185.hashopen(findfile("185test.db"))
        self.assertEqual(db["1"], "1")
        db.close()

    def test_whichdb(self):
        "verify that whichdb correctly sniffs the known hash v2 file"
        self.assertEqual(whichdb.whichdb(findfile("185test.db")), "bsddb185")

    def test_anydbm_create(self):
        "verify that anydbm.open does *not* create a bsddb185 file"
        tmpdir = tempfile.mkdtemp()
        try:
            try:
                dbfile = os.path.join(tmpdir, "foo.db")
                anydbm.open(dbfile, "c").close()
                ftype = whichdb.whichdb(findfile("foo.db"))
                self.assertNotEqual(ftype, "bsddb185")
            finally:
                os.unlink(dbfile)
        finally:
            os.rmdir(tmpdir)

def test_main():
    run_unittest(Bsddb185Tests)

if __name__ == "__main__":
    test_main()
