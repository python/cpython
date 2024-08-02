from test.support import import_helper
import unittest

_testcapi = import_helper.import_module('_testcapi')


class TypeTests(unittest.TestCase):
    def test_freeze(self):
        # test PyType_Freeze()
        type_freeze = _testcapi.type_freeze

        # simple case, no inherante
        class MyType:
            pass
        MyType.attr = "mutable"

        type_freeze(MyType)
        with self.assertRaises(TypeError):
            # the class is now immutable
            MyType.attr = "immutable"

        # test MRO: PyType_Freeze() requires base classes to be immutable
        class A: pass
        class B: pass
        class C(B): pass
        class D(A, C): pass

        self.assertEqual(D.mro(), [D, A, C, B, object])
        with self.assertRaises(TypeError):
            type_freeze(D)

        type_freeze(A)
        type_freeze(B)
        type_freeze(C)
        # all parent classes are now immutable, so D can be made immutable
        # as well
        type_freeze(D)
