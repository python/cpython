#
# This file is for everybody to add tests for bugs that aren't
# fixed yet. Please add a test case and appropriate bug description.
#
# When you fix one of the bugs, please move the test to the correct
# test_ module.
#

import unittest
from test import test_support

#
# One test case for outstanding bugs at the moment:
#

class TestDifflibLongestMatch(unittest.TestCase):
    # From Patch #1678339:
    # The find_longest_match method in the difflib's SequenceMatcher has a bug.

    # The bug is in turn caused by a problem with creating a b2j mapping which
    # should contain a list of indices for each of the list elements in b.
    # However, when the b2j mapping is being created (this is being done in
    # __chain_b method in the SequenceMatcher) the mapping becomes broken. The
    # cause of this is that for the frequently used elements the list of indices
    # is removed and the element is being enlisted in the populardict mapping.

    # The test case tries to match two strings like:
    # abbbbbb.... and ...bbbbbbc

    # The number of b is equal and the find_longest_match should have returned
    # the proper amount. However, in case the number of "b"s is large enough, the
    # method reports that the length of the longest common substring is 0. It
    # simply can't find it.

    # A bug was raised some time ago on this matter. It's ID is 1528074.

    def test_find_longest_match(self):
        import difflib
        for i in (190, 200, 210):
            text1 = "a" + "b"*i
            text2 = "b"*i + "c"
            m = difflib.SequenceMatcher(None, text1, text2)
            (aptr, bptr, l) = m.find_longest_match(0, len(text1), 0, len(text2))
            self.assertEquals(i, l)
            self.assertEquals(aptr, 1)
            self.assertEquals(bptr, 0)

# test_io
import io
class TextIOWrapperTest(unittest.TestCase):

    def setUp(self):
        self.testdata = b"AAA\r\nBBB\rCCC\r\nDDD\nEEE\r\n"
        self.normalized = b"AAA\nBBB\nCCC\nDDD\nEEE\n".decode("ASCII")

    def tearDown(self):
        test_support.unlink(test_support.TESTFN)


    def test_issue1395_1(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")

        # read one char at a time
        reads = ""
        while True:
            c = txt.read(1)
            if not c:
                break
            reads += c
        self.assertEquals(reads, self.normalized)

    def test_issue1395_2(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")
        txt._CHUNK_SIZE = 4

        reads = ""
        while True:
            c = txt.read(4)
            if not c:
                break
            reads += c
        self.assertEquals(reads, self.normalized)

    def test_issue1395_3(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read(4)
        reads += txt.readline()
        reads += txt.readline()
        reads += txt.readline()
        self.assertEquals(reads, self.normalized)

    def test_issue1395_4(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        reads += txt.read()
        self.assertEquals(reads, self.normalized)

    def test_issue1395_5(self):
        txt = io.TextIOWrapper(io.BytesIO(self.testdata), encoding="ASCII")
        txt._CHUNK_SIZE = 4

        reads = txt.read(4)
        pos = txt.tell()
        txt.seek(0)
        txt.seek(pos)
        self.assertEquals(txt.read(4), "BBB\n")



def test_main():
    test_support.run_unittest(
        TestDifflibLongestMatch,
        TextIOWrapperTest)

if __name__ == "__main__":
    test_main()
