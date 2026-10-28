import gc
import threading
import unittest

from test.support import import_helper
from test.support import threading_helper

# Skip this test if the _testcapi, _testlimitedcapi or _testinternalcapi
# modules aren't available.
_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
_testinternalcapi = import_helper.import_module('_testinternalcapi')


class set_subclass(set):
    pass

class frozenset_subclass(frozenset):
    pass


class BaseSetTests:
    def assertImmutable(self, action, *args):
        self.assertRaises(SystemError, action, frozenset(), *args)
        self.assertRaises(SystemError, action, frozenset({1}), *args)
        self.assertRaises(SystemError, action, frozenset_subclass(), *args)
        self.assertRaises(SystemError, action, frozenset_subclass({1}), *args)


class TestSetCAPI(BaseSetTests, unittest.TestCase):
    def test_set_check(self):
        check = _testlimitedcapi.set_check
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertFalse(check(frozenset()))
        self.assertTrue(check(set_subclass()))
        self.assertFalse(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_set_check_exact(self):
        check = _testlimitedcapi.set_checkexact
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertFalse(check(frozenset()))
        self.assertFalse(check(set_subclass()))
        self.assertFalse(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_frozenset_check(self):
        check = _testlimitedcapi.frozenset_check
        self.assertFalse(check(set()))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertFalse(check(set_subclass()))
        self.assertTrue(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_frozenset_check_exact(self):
        check = _testlimitedcapi.frozenset_checkexact
        self.assertFalse(check(set()))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertFalse(check(set_subclass()))
        self.assertFalse(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_anyset_check(self):
        check = _testlimitedcapi.anyset_check
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertTrue(check(set_subclass()))
        self.assertTrue(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_anyset_check_exact(self):
        check = _testlimitedcapi.anyset_checkexact
        self.assertTrue(check(set()))
        self.assertTrue(check({1, 2}))
        self.assertTrue(check(frozenset()))
        self.assertTrue(check(frozenset({1, 2})))
        self.assertFalse(check(set_subclass()))
        self.assertFalse(check(frozenset_subclass()))
        self.assertFalse(check(object()))
        # CRASHES: check(NULL)

    def test_set_new(self):
        set_new = _testlimitedcapi.set_new
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
        frozenset_new = _testlimitedcapi.frozenset_new
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
        get_size = _testlimitedcapi.set_size
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
        contains = _testlimitedcapi.set_contains
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
        add = _testlimitedcapi.set_add
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

    def test_add_frozenset(self):
       add = _testlimitedcapi.set_add
       frozen_set = frozenset()
       # test adding an element to a non-uniquely referenced frozenset throws an exception
       self.assertRaises(SystemError, add, frozen_set, 1)

    def test_discard(self):
        discard = _testlimitedcapi.set_discard
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
        pop = _testlimitedcapi.set_pop
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
        clear = _testlimitedcapi.set_clear
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


class TestInternalCAPI(BaseSetTests, unittest.TestCase):
    def test_set_update(self):
        update = _testinternalcapi.set_update
        for cls in (set, set_subclass):
            for it in ('ab', ('a', 'b'), ['a', 'b'],
                       set('ab'), set_subclass('ab'),
                       frozenset('ab'), frozenset_subclass('ab')):
                with self.subTest(cls=cls, it=it):
                    instance = cls()
                    self.assertEqual(update(instance, it), 0)
                    self.assertEqual(instance, {'a', 'b'})
                    instance = cls(it)
                    self.assertEqual(update(instance, it), 0)
                    self.assertEqual(instance, {'a', 'b'})
            with self.assertRaisesRegex(TypeError, 'object is not iterable'):
                update(cls(), 1)
            with self.assertRaisesRegex(TypeError, "unhashable type: 'dict'"):
                update(cls(), [{}])
        with self.assertRaises(SystemError):
            update(object(), 'ab')
        self.assertImmutable(update, 'ab')
        # CRASHES: update(NULL, object())
        # CRASHES: update(instance, NULL)
        # CRASHES: update(NULL, NULL)

    def test_set_next_entry(self):
        set_next = _testinternalcapi.set_next_entry
        for cls in (set, set_subclass, frozenset, frozenset_subclass):
            with self.subTest(cls=cls):
                instance = cls('abc')
                pos = 0
                items = []
                while True:
                    res = set_next(instance, pos)
                    if res is None:
                        break
                    rc, pos, hash_, item = res
                    items.append(item)
                    self.assertEqual(rc, 1)
                    self.assertIn(item, instance)
                    self.assertEqual(hash(item), hash_)
                self.assertEqual(items, list(instance))
        with self.assertRaises(SystemError):
            set_next(object(), 0)
        # CRASHES: set_next(NULL, 0)


class FrozenSetWriterTests(unittest.TestCase):
    PySet_MINSIZE = _testcapi.PySet_MINSIZE
    UNHASHABLE = [1, 2, 3]

    def create_writer(self, size=0):
        # Test PyFrozenSetWriter_Create()
        return _testcapi.PyFrozenSetWriter(size)

    def frozenset_equal(self, result, expected):
        if not isinstance(expected, frozenset):
            expected = frozenset(expected)
        self.assertEqual(result, expected)
        self.assertEqual(type(result), frozenset)

    def assert_unhashable(self):
        err_msg = f"unhashable type: '{type(self.UNHASHABLE).__name__}'"
        return self.assertRaisesRegex(TypeError, err_msg)

    def test_empty(self):
        for size in (0, 128):
            with self.subTest(size=size):
                writer = self.create_writer(size)
                empty_tuple = ()
                writer.update(empty_tuple)
                writer.update_array(empty_tuple)
                writer.update_array_steal(empty_tuple)
                self.assertEqual(writer.finish(), frozenset())

    def test_get_table_set(self):
        # Test size parameter of PyFrozenSetWriter_Create()
        PySet_MINSIZE = self.PySet_MINSIZE

        def check_table_size(size, table_size):
            writer = self.create_writer(size)
            self.assertEqual(writer.get_table_size(), table_size)

        check_table_size(0, PySet_MINSIZE)
        check_table_size(PySet_MINSIZE // 2, PySet_MINSIZE)
        check_table_size(PySet_MINSIZE - 1, PySet_MINSIZE * 2)
        check_table_size(PySet_MINSIZE, PySet_MINSIZE * 4)
        check_table_size(2 ** 15 - 1, 2 ** 16)

    def create_items(self, nitem):
        # Use mortal (and unique) objects to be able to detect reference leaks
        return tuple(object() for _ in range(nitem))

    def check_add_method(self, add_method):
        items = self.create_items(self.PySet_MINSIZE * 2 - 1)
        for prealloc in (0, len(items) // 2, len(items), len(items) * 10):
            with self.subTest(prealloc=prealloc):
                writer = self.create_writer()
                add = getattr(writer, add_method)
                for item in items:
                    add(item)
                self.frozenset_equal(writer.finish(), items)

        # Test error (unhashable type)
        writer = self.create_writer(0)
        writer.update('abc')
        add = getattr(writer, add_method)
        with self.assert_unhashable():
            add(self.UNHASHABLE)
        self.frozenset_equal(writer.finish(), frozenset('abc'))

    def test_add(self):
        # Test PyFrozenSetWriter_Add()
        self.check_add_method('add')

    def test_add_steal(self):
        # Test PyFrozenSetWriter_AddSteal()
        self.check_add_method('add_steal')

    def check_update_method(self, update_method):
        items = self.create_items(self.PySet_MINSIZE - 1)
        items2 = self.create_items(self.PySet_MINSIZE - 1)
        common = self.create_items(self.PySet_MINSIZE - 1)
        expected = set(items) | set(items2) | set(common)
        total = len(expected)
        self.assertEqual(total, len(items) + len(items2) + len(common))
        empty = ()

        for prealloc in (0, total // 2, total, total * 10):
            with self.subTest(prealloc=prealloc):
                writer = self.create_writer(prealloc)
                update = getattr(writer, update_method)
                update(items + common)
                update(common + items2)
                update(empty)  # test nitem=0
                self.frozenset_equal(writer.finish(), expected)

        # Test error path: first or last item is not hashable
        for first in (True, False):
            with self.subTest(first=first):
                if first:
                    items_error = (self.UNHASHABLE, *items)
                    expected = frozenset()
                else:
                    items_error = (*items, self.UNHASHABLE)
                    expected = frozenset(items)
                writer = self.create_writer(0)
                update = getattr(writer, update_method)
                with self.assert_unhashable():
                    update(items_error)
                self.frozenset_equal(writer.finish(), expected)

    def test_update(self):
        # Test PyFrozenSetWriter_Update()
        self.check_update_method('update')

    def test_update_array(self):
        # Test PyFrozenSetWriter_UpdateArray()
        self.check_update_method('update_array')

    def test_update_array_steal(self):
        # Test PyFrozenSetWriter_UpdateArraySteal()
        self.check_update_method('update_array_steal')

    def test_finish(self):
        # Test PyFrozenSetWriter_Finish(): only track the frozenset in the GC
        # if at least one item is tracked by the GC

        # Integers are not tracked by the GC
        items = [2, 5, 7]
        self.assertTrue(all(not gc.is_tracked(item) for item in items))
        writer = self.create_writer()
        writer.update(items)
        self.assertFalse(gc.is_tracked(writer.finish()))

        # Create an object tracked by the GC
        class MyObject:
            pass
        obj = MyObject()
        self.assertTrue(gc.is_tracked(obj))

        writer = self.create_writer()
        writer.add(obj)
        self.assertTrue(gc.is_tracked(writer.finish()))

        # test again, now using PyFrozenSetWriter_Update()
        writer = self.create_writer()
        writer.update([obj])
        self.assertTrue(gc.is_tracked(writer.finish()))

    def test_discard(self):
        # Test PyFrozenSetWriter_Discard(): must not leak memory

        writer = self.create_writer(0)
        writer.discard()

        for prealloc in (0, 3, 100):
            for items in (
                (),          # empty
                [2, 5, 7],   # use the small table
                range(100),  # allocate a new table
            ):
                with self.subTest(prealloc=prealloc, items=items):
                    writer = self.create_writer(prealloc)
                    writer.update(items)
                    writer.discard()

    @threading_helper.requires_working_threading()
    def test_thread(self):
        # PyFrozenSetWriter can be used by multiple threads: it's up to the caller
        # to implement a lock to prevent concurrent accesses.
        writer = self.create_writer()

        def thread_func(writer):
            writer.add('a')
            writer.add_steal('b')
            writer.update(('c',))
            writer.update_array(('d',))
            writer.update_array_steal(('e',))

        thread = threading.Thread(target=thread_func, args=(writer,))
        thread.start()
        threading_helper.join_thread(thread)

        self.frozenset_equal(writer.finish(), frozenset('abcde'))


if __name__ == "__main__":
    unittest.main()
