""" Tests for the internal type cache in CPython. """
import unittest
import dis
from test import support
from test.support import import_helper
try:
    from sys import _clear_type_cache
except ImportError:
    _clear_type_cache = None

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module("_testcapi")
type_get_version = _testcapi.type_get_version
type_assign_specific_version_unsafe = _testcapi.type_assign_specific_version_unsafe
type_assign_version = _testcapi.type_assign_version
type_modified = _testcapi.type_modified


@support.cpython_only
@unittest.skipIf(_clear_type_cache is None, "requires sys._clear_type_cache")
class TypeCacheTests(unittest.TestCase):
    def test_tp_version_tag_unique(self):
        """tp_version_tag should be unique assuming no overflow, even after
        clearing type cache.
        """
        # Check if global version tag has already overflowed.
        Y = type('Y', (), {})
        Y.x = 1
        Y.x  # Force a _PyType_Lookup, populating version tag
        y_ver = type_get_version(Y)
        # Overflow, or not enough left to conduct the test.
        if y_ver == 0 or y_ver > 0xFFFFF000:
            self.skipTest("Out of type version tags")
        # Note: try to avoid any method lookups within this loop,
        # It will affect global version tag.
        all_version_tags = []
        append_result = all_version_tags.append
        assertNotEqual = self.assertNotEqual
        for _ in range(30):
            _clear_type_cache()
            X = type('Y', (), {})
            X.x = 1
            X.x
            tp_version_tag_after = type_get_version(X)
            assertNotEqual(tp_version_tag_after, 0, msg="Version overflowed")
            append_result(tp_version_tag_after)
        self.assertEqual(len(set(all_version_tags)), 30,
                         msg=f"{all_version_tags} contains non-unique versions")

    def test_type_assign_version(self):
        class C:
            x = 5

        self.assertEqual(type_assign_version(C), 1)
        c_ver = type_get_version(C)

        C.x = 6
        self.assertEqual(type_get_version(C), 0)
        self.assertEqual(type_assign_version(C), 1)
        self.assertNotEqual(type_get_version(C), 0)
        self.assertNotEqual(type_get_version(C), c_ver)

    def test_type_assign_specific_version(self):
        """meta-test for type_assign_specific_version_unsafe"""
        class C:
            pass

        type_assign_version(C)
        orig_version = type_get_version(C)
        self.assertNotEqual(orig_version, 0)

        type_modified(C)
        type_assign_specific_version_unsafe(C, orig_version + 5)
        type_assign_version(C)  # this should do nothing

        new_version = type_get_version(C)
        self.assertEqual(new_version, orig_version + 5)

    def test_specialization_user_type_no_tag_overflow(self):
        class A:
            def foo(self):
                pass

        class B:
            def foo(self):
                pass

        type_modified(A)
        type_assign_version(A)
        type_modified(B)
        type_assign_version(B)
        self.assertNotEqual(type_get_version(A), 0)
        self.assertNotEqual(type_get_version(B), 0)
        self.assertNotEqual(type_get_version(A), type_get_version(B))

        def get_foo(type_):
            return type_.foo

        self.assertIn(
            "LOAD_ATTR",
            [instr.opname for instr in dis.Bytecode(get_foo, adaptive=True)],
        )

        get_foo(A)
        get_foo(A)

        # check that specialization has occurred
        self.assertNotIn(
            "LOAD_ATTR",
            [instr.opname for instr in dis.Bytecode(get_foo, adaptive=True)],
        )

    def test_specialization_user_type_tag_overflow(self):
        class A:
            def foo(self):
                pass

        class B:
            def foo(self):
                pass

        type_modified(A)
        type_assign_specific_version_unsafe(A, 0)
        type_modified(B)
        type_assign_specific_version_unsafe(B, 0)
        self.assertEqual(type_get_version(A), 0)
        self.assertEqual(type_get_version(B), 0)

        def get_foo(type_):
            return type_.foo

        self.assertIn(
            "LOAD_ATTR",
            [instr.opname for instr in dis.Bytecode(get_foo, adaptive=True)],
        )

        get_foo(A)
        get_foo(A)

        # check that specialization has not occurred due to version tag == 0
        self.assertIn(
            "LOAD_ATTR",
            [instr.opname for instr in dis.Bytecode(get_foo, adaptive=True)],
        )


if __name__ == "__main__":
    unittest.main()
