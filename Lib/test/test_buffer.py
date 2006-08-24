"""Unit tests for buffer objects.

For now, we just test (the brand new) rich comparison.

"""

import unittest
from test import test_support

class BufferTests(unittest.TestCase):

    def test_comparison(self):
        a = buffer("a.b.c")
        b = buffer("a.b" + ".c")
        self.assert_(a == b)
        self.assert_(a <= b)
        self.assert_(a >= b)
        self.assert_(a == "a.b.c")
        self.assert_(a <= "a.b.c")
        self.assert_(a >= "a.b.c")
        b = buffer("a.b.c.d")
        self.assert_(a != b)
        self.assert_(a <= b)
        self.assert_(a < b)
        self.assert_(a != "a.b.c.d")
        self.assert_(a < "a.b.c.d")
        self.assert_(a <= "a.b.c.d")
        b = buffer("a.b")
        self.assert_(a != b)
        self.assert_(a >= b)
        self.assert_(a > b)
        self.assert_(a != "a.b")
        self.assert_(a > "a.b")
        self.assert_(a >= "a.b")
        b = object()
        self.assert_(a != b)
        self.failIf(a == b)
        self.assertRaises(TypeError, lambda: a < b)

def test_main():
    test_support.run_unittest(BufferTests)

if __name__ == "__main__":
    test_main()
