"""Tests for the annotations module."""

import annotations
import functools
import pickle
import unittest

from test.test_inspect import inspect_stock_annotations
from test.test_inspect import inspect_stringized_annotations
from test.test_inspect import inspect_stringized_annotations_2

def times_three(fn):
    @functools.wraps(fn)
    def wrapper(a, b):
        return fn(a*3, b*3)
    return wrapper


class TestFormat(unittest.TestCase):
    def test_enum(self):
        self.assertEqual(annotations.Format.VALUE.value, 1)
        self.assertEqual(annotations.Format.VALUE, 1)

        self.assertEqual(annotations.Format.FORWARDREF.value, 2)
        self.assertEqual(annotations.Format.FORWARDREF, 2)

        self.assertEqual(annotations.Format.SOURCE.value, 3)
        self.assertEqual(annotations.Format.SOURCE, 3)


class TestForwardRefFormat(unittest.TestCase):
    def test_closure(self):
        def inner(arg: x):
            pass

        anno = annotations.get_annotations(inner, format=annotations.Format.FORWARDREF)
        fwdref = anno["arg"]
        self.assertIsInstance(fwdref, annotations.ForwardRef)
        self.assertEqual(fwdref.__forward_arg__, "x")
        with self.assertRaises(NameError):
            fwdref.evaluate()

        x = 1
        self.assertEqual(fwdref.evaluate(), x)

        anno = annotations.get_annotations(inner, format=annotations.Format.FORWARDREF)
        self.assertEqual(anno["arg"], x)

    def test_function(self):
        def f(x: int, y: doesntexist):
            pass

        anno = annotations.get_annotations(f, format=annotations.Format.FORWARDREF)
        self.assertIs(anno["x"], int)
        fwdref = anno["y"]
        self.assertIsInstance(fwdref, annotations.ForwardRef)
        self.assertEqual(fwdref.__forward_arg__, "doesntexist")
        with self.assertRaises(NameError):
            fwdref.evaluate()
        self.assertEqual(fwdref.evaluate(globals={"doesntexist": 1}), 1)


class TestSourceFormat(unittest.TestCase):
    def test_closure(self):
        x = 0

        def inner(arg: x):
            pass

        anno = annotations.get_annotations(inner, format=annotations.Format.SOURCE)
        self.assertEqual(anno, {"arg": "x"})

    def test_function(self):
        def f(x: int, y: doesntexist):
            pass

        anno = annotations.get_annotations(f, format=annotations.Format.SOURCE)
        self.assertEqual(anno, {"x": "int", "y": "doesntexist"})

    def test_expressions(self):
        def f(
            add: a + b,
            sub: a + b,
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
            pow_: a ** b,
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
        anno = annotations.get_annotations(f, format=annotations.Format.SOURCE)
        self.assertEqual(anno, {
            "add": "a + b",
            "sub": "a + b",
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
        })

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
            rpow: 1 ** a,
        ):
            pass

        anno = annotations.get_annotations(f, format=annotations.Format.SOURCE)
        self.assertEqual(anno, {
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
        })

    def test_nested_expressions(self):
        def f(
            nested: list[Annotated[set[int], "set of ints", 4j]],
            set: {a + b},  # single element because order is not guaranteed
            dict: {a + b: c + d, "key": e + g},
            list: [a, b, c],
            tuple: (a, b, c),
            slice: (a[b:c], a[b:c:d], a[:c], a[b:], a[:], a[::d], a[b::d]),
        ): pass
        anno = annotations.get_annotations(f, format=annotations.Format.SOURCE)
        self.assertEqual(anno, {
            "nested": "list[Annotated[set[int], 'set of ints', 4j]]",
            "set": "{a + b}",
            "dict": "{a + b: c + d, 'key': e + g}",
            "list": "[a, b, c]",
            "tuple": "(a, b, c)",
            "slice": "(a[b:c], a[b:c:d], a[:c], a[b:], a[:], a[::d], a[b::d])",
        })


class TestForwardRefClass(unittest.TestCase):
    def test_special_attrs(self):
        # Forward refs provide a different introspection API. __name__ and
        # __qualname__ make little sense for forward refs as they can store
        # complex typing expressions.
        fr = annotations.ForwardRef("set[Any]")
        self.assertFalse(hasattr(fr, "__name__"))
        self.assertFalse(hasattr(fr, "__qualname__"))
        self.assertEqual(fr.__module__, "annotations")
        # Forward refs are currently unpicklable once they contain a code object.
        fr.__forward_code__  # fill the cache
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises(TypeError):
                pickle.dumps(fr, proto)


class TestGetAnnotations(unittest.TestCase):
    def test_builtin_type(self):
        self.assertEqual(annotations.get_annotations(int), {})
        self.assertEqual(annotations.get_annotations(object), {})

    def test_custom_object_with_annotations(self):
        class C:
            def __init__(self, x: int = 0, y: str = ""):
                self.__annotations__ = {"x": int, "y": str}

        self.assertEqual(annotations.get_annotations(C()), {"x": int, "y": str})

    def test_custom_format_eval_str(self):
        def foo(): pass
        with self.assertRaises(ValueError):
            annotations.get_annotations(foo, format=annotations.Format.FORWARDREF, eval_str=True)
            annotations.get_annotations(foo, format=annotations.Format.SOURCE, eval_str=True)

    def test_stock_annotations(self):
        def foo(a:int, b:str): pass
        for format in (annotations.Format.VALUE, annotations.Format.FORWARDREF):
            with self.subTest(format=format):
                self.assertEqual(annotations.get_annotations(foo, format=format), {'a': int, 'b': str})
        self.assertEqual(annotations.get_annotations(foo, format=annotations.Format.SOURCE), {'a': 'int', 'b': 'str'})

        foo.__annotations__ = {'a': 'foo', 'b':'str'}
        for format in annotations.Format:
            with self.subTest(format=format):
                self.assertEqual(annotations.get_annotations(foo, format=format), {'a': 'foo', 'b': 'str'})

        self.assertEqual(annotations.get_annotations(foo, eval_str=True, locals=locals()), {'a': foo, 'b': str})
        self.assertEqual(annotations.get_annotations(foo, eval_str=True, globals=locals()), {'a': foo, 'b': str})

    def test_stock_annotations_in_module(self):
        isa = inspect_stock_annotations

        for kwargs in [
            {},
            {'eval_str': False},
            {'format': annotations.Format.VALUE},
            {'format': annotations.Format.FORWARDREF},
            {'format': annotations.Format.VALUE, 'eval_str': False},
            {'format': annotations.Format.FORWARDREF, 'eval_str': False},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(annotations.get_annotations(isa, **kwargs), {'a': int, 'b': str})
                self.assertEqual(annotations.get_annotations(isa.MyClass, **kwargs), {'a': int, 'b': str})
                self.assertEqual(annotations.get_annotations(isa.function, **kwargs), {'a': int, 'b': str, 'return': isa.MyClass})
                self.assertEqual(annotations.get_annotations(isa.function2, **kwargs), {'a': int, 'b': 'str', 'c': isa.MyClass, 'return': isa.MyClass})
                self.assertEqual(annotations.get_annotations(isa.function3, **kwargs), {'a': 'int', 'b': 'str', 'c': 'MyClass'})
                self.assertEqual(annotations.get_annotations(annotations, **kwargs), {}) # annotations module has no annotations
                self.assertEqual(annotations.get_annotations(isa.UnannotatedClass, **kwargs), {})
                self.assertEqual(annotations.get_annotations(isa.unannotated_function, **kwargs), {})

        for kwargs in [
            {"eval_str": True},
            {"format": annotations.Format.VALUE, "eval_str": True},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(annotations.get_annotations(isa, **kwargs), {'a': int, 'b': str})
                self.assertEqual(annotations.get_annotations(isa.MyClass, **kwargs), {'a': int, 'b': str})
                self.assertEqual(annotations.get_annotations(isa.function, **kwargs), {'a': int, 'b': str, 'return': isa.MyClass})
                self.assertEqual(annotations.get_annotations(isa.function2, **kwargs), {'a': int, 'b': str, 'c': isa.MyClass, 'return': isa.MyClass})
                self.assertEqual(annotations.get_annotations(isa.function3, **kwargs), {'a': int, 'b': str, 'c': isa.MyClass})
                self.assertEqual(annotations.get_annotations(annotations, **kwargs), {})
                self.assertEqual(annotations.get_annotations(isa.UnannotatedClass, **kwargs), {})
                self.assertEqual(annotations.get_annotations(isa.unannotated_function, **kwargs), {})

        self.assertEqual(annotations.get_annotations(isa, format=annotations.Format.SOURCE), {'a': 'int', 'b': 'str'})
        self.assertEqual(annotations.get_annotations(isa.MyClass, format=annotations.Format.SOURCE), {'a': 'int', 'b': 'str'})
        self.assertEqual(annotations.get_annotations(isa.function, format=annotations.Format.SOURCE), {'a': 'int', 'b': 'str', 'return': 'MyClass'})
        self.assertEqual(annotations.get_annotations(isa.function2, format=annotations.Format.SOURCE), {'a': 'int', 'b': 'str', 'c': 'MyClass', 'return': 'MyClass'})
        self.assertEqual(annotations.get_annotations(isa.function3, format=annotations.Format.SOURCE), {'a': 'int', 'b': 'str', 'c': 'MyClass'})
        self.assertEqual(annotations.get_annotations(annotations, format=annotations.Format.SOURCE), {})
        self.assertEqual(annotations.get_annotations(isa.UnannotatedClass, format=annotations.Format.SOURCE), {})
        self.assertEqual(annotations.get_annotations(isa.unannotated_function, format=annotations.Format.SOURCE), {})

    def test_stock_annotations_on_wrapper(self):
        isa = inspect_stock_annotations

        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, 'x'), isa.MyClass(3, 'xxx'))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(annotations.get_annotations(wrapped), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(wrapped, format=annotations.Format.FORWARDREF),
                         {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(wrapped, format=annotations.Format.SOURCE),
                         {'a': 'int', 'b': 'str', 'return': 'MyClass'})
        self.assertEqual(annotations.get_annotations(wrapped, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(wrapped, eval_str=False), {'a': int, 'b': str, 'return': isa.MyClass})

    def test_stringized_annotations_in_module(self):
        isa = inspect_stringized_annotations
        for kwargs in [
            {},
            {'eval_str': False},
            {'format': annotations.Format.VALUE},
            {'format': annotations.Format.FORWARDREF},
            {'format': annotations.Format.SOURCE},
            {'format': annotations.Format.VALUE, 'eval_str': False},
            {'format': annotations.Format.FORWARDREF, 'eval_str': False},
            {'format': annotations.Format.SOURCE, 'eval_str': False},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(annotations.get_annotations(isa, **kwargs), {'a': 'int', 'b': 'str'})
                self.assertEqual(annotations.get_annotations(isa.MyClass, **kwargs), {'a': 'int', 'b': 'str'})
                self.assertEqual(annotations.get_annotations(isa.function, **kwargs), {'a': 'int', 'b': 'str', 'return': 'MyClass'})
                self.assertEqual(annotations.get_annotations(isa.function2, **kwargs), {'a': 'int', 'b': "'str'", 'c': 'MyClass', 'return': 'MyClass'})
                self.assertEqual(annotations.get_annotations(isa.function3, **kwargs), {'a': "'int'", 'b': "'str'", 'c': "'MyClass'"})
                self.assertEqual(annotations.get_annotations(isa.UnannotatedClass, **kwargs), {})
                self.assertEqual(annotations.get_annotations(isa.unannotated_function, **kwargs), {})

        for kwargs in [
            {"eval_str": True},
            {"format": annotations.Format.VALUE, "eval_str": True},
        ]:
            with self.subTest(**kwargs):
                self.assertEqual(annotations.get_annotations(isa, **kwargs), {'a': int, 'b': str})
                self.assertEqual(annotations.get_annotations(isa.MyClass, **kwargs), {'a': int, 'b': str})
                self.assertEqual(annotations.get_annotations(isa.function, **kwargs), {'a': int, 'b': str, 'return': isa.MyClass})
                self.assertEqual(annotations.get_annotations(isa.function2, **kwargs), {'a': int, 'b': 'str', 'c': isa.MyClass, 'return': isa.MyClass})
                self.assertEqual(annotations.get_annotations(isa.function3, **kwargs), {'a': 'int', 'b': 'str', 'c': 'MyClass'})
                self.assertEqual(annotations.get_annotations(isa.UnannotatedClass, **kwargs), {})
                self.assertEqual(annotations.get_annotations(isa.unannotated_function, **kwargs), {})

    def test_stringized_annotations_in_empty_module(self):
        isa2 = inspect_stringized_annotations_2
        self.assertEqual(annotations.get_annotations(isa2), {})
        self.assertEqual(annotations.get_annotations(isa2, eval_str=True), {})
        self.assertEqual(annotations.get_annotations(isa2, eval_str=False), {})

    def test_stringized_annotations_on_wrapper(self):
        isa = inspect_stringized_annotations
        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, 'x'), isa.MyClass(3, 'xxx'))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(annotations.get_annotations(wrapped), {'a': 'int', 'b': 'str', 'return': 'MyClass'})
        self.assertEqual(annotations.get_annotations(wrapped, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(wrapped, eval_str=False), {'a': 'int', 'b': 'str', 'return': 'MyClass'})

    def test_stringized_annotations_on_class(self):
        isa = inspect_stringized_annotations
        # test that local namespace lookups work
        self.assertEqual(annotations.get_annotations(isa.MyClassWithLocalAnnotations), {'x': 'mytype'})
        self.assertEqual(annotations.get_annotations(isa.MyClassWithLocalAnnotations, eval_str=True), {'x': int})
