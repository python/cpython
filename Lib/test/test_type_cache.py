""" Tests for the internal type cache in CPython. """
import collections.abc
import dis
import sys
import unittest
import warnings
from test import support
from test.support import import_helper, requires_specialization
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

    def test_abc_register_invalidates_subclass_versions(self):
        class Parent:
            pass

        class Child(Parent):
            pass

        type_assign_version(Parent)
        type_assign_version(Child)
        parent_version = type_get_version(Parent)
        child_version = type_get_version(Child)
        if parent_version == 0 or child_version == 0:
            self.skipTest("Could not assign valid type versions")

        collections.abc.Mapping.register(Parent)

        self.assertEqual(type_get_version(Parent), 0)
        self.assertEqual(type_get_version(Child), 0)

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

    @requires_specialization
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


@support.cpython_only
class PerTypeLookupCacheTests(unittest.TestCase):
    """Tests for the per-type lookup cache."""

    type_cache_lookup = staticmethod(_testinternalcapi.type_cache_lookup)
    type_cache_invalidate = staticmethod(_testinternalcapi.type_cache_invalidate)

    def _make_type(self):
        class C:
            x = "x-value"
        return C

    def test_lookup_miss_on_empty_cache(self):
        # A freshly-created type has not cached any names yet; the cache
        # should report a miss for an arbitrary name.
        C = self._make_type()
        hit, value, version = self.type_cache_lookup(C, "x")
        self.assertEqual(hit, 0)
        self.assertIsNone(value)
        self.assertEqual(version, 0)

    def test_lookup_hit_after_access(self):
        # Reading an attribute goes through _PyType_Lookup which
        # caches the result. Subsequent lookups for the same name
        # should hit the cache.
        C = self._make_type()
        hit, value, version = self.type_cache_lookup(C, "x")
        self.assertEqual(hit, 0)
        attr = C.x
        hit, value, version = self.type_cache_lookup(C, "x")
        self.assertEqual(hit, 1)
        self.assertIs(value, attr)
        self.assertNotEqual(version, 0)
        self.assertEqual(version, type_get_version(C))

    def test_lookup_caches_missing_name(self):
        # _PyType_Lookup caches negative results too: a name that is not in
        # the MRO should still produce a cache hit with a None value.
        C = self._make_type()
        with self.assertRaises(AttributeError):
            C.does_not_exist
        hit, value, _ = self.type_cache_lookup(C, "does_not_exist")
        self.assertEqual(hit, 1)
        self.assertIsNone(value)

    def test_lookup_on_static_type(self):
        # The cache for static types is stored on interpreter for isolation
        # between subinterpreters, test that cache works for them as well.
        self.type_cache_invalidate(int)
        name = sys.intern("bit_length")
        self.assertEqual(self.type_cache_lookup(int, name)[0], 0)
        attr = getattr(int, name)
        hit, value, _ = self.type_cache_lookup(int, name)
        self.assertEqual(hit, 1)
        self.assertIs(value, attr)

    def test_invalidate_clears_cache(self):
        C = self._make_type()
        C.x  # populate cache
        self.assertEqual(self.type_cache_lookup(C, "x")[0], 1)

        self.type_cache_invalidate(C)
        hit, value, _ = self.type_cache_lookup(C, "x")
        self.assertEqual(hit, 0)
        self.assertIsNone(value)

    def test_setattr_invalidates_cache(self):
        # Mutating a type's attributes must invalidate any cached entries
        # for that type.
        C = self._make_type()
        C.x
        self.assertEqual(self.type_cache_lookup(C, "x")[0], 1)

        C.x = "new-value"
        hit, _, _ = self.type_cache_lookup(C, "x")
        self.assertEqual(hit, 0)

        # The next access should re-populate the cache with the new value.
        self.assertEqual(C.x, "new-value")
        hit, value, _ = self.type_cache_lookup(C, "x")
        self.assertEqual(hit, 1)
        self.assertEqual(value, "new-value")

    def test_setattr_on_subclass_preserves_base(self):
        # Adding an attribute to a subclass changes the lookup result for
        # the subclass, so its cache must be invalidated, but the base's
        # cache for the same name stays valid.
        class Base:
            x = "base"
        class Sub(Base):
            pass

        self.assertEqual(Sub.x, "base")
        self.assertEqual(Base.x, "base")
        self.assertEqual(self.type_cache_lookup(Sub, "x")[0], 1)
        self.assertEqual(self.type_cache_lookup(Base, "x")[0], 1)

        Sub.x = "sub"
        # Sub's cache should be invalidated.
        self.assertEqual(self.type_cache_lookup(Sub, "x")[0], 0)
        # Base is untouched.
        hit, value, _ = self.type_cache_lookup(Base, "x")
        self.assertEqual(hit, 1)
        self.assertEqual(value, "base")

    def test_setattr_on_base_invalidates_subclass(self):
        class Base:
            x = "base"
        class Sub(Base):
            pass

        Sub.x
        self.assertEqual(self.type_cache_lookup(Sub, "x")[0], 1)

        Base.x = "new-base"
        # Modifying the base must invalidate the subclass cache too.
        self.assertEqual(self.type_cache_lookup(Sub, "x")[0], 0)

    def test_lookup_detects_stale_cache_version(self):
        # The cache stores the type's tp_version_tag alongside its entries
        # and re-checks it after locating a hit. If the type version moves
        # forward without the cache being invalidated (the race window in
        # lock-free invalidation), the consistency check must downgrade
        # the hit to a miss.
        C = self._make_type()
        C.x  # populate cache
        orig_version = type_get_version(C)
        self.assertNotEqual(orig_version, 0)
        self.assertEqual(self.type_cache_lookup(C, "x")[0], 1)

        # Bump the type version directly without touching the cache slot
        # (PyType_Modified would also invalidate, defeating the test).
        type_assign_specific_version_unsafe(C, orig_version + 1)
        self.assertEqual(type_get_version(C), orig_version + 1)

        hit, value, _ = self.type_cache_lookup(C, "x")
        self.assertEqual(hit, 0)
        self.assertIsNone(value)

    def test_setattr_on_unrelated_type_preserves_cache(self):
        # Modifying one type must not invalidate a sibling's cache.
        class A:
            x = "a"
        class B:
            x = "b"

        A.x
        B.x
        self.assertEqual(self.type_cache_lookup(A, "x")[0], 1)
        self.assertEqual(self.type_cache_lookup(B, "x")[0], 1)

        B.x = "b2"
        # A's cache is unaffected.
        hit, value, _ = self.type_cache_lookup(A, "x")
        self.assertEqual(hit, 1)
        self.assertEqual(value, "a")


if __name__ == "__main__":
    unittest.main()
