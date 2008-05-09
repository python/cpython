import unittest
import sys
from test.test_support import (catch_warning, TestSkipped, run_unittest,
                                TestSkipped)
import warnings

if not sys.py3kwarning:
    raise TestSkipped('%s must be run with the -3 flag' % __name__)


class TestPy3KWarnings(unittest.TestCase):

    def test_type_inequality_comparisons(self):
        expected = 'type inequality comparisons not supported in 3.x'
        with catch_warning() as w:
            self.assertWarning(int < str, w, expected)
        with catch_warning() as w:
            self.assertWarning(type < object, w, expected)

    def test_object_inequality_comparisons(self):
        expected = 'comparing unequal types not supported in 3.x'
        with catch_warning() as w:
            self.assertWarning(str < [], w, expected)
        with catch_warning() as w:
            self.assertWarning(object() < (1, 2), w, expected)

    def test_dict_inequality_comparisons(self):
        expected = 'dict inequality comparisons not supported in 3.x'
        with catch_warning() as w:
            self.assertWarning({} < {2:3}, w, expected)
        with catch_warning() as w:
            self.assertWarning({} <= {}, w, expected)
        with catch_warning() as w:
            self.assertWarning({} > {2:3}, w, expected)
        with catch_warning() as w:
            self.assertWarning({2:3} >= {}, w, expected)

    def test_cell_inequality_comparisons(self):
        expected = 'cell comparisons not supported in 3.x'
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

    def test_code_inequality_comparisons(self):
        expected = 'code inequality comparisons not supported in 3.x'
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
                    'inequality comparisons not supported in 3.x')
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
        expected = "the cmp argument is not supported in 3.x"
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

    def test_sys_exc_clear(self):
        expected = 'sys.exc_clear() not supported in 3.x; use except clauses'
        with catch_warning() as w:
            self.assertWarning(sys.exc_clear(), w, expected)

    def test_methods_members(self):
        expected = '__members__ and __methods__ not supported in 3.x'
        class C:
            __methods__ = ['a']
            __members__ = ['b']
        c = C()
        with catch_warning() as w:
            self.assertWarning(dir(c), w, expected)

    def test_softspace(self):
        expected = 'file.softspace not supported in 3.x'
        with file(__file__) as f:
            with catch_warning() as w:
                self.assertWarning(f.softspace, w, expected)
            def set():
                f.softspace = 0
            with catch_warning() as w:
                self.assertWarning(set(), w, expected)

    def test_buffer(self):
        expected = 'buffer() not supported in 3.x; use memoryview()'
        with catch_warning() as w:
            self.assertWarning(buffer('a'), w, expected)


class TestStdlibRemovals(unittest.TestCase):

    all_platforms = ('audiodev', 'imputil', 'mutex', 'user', 'new')

    def check_removal(self, module_name):
        """Make sure the specified module, when imported, raises a
        DeprecationWarning and specifies itself in the message."""
        original_module = None
        if module_name in sys.modules:
            original_module = sys.modules[module_name]
            del sys.modules[module_name]
        try:
            with catch_warning() as w:
                warnings.filterwarnings("error", ".+ removed",
                                        DeprecationWarning)
                try:
                    __import__(module_name, level=0)
                except DeprecationWarning as exc:
                    self.assert_(module_name in exc.args[0])
                else:
                    self.fail("DeprecationWarning not raised for %s" %
                                module_name)
        finally:
            if original_module:
                sys.modules[module_name] = original_module


    def test_platform_independent_removals(self):
        # Make sure that the modules that are available on all platforms raise
        # the proper DeprecationWarning.
        for module_name in self.all_platforms:
            self.check_removal(module_name)

    def test_os_path_walk(self):
        msg = "In 3.x, os.path.walk is removed in favor of os.walk."
        def dumbo(where, names, args): pass
        for path_mod in ("ntpath", "macpath", "os2emxpath", "posixpath"):
            mod = __import__(path_mod)
            with catch_warning() as w:
                # Since os3exmpath just imports it from ntpath
                warnings.simplefilter("always")
                mod.walk(".", dumbo, None)
            self.assertEquals(str(w.message), msg)


def test_main():
    run_unittest(TestPy3KWarnings, TestStdlibRemovals)

if __name__ == '__main__':
    test_main()
