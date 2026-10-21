from test.support import import_helper, Py_GIL_DISABLED, refleak_helper
import unittest

_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')

NULL = None


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
        # Test PyType_GetName(), PyType_GetQualName(),
        # PyType_GetFullyQualifiedName() and PyType_GetModuleName().

        class MyType:
            pass

        from _testlimitedcapi import (
            get_type_name, get_type_qualname,
            get_type_fullyqualname, get_type_module_name)

        from collections import OrderedDict
        ht = _testlimitedcapi.get_heaptype_for_name()
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

        # CRASHES get_type_name(NULL)
        # CRASHES get_type_qualname(NULL)
        # CRASHES get_type_fullyqualname(NULL)
        # CRASHES get_type_module_name(NULL)
        # CRASHES get_type_name(object()): argument must be a type
        # CRASHES get_type_qualname(object()): argument must be a type
        # CRASHES get_type_fullyqualname(object()): argument must be a type
        # CRASHES get_type_module_name(object()): argument must be a type

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

    def test_get_module_by_token(self):
        token = _testcapi.pymodule_get_token(_testcapi)

        heaptype = _testcapi.create_type_with_token('_testcapi.H', 0)
        mod = _testcapi.pytype_getmodulebytoken(heaptype, token)
        self.assertIs(mod, _testcapi)

        class H1(heaptype): pass
        mod = _testcapi.pytype_getmodulebytoken(H1, token)
        self.assertIs(mod, _testcapi)

        with self.assertRaises(TypeError):
            _testcapi.pytype_getmodulebytoken(int, token)

        class H2(int): pass
        with self.assertRaises(TypeError):
            _testcapi.pytype_getmodulebytoken(H2, token)

    def test_freeze(self):
        # test PyType_Freeze()
        type_freeze = _testlimitedcapi.type_freeze

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

        # CRASHES type_freeze(NULL)
        # CRASHES type_freeze(object()): argument must be a type

    @unittest.skipIf(
        Py_GIL_DISABLED and refleak_helper.hunting_for_refleaks(),
        "Specialization failure triggers gh-127773")
    def test_freeze_meta(self):
        """test PyType_Freeze() with overridden MRO"""
        type_freeze = _testlimitedcapi.type_freeze

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

    def test_extension_managed_weakref_nogc_type(self):
        msg = ("type _testcapi.ManagedWeakrefNoGCType "
               "has the Py_TPFLAGS_MANAGED_WEAKREF "
               "flag but not Py_TPFLAGS_HAVE_GC flag")
        with self.assertRaisesRegex(SystemError, msg):
            _testcapi.create_managed_weakref_nogc_type()

    def test_type_ready(self):
        # Test PyType_Ready(): calling it on initialized types
        # must not raise an exception.
        type_ready = _testlimitedcapi.type_ready

        class HeapType:
            pass

        type_ready(int)
        type_ready(dict)
        type_ready(HeapType)

        # CRASHES type_ready(NULL)
        # CRASHES type_ready(123): argument must be a type

    def test_type_clearcache(self):
        # Test PyType_ClearCache()
        type_clearcache = _testlimitedcapi.type_clearcache
        version_tag = type_clearcache()
        self.assertEqual(type(version_tag), int)
        self.assertGreaterEqual(version_tag, 0)

    def test_type_getflags(self):
        # Test PyType_GetFlags()
        type_getflags = _testlimitedcapi.type_getflags

        from _testlimitedcapi import (
            Py_TPFLAGS_HEAPTYPE,
            Py_TPFLAGS_HAVE_GC,
            Py_TPFLAGS_HAVE_FINALIZE,
            Py_TPFLAGS_HAVE_VERSION_TAG,
            Py_TPFLAGS_VALID_VERSION_TAG,
            Py_TPFLAGS_HAVE_VECTORCALL,
            Py_TPFLAGS_DISALLOW_INSTANTIATION,
            Py_TPFLAGS_IMMUTABLETYPE,
            Py_TPFLAGS_READY,
            Py_TPFLAGS_READYING,
            Py_TPFLAGS_LONG_SUBCLASS,
            Py_TPFLAGS_LIST_SUBCLASS,
            Py_TPFLAGS_TUPLE_SUBCLASS,
            Py_TPFLAGS_BYTES_SUBCLASS,
            Py_TPFLAGS_UNICODE_SUBCLASS,
            Py_TPFLAGS_DICT_SUBCLASS,
            Py_TPFLAGS_BASE_EXC_SUBCLASS,
            Py_TPFLAGS_TYPE_SUBCLASS,
            Py_TPFLAGS_IS_ABSTRACT,
            Py_TPFLAGS_BASETYPE,
            _Py_TPFLAGS_MATCH_SELF,
            Py_TPFLAGS_ITEMS_AT_END,
            Py_TPFLAGS_METHOD_DESCRIPTOR,
        )
        from _testcapi import (
            _Py_TPFLAGS_STATIC_BUILTIN,
            Py_TPFLAGS_SEQUENCE,
            Py_TPFLAGS_MAPPING,
            Py_TPFLAGS_INLINE_VALUES,
            Py_TPFLAGS_MANAGED_WEAKREF,
            Py_TPFLAGS_MANAGED_DICT,
        )

        def check_flag(flags, flag, expected):
            self.assertEqual(bool(flags & flag), expected)

        def check_subclasses(test_type, flags):
            for flag, base_type in (
                (Py_TPFLAGS_LONG_SUBCLASS, int),
                (Py_TPFLAGS_LIST_SUBCLASS, list),
                (Py_TPFLAGS_TUPLE_SUBCLASS, tuple),
                (Py_TPFLAGS_BYTES_SUBCLASS, bytes),
                (Py_TPFLAGS_UNICODE_SUBCLASS, str),
                (Py_TPFLAGS_DICT_SUBCLASS, dict),
                (Py_TPFLAGS_BASE_EXC_SUBCLASS, BaseException),
                (Py_TPFLAGS_TYPE_SUBCLASS, type),
            ):
                with self.subTest(test_type=test_type, flag=flag, base_type=base_type):
                    check_flag(flags, flag, issubclass(test_type, base_type))

        def check_type(test_type, static_type, have_gc=False, have_vectorcall=False,
                       is_base_type=True, sequence=False, mapping=False,
                       match_self=True, items_at_end=False):
            heap_type = not static_type

            flags = type_getflags(test_type)
            check_flag(flags, _Py_TPFLAGS_STATIC_BUILTIN, static_type)
            check_flag(flags, Py_TPFLAGS_HEAPTYPE, heap_type)
            check_flag(flags, Py_TPFLAGS_HAVE_GC, have_gc)
            check_subclasses(test_type, flags)
            check_flag(flags, Py_TPFLAGS_HAVE_VECTORCALL, have_vectorcall)
            check_flag(flags, Py_TPFLAGS_DISALLOW_INSTANTIATION, False)
            check_flag(flags, Py_TPFLAGS_IMMUTABLETYPE, static_type)
            check_flag(flags, Py_TPFLAGS_READY, True)
            check_flag(flags, Py_TPFLAGS_READYING, False)
            check_flag(flags, Py_TPFLAGS_IS_ABSTRACT, False)
            check_flag(flags, Py_TPFLAGS_BASETYPE, is_base_type)
            check_flag(flags, Py_TPFLAGS_SEQUENCE, sequence)
            check_flag(flags, Py_TPFLAGS_MAPPING, mapping)

            check_flag(flags, Py_TPFLAGS_INLINE_VALUES, heap_type)
            check_flag(flags, Py_TPFLAGS_MANAGED_WEAKREF, heap_type)
            check_flag(flags, Py_TPFLAGS_MANAGED_DICT, heap_type)
            check_flag(flags, Py_TPFLAGS_ITEMS_AT_END, items_at_end)
            check_flag(flags, Py_TPFLAGS_METHOD_DESCRIPTOR, False)

            check_flag(flags, _Py_TPFLAGS_MATCH_SELF, match_self)

            # Flags kept for backward compatibility
            check_flag(flags, Py_TPFLAGS_HAVE_FINALIZE, False)
            check_flag(flags, Py_TPFLAGS_HAVE_VERSION_TAG, False)
            check_flag(flags, Py_TPFLAGS_VALID_VERSION_TAG, False)

        # Scalar types
        check_type(int, static_type=True)
        check_type(bool, static_type=True,
                   is_base_type=False)
        check_type(float, static_type=True)
        check_type(complex, static_type=True,
                   match_self=False)
        check_type(bytes, static_type=True)
        check_type(bytearray, static_type=True)
        check_type(str, static_type=True)

        # Collection types
        check_type(tuple, static_type=True, have_gc=True,
                   sequence=True)
        check_type(list, static_type=True, have_gc=True,
                   sequence=True)
        check_type(dict, static_type=True, have_gc=True,
                   mapping=True)
        check_type(frozendict, static_type=True, have_gc=True,
                   mapping=True)
        check_type(set, static_type=True, have_gc=True)
        check_type(frozenset, static_type=True, have_gc=True)

        # Other types
        check_type(BaseException, static_type=True, have_gc=True,
                   match_self=False)
        check_type(type, static_type=True, have_gc=True,
                   have_vectorcall=True,
                   match_self=False,
                   items_at_end=True)

        # Heap type
        class HeapType:
            pass
        check_type(HeapType, static_type=False, have_gc=True, match_self=False)

        # CRASHES type_getflags(NULL)

    def test_type_issubtype(self):
        # Test PyType_IsSubtype()
        _type_issubtype = _testlimitedcapi.type_issubtype

        def type_issubtype(type1, type2):
            res = _type_issubtype(type1, type2)
            self.assertIn(res, (0, 1))
            return bool(res)

        class MyList(list):
            pass

        self.assertTrue(type_issubtype(bool, int))
        self.assertTrue(type_issubtype(MyList, list))

        self.assertFalse(type_issubtype(int, type))
        self.assertFalse(type_issubtype(frozendict, dict))
        self.assertFalse(type_issubtype(MyList, tuple))

    def test_type_modified(self):
        # Test PyType_Modified()
        type_modified = _testlimitedcapi.type_modified

        class MyType:
            pass
        type_modified(MyType)

        # CRASHES type_modified(NULL)
        # CRASHES type_modified({}): argument must be a type
