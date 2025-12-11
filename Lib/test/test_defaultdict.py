"""Unit tests for collections.defaultdict."""

import copy
import pickle
import threading
import time
import unittest

from collections import defaultdict

def foobar():
    return list

class TestDefaultDict(unittest.TestCase):

    def test_basic(self):
        d1 = defaultdict()
        self.assertEqual(d1.default_factory, None)
        d1.default_factory = list
        d1[12].append(42)
        self.assertEqual(d1, {12: [42]})
        d1[12].append(24)
        self.assertEqual(d1, {12: [42, 24]})
        d1[13]
        d1[14]
        self.assertEqual(d1, {12: [42, 24], 13: [], 14: []})
        self.assertTrue(d1[12] is not d1[13] is not d1[14])
        d2 = defaultdict(list, foo=1, bar=2)
        self.assertEqual(d2.default_factory, list)
        self.assertEqual(d2, {"foo": 1, "bar": 2})
        self.assertEqual(d2["foo"], 1)
        self.assertEqual(d2["bar"], 2)
        self.assertEqual(d2[42], [])
        self.assertIn("foo", d2)
        self.assertIn("foo", d2.keys())
        self.assertIn("bar", d2)
        self.assertIn("bar", d2.keys())
        self.assertIn(42, d2)
        self.assertIn(42, d2.keys())
        self.assertNotIn(12, d2)
        self.assertNotIn(12, d2.keys())
        d2.default_factory = None
        self.assertEqual(d2.default_factory, None)
        try:
            d2[15]
        except KeyError as err:
            self.assertEqual(err.args, (15,))
        else:
            self.fail("d2[15] didn't raise KeyError")
        self.assertRaises(TypeError, defaultdict, 1)

    def test_missing(self):
        d1 = defaultdict()
        self.assertRaises(KeyError, d1.__missing__, 42)
        d1.default_factory = list
        self.assertEqual(d1.__missing__(42), [])

    def test_repr(self):
        d1 = defaultdict()
        self.assertEqual(d1.default_factory, None)
        self.assertEqual(repr(d1), "defaultdict(None, {})")
        self.assertEqual(eval(repr(d1)), d1)
        d1[11] = 41
        self.assertEqual(repr(d1), "defaultdict(None, {11: 41})")
        d2 = defaultdict(int)
        self.assertEqual(d2.default_factory, int)
        d2[12] = 42
        self.assertEqual(repr(d2), "defaultdict(<class 'int'>, {12: 42})")
        def foo(): return 43
        d3 = defaultdict(foo)
        self.assertTrue(d3.default_factory is foo)
        d3[13]
        self.assertEqual(repr(d3), "defaultdict(%s, {13: 43})" % repr(foo))

    def test_copy(self):
        d1 = defaultdict()
        d2 = d1.copy()
        self.assertEqual(type(d2), defaultdict)
        self.assertEqual(d2.default_factory, None)
        self.assertEqual(d2, {})
        d1.default_factory = list
        d3 = d1.copy()
        self.assertEqual(type(d3), defaultdict)
        self.assertEqual(d3.default_factory, list)
        self.assertEqual(d3, {})
        d1[42]
        d4 = d1.copy()
        self.assertEqual(type(d4), defaultdict)
        self.assertEqual(d4.default_factory, list)
        self.assertEqual(d4, {42: []})
        d4[12]
        self.assertEqual(d4, {42: [], 12: []})

        # Issue 6637: Copy fails for empty default dict
        d = defaultdict()
        d['a'] = 42
        e = d.copy()
        self.assertEqual(e['a'], 42)

    def test_shallow_copy(self):
        d1 = defaultdict(foobar, {1: 1})
        d2 = copy.copy(d1)
        self.assertEqual(d2.default_factory, foobar)
        self.assertEqual(d2, d1)
        d1.default_factory = list
        d2 = copy.copy(d1)
        self.assertEqual(d2.default_factory, list)
        self.assertEqual(d2, d1)

    def test_deep_copy(self):
        d1 = defaultdict(foobar, {1: [1]})
        d2 = copy.deepcopy(d1)
        self.assertEqual(d2.default_factory, foobar)
        self.assertEqual(d2, d1)
        self.assertTrue(d1[1] is not d2[1])
        d1.default_factory = list
        d2 = copy.deepcopy(d1)
        self.assertEqual(d2.default_factory, list)
        self.assertEqual(d2, d1)

    def test_keyerror_without_factory(self):
        d1 = defaultdict()
        try:
            d1[(1,)]
        except KeyError as err:
            self.assertEqual(err.args[0], (1,))
        else:
            self.fail("expected KeyError")

    def test_recursive_repr(self):
        # Issue2045: stack overflow when default_factory is a bound method
        class sub(defaultdict):
            def __init__(self):
                self.default_factory = self._factory
            def _factory(self):
                return []
        d = sub()
        self.assertRegex(repr(d),
            r"sub\(<bound method .*sub\._factory "
            r"of sub\(\.\.\., \{\}\)>, \{\}\)")

    def test_callable_arg(self):
        self.assertRaises(TypeError, defaultdict, {})

    def test_pickling(self):
        d = defaultdict(int)
        d[1]
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            s = pickle.dumps(d, proto)
            o = pickle.loads(s)
            self.assertEqual(d, o)

    def test_union(self):
        i = defaultdict(int, {1: 1, 2: 2})
        s = defaultdict(str, {0: "zero", 1: "one"})

        i_s = i | s
        self.assertIs(i_s.default_factory, int)
        self.assertDictEqual(i_s, {1: "one", 2: 2, 0: "zero"})
        self.assertEqual(list(i_s), [1, 2, 0])

        s_i = s | i
        self.assertIs(s_i.default_factory, str)
        self.assertDictEqual(s_i, {0: "zero", 1: 1, 2: 2})
        self.assertEqual(list(s_i), [0, 1, 2])

        i_ds = i | dict(s)
        self.assertIs(i_ds.default_factory, int)
        self.assertDictEqual(i_ds, {1: "one", 2: 2, 0: "zero"})
        self.assertEqual(list(i_ds), [1, 2, 0])

        ds_i = dict(s) | i
        self.assertIs(ds_i.default_factory, int)
        self.assertDictEqual(ds_i, {0: "zero", 1: 1, 2: 2})
        self.assertEqual(list(ds_i), [0, 1, 2])

        with self.assertRaises(TypeError):
            i | list(s.items())
        with self.assertRaises(TypeError):
            list(s.items()) | i

        # We inherit a fine |= from dict, so just a few sanity checks here:
        i |= list(s.items())
        self.assertIs(i.default_factory, int)
        self.assertDictEqual(i, {1: "one", 2: 2, 0: "zero"})
        self.assertEqual(list(i), [1, 2, 0])

        with self.assertRaises(TypeError):
            i |= None

    def test_no_value_overwrite_race_condition(self):
        """Test that concurrent access to missing keys doesn't overwrite values."""
        # Use a factory that returns unique objects so we can detect overwrites
        call_count = 0

        def unique_factory():
            nonlocal call_count
            call_count += 1
            # Return a unique object that identifies this call
            return f"value_{call_count}_{threading.get_ident()}"

        d = defaultdict(unique_factory)
        results = {}

        def worker(thread_id):
            # Multiple threads access the same missing key
            for _ in range(5):
                value = d['shared_key']
                if 'shared_key' not in results:
                    results['shared_key'] = value
                # Small delay to increase chance of race conditions
                time.sleep(0.001)

        # Start multiple threads
        threads = []
        for i in range(3):
            t = threading.Thread(target=worker, args=(i,))
            threads.append(t)
            t.start()

        # Wait for all threads to complete
        for t in threads:
            t.join()

        # Key should exist in the dictionary
        self.assertIn('shared_key', d)

        # All threads should see the same value (no overwrites occurred)
        final_value = d['shared_key']
        self.assertEqual(results['shared_key'], final_value)

        # The value should be from the first successful factory call
        self.assertTrue(final_value.startswith('value_'))

        # Factory should only be called once (since key only inserted once)
        self.assertEqual(call_count, 1)

    def test_factory_called_only_when_key_missing(self):
        """Test that factory is only called when key is truly missing."""
        factory_calls = []

        def tracked_factory():
            factory_calls.append(threading.get_ident())
            return [1, 2, 3]

        d = defaultdict(tracked_factory)

        # First access should call factory
        value1 = d['key']
        self.assertEqual(value1, [1, 2, 3])
        initial_call_count = len(factory_calls)

        # Multiple subsequent accesses should not call factory
        for _ in range(10):
            value = d['key']
            self.assertEqual(value, [1, 2, 3])

        # Factory call count should not have increased
        self.assertEqual(len(factory_calls), initial_call_count)


if __name__ == "__main__":
    unittest.main()
