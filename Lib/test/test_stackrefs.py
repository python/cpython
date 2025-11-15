import unittest
import sys
from test.support import cpython_only
try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None

@cpython_only
class TestDefinition(unittest.TestCase):

    def test_equivalence(self):
        def run_with_refcount_check(self, func, obj):
            refcount = sys.getrefcount(obj)
            res = func(obj)
            self.assertEqual(sys.getrefcount(obj), refcount)
            return res

        funcs_with_incref = [
            _testinternalcapi.stackref_from_object_new,
            _testinternalcapi.stackref_from_object_steal_with_incref,
            _testinternalcapi.stackref_make_heap_safe,
            _testinternalcapi.stackref_make_heap_safe_with_borrow,
            _testinternalcapi.stackref_strong_reference,
        ]

        funcs_with_borrow = [
            _testinternalcapi.stackref_from_object_borrow,
            _testinternalcapi.stackref_dup_borrowed_with_close,
        ]

        immortal_objs = (None, True, False, 42, '1')

        for obj in immortal_objs:
            results = set()
            for func in funcs_with_incref + funcs_with_borrow:
                res = run_with_refcount_check(self, func, obj)
                results.add(res)
            self.assertEqual(len(results), 1)

        mortal_objs = (5000, 3+2j, range(10), object())

        for obj in mortal_objs:
            results = set()
            for func in funcs_with_incref:
                res = run_with_refcount_check(self, func, obj)
                results.add(res)
            self.assertEqual(len(results), 1)

            results = set()
            for func in funcs_with_borrow:
                res = run_with_refcount_check(self, func, obj)
                results.add(res)
            self.assertEqual(len(results), 1)



if __name__ == "__main__":
    unittest.main()
