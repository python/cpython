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
        if orig_version == 0:
            self.skipTest("Could not assign a valid type version")

        type_modified(C)
        type_assign_specific_version_unsafe(C, orig_version + 5)
        type_assign_version(C)  # this should do nothing

        new_version = type_get_version(C)
        self.assertEqual(new_version, orig_version + 5)

        _clear_type_cache()


@support.cpython_only
class TypeCacheWithSpecializationTests(unittest.TestCase):
    def tearDown(self):
        _clear_type_cache()

    def _assign_valid_version_or_skip(self, type_):
        type_modified(type_)
        type_assign_version(type_)
        if type_get_version(type_) == 0:
            self.skipTest("Could not assign valid type version")

    def _assign_and_check_version_0(self, user_type):
        type_modified(user_type)
        type_assign_specific_version_unsafe(user_type, 0)
        self.assertEqual(type_get_version(user_type), 0)

    def _all_opnames(self, func):
        return set(instr.opname for instr in dis.Bytecode(func, adaptive=True))

    def _check_specialization(self, func, arg, opname, *, should_specialize):
        for _ in range(100):
            func(arg)

        if should_specialize:
            self.assertNotIn(opname, self._all_opnames(func))
        else:
            self.assertIn(opname, self._all_opnames(func))

    def test_class_load_attr_specialization_user_type(self):
        class A:
            def foo(self):
                pass

        self._assign_valid_version_or_skip(A)

        def load_foo_1(type_):
            type_.foo

        self._check_specialization(load_foo_1, A, "LOAD_ATTR", should_specialize=True)
        del load_foo_1

        self._assign_and_check_version_0(A)

        def load_foo_2(type_):
            return type_.foo

        self._check_specialization(load_foo_2, A, "LOAD_ATTR", should_specialize=False)

    def test_class_load_attr_specialization_static_type(self):
        self._assign_valid_version_or_skip(str)
        self._assign_valid_version_or_skip(bytes)

        def get_capitalize_1(type_):
            return type_.capitalize

        self._check_specialization(get_capitalize_1, str, "LOAD_ATTR", should_specialize=True)
        self.assertEqual(get_capitalize_1(str)('hello'), 'Hello')
        self.assertEqual(get_capitalize_1(bytes)(b'hello'), b'Hello')
        del get_capitalize_1

        # Permanently overflow the static type version counter, and force str and bytes
        # to have tp_version_tag == 0
        for _ in range(2**16):
            type_modified(str)
            type_assign_version(str)
            type_modified(bytes)
            type_assign_version(bytes)

        self.assertEqual(type_get_version(str), 0)
        self.assertEqual(type_get_version(bytes), 0)

        def get_capitalize_2(type_):
            return type_.capitalize

        self._check_specialization(get_capitalize_2, str, "LOAD_ATTR", should_specialize=False)
        self.assertEqual(get_capitalize_2(str)('hello'), 'Hello')
        self.assertEqual(get_capitalize_2(bytes)(b'hello'), b'Hello')

    def test_property_load_attr_specialization_user_type(self):
        class G:
            @property
            def x(self):
                return 9

        self._assign_valid_version_or_skip(G)

        def load_x_1(instance):
            instance.x

        self._check_specialization(load_x_1, G(), "LOAD_ATTR", should_specialize=True)
        del load_x_1

        self._assign_and_check_version_0(G)

        def load_x_2(instance):
            instance.x

        self._check_specialization(load_x_2, G(), "LOAD_ATTR", should_specialize=False)

    def test_store_attr_specialization_user_type(self):
        class B:
            __slots__ = ("bar",)

        self._assign_valid_version_or_skip(B)

        def store_bar_1(type_):
            type_.bar = 10

        self._check_specialization(store_bar_1, B(), "STORE_ATTR", should_specialize=True)
        del store_bar_1

        self._assign_and_check_version_0(B)

        def store_bar_2(type_):
            type_.bar = 10

        self._check_specialization(store_bar_2, B(), "STORE_ATTR", should_specialize=False)


if __name__ == "__main__":
    unittest.main()
