import copy_reg
import unittest

from test import test_support
from test.pickletester import ExtensionSaver

class C:
    pass


class WithoutSlots(object):
    pass

class WithWeakref(object):
    __slots__ = ('__weakref__',)

class WithPrivate(object):
    __slots__ = ('__spam',)

class WithSingleString(object):
    __slots__ = 'spam'

class WithInherited(WithSingleString):
    __slots__ = ('eggs',)


class CopyRegTestCase(unittest.TestCase):

    def test_class(self):
        self.assertRaises(TypeError, copy_reg.pickle,
                          C, None, None)

    def test_noncallable_reduce(self):
        self.assertRaises(TypeError, copy_reg.pickle,
                          type(1), "not a callable")

    def test_noncallable_constructor(self):
        self.assertRaises(TypeError, copy_reg.pickle,
                          type(1), int, "not a callable")

    def test_bool(self):
        import copy
        self.assertEqual(True, copy.copy(True))

    def test_extension_registry(self):
        mod, func, code = 'junk1 ', ' junk2', 0xabcd
        e = ExtensionSaver(code)
        try:
            # Shouldn't be in registry now.
            self.assertRaises(ValueError, copy_reg.remove_extension,
                              mod, func, code)
            copy_reg.add_extension(mod, func, code)
            # Should be in the registry.
            self.assertTrue(copy_reg._extension_registry[mod, func] == code)
            self.assertTrue(copy_reg._inverted_registry[code] == (mod, func))
            # Shouldn't be in the cache.
            self.assertNotIn(code, copy_reg._extension_cache)
            # Redundant registration should be OK.
            copy_reg.add_extension(mod, func, code)  # shouldn't blow up
            # Conflicting code.
            self.assertRaises(ValueError, copy_reg.add_extension,
                              mod, func, code + 1)
            self.assertRaises(ValueError, copy_reg.remove_extension,
                              mod, func, code + 1)
            # Conflicting module name.
            self.assertRaises(ValueError, copy_reg.add_extension,
                              mod[1:], func, code )
            self.assertRaises(ValueError, copy_reg.remove_extension,
                              mod[1:], func, code )
            # Conflicting function name.
            self.assertRaises(ValueError, copy_reg.add_extension,
                              mod, func[1:], code)
            self.assertRaises(ValueError, copy_reg.remove_extension,
                              mod, func[1:], code)
            # Can't remove one that isn't registered at all.
            if code + 1 not in copy_reg._inverted_registry:
                self.assertRaises(ValueError, copy_reg.remove_extension,
                                  mod[1:], func[1:], code + 1)

        finally:
            e.restore()

        # Shouldn't be there anymore.
        self.assertNotIn((mod, func), copy_reg._extension_registry)
        # The code *may* be in copy_reg._extension_registry, though, if
        # we happened to pick on a registered code.  So don't check for
        # that.

        # Check valid codes at the limits.
        for code in 1, 0x7fffffff:
            e = ExtensionSaver(code)
            try:
                copy_reg.add_extension(mod, func, code)
                copy_reg.remove_extension(mod, func, code)
            finally:
                e.restore()

        # Ensure invalid codes blow up.
        for code in -1, 0, 0x80000000L:
            self.assertRaises(ValueError, copy_reg.add_extension,
                              mod, func, code)

    def test_slotnames(self):
        self.assertEqual(copy_reg._slotnames(WithoutSlots), [])
        self.assertEqual(copy_reg._slotnames(WithWeakref), [])
        expected = ['_WithPrivate__spam']
        self.assertEqual(copy_reg._slotnames(WithPrivate), expected)
        self.assertEqual(copy_reg._slotnames(WithSingleString), ['spam'])
        expected = ['eggs', 'spam']
        expected.sort()
        result = copy_reg._slotnames(WithInherited)
        result.sort()
        self.assertEqual(result, expected)


def test_main():
    test_support.run_unittest(CopyRegTestCase)


if __name__ == "__main__":
    test_main()
