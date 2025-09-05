import copyreg
import unittest

from test.pickletester import ExtensionSaver

class C:
    pass

def pickle_C(c):
    return C, ()


class WithoutSlots(object):
    pass

class WithWeakref(object):
    __slots__ = ('__weakref__',)

class WithPrivate(object):
    __slots__ = ('__spam',)

class _WithLeadingUnderscoreAndPrivate(object):
    __slots__ = ('__spam',)

class ___(object):
    __slots__ = ('__spam',)

class WithSingleString(object):
    __slots__ = 'spam'

class WithInherited(WithSingleString):
    __slots__ = ('eggs',)


class CopyRegTestCase(unittest.TestCase):

    def test_class(self):
        copyreg.pickle(C, pickle_C)

    def test_noncallable_reduce(self):
        self.assertRaises(TypeError, copyreg.pickle,
                          C, "not a callable")

    def test_noncallable_constructor(self):
        self.assertRaises(TypeError, copyreg.pickle,
                          C, pickle_C, "not a callable")

    def test_bool(self):
        import copy
        self.assertEqual(True, copy.copy(True))

    def test_extension_registry(self):
        mod, func, code = 'junk1 ', ' junk2', 0xabcd
        e = ExtensionSaver(code)
        try:
            # Shouldn't be in registry now.
            self.assertRaises(ValueError, copyreg.remove_extension,
                              mod, func, code)
            copyreg.add_extension(mod, func, code)
            # Should be in the registry.
            self.assertTrue(copyreg._extension_registry[mod, func] == code)
            self.assertTrue(copyreg._inverted_registry[code] == (mod, func))
            # Shouldn't be in the cache.
            self.assertNotIn(code, copyreg._extension_cache)
            # Redundant registration should be OK.
            copyreg.add_extension(mod, func, code)  # shouldn't blow up
            # Conflicting code.
            self.assertRaises(ValueError, copyreg.add_extension,
                              mod, func, code + 1)
            self.assertRaises(ValueError, copyreg.remove_extension,
                              mod, func, code + 1)
            # Conflicting module name.
            self.assertRaises(ValueError, copyreg.add_extension,
                              mod[1:], func, code )
            self.assertRaises(ValueError, copyreg.remove_extension,
                              mod[1:], func, code )
            # Conflicting function name.
            self.assertRaises(ValueError, copyreg.add_extension,
                              mod, func[1:], code)
            self.assertRaises(ValueError, copyreg.remove_extension,
                              mod, func[1:], code)
            # Can't remove one that isn't registered at all.
            if code + 1 not in copyreg._inverted_registry:
                self.assertRaises(ValueError, copyreg.remove_extension,
                                  mod[1:], func[1:], code + 1)

        finally:
            e.restore()

        # Shouldn't be there anymore.
        self.assertNotIn((mod, func), copyreg._extension_registry)
        # The code *may* be in copyreg._extension_registry, though, if
        # we happened to pick on a registered code.  So don't check for
        # that.

        # Check valid codes at the limits.
        for code in 1, 0x7fffffff:
            e = ExtensionSaver(code)
            try:
                copyreg.add_extension(mod, func, code)
                copyreg.remove_extension(mod, func, code)
            finally:
                e.restore()

        # Ensure invalid codes blow up.
        for code in -1, 0, 0x80000000:
            self.assertRaises(ValueError, copyreg.add_extension,
                              mod, func, code)

    def test_slotnames(self):
        self.assertEqual(copyreg._slotnames(WithoutSlots), [])
        self.assertEqual(copyreg._slotnames(WithWeakref), [])
        expected = ['_WithPrivate__spam']
        self.assertEqual(copyreg._slotnames(WithPrivate), expected)
        expected = ['_WithLeadingUnderscoreAndPrivate__spam']
        self.assertEqual(copyreg._slotnames(_WithLeadingUnderscoreAndPrivate),
                         expected)
        self.assertEqual(copyreg._slotnames(___), ['__spam'])
        self.assertEqual(copyreg._slotnames(WithSingleString), ['spam'])
        expected = ['eggs', 'spam']
        expected.sort()
        result = copyreg._slotnames(WithInherited)
        result.sort()
        self.assertEqual(result, expected)


if __name__ == "__main__":
    unittest.main()
