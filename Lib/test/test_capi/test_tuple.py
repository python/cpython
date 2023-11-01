import unittest
import sys
from test.support import import_helper
_testcapi = import_helper.import_module('_testcapi')

NULL = None
PY_SSIZE_T_MIN = _testcapi.PY_SSIZE_T_MIN
PY_SSIZE_T_MAX = _testcapi.PY_SSIZE_T_MAX


class CAPITest(unittest.TestCase):
    def test_check(self):
        # Test PyList_Check()
        check = _testcapi.tuple_check
        self.assertTrue(check((1, 2)))
        self.assertTrue(check(()))
        self.assertFalse(check({1: 2}))
        self.assertFalse(check([1, 2]))
        self.assertFalse(check(42))
        self.assertFalse(check(object()))

        # CRASHES check(NULL)


    # def test_list_check_exact(self):
    #     # Test PyList_CheckExact()
    #     check = _testcapi.tuple_checkexact
    #     self.assertTrue(check([1]))
    #     self.assertTrue(check([]))
    #     self.assertFalse(check(ListSubclass([1])))
    #     self.assertFalse(check(UserList([1, 2])))
    #     self.assertFalse(check({1: 2}))
    #     self.assertFalse(check(object()))

        # CRASHES check(NULL)