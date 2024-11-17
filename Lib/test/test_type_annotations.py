import annotationlib
import textwrap
import types
import unittest
from test.support import run_code, check_syntax_error


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
            anno(2)

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
                with self.assertRaises(NotImplementedError):
                    annotate(None)
                self.assertEqual(annotate(annotationlib.Format.VALUE), {"x": int})

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
        code = """
        class format: pass

        def f(x: format): pass
        """
        ns = run_code(code)
        f = ns["f"]
        self.assertEqual(f.__annotations__, {"x": ns["format"]})
