import itertools
import sys
import types
import unittest

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')
_interpreters = import_helper.import_module('_interpreters')
from _interpreters import NotShareableError


from test import _crossinterp_definitions as defs


BUILTIN_TYPES = [o for _, o in __builtins__.items()
                 if isinstance(o, type)]
EXCEPTION_TYPES = [cls for cls in BUILTIN_TYPES
                   if issubclass(cls, BaseException)]
OTHER_TYPES = [o for n, o in vars(types).items()
               if (isinstance(o, type) and
                  n not in ('DynamicClassAttribute', '_GeneratorWrapper'))]


class _GetXIDataTests(unittest.TestCase):

    MODE = None

    def get_xidata(self, obj, *, mode=None):
        mode = self._resolve_mode(mode)
        return _testinternalcapi.get_crossinterp_data(obj, mode)

    def get_roundtrip(self, obj, *, mode=None):
        mode = self._resolve_mode(mode)
        xid =_testinternalcapi.get_crossinterp_data(obj, mode)
        return _testinternalcapi.restore_crossinterp_data(xid)

    def iter_roundtrip_values(self, values, *, mode=None):
        mode = self._resolve_mode(mode)
        for obj in values:
            with self.subTest(obj):
                xid = _testinternalcapi.get_crossinterp_data(obj, mode)
                got = _testinternalcapi.restore_crossinterp_data(xid)
                yield obj, got

    def assert_roundtrip_identical(self, values, *, mode=None):
        for obj, got in self.iter_roundtrip_values(values, mode=mode):
            # XXX What about between interpreters?
            self.assertIs(got, obj)

    def assert_roundtrip_equal(self, values, *, mode=None, expecttype=None):
        for obj, got in self.iter_roundtrip_values(values, mode=mode):
            self.assertEqual(got, obj)
            self.assertIs(type(got),
                          type(obj) if expecttype is None else expecttype)

#    def assert_roundtrip_equal_not_identical(self, values, *,
#                                            mode=None, expecttype=None):
#        mode = self._resolve_mode(mode)
#        for obj in values:
#            cls = type(obj)
#            with self.subTest(obj):
#                got = self._get_roundtrip(obj, mode)
#                self.assertIsNot(got, obj)
#                self.assertIs(type(got), type(obj))
#                self.assertEqual(got, obj)
#                self.assertIs(type(got),
#                              cls if expecttype is None else expecttype)
#
#    def assert_roundtrip_not_equal(self, values, *, mode=None, expecttype=None):
#        mode = self._resolve_mode(mode)
#        for obj in values:
#            cls = type(obj)
#            with self.subTest(obj):
#                got = self._get_roundtrip(obj, mode)
#                self.assertIsNot(got, obj)
#                self.assertIs(type(got), type(obj))
#                self.assertNotEqual(got, obj)
#                self.assertIs(type(got),
#                              cls if expecttype is None else expecttype)

    def assert_not_shareable(self, values, exctype=None, *, mode=None):
        mode = self._resolve_mode(mode)
        for obj in values:
            with self.subTest(obj):
                with self.assertRaises(NotShareableError) as cm:
                    _testinternalcapi.get_crossinterp_data(obj, mode)
                if exctype is not None:
                    self.assertIsInstance(cm.exception.__cause__, exctype)

    def _resolve_mode(self, mode):
        if mode is None:
            mode = self.MODE
        assert mode
        return mode


class MarshalTests(_GetXIDataTests):

    MODE = 'marshal'

    def test_simple_builtin_singletons(self):
        self.assert_roundtrip_identical([
            True,
            False,
            None,
            Ellipsis,
        ])
        self.assert_not_shareable([
            NotImplemented,
        ])

    def test_simple_builtin_objects(self):
        self.assert_roundtrip_equal([
            # int
            *range(-1, 258),
            sys.maxsize + 1,
            sys.maxsize,
            -sys.maxsize - 1,
            -sys.maxsize - 2,
            2**1000,
            # complex
            1+2j,
            # float
            0.0,
            1.1,
            -1.0,
            0.12345678,
            -0.12345678,
            # bytes
            *(i.to_bytes(2, 'little', signed=True)
              for i in range(-1, 258)),
            b'hello world',
            # str
            'hello world',
            '你好世界',
            '',
        ])
        self.assert_not_shareable([
            object(),
            types.SimpleNamespace(),
        ])

    def test_bytearray(self):
        # bytearray is special because it unmarshals to bytes, not bytearray.
        self.assert_roundtrip_equal([
            bytearray(),
            bytearray(b'hello world'),
        ], expecttype=bytes)

    def test_compound_immutable_builtin_objects(self):
        self.assert_roundtrip_equal([
            # tuple
            (),
            (1,),
            ("hello", "world"),
            (1, True, "hello"),
            # frozenset
            frozenset([1, 2, 3]),
        ])
        # nested
        self.assert_roundtrip_equal([
            # tuple
            ((1,),),
            ((1, 2), (3, 4)),
            ((1, 2), (3, 4), (5, 6)),
            # frozenset
            frozenset([frozenset([1]), frozenset([2]), frozenset([3])]),
        ])

    def test_compound_mutable_builtin_objects(self):
        self.assert_roundtrip_equal([
            # list
            [],
            [1, 2, 3],
            # dict
            {},
            {1: 7, 2: 8, 3: 9},
            # set
            set(),
            {1, 2, 3},
        ])
        # nested
        self.assert_roundtrip_equal([
            [[1], [2], [3]],
            {1: {'a': True}, 2: {'b': False}},
            {(1, 2, 3,)},
        ])

    def test_compound_builtin_objects_with_bad_items(self):
        bogus = object()
        self.assert_not_shareable([
            (bogus,),
            frozenset([bogus]),
            [bogus],
            {bogus: True},
            {True: bogus},
            {bogus},
        ])

    def test_builtin_code(self):
        self.assert_roundtrip_equal([
            *(f.__code__ for f in defs.FUNCTIONS),
            *(f.__code__ for f in defs.FUNCTION_LIKE),
        ])

    def test_builtin_type(self):
        shareable = [
            StopIteration,
        ]
        types = [
            *BUILTIN_TYPES,
            *OTHER_TYPES,
        ]
        self.assert_not_shareable(cls for cls in types
                                  if cls not in shareable)
        self.assert_roundtrip_identical(cls for cls in types
                                        if cls in shareable)

    def test_builtin_function(self):
        functions = [
            len,
            sys.is_finalizing,
            sys.exit,
            _testinternalcapi.get_crossinterp_data,
        ]
        for func in functions:
            assert type(func) is types.BuiltinFunctionType, func

        self.assert_not_shareable(functions)

    def test_builtin_exception(self):
        msg = 'error!'
        try:
            raise Exception
        except Exception as exc:
            caught = exc
        special = {
            BaseExceptionGroup: (msg, [caught]),
            ExceptionGroup: (msg, [caught]),
#            UnicodeError: (None, msg, None, None, None),
            UnicodeEncodeError: ('utf-8', '', 1, 3, msg),
            UnicodeDecodeError: ('utf-8', b'', 1, 3, msg),
            UnicodeTranslateError: ('', 1, 3, msg),
        }
        exceptions = []
        for cls in EXCEPTION_TYPES:
            args = special.get(cls) or (msg,)
            exceptions.append(cls(*args))

        self.assert_not_shareable(exceptions)
        # Note that StopIteration (the type) can be marshalled,
        # but its instances cannot.

    def test_module(self):
        assert type(sys) is types.ModuleType, type(sys)
        assert type(defs) is types.ModuleType, type(defs)
        assert type(unittest) is types.ModuleType, type(defs)

        assert 'emptymod' not in sys.modules
        with import_helper.ready_to_import('emptymod', ''):
            import emptymod

        self.assert_not_shareable([
            sys,
            defs,
            unittest,
            emptymod,
        ])

    def test_user_class(self):
        self.assert_not_shareable(defs.TOP_CLASSES)

        instances = []
        for cls, args in defs.TOP_CLASSES.items():
            instances.append(cls(*args))
        self.assert_not_shareable(instances)

    def test_user_function(self):
        self.assert_not_shareable(defs.TOP_FUNCTIONS)

    def test_user_exception(self):
        self.assert_not_shareable([
            defs.MimimalError('error!'),
            defs.RichError('error!', 42),
        ])


class ShareableTypeTests(_GetXIDataTests):

    MODE = 'xidata'

    def test_singletons(self):
        self.assert_roundtrip_identical([
            None,
            True,
            False,
        ])
        self.assert_not_shareable([
            Ellipsis,
            NotImplemented,
        ])

    def test_types(self):
        self.assert_roundtrip_equal([
            b'spam',
            9999,
        ])

    def test_bytes(self):
        values = (i.to_bytes(2, 'little', signed=True)
                  for i in range(-1, 258))
        self.assert_roundtrip_equal(values)

    def test_strs(self):
        self.assert_roundtrip_equal([
            'hello world',
            '你好世界',
            '',
        ])

    def test_int(self):
        bounds = [sys.maxsize, -sys.maxsize - 1]
        values = itertools.chain(range(-1, 258), bounds)
        self.assert_roundtrip_equal(values)

    def test_non_shareable_int(self):
        ints = [
            sys.maxsize + 1,
            -sys.maxsize - 2,
            2**1000,
        ]
        self.assert_not_shareable(ints, OverflowError)

    def test_float(self):
        self.assert_roundtrip_equal([
            0.0,
            1.1,
            -1.0,
            0.12345678,
            -0.12345678,
        ])

    def test_tuple(self):
        self.assert_roundtrip_equal([
            (),
            (1,),
            ("hello", "world", ),
            (1, True, "hello"),
        ])
        # Test nesting
        self.assert_roundtrip_equal([
            ((1,),),
            ((1, 2), (3, 4)),
            ((1, 2), (3, 4), (5, 6)),
        ])

    def test_tuples_containing_non_shareable_types(self):
        non_shareables = [
                Exception(),
                object(),
        ]
        for s in non_shareables:
            value = tuple([0, 1.0, s])
            with self.subTest(repr(value)):
                with self.assertRaises(NotShareableError):
                    self.get_xidata(value)
            # Check nested as well
            value = tuple([0, 1., (s,)])
            with self.subTest("nested " + repr(value)):
                with self.assertRaises(NotShareableError):
                    self.get_xidata(value)

    # The rest are not shareable.

    def test_object(self):
        self.assert_not_shareable([
            object(),
        ])

    def test_function_object(self):
        for func in defs.FUNCTIONS:
            assert type(func) is types.FunctionType, func
        assert type(defs.SpamOkay.okay) is types.FunctionType, func
        assert type(lambda: None) is types.LambdaType

        self.assert_not_shareable([
            *defs.FUNCTIONS,
            defs.SpamOkay.okay,
            (lambda: None),
        ])

    def test_builtin_function(self):
        functions = [
            len,
            sys.is_finalizing,
            sys.exit,
            _testinternalcapi.get_crossinterp_data,
        ]
        for func in functions:
            assert type(func) is types.BuiltinFunctionType, func

        self.assert_not_shareable(functions)

    def test_function_like(self):
        self.assert_not_shareable(defs.FUNCTION_LIKE)
        self.assert_not_shareable(defs.FUNCTION_LIKE_APPLIED)

    def test_builtin_wrapper(self):
        _wrappers = {
            defs.SpamOkay().okay: types.MethodType,
            [].append: types.BuiltinMethodType,
            dict.__dict__['fromkeys']: types.ClassMethodDescriptorType,
            types.FunctionType.__code__: types.GetSetDescriptorType,
            types.FunctionType.__globals__: types.MemberDescriptorType,
            str.join: types.MethodDescriptorType,
            object().__str__: types.MethodWrapperType,
            object.__init__: types.WrapperDescriptorType,
        }
        for obj, expected in _wrappers.items():
            assert type(obj) is expected, (obj, expected)

        self.assert_not_shareable([
            *_wrappers,
            staticmethod(defs.SpamOkay.okay),
            classmethod(defs.SpamOkay.okay),
            property(defs.SpamOkay.okay),
        ])

    def test_module(self):
        assert type(sys) is types.ModuleType, type(sys)
        assert type(defs) is types.ModuleType, type(defs)
        assert type(unittest) is types.ModuleType, type(defs)

        assert 'emptymod' not in sys.modules
        with import_helper.ready_to_import('emptymod', ''):
            import emptymod

        self.assert_not_shareable([
            sys,
            defs,
            unittest,
            emptymod,
        ])

    def test_class(self):
        self.assert_not_shareable([
            defs.Spam,
            defs.SpamOkay,
            defs.SpamFull,
            defs.SubSpamFull,
            defs.SubTuple,
            defs.EggsNested,
        ])
        self.assert_not_shareable([
            defs.Spam(),
            defs.SpamOkay(),
            defs.SpamFull(1, 2, 3),
            defs.SubSpamFull(1, 2, 3),
            defs.SubTuple([1, 2, 3]),
            defs.EggsNested(),
        ])

    def test_builtin_type(self):
        self.assert_not_shareable([
            *BUILTIN_TYPES,
            *OTHER_TYPES,
        ])

    def test_exception(self):
        self.assert_not_shareable([
            defs.MimimalError('error!'),
        ])

    def test_builtin_exception(self):
        msg = 'error!'
        try:
            raise Exception
        except Exception as exc:
            caught = exc
        special = {
            BaseExceptionGroup: (msg, [caught]),
            ExceptionGroup: (msg, [caught]),
#            UnicodeError: (None, msg, None, None, None),
            UnicodeEncodeError: ('utf-8', '', 1, 3, msg),
            UnicodeDecodeError: ('utf-8', b'', 1, 3, msg),
            UnicodeTranslateError: ('', 1, 3, msg),
        }
        exceptions = []
        for cls in EXCEPTION_TYPES:
            args = special.get(cls) or (msg,)
            exceptions.append(cls(*args))

        self.assert_not_shareable(exceptions)

    def test_builtin_objects(self):
        ns = {}
        exec("""if True:
            try:
                raise Exception
            except Exception as exc:
                TRACEBACK = exc.__traceback__
                FRAME = TRACEBACK.tb_frame
            """, ns, ns)

        self.assert_not_shareable([
            types.MappingProxyType({}),
            types.SimpleNamespace(),
            # types.CodeType
            defs.spam_minimal.__code__,
            defs.spam_full.__code__,
            defs.spam_CC.__code__,
            defs.eggs_closure_C.__code__,
            defs.ham_C_closure.__code__,
            # types.CellType
            types.CellType(),
            # types.FrameType
            ns['FRAME'],
            # types.TracebackType
            ns['TRACEBACK'],
        ])


if __name__ == '__main__':
    unittest.main()
