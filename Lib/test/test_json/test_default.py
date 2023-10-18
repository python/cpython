import collections
import collections.abc
from test.test_json import PyTest, CTest


class CustomIndexDict(collections.abc.Mapping, dict):
    # Using `Mapping` here to make this a complete dict subclass, but note the bug in #110941
    # is not specific to `Mapping` subclasses and the outcome would have been the same with a
    # fully manually implemented dict subclass.

    def __init__(self, keys: tuple = ()):
        self._keys = keys

    def __getitem__(self, k):
        try:
            return self._keys.index(k)
        except ValueError:
            raise KeyError(k)

    def __iter__(self):
        return iter(self._keys)

    def __len__(self):
        return len(self._keys)


class TestDefault:
    def test_default(self):
        self.assertEqual(
            self.dumps(type, default=repr),
            self.dumps(repr(type)))

    def test_ordereddict(self):
        od = collections.OrderedDict(a=1, b=2, c=3, d=4)
        od.move_to_end('b')
        self.assertEqual(
            self.dumps(od),
            '{"a": 1, "c": 3, "d": 4, "b": 2}')
        self.assertEqual(
            self.dumps(od, sort_keys=True),
            '{"a": 1, "b": 2, "c": 3, "d": 4}')

    # This should behave identically for PyTest and CTest: see #110941.
    def test_custom_dict(self):
        cd = CustomIndexDict(("b", "a"))
        self.assertEqual(
            self.dumps(cd),
            '{"b": 0, "a": 1}')
        self.assertEqual(
            self.dumps(cd, sort_keys=True),
            '{"a": 1, "b": 0}')

    def test_empty_custom_dict(self):
        # Exercise the fast path when a dict subclass is empty.
        cd = CustomIndexDict()
        self.assertEqual(
            self.dumps(cd),
            '{}')


class TestPyDefault(TestDefault, PyTest): pass
class TestCDefault(TestDefault, CTest): pass
