import asyncio
import textwrap
import unittest

from typing import Any, Sequence, TypeVar, TypeVarTuple, ParamSpec


def run_code(code: str) -> dict[str, Any]:
    ns = {}
    exec(textwrap.dedent(code), ns)
    return ns


class TypeParamsInvalidTest(unittest.TestCase):
    def test_name_collision_01(self):
        with self.assertRaisesRegex(SyntaxError, "duplicate type parameter 'A'"):
            run_code("""def func[**A, A](): ...""")

    def test_name_non_collision_02(self):
        run_code("""def func[A](A): ...""")

    def test_name_non_collision_03(self):
        run_code("""def func[A](*A): ...""")

    def test_name_non_collision_04(self):
        # Mangled names should not cause a conflict.
        run_code("""\
            class ClassA:
                def func[__A](self, __A): ...
            """
        )

    def test_name_non_collision_05(self):
        run_code("""\
            class ClassA:
                def func[_ClassA__A](self, __A): ...
            """
        )

    def test_name_non_collision_06(self):
        run_code("""\
            class ClassA[X]:
                def func(self, X): ...
            """
        )

    def test_name_non_collision_07(self):
        run_code("""\
            class ClassA[X]:
                def func(self):
                    X = 1
            """
        )

    def test_name_non_collision_08(self):
        run_code("""\
            class ClassA[X]:
                def func(self):
                    a = [X for X in []]
            """
        )

    def test_name_non_collision_9(self):
        run_code("""\
            class ClassA[X]:
                def func[X](self):
                    ...
            """
        )

    def test_name_non_collision_10(self):
        run_code("""\
            class ClassA[X]:
                X: int
            """
        )

    def test_name_non_collision_11(self):
        run_code("""\
            def outer():
                X = 1
                def inner[X]():
                    nonlocal X
            """
        )

    def test_name_non_collision_13(self):
        run_code("""\
            X = 1
            def outer():
                def inner[X]():
                    global X
            """
        )

    def test_disallowed_expressions(self):
        with self.assertRaises(SyntaxError):
            run_code("type X = (yield)")
        with self.assertRaises(SyntaxError):
            run_code("type X = (yield from x)")
        with self.assertRaises(SyntaxError):
            run_code("type X = (await 42)")
        with self.assertRaises(SyntaxError):
            run_code("async def f(): type X = (yield)")
        with self.assertRaises(SyntaxError):
            run_code("type X = (y := 3)")
        with self.assertRaises(SyntaxError):
            run_code("class X[T: (yield)]: pass")
        with self.assertRaises(SyntaxError):
            run_code("class X[T: (yield from x)]: pass")
        with self.assertRaises(SyntaxError):
            run_code("class X[T: (await 42)]: pass")
        with self.assertRaises(SyntaxError):
            run_code("class X[T: (y := 3)]: pass")
        with self.assertRaises(SyntaxError):
            run_code("class X[T](y := Sequence[T]): pass")
        with self.assertRaises(SyntaxError):
            run_code("def f[T](y: (x := Sequence[T])): pass")


class TypeParamsAccessTest(unittest.TestCase):
    def test_class_access_01(self):
        run_code("""\
            class ClassA[A, B](dict[A, B]):
                ...
            """
        )

    def test_class_access_02(self):
        run_code("""\
            class MyMeta[A, B](type): ...
            class ClassA[A, B](metaclass=MyMeta[A, B]):
                ...
            """
        )

    def test_class_access_03(self):
        code = """\
            def my_decorator(a):
                ...
            @my_decorator(A)
            class ClassA[A, B]():
                ...
            """

        with self.assertRaisesRegex(NameError, "name 'A' is not defined"):
            run_code(code)

    def test_function_access_01(self):
        run_code("""\
            def func[A, B](a: dict[A, B]):
                ...
            """
        )

    def test_function_access_02(self):
        code = """\
            def func[A](a = list[A]()):
                ...
            """

        with self.assertRaisesRegex(NameError, "name 'A' is not defined"):
            run_code(code)

    def test_function_access_03(self):
        code = """\
            def my_decorator(a):
                ...
            @my_decorator(A)
            def func[A]():
                ...
            """

        with self.assertRaisesRegex(NameError, "name 'A' is not defined"):
            run_code(code)

    def test_method_access_01(self):
        run_code("""\
            class ClassA:
                x = int
                def func[T](self, a: x, b: T):
                    ...

            assert ClassA.func.__annotations__["a"] is int
            """
        )

    def test_nested_access_01(self):
        run_code("""\
            class ClassA[A]:
                def funcB[B](self):
                    class ClassC[C]:
                        def funcD[D](self):
                            lambda : (A, B, C, D)
            """
        )

    def test_out_of_scope_01(self):
        code = """\
            class ClassA[T]: ...
            x = T
            """

        with self.assertRaisesRegex(NameError, "name 'T' is not defined"):
            run_code(code)

    def test_out_of_scope_02(self):
        code = """\
            class ClassA[A]:
                def funcB[B](self): ...

                x = B
            """

        with self.assertRaisesRegex(NameError, "name 'B' is not defined"):
            run_code(code)

    def test_class_scope_interaction_01(self):
        run_code("""\
            class C:
                x = 1
                def method[T](self, arg: x): pass

            assert C.method.__annotations__["arg"] == 1
        """)

    def test_class_scope_interaction_02(self):
        run_code("""\
            from typing import Generic
            class C:
                class Base: pass
                class Child[T](Base): pass

            assert C.Child.__bases__ == (C.Base, Generic)
        """)

    def test_class_deref(self):
        run_code("""\
            class C[T]:
                T = "class"
                type Alias = T

            assert C.Alias.__value__ == "class", repr(C.Alias)
        """)

    def test_nonlocal(self):
        code = """\
            def outer2[T]():

                def inner1():
                    nonlocal T
        """
        with self.assertRaisesRegex(SyntaxError, "nonlocal binding not allowed for type parameter 'T'"):
            run_code(code)

    def test_reference_previous_typevar(self):
        def func[S, T: Sequence[S]]():
            pass

        S, T = func.__type_params__
        self.assertEqual(T.__bound__, Sequence[S])


class TypeParamsLazyEvaluationTest(unittest.TestCase):
    def test_recursive_class(self):
        class Foo[T: Foo, U: (Foo, Foo)]:
            pass

        type_params = Foo.__type_params__
        self.assertEqual(len(type_params), 2)
        self.assertEqual(type_params[0].__name__, "T")
        self.assertEqual(type_params[0].__bound__, Foo)
        self.assertEqual(type_params[0].__constraints__, None)

        self.assertEqual(type_params[1].__name__, "U")
        self.assertEqual(type_params[1].__bound__, None)
        self.assertEqual(type_params[1].__constraints__, (Foo, Foo))

    def test_evaluation_error(self):
        class Foo[T: Undefined, U: (Undefined,)]:
            pass

        type_params = Foo.__type_params__
        with self.assertRaises(NameError):
            type_params[0].__bound__
        self.assertEqual(type_params[0].__constraints__, None)
        self.assertEqual(type_params[1].__bound__, None)
        with self.assertRaises(NameError):
            type_params[1].__constraints__

        Undefined = "defined"
        self.assertEqual(type_params[0].__bound__, "defined")
        self.assertEqual(type_params[0].__constraints__, None)

        self.assertEqual(type_params[1].__bound__, None)
        self.assertEqual(type_params[1].__constraints__, ("defined",))


class TypeParamsClassScopeTest(unittest.TestCase):
    def test_alias(self):
        class X:
            T = int
            type U = T
        self.assertIs(X.U.__value__, int)

        run_code("""\
            glb = "global"
            class X:
                cls = "class"
                type U = (glb, cls)

            assert X.U.__value__ == ("global", "class"), X.U.__value__
        """)

    def test_bound(self):
        class X:
            T = int
            def foo[U: T](self): ...
        self.assertIs(X.foo.__type_params__[0].__bound__, int)

        run_code("""\
            glb = "global"
            class X:
                cls = "class"
                def foo[T: glb, U: cls](self): ...

            T, U = X.foo.__type_params__
            assert T.__bound__ == "global"
            assert U.__bound__ == "class"
        """)


class ManglingTest(unittest.TestCase):
    def test_mangling(self):
        class Foo[__T]:
            param = __T
            def meth[__U](self, arg: __T, arg2: __U):
                return (__T, __U)
            type Alias[__V] = (__T, __V)

        T = Foo.__type_params__[0]
        self.assertEqual(T.__name__, "__T")
        U = Foo.meth.__type_params__[0]
        self.assertEqual(U.__name__, "__U")
        V = Foo.Alias.__type_params__[0]
        self.assertEqual(V.__name__, "__V")

        anno = Foo.meth.__annotations__
        self.assertIs(anno["arg"], T)
        self.assertIs(anno["arg2"], U)
        self.assertEqual(Foo().meth(1, 2), (T, U))

        self.assertEqual(Foo.Alias.__value__, (T, V))


class TypeParamsTraditionalTypeVars(unittest.TestCase):
    def test_traditional_01(self):
        code = """\
            from typing import Generic
            class ClassA[T](Generic[T]): ...
            """

        with self.assertRaisesRegex(TypeError, r"Cannot inherit from Generic\[...\] multiple types."):
            run_code(code)

    def test_traditional_02(self):
        from typing import TypeVar
        S = TypeVar("S")
        with self.assertRaises(TypeError):
            class ClassA[T](dict[T, S]): ...

    def test_traditional_03(self):
        # This does not generate a runtime error, but it should be
        # flagged as an error by type checkers.
        from typing import TypeVar
        S = TypeVar("S")
        def func[T](a: T, b: S) -> T | S:
            return a


class TypeParamsTypeVarTest(unittest.TestCase):
    def test_typevar_01(self):
        def func1[A: str, B: str | int, C: (int, str)]():
            return (A, B, C)

        a, b, c = func1()

        self.assertIsInstance(a, TypeVar)
        self.assertEqual(a.__bound__, str)
        self.assertTrue(a.__infer_variance__)
        self.assertFalse(a.__covariant__)
        self.assertFalse(a.__contravariant__)

        self.assertIsInstance(b, TypeVar)
        self.assertEqual(b.__bound__, str | int)
        self.assertTrue(b.__infer_variance__)
        self.assertFalse(b.__covariant__)
        self.assertFalse(b.__contravariant__)

        self.assertIsInstance(c, TypeVar)
        self.assertEqual(c.__bound__, None)
        self.assertEqual(c.__constraints__, (int, str))
        self.assertTrue(c.__infer_variance__)
        self.assertFalse(c.__covariant__)
        self.assertFalse(c.__contravariant__)

    def test_typevar_generator(self):
        def get_generator[A]():
            def generator1[C]():
                yield C

            def generator2[B]():
                yield A
                yield B
                yield from generator1()
            return generator2

        gen = get_generator()

        a, b, c = [x for x in gen()]

        self.assertIsInstance(a, TypeVar)
        self.assertEqual(a.__name__, "A")
        self.assertIsInstance(b, TypeVar)
        self.assertEqual(b.__name__, "B")
        self.assertIsInstance(c, TypeVar)
        self.assertEqual(c.__name__, "C")

    def test_typevar_coroutine(self):
        def get_coroutine[A]():
            async def coroutine[B]():
                return (A, B)
            return coroutine

        co = get_coroutine()

        self.addCleanup(asyncio.set_event_loop_policy, None)
        a, b = asyncio.run(co())

        self.assertIsInstance(a, TypeVar)
        self.assertEqual(a.__name__, "A")
        self.assertIsInstance(b, TypeVar)
        self.assertEqual(b.__name__, "B")


class TypeParamsTypeVarTupleTest(unittest.TestCase):
    def test_typevartuple_01(self):
        code = """\
            def func1[*A: str]():
                return (A, B, C)
            """

        with self.assertRaisesRegex(SyntaxError, r"expected '\('"):
            run_code(code)

    def test_typevartuple_02(self):
        def func1[*A]():
            return A

        a = func1()
        self.assertIsInstance(a, TypeVarTuple)


class TypeParamsTypeVarParamSpec(unittest.TestCase):
    def test_paramspec_01(self):
        code = """\
            def func1[**A: str]():
                return (A, B, C)
            """

        with self.assertRaisesRegex(SyntaxError, r"expected '\('"):
            run_code(code)

    def test_paramspec_02(self):
        def func1[**A]():
            return A

        a = func1()
        self.assertIsInstance(a, ParamSpec)
        self.assertTrue(a.__infer_variance__)
        self.assertFalse(a.__covariant__)
        self.assertFalse(a.__contravariant__)


class TypeParamsTypeParamsDunder(unittest.TestCase):
    def test_typeparams_dunder_class_01(self):
        class Outer[A, B]:
            class Inner[C, D]:
                @staticmethod
                def get_typeparams():
                    return A, B, C, D

        a, b, c, d = Outer.Inner.get_typeparams()
        self.assertEqual(Outer.__type_params__, (a, b))
        self.assertEqual(Outer.Inner.__type_params__, (c, d))

        self.assertEqual(Outer.__parameters__, (a, b))
        self.assertEqual(Outer.Inner.__parameters__, (c, d))

    def test_typeparams_dunder_class_02(self):
        class ClassA:
            pass

        self.assertEqual(ClassA.__type_params__, ())

    def test_typeparams_dunder_class_03(self):
        code = """\
            class ClassA[A]():
                pass
            ClassA.__type_params__ = ()
            """

        with self.assertRaisesRegex(AttributeError, "attribute '__type_params__' of 'type' objects is not writable"):
            run_code(code)

    def test_typeparams_dunder_function_01(self):
        def outer[A, B]():
            def inner[C, D]():
                return A, B, C, D

            return inner

        inner = outer()
        a, b, c, d = inner()
        self.assertEqual(outer.__type_params__, (a, b))
        self.assertEqual(inner.__type_params__, (c, d))

    def test_typeparams_dunder_function_02(self):
        def func1():
            pass

        self.assertEqual(func1.__type_params__, ())

    def test_typeparams_dunder_function_03(self):
        code = """\
            def func[A]():
                pass
            func.__type_params__ = ()
            """

        with self.assertRaisesRegex(AttributeError, "attribute '__type_params__' of 'function' objects is not writable"):
            run_code(code)
