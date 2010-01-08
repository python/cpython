from test.test_support import run_unittest, have_unicode
import unittest
import sys

class TestImplementationComparisons(unittest.TestCase):

    def test_type_comparisons(self):
        self.assertTrue(str < int or str > int)
        self.assertTrue(int <= str or int >= str)
        self.assertTrue(cmp(int, str) != 0)
        self.assertTrue(int is int)
        self.assertTrue(str == str)
        self.assertTrue(int != str)

    def test_cell_comparisons(self):
        def f(x):
            if x:
                y = 1
            def g():
                return x
            def h():
                return y
            return g, h
        g, h = f(0)
        g_cell, = g.func_closure
        h_cell, = h.func_closure
        self.assertTrue(h_cell < g_cell)
        self.assertTrue(g_cell >= h_cell)
        self.assertEqual(cmp(g_cell, h_cell), 1)
        self.assertTrue(g_cell is g_cell)
        self.assertTrue(g_cell == g_cell)
        self.assertTrue(h_cell == h_cell)
        self.assertTrue(g_cell != h_cell)

def test_main():
    run_unittest(TestImplementationComparisons)

if __name__ == '__main__':
    test_main()
