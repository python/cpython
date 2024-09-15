import unittest

from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')

class set_subclass(set):
    pass

class frozenset_subclass(frozenset):
    pass


class TestSetCAPI(unittest.TestCase):
    def assertImmutable(self, action, *args):
        self.assertRaises(SystemError, action, frozenset(), *args)
        self.assertRaises(SystemError, action, frozenset({1}), *args)
        self.assertRaises(SystemError, action, frozenset_subclass(), *args)
        self.assertRaises(SystemError, action, frozenset_subclass({1}), *args)

    def test_set_check(self):
        check = _testcapi.set_check
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertFalse(check(frozenset()))
        self.assertTrue(check(set_subclass()))
        self.assertFalse(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_set_check_exact(self):
        check = _testcapi.set_checkexact
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertFalse(check(frozenset()))
        self.assertFalse(check(set_subclass()))
        self.assertFalse(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_frozenset_check(self):
        check = _testcapi.frozenset_check
        self.assertFalse(check(set()))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertFalse(check(set_subclass()))
        self.assertTrue(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_frozenset_check_exact(self):
        check = _testcapi.frozenset_checkexact
        self.assertFalse(check(set()))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertFalse(check(set_subclass()))
        self.assertFalse(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_anyset_check(self):
        check = _testcapi.anyset_check
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertTrue(check(set_subclass()))
        self.assertTrue(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_anyset_check_exact(self):
        check = _testcapi.anyset_checkexact
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertFalse(check(set_subclass()))
        self.assertFalse(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_set_new(self):
        set_new = _testcapi.set_new
        self.assertEqual(set_new().__class__, set)
        self.assertEqual(set_new(), set())
        self.assertEqual(set_new((1, 1, 2)), {1, 2})
        self.assertEqual(set_new([1, 1, 2]), {1, 2})
        with self.assertRaisesRegex(TypeError, 'object is not iterable'):
            set_new(object())
        with self.assertRaisesRegex(TypeError, 'object is not iterable'):
            set_new(1)
        with self.assertRaisesRegex(TypeError, "unhashable type: 'dict'"):
            set_new((1, {}))

    def test_frozenset_new(self):
        frozenset_new = _testcapi.frozenset_new
        self.assertEqual(frozenset_new().__class__, frozenset)
        self.assertEqual(frozenset_new(), frozenset())
        self.assertEqual(frozenset_new((1, 1, 2)), frozenset({1, 2}))
        self.assertEqual(frozenset_new([1, 1, 2]), frozenset({1, 2}))
        with self.assertRaisesRegex(TypeError, 'object is not iterable'):
            frozenset_new(object())
        with self.assertRaisesRegex(TypeError, 'object is not iterable'):
            frozenset_new(1)
        with self.assertRaisesRegex(TypeError, "unhashable type: 'dict'"):
            frozenset_new((1, {}))

    def test_set_size(self):
        get_size = _testcapi.set_size
        self.assertEqual(get_size(set()), 0)
        self.assertEqual(get_size(frozenset()), 0)
        self.assertEqual(get_size({1, 1, 2}), 2)
        self.assertEqual(get_size(frozenset({1, 1, 2})), 2)
        self.assertEqual(get_size(set_subclass((1, 2, 3))), 3)
        self.assertEqual(get_size(frozenset_subclass((1, 2, 3))), 3)
        with self.assertRaises(SystemError):
            get_size(object())
        # CRASHES: get_size(NULL)

    def test_set_get_size(self):
        get_size = _testcapi.set_get_size
        self.assertEqual(get_size(set()), 0)
        self.assertEqual(get_size(frozenset()), 0)
        self.assertEqual(get_size({1, 1, 2}), 2)
        self.assertEqual(get_size(frozenset({1, 1, 2})), 2)
        self.assertEqual(get_size(set_subclass((1, 2, 3))), 3)
        self.assertEqual(get_size(frozenset_subclass((1, 2, 3))), 3)
        # CRASHES: get_size(NULL)
        # CRASHES: get_size(object())

    def test_set_contains(self):
        contains = _testcapi.set_contains
        for cls in (set, frozenset, set_subclass, frozenset_subclass):
            with self.subTest(cls=cls):
                instance = cls((1, 2))
                self.assertTrue(contains(instance, 1))
                self.assertFalse(contains(instance, 'missing'))
                with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
                    contains(instance, [])
        # CRASHES: contains(instance, NULL)
        # CRASHES: contains(NULL, object())
        # CRASHES: contains(NULL, NULL)

    def test_add(self):
        add = _testcapi.set_add
        for cls in (set, set_subclass):
            with self.subTest(cls=cls):
                instance = cls((1, 2))
                self.assertEqual(add(instance, 1), 0)
                self.assertEqual(instance, {1, 2})
                self.assertEqual(add(instance, 3), 0)
                self.assertEqual(instance, {1, 2, 3})
                with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
                    add(instance, [])
        with self.assertRaises(SystemError):
            add(object(), 1)
        self.assertImmutable(add, 1)
        # CRASHES: add(NULL, object())
        # CRASHES: add(instance, NULL)
        # CRASHES: add(NULL, NULL)

    def test_discard(self):
        discard = _testcapi.set_discard
        for cls in (set, set_subclass):
            with self.subTest(cls=cls):
                instance = cls((1, 2))
                self.assertEqual(discard(instance, 3), 0)
                self.assertEqual(instance, {1, 2})
                self.assertEqual(discard(instance, 1), 1)
                self.assertEqual(instance, {2})
                self.assertEqual(discard(instance, 2), 1)
                self.assertEqual(instance, set())
                self.assertEqual(discard(instance, 2), 0)
                self.assertEqual(instance, set())
                with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
                    discard(instance, [])
        with self.assertRaises(SystemError):
            discard(object(), 1)
        self.assertImmutable(discard, 1)
        # CRASHES: discard(NULL, object())
        # CRASHES: discard(instance, NULL)
        # CRASHES: discard(NULL, NULL)

    def test_pop(self):
        pop = _testcapi.set_pop
        orig = (1, 2)
        for cls in (set, set_subclass):
            with self.subTest(cls=cls):
                instance = cls(orig)
                self.assertIn(pop(instance), orig)
                self.assertEqual(len(instance), 1)
                self.assertIn(pop(instance), orig)
                self.assertEqual(len(instance), 0)
                with self.assertRaises(KeyError):
                    pop(instance)
        with self.assertRaises(SystemError):
            pop(object())
        self.assertImmutable(pop)
        # CRASHES: pop(NULL)

    def test_clear(self):
        clear = _testcapi.set_clear
        for cls in (set, set_subclass):
            with self.subTest(cls=cls):
                instance = cls((1, 2))
                self.assertEqual(clear(instance), 0)
                self.assertEqual(instance, set())
                self.assertEqual(clear(instance), 0)
                self.assertEqual(instance, set())
        with self.assertRaises(SystemError):
            clear(object())
        self.assertImmutable(clear)
        # CRASHES: clear(NULL)


if __name__ == "__main__":
    unittest.main()
