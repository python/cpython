import unittest
from test import support


class CAPITest(unittest.TestCase):

    # Test _PyDict_GetItem_KnownHash()
    @support.cpython_only
    def test_getitem_knownhash(self):
        from _testcapi import dict_getitem_knownhash

        d = {'x': 1, 'y': 2, 'z': 3}
        self.assertEqual(dict_getitem_knownhash(d, 'x', hash('x')), 1)
        self.assertEqual(dict_getitem_knownhash(d, 'y', hash('y')), 2)
        self.assertEqual(dict_getitem_knownhash(d, 'z', hash('z')), 3)

        # not a dict
        self.assertRaises(SystemError, dict_getitem_knownhash, [], 1, hash(1))
        # key does not exist
        self.assertRaises(KeyError, dict_getitem_knownhash, {}, 1, hash(1))

        class Exc(Exception): pass
        class BadEq:
            def __eq__(self, other):
                raise Exc
            def __hash__(self):
                return 7

        k1, k2 = BadEq(), BadEq()
        d = {k1: 1}
        self.assertEqual(dict_getitem_knownhash(d, k1, hash(k1)), 1)
        self.assertRaises(Exc, dict_getitem_knownhash, d, k2, hash(k2))


if __name__ == "__main__":
    unittest.main()
