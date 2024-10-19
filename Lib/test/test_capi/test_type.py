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
        err_msg = "cannot set 'attr' attribute of immutable type 'MyType'"
        with self.assertRaisesRegex(TypeError, err_msg):
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

    def test_freeze_meta(self):
        """test PyType_Freeze() with overridden MRO"""
        type_freeze = _testcapi.type_freeze

        class Base:
            value = 1

        class Meta(type):
            def mro(cls):
                return (cls, Base, object)

        class FreezeThis(metaclass=Meta):
            """This has `Base` in the MRO, but not tp_bases"""

        self.assertEqual(FreezeThis.value, 1)

        with self.assertRaises(TypeError):
            type_freeze(FreezeThis)

        Base.value = 2
        self.assertEqual(FreezeThis.value, 2)

        type_freeze(Base)
        with self.assertRaises(TypeError):
            Base.value = 3
        type_freeze(FreezeThis)
        self.assertEqual(FreezeThis.value, 2)
