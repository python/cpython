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
        self.assert_(1 in b)
        self.assert_(0 not in b)
        self.assert_(1 in c)
        self.assert_(0 not in c)
        self.assertRaises(TypeError, lambda: 1 in a)
        self.assertRaises(TypeError, lambda: 1 not in a)

        # test char in string
        self.assert_('c' in 'abc')
        self.assert_('d' not in 'abc')

        self.assert_('' in '')
        self.assert_('' in 'abc')

        self.assertRaises(TypeError, lambda: None in 'abc')

    if have_unicode:
        def test_char_in_unicode(self):
            self.assert_('c' in unicode('abc'))
            self.assert_('d' not in unicode('abc'))

            self.assert_('' in unicode(''))
            self.assert_(unicode('') in '')
            self.assert_(unicode('') in unicode(''))
            self.assert_('' in unicode('abc'))
            self.assert_(unicode('') in 'abc')
            self.assert_(unicode('') in unicode('abc'))

            self.assertRaises(TypeError, lambda: None in unicode('abc'))

            # test Unicode char in Unicode
            self.assert_(unicode('c') in unicode('abc'))
            self.assert_(unicode('d') not in unicode('abc'))

            # test Unicode char in string
            self.assert_(unicode('c') in 'abc')
            self.assert_(unicode('d') not in 'abc')

    def test_builtin_sequence_types(self):
        # a collection of tests on builtin sequence types
        a = range(10)
        for i in a:
            self.assert_(i in a)
        self.assert_(16 not in a)
        self.assert_(a not in a)

        a = tuple(a)
        for i in a:
            self.assert_(i in a)
        self.assert_(16 not in a)
        self.assert_(a not in a)

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

        self.assert_(Deviant1() not in Deviant1.aList)

        class Deviant2:
            """Behaves strangely when compared

            This class raises an exception during comparison.  That in
            turn causes the comparison to fail with a TypeError.
            """
            def __cmp__(self, other):
                if other == 4:
                    raise RuntimeError, "gotcha"

        try:
            self.assert_(Deviant2() not in a)
        except TypeError:
            pass


def test_main():
    run_unittest(TestContains)

if __name__ == '__main__':
    test_main()
