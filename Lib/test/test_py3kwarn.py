import unittest
import sys
from test.test_support import (catch_warning, TestSkipped, run_unittest,
                                TestSkipped)
import warnings

if not sys.py3kwarning:
    raise TestSkipped('%s must be run with the -3 flag' % __name__)


class TestPy3KWarnings(unittest.TestCase):

    def test_type_inequality_comparisons(self):
        expected = 'type inequality comparisons not supported in 3.x.'
        with catch_warning() as w:
            self.assertWarning(int < str, w, expected)
        with catch_warning() as w:
            self.assertWarning(type < object, w, expected)

    def test_object_inequality_comparisons(self):
        expected = 'comparing unequal types not supported in 3.x.'
        with catch_warning() as w:
            self.assertWarning(str < [], w, expected)
        with catch_warning() as w:
            self.assertWarning(object() < (1, 2), w, expected)

    def test_dict_inequality_comparisons(self):
        expected = 'dict inequality comparisons not supported in 3.x.'
        with catch_warning() as w:
            self.assertWarning({} < {2:3}, w, expected)
        with catch_warning() as w:
            self.assertWarning({} <= {}, w, expected)
        with catch_warning() as w:
            self.assertWarning({} > {2:3}, w, expected)
        with catch_warning() as w:
            self.assertWarning({2:3} >= {}, w, expected)

    def test_cell_inequality_comparisons(self):
        expected = 'cell comparisons not supported in 3.x.'
        def f(x):
            def g():
                return x
            return g
        cell0, = f(0).func_closure
        cell1, = f(1).func_closure
        with catch_warning() as w:
            self.assertWarning(cell0 == cell1, w, expected)
        with catch_warning() as w:
            self.assertWarning(cell0 < cell1, w, expected)

    def test_filter(self):
        from itertools import ifilter
        from future_builtins import filter
        expected = 'ifilter with None as a first argument is not supported '\
                   'in 3.x.  Use a list comprehension instead.'

        with catch_warning() as w:
            self.assertWarning(ifilter(None, []), w, expected)
        with catch_warning() as w:
            self.assertWarning(filter(None, []), w, expected)

    def test_code_inequality_comparisons(self):
        expected = 'code inequality comparisons not supported in 3.x.'
        def f(x):
            pass
        def g(x):
            pass
        with catch_warning() as w:
            self.assertWarning(f.func_code < g.func_code, w, expected)
        with catch_warning() as w:
            self.assertWarning(f.func_code <= g.func_code, w, expected)
        with catch_warning() as w:
            self.assertWarning(f.func_code >= g.func_code, w, expected)
        with catch_warning() as w:
            self.assertWarning(f.func_code > g.func_code, w, expected)

    def test_builtin_function_or_method_comparisons(self):
        expected = ('builtin_function_or_method '
                    'inequality comparisons not supported in 3.x.')
        func = eval
        meth = {}.get
        with catch_warning() as w:
            self.assertWarning(func < meth, w, expected)
        with catch_warning() as w:
            self.assertWarning(func > meth, w, expected)
        with catch_warning() as w:
            self.assertWarning(meth <= func, w, expected)
        with catch_warning() as w:
            self.assertWarning(meth >= func, w, expected)

    def assertWarning(self, _, warning, expected_message):
        self.assertEqual(str(warning.message), expected_message)

    def test_sort_cmp_arg(self):
        expected = "In 3.x, the cmp argument is no longer supported."
        lst = range(5)
        cmp = lambda x,y: -1

        with catch_warning() as w:
            self.assertWarning(lst.sort(cmp=cmp), w, expected)
        with catch_warning() as w:
            self.assertWarning(sorted(lst, cmp=cmp), w, expected)
        with catch_warning() as w:
            self.assertWarning(lst.sort(cmp), w, expected)
        with catch_warning() as w:
            self.assertWarning(sorted(lst, cmp), w, expected)

def test_main():
    run_unittest(TestPy3KWarnings)

if __name__ == '__main__':
    test_main()
