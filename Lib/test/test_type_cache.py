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
type_modified = _testcapi.type_modified


def type_assign_version(type_):
    try:
        type_.x
    except AttributeError:
        pass


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

    def test_load_method_specialization_user_type(self):
        class A:
            def foo(self):
                pass

        self._assign_valid_version_or_skip(A)

        def load_foo_1(instance):
            instance.foo()

        self._check_specialization(
            load_foo_1, A(), "LOAD_METHOD_ADAPTIVE", should_specialize=True
        )
        del load_foo_1

        self._assign_and_check_version_0(A)

        def load_foo_2(instance):
            instance.foo()

        self._check_specialization(
            load_foo_2, A(), "LOAD_METHOD_ADAPTIVE", should_specialize=False
        )

    def test_store_attr_specialization_user_type(self):
        class B:
            __slots__ = ("bar",)

        self._assign_valid_version_or_skip(B)

        def store_bar_1(instance):
            instance.bar = 10

        self._check_specialization(
            store_bar_1, B(), "STORE_ATTR_ADAPTIVE", should_specialize=True
        )
        del store_bar_1

        self._assign_and_check_version_0(B)

        def store_bar_2(instance):
            instance.bar = 10

        self._check_specialization(
            store_bar_2, B(), "STORE_ATTR_ADAPTIVE", should_specialize=False
        )

    def test_load_attr_specialization_user_type(self):
        class C:
            __slots__ = ("biz",)
            def __init__(self):
                self.biz = 8

        self._assign_valid_version_or_skip(C)

        def load_biz_1(type_):
            type_.biz

        self._check_specialization(
            load_biz_1, C(), "LOAD_ATTR_ADAPTIVE", should_specialize=True
        )
        del load_biz_1

        self._assign_and_check_version_0(C)

        def load_biz_2(type_):
            type_.biz

        self._check_specialization(
            load_biz_2, C(), "LOAD_ATTR_ADAPTIVE", should_specialize=False
        )

    def test_binary_subscript_specialization_user_type(self):
        class D:
            def __getitem__(self, _):
                return 1

        self._assign_valid_version_or_skip(D)

        def subscript_1(instance):
            instance[6]

        self._check_specialization(
            subscript_1, D(), "BINARY_SUBSCR_ADAPTIVE", should_specialize=True
        )
        del subscript_1

        self._assign_and_check_version_0(D)

        def subscript_2(instance):
            instance[6]

        self._check_specialization(
            subscript_2, D(), "BINARY_SUBSCR_ADAPTIVE", should_specialize=False
        )



if __name__ == "__main__":
    unittest.main()
