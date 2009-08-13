from collections import deque
from test.support import run_unittest
import unittest


class base_set:
    def __init__(self, el):
        self.el = el

class myset(base_set):
    def __contains__(self, el):
        return self.el == el

class seq(base_set):
    def __getitem__(self, n):
        return [self.el][n]

class TestContains(unittest.TestCase):
    def test_common_tests(self):
        a = base_set(1)
        b = myset(1)
        c = seq(1)
        self.assertTrue(1 in b)
        self.assertTrue(0 not in b)
        self.assertTrue(1 in c)
        self.assertTrue(0 not in c)
        self.assertRaises(TypeError, lambda: 1 in a)
        self.assertRaises(TypeError, lambda: 1 not in a)

        # test char in string
        self.assertTrue('c' in 'abc')
        self.assertTrue('d' not in 'abc')

        self.assertTrue('' in '')
        self.assertTrue('' in 'abc')

        self.assertRaises(TypeError, lambda: None in 'abc')

    def test_builtin_sequence_types(self):
        # a collection of tests on builtin sequence types
        a = range(10)
        for i in a:
            self.assertTrue(i in a)
        self.assertTrue(16 not in a)
        self.assertTrue(a not in a)

        a = tuple(a)
        for i in a:
            self.assertTrue(i in a)
        self.assertTrue(16 not in a)
        self.assertTrue(a not in a)

        class Deviant1:
            """Behaves strangely when compared

            This class is designed to make sure that the contains code
            works when the list is modified during the check.
            """
            aList = list(range(15))
            def __eq__(self, other):
                if other == 12:
                    self.aList.remove(12)
                    self.aList.remove(13)
                    self.aList.remove(14)
                return 0

        self.assertTrue(Deviant1() not in Deviant1.aList)

    def test_nonreflexive(self):
        # containment and equality tests involving elements that are
        # not necessarily equal to themselves

        class MyNonReflexive(object):
            def __eq__(self, other):
                return False
            def __hash__(self):
                return 28

        values = float('nan'), 1, None, 'abc', MyNonReflexive()
        constructors = list, tuple, dict.fromkeys, set, frozenset, deque
        for constructor in constructors:
            container = constructor(values)
            for elem in container:
                self.assertTrue(elem in container)
            self.assertTrue(container == constructor(values))
            self.assertTrue(container == container)


def test_main():
    run_unittest(TestContains)

if __name__ == '__main__':
    test_main()
