""" Tests for the internal type cache in CPython. """
import dis
import unittest
import warnings
from test import support
from test.support import import_helper, requires_specialization, requires_specialization_ft
try:
    from sys import _clear_type_cache
except ImportError:
    _clear_type_cache = None

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module("_testcapi")
_testinternalcapi = import_helper.import_module("_testinternalcapi")
type_get_version = _testcapi.type_get_version
type_assign_specific_version_unsafe = _testinternalcapi.type_assign_specific_version_unsafe
type_assign_version = _testcapi.type_assign_version
type_modified = _testcapi.type_modified

def clear_type_cache():
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", DeprecationWarning)
        _clear_type_cache()

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
            clear_type_cache()
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

        clear_type_cache()

    def test_per_class_limit(self):
        class C:
            x = 0

        type_assign_version(C)
        orig_version = type_get_version(C)
        for i in range(1001):
            C.x = i
            type_assign_version(C)

        new_version = type_get_version(C)
        self.assertEqual(new_version, 0)

    def test_119462(self):

        class Holder:
            value = None

            @classmethod
            def set_value(cls):
                cls.value = object()

        class HolderSub(Holder):
            pass

        for _ in range(1050):
            Holder.set_value()
            HolderSub.value

@support.cpython_only
class TypeCacheWithSpecializationTests(unittest.TestCase):
    def tearDown(self):
        clear_type_cache()

    def _assign_valid_version_or_skip(self, type_):
        type_modified(type_)
        type_assign_version(type_)
        if type_get_version(type_) == 0:
            self.skipTest("Could not assign valid type version")

    def _no_more_versions(self, user_type):
        type_modified(user_type)
        for _ in range(1001):
            type_assign_specific_version_unsafe(user_type, 1000_000_000)
        type_assign_specific_version_unsafe(user_type, 0)
        self.assertEqual(type_get_version(user_type), 0)

    def _all_opnames(self, func):
        return set(instr.opname for instr in dis.Bytecode(func, adaptive=True))

    def _check_specialization(self, func, arg, opname, *, should_specialize):
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            func(arg)

        if should_specialize:
            self.assertNotIn(opname, self._all_opnames(func))
        else:
            self.assertIn(opname, self._all_opnames(func))

    @requires_specialization
    def test_class_load_attr_specialization_user_type(self):
        class A:
            def foo(self):
                pass

        self._assign_valid_version_or_skip(A)

        def load_foo_1(type_):
            type_.foo

        self._check_specialization(load_foo_1, A, "LOAD_ATTR", should_specialize=True)
        del load_foo_1

        self._no_more_versions(A)

        def load_foo_2(type_):
            return type_.foo

        self._check_specialization(load_foo_2, A, "LOAD_ATTR", should_specialize=False)

    @requires_specialization
    def test_class_load_attr_specialization_static_type(self):
        self.assertNotEqual(type_get_version(str), 0)
        self.assertNotEqual(type_get_version(bytes), 0)

        def get_capitalize_1(type_):
            return type_.capitalize

        self._check_specialization(get_capitalize_1, str, "LOAD_ATTR", should_specialize=True)
        self.assertEqual(get_capitalize_1(str)('hello'), 'Hello')
        self.assertEqual(get_capitalize_1(bytes)(b'hello'), b'Hello')

    @requires_specialization
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

        self._no_more_versions(G)

        def load_x_2(instance):
            instance.x

        self._check_specialization(load_x_2, G(), "LOAD_ATTR", should_specialize=False)

    @requires_specialization
    def test_store_attr_specialization_user_type(self):
        class B:
            __slots__ = ("bar",)

        self._assign_valid_version_or_skip(B)

        def store_bar_1(type_):
            type_.bar = 10

        self._check_specialization(store_bar_1, B(), "STORE_ATTR", should_specialize=True)
        del store_bar_1

        self._no_more_versions(B)

        def store_bar_2(type_):
            type_.bar = 10

        self._check_specialization(store_bar_2, B(), "STORE_ATTR", should_specialize=False)

    @requires_specialization_ft
    def test_class_call_specialization_user_type(self):
        class F:
            def __init__(self):
                pass

        self._assign_valid_version_or_skip(F)

        def call_class_1(type_):
            type_()

        self._check_specialization(call_class_1, F, "CALL", should_specialize=True)
        del call_class_1

        self._no_more_versions(F)

        def call_class_2(type_):
            type_()

        self._check_specialization(call_class_2, F, "CALL", should_specialize=False)

    @requires_specialization
    def test_to_bool_specialization_user_type(self):
        class H:
            pass

        self._assign_valid_version_or_skip(H)

        def to_bool_1(instance):
            not instance

        self._check_specialization(to_bool_1, H(), "TO_BOOL", should_specialize=True)
        del to_bool_1

        self._no_more_versions(H)

        def to_bool_2(instance):
            not instance

        self._check_specialization(to_bool_2, H(), "TO_BOOL", should_specialize=False)


if __name__ == "__main__":
    unittest.main()
