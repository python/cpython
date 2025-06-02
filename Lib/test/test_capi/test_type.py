from test.support import import_helper, Py_GIL_DISABLED, refleak_helper
import unittest

_testcapi = import_helper.import_module('_testcapi')


class BuiltinStaticTypesTests(unittest.TestCase):

    TYPES = [
        object,
        type,
        int,
        str,
        dict,
        type(None),
        bool,
        BaseException,
        Exception,
        Warning,
        DeprecationWarning,  # Warning subclass
    ]

    def test_tp_bases_is_set(self):
        # PyTypeObject.tp_bases is documented as public API.
        # See https://github.com/python/cpython/issues/105020.
        for typeobj in self.TYPES:
            with self.subTest(typeobj):
                bases = _testcapi.type_get_tp_bases(typeobj)
                self.assertIsNot(bases, None)

    def test_tp_mro_is_set(self):
        # PyTypeObject.tp_bases is documented as public API.
        # See https://github.com/python/cpython/issues/105020.
        for typeobj in self.TYPES:
            with self.subTest(typeobj):
                mro = _testcapi.type_get_tp_mro(typeobj)
                self.assertIsNot(mro, None)


class TypeTests(unittest.TestCase):
    def test_get_type_name(self):
        class MyType:
            pass

        from _testcapi import (
            get_type_name, get_type_qualname,
            get_type_fullyqualname, get_type_module_name)

        from collections import OrderedDict
        ht = _testcapi.get_heaptype_for_name()
        for cls, fullname, modname, qualname, name in (
            (int,
             'int',
             'builtins',
             'int',
             'int'),
            (OrderedDict,
             'collections.OrderedDict',
             'collections',
             'OrderedDict',
             'OrderedDict'),
            (ht,
             '_testcapi.HeapTypeNameType',
             '_testcapi',
             'HeapTypeNameType',
             'HeapTypeNameType'),
            (MyType,
             f'{__name__}.TypeTests.test_get_type_name.<locals>.MyType',
             __name__,
             'TypeTests.test_get_type_name.<locals>.MyType',
             'MyType'),
        ):
            with self.subTest(cls=repr(cls)):
                self.assertEqual(get_type_fullyqualname(cls), fullname)
                self.assertEqual(get_type_module_name(cls), modname)
                self.assertEqual(get_type_qualname(cls), qualname)
                self.assertEqual(get_type_name(cls), name)

        # override __module__
        ht.__module__ = 'test_module'
        self.assertEqual(get_type_fullyqualname(ht), 'test_module.HeapTypeNameType')
        self.assertEqual(get_type_module_name(ht), 'test_module')
        self.assertEqual(get_type_qualname(ht), 'HeapTypeNameType')
        self.assertEqual(get_type_name(ht), 'HeapTypeNameType')

        # override __name__ and __qualname__
        MyType.__name__ = 'my_name'
        MyType.__qualname__ = 'my_qualname'
        self.assertEqual(get_type_fullyqualname(MyType), f'{__name__}.my_qualname')
        self.assertEqual(get_type_module_name(MyType), __name__)
        self.assertEqual(get_type_qualname(MyType), 'my_qualname')
        self.assertEqual(get_type_name(MyType), 'my_name')

        # override also __module__
        MyType.__module__ = 'my_module'
        self.assertEqual(get_type_fullyqualname(MyType), 'my_module.my_qualname')
        self.assertEqual(get_type_module_name(MyType), 'my_module')
        self.assertEqual(get_type_qualname(MyType), 'my_qualname')
        self.assertEqual(get_type_name(MyType), 'my_name')

        # PyType_GetFullyQualifiedName() ignores the module if it's "builtins"
        # or "__main__" of it is not a string
        MyType.__module__ = 'builtins'
        self.assertEqual(get_type_fullyqualname(MyType), 'my_qualname')
        MyType.__module__ = '__main__'
        self.assertEqual(get_type_fullyqualname(MyType), 'my_qualname')
        MyType.__module__ = 123
        self.assertEqual(get_type_fullyqualname(MyType), 'my_qualname')

    def test_get_base_by_token(self):
        def get_base_by_token(src, key, comparable=True):
            def run(use_mro):
                find_first = _testcapi.pytype_getbasebytoken
                ret1, result = find_first(src, key, use_mro, True)
                ret2, no_result = find_first(src, key, use_mro, False)
                self.assertIn(ret1, (0, 1))
                self.assertEqual(ret1, result is not None)
                self.assertEqual(ret1, ret2)
                self.assertIsNone(no_result)
                return result

            found_in_mro = run(True)
            found_in_bases = run(False)
            if comparable:
                self.assertIs(found_in_mro, found_in_bases)
                return found_in_mro
            return found_in_mro, found_in_bases

        create_type = _testcapi.create_type_with_token
        get_token = _testcapi.get_tp_token

        Py_TP_USE_SPEC = _testcapi.Py_TP_USE_SPEC
        self.assertEqual(Py_TP_USE_SPEC, 0)

        A1 = create_type('_testcapi.A1', Py_TP_USE_SPEC)
        self.assertTrue(get_token(A1) != Py_TP_USE_SPEC)

        B1 = create_type('_testcapi.B1', id(self))
        self.assertTrue(get_token(B1) == id(self))

        tokenA1 = get_token(A1)
        # find A1 from A1
        found = get_base_by_token(A1, tokenA1)
        self.assertIs(found, A1)

        # no token in static types
        STATIC = type(1)
        self.assertEqual(get_token(STATIC), 0)
        found = get_base_by_token(STATIC, tokenA1)
        self.assertIs(found, None)

        # no token in pure subtypes
        class A2(A1): pass
        self.assertEqual(get_token(A2), 0)
        # find A1
        class Z(STATIC, B1, A2): pass
        found = get_base_by_token(Z, tokenA1)
        self.assertIs(found, A1)

        # searching for NULL token is an error
        with self.assertRaises(SystemError):
            get_base_by_token(Z, 0)
        with self.assertRaises(SystemError):
            get_base_by_token(STATIC, 0)

        # share the token with A1
        C1 = create_type('_testcapi.C1', tokenA1)
        self.assertTrue(get_token(C1) == tokenA1)

        # find C1 first by shared token
        class Z(C1, A2): pass
        found = get_base_by_token(Z, tokenA1)
        self.assertIs(found, C1)
        # B1 not found
        found = get_base_by_token(Z, get_token(B1))
        self.assertIs(found, None)

        with self.assertRaises(TypeError):
            _testcapi.pytype_getbasebytoken(
                'not a type', id(self), True, False)

    def test_get_module_by_def(self):
        heaptype = _testcapi.create_type_with_token('_testcapi.H', 0)
        mod = _testcapi.pytype_getmodulebydef(heaptype)
        self.assertIs(mod, _testcapi)

        class H1(heaptype): pass
        mod = _testcapi.pytype_getmodulebydef(H1)
        self.assertIs(mod, _testcapi)

        with self.assertRaises(TypeError):
            _testcapi.pytype_getmodulebydef(int)

        class H2(int): pass
        with self.assertRaises(TypeError):
            _testcapi.pytype_getmodulebydef(H2)

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

    @unittest.skipIf(
        Py_GIL_DISABLED and refleak_helper.hunting_for_refleaks(),
        "Specialization failure triggers gh-127773")
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

    def test_manual_heap_type(self):
        # gh-128923: test that a manually allocated and initailized heap type
        # works correctly
        ManualHeapType = _testcapi.ManualHeapType
        for i in range(100):
            self.assertIsInstance(ManualHeapType(), ManualHeapType)

    def test_extension_managed_dict_type(self):
        ManagedDictType = _testcapi.ManagedDictType
        obj = ManagedDictType()
        obj.foo = 42
        self.assertEqual(obj.foo, 42)
        self.assertEqual(obj.__dict__, {'foo': 42})
        obj.__dict__ = {'bar': 3}
        self.assertEqual(obj.__dict__, {'bar': 3})
        self.assertEqual(obj.bar, 3)
