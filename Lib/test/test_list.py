import unittest
from test import test_support, list_tests

class ListTest(list_tests.CommonTest):
    type2test = list

    def test_truth(self):
        super(ListTest, self).test_truth()
        self.assert_(not [])
        self.assert_([42])

    def test_identity(self):
        self.assert_([] is not [])

    def test_len(self):
        super(ListTest, self).test_len()
        self.assertEqual(len([]), 0)
        self.assertEqual(len([0]), 1)
        self.assertEqual(len([0, 1, 2]), 3)

def test_main():
    test_support.run_unittest(ListTest)

if __name__=="__main__":
    test_main()
