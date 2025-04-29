import annotationlib
import textwrap
import types
import unittest
import pickle
import weakref
from test.support import check_syntax_error, run_code, run_no_yield_async_fn

from typing import Generic, NoDefault, Sequence, TypeAliasType, TypeVar, TypeVarTuple, ParamSpec, get_args


class TypeParamsInvalidTest(unittest.TestCase):
    def test_name_collisions(self):
        check_syntax_error(self, 'def func[**A, A](): ...', "duplicate type parameter 'A'")
        check_syntax_error(self, 'def func[A, *A](): ...', "duplicate type parameter 'A'")
        check_syntax_error(self, 'def func[*A, **A](): ...', "duplicate type parameter 'A'")

        check_syntax_error(self, 'class C[**A, A](): ...', "duplicate type parameter 'A'")
        check_syntax_error(self, 'class C[A, *A](): ...', "duplicate type parameter 'A'")
        check_syntax_error(self, 'class C[*A, **A](): ...', "duplicate type parameter 'A'")

    def test_name_non_collision_02(self):
        ns = run_code("""def func[A](A): return A""")
        func = ns["func"]
        self.assertEqual(func(1), 1)
        A, = func.__type_params__
        self.assertEqual(A.__name__, "A")

    def test_name_non_collision_03(self):
        ns = run_code("""def func[A](*A): return A""")
        func = ns["func"]
        self.assertEqual(func(1), (1,))
        A, = func.__type_params__
        self.assertEqual(A.__name__, "A")

    def test_name_non_collision_04(self):
        # Mangled names should not cause a conflict.
        ns = run_code("""
            class ClassA:
                def func[__A](self, __A): return __A
            """
        )
        cls = ns["ClassA"]
        self.assertEqual(cls().func(1), 1)
        A, = cls.func.__type_params__
        self.assertEqual(A.__name__, "__A")

    def test_name_non_collision_05(self):
        ns = run_code("""
            class ClassA:
                def func[_ClassA__A](self, __A): return __A
            """
        )
        cls = ns["ClassA"]
        self.assertEqual(cls().func(1), 1)
        A, = cls.func.__type_params__
        self.assertEqual(A.__name__, "_ClassA__A")

    def test_name_non_collision_06(self):
        ns = run_code("""
            class ClassA[X]:
                def func(self, X): return X
            """
        )
        cls = ns["ClassA"]
        self.assertEqual(cls().func(1), 1)
        X, = cls.__type_params__
        self.assertEqual(X.__name__, "X")

    def test_name_non_collision_07(self):
        ns = run_code("""
            class ClassA[X]:
                def func(self):
                    X = 1
                    return X
            """
        )
        cls = ns["ClassA"]
        self.assertEqual(cls().func(), 1)
        X, = cls.__type_params__
        self.assertEqual(X.__name__, "X")

    def test_name_non_collision_08(self):
        ns = run_code("""
            class ClassA[X]:
                def func(self):
                    return [X for X in [1, 2]]
            """
        )
        cls = ns["ClassA"]
        self.assertEqual(cls().func(), [1, 2])
        X, = cls.__type_params__
        self.assertEqual(X.__name__, "X")

    def test_name_non_collision_9(self):
        ns = run_code("""
            class ClassA[X]:
                def func[X](self):
                    ...
            """
        )
        cls = ns["ClassA"]
        outer_X, = cls.__type_params__
        inner_X, = cls.func.__type_params__
        self.assertEqual(outer_X.__name__, "X")
        self.assertEqual(inner_X.__name__, "X")
        self.assertIsNot(outer_X, inner_X)

    def test_name_non_collision_10(self):
        ns = run_code("""
            class ClassA[X]:
                X: int
            """
        )
        cls = ns["ClassA"]
        X, = cls.__type_params__
        self.assertEqual(X.__name__, "X")
        self.assertIs(cls.__annotations__["X"], int)

    def test_name_non_collision_13(self):
        ns = run_code("""
            X = 1
            def outer():
                def inner[X]():
                    global X
                    X = 2
                return inner
            """
        )
        self.assertEqual(ns["X"], 1)
        outer = ns["outer"]
        outer()()
        self.assertEqual(ns["X"], 2)

    def test_disallowed_expressions(self):
        check_syntax_error(self, "type X = (yield)")
        check_syntax_error(self, "type X = (yield from x)")
        check_syntax_error(self, "type X = (await 42)")
        check_syntax_error(self, "async def f(): type X = (yield)")
        check_syntax_error(self, "type X = (y := 3)")
        check_syntax_error(self, "class X[T: (yield)]: pass")
        check_syntax_error(self, "class X[T: (yield from x)]: pass")
        check_syntax_error(self, "class X[T: (await 42)]: pass")
        check_syntax_error(self, "class X[T: (y := 3)]: pass")
        check_syntax_error(self, "class X[T](y := Sequence[T]): pass")
        check_syntax_error(self, "def f[T](y: (x := Sequence[T])): pass")
        check_syntax_error(self, "class X[T]([(x := 3) for _ in range(2)] and B): pass")
        check_syntax_error(self, "def f[T: [(x := 3) for _ in range(2)]](): pass")
        check_syntax_error(self, "type T = [(x := 3) for _ in range(2)]")

    def test_incorrect_mro_explicit_object(self):
        with self.assertRaisesRegex(TypeError, r"\(MRO\) for bases object, Generic"):
            class My[X](object): ...


class TypeParamsNonlocalTest(unittest.TestCase):
    def test_nonlocal_disallowed_01(self):
        code = """
            def outer():
                X = 1
                def inner[X]():
                    nonlocal X
                return X
            """
        check_syntax_error(self, code)

    def test_nonlocal_disallowed_02(self):
        code = """
            def outer2[T]():
                def inner1():
                    nonlocal T
        """
        check_syntax_error(self, textwrap.dedent(code))

    def test_nonlocal_disallowed_03(self):
        code = """
            class Cls[T]:
                nonlocal T
        """
        check_syntax_error(self, textwrap.dedent(code))

    def test_nonlocal_allowed(self):
        code = """
            def func[T]():
                T = "func"
                def inner():
                    nonlocal T
                    T = "inner"
                inner()
                assert T == "inner"
        """
        ns = run_code(code)
        func = ns["func"]
        T, = func.__type_params__
        self.assertEqual(T.__name__, "T")


class TypeParamsAccessTest(unittest.TestCase):
    def test_class_access_01(self):
        ns = run_code("""
            class ClassA[A, B](dict[A, B]):
                ...
            """
        )
        cls = ns["ClassA"]
        A, B = cls.__type_params__
        self.assertEqual(types.get_original_bases(cls), (dict[A, B], Generic[A, B]))

    def test_class_access_02(self):
        ns = run_code("""
            class MyMeta[A, B](type): ...
            class ClassA[A, B](metaclass=MyMeta[A, B]):
                ...
            """
        )
        meta = ns["MyMeta"]
        cls = ns["ClassA"]
        A1, B1 = meta.__type_params__
        A2, B2 = cls.__type_params__
        self.assertIsNot(A1, A2)
        self.assertIsNot(B1, B2)
        self.assertIs(type(cls), meta)

    def test_class_access_03(self):
        code = """
            def my_decorator(a):
                ...
            @my_decorator(A)
            class ClassA[A, B]():
                ...
            """

        with self.assertRaisesRegex(NameError, "name 'A' is not defined"):
            run_code(code)

    def test_function_access_01(self):
        ns = run_code("""
            def func[A, B](a: dict[A, B]):
                ...
            """
        )
        func = ns["func"]
        A, B = func.__type_params__
        self.assertEqual(func.__annotations__["a"], dict[A, B])

    def test_function_access_02(self):
        code = """
            def func[A](a = list[A]()):
                ...
            """

        with self.assertRaisesRegex(NameError, "name 'A' is not defined"):
            run_code(code)

    def test_function_access_03(self):
        code = """
            def my_decorator(a):
                ...
            @my_decorator(A)
            def func[A]():
                ...
            """

        with self.assertRaisesRegex(NameError, "name 'A' is not defined"):
            run_code(code)

    def test_method_access_01(self):
        ns = run_code("""
            class ClassA:
                x = int
                def func[T](self, a: x, b: T):
                    ...
            """
        )
        cls = ns["ClassA"]
        self.assertIs(cls.func.__annotations__["a"], int)
        T, = cls.func.__type_params__
        self.assertIs(cls.func.__annotations__["b"], T)

    def test_nested_access_01(self):
        ns = run_code("""
            class ClassA[A]:
                def funcB[B](self):
                    class ClassC[C]:
                        def funcD[D](self):
                            return lambda: (A, B, C, D)
                    return ClassC
            """
        )
        cls = ns["ClassA"]
        A, = cls.__type_params__
        B, = cls.funcB.__type_params__
        classC = cls().funcB()
        C, = classC.__type_params__
        D, = classC.funcD.__type_params__
        self.assertEqual(classC().funcD()(), (A, B, C, D))

    def test_out_of_scope_01(self):
        code = """
            class ClassA[T]: ...
            x = T
            """

        with self.assertRaisesRegex(NameError, "name 'T' is not defined"):
            run_code(code)

    def test_out_of_scope_02(self):
        code = """
            class ClassA[A]:
                def funcB[B](self): ...

                x = B
            """

        with self.assertRaisesRegex(NameError, "name 'B' is not defined"):
            run_code(code)

    def test_class_scope_interaction_01(self):
        ns = run_code("""
            class C:
                x = 1
                def method[T](self, arg: x): pass
        """)
        cls = ns["C"]
        self.assertEqual(cls.method.__annotations__["arg"], 1)

    def test_class_scope_interaction_02(self):
        ns = run_code("""
            class C:
                class Base: pass
                class Child[T](Base): pass
        """)
        cls = ns["C"]
        self.assertEqual(cls.Child.__bases__, (cls.Base, Generic))
        T, = cls.Child.__type_params__
        self.assertEqual(types.get_original_bases(cls.Child), (cls.Base, Generic[T]))

    def test_class_deref(self):
        ns = run_code("""
            class C[T]:
                T = "class"
                type Alias = T
        """)
        cls = ns["C"]
        self.assertEqual(cls.Alias.__value__, "class")

    def test_shadowing_nonlocal(self):
        ns = run_code("""
            def outer[T]():
                T = "outer"
                def inner():
                    nonlocal T
                    T = "inner"
                    return T
                return lambda: T, inner
        """)
        outer = ns["outer"]
        T, = outer.__type_params__
        self.assertEqual(T.__name__, "T")
        getter, inner = outer()
        self.assertEqual(getter(), "outer")
        self.assertEqual(inner(), "inner")
        self.assertEqual(getter(), "inner")

    def test_reference_previous_typevar(self):
        def func[S, T: Sequence[S]]():
            pass

        S, T = func.__type_params__
        self.assertEqual(T.__bound__, Sequence[S])

    def test_super(self):
        class Base:
            def meth(self):
                return "base"

        class Child(Base):
            # Having int in the annotation ensures the class gets cells for both
            # __class__ and __classdict__
            def meth[T](self, arg: int) -> T:
                return super().meth() + "child"

        c = Child()
        self.assertEqual(c.meth(1), "basechild")

    def test_type_alias_containing_lambda(self):
        type Alias[T] = lambda: T
        T, = Alias.__type_params__
        self.assertIs(Alias.__value__(), T)

    def test_class_base_containing_lambda(self):
        # Test that scopes nested inside hidden functions work correctly
        outer_var = "outer"
        class Base[T]: ...
        class Child[T](Base[lambda: (int, outer_var, T)]): ...
        base, _ = types.get_original_bases(Child)
        func, = get_args(base)
        T, = Child.__type_params__
        self.assertEqual(func(), (int, "outer", T))

    def test_comprehension_01(self):
        type Alias[T: ([T for T in (T, [1])[1]], T)] = [T for T in T.__name__]
        self.assertEqual(Alias.__value__, ["T"])
        T, = Alias.__type_params__
        self.assertEqual(T.__constraints__, ([1], T))

    def test_comprehension_02(self):
        type Alias[T: [lambda: T for T in (T, [1])[1]]] = [lambda: T for T in T.__name__]
        func, = Alias.__value__
        self.assertEqual(func(), "T")
        T, = Alias.__type_params__
        func, = T.__bound__
        self.assertEqual(func(), 1)

    def test_comprehension_03(self):
        def F[T: [lambda: T for T in (T, [1])[1]]](): return [lambda: T for T in T.__name__]
        func, = F()
        self.assertEqual(func(), "T")
        T, = F.__type_params__
        func, = T.__bound__
        self.assertEqual(func(), 1)

    def test_gen_exp_in_nested_class(self):
        code = """
            from test.test_type_params import make_base

            class C[T]:
                T = "class"
                class Inner(make_base(T for _ in (1,)), make_base(T)):
                    pass
        """
        C = run_code(code)["C"]
        T, = C.__type_params__
        base1, base2 = C.Inner.__bases__
        self.assertEqual(list(base1.__arg__), [T])
        self.assertEqual(base2.__arg__, "class")

    def test_gen_exp_in_nested_generic_class(self):
        code = """
            from test.test_type_params import make_base

            class C[T]:
                T = "class"
                class Inner[U](make_base(T for _ in (1,)), make_base(T)):
                    pass
        """
        ns = run_code(code)
        inner = ns["C"].Inner
        base1, base2, _ = inner.__bases__
        self.assertEqual(list(base1.__arg__), [ns["C"].__type_params__[0]])
        self.assertEqual(base2.__arg__, "class")

    def test_listcomp_in_nested_class(self):
        code = """
            from test.test_type_params import make_base

            class C[T]:
                T = "class"
                class Inner(make_base([T for _ in (1,)]), make_base(T)):
                    pass
        """
        C = run_code(code)["C"]
        T, = C.__type_params__
        base1, base2 = C.Inner.__bases__
        self.assertEqual(base1.__arg__, [T])
        self.assertEqual(base2.__arg__, "class")

    def test_listcomp_in_nested_generic_class(self):
        code = """
            from test.test_type_params import make_base

            class C[T]:
                T = "class"
                class Inner[U](make_base([T for _ in (1,)]), make_base(T)):
                    pass
        """
        ns = run_code(code)
        inner = ns["C"].Inner
        base1, base2, _ = inner.__bases__
        self.assertEqual(base1.__arg__, [ns["C"].__type_params__[0]])
        self.assertEqual(base2.__arg__, "class")

    def test_gen_exp_in_generic_method(self):
        code = """
            class C[T]:
                T = "class"
                def meth[U](x: (T for _ in (1,)), y: T):
                    pass
        """
        ns = run_code(code)
        meth = ns["C"].meth
        self.assertEqual(list(meth.__annotations__["x"]), [ns["C"].__type_params__[0]])
        self.assertEqual(meth.__annotations__["y"], "class")

    def test_nested_scope_in_generic_alias(self):
        code = """
            T = "global"
            class C:
                T = "class"
                {}
        """
        cases = [
            "type Alias[T] = (T for _ in (1,))",
            "type Alias = (T for _ in (1,))",
            "type Alias[T] = [T for _ in (1,)]",
            "type Alias = [T for _ in (1,)]",
        ]
        for case in cases:
            with self.subTest(case=case):
                ns = run_code(code.format(case))
                alias = ns["C"].Alias
                value = list(alias.__value__)[0]
                if alias.__type_params__:
                    self.assertIs(value, alias.__type_params__[0])
                else:
                    self.assertEqual(value, "global")

    def test_lambda_in_alias_in_class(self):
        code = """
            T = "global"
            class C:
                T = "class"
                type Alias = lambda: T
        """
        C = run_code(code)["C"]
        self.assertEqual(C.Alias.__value__(), "global")

    def test_lambda_in_alias_in_generic_class(self):
        code = """
            class C[T]:
                T = "class"
                type Alias = lambda: T
        """
        C = run_code(code)["C"]
        self.assertIs(C.Alias.__value__(), C.__type_params__[0])

    def test_lambda_in_generic_alias_in_class(self):
        # A lambda nested in the alias cannot see the class scope, but can see
        # a surrounding annotation scope.
        code = """
            T = U = "global"
            class C:
                T = "class"
                U = "class"
                type Alias[T] = lambda: (T, U)
        """
        C = run_code(code)["C"]
        T, U = C.Alias.__value__()
        self.assertIs(T, C.Alias.__type_params__[0])
        self.assertEqual(U, "global")

    def test_lambda_in_generic_alias_in_generic_class(self):
        # A lambda nested in the alias cannot see the class scope, but can see
        # a surrounding annotation scope.
        code = """
            class C[T, U]:
                T = "class"
                U = "class"
                type Alias[T] = lambda: (T, U)
        """
        C = run_code(code)["C"]
        T, U = C.Alias.__value__()
        self.assertIs(T, C.Alias.__type_params__[0])
        self.assertIs(U, C.__type_params__[1])

    def test_type_special_case(self):
        # https://github.com/python/cpython/issues/119011
        self.assertEqual(type.__type_params__, ())
        self.assertEqual(object.__type_params__, ())


def make_base(arg):
    class Base:
        __arg__ = arg
    return Base


def global_generic_func[T]():
    pass

class GlobalGenericClass[T]:
    pass


class TypeParamsLazyEvaluationTest(unittest.TestCase):
    def test_qualname(self):
        class Foo[T]:
            pass

        def func[T]():
            pass

        self.assertEqual(Foo.__qualname__, "TypeParamsLazyEvaluationTest.test_qualname.<locals>.Foo")
        self.assertEqual(func.__qualname__, "TypeParamsLazyEvaluationTest.test_qualname.<locals>.func")
        self.assertEqual(global_generic_func.__qualname__, "global_generic_func")
        self.assertEqual(GlobalGenericClass.__qualname__, "GlobalGenericClass")

    def test_recursive_class(self):
        class Foo[T: Foo, U: (Foo, Foo)]:
            pass

        type_params = Foo.__type_params__
        self.assertEqual(len(type_params), 2)
        self.assertEqual(type_params[0].__name__, "T")
        self.assertIs(type_params[0].__bound__, Foo)
        self.assertEqual(type_params[0].__constraints__, ())
        self.assertIs(type_params[0].__default__, NoDefault)

        self.assertEqual(type_params[1].__name__, "U")
        self.assertIs(type_params[1].__bound__, None)
        self.assertEqual(type_params[1].__constraints__, (Foo, Foo))
        self.assertIs(type_params[1].__default__, NoDefault)

    def test_evaluation_error(self):
        class Foo[T: Undefined, U: (Undefined,)]:
            pass

        type_params = Foo.__type_params__
        with self.assertRaises(NameError):
            type_params[0].__bound__
        self.assertEqual(type_params[0].__constraints__, ())
        self.assertIs(type_params[1].__bound__, None)
        self.assertIs(type_params[0].__default__, NoDefault)
        self.assertIs(type_params[1].__default__, NoDefault)
        with self.assertRaises(NameError):
            type_params[1].__constraints__

        Undefined = "defined"
        self.assertEqual(type_params[0].__bound__, "defined")
        self.assertEqual(type_params[0].__constraints__, ())

        self.assertIs(type_params[1].__bound__, None)
        self.assertEqual(type_params[1].__constraints__, ("defined",))


class TypeParamsClassScopeTest(unittest.TestCase):
    def test_alias(self):
        class X:
            T = int
            type U = T
        self.assertIs(X.U.__value__, int)

        ns = run_code("""
            glb = "global"
            class X:
                cls = "class"
                type U = (glb, cls)
        """)
        cls = ns["X"]
        self.assertEqual(cls.U.__value__, ("global", "class"))

    def test_bound(self):
        class X:
            T = int
            def foo[U: T](self): ...
        self.assertIs(X.foo.__type_params__[0].__bound__, int)

        ns = run_code("""
            glb = "global"
            class X:
                cls = "class"
                def foo[T: glb, U: cls](self): ...
        """)
        cls = ns["X"]
        T, U = cls.foo.__type_params__
        self.assertEqual(T.__bound__, "global")
        self.assertEqual(U.__bound__, "class")

    def test_modified_later(self):
        class X:
            T = int
            def foo[U: T](self): ...
            type Alias = T
        X.T = float
        self.assertIs(X.foo.__type_params__[0].__bound__, float)
        self.assertIs(X.Alias.__value__, float)

    def test_binding_uses_global(self):
        ns = run_code("""
            x = "global"
            def outer():
                x = "nonlocal"
                class Cls:
                    type Alias = x
                    val = Alias.__value__
                    def meth[T: x](self, arg: x): ...
                    bound = meth.__type_params__[0].__bound__
                    annotation = meth.__annotations__["arg"]
                    x = "class"
                return Cls
        """)
        cls = ns["outer"]()
        self.assertEqual(cls.val, "global")
        self.assertEqual(cls.bound, "global")
        self.assertEqual(cls.annotation, "global")

    def test_no_binding_uses_nonlocal(self):
        ns = run_code("""
            x = "global"
            def outer():
                x = "nonlocal"
                class Cls:
                    type Alias = x
                    val = Alias.__value__
                    def meth[T: x](self, arg: x): ...
                    bound = meth.__type_params__[0].__bound__
                return Cls
        """)
        cls = ns["outer"]()
        self.assertEqual(cls.val, "nonlocal")
        self.assertEqual(cls.bound, "nonlocal")
        self.assertEqual(cls.meth.__annotations__["arg"], "nonlocal")

    def test_explicit_global(self):
        ns = run_code("""
            x = "global"
            def outer():
                x = "nonlocal"
                class Cls:
                    global x
                    type Alias = x
                Cls.x = "class"
                return Cls
        """)
        cls = ns["outer"]()
        self.assertEqual(cls.Alias.__value__, "global")

    def test_explicit_global_with_no_static_bound(self):
        ns = run_code("""
            def outer():
                class Cls:
                    global x
                    type Alias = x
                Cls.x = "class"
                return Cls
        """)
        ns["x"] = "global"
        cls = ns["outer"]()
        self.assertEqual(cls.Alias.__value__, "global")

    def test_explicit_global_with_assignment(self):
        ns = run_code("""
            x = "global"
            def outer():
                x = "nonlocal"
                class Cls:
                    global x
                    type Alias = x
                    x = "global from class"
                Cls.x = "class"
                return Cls
        """)
        cls = ns["outer"]()
        self.assertEqual(cls.Alias.__value__, "global from class")

    def test_explicit_nonlocal(self):
        ns = run_code("""
            x = "global"
            def outer():
                x = "nonlocal"
                class Cls:
                    nonlocal x
                    type Alias = x
                    x = "class"
                return Cls
        """)
        cls = ns["outer"]()
        self.assertEqual(cls.Alias.__value__, "class")

    def test_nested_free(self):
        ns = run_code("""
            def f():
                T = str
                class C:
                    T = int
                    class D[U](T):
                        x = T
                return C
        """)
        C = ns["f"]()
        self.assertIn(int, C.D.__bases__)
        self.assertIs(C.D.x, str)


class DynamicClassTest(unittest.TestCase):
    def _set_type_params(self, ns, params):
        ns['__type_params__'] = params

    def test_types_new_class_with_callback(self):
        T = TypeVar('T', infer_variance=True)
        Klass = types.new_class('Klass', (Generic[T],), {},
                                lambda ns: self._set_type_params(ns, (T,)))

        self.assertEqual(Klass.__bases__, (Generic,))
        self.assertEqual(Klass.__orig_bases__, (Generic[T],))
        self.assertEqual(Klass.__type_params__, (T,))
        self.assertEqual(Klass.__parameters__, (T,))

    def test_types_new_class_no_callback(self):
        T = TypeVar('T', infer_variance=True)
        Klass = types.new_class('Klass', (Generic[T],), {})

        self.assertEqual(Klass.__bases__, (Generic,))
        self.assertEqual(Klass.__orig_bases__, (Generic[T],))
        self.assertEqual(Klass.__type_params__, ())  # must be explicitly set
        self.assertEqual(Klass.__parameters__, (T,))


class TypeParamsManglingTest(unittest.TestCase):
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

    def test_no_leaky_mangling_in_module(self):
        ns = run_code("""
            __before = "before"
            class X[T]: pass
            __after = "after"
        """)
        self.assertEqual(ns["__before"], "before")
        self.assertEqual(ns["__after"], "after")

    def test_no_leaky_mangling_in_function(self):
        ns = run_code("""
            def f():
                class X[T]: pass
                _X_foo = 2
                __foo = 1
                assert locals()['__foo'] == 1
                return __foo
        """)
        self.assertEqual(ns["f"](), 1)

    def test_no_leaky_mangling_in_class(self):
        ns = run_code("""
            class Outer:
                __before = "before"
                class Inner[T]:
                    __x = "inner"
                __after = "after"
        """)
        Outer = ns["Outer"]
        self.assertEqual(Outer._Outer__before, "before")
        self.assertEqual(Outer.Inner._Inner__x, "inner")
        self.assertEqual(Outer._Outer__after, "after")

    def test_no_mangling_in_bases(self):
        ns = run_code("""
            class __Base:
                def __init_subclass__(self, **kwargs):
                    self.kwargs = kwargs

            class Derived[T](__Base, __kwarg=1):
                pass
        """)
        Derived = ns["Derived"]
        self.assertEqual(Derived.__bases__, (ns["__Base"], Generic))
        self.assertEqual(Derived.kwargs, {"__kwarg": 1})

    def test_no_mangling_in_nested_scopes(self):
        ns = run_code("""
            from test.test_type_params import make_base

            class __X:
                pass

            class Y[T: __X](
                make_base(lambda: __X),
                # doubly nested scope
                make_base(lambda: (lambda: __X)),
                # list comprehension
                make_base([__X for _ in (1,)]),
                # genexp
                make_base(__X for _ in (1,)),
            ):
                pass
        """)
        Y = ns["Y"]
        T, = Y.__type_params__
        self.assertIs(T.__bound__, ns["__X"])
        base0 = Y.__bases__[0]
        self.assertIs(base0.__arg__(), ns["__X"])
        base1 = Y.__bases__[1]
        self.assertIs(base1.__arg__()(), ns["__X"])
        base2 = Y.__bases__[2]
        self.assertEqual(base2.__arg__, [ns["__X"]])
        base3 = Y.__bases__[3]
        self.assertEqual(list(base3.__arg__), [ns["__X"]])

    def test_type_params_are_mangled(self):
        ns = run_code("""
            from test.test_type_params import make_base

            class Foo[__T, __U: __T](make_base(__T), make_base(lambda: __T)):
                param = __T
        """)
        Foo = ns["Foo"]
        T, U = Foo.__type_params__
        self.assertEqual(T.__name__, "__T")
        self.assertEqual(U.__name__, "__U")
        self.assertIs(U.__bound__, T)
        self.assertIs(Foo.param, T)

        base1, base2, *_ = Foo.__bases__
        self.assertIs(base1.__arg__, T)
        self.assertIs(base2.__arg__(), T)


class TypeParamsComplexCallsTest(unittest.TestCase):
    def test_defaults(self):
        # Generic functions with both defaults and kwdefaults trigger a specific code path
        # in the compiler.
        def func[T](a: T = "a", *, b: T = "b"):
            return (a, b)

        T, = func.__type_params__
        self.assertIs(func.__annotations__["a"], T)
        self.assertIs(func.__annotations__["b"], T)
        self.assertEqual(func(), ("a", "b"))
        self.assertEqual(func(1), (1, "b"))
        self.assertEqual(func(b=2), ("a", 2))

    def test_complex_base(self):
        class Base:
            def __init_subclass__(cls, **kwargs) -> None:
                cls.kwargs = kwargs

        kwargs = {"c": 3}
        # Base classes with **kwargs trigger a different code path in the compiler.
        class C[T](Base, a=1, b=2, **kwargs):
            pass

        T, = C.__type_params__
        self.assertEqual(T.__name__, "T")
        self.assertEqual(C.kwargs, {"a": 1, "b": 2, "c": 3})
        self.assertEqual(C.__bases__, (Base, Generic))

        bases = (Base,)
        class C2[T](*bases, **kwargs):
            pass

        T, = C2.__type_params__
        self.assertEqual(T.__name__, "T")
        self.assertEqual(C2.kwargs, {"c": 3})
        self.assertEqual(C2.__bases__, (Base, Generic))

    def test_starargs_base(self):
        class C1[T](*()): pass

        T, = C1.__type_params__
        self.assertEqual(T.__name__, "T")
        self.assertEqual(C1.__bases__, (Generic,))

        class Base: pass
        bases = [Base]
        class C2[T](*bases): pass

        T, = C2.__type_params__
        self.assertEqual(T.__name__, "T")
        self.assertEqual(C2.__bases__, (Base, Generic))


class TypeParamsTraditionalTypeVarsTest(unittest.TestCase):
    def test_traditional_01(self):
        code = """
            from typing import Generic
            class ClassA[T](Generic[T]): ...
        """

        with self.assertRaisesRegex(TypeError, r"Cannot inherit from Generic\[...\] multiple times."):
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

        a, b = run_no_yield_async_fn(co)

        self.assertIsInstance(a, TypeVar)
        self.assertEqual(a.__name__, "A")
        self.assertIsInstance(b, TypeVar)
        self.assertEqual(b.__name__, "B")


class TypeParamsTypeVarTupleTest(unittest.TestCase):
    def test_typevartuple_01(self):
        code = """def func1[*A: str](): pass"""
        check_syntax_error(self, code, "cannot use bound with TypeVarTuple")
        code = """def func1[*A: (int, str)](): pass"""
        check_syntax_error(self, code, "cannot use constraints with TypeVarTuple")
        code = """class X[*A: str]: pass"""
        check_syntax_error(self, code, "cannot use bound with TypeVarTuple")
        code = """class X[*A: (int, str)]: pass"""
        check_syntax_error(self, code, "cannot use constraints with TypeVarTuple")
        code = """type X[*A: str] = int"""
        check_syntax_error(self, code, "cannot use bound with TypeVarTuple")
        code = """type X[*A: (int, str)] = int"""
        check_syntax_error(self, code, "cannot use constraints with TypeVarTuple")

    def test_typevartuple_02(self):
        def func1[*A]():
            return A

        a = func1()
        self.assertIsInstance(a, TypeVarTuple)


class TypeParamsTypeVarParamSpecTest(unittest.TestCase):
    def test_paramspec_01(self):
        code = """def func1[**A: str](): pass"""
        check_syntax_error(self, code, "cannot use bound with ParamSpec")
        code = """def func1[**A: (int, str)](): pass"""
        check_syntax_error(self, code, "cannot use constraints with ParamSpec")
        code = """class X[**A: str]: pass"""
        check_syntax_error(self, code, "cannot use bound with ParamSpec")
        code = """class X[**A: (int, str)]: pass"""
        check_syntax_error(self, code, "cannot use constraints with ParamSpec")
        code = """type X[**A: str] = int"""
        check_syntax_error(self, code, "cannot use bound with ParamSpec")
        code = """type X[**A: (int, str)] = int"""
        check_syntax_error(self, code, "cannot use constraints with ParamSpec")

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
        code = """
            class ClassA[A]():
                pass
            ClassA.__type_params__ = ()
            params = ClassA.__type_params__
        """

        ns = run_code(code)
        self.assertEqual(ns["params"], ())

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
        code = """
            def func[A]():
                pass
            func.__type_params__ = ()
        """

        ns = run_code(code)
        self.assertEqual(ns["func"].__type_params__, ())



# All these type aliases are used for pickling tests:
T = TypeVar('T')
def func1[X](x: X) -> X: ...
def func2[X, Y](x: X | Y) -> X | Y: ...
def func3[X, *Y, **Z](x: X, y: tuple[*Y], z: Z) -> X: ...
def func4[X: int, Y: (bytes, str)](x: X, y: Y) -> X | Y: ...

class Class1[X]: ...
class Class2[X, Y]: ...
class Class3[X, *Y, **Z]: ...
class Class4[X: int, Y: (bytes, str)]: ...


class TypeParamsPickleTest(unittest.TestCase):
    def test_pickling_functions(self):
        things_to_test = [
            func1,
            func2,
            func3,
            func4,
        ]
        for thing in things_to_test:
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(thing=thing, proto=proto):
                    pickled = pickle.dumps(thing, protocol=proto)
                    self.assertEqual(pickle.loads(pickled), thing)

    def test_pickling_classes(self):
        things_to_test = [
            Class1,
            Class1[int],
            Class1[T],

            Class2,
            Class2[int, T],
            Class2[T, int],
            Class2[int, str],

            Class3,
            Class3[int, T, str, bytes, [float, object, T]],

            Class4,
            Class4[int, bytes],
            Class4[T, bytes],
            Class4[int, T],
            Class4[T, T],
        ]
        for thing in things_to_test:
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(thing=thing, proto=proto):
                    pickled = pickle.dumps(thing, protocol=proto)
                    self.assertEqual(pickle.loads(pickled), thing)

        for klass in things_to_test:
            real_class = getattr(klass, '__origin__', klass)
            thing = klass()
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.subTest(thing=thing, proto=proto):
                    pickled = pickle.dumps(thing, protocol=proto)
                    # These instances are not equal,
                    # but class check is good enough:
                    self.assertIsInstance(pickle.loads(pickled), real_class)


class TypeParamsWeakRefTest(unittest.TestCase):
    def test_weakrefs(self):
        T = TypeVar('T')
        P = ParamSpec('P')
        class OldStyle(Generic[T]):
            pass

        class NewStyle[T]:
            pass

        cases = [
            T,
            TypeVar('T', bound=int),
            P,
            P.args,
            P.kwargs,
            TypeVarTuple('Ts'),
            OldStyle,
            OldStyle[int],
            OldStyle(),
            NewStyle,
            NewStyle[int],
            NewStyle(),
            Generic[T],
        ]
        for case in cases:
            with self.subTest(case=case):
                weakref.ref(case)


class TypeParamsRuntimeTest(unittest.TestCase):
    def test_name_error(self):
        # gh-109118: This crashed the interpreter due to a refcounting bug
        code = """
        class name_2[name_5]:
            class name_4[name_5](name_0):
                pass
        """
        with self.assertRaises(NameError):
            run_code(code)

        # Crashed with a slightly different stack trace
        code = """
        class name_2[name_5]:
            class name_4[name_5: name_5](name_0):
                pass
        """
        with self.assertRaises(NameError):
            run_code(code)

    def test_broken_class_namespace(self):
        code = """
        class WeirdMapping(dict):
            def __missing__(self, key):
                if key == "T":
                    raise RuntimeError
                raise KeyError(key)

        class Meta(type):
            def __prepare__(name, bases):
                return WeirdMapping()

        class MyClass[V](metaclass=Meta):
            class Inner[U](T):
                pass
        """
        with self.assertRaises(RuntimeError):
            run_code(code)


class DefaultsTest(unittest.TestCase):
    def test_defaults_on_func(self):
        ns = run_code("""
            def func[T=int, **U=float, *V=None]():
                pass
        """)

        T, U, V = ns["func"].__type_params__
        self.assertIs(T.__default__, int)
        self.assertIs(U.__default__, float)
        self.assertIs(V.__default__, None)

    def test_defaults_on_class(self):
        ns = run_code("""
            class C[T=int, **U=float, *V=None]:
                pass
        """)

        T, U, V = ns["C"].__type_params__
        self.assertIs(T.__default__, int)
        self.assertIs(U.__default__, float)
        self.assertIs(V.__default__, None)

    def test_defaults_on_type_alias(self):
        ns = run_code("""
            type Alias[T = int, **U = float, *V = None] = int
        """)

        T, U, V = ns["Alias"].__type_params__
        self.assertIs(T.__default__, int)
        self.assertIs(U.__default__, float)
        self.assertIs(V.__default__, None)

    def test_starred_invalid(self):
        check_syntax_error(self, "type Alias[T = *int] = int")
        check_syntax_error(self, "type Alias[**P = *int] = int")

    def test_starred_typevartuple(self):
        ns = run_code("""
            default = tuple[int, str]
            type Alias[*Ts = *default] = Ts
        """)

        Ts, = ns["Alias"].__type_params__
        self.assertEqual(Ts.__default__, next(iter(ns["default"])))

    def test_nondefault_after_default(self):
        check_syntax_error(self, "def func[T=int, U](): pass", "non-default type parameter 'U' follows default type parameter")
        check_syntax_error(self, "class C[T=int, U]: pass", "non-default type parameter 'U' follows default type parameter")
        check_syntax_error(self, "type A[T=int, U] = int", "non-default type parameter 'U' follows default type parameter")

    def test_lazy_evaluation(self):
        ns = run_code("""
            type Alias[T = Undefined, *U = Undefined, **V = Undefined] = int
        """)

        T, U, V = ns["Alias"].__type_params__

        with self.assertRaises(NameError):
            T.__default__
        with self.assertRaises(NameError):
            U.__default__
        with self.assertRaises(NameError):
            V.__default__

        ns["Undefined"] = "defined"
        self.assertEqual(T.__default__, "defined")
        self.assertEqual(U.__default__, "defined")
        self.assertEqual(V.__default__, "defined")

        # Now it is cached
        ns["Undefined"] = "redefined"
        self.assertEqual(T.__default__, "defined")
        self.assertEqual(U.__default__, "defined")
        self.assertEqual(V.__default__, "defined")

    def test_symtable_key_regression_default(self):
        # Test against the bugs that would happen if we used .default_
        # as the key in the symtable.
        ns = run_code("""
            type X[T = [T for T in [T]]] = T
        """)

        T, = ns["X"].__type_params__
        self.assertEqual(T.__default__, [T])

    def test_symtable_key_regression_name(self):
        # Test against the bugs that would happen if we used .name
        # as the key in the symtable.
        ns = run_code("""
            type X1[T = A] = T
            type X2[T = B] = T
            A = "A"
            B = "B"
        """)

        self.assertEqual(ns["X1"].__type_params__[0].__default__, "A")
        self.assertEqual(ns["X2"].__type_params__[0].__default__, "B")


class TestEvaluateFunctions(unittest.TestCase):
    def test_general(self):
        type Alias = int
        Alias2 = TypeAliasType("Alias2", int)
        def f[T: int = int, **P = int, *Ts = int](): pass
        T, P, Ts = f.__type_params__
        T2 = TypeVar("T2", bound=int, default=int)
        P2 = ParamSpec("P2", default=int)
        Ts2 = TypeVarTuple("Ts2", default=int)
        cases = [
            Alias.evaluate_value,
            Alias2.evaluate_value,
            T.evaluate_bound,
            T.evaluate_default,
            P.evaluate_default,
            Ts.evaluate_default,
            T2.evaluate_bound,
            T2.evaluate_default,
            P2.evaluate_default,
            Ts2.evaluate_default,
        ]
        for case in cases:
            with self.subTest(case=case):
                self.assertIs(case(1), int)
                self.assertIs(annotationlib.call_evaluate_function(case, annotationlib.Format.VALUE), int)
                self.assertIs(annotationlib.call_evaluate_function(case, annotationlib.Format.FORWARDREF), int)
                self.assertEqual(annotationlib.call_evaluate_function(case, annotationlib.Format.STRING), 'int')

    def test_constraints(self):
        def f[T: (int, str)](): pass
        T, = f.__type_params__
        T2 = TypeVar("T2", int, str)
        for case in [T, T2]:
            with self.subTest(case=case):
                self.assertEqual(case.evaluate_constraints(1), (int, str))
                self.assertEqual(annotationlib.call_evaluate_function(case.evaluate_constraints, annotationlib.Format.VALUE), (int, str))
                self.assertEqual(annotationlib.call_evaluate_function(case.evaluate_constraints, annotationlib.Format.FORWARDREF), (int, str))
                self.assertEqual(annotationlib.call_evaluate_function(case.evaluate_constraints, annotationlib.Format.STRING), '(int, str)')

    def test_const_evaluator(self):
        T = TypeVar("T", bound=int)
        self.assertEqual(repr(T.evaluate_bound), "<constevaluator <class 'int'>>")

        ConstEvaluator = type(T.evaluate_bound)

        with self.assertRaisesRegex(TypeError, r"cannot create '_typing\._ConstEvaluator' instances"):
            ConstEvaluator()  # This used to segfault.
        with self.assertRaisesRegex(TypeError, r"cannot set 'attribute' attribute of immutable type '_typing\._ConstEvaluator'"):
            ConstEvaluator.attribute = 1
