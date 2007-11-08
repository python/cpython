"""Unit tests for the memoryview

XXX We need more tests! Some tests are in test_bytes
"""

import unittest
import test.test_support

class MemoryviewTest(unittest.TestCase):

    def test_constructor(self):
        ob = b'test'
        self.assert_(memoryview(ob))
        self.assert_(memoryview(object=ob))
        self.assertRaises(TypeError, memoryview)
        self.assertRaises(TypeError, memoryview, ob, ob)
        self.assertRaises(TypeError, memoryview, argument=ob)
        self.assertRaises(TypeError, memoryview, ob, argument=True)

def test_main():
    test.test_support.run_unittest(MemoryviewTest)


if __name__ == "__main__":
    test_main()
