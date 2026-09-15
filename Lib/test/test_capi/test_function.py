import unittest
from test.support import import_helper


_testcapi = import_helper.import_module('_testcapi')


class FunctionTest(unittest.TestCase):
    def test_function_get_code(self):
        # Test PyFunction_GetCode()
        import types

        def some():
            pass

        code = _testcapi.function_get_code(some)
        self.assertIsInstance(code, types.CodeType)
        self.assertEqual(code, some.__code__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_code(None)  # not a function

    def test_function_get_globals(self):
        # Test PyFunction_GetGlobals()
        def some():
            pass

        globals_ = _testcapi.function_get_globals(some)
        self.assertIsInstance(globals_, dict)
        self.assertEqual(globals_, some.__globals__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_globals(None)  # not a function

    def test_function_get_module(self):
        # Test PyFunction_GetModule()
        def some():
            pass

        module = _testcapi.function_get_module(some)
        self.assertIsInstance(module, str)
        self.assertEqual(module, some.__module__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_module(None)  # not a function

    def test_function_get_defaults(self):
        # Test PyFunction_GetDefaults()
        def some(
            pos_only1, pos_only2='p',
            /,
            zero=0, optional=None,
            *,
            kw1,
            kw2=True,
        ):
            pass

        defaults = _testcapi.function_get_defaults(some)
        self.assertEqual(defaults, ('p', 0, None))
        self.assertEqual(defaults, some.__defaults__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_defaults(None)  # not a function

    def test_function_set_defaults(self):
        # Test PyFunction_SetDefaults()
        def some(
            pos_only1, pos_only2='p',
            /,
            zero=0, optional=None,
            *,
            kw1,
            kw2=True,
        ):
            pass

        old_defaults = ('p', 0, None)
        self.assertEqual(_testcapi.function_get_defaults(some), old_defaults)
        self.assertEqual(some.__defaults__, old_defaults)

        with self.assertRaises(SystemError):
            _testcapi.function_set_defaults(some, 1)  # not tuple or None
        self.assertEqual(_testcapi.function_get_defaults(some), old_defaults)
        self.assertEqual(some.__defaults__, old_defaults)

        with self.assertRaises(SystemError):
            _testcapi.function_set_defaults(1, ())    # not a function
        self.assertEqual(_testcapi.function_get_defaults(some), old_defaults)
        self.assertEqual(some.__defaults__, old_defaults)

        new_defaults = ('q', 1, None)
        _testcapi.function_set_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_defaults(some), new_defaults)
        self.assertEqual(some.__defaults__, new_defaults)

        # Empty tuple is fine:
        new_defaults = ()
        _testcapi.function_set_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_defaults(some), new_defaults)
        self.assertEqual(some.__defaults__, new_defaults)

        class tuplesub(tuple): ...  # tuple subclasses must work

        new_defaults = tuplesub(((1, 2), ['a', 'b'], None))
        _testcapi.function_set_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_defaults(some), new_defaults)
        self.assertEqual(some.__defaults__, new_defaults)

        # `None` is special, it sets `defaults` to `NULL`,
        # it needs special handling in `_testcapi`:
        _testcapi.function_set_defaults(some, None)
        self.assertEqual(_testcapi.function_get_defaults(some), None)
        self.assertEqual(some.__defaults__, None)

    def test_function_get_kw_defaults(self):
        # Test PyFunction_GetKwDefaults()
        def some(
            pos_only1, pos_only2='p',
            /,
            zero=0, optional=None,
            *,
            kw1,
            kw2=True,
        ):
            pass

        defaults = _testcapi.function_get_kw_defaults(some)
        self.assertEqual(defaults, {'kw2': True})
        self.assertEqual(defaults, some.__kwdefaults__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_kw_defaults(None)  # not a function

    def test_function_set_kw_defaults(self):
        # Test PyFunction_SetKwDefaults()
        def some(
            pos_only1, pos_only2='p',
            /,
            zero=0, optional=None,
            *,
            kw1,
            kw2=True,
        ):
            pass

        old_defaults = {'kw2': True}
        self.assertEqual(_testcapi.function_get_kw_defaults(some), old_defaults)
        self.assertEqual(some.__kwdefaults__, old_defaults)

        with self.assertRaises(SystemError):
            _testcapi.function_set_kw_defaults(some, 1)  # not dict or None
        self.assertEqual(_testcapi.function_get_kw_defaults(some), old_defaults)
        self.assertEqual(some.__kwdefaults__, old_defaults)

        with self.assertRaises(SystemError):
            _testcapi.function_set_kw_defaults(1, {})    # not a function
        self.assertEqual(_testcapi.function_get_kw_defaults(some), old_defaults)
        self.assertEqual(some.__kwdefaults__, old_defaults)

        new_defaults = {'kw2': (1, 2, 3)}
        _testcapi.function_set_kw_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_kw_defaults(some), new_defaults)
        self.assertEqual(some.__kwdefaults__, new_defaults)

        # Empty dict is fine:
        new_defaults = {}
        _testcapi.function_set_kw_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_kw_defaults(some), new_defaults)
        self.assertEqual(some.__kwdefaults__, new_defaults)

        class dictsub(dict): ...  # dict subclasses must work

        new_defaults = dictsub({'kw2': None})
        _testcapi.function_set_kw_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_kw_defaults(some), new_defaults)
        self.assertEqual(some.__kwdefaults__, new_defaults)

        # `None` is special, it sets `kwdefaults` to `NULL`,
        # it needs special handling in `_testcapi`:
        _testcapi.function_set_kw_defaults(some, None)
        self.assertEqual(_testcapi.function_get_kw_defaults(some), None)
        self.assertEqual(some.__kwdefaults__, None)

    def test_function_get_closure(self):
        # Test PyFunction_GetClosure()
        from types import CellType

        def regular_function(): ...
        def unused_one_level(arg1):
            def inner(arg2, arg3): ...
            return inner
        def unused_two_levels(arg1, arg2):
            def decorator(arg3, arg4):
                def inner(arg5, arg6): ...
                return inner
            return decorator
        def with_one_level(arg1):
            def inner(arg2, arg3):
                return arg1 + arg2 + arg3
            return inner
        def with_two_levels(arg1, arg2):
            def decorator(arg3, arg4):
                def inner(arg5, arg6):
                    return arg1 + arg2 + arg3 + arg4 + arg5 + arg6
                return inner
            return decorator

        # Functions without closures:
        self.assertIsNone(_testcapi.function_get_closure(regular_function))
        self.assertIsNone(regular_function.__closure__)

        func = unused_one_level(1)
        closure = _testcapi.function_get_closure(func)
        self.assertIsNone(closure)
        self.assertIsNone(func.__closure__)

        func = unused_two_levels(1, 2)(3, 4)
        closure = _testcapi.function_get_closure(func)
        self.assertIsNone(closure)
        self.assertIsNone(func.__closure__)

        # Functions with closures:
        func = with_one_level(5)
        closure = _testcapi.function_get_closure(func)
        self.assertEqual(closure, func.__closure__)
        self.assertIsInstance(closure, tuple)
        self.assertEqual(len(closure), 1)
        self.assertEqual(len(closure), len(func.__code__.co_freevars))
        for cell in closure:
            self.assertIsInstance(cell, CellType)
        self.assertTrue(closure[0].cell_contents, 5)

        func = with_two_levels(1, 2)(3, 4)
        closure = _testcapi.function_get_closure(func)
        self.assertEqual(closure, func.__closure__)
        self.assertIsInstance(closure, tuple)
        self.assertEqual(len(closure), 4)
        self.assertEqual(len(closure), len(func.__code__.co_freevars))
        for cell in closure:
            self.assertIsInstance(cell, CellType)
        self.assertEqual([cell.cell_contents for cell in closure],
                         [1, 2, 3, 4])

    def test_function_get_closure_error(self):
        # Test PyFunction_GetClosure()
        with self.assertRaises(SystemError):
            _testcapi.function_get_closure(1)
        with self.assertRaises(SystemError):
            _testcapi.function_get_closure(None)

    def test_function_set_closure(self):
        # Test PyFunction_SetClosure()
        from types import CellType

        def function_without_closure(): ...
        def function_with_closure(arg):
            def inner():
                return arg
            return inner

        func = function_without_closure
        _testcapi.function_set_closure(func, (CellType(1), CellType(1)))
        closure = _testcapi.function_get_closure(func)
        self.assertEqual([c.cell_contents for c in closure], [1, 1])
        self.assertEqual([c.cell_contents for c in func.__closure__], [1, 1])

        func = function_with_closure(1)
        _testcapi.function_set_closure(func,
                                       (CellType(1), CellType(2), CellType(3)))
        closure = _testcapi.function_get_closure(func)
        self.assertEqual([c.cell_contents for c in closure], [1, 2, 3])
        self.assertEqual([c.cell_contents for c in func.__closure__], [1, 2, 3])

    def test_function_set_closure_none(self):
        # Test PyFunction_SetClosure()
        def function_without_closure(): ...
        def function_with_closure(arg):
            def inner():
                return arg
            return inner

        _testcapi.function_set_closure(function_without_closure, None)
        self.assertIsNone(
            _testcapi.function_get_closure(function_without_closure))
        self.assertIsNone(function_without_closure.__closure__)

        _testcapi.function_set_closure(function_with_closure, None)
        self.assertIsNone(
            _testcapi.function_get_closure(function_with_closure))
        self.assertIsNone(function_with_closure.__closure__)

    def test_function_set_closure_errors(self):
        # Test PyFunction_SetClosure()
        def function_without_closure(): ...

        with self.assertRaises(SystemError):
            _testcapi.function_set_closure(None, ())  # not a function

        with self.assertRaises(SystemError):
            _testcapi.function_set_closure(function_without_closure, 1)
        self.assertIsNone(function_without_closure.__closure__)  # no change

        # NOTE: this works, but goes against the docs:
        _testcapi.function_set_closure(function_without_closure, (1, 2))
        self.assertEqual(
            _testcapi.function_get_closure(function_without_closure), (1, 2))
        self.assertEqual(function_without_closure.__closure__, (1, 2))

    def test_function_get_annotations(self):
        # Test PyFunction_GetAnnotations()
        def normal():
            pass

        def annofn(arg: int) -> str:
            return f'arg = {arg}'

        annotations = _testcapi.function_get_annotations(normal)
        self.assertIsNone(annotations)

        annotations = _testcapi.function_get_annotations(annofn)
        self.assertIsInstance(annotations, dict)
        self.assertEqual(annotations, annofn.__annotations__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_annotations(None)

    # TODO: test PyFunction_New()
    # TODO: test PyFunction_NewWithQualName()
    # TODO: test PyFunction_SetVectorcall()
    # TODO: test PyFunction_SetAnnotations()
    # TODO: test PyClassMethod_New()
    # TODO: test PyStaticMethod_New()
    #
    # PyFunction_AddWatcher() and PyFunction_ClearWatcher() are tested by
    # test_capi.test_watchers.


if __name__ == "__main__":
    unittest.main()
