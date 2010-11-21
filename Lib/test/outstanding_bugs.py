#
# This file is for everybody to add tests for bugs that aren't
# fixed yet. Please add a test case and appropriate bug description.
#
# When you fix one of the bugs, please move the test to the correct
# test_ module.
#

import unittest
from test import support

#
# One test case for outstanding bugs at the moment:
#

# test_io
import io
class TextIOWrapperTest(unittest.TestCase):

    def setUp(self):
        self.testdata = b"AAA\r\nBBB\rCCC\r\nDDD\nEEE\r\n"
        self.normalized = b"AAA\nBBB\nCCC\nDDD\nEEE\n".decode("ASCII")

    def tearDown(self):
        support.unlink(support.TESTFN)


    def test_issue1395_1(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")

        # read one char at a time
        reads = ""
        while True:
            c = txt.read(1)
            if not c:
                break
            reads += c
        self.assertEqual(reads, self.normalized)

    def test_issue1395_2(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")
        txt._CHUNK_SIZE = 4

        reads = ""
        while True:
            c = txt.read(4)
            if not c:
                break
            reads += c
        self.assertEqual(reads, self.normalized)

    def test_issue1395_3(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read(4)
        reads += txt.readline()
        reads += txt.readline()
        reads += txt.readline()
        self.assertEqual(reads, self.normalized)

    def test_issue1395_4(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read()
        self.assertEqual(reads, self.normalized)

    def test_issue1395_5(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        pos = txt.tell()
        txt.seek(0)
        txt.seek(pos)
        self.assertEqual(txt.read(4), "BBB\n")



def test_main():
    support.run_unittest(
        TextIOWrapperTest)

if __name__ == "__main__":
    test_main()
