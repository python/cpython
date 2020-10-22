"""
Test implementation of the PEP 509: dictionary versionning.
"""
import unittest
from test.support import import_helper

# PEP 509 is implemented in CPython but other Python implementations
# don't require to implement it
_testcapi = import_helper.import_module('_testcapi')


class DictVersionTests(unittest.TestCase):
    type2test = dict

    def setUp(self):
        self.seen_versions = set()
        self.dict = None

    def check_version_unique(self, mydict):
        version = _testcapi.dict_get_version(mydict)
        self.assertNotIn(version, self.seen_versions)
        self.seen_versions.add(version)

    def check_version_changed(self, mydict, method, *args, **kw):
        result = method(*args, **kw)
        self.check_version_unique(mydict)
        return result

    def check_version_dont_change(self, mydict, method, *args, **kw):
        version1 = _testcapi.dict_get_version(mydict)
        self.seen_versions.add(version1)

        result = method(*args, **kw)

        version2 = _testcapi.dict_get_version(mydict)
        self.assertEqual(version2, version1, "version changed")

        return  result

    def new_dict(self, *args, **kw):
        d = self.type2test(*args, **kw)
        self.check_version_unique(d)
        return d

    def test_constructor(self):
        # new empty dictionaries must all have an unique version
        empty1 = self.new_dict()
        empty2 = self.new_dict()
        empty3 = self.new_dict()

        # non-empty dictionaries must also have an unique version
        nonempty1 = self.new_dict(x='x')
        nonempty2 = self.new_dict(x='x', y='y')

    def test_copy(self):
        d = self.new_dict(a=1, b=2)

        d2 = self.check_version_dont_change(d, d.copy)

        # dict.copy() must create a dictionary with a new unique version
        self.check_version_unique(d2)

    def test_setitem(self):
        d = self.new_dict()

        # creating new keys must change the version
        self.check_version_changed(d, d.__setitem__, 'x', 'x')
        self.check_version_changed(d, d.__setitem__, 'y', 'y')

        # changing values must change the version
        self.check_version_changed(d, d.__setitem__, 'x', 1)
        self.check_version_changed(d, d.__setitem__, 'y', 2)

    def test_setitem_same_value(self):
        value = object()
        d = self.new_dict()

        # setting a key must change the version
        self.check_version_changed(d, d.__setitem__, 'key', value)

        # setting a key to the same value with dict.__setitem__
        # must change the version
        self.check_version_dont_change(d, d.__setitem__, 'key', value)

        # setting a key to the same value with dict.update
        # must change the version
        self.check_version_dont_change(d, d.update, key=value)

        d2 = self.new_dict(key=value)
        self.check_version_dont_change(d, d.update, d2)

    def test_setitem_equal(self):
        class AlwaysEqual:
            def __eq__(self, other):
                return True

        value1 = AlwaysEqual()
        value2 = AlwaysEqual()
        self.assertTrue(value1 == value2)
        self.assertFalse(value1 != value2)
        self.assertIsNot(value1, value2)

        d = self.new_dict()
        self.check_version_changed(d, d.__setitem__, 'key', value1)
        self.assertIs(d['key'], value1)

        # setting a key to a value equal to the current value
        # with dict.__setitem__() must change the version
        self.check_version_changed(d, d.__setitem__, 'key', value2)
        self.assertIs(d['key'], value2)

        # setting a key to a value equal to the current value
        # with dict.update() must change the version
        self.check_version_changed(d, d.update, key=value1)
        self.assertIs(d['key'], value1)

        d2 = self.new_dict(key=value2)
        self.check_version_changed(d, d.update, d2)
        self.assertIs(d['key'], value2)

    def test_setdefault(self):
        d = self.new_dict()

        # setting a key with dict.setdefault() must change the version
        self.check_version_changed(d, d.setdefault, 'key', 'value1')

        # don't change the version if the key already exists
        self.check_version_dont_change(d, d.setdefault, 'key', 'value2')

    def test_delitem(self):
        d = self.new_dict(key='value')

        # deleting a key with dict.__delitem__() must change the version
        self.check_version_changed(d, d.__delitem__, 'key')

        # don't change the version if the key doesn't exist
        self.check_version_dont_change(d, self.assertRaises, KeyError,
                                       d.__delitem__, 'key')

    def test_pop(self):
        d = self.new_dict(key='value')

        # pop() must change the version if the key exists
        self.check_version_changed(d, d.pop, 'key')

        # pop() must not change the version if the key does not exist
        self.check_version_dont_change(d, self.assertRaises, KeyError,
                                       d.pop, 'key')

    def test_popitem(self):
        d = self.new_dict(key='value')

        # popitem() must change the version if the dict is not empty
        self.check_version_changed(d, d.popitem)

        # popitem() must not change the version if the dict is empty
        self.check_version_dont_change(d, self.assertRaises, KeyError,
                                       d.popitem)

    def test_update(self):
        d = self.new_dict(key='value')

        # update() calling with no argument must not change the version
        self.check_version_dont_change(d, d.update)

        # update() must change the version
        self.check_version_changed(d, d.update, key='new value')

        d2 = self.new_dict(key='value 3')
        self.check_version_changed(d, d.update, d2)

    def test_clear(self):
        d = self.new_dict(key='value')

        # clear() must change the version if the dict is not empty
        self.check_version_changed(d, d.clear)

        # clear() must not change the version if the dict is empty
        self.check_version_dont_change(d, d.clear)


class Dict(dict):
    pass


class DictSubtypeVersionTests(DictVersionTests):
    type2test = Dict


if __name__ == "__main__":
    unittest.main()
