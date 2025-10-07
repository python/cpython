import unittest
import sys
from test.support import cpython_only
try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None


class StackRef:
    def __init__(self, obj, flags):
        self.obj = obj
        self.flags = flags
        self.refcount = sys.getrefcount(obj)

    def __str__(self):
        return f"StackRef(obj={self.obj}, flags={self.flags}, refcount={self.refcount})"

    def __eq__(self, other):
        return (
            self.obj is other.obj
            and self.flags == other.flags
            and self.refcount == other.refcount
        )

    def __hash__(self):
        return tuple.__hash__((self.obj, self.flags, self.refcount))


@cpython_only
class TestStackRef(unittest.TestCase):

    def test_equivalent(self):
        def run_with_refcount_check(self, func, obj):
            refcount = sys.getrefcount(obj)
            res = func(obj)
            self.assertEqual(sys.getrefcount(obj), refcount)
            return res

        def stackref_from_object_borrow(obj):
            return _testinternalcapi.stackref_from_object_borrow(obj)

        funcs = [
            _testinternalcapi.stackref_from_object_new,
            _testinternalcapi.stackref_from_object_new2,
            _testinternalcapi.stackref_from_object_steal_with_incref,
            _testinternalcapi.stackref_make_heap_safe,
            _testinternalcapi.stackref_make_heap_safe_with_borrow,
            _testinternalcapi.stackref_strong_reference,
        ]

        funcs2 = [
            _testinternalcapi.stackref_from_object_borrow,
            _testinternalcapi.stackref_dup_borrowed_with_close,
        ]

        for obj in (None, True, False, 42, '1'):
            results = set()
            for func in funcs + funcs2:
                res = run_with_refcount_check(self, func, obj)
                #print(func.__name__, obj, res)
                results.add(res)
            self.assertEqual(len(results), 1)

        for obj in (5000, range(10)):
            results = set()
            for func in funcs:
                res = run_with_refcount_check(self, func, obj)
                #print(func.__name__, obj, res)
                results.add(res)
            self.assertEqual(len(results), 1)

            results = set()
            for func in funcs2:
                res = run_with_refcount_check(self, func, obj)
                #print(func.__name__, obj, res)
                results.add(res)
            self.assertEqual(len(results), 1)



if __name__ == "__main__":
    unittest.main()
