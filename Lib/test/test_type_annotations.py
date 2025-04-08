import annotationlib
import inspect
import textwrap
import types
import unittest
from test.support import run_code, check_syntax_error, cpython_only


class TypeAnnotationTests(unittest.TestCase):

    def test_lazy_create_annotations(self):
        # type objects lazy create their __annotations__ dict on demand.
        # the annotations dict is stored in type.__dict__.
        # a freshly created type shouldn't have an annotations dict yet.
        foo = type("Foo", (), {})
        for i in range(3):
            self.assertFalse("__annotations__" in foo.__dict__)
            d = foo.__annotations__
            self.assertTrue("__annotations__" in foo.__dict__)
            self.assertEqual(foo.__annotations__, d)
            self.assertEqual(foo.__dict__['__annotations__'], d)
            del foo.__annotations__

    def test_setting_annotations(self):
        foo = type("Foo", (), {})
        for i in range(3):
            self.assertFalse("__annotations__" in foo.__dict__)
            d = {'a': int}
            foo.__annotations__ = d
            self.assertTrue("__annotations__" in foo.__dict__)
            self.assertEqual(foo.__annotations__, d)
            self.assertEqual(foo.__dict__['__annotations__'], d)
            del foo.__annotations__

    def test_annotations_getset_raises(self):
        # builtin types don't have __annotations__ (yet!)
        with self.assertRaises(AttributeError):
            print(float.__annotations__)
        with self.assertRaises(TypeError):
            float.__annotations__ = {}
        with self.assertRaises(TypeError):
            del float.__annotations__

        # double delete
        foo = type("Foo", (), {})
        foo.__annotations__ = {}
        del foo.__annotations__
        with self.assertRaises(AttributeError):
            del foo.__annotations__

    def test_annotations_are_created_correctly(self):
        class C:
            a:int=3
            b:str=4
        self.assertEqual(C.__annotations__, {"a": int, "b": str})
        self.assertTrue("__annotations__" in C.__dict__)
        del C.__annotations__
        self.assertFalse("__annotations__" in C.__dict__)

    def test_descriptor_still_works(self):
        class C:
            def __init__(self, name=None, bases=None, d=None):
                self.my_annotations = None

            @property
            def __annotations__(self):
                if not hasattr(self, 'my_annotations'):
                    self.my_annotations = {}
                if not isinstance(self.my_annotations, dict):
                    self.my_annotations = {}
                return self.my_annotations

            @__annotations__.setter
            def __annotations__(self, value):
                if not isinstance(value, dict):
                    raise ValueError("can only set __annotations__ to a dict")
                self.my_annotations = value

            @__annotations__.deleter
            def __annotations__(self):
                if getattr(self, 'my_annotations', False) is None:
                    raise AttributeError('__annotations__')
                self.my_annotations = None

        c = C()
        self.assertEqual(c.__annotations__, {})
        d = {'a':'int'}
        c.__annotations__ = d
        self.assertEqual(c.__annotations__, d)
        with self.assertRaises(ValueError):
            c.__annotations__ = 123
        del c.__annotations__
        with self.assertRaises(AttributeError):
            del c.__annotations__
        self.assertEqual(c.__annotations__, {})


        class D(metaclass=C):
            pass

        self.assertEqual(D.__annotations__, {})
        d = {'a':'int'}
        D.__annotations__ = d
        self.assertEqual(D.__annotations__, d)
        with self.assertRaises(ValueError):
            D.__annotations__ = 123
        del D.__annotations__
        with self.assertRaises(AttributeError):
            del D.__annotations__
        self.assertEqual(D.__annotations__, {})

    @cpython_only
    def test_no_cell(self):
        # gh-130924: Test that uses of annotations in local scopes do not
        # create cell variables.
        def f(x):
            a: x
            return x

        self.assertEqual(f.__code__.co_cellvars, ())


def build_module(code: str, name: str = "top") -> types.ModuleType:
    ns = run_code(code)
    mod = types.ModuleType(name)
    mod.__dict__.update(ns)
    return mod


class TestSetupAnnotations(unittest.TestCase):
    def check(self, code: str):
        code = textwrap.dedent(code)
        for scope in ("module", "class"):
            with self.subTest(scope=scope):
                if scope == "class":
                    code = f"class C:\n{textwrap.indent(code, '    ')}"
                    ns = run_code(code)
                    annotations = ns["C"].__annotations__
                else:
                    annotations = build_module(code).__annotations__
                self.assertEqual(annotations, {"x": int})

    def test_top_level(self):
        self.check("x: int = 1")

    def test_blocks(self):
        self.check("if True:\n    x: int = 1")
        self.check("""
            while True:
                x: int = 1
                break
        """)
        self.check("""
            while False:
                pass
            else:
                x: int = 1
        """)
        self.check("""
            for i in range(1):
                x: int = 1
        """)
        self.check("""
            for i in range(1):
                pass
            else:
                x: int = 1
        """)

    def test_try(self):
        self.check("""
            try:
                x: int = 1
            except:
                pass
        """)
        self.check("""
            try:
                pass
            except:
                pass
            else:
                x: int = 1
        """)
        self.check("""
            try:
                pass
            except:
                pass
            finally:
                x: int = 1
        """)
        self.check("""
            try:
                1/0
            except:
                x: int = 1
        """)

    def test_try_star(self):
        self.check("""
            try:
                x: int = 1
            except* Exception:
                pass
        """)
        self.check("""
            try:
                pass
            except* Exception:
                pass
            else:
                x: int = 1
        """)
        self.check("""
            try:
                pass
            except* Exception:
                pass
            finally:
                x: int = 1
        """)
        self.check("""
            try:
                1/0
            except* Exception:
                x: int = 1
        """)

    def test_match(self):
        self.check("""
            match 0:
                case 0:
                    x: int = 1
        """)


class AnnotateTests(unittest.TestCase):
    """See PEP 649."""
    def test_manual_annotate(self):
        def f():
            pass
        mod = types.ModuleType("mod")
        class X:
            pass

        for obj in (f, mod, X):
            with self.subTest(obj=obj):
                self.check_annotations(obj)

    def check_annotations(self, f):
        self.assertEqual(f.__annotations__, {})
        self.assertIs(f.__annotate__, None)

        with self.assertRaisesRegex(TypeError, "__annotate__ must be callable or None"):
            f.__annotate__ = 42
        f.__annotate__ = lambda: 42
        with self.assertRaisesRegex(TypeError, r"takes 0 positional arguments but 1 was given"):
            print(f.__annotations__)

        f.__annotate__ = lambda x: 42
        with self.assertRaisesRegex(TypeError, r"__annotate__ returned non-dict of type 'int'"):
            print(f.__annotations__)

        f.__annotate__ = lambda x: {"x": x}
        self.assertEqual(f.__annotations__, {"x": 1})

        # Setting annotate to None does not invalidate the cached __annotations__
        f.__annotate__ = None
        self.assertEqual(f.__annotations__, {"x": 1})

        # But setting it to a new callable does
        f.__annotate__ = lambda x: {"y": x}
        self.assertEqual(f.__annotations__, {"y": 1})

        # Setting f.__annotations__ also clears __annotate__
        f.__annotations__ = {"z": 43}
        self.assertIs(f.__annotate__, None)


class DeferredEvaluationTests(unittest.TestCase):
    def test_function(self):
        def func(x: undefined, /, y: undefined, *args: undefined, z: undefined, **kwargs: undefined) -> undefined:
            pass

        with self.assertRaises(NameError):
            func.__annotations__

        undefined = 1
        self.assertEqual(func.__annotations__, {
            "x": 1,
            "y": 1,
            "args": 1,
            "z": 1,
            "kwargs": 1,
            "return": 1,
        })

    def test_async_function(self):
        async def func(x: undefined, /, y: undefined, *args: undefined, z: undefined, **kwargs: undefined) -> undefined:
            pass

        with self.assertRaises(NameError):
            func.__annotations__

        undefined = 1
        self.assertEqual(func.__annotations__, {
            "x": 1,
            "y": 1,
            "args": 1,
            "z": 1,
            "kwargs": 1,
            "return": 1,
        })

    def test_class(self):
        class X:
            a: undefined

        with self.assertRaises(NameError):
            X.__annotations__

        undefined = 1
        self.assertEqual(X.__annotations__, {"a": 1})

    def test_module(self):
        ns = run_code("x: undefined = 1")
        anno = ns["__annotate__"]
        with self.assertRaises(NotImplementedError):
            anno(3)

        with self.assertRaises(NameError):
            anno(1)

        ns["undefined"] = 1
        self.assertEqual(anno(1), {"x": 1})

    def test_class_scoping(self):
        class Outer:
            def meth(self, x: Nested): ...
            x: Nested
            class Nested: ...

        self.assertEqual(Outer.meth.__annotations__, {"x": Outer.Nested})
        self.assertEqual(Outer.__annotations__, {"x": Outer.Nested})

    def test_no_exotic_expressions(self):
        check_syntax_error(self, "def func(x: (yield)): ...", "yield expression cannot be used within an annotation")
        check_syntax_error(self, "def func(x: (yield from x)): ...", "yield expression cannot be used within an annotation")
        check_syntax_error(self, "def func(x: (y := 3)): ...", "named expression cannot be used within an annotation")
        check_syntax_error(self, "def func(x: (await 42)): ...", "await expression cannot be used within an annotation")

    def test_no_exotic_expressions_in_unevaluated_annotations(self):
        preludes = [
            "",
            "class X: ",
            "def f(): ",
            "async def f(): ",
        ]
        for prelude in preludes:
            with self.subTest(prelude=prelude):
                check_syntax_error(self, prelude + "(x): (yield)", "yield expression cannot be used within an annotation")
                check_syntax_error(self, prelude + "(x): (yield from x)", "yield expression cannot be used within an annotation")
                check_syntax_error(self, prelude + "(x): (y := 3)", "named expression cannot be used within an annotation")
                check_syntax_error(self, prelude + "(x): (__debug__ := 3)", "named expression cannot be used within an annotation")
                check_syntax_error(self, prelude + "(x): (await 42)", "await expression cannot be used within an annotation")

    def test_ignore_non_simple_annotations(self):
        ns = run_code("class X: (y): int")
        self.assertEqual(ns["X"].__annotations__, {})
        ns = run_code("class X: int.b: int")
        self.assertEqual(ns["X"].__annotations__, {})
        ns = run_code("class X: int[str]: int")
        self.assertEqual(ns["X"].__annotations__, {})

    def test_generated_annotate(self):
        def func(x: int):
            pass
        class X:
            x: int
        mod = build_module("x: int")
        for obj in (func, X, mod):
            with self.subTest(obj=obj):
                annotate = obj.__annotate__
                self.assertIsInstance(annotate, types.FunctionType)
                self.assertEqual(annotate.__name__, "__annotate__")
                with self.assertRaises(NotImplementedError):
                    annotate(annotationlib.Format.FORWARDREF)
                with self.assertRaises(NotImplementedError):
                    annotate(annotationlib.Format.STRING)
                with self.assertRaises(TypeError):
                    annotate(None)
                self.assertEqual(annotate(annotationlib.Format.VALUE), {"x": int})

                sig = inspect.signature(annotate)
                self.assertEqual(sig, inspect.Signature([
                    inspect.Parameter("format", inspect.Parameter.POSITIONAL_ONLY)
                ]))

    def test_comprehension_in_annotation(self):
        # This crashed in an earlier version of the code
        ns = run_code("x: [y for y in range(10)]")
        self.assertEqual(ns["__annotate__"](1), {"x": list(range(10))})

    def test_future_annotations(self):
        code = """
        from __future__ import annotations

        def f(x: int) -> int: pass
        """
        ns = run_code(code)
        f = ns["f"]
        self.assertIsInstance(f.__annotate__, types.FunctionType)
        annos = {"x": "int", "return": "int"}
        self.assertEqual(f.__annotate__(annotationlib.Format.VALUE), annos)
        self.assertEqual(f.__annotations__, annos)

    def test_name_clash_with_format(self):
        # this test would fail if __annotate__'s parameter was called "format"
        # during symbol table construction
        code = """
        class format: pass

        def f(x: format): pass
        """
        ns = run_code(code)
        f = ns["f"]
        self.assertEqual(f.__annotations__, {"x": ns["format"]})

        code = """
        class Outer:
            class format: pass

            def meth(self, x: format): ...
        """
        ns = run_code(code)
        self.assertEqual(ns["Outer"].meth.__annotations__, {"x": ns["Outer"].format})

        code = """
        def f(format):
            def inner(x: format): pass
            return inner
        res = f("closure var")
        """
        ns = run_code(code)
        self.assertEqual(ns["res"].__annotations__, {"x": "closure var"})

        code = """
        def f(x: format):
            pass
        """
        ns = run_code(code)
        # picks up the format() builtin
        self.assertEqual(ns["f"].__annotations__, {"x": format})

        code = """
        def outer():
            def f(x: format):
                pass
            if False:
                class format: pass
            return f
        f = outer()
        """
        ns = run_code(code)
        with self.assertRaisesRegex(
            NameError,
            "cannot access free variable 'format' where it is not associated with a value in enclosing scope",
        ):
            ns["f"].__annotations__


class ConditionalAnnotationTests(unittest.TestCase):
    def check_scopes(self, code, true_annos, false_annos):
        for scope in ("class", "module"):
            for (cond, expected) in (
                # Constants (so code might get optimized out)
                (True, true_annos), (False, false_annos),
                # Non-constant expressions
                ("not not len", true_annos), ("not len", false_annos),
            ):
                with self.subTest(scope=scope, cond=cond):
                    code_to_run = code.format(cond=cond)
                    if scope == "class":
                        code_to_run = "class Cls:\n" + textwrap.indent(textwrap.dedent(code_to_run), " " * 4)
                    ns = run_code(code_to_run)
                    if scope == "class":
                        self.assertEqual(ns["Cls"].__annotations__, expected)
                    else:
                        self.assertEqual(ns["__annotate__"](annotationlib.Format.VALUE),
                                         expected)

    def test_with(self):
        code = """
            class Swallower:
                def __enter__(self):
                    pass

                def __exit__(self, *args):
                    return True

            with Swallower():
                if {cond}:
                    about_to_raise: int
                    raise Exception
                in_with: "with"
        """
        self.check_scopes(code, {"about_to_raise": int}, {"in_with": "with"})

    def test_simple_if(self):
        code = """
            if {cond}:
                in_if: "if"
            else:
                in_if: "else"
        """
        self.check_scopes(code, {"in_if": "if"}, {"in_if": "else"})

    def test_if_elif(self):
        code = """
            if not len:
                in_if: "if"
            elif {cond}:
                in_elif: "elif"
            else:
                in_else: "else"
        """
        self.check_scopes(
            code,
            {"in_elif": "elif"},
            {"in_else": "else"}
        )

    def test_try(self):
        code = """
            try:
                if {cond}:
                    raise Exception
                in_try: "try"
            except Exception:
                in_except: "except"
            finally:
                in_finally: "finally"
        """
        self.check_scopes(
            code,
            {"in_except": "except", "in_finally": "finally"},
            {"in_try": "try", "in_finally": "finally"}
        )

    def test_try_star(self):
        code = """
            try:
                if {cond}:
                    raise Exception
                in_try_star: "try"
            except* Exception:
                in_except_star: "except"
            finally:
                in_finally: "finally"
        """
        self.check_scopes(
            code,
            {"in_except_star": "except", "in_finally": "finally"},
            {"in_try_star": "try", "in_finally": "finally"}
        )

    def test_while(self):
        code = """
            while {cond}:
                in_while: "while"
                break
            else:
                in_else: "else"
        """
        self.check_scopes(
            code,
            {"in_while": "while"},
            {"in_else": "else"}
        )

    def test_for(self):
        code = """
            for _ in ([1] if {cond} else []):
                in_for: "for"
            else:
                in_else: "else"
        """
        self.check_scopes(
            code,
            {"in_for": "for", "in_else": "else"},
            {"in_else": "else"}
        )

    def test_match(self):
        code = """
            match {cond}:
                case True:
                    x: "true"
                case False:
                    x: "false"
        """
        self.check_scopes(
            code,
            {"x": "true"},
            {"x": "false"}
        )

    def test_nesting_override(self):
        code = """
            if {cond}:
                x: "foo"
                if {cond}:
                    x: "bar"
        """
        self.check_scopes(
            code,
            {"x": "bar"},
            {}
        )

    def test_nesting_outer(self):
        code = """
            if {cond}:
                outer_before: "outer_before"
                if len:
                    inner_if: "inner_if"
                else:
                    inner_else: "inner_else"
                outer_after: "outer_after"
        """
        self.check_scopes(
            code,
            {"outer_before": "outer_before", "inner_if": "inner_if",
             "outer_after": "outer_after"},
            {}
        )

    def test_nesting_inner(self):
        code = """
            if len:
                outer_before: "outer_before"
                if {cond}:
                    inner_if: "inner_if"
                else:
                    inner_else: "inner_else"
                outer_after: "outer_after"
        """
        self.check_scopes(
            code,
            {"outer_before": "outer_before", "inner_if": "inner_if",
             "outer_after": "outer_after"},
            {"outer_before": "outer_before", "inner_else": "inner_else",
             "outer_after": "outer_after"},
        )

    def test_non_name_annotations(self):
        code = """
            before: "before"
            if {cond}:
                a = "x"
                a[0]: int
            else:
                a = object()
                a.b: str
            after: "after"
        """
        expected = {"before": "before", "after": "after"}
        self.check_scopes(code, expected, expected)
