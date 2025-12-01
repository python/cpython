"""Tests for the annotations module."""

import textwrap
import annotationlib
import builtins
import collections
import functools
import itertools
import pickle
from string.templatelib import Template, Interpolation
import typing
import sys
import unittest
from annotationlib import (
    Format,
    ForwardRef,
    get_annotations,
    annotations_to_string,
    type_repr,
)
from typing import Unpack, get_type_hints, List, Union

from test import support
from test.support import import_helper
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

        anno = get_annotations(inner, format=Format.FORWARDREF)
        fwdref = anno["arg"]
        self.assertIsInstance(fwdref, ForwardRef)
        self.assertEqual(fwdref.__forward_arg__, "x")
        with self.assertRaises(NameError):
            fwdref.evaluate()

        x = 1
        self.assertEqual(fwdref.evaluate(), x)

        anno = get_annotations(inner, format=Format.FORWARDREF)
        self.assertEqual(anno["arg"], x)

    def test_multiple_closure(self):
        def inner(arg: x[y]):
            pass

        fwdref = get_annotations(inner, format=Format.FORWARDREF)["arg"]
        self.assertIsInstance(fwdref, ForwardRef)
        self.assertEqual(fwdref.__forward_arg__, "x[y]")
        with self.assertRaises(NameError):
            fwdref.evaluate()

        y = str
        fwdref = get_annotations(inner, format=Format.FORWARDREF)["arg"]
        self.assertIsInstance(fwdref, ForwardRef)
        extra_name, extra_val = next(iter(fwdref.__extra_names__.items()))
        self.assertEqual(fwdref.__forward_arg__.replace(extra_name, extra_val.__name__), "x[str]")
        with self.assertRaises(NameError):
            fwdref.evaluate()

        x = list
        self.assertEqual(fwdref.evaluate(), x[y])

        fwdref = get_annotations(inner, format=Format.FORWARDREF)["arg"]
        self.assertEqual(fwdref, x[y])

    def test_function(self):
        def f(x: int, y: doesntexist):
            pass

        anno = get_annotations(f, format=Format.FORWARDREF)
        self.assertIs(anno["x"], int)
        fwdref = anno["y"]
        self.assertIsInstance(fwdref, ForwardRef)
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
            delta: some | {obj: module},
            epsilon: some | {obj, module},
            zeta: some | [obj],
            eta: some | (),
        ):
            pass

        anno = get_annotations(f, format=Format.FORWARDREF)
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

        delta_anno = anno["delta"]
        self.assertIsInstance(delta_anno, ForwardRef)
        self.assertEqual(delta_anno, support.EqualToForwardRef("some | {obj: module}", owner=f))

        epsilon_anno = anno["epsilon"]
        self.assertIsInstance(epsilon_anno, ForwardRef)
        self.assertEqual(epsilon_anno, support.EqualToForwardRef("some | {obj, module}", owner=f))

        zeta_anno = anno["zeta"]
        self.assertIsInstance(zeta_anno, ForwardRef)
        self.assertEqual(zeta_anno, support.EqualToForwardRef("some | [obj]", owner=f))

        eta_anno = anno["eta"]
        self.assertIsInstance(eta_anno, ForwardRef)
        self.assertEqual(eta_anno, support.EqualToForwardRef("some | ()", owner=f))

    def test_partially_nonexistent(self):
        # These annotations start with a non-existent variable and then use
        # global types with defined values. This partially evaluates by putting
        # those globals into `fwdref.__extra_names__`.
        def f(
            x: obj | int,
            y: container[int:obj, int],
            z: dict_val | {str: int},
            alpha: set_val | {str, int},
            beta: obj | bool | int,
            gamma: obj | call_func(int, kwd=bool),
        ):
            pass

        def func(*args, **kwargs):
            return Union[*args, *(kwargs.values())]

        anno = get_annotations(f, format=Format.FORWARDREF)
        globals_ = {
            "obj": str, "container": list, "dict_val": {1: 2}, "set_val": {1, 2},
            "call_func": func
        }

        x_anno = anno["x"]
        self.assertIsInstance(x_anno, ForwardRef)
        self.assertEqual(x_anno.evaluate(globals=globals_), str | int)

        y_anno = anno["y"]
        self.assertIsInstance(y_anno, ForwardRef)
        self.assertEqual(y_anno.evaluate(globals=globals_), list[int:str, int])

        z_anno = anno["z"]
        self.assertIsInstance(z_anno, ForwardRef)
        self.assertEqual(z_anno.evaluate(globals=globals_), {1: 2} | {str: int})

        alpha_anno = anno["alpha"]
        self.assertIsInstance(alpha_anno, ForwardRef)
        self.assertEqual(alpha_anno.evaluate(globals=globals_), {1, 2} | {str, int})

        beta_anno = anno["beta"]
        self.assertIsInstance(beta_anno, ForwardRef)
        self.assertEqual(beta_anno.evaluate(globals=globals_), str | bool | int)

        gamma_anno = anno["gamma"]
        self.assertIsInstance(gamma_anno, ForwardRef)
        self.assertEqual(gamma_anno.evaluate(globals=globals_), str | func(int, kwd=bool))

    def test_partially_nonexistent_union(self):
        # Test unions with '|' syntax equal unions with typing.Union[] with some forwardrefs
        class UnionForwardrefs:
            pipe: str | undefined
            union: Union[str, undefined]

        annos = get_annotations(UnionForwardrefs, format=Format.FORWARDREF)

        pipe = annos["pipe"]
        self.assertIsInstance(pipe, ForwardRef)
        self.assertEqual(
            pipe.evaluate(globals={"undefined": int}),
            str | int,
        )
        union = annos["union"]
        self.assertIsInstance(union, Union)
        arg1, arg2 = typing.get_args(union)
        self.assertIs(arg1, str)
        self.assertEqual(
            arg2, support.EqualToForwardRef("undefined", is_class=True, owner=UnionForwardrefs)
        )


class TestStringFormat(unittest.TestCase):
    def test_closure(self):
        x = 0

        def inner(arg: x):
            pass

        anno = get_annotations(inner, format=Format.STRING)
        self.assertEqual(anno, {"arg": "x"})

    def test_closure_undefined(self):
        if False:
            x = 0

        def inner(arg: x):
            pass

        anno = get_annotations(inner, format=Format.STRING)
        self.assertEqual(anno, {"arg": "x"})

    def test_function(self):
        def f(x: int, y: doesntexist):
            pass

        anno = get_annotations(f, format=Format.STRING)
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

        anno = get_annotations(f, format=Format.STRING)
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

        anno = get_annotations(f, format=Format.STRING)
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

    def test_template_str(self):
        def f(
            x: t"{a}",
            y: list[t"{a}"],
            z: t"{a:b} {c!r} {d!s:t}",
            a: t"a{b}c{d}e{f}g",
            b: t"{a:{1}}",
            c: t"{a | b * c}",
            gh138558: t"{ 0}",
        ): pass

        annos = get_annotations(f, format=Format.STRING)
        self.assertEqual(annos, {
            "x": "t'{a}'",
            "y": "list[t'{a}']",
            "z": "t'{a:b} {c!r} {d!s:t}'",
            "a": "t'a{b}c{d}e{f}g'",
            # interpolations in the format spec are eagerly evaluated so we can't recover the source
            "b": "t'{a:1}'",
            "c": "t'{a | b * c}'",
            "gh138558": "t'{ 0}'",
        })

        def g(
            x: t"{a}",
        ): ...

        annos = get_annotations(g, format=Format.FORWARDREF)
        templ = annos["x"]
        # Template and Interpolation don't have __eq__ so we have to compare manually
        self.assertIsInstance(templ, Template)
        self.assertEqual(templ.strings, ("", ""))
        self.assertEqual(len(templ.interpolations), 1)
        interp = templ.interpolations[0]
        self.assertEqual(interp.value, support.EqualToForwardRef("a", owner=g))
        self.assertEqual(interp.expression, "a")
        self.assertIsNone(interp.conversion)
        self.assertEqual(interp.format_spec, "")

    def test_getitem(self):
        def f(x: undef1[str, undef2]):
            pass
        anno = get_annotations(f, format=Format.STRING)
        self.assertEqual(anno, {"x": "undef1[str, undef2]"})

        anno = get_annotations(f, format=Format.FORWARDREF)
        fwdref = anno["x"]
        self.assertIsInstance(fwdref, ForwardRef)
        self.assertEqual(
            fwdref.evaluate(globals={"undef1": dict, "undef2": float}), dict[str, float]
        )

    def test_slice(self):
        def f(x: a[b:c]):
            pass
        anno = get_annotations(f, format=Format.STRING)
        self.assertEqual(anno, {"x": "a[b:c]"})

        def f(x: a[b:c, d:e]):
            pass
        anno = get_annotations(f, format=Format.STRING)
        self.assertEqual(anno, {"x": "a[b:c, d:e]"})

        obj = slice(1, 1, 1)
        def f(x: obj):
            pass
        anno = get_annotations(f, format=Format.STRING)
        self.assertEqual(anno, {"x": "obj"})

    def test_literals(self):
        def f(
            a: 1,
            b: 1.0,
            c: "hello",
            d: b"hello",
            e: True,
            f: None,
            g: ...,
            h: 1j,
        ):
            pass

        anno = get_annotations(f, format=Format.STRING)
        self.assertEqual(
            anno,
            {
                "a": "1",
                "b": "1.0",
                "c": 'hello',
                "d": "b'hello'",
                "e": "True",
                "f": "None",
                "g": "...",
                "h": "1j",
            },
        )

    def test_displays(self):
        # Simple case first
        def f(x: a[[int, str], float]):
            pass
        anno = get_annotations(f, format=Format.STRING)
        self.assertEqual(anno, {"x": "a[[int, str], float]"})

        def g(
            w: a[[int, str], float],
            x: a[{int}, 3],
            y: a[{int: str}, 4],
            z: a[(int, str), 5],
        ):
            pass
        anno = get_annotations(g, format=Format.STRING)
        self.assertEqual(
            anno,
            {
                "w": "a[[int, str], float]",
                "x": "a[{int}, 3]",
                "y": "a[{int: str}, 4]",
                "z": "a[(int, str), 5]",
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

        anno = get_annotations(f, format=Format.STRING)
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
            get_annotations(f, format=Format.STRING)

        def f(fstring_format: f"{a:02d}"):
            pass

        with self.assertRaisesRegex(TypeError, format_msg):
            get_annotations(f, format=Format.STRING)

    def test_shenanigans(self):
        # In cases like this we can't reconstruct the source; test that we do something
        # halfway reasonable.
        def f(x: x | (1).__class__, y: (1).__class__):
            pass

        self.assertEqual(
            get_annotations(f, format=Format.STRING),
            {"x": "x | <class 'int'>", "y": "<class 'int'>"},
        )


class TestGetAnnotations(unittest.TestCase):
    def test_builtin_type(self):
        self.assertEqual(get_annotations(int), {})
        self.assertEqual(get_annotations(object), {})

    def test_custom_metaclass(self):
        class Meta(type):
            pass

        class C(metaclass=Meta):
            x: int

        self.assertEqual(get_annotations(C), {"x": int})

    def test_missing_dunder_dict(self):
        class NoDict(type):
            @property
            def __dict__(cls):
                raise AttributeError

            b: str

        class C1(metaclass=NoDict):
            a: int

        self.assertEqual(get_annotations(C1), {"a": int})
        self.assertEqual(
            get_annotations(C1, format=Format.FORWARDREF),
            {"a": int},
        )
        self.assertEqual(
            get_annotations(C1, format=Format.STRING),
            {"a": "int"},
        )
        self.assertEqual(get_annotations(NoDict), {"b": str})
        self.assertEqual(
            get_annotations(NoDict, format=Format.FORWARDREF),
            {"b": str},
        )
        self.assertEqual(
            get_annotations(NoDict, format=Format.STRING),
            {"b": "str"},
        )

    def test_format(self):
        def f1(a: int):
            pass

        def f2(a: undefined):
            pass

        self.assertEqual(
            get_annotations(f1, format=Format.VALUE),
            {"a": int},
        )
        self.assertEqual(get_annotations(f1, format=1), {"a": int})

        fwd = support.EqualToForwardRef("undefined", owner=f2)
        self.assertEqual(
            get_annotations(f2, format=Format.FORWARDREF),
            {"a": fwd},
        )
        self.assertEqual(get_annotations(f2, format=3), {"a": fwd})

        self.assertEqual(
            get_annotations(f1, format=Format.STRING),
            {"a": "int"},
        )
        self.assertEqual(get_annotations(f1, format=4), {"a": "int"})

        with self.assertRaises(ValueError):
            get_annotations(f1, format=42)

        with self.assertRaisesRegex(
            ValueError,
            r"The VALUE_WITH_FAKE_GLOBALS format is for internal use only",
        ):
            get_annotations(f1, format=Format.VALUE_WITH_FAKE_GLOBALS)

        with self.assertRaisesRegex(
            ValueError,
            r"The VALUE_WITH_FAKE_GLOBALS format is for internal use only",
        ):
            get_annotations(f1, format=2)

    def test_custom_object_with_annotations(self):
        class C:
            def __init__(self):
                self.__annotations__ = {"x": int, "y": str}

        self.assertEqual(get_annotations(C()), {"x": int, "y": str})

    def test_custom_format_eval_str(self):
        def foo():
            pass

        with self.assertRaises(ValueError):
            get_annotations(foo, format=Format.FORWARDREF, eval_str=True)
            get_annotations(foo, format=Format.STRING, eval_str=True)

    def test_stock_annotations(self):
        def foo(a: int, b: str):
            pass

        for format in (Format.VALUE, Format.FORWARDREF):
            with self.subTest(format=format):
                self.assertEqual(
                    get_annotations(foo, format=format),
                    {"a": int, "b": str},
                )
        self.assertEqual(
            get_annotations(foo, format=Format.STRING),
            {"a": "int", "b": "str"},
        )

        foo.__annotations__ = {"a": "foo", "b": "str"}
        for format in Format:
            if format == Format.VALUE_WITH_FAKE_GLOBALS:
                continue
            with self.subTest(format=format):
                self.assertEqual(
                    get_annotations(foo, format=format),
                    {"a": "foo", "b": "str"},
                )

        self.assertEqual(
            get_annotations(foo, eval_str=True, locals=locals()),
            {"a": foo, "b": str},
        )
        self.assertEqual(
            get_annotations(foo, eval_str=True, globals=locals()),
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
                self.assertEqual(get_annotations(isa, **kwargs), {"a": int, "b": str})
                self.assertEqual(
                    get_annotations(isa.MyClass, **kwargs),
                    {"a": int, "b": str},
                )
                self.assertEqual(
                    get_annotations(isa.function, **kwargs),
                    {"a": int, "b": str, "return": isa.MyClass},
                )
                self.assertEqual(
                    get_annotations(isa.function2, **kwargs),
                    {"a": int, "b": "str", "c": isa.MyClass, "return": isa.MyClass},
                )
                self.assertEqual(
                    get_annotations(isa.function3, **kwargs),
                    {"a": "int", "b": "str", "c": "MyClass"},
                )
                self.assertEqual(
                    get_annotations(annotationlib, **kwargs), {}
                )  # annotations module has no annotations
                self.assertEqual(get_annotations(isa.UnannotatedClass, **kwargs), {})
                self.assertEqual(
                    get_annotations(isa.unannotated_function, **kwargs),
                    {},
                )

        for kwargs in [
            {"eval_str": True},
            {"format": Format.VALUE, "eval_str": True},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(get_annotations(isa, **kwargs), {"a": int, "b": str})
                self.assertEqual(
                    get_annotations(isa.MyClass, **kwargs),
                    {"a": int, "b": str},
                )
                self.assertEqual(
                    get_annotations(isa.function, **kwargs),
                    {"a": int, "b": str, "return": isa.MyClass},
                )
                self.assertEqual(
                    get_annotations(isa.function2, **kwargs),
                    {"a": int, "b": str, "c": isa.MyClass, "return": isa.MyClass},
                )
                self.assertEqual(
                    get_annotations(isa.function3, **kwargs),
                    {"a": int, "b": str, "c": isa.MyClass},
                )
                self.assertEqual(get_annotations(annotationlib, **kwargs), {})
                self.assertEqual(get_annotations(isa.UnannotatedClass, **kwargs), {})
                self.assertEqual(
                    get_annotations(isa.unannotated_function, **kwargs),
                    {},
                )

        self.assertEqual(
            get_annotations(isa, format=Format.STRING),
            {"a": "int", "b": "str"},
        )
        self.assertEqual(
            get_annotations(isa.MyClass, format=Format.STRING),
            {"a": "int", "b": "str"},
        )
        self.assertEqual(
            get_annotations(isa.function, format=Format.STRING),
            {"a": "int", "b": "str", "return": "MyClass"},
        )
        self.assertEqual(
            get_annotations(isa.function2, format=Format.STRING),
            {"a": "int", "b": "str", "c": "MyClass", "return": "MyClass"},
        )
        self.assertEqual(
            get_annotations(isa.function3, format=Format.STRING),
            {"a": "int", "b": "str", "c": "MyClass"},
        )
        self.assertEqual(
            get_annotations(annotationlib, format=Format.STRING),
            {},
        )
        self.assertEqual(
            get_annotations(isa.UnannotatedClass, format=Format.STRING),
            {},
        )
        self.assertEqual(
            get_annotations(isa.unannotated_function, format=Format.STRING),
            {},
        )

    def test_stock_annotations_on_wrapper(self):
        isa = inspect_stock_annotations

        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, "x"), isa.MyClass(3, "xxx"))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(
            get_annotations(wrapped),
            {"a": int, "b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            get_annotations(wrapped, format=Format.FORWARDREF),
            {"a": int, "b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            get_annotations(wrapped, format=Format.STRING),
            {"a": "int", "b": "str", "return": "MyClass"},
        )
        self.assertEqual(
            get_annotations(wrapped, eval_str=True),
            {"a": int, "b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            get_annotations(wrapped, eval_str=False),
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
                    get_annotations(isa, **kwargs),
                    {"a": "int", "b": "str"},
                )
                self.assertEqual(
                    get_annotations(isa.MyClass, **kwargs),
                    {"a": "int", "b": "str"},
                )
                self.assertEqual(
                    get_annotations(isa.function, **kwargs),
                    {"a": "int", "b": "str", "return": "MyClass"},
                )
                self.assertEqual(
                    get_annotations(isa.function2, **kwargs),
                    {"a": "int", "b": "'str'", "c": "MyClass", "return": "MyClass"},
                )
                self.assertEqual(
                    get_annotations(isa.function3, **kwargs),
                    {"a": "'int'", "b": "'str'", "c": "'MyClass'"},
                )
                self.assertEqual(get_annotations(isa.UnannotatedClass, **kwargs), {})
                self.assertEqual(
                    get_annotations(isa.unannotated_function, **kwargs),
                    {},
                )

        for kwargs in [
            {"eval_str": True},
            {"eval_str": True, "globals": isa.__dict__, "locals": {}},
            {"eval_str": True, "globals": {}, "locals": isa.__dict__},
            {"format": Format.VALUE, "eval_str": True},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(get_annotations(isa, **kwargs), {"a": int, "b": str})
                self.assertEqual(
                    get_annotations(isa.MyClass, **kwargs),
                    {"a": int, "b": str},
                )
                self.assertEqual(
                    get_annotations(isa.function, **kwargs),
                    {"a": int, "b": str, "return": isa.MyClass},
                )
                self.assertEqual(
                    get_annotations(isa.function2, **kwargs),
                    {"a": int, "b": "str", "c": isa.MyClass, "return": isa.MyClass},
                )
                self.assertEqual(
                    get_annotations(isa.function3, **kwargs),
                    {"a": "int", "b": "str", "c": "MyClass"},
                )
                self.assertEqual(get_annotations(isa.UnannotatedClass, **kwargs), {})
                self.assertEqual(
                    get_annotations(isa.unannotated_function, **kwargs),
                    {},
                )

    def test_stringized_annotations_in_empty_module(self):
        isa2 = inspect_stringized_annotations_2
        self.assertEqual(get_annotations(isa2), {})
        self.assertEqual(get_annotations(isa2, eval_str=True), {})
        self.assertEqual(get_annotations(isa2, eval_str=False), {})

    def test_stringized_annotations_with_star_unpack(self):
        def f(*args: "*tuple[int, ...]"): ...
        self.assertEqual(get_annotations(f, eval_str=True),
                         {'args': (*tuple[int, ...],)[0]})


    def test_stringized_annotations_on_wrapper(self):
        isa = inspect_stringized_annotations
        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, "x"), isa.MyClass(3, "xxx"))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(
            get_annotations(wrapped),
            {"a": "int", "b": "str", "return": "MyClass"},
        )
        self.assertEqual(
            get_annotations(wrapped, eval_str=True),
            {"a": int, "b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            get_annotations(wrapped, eval_str=False),
            {"a": "int", "b": "str", "return": "MyClass"},
        )

    def test_stringized_annotations_on_partial_wrapper(self):
        isa = inspect_stringized_annotations

        def times_three_str(fn: typing.Callable[[str], isa.MyClass]):
            @functools.wraps(fn)
            def wrapper(b: "str") -> "MyClass":
                return fn(b * 3)

            return wrapper

        wrapped = times_three_str(functools.partial(isa.function, 1))
        self.assertEqual(wrapped("x"), isa.MyClass(1, "xxx"))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(
            get_annotations(wrapped, eval_str=True),
            {"b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            get_annotations(wrapped, eval_str=False),
            {"b": "str", "return": "MyClass"},
        )

        # If functools is not loaded, names will be evaluated in the current
        # module instead of being unwrapped to the original.
        functools_mod = sys.modules["functools"]
        del sys.modules["functools"]

        self.assertEqual(
            get_annotations(wrapped, eval_str=True),
            {"b": str, "return": MyClass},
        )
        self.assertEqual(
            get_annotations(wrapped, eval_str=False),
            {"b": "str", "return": "MyClass"},
        )

        sys.modules["functools"] = functools_mod

    def test_stringized_annotations_on_class(self):
        isa = inspect_stringized_annotations
        # test that local namespace lookups work
        self.assertEqual(
            get_annotations(isa.MyClassWithLocalAnnotations),
            {"x": "mytype"},
        )
        self.assertEqual(
            get_annotations(isa.MyClassWithLocalAnnotations, eval_str=True),
            {"x": int},
        )

    def test_stringized_annotations_on_custom_object(self):
        class HasAnnotations:
            @property
            def __annotations__(self):
                return {"x": "int"}

        ha = HasAnnotations()
        self.assertEqual(get_annotations(ha), {"x": "int"})
        self.assertEqual(get_annotations(ha, eval_str=True), {"x": int})

    def test_stringized_annotation_permutations(self):
        def define_class(name, has_future, has_annos, base_text, extra_names=None):
            lines = []
            if has_future:
                lines.append("from __future__ import annotations")
            lines.append(f"class {name}({base_text}):")
            if has_annos:
                lines.append(f"    {name}_attr: int")
            else:
                lines.append("    pass")
            code = "\n".join(lines)
            ns = support.run_code(code, extra_names=extra_names)
            return ns[name]

        def check_annotations(cls, has_future, has_annos):
            if has_annos:
                if has_future:
                    anno = "int"
                else:
                    anno = int
                self.assertEqual(get_annotations(cls), {f"{cls.__name__}_attr": anno})
            else:
                self.assertEqual(get_annotations(cls), {})

        for meta_future, base_future, child_future, meta_has_annos, base_has_annos, child_has_annos in itertools.product(
            (False, True),
            (False, True),
            (False, True),
            (False, True),
            (False, True),
            (False, True),
        ):
            with self.subTest(
                meta_future=meta_future,
                base_future=base_future,
                child_future=child_future,
                meta_has_annos=meta_has_annos,
                base_has_annos=base_has_annos,
                child_has_annos=child_has_annos,
            ):
                meta = define_class(
                    "Meta",
                    has_future=meta_future,
                    has_annos=meta_has_annos,
                    base_text="type",
                )
                base = define_class(
                    "Base",
                    has_future=base_future,
                    has_annos=base_has_annos,
                    base_text="metaclass=Meta",
                    extra_names={"Meta": meta},
                )
                child = define_class(
                    "Child",
                    has_future=child_future,
                    has_annos=child_has_annos,
                    base_text="Base",
                    extra_names={"Base": base},
                )
                check_annotations(meta, meta_future, meta_has_annos)
                check_annotations(base, base_future, base_has_annos)
                check_annotations(child, child_future, child_has_annos)

    def test_modify_annotations(self):
        def f(x: int):
            pass

        self.assertEqual(get_annotations(f), {"x": int})
        self.assertEqual(
            get_annotations(f, format=Format.FORWARDREF),
            {"x": int},
        )

        f.__annotations__["x"] = str
        # The modification is reflected in VALUE (the default)
        self.assertEqual(get_annotations(f), {"x": str})
        # ... and also in FORWARDREF, which tries __annotations__ if available
        self.assertEqual(
            get_annotations(f, format=Format.FORWARDREF),
            {"x": str},
        )
        # ... but not in STRING which always uses __annotate__
        self.assertEqual(
            get_annotations(f, format=Format.STRING),
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
                get_annotations(wa, format=format)

    def test_annotations_on_custom_object(self):
        class HasAnnotations:
            @property
            def __annotations__(self):
                return {"x": int}

        ha = HasAnnotations()
        self.assertEqual(get_annotations(ha, format=Format.VALUE), {"x": int})
        self.assertEqual(get_annotations(ha, format=Format.FORWARDREF), {"x": int})

        self.assertEqual(get_annotations(ha, format=Format.STRING), {"x": "int"})

    def test_raising_annotations_on_custom_object(self):
        class HasRaisingAnnotations:
            @property
            def __annotations__(self):
                return {"x": undefined}

        hra = HasRaisingAnnotations()

        with self.assertRaises(NameError):
            get_annotations(hra, format=Format.VALUE)

        with self.assertRaises(NameError):
            get_annotations(hra, format=Format.FORWARDREF)

        undefined = float
        self.assertEqual(get_annotations(hra, format=Format.VALUE), {"x": float})

    def test_forwardref_prefers_annotations(self):
        class HasBoth:
            @property
            def __annotations__(self):
                return {"x": int}

            @property
            def __annotate__(self):
                return lambda format: {"x": str}

        hb = HasBoth()
        self.assertEqual(get_annotations(hb, format=Format.VALUE), {"x": int})
        self.assertEqual(get_annotations(hb, format=Format.FORWARDREF), {"x": int})
        self.assertEqual(get_annotations(hb, format=Format.STRING), {"x": str})

    def test_only_annotate(self):
        def f(x: int):
            pass

        class OnlyAnnotate:
            @property
            def __annotate__(self):
                return f.__annotate__

        oa = OnlyAnnotate()
        self.assertEqual(get_annotations(oa, format=Format.VALUE), {"x": int})
        self.assertEqual(get_annotations(oa, format=Format.FORWARDREF), {"x": int})
        self.assertEqual(
            get_annotations(oa, format=Format.STRING),
            {"x": "int"},
        )

    def test_non_dict_annotate(self):
        class WeirdAnnotate:
            def __annotate__(self, *args, **kwargs):
                return "not a dict"

        wa = WeirdAnnotate()
        for format in Format:
            if format == Format.VALUE_WITH_FAKE_GLOBALS:
                continue
            with (
                self.subTest(format=format),
                self.assertRaisesRegex(
                    ValueError, r".*__annotate__ returned a non-dict"
                ),
            ):
                get_annotations(wa, format=format)

    def test_no_annotations(self):
        class CustomClass:
            pass

        class MyCallable:
            def __call__(self):
                pass

        for format in Format:
            if format == Format.VALUE_WITH_FAKE_GLOBALS:
                continue
            for obj in (None, 1, object(), CustomClass()):
                with self.subTest(format=format, obj=obj):
                    with self.assertRaises(TypeError):
                        get_annotations(obj, format=format)

            # Callables and types with no annotations return an empty dict
            for obj in (int, len, MyCallable()):
                with self.subTest(format=format, obj=obj):
                    self.assertEqual(get_annotations(obj, format=format), {})

    def test_pep695_generic_class_with_future_annotations(self):
        ann_module695 = inspect_stringized_annotations_pep695
        A_annotations = get_annotations(ann_module695.A, eval_str=True)
        A_type_params = ann_module695.A.__type_params__
        self.assertIs(A_annotations["x"], A_type_params[0])
        self.assertEqual(A_annotations["y"].__args__[0], Unpack[A_type_params[1]])
        self.assertIs(A_annotations["z"].__args__[0], A_type_params[2])

    def test_pep695_generic_class_with_future_annotations_and_local_shadowing(self):
        B_annotations = get_annotations(
            inspect_stringized_annotations_pep695.B, eval_str=True
        )
        self.assertEqual(B_annotations, {"x": int, "y": str, "z": bytes})

    def test_pep695_generic_class_with_future_annotations_name_clash_with_global_vars(
        self,
    ):
        ann_module695 = inspect_stringized_annotations_pep695
        C_annotations = get_annotations(ann_module695.C, eval_str=True)
        self.assertEqual(
            set(C_annotations.values()), set(ann_module695.C.__type_params__)
        )

    def test_pep_695_generic_function_with_future_annotations(self):
        ann_module695 = inspect_stringized_annotations_pep695
        generic_func_annotations = get_annotations(
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
                get_annotations(
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
        generic_method_annotations = get_annotations(
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
                get_annotations(
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
            get_annotations(inspect_stringized_annotations_pep695.E, eval_str=True),
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

    def test_partial_evaluation(self):
        def f(
            x: builtins.undef,
            y: list[int],
            z: 1 + int,
            a: builtins.int,
            b: [builtins.undef, builtins.int],
        ):
            pass

        self.assertEqual(
            get_annotations(f, format=Format.FORWARDREF),
            {
                "x": support.EqualToForwardRef("builtins.undef", owner=f),
                "y": list[int],
                "z": support.EqualToForwardRef("1 + int", owner=f),
                "a": int,
                "b": [
                    support.EqualToForwardRef("builtins.undef", owner=f),
                    # We can't resolve this because we have to evaluate the whole annotation
                    support.EqualToForwardRef("builtins.int", owner=f),
                ],
            },
        )

        self.assertEqual(
            get_annotations(f, format=Format.STRING),
            {
                "x": "builtins.undef",
                "y": "list[int]",
                "z": "1 + int",
                "a": "builtins.int",
                "b": "[builtins.undef, builtins.int]",
            },
        )

    def test_partial_evaluation_error(self):
        def f(x: range[1]):
            pass
        with self.assertRaisesRegex(
            TypeError, "type 'range' is not subscriptable"
        ):
            f.__annotations__

        self.assertEqual(
            get_annotations(f, format=Format.FORWARDREF),
            {
                "x": support.EqualToForwardRef("range[1]", owner=f),
            },
        )

    def test_partial_evaluation_cell(self):
        obj = object()

        class RaisesAttributeError:
            attriberr: obj.missing

        anno = get_annotations(RaisesAttributeError, format=Format.FORWARDREF)
        self.assertEqual(
            anno,
            {
                "attriberr": support.EqualToForwardRef(
                    "obj.missing", is_class=True, owner=RaisesAttributeError
                )
            },
        )

    def test_nonlocal_in_annotation_scope(self):
        class Demo:
            nonlocal sequence_b
            x: sequence_b
            y: sequence_b[int]

        fwdrefs = get_annotations(Demo, format=Format.FORWARDREF)

        self.assertIsInstance(fwdrefs["x"], ForwardRef)
        self.assertIsInstance(fwdrefs["y"], ForwardRef)

        sequence_b = list
        self.assertIs(fwdrefs["x"].evaluate(), list)
        self.assertEqual(fwdrefs["y"].evaluate(), list[int])

    def test_raises_error_from_value(self):
        # test that if VALUE is the only supported format, but raises an error
        # that error is propagated from get_annotations
        class DemoException(Exception): ...

        def annotate(format, /):
            if format == Format.VALUE:
                raise DemoException()
            else:
                raise NotImplementedError(format)

        def f(): ...

        f.__annotate__ = annotate

        for fmt in [Format.VALUE, Format.FORWARDREF, Format.STRING]:
            with self.assertRaises(DemoException):
                get_annotations(f, format=fmt)


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

    def test_fake_global_evaluation(self):
        # This will raise an AttributeError
        def evaluate_union(format, exc=NotImplementedError):
            if format == Format.VALUE_WITH_FAKE_GLOBALS:
                # Return a ForwardRef
                return builtins.undefined | list[int]
            raise exc

        self.assertEqual(
            annotationlib.call_evaluate_function(evaluate_union, Format.FORWARDREF),
            support.EqualToForwardRef("builtins.undefined | list[int]"),
        )

        # This will raise an AttributeError
        def evaluate_intermediate(format, exc=NotImplementedError):
            if format == Format.VALUE_WITH_FAKE_GLOBALS:
                intermediate = builtins.undefined
                # Return a literal
                return intermediate is None
            raise exc

        self.assertIs(
            annotationlib.call_evaluate_function(evaluate_intermediate, Format.FORWARDREF),
            False,
        )


class TestCallAnnotateFunction(unittest.TestCase):
    # Tests for user defined annotate functions.

    # Format and NotImplementedError are provided as arguments so they exist in
    # the fake globals namespace.
    # This avoids non-matching conditions passing by being converted to stringifiers.
    # See: https://github.com/python/cpython/issues/138764

    def test_user_annotate_value(self):
        def annotate(format, /):
            if format == Format.VALUE:
                return {"x": str}
            else:
                raise NotImplementedError(format)

        annotations = annotationlib.call_annotate_function(
            annotate,
            Format.VALUE,
        )

        self.assertEqual(annotations, {"x": str})

    def test_user_annotate_forwardref_supported(self):
        # If Format.FORWARDREF is supported prefer it over Format.VALUE
        def annotate(format, /, __Format=Format, __NotImplementedError=NotImplementedError):
            if format == __Format.VALUE:
                return {'x': str}
            elif format == __Format.VALUE_WITH_FAKE_GLOBALS:
                return {'x': int}
            elif format == __Format.FORWARDREF:
                return {'x': float}
            else:
                raise __NotImplementedError(format)

        annotations = annotationlib.call_annotate_function(
            annotate,
            Format.FORWARDREF
        )

        self.assertEqual(annotations, {"x": float})

    def test_user_annotate_forwardref_fakeglobals(self):
        # If Format.FORWARDREF is not supported, use Format.VALUE_WITH_FAKE_GLOBALS
        # before falling back to Format.VALUE
        def annotate(format, /, __Format=Format, __NotImplementedError=NotImplementedError):
            if format == __Format.VALUE:
                return {'x': str}
            elif format == __Format.VALUE_WITH_FAKE_GLOBALS:
                return {'x': int}
            else:
                raise __NotImplementedError(format)

        annotations = annotationlib.call_annotate_function(
            annotate,
            Format.FORWARDREF
        )

        self.assertEqual(annotations, {"x": int})

    def test_user_annotate_forwardref_value_fallback(self):
        # If Format.FORWARDREF and Format.VALUE_WITH_FAKE_GLOBALS are not supported
        # use Format.VALUE
        def annotate(format, /, __Format=Format, __NotImplementedError=NotImplementedError):
            if format == __Format.VALUE:
                return {"x": str}
            else:
                raise __NotImplementedError(format)

        annotations = annotationlib.call_annotate_function(
            annotate,
            Format.FORWARDREF,
        )

        self.assertEqual(annotations, {"x": str})

    def test_user_annotate_string_supported(self):
        # If Format.STRING is supported prefer it over Format.VALUE
        def annotate(format, /, __Format=Format, __NotImplementedError=NotImplementedError):
            if format == __Format.VALUE:
                return {'x': str}
            elif format == __Format.VALUE_WITH_FAKE_GLOBALS:
                return {'x': int}
            elif format == __Format.STRING:
                return {'x': "float"}
            else:
                raise __NotImplementedError(format)

        annotations = annotationlib.call_annotate_function(
            annotate,
            Format.STRING,
        )

        self.assertEqual(annotations, {"x": "float"})

    def test_user_annotate_string_fakeglobals(self):
        # If Format.STRING is not supported but Format.VALUE_WITH_FAKE_GLOBALS is
        # prefer that over Format.VALUE
        def annotate(format, /, __Format=Format, __NotImplementedError=NotImplementedError):
            if format == __Format.VALUE:
                return {'x': str}
            elif format == __Format.VALUE_WITH_FAKE_GLOBALS:
                return {'x': int}
            else:
                raise __NotImplementedError(format)

        annotations = annotationlib.call_annotate_function(
            annotate,
            Format.STRING,
        )

        self.assertEqual(annotations, {"x": "int"})

    def test_user_annotate_string_value_fallback(self):
        # If Format.STRING and Format.VALUE_WITH_FAKE_GLOBALS are not
        # supported fall back to Format.VALUE and convert to strings
        def annotate(format, /, __Format=Format, __NotImplementedError=NotImplementedError):
            if format == __Format.VALUE:
                return {"x": str}
            else:
                raise __NotImplementedError(format)

        annotations = annotationlib.call_annotate_function(
            annotate,
            Format.STRING,
        )

        self.assertEqual(annotations, {"x": "str"})

    def test_condition_not_stringified(self):
        # Make sure the first condition isn't evaluated as True by being converted
        # to a _Stringifier
        def annotate(format, /):
            if format == Format.FORWARDREF:
                return {"x": str}
            else:
                raise NotImplementedError(format)

        with self.assertRaises(NotImplementedError):
            annotationlib.call_annotate_function(annotate, Format.STRING)

    def test_unsupported_formats(self):
        def annotate(format, /):
            if format == Format.FORWARDREF:
                return {"x": str}
            else:
                raise NotImplementedError(format)

        with self.assertRaises(ValueError):
            annotationlib.call_annotate_function(annotate, Format.VALUE_WITH_FAKE_GLOBALS)

        with self.assertRaises(RuntimeError):
            annotationlib.call_annotate_function(annotate, Format.VALUE)

        with self.assertRaises(ValueError):
            # Some non-Format value
            annotationlib.call_annotate_function(annotate, 7)

    def test_error_from_value_raised(self):
        # Test that the error from format.VALUE is raised
        # if all formats fail

        class DemoException(Exception): ...

        def annotate(format, /):
            if format == Format.VALUE:
                raise DemoException()
            else:
                raise NotImplementedError(format)

        for fmt in [Format.VALUE, Format.FORWARDREF, Format.STRING]:
            with self.assertRaises(DemoException):
                annotationlib.call_annotate_function(annotate, format=fmt)


class MetaclassTests(unittest.TestCase):
    def test_annotated_meta(self):
        class Meta(type):
            a: int

        class X(metaclass=Meta):
            pass

        class Y(metaclass=Meta):
            b: float

        self.assertEqual(get_annotations(Meta), {"a": int})
        self.assertEqual(Meta.__annotate__(Format.VALUE), {"a": int})

        self.assertEqual(get_annotations(X), {})
        self.assertIs(X.__annotate__, None)

        self.assertEqual(get_annotations(Y), {"b": float})
        self.assertEqual(Y.__annotate__(Format.VALUE), {"b": float})

    def test_unannotated_meta(self):
        class Meta(type):
            pass

        class X(metaclass=Meta):
            a: str

        class Y(X):
            pass

        self.assertEqual(get_annotations(Meta), {})
        self.assertIs(Meta.__annotate__, None)

        self.assertEqual(get_annotations(Y), {})
        self.assertIs(Y.__annotate__, None)

        self.assertEqual(get_annotations(X), {"a": str})
        self.assertEqual(X.__annotate__(Format.VALUE), {"a": str})

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
                        annotate_func = getattr(c, "__annotate__", None)
                        if c.expected_annotations:
                            self.assertEqual(
                                annotate_func(Format.VALUE), c.expected_annotations
                            )
                        else:
                            self.assertIs(annotate_func, None)


class TestGetAnnotateFromClassNamespace(unittest.TestCase):
    def test_with_metaclass(self):
        class Meta(type):
            def __new__(mcls, name, bases, ns):
                annotate = annotationlib.get_annotate_from_class_namespace(ns)
                expected = ns["expected_annotate"]
                with self.subTest(name=name):
                    if expected:
                        self.assertIsNotNone(annotate)
                    else:
                        self.assertIsNone(annotate)
                return super().__new__(mcls, name, bases, ns)

        class HasAnnotations(metaclass=Meta):
            expected_annotate = True
            a: int

        class NoAnnotations(metaclass=Meta):
            expected_annotate = False

        class CustomAnnotate(metaclass=Meta):
            expected_annotate = True
            def __annotate__(format):
                return {}

        code = """
            from __future__ import annotations

            class HasFutureAnnotations(metaclass=Meta):
                expected_annotate = False
                a: int
        """
        exec(textwrap.dedent(code), {"Meta": Meta})


class TestTypeRepr(unittest.TestCase):
    def test_type_repr(self):
        class Nested:
            pass

        def nested():
            pass

        self.assertEqual(type_repr(int), "int")
        self.assertEqual(type_repr(MyClass), f"{__name__}.MyClass")
        self.assertEqual(
            type_repr(Nested), f"{__name__}.TestTypeRepr.test_type_repr.<locals>.Nested"
        )
        self.assertEqual(
            type_repr(nested), f"{__name__}.TestTypeRepr.test_type_repr.<locals>.nested"
        )
        self.assertEqual(type_repr(len), "len")
        self.assertEqual(type_repr(type_repr), "annotationlib.type_repr")
        self.assertEqual(type_repr(times_three), f"{__name__}.times_three")
        self.assertEqual(type_repr(...), "...")
        self.assertEqual(type_repr(None), "None")
        self.assertEqual(type_repr(1), "1")
        self.assertEqual(type_repr("1"), "'1'")
        self.assertEqual(type_repr(Format.VALUE), repr(Format.VALUE))
        self.assertEqual(type_repr(MyClass()), "my repr")
        # gh138558 tests
        self.assertEqual(type_repr(t'''{ 0
            & 1
            | 2
        }'''), 't"""{ 0\n            & 1\n            | 2}"""')
        self.assertEqual(
            type_repr(Template("hi", Interpolation(42, "42"))), "t'hi{42}'"
        )
        self.assertEqual(
            type_repr(Template("hi", Interpolation(42))),
            "Template('hi', Interpolation(42, '', None, ''))",
        )
        self.assertEqual(
            type_repr(Template("hi", Interpolation(42, "   "))),
            "Template('hi', Interpolation(42, '   ', None, ''))",
        )
        # gh138558: perhaps in the future, we can improve this behavior:
        self.assertEqual(type_repr(Template(Interpolation(42, "99"))), "t'{99}'")


class TestAnnotationsToString(unittest.TestCase):
    def test_annotations_to_string(self):
        self.assertEqual(annotations_to_string({}), {})
        self.assertEqual(annotations_to_string({"x": int}), {"x": "int"})
        self.assertEqual(annotations_to_string({"x": "int"}), {"x": "int"})
        self.assertEqual(
            annotations_to_string({"x": int, "y": str}), {"x": "int", "y": "str"}
        )


class A:
    pass

TypeParamsAlias1 = int

class TypeParamsSample[TypeParamsAlias1, TypeParamsAlias2]:
    TypeParamsAlias2 = str


class TestForwardRefClass(unittest.TestCase):
    def test_forwardref_instance_type_error(self):
        fr = ForwardRef("int")
        with self.assertRaises(TypeError):
            isinstance(42, fr)

    def test_forwardref_subclass_type_error(self):
        fr = ForwardRef("int")
        with self.assertRaises(TypeError):
            issubclass(int, fr)

    def test_forwardref_only_str_arg(self):
        with self.assertRaises(TypeError):
            ForwardRef(1)  # only `str` type is allowed

    def test_forward_equality(self):
        fr = ForwardRef("int")
        self.assertEqual(fr, ForwardRef("int"))
        self.assertNotEqual(List["int"], List[int])
        self.assertNotEqual(fr, ForwardRef("int", module=__name__))
        frm = ForwardRef("int", module=__name__)
        self.assertEqual(frm, ForwardRef("int", module=__name__))
        self.assertNotEqual(frm, ForwardRef("int", module="__other_name__"))

    def test_forward_equality_get_type_hints(self):
        c1 = ForwardRef("C")
        c1_gth = ForwardRef("C")
        c2 = ForwardRef("C")
        c2_gth = ForwardRef("C")

        class C:
            pass

        def foo(a: c1_gth, b: c2_gth):
            pass

        self.assertEqual(get_type_hints(foo, globals(), locals()), {"a": C, "b": C})
        self.assertEqual(c1, c2)
        self.assertEqual(c1, c1_gth)
        self.assertEqual(c1_gth, c2_gth)
        self.assertEqual(List[c1], List[c1_gth])
        self.assertNotEqual(List[c1], List[C])
        self.assertNotEqual(List[c1_gth], List[C])
        self.assertEqual(Union[c1, c1_gth], Union[c1])
        self.assertEqual(Union[c1, c1_gth, int], Union[c1, int])

    def test_forward_equality_hash(self):
        c1 = ForwardRef("int")
        c1_gth = ForwardRef("int")
        c2 = ForwardRef("int")
        c2_gth = ForwardRef("int")

        def foo(a: c1_gth, b: c2_gth):
            pass

        get_type_hints(foo, globals(), locals())

        self.assertEqual(hash(c1), hash(c2))
        self.assertEqual(hash(c1_gth), hash(c2_gth))
        self.assertEqual(hash(c1), hash(c1_gth))

        c3 = ForwardRef("int", module=__name__)
        c4 = ForwardRef("int", module="__other_name__")

        self.assertNotEqual(hash(c3), hash(c1))
        self.assertNotEqual(hash(c3), hash(c1_gth))
        self.assertNotEqual(hash(c3), hash(c4))
        self.assertEqual(hash(c3), hash(ForwardRef("int", module=__name__)))

    def test_forward_equality_namespace(self):
        def namespace1():
            a = ForwardRef("A")

            def fun(x: a):
                pass

            get_type_hints(fun, globals(), locals())
            return a

        def namespace2():
            a = ForwardRef("A")

            class A:
                pass

            def fun(x: a):
                pass

            get_type_hints(fun, globals(), locals())
            return a

        self.assertEqual(namespace1(), namespace1())
        self.assertEqual(namespace1(), namespace2())

    def test_forward_repr(self):
        self.assertEqual(repr(List["int"]), "typing.List[ForwardRef('int')]")
        self.assertEqual(
            repr(List[ForwardRef("int", module="mod")]),
            "typing.List[ForwardRef('int', module='mod')]",
        )
        self.assertEqual(
            repr(List[ForwardRef("int", module="mod", is_class=True)]),
            "typing.List[ForwardRef('int', module='mod', is_class=True)]",
        )
        self.assertEqual(
            repr(List[ForwardRef("int", owner="class")]),
            "typing.List[ForwardRef('int', owner='class')]",
        )

    def test_forward_recursion_actually(self):
        def namespace1():
            a = ForwardRef("A")
            A = a

            def fun(x: a):
                pass

            ret = get_type_hints(fun, globals(), locals())
            return a

        def namespace2():
            a = ForwardRef("A")
            A = a

            def fun(x: a):
                pass

            ret = get_type_hints(fun, globals(), locals())
            return a

        r1 = namespace1()
        r2 = namespace2()
        self.assertIsNot(r1, r2)
        self.assertEqual(r1, r2)

    def test_syntax_error(self):

        with self.assertRaises(SyntaxError):
            typing.Generic["/T"]

    def test_delayed_syntax_error(self):

        def foo(a: "Node[T"):
            pass

        with self.assertRaises(SyntaxError):
            get_type_hints(foo)

    def test_syntax_error_empty_string(self):
        for form in [typing.List, typing.Set, typing.Type, typing.Deque]:
            with self.subTest(form=form):
                with self.assertRaises(SyntaxError):
                    form[""]

    def test_or(self):
        X = ForwardRef("X")
        # __or__/__ror__ itself
        self.assertEqual(X | "x", Union[X, "x"])
        self.assertEqual("x" | X, Union["x", X])

    def test_multiple_ways_to_create(self):
        X1 = Union["X"]
        self.assertIsInstance(X1, ForwardRef)
        X2 = ForwardRef("X")
        self.assertIsInstance(X2, ForwardRef)
        self.assertEqual(X1, X2)

    def test_special_attrs(self):
        # Forward refs provide a different introspection API. __name__ and
        # __qualname__ make little sense for forward refs as they can store
        # complex typing expressions.
        fr = ForwardRef("set[Any]")
        self.assertNotHasAttr(fr, "__name__")
        self.assertNotHasAttr(fr, "__qualname__")
        self.assertEqual(fr.__module__, "annotationlib")
        # Forward refs are currently unpicklable once they contain a code object.
        fr.__forward_code__  # fill the cache
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises(TypeError):
                pickle.dumps(fr, proto)

    def test_evaluate_string_format(self):
        fr = ForwardRef("set[Any]")
        self.assertEqual(fr.evaluate(format=Format.STRING), "set[Any]")

    def test_evaluate_forwardref_format(self):
        fr = ForwardRef("undef")
        evaluated = fr.evaluate(format=Format.FORWARDREF)
        self.assertIs(fr, evaluated)

        fr = ForwardRef("set[undefined]")
        evaluated = fr.evaluate(format=Format.FORWARDREF)
        self.assertEqual(
            evaluated,
            set[support.EqualToForwardRef("undefined")],
        )

        fr = ForwardRef("a + b")
        self.assertEqual(
            fr.evaluate(format=Format.FORWARDREF),
            support.EqualToForwardRef("a + b"),
        )
        self.assertEqual(
            fr.evaluate(format=Format.FORWARDREF, locals={"a": 1, "b": 2}),
            3,
        )

        fr = ForwardRef('"a" + 1')
        self.assertEqual(
            fr.evaluate(format=Format.FORWARDREF),
            support.EqualToForwardRef('"a" + 1'),
        )

    def test_evaluate_notimplemented_format(self):
        class C:
            x: alias

        fwdref = get_annotations(C, format=Format.FORWARDREF)["x"]

        with self.assertRaises(NotImplementedError):
            fwdref.evaluate(format=Format.VALUE_WITH_FAKE_GLOBALS)

        with self.assertRaises(NotImplementedError):
            # Some other unsupported value
            fwdref.evaluate(format=7)

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

    def test_evaluate_with_type_params_and_scope_conflict(self):
        for is_class in (False, True):
            with self.subTest(is_class=is_class):
                fwdref1 = ForwardRef("TypeParamsAlias1", owner=TypeParamsSample, is_class=is_class)
                fwdref2 = ForwardRef("TypeParamsAlias2", owner=TypeParamsSample, is_class=is_class)

                self.assertIs(
                    fwdref1.evaluate(),
                    TypeParamsSample.__type_params__[0],
                )
                self.assertIs(
                    fwdref2.evaluate(),
                    TypeParamsSample.TypeParamsAlias2,
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

        with self.assertRaises(NameError, msg="name 'doesntexist' is not defined") as exc:
            ForwardRef("doesntexist").evaluate()

        self.assertEqual(exc.exception.name, "doesntexist")

    def test_evaluate_undefined_generic(self):
        # Test the codepath where have to eval() with undefined variables.
        class C:
            x: alias[int, undef]

        generic = get_annotations(C, format=Format.FORWARDREF)["x"].evaluate(
            format=Format.FORWARDREF,
            globals={"alias": dict}
        )
        self.assertNotIsInstance(generic, ForwardRef)
        self.assertIs(generic.__origin__, dict)
        self.assertEqual(len(generic.__args__), 2)
        self.assertIs(generic.__args__[0], int)
        self.assertIsInstance(generic.__args__[1], ForwardRef)

        generic = get_annotations(C, format=Format.FORWARDREF)["x"].evaluate(
            format=Format.FORWARDREF,
            globals={"alias": Union},
            locals={"alias": dict}
        )
        self.assertNotIsInstance(generic, ForwardRef)
        self.assertIs(generic.__origin__, dict)
        self.assertEqual(len(generic.__args__), 2)
        self.assertIs(generic.__args__[0], int)
        self.assertIsInstance(generic.__args__[1], ForwardRef)

    def test_fwdref_invalid_syntax(self):
        fr = ForwardRef("if")
        with self.assertRaises(SyntaxError):
            fr.evaluate()
        fr = ForwardRef("1+")
        with self.assertRaises(SyntaxError):
            fr.evaluate()

    def test_re_evaluate_generics(self):
        global global_alias

        # If we've already run this test before,
        # ensure the variable is still undefined
        if "global_alias" in globals():
            del global_alias

        class C:
            x: global_alias[int]

        # Evaluate the ForwardRef once
        evaluated = get_annotations(C, format=Format.FORWARDREF)["x"].evaluate(
            format=Format.FORWARDREF
        )

        # Now define the global and ensure that the ForwardRef evaluates
        global_alias = list
        self.assertEqual(evaluated.evaluate(), list[int])

    def test_fwdref_evaluate_argument_mutation(self):
        class C[T]:
            nonlocal alias
            x: alias[T]

        # Mutable arguments
        globals_ = globals()
        globals_copy = globals_.copy()
        locals_ = locals()
        locals_copy = locals_.copy()

        # Evaluate the ForwardRef, ensuring we use __cell__ and type params
        get_annotations(C, format=Format.FORWARDREF)["x"].evaluate(
            globals=globals_,
            locals=locals_,
            type_params=C.__type_params__,
            format=Format.FORWARDREF,
        )

        # Check if the passed in mutable arguments equal the originals
        self.assertEqual(globals_, globals_copy)
        self.assertEqual(locals_, locals_copy)

        alias = list

    def test_fwdref_final_class(self):
        with self.assertRaises(TypeError):
            class C(ForwardRef):
                pass


class TestAnnotationLib(unittest.TestCase):
    def test__all__(self):
        support.check__all__(self, annotationlib)

    @support.cpython_only
    def test_lazy_imports(self):
        import_helper.ensure_lazy_imports(
            "annotationlib",
            {
                "typing",
                "warnings",
            },
        )
