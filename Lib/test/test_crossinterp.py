import contextlib
import importlib
import importlib.util
import itertools
import sys
import types
import unittest
import warnings

from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')
_interpreters = import_helper.import_module('_interpreters')
from _interpreters import NotShareableError

from test import _code_definitions as code_defs
from test import _crossinterp_definitions as defs


@contextlib.contextmanager
def ignore_byteswarning():
    with warnings.catch_warnings():
        warnings.filterwarnings('ignore', category=BytesWarning)
        yield


# builtin types

BUILTINS_TYPES = [o for _, o in __builtins__.items() if isinstance(o, type)]
EXCEPTION_TYPES = [cls for cls in BUILTINS_TYPES
                   if issubclass(cls, BaseException)]
OTHER_TYPES = [o for n, o in vars(types).items()
               if (isinstance(o, type) and
                   n not in ('DynamicClassAttribute', '_GeneratorWrapper'))]
BUILTIN_TYPES = [
    *BUILTINS_TYPES,
    *OTHER_TYPES,
]

# builtin exceptions

try:
    raise Exception
except Exception as exc:
    CAUGHT = exc
EXCEPTIONS_WITH_SPECIAL_SIG = {
    BaseExceptionGroup: (lambda msg: (msg, [CAUGHT])),
    ExceptionGroup: (lambda msg: (msg, [CAUGHT])),
    UnicodeError: (lambda msg: (None, msg, None, None, None)),
    UnicodeEncodeError: (lambda msg: ('utf-8', '', 1, 3, msg)),
    UnicodeDecodeError: (lambda msg: ('utf-8', b'', 1, 3, msg)),
    UnicodeTranslateError: (lambda msg: ('', 1, 3, msg)),
}
BUILTIN_EXCEPTIONS = [
    *(cls(*sig('error!')) for cls, sig in EXCEPTIONS_WITH_SPECIAL_SIG.items()),
    *(cls('error!') for cls in EXCEPTION_TYPES
      if cls not in EXCEPTIONS_WITH_SPECIAL_SIG),
]

# other builtin objects

METHOD = defs.SpamOkay().okay
BUILTIN_METHOD = [].append
METHOD_DESCRIPTOR_WRAPPER = str.join
METHOD_WRAPPER = object().__str__
WRAPPER_DESCRIPTOR = object.__init__
BUILTIN_WRAPPERS = {
    METHOD: types.MethodType,
    BUILTIN_METHOD: types.BuiltinMethodType,
    dict.__dict__['fromkeys']: types.ClassMethodDescriptorType,
    types.FunctionType.__code__: types.GetSetDescriptorType,
    types.FunctionType.__globals__: types.MemberDescriptorType,
    METHOD_DESCRIPTOR_WRAPPER: types.MethodDescriptorType,
    METHOD_WRAPPER: types.MethodWrapperType,
    WRAPPER_DESCRIPTOR: types.WrapperDescriptorType,
    staticmethod(defs.SpamOkay.okay): None,
    classmethod(defs.SpamOkay.okay): None,
    property(defs.SpamOkay.okay): None,
}
BUILTIN_FUNCTIONS = [
    # types.BuiltinFunctionType
    len,
    sys.is_finalizing,
    sys.exit,
    _testinternalcapi.get_crossinterp_data,
]
assert 'emptymod' not in sys.modules
with import_helper.ready_to_import('emptymod', ''):
    import emptymod as EMPTYMOD
MODULES = [
    sys,
    defs,
    unittest,
    EMPTYMOD,
]
OBJECT = object()
EXCEPTION = Exception()
LAMBDA = (lambda: None)
BUILTIN_SIMPLE = [
    OBJECT,
    # singletons
    None,
    True,
    False,
    Ellipsis,
    NotImplemented,
    # bytes
    *(i.to_bytes(2, 'little', signed=True)
      for i in range(-1, 258)),
    # str
    'hello world',
    '你好世界',
    '',
    # int
    sys.maxsize + 1,
    sys.maxsize,
    -sys.maxsize - 1,
    -sys.maxsize - 2,
    *range(-1, 258),
    2**1000,
    # float
    0.0,
    1.1,
    -1.0,
    0.12345678,
    -0.12345678,
]
TUPLE_EXCEPTION = (0, 1.0, EXCEPTION)
TUPLE_OBJECT = (0, 1.0, OBJECT)
TUPLE_NESTED_EXCEPTION = (0, 1.0, (EXCEPTION,))
TUPLE_NESTED_OBJECT = (0, 1.0, (OBJECT,))
MEMORYVIEW_EMPTY = memoryview(b'')
MEMORYVIEW_NOT_EMPTY = memoryview(b'spam'*42)
MAPPING_PROXY_EMPTY = types.MappingProxyType({})
BUILTIN_CONTAINERS = [
    # tuple (flat)
    (),
    (1,),
    ("hello", "world", ),
    (1, True, "hello"),
    TUPLE_EXCEPTION,
    TUPLE_OBJECT,
    # tuple (nested)
    ((1,),),
    ((1, 2), (3, 4)),
    ((1, 2), (3, 4), (5, 6)),
    TUPLE_NESTED_EXCEPTION,
    TUPLE_NESTED_OBJECT,
    # buffer
    MEMORYVIEW_EMPTY,
    MEMORYVIEW_NOT_EMPTY,
    # list
    [],
    [1, 2, 3],
    [[1], (2,), {3: 4}],
    # dict
    {},
    {1: 7, 2: 8, 3: 9},
    {1: [1], 2: (2,), 3: {3: 4}},
    # set
    set(),
    {1, 2, 3},
    {frozenset({1}), (2,)},
    # frozenset
    frozenset([]),
    frozenset({frozenset({1}), (2,)}),
    # bytearray
    bytearray(b''),
    # other
    MAPPING_PROXY_EMPTY,
    types.SimpleNamespace(),
]
ns = {}
exec("""
try:
    raise Exception
except Exception as exc:
    TRACEBACK = exc.__traceback__
    FRAME = TRACEBACK.tb_frame
""", ns, ns)
BUILTIN_OTHER = [
    # types.CellType
    types.CellType(),
    # types.FrameType
    ns['FRAME'],
    # types.TracebackType
    ns['TRACEBACK'],
]
del ns

# user-defined objects

USER_TOP_INSTANCES = [c(*a) for c, a in defs.TOP_CLASSES.items()]
USER_NESTED_INSTANCES = [c(*a) for c, a in defs.NESTED_CLASSES.items()]
USER_INSTANCES = [
    *USER_TOP_INSTANCES,
    *USER_NESTED_INSTANCES,
]
USER_EXCEPTIONS = [
    defs.MimimalError('error!'),
]

# shareable objects

TUPLES_WITHOUT_EQUALITY = [
    TUPLE_EXCEPTION,
    TUPLE_OBJECT,
    TUPLE_NESTED_EXCEPTION,
    TUPLE_NESTED_OBJECT,
]
_UNSHAREABLE_SIMPLE = [
    Ellipsis,
    NotImplemented,
    OBJECT,
    sys.maxsize + 1,
    -sys.maxsize - 2,
    2**1000,
]
with ignore_byteswarning():
    _SHAREABLE_SIMPLE = [o for o in BUILTIN_SIMPLE
                         if o not in _UNSHAREABLE_SIMPLE]
    _SHAREABLE_CONTAINERS = [
        *(o for o in BUILTIN_CONTAINERS if type(o) is memoryview),
        *(o for o in BUILTIN_CONTAINERS
          if type(o) is tuple and o not in TUPLES_WITHOUT_EQUALITY),
    ]
    _UNSHAREABLE_CONTAINERS = [o for o in BUILTIN_CONTAINERS
                               if o not in _SHAREABLE_CONTAINERS]
SHAREABLE = [
    *_SHAREABLE_SIMPLE,
    *_SHAREABLE_CONTAINERS,
]
NOT_SHAREABLE = [
    *_UNSHAREABLE_SIMPLE,
    *_UNSHAREABLE_CONTAINERS,
    *BUILTIN_TYPES,
    *BUILTIN_WRAPPERS,
    *BUILTIN_EXCEPTIONS,
    *BUILTIN_FUNCTIONS,
    *MODULES,
    *BUILTIN_OTHER,
    # types.CodeType
    *(f.__code__ for f in defs.FUNCTIONS),
    *(f.__code__ for f in defs.FUNCTION_LIKE),
    # types.FunctionType
    *defs.FUNCTIONS,
    defs.SpamOkay.okay,
    LAMBDA,
    *defs.FUNCTION_LIKE,
    # coroutines and generators
    *defs.FUNCTION_LIKE_APPLIED,
    # user classes
    *defs.CLASSES,
    *USER_INSTANCES,
    # user exceptions
    *USER_EXCEPTIONS,
]

# pickleable objects

PICKLEABLE = [
    *BUILTIN_SIMPLE,
    *(o for o in BUILTIN_CONTAINERS if o not in [
        MEMORYVIEW_EMPTY,
        MEMORYVIEW_NOT_EMPTY,
        MAPPING_PROXY_EMPTY,
    ] or type(o) is dict),
    *BUILTINS_TYPES,
    *BUILTIN_EXCEPTIONS,
    *BUILTIN_FUNCTIONS,
    *defs.TOP_FUNCTIONS,
    defs.SpamOkay.okay,
    *defs.FUNCTION_LIKE,
    *defs.TOP_CLASSES,
    *USER_TOP_INSTANCES,
    *USER_EXCEPTIONS,
    # from OTHER_TYPES
    types.NoneType,
    types.EllipsisType,
    types.NotImplementedType,
    types.GenericAlias,
    types.UnionType,
    types.SimpleNamespace,
    # from BUILTIN_WRAPPERS
    METHOD,
    BUILTIN_METHOD,
    METHOD_DESCRIPTOR_WRAPPER,
    METHOD_WRAPPER,
    WRAPPER_DESCRIPTOR,
]
assert not any(isinstance(o, types.MappingProxyType) for o in PICKLEABLE)


# helpers

DEFS = defs
with open(code_defs.__file__) as infile:
    _code_defs_text = infile.read()
with open(DEFS.__file__) as infile:
    _defs_text = infile.read()
    _defs_text = _defs_text.replace('from ', '# from ')
DEFS_TEXT = f"""
#######################################
# from {code_defs.__file__}

{_code_defs_text}

#######################################
# from {defs.__file__}

{_defs_text}
"""
del infile, _code_defs_text, _defs_text


def load_defs(module=None):
    """Return a new copy of the test._crossinterp_definitions module.

    The module's __name__ matches the "module" arg, which is either
    a str or a module.

    If the "module" arg is a module then the just-loaded defs are also
    copied into that module.

    Note that the new module is not added to sys.modules.
    """
    if module is None:
        modname = DEFS.__name__
    elif isinstance(module, str):
        modname = module
        module = None
    else:
        modname = module.__name__
    # Create the new module and populate it.
    defs = import_helper.create_module(modname)
    defs.__file__ = DEFS.__file__
    exec(DEFS_TEXT, defs.__dict__)
    # Copy the defs into the module arg, if any.
    if module is not None:
        for name, value in defs.__dict__.items():
            if name.startswith('_'):
                continue
            assert not hasattr(module, name), (name, getattr(module, name))
            setattr(module, name, value)
    return defs


@contextlib.contextmanager
def using___main__():
    """Make sure __main__ module exists (and clean up after)."""
    modname = '__main__'
    if modname not in sys.modules:
        with import_helper.isolated_modules():
            yield import_helper.add_module(modname)
    else:
        with import_helper.module_restored(modname) as mod:
            yield mod


@contextlib.contextmanager
def temp_module(modname):
    """Create the module and add to sys.modules, then remove it after."""
    assert modname not in sys.modules, (modname,)
    with import_helper.isolated_modules():
        yield import_helper.add_module(modname)


@contextlib.contextmanager
def missing_defs_module(modname, *, prep=False):
    assert modname not in sys.modules, (modname,)
    if prep:
        with import_helper.ready_to_import(modname, DEFS_TEXT):
            yield modname
    else:
        with import_helper.isolated_modules():
            yield modname


class _GetXIDataTests(unittest.TestCase):

    MODE = None

    def assert_functions_equal(self, func1, func2):
        assert type(func1) is types.FunctionType, repr(func1)
        assert type(func2) is types.FunctionType, repr(func2)
        self.assertEqual(func1.__name__, func2.__name__)
        self.assertEqual(func1.__code__, func2.__code__)
        self.assertEqual(func1.__defaults__, func2.__defaults__)
        self.assertEqual(func1.__kwdefaults__, func2.__kwdefaults__)
        # We don't worry about __globals__ for now.

    def assert_exc_args_equal(self, exc1, exc2):
        args1 = exc1.args
        args2 = exc2.args
        if isinstance(exc1, ExceptionGroup):
            self.assertIs(type(args1), type(args2))
            self.assertEqual(len(args1), 2)
            self.assertEqual(len(args1), len(args2))
            self.assertEqual(args1[0], args2[0])
            group1 = args1[1]
            group2 = args2[1]
            self.assertEqual(len(group1), len(group2))
            for grouped1, grouped2 in zip(group1, group2):
                # Currently the "extra" attrs are not preserved
                # (via __reduce__).
                self.assertIs(type(exc1), type(exc2))
                self.assert_exc_equal(grouped1, grouped2)
        else:
            self.assertEqual(args1, args2)

    def assert_exc_equal(self, exc1, exc2):
        self.assertIs(type(exc1), type(exc2))

        if type(exc1).__eq__ is not object.__eq__:
            self.assertEqual(exc1, exc2)

        self.assert_exc_args_equal(exc1, exc2)
        # XXX For now we do not preserve tracebacks.
        if exc1.__traceback__ is not None:
            self.assertEqual(exc1.__traceback__, exc2.__traceback__)
        self.assertEqual(
            getattr(exc1, '__notes__', None),
            getattr(exc2, '__notes__', None),
        )
        # We assume there are no cycles.
        if exc1.__cause__ is None:
            self.assertIs(exc1.__cause__, exc2.__cause__)
        else:
            self.assert_exc_equal(exc1.__cause__, exc2.__cause__)
        if exc1.__context__ is None:
            self.assertIs(exc1.__context__, exc2.__context__)
        else:
            self.assert_exc_equal(exc1.__context__, exc2.__context__)

    def assert_equal_or_equalish(self, obj, expected):
        cls = type(expected)
        if cls.__eq__ is not object.__eq__:
            self.assertEqual(obj, expected)
        elif cls is types.FunctionType:
            self.assert_functions_equal(obj, expected)
        elif isinstance(expected, BaseException):
            self.assert_exc_equal(obj, expected)
        elif cls is types.MethodType:
            raise NotImplementedError(cls)
        elif cls is types.BuiltinMethodType:
            raise NotImplementedError(cls)
        elif cls is types.MethodWrapperType:
            raise NotImplementedError(cls)
        elif cls.__bases__ == (object,):
            self.assertEqual(obj.__dict__, expected.__dict__)
        else:
            raise NotImplementedError(cls)

    def get_xidata(self, obj, *, mode=None):
        mode = self._resolve_mode(mode)
        return _testinternalcapi.get_crossinterp_data(obj, mode)

    def get_roundtrip(self, obj, *, mode=None):
        mode = self._resolve_mode(mode)
        return self._get_roundtrip(obj, mode)

    def _get_roundtrip(self, obj, mode):
        xid = _testinternalcapi.get_crossinterp_data(obj, mode)
        return _testinternalcapi.restore_crossinterp_data(xid)

    def assert_roundtrip_identical(self, values, *, mode=None):
        mode = self._resolve_mode(mode)
        for obj in values:
            with self.subTest(repr(obj)):
                got = self._get_roundtrip(obj, mode)
                self.assertIs(got, obj)

    def assert_roundtrip_equal(self, values, *, mode=None, expecttype=None):
        mode = self._resolve_mode(mode)
        for obj in values:
            with self.subTest(repr(obj)):
                got = self._get_roundtrip(obj, mode)
                if got is obj:
                    continue
                self.assertIs(type(got),
                              type(obj) if expecttype is None else expecttype)
                self.assert_equal_or_equalish(got, obj)

    def assert_roundtrip_equal_not_identical(self, values, *,
                                             mode=None, expecttype=None):
        mode = self._resolve_mode(mode)
        for obj in values:
            with self.subTest(repr(obj)):
                got = self._get_roundtrip(obj, mode)
                self.assertIsNot(got, obj)
                self.assertIs(type(got),
                              type(obj) if expecttype is None else expecttype)
                self.assert_equal_or_equalish(got, obj)

    def assert_roundtrip_not_equal(self, values, *,
                                   mode=None, expecttype=None):
        mode = self._resolve_mode(mode)
        for obj in values:
            with self.subTest(repr(obj)):
                got = self._get_roundtrip(obj, mode)
                self.assertIsNot(got, obj)
                self.assertIs(type(got),
                              type(obj) if expecttype is None else expecttype)
                self.assertNotEqual(got, obj)

    def assert_not_shareable(self, values, exctype=None, *, mode=None):
        mode = self._resolve_mode(mode)
        for obj in values:
            with self.subTest(repr(obj)):
                with self.assertRaises(NotShareableError) as cm:
                    _testinternalcapi.get_crossinterp_data(obj, mode)
                if exctype is not None:
                    self.assertIsInstance(cm.exception.__cause__, exctype)

    def _resolve_mode(self, mode):
        if mode is None:
            mode = self.MODE
        assert mode
        return mode


class PickleTests(_GetXIDataTests):

    MODE = 'pickle'

    def test_shareable(self):
        with ignore_byteswarning():
            for obj in SHAREABLE:
                if obj in PICKLEABLE:
                    self.assert_roundtrip_equal([obj])
                else:
                    self.assert_not_shareable([obj])

    def test_not_shareable(self):
        with ignore_byteswarning():
            for obj in NOT_SHAREABLE:
                if type(obj) is types.MappingProxyType:
                    self.assert_not_shareable([obj])
                elif obj in PICKLEABLE:
                    with self.subTest(repr(obj)):
                        # We don't worry about checking the actual value.
                        # The other tests should cover that well enough.
                        got = self.get_roundtrip(obj)
                        self.assertIs(type(got), type(obj))
                else:
                    self.assert_not_shareable([obj])

    def test_list(self):
        self.assert_roundtrip_equal_not_identical([
            [],
            [1, 2, 3],
            [[1], (2,), {3: 4}],
        ])

    def test_dict(self):
        self.assert_roundtrip_equal_not_identical([
            {},
            {1: 7, 2: 8, 3: 9},
            {1: [1], 2: (2,), 3: {3: 4}},
        ])

    def test_set(self):
        self.assert_roundtrip_equal_not_identical([
            set(),
            {1, 2, 3},
            {frozenset({1}), (2,)},
        ])

    # classes

    def assert_class_defs_same(self, defs):
        # Unpickle relative to the unchanged original module.
        self.assert_roundtrip_identical(defs.TOP_CLASSES)

        instances = []
        for cls, args in defs.TOP_CLASSES.items():
            if cls in defs.CLASSES_WITHOUT_EQUALITY:
                continue
            instances.append(cls(*args))
        self.assert_roundtrip_equal_not_identical(instances)

        # these don't compare equal
        instances = []
        for cls, args in defs.TOP_CLASSES.items():
            if cls not in defs.CLASSES_WITHOUT_EQUALITY:
                continue
            instances.append(cls(*args))
        self.assert_roundtrip_equal(instances)

    def assert_class_defs_other_pickle(self, defs, mod):
        # Pickle relative to a different module than the original.
        for cls in defs.TOP_CLASSES:
            assert not hasattr(mod, cls.__name__), (cls, getattr(mod, cls.__name__))
        self.assert_not_shareable(defs.TOP_CLASSES)

        instances = []
        for cls, args in defs.TOP_CLASSES.items():
            instances.append(cls(*args))
        self.assert_not_shareable(instances)

    def assert_class_defs_other_unpickle(self, defs, mod, *, fail=False):
        # Unpickle relative to a different module than the original.
        for cls in defs.TOP_CLASSES:
            assert not hasattr(mod, cls.__name__), (cls, getattr(mod, cls.__name__))

        instances = []
        for cls, args in defs.TOP_CLASSES.items():
            with self.subTest(repr(cls)):
                setattr(mod, cls.__name__, cls)
                xid = self.get_xidata(cls)
                inst = cls(*args)
                instxid = self.get_xidata(inst)
                instances.append(
                        (cls, xid, inst, instxid))

        for cls, xid, inst, instxid in instances:
            with self.subTest(repr(cls)):
                delattr(mod, cls.__name__)
                if fail:
                    with self.assertRaises(NotShareableError):
                        _testinternalcapi.restore_crossinterp_data(xid)
                    continue
                got = _testinternalcapi.restore_crossinterp_data(xid)
                self.assertIsNot(got, cls)
                self.assertNotEqual(got, cls)

                gotcls = got
                got = _testinternalcapi.restore_crossinterp_data(instxid)
                self.assertIsNot(got, inst)
                self.assertIs(type(got), gotcls)
                if cls in defs.CLASSES_WITHOUT_EQUALITY:
                    self.assertNotEqual(got, inst)
                elif cls in defs.BUILTIN_SUBCLASSES:
                    self.assertEqual(got, inst)
                else:
                    self.assertNotEqual(got, inst)

    def assert_class_defs_not_shareable(self, defs):
        self.assert_not_shareable(defs.TOP_CLASSES)

        instances = []
        for cls, args in defs.TOP_CLASSES.items():
            instances.append(cls(*args))
        self.assert_not_shareable(instances)

    def test_user_class_normal(self):
        self.assert_class_defs_same(defs)

    def test_user_class_in___main__(self):
        with using___main__() as mod:
            defs = load_defs(mod)
            self.assert_class_defs_same(defs)

    def test_user_class_not_in___main___with_filename(self):
        with using___main__() as mod:
            defs = load_defs('__main__')
            assert defs.__file__
            mod.__file__ = defs.__file__
            self.assert_class_defs_not_shareable(defs)

    def test_user_class_not_in___main___without_filename(self):
        with using___main__() as mod:
            defs = load_defs('__main__')
            defs.__file__ = None
            mod.__file__ = None
            self.assert_class_defs_not_shareable(defs)

    def test_user_class_not_in___main___unpickle_with_filename(self):
        with using___main__() as mod:
            defs = load_defs('__main__')
            assert defs.__file__
            mod.__file__ = defs.__file__
            self.assert_class_defs_other_unpickle(defs, mod)

    def test_user_class_not_in___main___unpickle_without_filename(self):
        with using___main__() as mod:
            defs = load_defs('__main__')
            defs.__file__ = None
            mod.__file__ = None
            self.assert_class_defs_other_unpickle(defs, mod, fail=True)

    def test_user_class_in_module(self):
        with temp_module('__spam__') as mod:
            defs = load_defs(mod)
            self.assert_class_defs_same(defs)

    def test_user_class_not_in_module_with_filename(self):
        with temp_module('__spam__') as mod:
            defs = load_defs(mod.__name__)
            assert defs.__file__
            # For now, we only address this case for __main__.
            self.assert_class_defs_not_shareable(defs)

    def test_user_class_not_in_module_without_filename(self):
        with temp_module('__spam__') as mod:
            defs = load_defs(mod.__name__)
            defs.__file__ = None
            self.assert_class_defs_not_shareable(defs)

    def test_user_class_module_missing_then_imported(self):
        with missing_defs_module('__spam__', prep=True) as modname:
            defs = load_defs(modname)
            # For now, we only address this case for __main__.
            self.assert_class_defs_not_shareable(defs)

    def test_user_class_module_missing_not_available(self):
        with missing_defs_module('__spam__') as modname:
            defs = load_defs(modname)
            self.assert_class_defs_not_shareable(defs)

    def test_nested_class(self):
        eggs = defs.EggsNested()
        with self.assertRaises(NotShareableError):
            self.get_roundtrip(eggs)

    # functions

    def assert_func_defs_same(self, defs):
        # Unpickle relative to the unchanged original module.
        self.assert_roundtrip_identical(defs.TOP_FUNCTIONS)

    def assert_func_defs_other_pickle(self, defs, mod):
        # Pickle relative to a different module than the original.
        for func in defs.TOP_FUNCTIONS:
            assert not hasattr(mod, func.__name__), (getattr(mod, func.__name__),)
        self.assert_not_shareable(defs.TOP_FUNCTIONS)

    def assert_func_defs_other_unpickle(self, defs, mod, *, fail=False):
        # Unpickle relative to a different module than the original.
        for func in defs.TOP_FUNCTIONS:
            assert not hasattr(mod, func.__name__), (getattr(mod, func.__name__),)

        captured = []
        for func in defs.TOP_FUNCTIONS:
            with self.subTest(func):
                setattr(mod, func.__name__, func)
                xid = self.get_xidata(func)
                captured.append(
                        (func, xid))

        for func, xid in captured:
            with self.subTest(func):
                delattr(mod, func.__name__)
                if fail:
                    with self.assertRaises(NotShareableError):
                        _testinternalcapi.restore_crossinterp_data(xid)
                    continue
                got = _testinternalcapi.restore_crossinterp_data(xid)
                self.assertIsNot(got, func)
                self.assertNotEqual(got, func)

    def assert_func_defs_not_shareable(self, defs):
        self.assert_not_shareable(defs.TOP_FUNCTIONS)

    def test_user_function_normal(self):
        self.assert_roundtrip_equal(defs.TOP_FUNCTIONS)
        self.assert_func_defs_same(defs)

    def test_user_func_in___main__(self):
        with using___main__() as mod:
            defs = load_defs(mod)
            self.assert_func_defs_same(defs)

    def test_user_func_not_in___main___with_filename(self):
        with using___main__() as mod:
            defs = load_defs('__main__')
            assert defs.__file__
            mod.__file__ = defs.__file__
            self.assert_func_defs_not_shareable(defs)

    def test_user_func_not_in___main___without_filename(self):
        with using___main__() as mod:
            defs = load_defs('__main__')
            defs.__file__ = None
            mod.__file__ = None
            self.assert_func_defs_not_shareable(defs)

    def test_user_func_not_in___main___unpickle_with_filename(self):
        with using___main__() as mod:
            defs = load_defs('__main__')
            assert defs.__file__
            mod.__file__ = defs.__file__
            self.assert_func_defs_other_unpickle(defs, mod)

    def test_user_func_not_in___main___unpickle_without_filename(self):
        with using___main__() as mod:
            defs = load_defs('__main__')
            defs.__file__ = None
            mod.__file__ = None
            self.assert_func_defs_other_unpickle(defs, mod, fail=True)

    def test_user_func_in_module(self):
        with temp_module('__spam__') as mod:
            defs = load_defs(mod)
            self.assert_func_defs_same(defs)

    def test_user_func_not_in_module_with_filename(self):
        with temp_module('__spam__') as mod:
            defs = load_defs(mod.__name__)
            assert defs.__file__
            # For now, we only address this case for __main__.
            self.assert_func_defs_not_shareable(defs)

    def test_user_func_not_in_module_without_filename(self):
        with temp_module('__spam__') as mod:
            defs = load_defs(mod.__name__)
            defs.__file__ = None
            self.assert_func_defs_not_shareable(defs)

    def test_user_func_module_missing_then_imported(self):
        with missing_defs_module('__spam__', prep=True) as modname:
            defs = load_defs(modname)
            # For now, we only address this case for __main__.
            self.assert_func_defs_not_shareable(defs)

    def test_user_func_module_missing_not_available(self):
        with missing_defs_module('__spam__') as modname:
            defs = load_defs(modname)
            self.assert_func_defs_not_shareable(defs)

    def test_nested_function(self):
        self.assert_not_shareable(defs.NESTED_FUNCTIONS)

    # exceptions

    def test_user_exception_normal(self):
        self.assert_roundtrip_equal([
            defs.MimimalError('error!'),
        ])
        self.assert_roundtrip_equal_not_identical([
            defs.RichError('error!', 42),
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
            UnicodeError: (None, msg, None, None, None),
            UnicodeEncodeError: ('utf-8', '', 1, 3, msg),
            UnicodeDecodeError: ('utf-8', b'', 1, 3, msg),
            UnicodeTranslateError: ('', 1, 3, msg),
        }
        exceptions = []
        for cls in EXCEPTION_TYPES:
            args = special.get(cls) or (msg,)
            exceptions.append(cls(*args))

        self.assert_roundtrip_equal(exceptions)


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
            OBJECT,
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
        types = BUILTIN_TYPES
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


class CodeTests(_GetXIDataTests):

    MODE = 'code'

    def test_function_code(self):
        self.assert_roundtrip_equal_not_identical([
            *(f.__code__ for f in defs.FUNCTIONS),
            *(f.__code__ for f in defs.FUNCTION_LIKE),
        ])

    def test_functions(self):
        self.assert_not_shareable([
            *defs.FUNCTIONS,
            *defs.FUNCTION_LIKE,
        ])

    def test_other_objects(self):
        self.assert_not_shareable([
            None,
            True,
            False,
            Ellipsis,
            NotImplemented,
            9999,
            'spam',
            b'spam',
            (),
            [],
            {},
            object(),
        ])


class ShareableFuncTests(_GetXIDataTests):

    MODE = 'func'

    def test_stateless(self):
        self.assert_roundtrip_equal([
            *defs.STATELESS_FUNCTIONS,
            # Generators can be stateless too.
            *defs.FUNCTION_LIKE,
        ])

    def test_not_stateless(self):
        self.assert_not_shareable([
            *(f for f in defs.FUNCTIONS
              if f not in defs.STATELESS_FUNCTIONS),
        ])

    def test_other_objects(self):
        self.assert_not_shareable([
            None,
            True,
            False,
            Ellipsis,
            NotImplemented,
            9999,
            'spam',
            b'spam',
            (),
            [],
            {},
            object(),
        ])


class PureShareableScriptTests(_GetXIDataTests):

    MODE = 'script-pure'

    VALID_SCRIPTS = [
        '',
        'spam',
        '# a comment',
        'print("spam")',
        'raise Exception("spam")',
        """if True:
            do_something()
            """,
        """if True:
            def spam(x):
                return x
            class Spam:
                def eggs(self):
                    return 42
            x = Spam().eggs()
            raise ValueError(spam(x))
            """,
    ]
    INVALID_SCRIPTS = [
        '    pass',  # IndentationError
        '----',  # SyntaxError
        """if True:
            def spam():
                # no body
            spam()
            """,  # IndentationError
    ]

    def test_valid_str(self):
        self.assert_roundtrip_not_equal([
            *self.VALID_SCRIPTS,
        ], expecttype=types.CodeType)

    def test_invalid_str(self):
        self.assert_not_shareable([
            *self.INVALID_SCRIPTS,
        ])

    def test_valid_bytes(self):
        self.assert_roundtrip_not_equal([
            *(s.encode('utf8') for s in self.VALID_SCRIPTS),
        ], expecttype=types.CodeType)

    def test_invalid_bytes(self):
        self.assert_not_shareable([
            *(s.encode('utf8') for s in self.INVALID_SCRIPTS),
        ])

    def test_pure_script_code(self):
        self.assert_roundtrip_equal_not_identical([
            *(f.__code__ for f in defs.PURE_SCRIPT_FUNCTIONS),
        ])

    def test_impure_script_code(self):
        self.assert_not_shareable([
            *(f.__code__ for f in defs.SCRIPT_FUNCTIONS
              if f not in defs.PURE_SCRIPT_FUNCTIONS),
        ])

    def test_other_code(self):
        self.assert_not_shareable([
            *(f.__code__ for f in defs.FUNCTIONS
              if f not in defs.SCRIPT_FUNCTIONS),
            *(f.__code__ for f in defs.FUNCTION_LIKE),
        ])

    def test_pure_script_function(self):
        self.assert_roundtrip_not_equal([
            *defs.PURE_SCRIPT_FUNCTIONS,
        ], expecttype=types.CodeType)

    def test_impure_script_function(self):
        self.assert_not_shareable([
            *(f for f in defs.SCRIPT_FUNCTIONS
              if f not in defs.PURE_SCRIPT_FUNCTIONS),
        ])

    def test_other_function(self):
        self.assert_not_shareable([
            *(f for f in defs.FUNCTIONS
              if f not in defs.SCRIPT_FUNCTIONS),
            *defs.FUNCTION_LIKE,
        ])

    def test_other_objects(self):
        self.assert_not_shareable([
            None,
            True,
            False,
            Ellipsis,
            NotImplemented,
            (),
            [],
            {},
            object(),
        ])


class ShareableScriptTests(PureShareableScriptTests):

    MODE = 'script'

    def test_impure_script_code(self):
        self.assert_roundtrip_equal_not_identical([
            *(f.__code__ for f in defs.SCRIPT_FUNCTIONS
              if f not in defs.PURE_SCRIPT_FUNCTIONS),
        ])

    def test_impure_script_function(self):
        self.assert_roundtrip_not_equal([
            *(f for f in defs.SCRIPT_FUNCTIONS
              if f not in defs.PURE_SCRIPT_FUNCTIONS),
        ], expecttype=types.CodeType)


class ShareableFallbackTests(_GetXIDataTests):

    MODE = 'fallback'

    def test_shareable(self):
        self.assert_roundtrip_equal(SHAREABLE)

    def test_not_shareable(self):
        okay = [
            *PICKLEABLE,
            *defs.STATELESS_FUNCTIONS,
            LAMBDA,
        ]
        ignored = [
            *TUPLES_WITHOUT_EQUALITY,
            OBJECT,
            METHOD,
            BUILTIN_METHOD,
            METHOD_WRAPPER,
        ]
        with ignore_byteswarning():
            self.assert_roundtrip_equal([
                *(o for o in NOT_SHAREABLE
                  if o in okay and o not in ignored
                  and o is not MAPPING_PROXY_EMPTY),
            ])
            self.assert_roundtrip_not_equal([
                *(o for o in NOT_SHAREABLE
                  if o in ignored and o is not MAPPING_PROXY_EMPTY),
            ])
            self.assert_not_shareable([
                *(o for o in NOT_SHAREABLE if o not in okay),
                MAPPING_PROXY_EMPTY,
            ])


class ShareableTypeTests(_GetXIDataTests):

    MODE = 'xidata'

    def test_shareable(self):
        self.assert_roundtrip_equal(SHAREABLE)

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
            EXCEPTION,
            OBJECT,
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

    def test_not_shareable(self):
        self.assert_not_shareable(NOT_SHAREABLE)

    def test_object(self):
        self.assert_not_shareable([
            object(),
        ])

    def test_code(self):
        # types.CodeType
        self.assert_not_shareable([
            *(f.__code__ for f in defs.FUNCTIONS),
            *(f.__code__ for f in defs.FUNCTION_LIKE),
        ])

    def test_function_object(self):
        for func in defs.FUNCTIONS:
            assert type(func) is types.FunctionType, func
        assert type(defs.SpamOkay.okay) is types.FunctionType, func
        assert type(LAMBDA) is types.LambdaType

        self.assert_not_shareable([
            *defs.FUNCTIONS,
            defs.SpamOkay.okay,
            LAMBDA,
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
        self.assert_not_shareable(defs.CLASSES)

        instances = []
        for cls, args in defs.CLASSES.items():
            instances.append(cls(*args))
        self.assert_not_shareable(instances)

    def test_builtin_type(self):
        self.assert_not_shareable(BUILTIN_TYPES)

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
            MAPPING_PROXY_EMPTY,
            types.SimpleNamespace(),
            # types.CellType
            types.CellType(),
            # types.FrameType
            ns['FRAME'],
            # types.TracebackType
            ns['TRACEBACK'],
        ])


if __name__ == '__main__':
    unittest.main()
