from test.test_support import have_unicode, run_unittest
import unittest


class base_set:
    def __init__(self, el):
        self.el = el

class set(base_set):
    def __contains__(self, el):
        return self.el == el

class seq(base_set):
    def __getitem__(self, n):
        return [self.el][n]


class TestContains(unittest.TestCase):
    def test_common_tests(self):
        a = base_set(1)
        b = set(1)
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

    if have_unicode:
        def test_char_in_unicode(self):
            self.assertIn('c', unicode('abc'))
            self.assertNotIn('d', unicode('abc'))

            self.assertIn('', unicode(''))
            self.assertIn(unicode(''), '')
            self.assertIn(unicode(''), unicode(''))
            self.assertIn('', unicode('abc'))
            self.assertIn(unicode(''), 'abc')
            self.assertIn(unicode(''), unicode('abc'))

            self.assertRaises(TypeError, lambda: None in unicode('abc'))

            # test Unicode char in Unicode
            self.assertIn(unicode('c'), unicode('abc'))
            self.assertNotIn(unicode('d'), unicode('abc'))

            # test Unicode char in string
            self.assertIn(unicode('c'), 'abc')
            self.assertNotIn(unicode('d'), 'abc')

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
            aList = range(15)
            def __cmp__(self, other):
                if other == 12:
                    self.aList.remove(12)
                    self.aList.remove(13)
                    self.aList.remove(14)
                return 1

        self.assertNotIn(Deviant1(), Deviant1.aList)

        class Deviant2:
            """Behaves strangely when compared

            This class raises an exception during comparison.  That in
            turn causes the comparison to fail with a TypeError.
            """
            def __cmp__(self, other):
                if other == 4:
                    raise RuntimeError, "gotcha"

        try:
            self.assertNotIn(Deviant2(), a)
        except TypeError:
            pass


def test_main():
    run_unittest(TestContains)

if __name__ == '__main__':
    test_main()
