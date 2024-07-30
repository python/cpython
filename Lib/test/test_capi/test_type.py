from test.support import import_helper
import unittest

_testcapi = import_helper.import_module('_testcapi')


class TypeTests(unittest.TestCase):
    def test_freeze(self):
        # test PyType_Freeze()
        type_freeze = _testcapi.type_freeze

        class MyType:
            pass
        MyType.attr = "mutable"

        type_freeze(MyType)
        with self.assertRaises(TypeError):
            MyType.attr = "immutable"

        # PyType_Freeze() requires base classes to be immutable
        class Mutable:
            "mutable base class"
            pass
        class MyType2(Mutable):
            pass
        with self.assertRaises(TypeError):
            type_freeze(MyType2)
