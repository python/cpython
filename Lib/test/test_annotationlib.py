"""Tests for the annotations module."""

import textwrap
import annotationlib
import builtins
import collections
import functools
import itertools
import pickle
from string.templatelib import Template, Interpolation
import types
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
            epsilon: some | {obj},
            zeta: some | [obj, module],
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
        self.assertEqual(epsilon_anno, support.EqualToForwardRef("some | {obj}", owner=f))

        zeta_anno = anno["zeta"]
        self.assertIsInstance(zeta_anno, ForwardRef)
        self.assertEqual(zeta_anno, support.EqualToForwardRef("some | [obj, module]", owner=f))

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
            {"x": "x | <class 'int'>", "y": "int"},
        )

    def test_comprehension_lambda_and_genexpr(self):
        # gh-157056: dict comprehensions used to raise ValueError while
        # stringifying, and lambda / generator-expression annotations leaked
        # a memory address via repr().
        def f(x: {k: v for k, v in items}):
            pass

        self.assertEqual(
            get_annotations(f, format=Format.STRING),
            {"x": "{k: v for k, v in items}"},
        )

        def g(x: lambda q: q):
            pass

        g_anno = get_annotations(g, format=Format.STRING)
        self.assertEqual(g_anno, {"x": "lambda q: q"})
        self.assertNotIn("0x", g_anno["x"].lower())

        def h(x: (w for w in seq)):
            pass

        h_anno = get_annotations(h, format=Format.STRING)
        self.assertEqual(h_anno, {"x": "(w for w in seq)"})
        self.assertNotIn("0x", h_anno["x"].lower())

        def mixed(a: int, b: {k: v for k, v in items}, c: lambda q: q):
            pass

        self.assertEqual(
            get_annotations(mixed, format=Format.STRING),
            {
                "a": "int",
                "b": "{k: v for k, v in items}",
                "c": "lambda q: q",
            },
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

    def test_eval_str_wrapped_cycle_self(self):
        # gh-146556: self-referential __wrapped__ cycle must not hang.
        def f(x: 'int') -> 'str': ...
        f.__wrapped__ = f
        # Cycle is detected and broken; globals from f itself are used.
        result = get_annotations(f, eval_str=True)
        self.assertEqual(result, {'x': int, 'return': str})

    def test_eval_str_wrapped_partial_cycle_self(self):
        def f(x: 'int') -> 'str': ...
        f.__wrapped__ = functools.partial(f, 0)
        # Cycle is detected and broken; globals from f itself are used.
        result = get_annotations(f, eval_str=True)
        self.assertEqual(result, {'x': int, 'return': str})

    def test_eval_str_wrapped_cycle_mutual(self):
        # gh-146556: mutual __wrapped__ cycle (a -> b -> a) must not hang.
        def a(x: 'int'): ...
        def b(): ...
        a.__wrapped__ = b
        b.__wrapped__ = a
        result = get_annotations(a, eval_str=True)
        self.assertEqual(result, {'x': int})

    def test_eval_str_wrapped_chain_no_cycle(self):
        # gh-146556: a valid (non-cyclic) __wrapped__ chain must still work.
        def inner(x: 'int'): ...
        def outer(x: 'int'): ...
        outer.__wrapped__ = inner
        result = get_annotations(outer, eval_str=True)
        self.assertEqual(result, {'x': int})

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
