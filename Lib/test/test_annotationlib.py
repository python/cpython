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
