"""Tests for the annotations module."""

import annotationlib
import functools
import itertools
import pickle
import unittest
from annotationlib import Format, get_annotations, get_annotate_function
from typing import Unpack

from test.test_inspect import inspect_stock_annotations
from test.test_inspect import inspect_stringized_annotations
from test.test_inspect import inspect_stringized_annotations_2
from test.test_inspect import inspect_stringized_annotations_pep695


def times_three(fn):
    @functools.wraps(fn)
    def wrapper(a, b):
        return fn(a * 3, b * 3)

    return wrapper


class TestFormat(unittest.TestCase):
    def test_enum(self):
        self.assertEqual(annotationlib.Format.VALUE.value, 1)
        self.assertEqual(annotationlib.Format.VALUE, 1)

        self.assertEqual(annotationlib.Format.FORWARDREF.value, 2)
        self.assertEqual(annotationlib.Format.FORWARDREF, 2)

        self.assertEqual(annotationlib.Format.SOURCE.value, 3)
        self.assertEqual(annotationlib.Format.SOURCE, 3)


class TestForwardRefFormat(unittest.TestCase):
    def test_closure(self):
        def inner(arg: x):
            pass

        anno = annotationlib.get_annotations(
            inner, format=annotationlib.Format.FORWARDREF
        )
        fwdref = anno["arg"]
        self.assertIsInstance(fwdref, annotationlib.ForwardRef)
        self.assertEqual(fwdref.__forward_arg__, "x")
        with self.assertRaises(NameError):
            fwdref.evaluate()

        x = 1
        self.assertEqual(fwdref.evaluate(), x)

        anno = annotationlib.get_annotations(
            inner, format=annotationlib.Format.FORWARDREF
        )
        self.assertEqual(anno["arg"], x)

    def test_function(self):
        def f(x: int, y: doesntexist):
            pass

        anno = annotationlib.get_annotations(f, format=annotationlib.Format.FORWARDREF)
        self.assertIs(anno["x"], int)
        fwdref = anno["y"]
        self.assertIsInstance(fwdref, annotationlib.ForwardRef)
        self.assertEqual(fwdref.__forward_arg__, "doesntexist")
        with self.assertRaises(NameError):
            fwdref.evaluate()
        self.assertEqual(fwdref.evaluate(globals={"doesntexist": 1}), 1)


class TestSourceFormat(unittest.TestCase):
    def test_closure(self):
        x = 0

        def inner(arg: x):
            pass

        anno = annotationlib.get_annotations(inner, format=annotationlib.Format.SOURCE)
        self.assertEqual(anno, {"arg": "x"})

    def test_function(self):
        def f(x: int, y: doesntexist):
            pass

        anno = annotationlib.get_annotations(f, format=annotationlib.Format.SOURCE)
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

        anno = annotationlib.get_annotations(f, format=annotationlib.Format.SOURCE)
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

        anno = annotationlib.get_annotations(f, format=annotationlib.Format.SOURCE)
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

        anno = annotationlib.get_annotations(f, format=annotationlib.Format.SOURCE)
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
            annotationlib.get_annotations(f, format=annotationlib.Format.SOURCE)

        def f(fstring_format: f"{a:02d}"):
            pass

        with self.assertRaisesRegex(TypeError, format_msg):
            annotationlib.get_annotations(f, format=annotationlib.Format.SOURCE)


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
            annotationlib.get_annotations(C1, format=annotationlib.Format.FORWARDREF),
            {"a": int},
        )
        self.assertEqual(
            annotationlib.get_annotations(C1, format=annotationlib.Format.SOURCE),
            {"a": "int"},
        )
        self.assertEqual(annotationlib.get_annotations(NoDict), {"b": str})
        self.assertEqual(
            annotationlib.get_annotations(NoDict, format=annotationlib.Format.FORWARDREF),
            {"b": str},
        )
        self.assertEqual(
            annotationlib.get_annotations(NoDict, format=annotationlib.Format.SOURCE),
            {"b": "str"},
        )

    def test_format(self):
        def f1(a: int):
            pass

        def f2(a: undefined):
            pass

        self.assertEqual(
            annotationlib.get_annotations(f1, format=annotationlib.Format.VALUE),
            {"a": int},
        )
        self.assertEqual(annotationlib.get_annotations(f1, format=1), {"a": int})

        fwd = annotationlib.ForwardRef("undefined")
        self.assertEqual(
            annotationlib.get_annotations(f2, format=annotationlib.Format.FORWARDREF),
            {"a": fwd},
        )
        self.assertEqual(annotationlib.get_annotations(f2, format=2), {"a": fwd})

        self.assertEqual(
            annotationlib.get_annotations(f1, format=annotationlib.Format.SOURCE),
            {"a": "int"},
        )
        self.assertEqual(annotationlib.get_annotations(f1, format=3), {"a": "int"})

        with self.assertRaises(ValueError):
            annotationlib.get_annotations(f1, format=0)

        with self.assertRaises(ValueError):
            annotationlib.get_annotations(f1, format=4)

    def test_custom_object_with_annotations(self):
        class C:
            def __init__(self):
                self.__annotations__ = {"x": int, "y": str}

        self.assertEqual(annotationlib.get_annotations(C()), {"x": int, "y": str})

    def test_custom_format_eval_str(self):
        def foo():
            pass

        with self.assertRaises(ValueError):
            annotationlib.get_annotations(
                foo, format=annotationlib.Format.FORWARDREF, eval_str=True
            )
            annotationlib.get_annotations(
                foo, format=annotationlib.Format.SOURCE, eval_str=True
            )

    def test_stock_annotations(self):
        def foo(a: int, b: str):
            pass

        for format in (annotationlib.Format.VALUE, annotationlib.Format.FORWARDREF):
            with self.subTest(format=format):
                self.assertEqual(
                    annotationlib.get_annotations(foo, format=format),
                    {"a": int, "b": str},
                )
        self.assertEqual(
            annotationlib.get_annotations(foo, format=annotationlib.Format.SOURCE),
            {"a": "int", "b": "str"},
        )

        foo.__annotations__ = {"a": "foo", "b": "str"}
        for format in annotationlib.Format:
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
            {"format": annotationlib.Format.VALUE},
            {"format": annotationlib.Format.FORWARDREF},
            {"format": annotationlib.Format.VALUE, "eval_str": False},
            {"format": annotationlib.Format.FORWARDREF, "eval_str": False},
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
            {"format": annotationlib.Format.VALUE, "eval_str": True},
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
            annotationlib.get_annotations(isa, format=annotationlib.Format.SOURCE),
            {"a": "int", "b": "str"},
        )
        self.assertEqual(
            annotationlib.get_annotations(
                isa.MyClass, format=annotationlib.Format.SOURCE
            ),
            {"a": "int", "b": "str"},
        )
        self.assertEqual(
            annotationlib.get_annotations(
                isa.function, format=annotationlib.Format.SOURCE
            ),
            {"a": "int", "b": "str", "return": "MyClass"},
        )
        self.assertEqual(
            annotationlib.get_annotations(
                isa.function2, format=annotationlib.Format.SOURCE
            ),
            {"a": "int", "b": "str", "c": "MyClass", "return": "MyClass"},
        )
        self.assertEqual(
            annotationlib.get_annotations(
                isa.function3, format=annotationlib.Format.SOURCE
            ),
            {"a": "int", "b": "str", "c": "MyClass"},
        )
        self.assertEqual(
            annotationlib.get_annotations(
                annotationlib, format=annotationlib.Format.SOURCE
            ),
            {},
        )
        self.assertEqual(
            annotationlib.get_annotations(
                isa.UnannotatedClass, format=annotationlib.Format.SOURCE
            ),
            {},
        )
        self.assertEqual(
            annotationlib.get_annotations(
                isa.unannotated_function, format=annotationlib.Format.SOURCE
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
            annotationlib.get_annotations(
                wrapped, format=annotationlib.Format.FORWARDREF
            ),
            {"a": int, "b": str, "return": isa.MyClass},
        )
        self.assertEqual(
            annotationlib.get_annotations(wrapped, format=annotationlib.Format.SOURCE),
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
            {"format": annotationlib.Format.VALUE},
            {"format": annotationlib.Format.FORWARDREF},
            {"format": annotationlib.Format.SOURCE},
            {"format": annotationlib.Format.VALUE, "eval_str": False},
            {"format": annotationlib.Format.FORWARDREF, "eval_str": False},
            {"format": annotationlib.Format.SOURCE, "eval_str": False},
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
            {"format": annotationlib.Format.VALUE, "eval_str": True},
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
            annotationlib.get_annotations(f, format=annotationlib.Format.FORWARDREF),
            {"x": int},
        )

        f.__annotations__["x"] = str
        # The modification is reflected in VALUE (the default)
        self.assertEqual(annotationlib.get_annotations(f), {"x": str})
        # ... but not in FORWARDREF, which uses __annotate__
        self.assertEqual(
            annotationlib.get_annotations(f, format=annotationlib.Format.FORWARDREF),
            {"x": int},
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

    def test_pep695_generic_class_with_future_annotations_name_clash_with_global_vars(self):
        ann_module695 = inspect_stringized_annotations_pep695
        C_annotations = annotationlib.get_annotations(ann_module695.C, eval_str=True)
        self.assertEqual(
            set(C_annotations.values()),
            set(ann_module695.C.__type_params__)
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

    def test_pep_695_generic_function_with_future_annotations_name_clash_with_global_vars(self):
        self.assertEqual(
            set(
                annotationlib.get_annotations(
                    inspect_stringized_annotations_pep695.generic_function_2,
                    eval_str=True
                ).values()
            ),
            set(
                inspect_stringized_annotations_pep695.generic_function_2.__type_params__
            )
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
            {"x": params["Foo"], "y": params["Bar"], "return": None}
        )

    def test_pep_695_generic_method_with_future_annotations_name_clash_with_global_vars(self):
        self.assertEqual(
            set(
                annotationlib.get_annotations(
                    inspect_stringized_annotations_pep695.D.generic_method_2,
                    eval_str=True
                ).values()
            ),
            set(
                inspect_stringized_annotations_pep695.D.generic_method_2.__type_params__
            )
        )

    def test_pep_695_generic_method_with_future_annotations_name_clash_with_global_and_local_vars(self):
        self.assertEqual(
            annotationlib.get_annotations(
                inspect_stringized_annotations_pep695.E, eval_str=True
            ),
            {"x": str},
        )

    def test_pep_695_generics_with_future_annotations_nested_in_function(self):
        results = inspect_stringized_annotations_pep695.nested()

        self.assertEqual(
            set(results.F_annotations.values()),
            set(results.F.__type_params__)
        )
        self.assertEqual(
            set(results.F_meth_annotations.values()),
            set(results.F.generic_method.__type_params__)
        )
        self.assertNotEqual(
            set(results.F_meth_annotations.values()),
            set(results.F.__type_params__)
        )
        self.assertEqual(
            set(results.F_meth_annotations.values()).intersection(results.F.__type_params__),
            set()
        )

        self.assertEqual(results.G_annotations, {"x": str})

        self.assertEqual(
            set(results.generic_func_annotations.values()),
            set(results.generic_func.__type_params__),
        )


class TestCallEvaluateFunction(unittest.TestCase):
    def test_evaluation(self):
        def evaluate(format, exc=NotImplementedError):
            if format != 1:
                raise exc
            return undefined

        with self.assertRaises(NameError):
            annotationlib.call_evaluate_function(evaluate, annotationlib.Format.VALUE)
        self.assertEqual(
            annotationlib.call_evaluate_function(evaluate, annotationlib.Format.FORWARDREF),
            annotationlib.ForwardRef("undefined"),
        )
        self.assertEqual(
            annotationlib.call_evaluate_function(evaluate, annotationlib.Format.SOURCE),
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
        class Meta(type): pass

        class X(metaclass=Meta):
            a: str

        class Y(X): pass

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
                            self.assertEqual(annotate_func(Format.VALUE), c.expected_annotations)
                        else:
                            self.assertIs(annotate_func, None)
