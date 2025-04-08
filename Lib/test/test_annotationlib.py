"""Tests for the annotations module."""

import annotationlib
import builtins
import collections
import functools
import itertools
import pickle
import unittest
from annotationlib import (
    Format,
    ForwardRef,
    get_annotations,
    get_annotate_function,
    annotations_to_string,
    value_to_string,
)
from typing import Unpack

from test import support
from test.test_inspect import inspect_stock_annotations
from test.test_inspect import inspect_stringized_annotations
from test.test_inspect import inspect_stringized_annotations_2
from test.test_inspect import inspect_stringized_annotations_pep695


def times_three(fn):
    @functools.wraps(fn)
    def wrapper(a, b):
        return fn(a * 3, b * 3)

    return wrapper


class MyClass:
    def __repr__(self):
        return "my repr"


class TestFormat(unittest.TestCase):
    def test_enum(self):
        self.assertEqual(Format.VALUE.value, 1)
        self.assertEqual(Format.VALUE, 1)

        self.assertEqual(Format.VALUE_WITH_FAKE_GLOBALS.value, 2)
        self.assertEqual(Format.VALUE_WITH_FAKE_GLOBALS, 2)

        self.assertEqual(Format.FORWARDREF.value, 3)
        self.assertEqual(Format.FORWARDREF, 3)

        self.assertEqual(Format.STRING.value, 4)
        self.assertEqual(Format.STRING, 4)


class TestForwardRefFormat(unittest.TestCase):
    def test_closure(self):
        def inner(arg: x):
            pass

        anno = annotationlib.get_annotations(inner, format=Format.FORWARDREF)
        fwdref = anno["arg"]
        self.assertIsInstance(fwdref, annotationlib.ForwardRef)
        self.assertEqual(fwdref.__forward_arg__, "x")
        with self.assertRaises(NameError):
            fwdref.evaluate()

        x = 1
        self.assertEqual(fwdref.evaluate(), x)

        anno = annotationlib.get_annotations(inner, format=Format.FORWARDREF)
        self.assertEqual(anno["arg"], x)

    def test_function(self):
        def f(x: int, y: doesntexist):
            pass

        anno = annotationlib.get_annotations(f, format=Format.FORWARDREF)
        self.assertIs(anno["x"], int)
        fwdref = anno["y"]
        self.assertIsInstance(fwdref, annotationlib.ForwardRef)
        self.assertEqual(fwdref.__forward_arg__, "doesntexist")
        with self.assertRaises(NameError):
            fwdref.evaluate()
        self.assertEqual(fwdref.evaluate(globals={"doesntexist": 1}), 1)

    def test_nonexistent_attribute(self):
        def f(
            x: some.module,
            y: some[module],
            z: some(module),
            alpha: some | obj,
            beta: +some,
            gamma: some < obj,
        ):
            pass

        anno = annotationlib.get_annotations(f, format=Format.FORWARDREF)
        x_anno = anno["x"]
        self.assertIsInstance(x_anno, ForwardRef)
        self.assertEqual(x_anno, support.EqualToForwardRef("some.module", owner=f))

        y_anno = anno["y"]
        self.assertIsInstance(y_anno, ForwardRef)
        self.assertEqual(y_anno, support.EqualToForwardRef("some[module]", owner=f))

        z_anno = anno["z"]
        self.assertIsInstance(z_anno, ForwardRef)
        self.assertEqual(z_anno, support.EqualToForwardRef("some(module)", owner=f))

        alpha_anno = anno["alpha"]
        self.assertIsInstance(alpha_anno, ForwardRef)
        self.assertEqual(alpha_anno, support.EqualToForwardRef("some | obj", owner=f))

        beta_anno = anno["beta"]
        self.assertIsInstance(beta_anno, ForwardRef)
        self.assertEqual(beta_anno, support.EqualToForwardRef("+some", owner=f))

        gamma_anno = anno["gamma"]
        self.assertIsInstance(gamma_anno, ForwardRef)
        self.assertEqual(gamma_anno, support.EqualToForwardRef("some < obj", owner=f))


class TestSourceFormat(unittest.TestCase):
    def test_closure(self):
        x = 0

        def inner(arg: x):
            pass

        anno = annotationlib.get_annotations(inner, format=Format.STRING)
        self.assertEqual(anno, {"arg": "x"})

    def test_closure_undefined(self):
        if False:
            x = 0

        def inner(arg: x):
            pass

        anno = annotationlib.get_annotations(inner, format=Format.STRING)
        self.assertEqual(anno, {"arg": "x"})

    def test_function(self):
        def f(x: int, y: doesntexist):
            pass

        anno = annotationlib.get_annotations(f, format=Format.STRING)
        self.assertEqual(anno, {"x": "int", "y": "doesntexist"})

    def test_expressions(self):
        def f(
            add: a + b,
            sub: a - b,
            mul: a * b,
            matmul: a @ b,
            truediv: a / b,
            mod: a % b,
            lshift: a << b,
            rshift: a >> b,
            or_: a | b,
            xor: a ^ b,
            and_: a & b,
            floordiv: a // b,
            pow_: a**b,
            lt: a < b,
            le: a <= b,
            eq: a == b,
            ne: a != b,
            gt: a > b,
            ge: a >= b,
            invert: ~a,
            neg: -a,
            pos: +a,
            getitem: a[b],
            getattr: a.b,
            call: a(b, *c, d=e),  # **kwargs are not supported
            *args: *a,
        ):
            pass

        anno = annotationlib.get_annotations(f, format=Format.STRING)
        self.assertEqual(
            anno,
            {
                "add": "a + b",
                "sub": "a - b",
                "mul": "a * b",
                "matmul": "a @ b",
                "truediv": "a / b",
                "mod": "a % b",
                "lshift": "a << b",
                "rshift": "a >> b",
                "or_": "a | b",
                "xor": "a ^ b",
                "and_": "a & b",
                "floordiv": "a // b",
                "pow_": "a ** b",
                "lt": "a < b",
                "le": "a <= b",
                "eq": "a == b",
                "ne": "a != b",
                "gt": "a > b",
                "ge": "a >= b",
                "invert": "~a",
                "neg": "-a",
                "pos": "+a",
                "getitem": "a[b]",
                "getattr": "a.b",
                "call": "a(b, *c, d=e)",
                "args": "*a",
            },
        )

    def test_reverse_ops(self):
        def f(
            radd: 1 + a,
            rsub: 1 - a,
            rmul: 1 * a,
            rmatmul: 1 @ a,
            rtruediv: 1 / a,
            rmod: 1 % a,
            rlshift: 1 << a,
            rrshift: 1 >> a,
            ror: 1 | a,
            rxor: 1 ^ a,
            rand: 1 & a,
            rfloordiv: 1 // a,
            rpow: 1**a,
        ):
            pass

        anno = annotationlib.get_annotations(f, format=Format.STRING)
        self.assertEqual(
            anno,
            {
                "radd": "1 + a",
                "rsub": "1 - a",
                "rmul": "1 * a",
                "rmatmul": "1 @ a",
                "rtruediv": "1 / a",
                "rmod": "1 % a",
                "rlshift": "1 << a",
                "rrshift": "1 >> a",
                "ror": "1 | a",
                "rxor": "1 ^ a",
                "rand": "1 & a",
                "rfloordiv": "1 // a",
                "rpow": "1 ** a",
            },
        )

    def test_nested_expressions(self):
        def f(
            nested: list[Annotated[set[int], "set of ints", 4j]],
            set: {a + b},  # single element because order is not guaranteed
            dict: {a + b: c + d, "key": e + g},
            list: [a, b, c],
            tuple: (a, b, c),
            slice: (a[b:c], a[b:c:d], a[:c], a[b:], a[:], a[::d], a[b::d]),
            extended_slice: a[:, :, c:d],
            unpack1: [*a],
            unpack2: [*a, b, c],
        ):
            pass

        anno = annotationlib.get_annotations(f, format=Format.STRING)
        self.assertEqual(
            anno,
            {
                "nested": "list[Annotated[set[int], 'set of ints', 4j]]",
                "set": "{a + b}",
                "dict": "{a + b: c + d, 'key': e + g}",
                "list": "[a, b, c]",
                "tuple": "(a, b, c)",
                "slice": "(a[b:c], a[b:c:d], a[:c], a[b:], a[:], a[::d], a[b::d])",
                "extended_slice": "a[:, :, c:d]",
                "unpack1": "[*a]",
                "unpack2": "[*a, b, c]",
            },
        )

    def test_unsupported_operations(self):
        format_msg = "Cannot stringify annotation containing string formatting"

        def f(fstring: f"{a}"):
            pass

        with self.assertRaisesRegex(TypeError, format_msg):
            annotationlib.get_annotations(f, format=Format.STRING)

        def f(fstring_format: f"{a:02d}"):
            pass

        with self.assertRaisesRegex(TypeError, format_msg):
            annotationlib.get_annotations(f, format=Format.STRING)


class TestForwardRefClass(unittest.TestCase):
    def test_special_attrs(self):
        # Forward refs provide a different introspection API. __name__ and
        # __qualname__ make little sense for forward refs as they can store
        # complex typing expressions.
        fr = annotationlib.ForwardRef("set[Any]")
        self.assertFalse(hasattr(fr, "__name__"))
        self.assertFalse(hasattr(fr, "__qualname__"))
        self.assertEqual(fr.__module__, "annotationlib")
        # Forward refs are currently unpicklable once they contain a code object.
        fr.__forward_code__  # fill the cache
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises(TypeError):
                pickle.dumps(fr, proto)

    def test_evaluate_with_type_params(self):
        class Gen[T]:
            alias = int

        with self.assertRaises(NameError):
            ForwardRef("T").evaluate()
        with self.assertRaises(NameError):
            ForwardRef("T").evaluate(type_params=())
        with self.assertRaises(NameError):
            ForwardRef("T").evaluate(owner=int)

        (T,) = Gen.__type_params__
        self.assertIs(ForwardRef("T").evaluate(type_params=Gen.__type_params__), T)
        self.assertIs(ForwardRef("T").evaluate(owner=Gen), T)

        with self.assertRaises(NameError):
            ForwardRef("alias").evaluate(type_params=Gen.__type_params__)
        self.assertIs(ForwardRef("alias").evaluate(owner=Gen), int)
        # If you pass custom locals, we don't look at the owner's locals
        with self.assertRaises(NameError):
            ForwardRef("alias").evaluate(owner=Gen, locals={})
        # But if the name exists in the locals, it works
        self.assertIs(
            ForwardRef("alias").evaluate(owner=Gen, locals={"alias": str}), str
        )

    def test_fwdref_with_module(self):
        self.assertIs(ForwardRef("Format", module="annotationlib").evaluate(), Format)
        self.assertIs(
            ForwardRef("Counter", module="collections").evaluate(), collections.Counter
        )
        self.assertEqual(
            ForwardRef("Counter[int]", module="collections").evaluate(),
            collections.Counter[int],
        )

        with self.assertRaises(NameError):
            # If globals are passed explicitly, we don't look at the module dict
            ForwardRef("Format", module="annotationlib").evaluate(globals={})

    def test_fwdref_to_builtin(self):
        self.assertIs(ForwardRef("int").evaluate(), int)
        self.assertIs(ForwardRef("int", module="collections").evaluate(), int)
        self.assertIs(ForwardRef("int", owner=str).evaluate(), int)

        # builtins are still searched with explicit globals
        self.assertIs(ForwardRef("int").evaluate(globals={}), int)

        # explicit values in globals have precedence
        obj = object()
        self.assertIs(ForwardRef("int").evaluate(globals={"int": obj}), obj)

    def test_fwdref_value_is_not_cached(self):
        fr = ForwardRef("hello")
        with self.assertRaises(NameError):
            fr.evaluate()
        self.assertIs(fr.evaluate(globals={"hello": str}), str)
        with self.assertRaises(NameError):
            fr.evaluate()

    def test_fwdref_with_owner(self):
        self.assertEqual(
            ForwardRef("Counter[int]", owner=collections).evaluate(),
            collections.Counter[int],
        )

    def test_name_lookup_without_eval(self):
        # test the codepath where we look up simple names directly in the
        # namespaces without going through eval()
        self.assertIs(ForwardRef("int").evaluate(), int)
        self.assertIs(ForwardRef("int").evaluate(locals={"int": str}), str)
        self.assertIs(
            ForwardRef("int").evaluate(locals={"int": float}, globals={"int": str}),
            float,
        )
        self.assertIs(ForwardRef("int").evaluate(globals={"int": str}), str)
        with support.swap_attr(builtins, "int", dict):
            self.assertIs(ForwardRef("int").evaluate(), dict)

        with self.assertRaises(NameError):
            ForwardRef("doesntexist").evaluate()

    def test_fwdref_invalid_syntax(self):
        fr = ForwardRef("if")
        with self.assertRaises(SyntaxError):
            fr.evaluate()
        fr = ForwardRef("1+")
        with self.assertRaises(SyntaxError):
            fr.evaluate()


class TestGetAnnotations(unittest.TestCase):
    def test_builtin_type(self):
        self.assertEqual(annotationlib.get_annotations(int), {})
        self.assertEqual(annotationlib.get_annotations(object), {})

    def test_custom_metaclass(self):
        class Meta(type):
            pass

        class C(metaclass=Meta):
            x: int

        self.assertEqual(annotationlib.get_annotations(C), {"x": int})

    def test_missing_dunder_dict(self):
        class NoDict(type):
            @property
            def __dict__(cls):
                raise AttributeError

            b: str

        class C1(metaclass=NoDict):
            a: int

        self.assertEqual(annotationlib.get_annotations(C1), {"a": int})
        self.assertEqual(
            annotationlib.get_annotations(C1, format=Format.FORWARDREF),
            {"a": int},
        )
        self.assertEqual(
            annotationlib.get_annotations(C1, format=Format.STRING),
            {"a": "int"},
        )
        self.assertEqual(annotationlib.get_annotations(NoDict), {"b": str})
        self.assertEqual(
            annotationlib.get_annotations(NoDict, format=Format.FORWARDREF),
            {"b": str},
        )
        self.assertEqual(
            annotationlib.get_annotations(NoDict, format=Format.STRING),
            {"b": "str"},
        )

    def test_format(self):
        def f1(a: int):
            pass

        def f2(a: undefined):
            pass

        self.assertEqual(
            annotationlib.get_annotations(f1, format=Format.VALUE),
            {"a": int},
        )
        self.assertEqual(annotationlib.get_annotations(f1, format=1), {"a": int})

        fwd = support.EqualToForwardRef("undefined", owner=f2)
        self.assertEqual(
            annotationlib.get_annotations(f2, format=Format.FORWARDREF),
            {"a": fwd},
        )
        self.assertEqual(annotationlib.get_annotations(f2, format=3), {"a": fwd})

        self.assertEqual(
            annotationlib.get_annotations(f1, format=Format.STRING),
            {"a": "int"},
        )
        self.assertEqual(annotationlib.get_annotations(f1, format=4), {"a": "int"})

        with self.assertRaises(ValueError):
            annotationlib.get_annotations(f1, format=42)

        with self.assertRaisesRegex(
            ValueError,
            r"The VALUE_WITH_FAKE_GLOBALS format is for internal use only",
        ):
            annotationlib.get_annotations(f1, format=Format.VALUE_WITH_FAKE_GLOBALS)

        with self.assertRaisesRegex(
            ValueError,
            r"The VALUE_WITH_FAKE_GLOBALS format is for internal use only",
        ):
            annotationlib.get_annotations(f1, format=2)

    def test_custom_object_with_annotations(self):
        class C:
            def __init__(self):
                self.__annotations__ = {"x": int, "y": str}

        self.assertEqual(annotationlib.get_annotations(C()), {"x": int, "y": str})

    def test_custom_format_eval_str(self):
        def foo():
            pass

        with self.assertRaises(ValueError):
            annotationlib.get_annotations(foo, format=Format.FORWARDREF, eval_str=True)
            annotationlib.get_annotations(foo, format=Format.STRING, eval_str=True)

    def test_stock_annotations(self):
        def foo(a: int, b: str):
            pass

        for format in (Format.VALUE, Format.FORWARDREF):
            with self.subTest(format=format):
                self.assertEqual(
                    annotationlib.get_annotations(foo, format=format),
                    {"a": int, "b": str},
                )
        self.assertEqual(
            annotationlib.get_annotations(foo, format=Format.STRING),
            {"a": "int", "b": "str"},
        )

        foo.__annotations__ = {"a": "foo", "b": "str"}
        for format in Format:
            if format == Format.VALUE_WITH_FAKE_GLOBALS:
                continue
            with self.subTest(format=format):
                self.assertEqual(
                    annotationlib.get_annotations(foo, format=format),
                    {"a": "foo", "b": "str"},
                )

        self.assertEqual(
            annotationlib.get_annotations(foo, eval_str=True, locals=locals()),
            {"a": foo, "b": str},
        )
        self.assertEqual(
            annotationlib.get_annotations(foo, eval_str=True, globals=locals()),
            {"a": foo, "b": str},
        )

    def test_stock_annotations_in_module(self):
        isa = inspect_stock_annotations

        for kwargs in [
            {},
            {"eval_str": False},
            {"format": Format.VALUE},
            {"format": Format.FORWARDREF},
            {"format": Format.VALUE, "eval_str": False},
            {"format": Format.FORWARDREF, "eval_str": False},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(
                    annotationlib.get_annotations(isa, **kwargs), {"a": int, "b": str}
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.MyClass, **kwargs),
                    {"a": int, "b": str},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function, **kwargs),
                    {"a": int, "b": str, "return": isa.MyClass},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function2, **kwargs),
                    {"a": int, "b": "str", "c": isa.MyClass, "return": isa.MyClass},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function3, **kwargs),
                    {"a": "int", "b": "str", "c": "MyClass"},
                )
                self.assertEqual(
                    annotationlib.get_annotations(annotationlib, **kwargs), {}
                )  # annotations module has no annotations
                self.assertEqual(
                    annotationlib.get_annotations(isa.UnannotatedClass, **kwargs), {}
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.unannotated_function, **kwargs),
                    {},
                )

        for kwargs in [
            {"eval_str": True},
            {"format": Format.VALUE, "eval_str": True},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(
                    annotationlib.get_annotations(isa, **kwargs), {"a": int, "b": str}
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.MyClass, **kwargs),
                    {"a": int, "b": str},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function, **kwargs),
                    {"a": int, "b": str, "return": isa.MyClass},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function2, **kwargs),
                    {"a": int, "b": str, "c": isa.MyClass, "return": isa.MyClass},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function3, **kwargs),
                    {"a": int, "b": str, "c": isa.MyClass},
                )
                self.assertEqual(
                    annotationlib.get_annotations(annotationlib, **kwargs), {}
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.UnannotatedClass, **kwargs), {}
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.unannotated_function, **kwargs),
                    {},
                )

        self.assertEqual(
            annotationlib.get_annotations(isa, format=Format.STRING),
            {"a": "int", "b": "str"},
        )
        self.assertEqual(
            annotationlib.get_annotations(isa.MyClass, format=Format.STRING),
            {"a": "int", "b": "str"},
        )
        self.assertEqual(
            annotationlib.get_annotations(isa.function, format=Format.STRING),
            {"a": "int", "b": "str", "return": "MyClass"},
        )
        self.assertEqual(
            annotationlib.get_annotations(isa.function2, format=Format.STRING),
            {"a": "int", "b": "str", "c": "MyClass", "return": "MyClass"},
        )
        self.assertEqual(
            annotationlib.get_annotations(isa.function3, format=Format.STRING),
            {"a": "int", "b": "str", "c": "MyClass"},
        )
        self.assertEqual(
            annotationlib.get_annotations(annotationlib, format=Format.STRING),
            {},
        )
        self.assertEqual(
            annotationlib.get_annotations(isa.UnannotatedClass, format=Format.STRING),
            {},
        )
        self.assertEqual(
            annotationlib.get_annotations(
                isa.unannotated_function, format=Format.STRING
            ),
            {},
        )

    def test_stock_annotations_on_wrapper(self):
        isa = inspect_stock_annotations

        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, "x"), isa.MyClass(3, "xxx"))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(
            annotationlib.get_annotations(wrapped),
            {"a": int, "b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            annotationlib.get_annotations(wrapped, format=Format.FORWARDREF),
            {"a": int, "b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            annotationlib.get_annotations(wrapped, format=Format.STRING),
            {"a": "int", "b": "str", "return": "MyClass"},
        )
        self.assertEqual(
            annotationlib.get_annotations(wrapped, eval_str=True),
            {"a": int, "b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            annotationlib.get_annotations(wrapped, eval_str=False),
            {"a": int, "b": str, "return": isa.MyClass},
        )

    def test_stringized_annotations_in_module(self):
        isa = inspect_stringized_annotations
        for kwargs in [
            {},
            {"eval_str": False},
            {"format": Format.VALUE},
            {"format": Format.FORWARDREF},
            {"format": Format.STRING},
            {"format": Format.VALUE, "eval_str": False},
            {"format": Format.FORWARDREF, "eval_str": False},
            {"format": Format.STRING, "eval_str": False},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(
                    annotationlib.get_annotations(isa, **kwargs),
                    {"a": "int", "b": "str"},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.MyClass, **kwargs),
                    {"a": "int", "b": "str"},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function, **kwargs),
                    {"a": "int", "b": "str", "return": "MyClass"},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function2, **kwargs),
                    {"a": "int", "b": "'str'", "c": "MyClass", "return": "MyClass"},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function3, **kwargs),
                    {"a": "'int'", "b": "'str'", "c": "'MyClass'"},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.UnannotatedClass, **kwargs), {}
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.unannotated_function, **kwargs),
                    {},
                )

        for kwargs in [
            {"eval_str": True},
            {"format": Format.VALUE, "eval_str": True},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(
                    annotationlib.get_annotations(isa, **kwargs), {"a": int, "b": str}
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.MyClass, **kwargs),
                    {"a": int, "b": str},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function, **kwargs),
                    {"a": int, "b": str, "return": isa.MyClass},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function2, **kwargs),
                    {"a": int, "b": "str", "c": isa.MyClass, "return": isa.MyClass},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.function3, **kwargs),
                    {"a": "int", "b": "str", "c": "MyClass"},
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.UnannotatedClass, **kwargs), {}
                )
                self.assertEqual(
                    annotationlib.get_annotations(isa.unannotated_function, **kwargs),
                    {},
                )

    def test_stringized_annotations_in_empty_module(self):
        isa2 = inspect_stringized_annotations_2
        self.assertEqual(annotationlib.get_annotations(isa2), {})
        self.assertEqual(annotationlib.get_annotations(isa2, eval_str=True), {})
        self.assertEqual(annotationlib.get_annotations(isa2, eval_str=False), {})

    def test_stringized_annotations_on_wrapper(self):
        isa = inspect_stringized_annotations
        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, "x"), isa.MyClass(3, "xxx"))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(
            annotationlib.get_annotations(wrapped),
            {"a": "int", "b": "str", "return": "MyClass"},
        )
        self.assertEqual(
            annotationlib.get_annotations(wrapped, eval_str=True),
            {"a": int, "b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            annotationlib.get_annotations(wrapped, eval_str=False),
            {"a": "int", "b": "str", "return": "MyClass"},
        )

    def test_stringized_annotations_on_class(self):
        isa = inspect_stringized_annotations
        # test that local namespace lookups work
        self.assertEqual(
            annotationlib.get_annotations(isa.MyClassWithLocalAnnotations),
            {"x": "mytype"},
        )
        self.assertEqual(
            annotationlib.get_annotations(
                isa.MyClassWithLocalAnnotations, eval_str=True
            ),
            {"x": int},
        )

    def test_modify_annotations(self):
        def f(x: int):
            pass

        self.assertEqual(annotationlib.get_annotations(f), {"x": int})
        self.assertEqual(
            annotationlib.get_annotations(f, format=Format.FORWARDREF),
            {"x": int},
        )

        f.__annotations__["x"] = str
        # The modification is reflected in VALUE (the default)
        self.assertEqual(annotationlib.get_annotations(f), {"x": str})
        # ... and also in FORWARDREF, which tries __annotations__ if available
        self.assertEqual(
            annotationlib.get_annotations(f, format=Format.FORWARDREF),
            {"x": str},
        )
        # ... but not in STRING which always uses __annotate__
        self.assertEqual(
            annotationlib.get_annotations(f, format=Format.STRING),
            {"x": "int"},
        )

    def test_non_dict_annotations(self):
        class WeirdAnnotations:
            @property
            def __annotations__(self):
                return "not a dict"

        wa = WeirdAnnotations()
        for format in Format:
            if format == Format.VALUE_WITH_FAKE_GLOBALS:
                continue
            with (
                self.subTest(format=format),
                self.assertRaisesRegex(
                    ValueError, r".*__annotations__ is neither a dict nor None"
                ),
            ):
                annotationlib.get_annotations(wa, format=format)

    def test_annotations_on_custom_object(self):
        class HasAnnotations:
            @property
            def __annotations__(self):
                return {"x": int}

        ha = HasAnnotations()
        self.assertEqual(
            annotationlib.get_annotations(ha, format=Format.VALUE), {"x": int}
        )
        self.assertEqual(
            annotationlib.get_annotations(ha, format=Format.FORWARDREF), {"x": int}
        )

        self.assertEqual(
            annotationlib.get_annotations(ha, format=Format.STRING), {"x": "int"}
        )

    def test_raising_annotations_on_custom_object(self):
        class HasRaisingAnnotations:
            @property
            def __annotations__(self):
                return {"x": undefined}

        hra = HasRaisingAnnotations()

        with self.assertRaises(NameError):
            annotationlib.get_annotations(hra, format=Format.VALUE)

        with self.assertRaises(NameError):
            annotationlib.get_annotations(hra, format=Format.FORWARDREF)

        undefined = float
        self.assertEqual(
            annotationlib.get_annotations(hra, format=Format.VALUE), {"x": float}
        )

    def test_forwardref_prefers_annotations(self):
        class HasBoth:
            @property
            def __annotations__(self):
                return {"x": int}

            @property
            def __annotate__(self):
                return lambda format: {"x": str}

        hb = HasBoth()
        self.assertEqual(
            annotationlib.get_annotations(hb, format=Format.VALUE), {"x": int}
        )
        self.assertEqual(
            annotationlib.get_annotations(hb, format=Format.FORWARDREF), {"x": int}
        )
        self.assertEqual(
            annotationlib.get_annotations(hb, format=Format.STRING), {"x": str}
        )

    def test_pep695_generic_class_with_future_annotations(self):
        ann_module695 = inspect_stringized_annotations_pep695
        A_annotations = annotationlib.get_annotations(ann_module695.A, eval_str=True)
        A_type_params = ann_module695.A.__type_params__
        self.assertIs(A_annotations["x"], A_type_params[0])
        self.assertEqual(A_annotations["y"].__args__[0], Unpack[A_type_params[1]])
        self.assertIs(A_annotations["z"].__args__[0], A_type_params[2])

    def test_pep695_generic_class_with_future_annotations_and_local_shadowing(self):
        B_annotations = annotationlib.get_annotations(
            inspect_stringized_annotations_pep695.B, eval_str=True
        )
        self.assertEqual(B_annotations, {"x": int, "y": str, "z": bytes})

    def test_pep695_generic_class_with_future_annotations_name_clash_with_global_vars(
        self,
    ):
        ann_module695 = inspect_stringized_annotations_pep695
        C_annotations = annotationlib.get_annotations(ann_module695.C, eval_str=True)
        self.assertEqual(
            set(C_annotations.values()), set(ann_module695.C.__type_params__)
        )

    def test_pep_695_generic_function_with_future_annotations(self):
        ann_module695 = inspect_stringized_annotations_pep695
        generic_func_annotations = annotationlib.get_annotations(
            ann_module695.generic_function, eval_str=True
        )
        func_t_params = ann_module695.generic_function.__type_params__
        self.assertEqual(
            generic_func_annotations.keys(), {"x", "y", "z", "zz", "return"}
        )
        self.assertIs(generic_func_annotations["x"], func_t_params[0])
        self.assertEqual(generic_func_annotations["y"], Unpack[func_t_params[1]])
        self.assertIs(generic_func_annotations["z"].__origin__, func_t_params[2])
        self.assertIs(generic_func_annotations["zz"].__origin__, func_t_params[2])

    def test_pep_695_generic_function_with_future_annotations_name_clash_with_global_vars(
        self,
    ):
        self.assertEqual(
            set(
                annotationlib.get_annotations(
                    inspect_stringized_annotations_pep695.generic_function_2,
                    eval_str=True,
                ).values()
            ),
            set(
                inspect_stringized_annotations_pep695.generic_function_2.__type_params__
            ),
        )

    def test_pep_695_generic_method_with_future_annotations(self):
        ann_module695 = inspect_stringized_annotations_pep695
        generic_method_annotations = annotationlib.get_annotations(
            ann_module695.D.generic_method, eval_str=True
        )
        params = {
            param.__name__: param
            for param in ann_module695.D.generic_method.__type_params__
        }
        self.assertEqual(
            generic_method_annotations,
            {"x": params["Foo"], "y": params["Bar"], "return": None},
        )

    def test_pep_695_generic_method_with_future_annotations_name_clash_with_global_vars(
        self,
    ):
        self.assertEqual(
            set(
                annotationlib.get_annotations(
                    inspect_stringized_annotations_pep695.D.generic_method_2,
                    eval_str=True,
                ).values()
            ),
            set(
                inspect_stringized_annotations_pep695.D.generic_method_2.__type_params__
            ),
        )

    def test_pep_695_generic_method_with_future_annotations_name_clash_with_global_and_local_vars(
        self,
    ):
        self.assertEqual(
            annotationlib.get_annotations(
                inspect_stringized_annotations_pep695.E, eval_str=True
            ),
            {"x": str},
        )

    def test_pep_695_generics_with_future_annotations_nested_in_function(self):
        results = inspect_stringized_annotations_pep695.nested()

        self.assertEqual(
            set(results.F_annotations.values()), set(results.F.__type_params__)
        )
        self.assertEqual(
            set(results.F_meth_annotations.values()),
            set(results.F.generic_method.__type_params__),
        )
        self.assertNotEqual(
            set(results.F_meth_annotations.values()), set(results.F.__type_params__)
        )
        self.assertEqual(
            set(results.F_meth_annotations.values()).intersection(
                results.F.__type_params__
            ),
            set(),
        )

        self.assertEqual(results.G_annotations, {"x": str})

        self.assertEqual(
            set(results.generic_func_annotations.values()),
            set(results.generic_func.__type_params__),
        )


class TestCallEvaluateFunction(unittest.TestCase):
    def test_evaluation(self):
        def evaluate(format, exc=NotImplementedError):
            if format > 2:
                raise exc
            return undefined

        with self.assertRaises(NameError):
            annotationlib.call_evaluate_function(evaluate, Format.VALUE)
        self.assertEqual(
            annotationlib.call_evaluate_function(evaluate, Format.FORWARDREF),
            support.EqualToForwardRef("undefined"),
        )
        self.assertEqual(
            annotationlib.call_evaluate_function(evaluate, Format.STRING),
            "undefined",
        )


class MetaclassTests(unittest.TestCase):
    def test_annotated_meta(self):
        class Meta(type):
            a: int

        class X(metaclass=Meta):
            pass

        class Y(metaclass=Meta):
            b: float

        self.assertEqual(get_annotations(Meta), {"a": int})
        self.assertEqual(get_annotate_function(Meta)(Format.VALUE), {"a": int})

        self.assertEqual(get_annotations(X), {})
        self.assertIs(get_annotate_function(X), None)

        self.assertEqual(get_annotations(Y), {"b": float})
        self.assertEqual(get_annotate_function(Y)(Format.VALUE), {"b": float})

    def test_unannotated_meta(self):
        class Meta(type):
            pass

        class X(metaclass=Meta):
            a: str

        class Y(X):
            pass

        self.assertEqual(get_annotations(Meta), {})
        self.assertIs(get_annotate_function(Meta), None)

        self.assertEqual(get_annotations(Y), {})
        self.assertIs(get_annotate_function(Y), None)

        self.assertEqual(get_annotations(X), {"a": str})
        self.assertEqual(get_annotate_function(X)(Format.VALUE), {"a": str})

    def test_ordering(self):
        # Based on a sample by David Ellis
        # https://discuss.python.org/t/pep-749-implementing-pep-649/54974/38

        def make_classes():
            class Meta(type):
                a: int
                expected_annotations = {"a": int}

            class A(type, metaclass=Meta):
                b: float
                expected_annotations = {"b": float}

            class B(metaclass=A):
                c: str
                expected_annotations = {"c": str}

            class C(B):
                expected_annotations = {}

            class D(metaclass=Meta):
                expected_annotations = {}

            return Meta, A, B, C, D

        classes = make_classes()
        class_count = len(classes)
        for order in itertools.permutations(range(class_count), class_count):
            names = ", ".join(classes[i].__name__ for i in order)
            with self.subTest(names=names):
                classes = make_classes()  # Regenerate classes
                for i in order:
                    get_annotations(classes[i])
                for c in classes:
                    with self.subTest(c=c):
                        self.assertEqual(get_annotations(c), c.expected_annotations)
                        annotate_func = get_annotate_function(c)
                        if c.expected_annotations:
                            self.assertEqual(
                                annotate_func(Format.VALUE), c.expected_annotations
                            )
                        else:
                            self.assertIs(annotate_func, None)


class TestGetAnnotateFunction(unittest.TestCase):
    def test_static_class(self):
        self.assertIsNone(get_annotate_function(object))
        self.assertIsNone(get_annotate_function(int))

    def test_unannotated_class(self):
        class C:
            pass

        self.assertIsNone(get_annotate_function(C))

        D = type("D", (), {})
        self.assertIsNone(get_annotate_function(D))

    def test_annotated_class(self):
        class C:
            a: int

        self.assertEqual(get_annotate_function(C)(Format.VALUE), {"a": int})


class TestToSource(unittest.TestCase):
    def test_value_to_string(self):
        self.assertEqual(value_to_string(int), "int")
        self.assertEqual(value_to_string(MyClass), "test.test_annotationlib.MyClass")
        self.assertEqual(value_to_string(len), "len")
        self.assertEqual(value_to_string(value_to_string), "value_to_string")
        self.assertEqual(value_to_string(times_three), "times_three")
        self.assertEqual(value_to_string(...), "...")
        self.assertEqual(value_to_string(None), "None")
        self.assertEqual(value_to_string(1), "1")
        self.assertEqual(value_to_string("1"), "'1'")
        self.assertEqual(value_to_string(Format.VALUE), repr(Format.VALUE))
        self.assertEqual(value_to_string(MyClass()), "my repr")

    def test_annotations_to_string(self):
        self.assertEqual(annotations_to_string({}), {})
        self.assertEqual(annotations_to_string({"x": int}), {"x": "int"})
        self.assertEqual(annotations_to_string({"x": "int"}), {"x": "int"})
        self.assertEqual(
            annotations_to_string({"x": int, "y": str}), {"x": "int", "y": "str"}
        )


class TestAnnotationLib(unittest.TestCase):
    def test__all__(self):
        support.check__all__(self, annotationlib)
