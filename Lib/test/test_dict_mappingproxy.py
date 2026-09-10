"""Tests for dict() / update / unpacking of types.MappingProxyType.

gh-157217: merging from a mappingproxy wrapping a dict should take the
locked dict-to-dict path instead of iterating the proxy unlocked.
"""

import collections
import types
import unittest


class MappingProxyDictMergeTests(unittest.TestCase):
    def test_update_from_mappingproxy_dict(self):
        d = {}
        d.update(types.MappingProxyType({1: 1, 2: 2, 3: 3}))
        self.assertEqual(d, {1: 1, 2: 2, 3: 3})

    def test_dict_constructor_from_mappingproxy(self):
        view = types.MappingProxyType({'a': 1, 'b': 2})
        self.assertEqual(dict(view), {'a': 1, 'b': 2})
        self.assertEqual({**view}, {'a': 1, 'b': 2})
        dest = {'z': 0}
        dest.update(view)
        self.assertEqual(dest, {'z': 0, 'a': 1, 'b': 2})

    def test_dict_constructor_from_mappingproxy_userdict(self):
        self.assertEqual(
            dict(types.MappingProxyType(collections.UserDict(x=1))),
            {'x': 1},
        )


if __name__ == '__main__':
    unittest.main()
