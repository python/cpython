from collections import deque
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
        self.assertIn(1, b)
        self.assertNotIn(0, b)
        self.assertIn(1, c)
        self.assertNotIn(0, c)
        self.assertRaises(TypeError, lambda: 1 in a)
        self.assertRaises(TypeError, lambda: 1 not in a)

        # test char in string
        self.assertIn('c', 'abc')
        self.assertNotIn('d', 'abc')

        self.assertIn('', '')
        self.assertIn('', 'abc')

        self.assertRaises(TypeError, lambda: None in 'abc')

    def test_builtin_sequence_types(self):
        # a collection of tests on builtin sequence types
        a = range(10)
        for i in a:
            self.assertIn(i, a)
        self.assertNotIn(16, a)
        self.assertNotIn(a, a)

        a = tuple(a)
        for i in a:
            self.assertIn(i, a)
        self.assertNotIn(16, a)
        self.assertNotIn(a, a)

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

        self.assertNotIn(Deviant1(), Deviant1.aList)

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
                self.assertIn(elem, container)
            self.assertTrue(container == constructor(values))
            self.assertTrue(container == container)

    def test_block_fallback(self):
        # blocking fallback with __contains__ = None
        class ByContains(object):
            def __contains__(self, other):
                return False
        c = ByContains()
        class BlockContains(ByContains):
            """Is not a container

            This class is a perfectly good iterable (as tested by
            list(bc)), as well as inheriting from a perfectly good
            container, but __contains__ = None prevents the usual
            fallback to iteration in the container protocol. That
            is, normally, 0 in bc would fall back to the equivalent
            of any(x==0 for x in bc), but here it's blocked from
            doing so.
            """
            def __iter__(self):
                while False:
                    yield None
            __contains__ = None
        bc = BlockContains()
        self.assertFalse(0 in c)
        self.assertFalse(0 in list(bc))
        self.assertRaises(TypeError, lambda: 0 in bc)

if __name__ == '__main__':
    unittest.main()
