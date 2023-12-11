import unittest
from test.support import import_helper
_testcapi = import_helper.import_module('_testcapi')


class CAPITest(unittest.TestCase):
    def test_tuple_fromarray(self):
        # Test PyTuple_FromArray()
        tuple_fromarray = _testcapi.tuple_fromarraymoveref

        tup = tuple(object() for _ in range(5))
        copy = tuple_fromarray(tup)
        self.assertEqual(copy, tup)

    def test_tuple_fromarraymoveref(self):
        # Test PyTuple_FromArrayMoveRef()
        tuple_fromarraymoveref = _testcapi.tuple_fromarraymoveref

        tup = tuple(object() for _ in range(5))
        copy = tuple_fromarraymoveref(tup)
        self.assertEqual(copy, tup)
