import unittest

from test.support import import_helper

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')

class set_child(set):
    pass

class frozenset_child(frozenset):
    pass


class TestSetCAPI(unittest.TestCase):
    def assertImmutable(self, action, *args):
        self.assertRaises(SystemError, action, frozenset(), *args)
        self.assertRaises(SystemError, action, frozenset({1}), *args)
        self.assertRaises(SystemError, action, frozenset_child(), *args)
        self.assertRaises(SystemError, action, frozenset_child({1}), *args)

    def test_set_check(self):
        check = _testcapi.set_check
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertFalse(check(frozenset()))
        self.assertTrue(check(set_child()))
        self.assertFalse(check(frozenset_child()))
        self.assertFalse(check(object()))

    def test_set_check_exact(self):
        check = _testcapi.set_checkexact
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertFalse(check(frozenset()))
        self.assertFalse(check(set_child()))
        self.assertFalse(check(frozenset_child()))
        self.assertFalse(check(object()))

    def test_frozenset_check(self):
        check = _testcapi.frozenset_check
        self.assertFalse(check(set()))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertFalse(check(set_child()))
        self.assertTrue(check(frozenset_child()))
        self.assertFalse(check(object()))

    def test_frozenset_check_exact(self):
        check = _testcapi.frozenset_checkexact
        self.assertFalse(check(set()))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertFalse(check(set_child()))
        self.assertFalse(check(frozenset_child()))
        self.assertFalse(check(object()))

    def test_anyset_check(self):
        check = _testcapi.anyset_check
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertTrue(check(set_child()))
        self.assertTrue(check(frozenset_child()))
        self.assertFalse(check(object()))

    def test_anyset_check_exact(self):
        check = _testcapi.anyset_checkexact
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertFalse(check(set_child()))
        self.assertFalse(check(frozenset_child()))
        self.assertFalse(check(object()))

    def test_set_new(self):
        new = _testcapi.set_new
        self.assertEqual(new().__class__, set)
        self.assertEqual(new(), set())
        self.assertEqual(new((1, 1, 2)), {1, 2})
        with self.assertRaisesRegex(TypeError, 'object is not iterable'):
            new(object())
        with self.assertRaisesRegex(TypeError, "unhashable type: 'dict'"):
            new((1, {}))

    def test_frozenset_new(self):
        new = _testcapi.frozenset_new
        self.assertEqual(new().__class__, frozenset)
        self.assertEqual(new(), frozenset())
        self.assertEqual(new((1, 1, 2)), frozenset({1, 2}))
        with self.assertRaisesRegex(TypeError, 'object is not iterable'):
            new(object())
        with self.assertRaisesRegex(TypeError, "unhashable type: 'dict'"):
            new((1, {}))

    def test_set_size(self):
        l = _testcapi.set_size
        self.assertEqual(l(set()), 0)
        self.assertEqual(l(frozenset()), 0)
        self.assertEqual(l({1, 1, 2}), 2)
        self.assertEqual(l(frozenset({1, 1, 2})), 2)
        self.assertEqual(l(set_child((1, 2, 3))), 3)
        self.assertEqual(l(frozenset_child((1, 2, 3))), 3)
        with self.assertRaises(SystemError):
            l([])

    def test_set_get_size(self):
        l = _testcapi.set_get_size
        self.assertEqual(l(set()), 0)
        self.assertEqual(l(frozenset()), 0)
        self.assertEqual(l({1, 1, 2}), 2)
        self.assertEqual(l(frozenset({1, 1, 2})), 2)
        self.assertEqual(l(set_child((1, 2, 3))), 3)
        self.assertEqual(l(frozenset_child((1, 2, 3))), 3)
        # CRASHES: l([])

    def test_set_contains(self):
        c = _testcapi.set_contains
        for cls in (set, frozenset, set_child, frozenset_child):
            with self.subTest(cls=cls):
                instance = cls((1, 2))
                self.assertTrue(c(instance, 1))
                self.assertFalse(c(instance, 'missing'))

    def test_add(self):
        add = _testcapi.set_add
        for cls in (set, set_child):
            with self.subTest(cls=cls):
                instance = cls((1, 2))
                self.assertEqual(add(instance, 1), 0)
                self.assertEqual(instance, {1, 2})
                self.assertEqual(add(instance, 3), 0)
                self.assertEqual(instance, {1, 2, 3})
        self.assertImmutable(add, 1)

    def test_discard(self):
        discard = _testcapi.set_discard
        for cls in (set, set_child):
            with self.subTest(cls=cls):
                instance = cls((1, 2))
                self.assertEqual(discard(instance, 3), 0)
                self.assertEqual(instance, {1, 2})
                self.assertEqual(discard(instance, 1), 1)
                self.assertEqual(instance, {2})
                self.assertEqual(discard(instance, 2), 1)
                self.assertEqual(instance, set())
                # Discarding from empty set works
                self.assertEqual(discard(instance, 2), 0)
                self.assertEqual(instance, set())
        self.assertImmutable(discard, 1)

    def test_pop(self):
        pop = _testcapi.set_pop
        orig = (1, 2)
        for cls in (set, set_child):
            with self.subTest(cls=cls):
                instance = cls(orig)
                self.assertIn(pop(instance), orig)
                self.assertEqual(len(instance), 1)
                self.assertIn(pop(instance), orig)
                self.assertEqual(len(instance), 0)
                with self.assertRaises(KeyError):
                    pop(instance)
        self.assertImmutable(pop)

    def test_clear(self):
        clear = _testcapi.set_clear
        for cls in (set, set_child):
            with self.subTest(cls=cls):
                instance = cls((1, 2))
                self.assertEqual(clear(instance), 0)
                self.assertEqual(instance, set())
        self.assertImmutable(clear)
