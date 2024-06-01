"""Tests for the annotations module."""

import annotations
import functools
import pickle
import unittest

from test.test_inspect import inspect_stock_annotations
from test.test_inspect import inspect_stringized_annotations
from test.test_inspect import inspect_stringized_annotations_2


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

    def test_get_annotations_with_stock_annotations(self):
        def foo(a:int, b:str): pass
        self.assertEqual(annotations.get_annotations(foo), {'a': int, 'b': str})

        foo.__annotations__ = {'a': 'foo', 'b':'str'}
        self.assertEqual(annotations.get_annotations(foo), {'a': 'foo', 'b': 'str'})

        self.assertEqual(annotations.get_annotations(foo, eval_str=True, locals=locals()), {'a': foo, 'b': str})
        self.assertEqual(annotations.get_annotations(foo, eval_str=True, globals=locals()), {'a': foo, 'b': str})

        isa = inspect_stock_annotations
        self.assertEqual(annotations.get_annotations(isa), {'a': int, 'b': str})
        self.assertEqual(annotations.get_annotations(isa.MyClass), {'a': int, 'b': str})
        self.assertEqual(annotations.get_annotations(isa.function), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(isa.function2), {'a': int, 'b': 'str', 'c': isa.MyClass, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(isa.function3), {'a': 'int', 'b': 'str', 'c': 'MyClass'})
        self.assertEqual(annotations.get_annotations(annotations), {}) # annotations module has no annotations
        self.assertEqual(annotations.get_annotations(isa.UnannotatedClass), {})
        self.assertEqual(annotations.get_annotations(isa.unannotated_function), {})

        self.assertEqual(annotations.get_annotations(isa, eval_str=True), {'a': int, 'b': str})
        self.assertEqual(annotations.get_annotations(isa.MyClass, eval_str=True), {'a': int, 'b': str})
        self.assertEqual(annotations.get_annotations(isa.function, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(isa.function2, eval_str=True), {'a': int, 'b': str, 'c': isa.MyClass, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(isa.function3, eval_str=True), {'a': int, 'b': str, 'c': isa.MyClass})
        self.assertEqual(annotations.get_annotations(annotations, eval_str=True), {})
        self.assertEqual(annotations.get_annotations(isa.UnannotatedClass, eval_str=True), {})
        self.assertEqual(annotations.get_annotations(isa.unannotated_function, eval_str=True), {})

        self.assertEqual(annotations.get_annotations(isa, eval_str=False), {'a': int, 'b': str})
        self.assertEqual(annotations.get_annotations(isa.MyClass, eval_str=False), {'a': int, 'b': str})
        self.assertEqual(annotations.get_annotations(isa.function, eval_str=False), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(isa.function2, eval_str=False), {'a': int, 'b': 'str', 'c': isa.MyClass, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(isa.function3, eval_str=False), {'a': 'int', 'b': 'str', 'c': 'MyClass'})
        self.assertEqual(annotations.get_annotations(annotations, eval_str=False), {})
        self.assertEqual(annotations.get_annotations(isa.UnannotatedClass, eval_str=False), {})
        self.assertEqual(annotations.get_annotations(isa.unannotated_function, eval_str=False), {})

        def times_three(fn):
            @functools.wraps(fn)
            def wrapper(a, b):
                return fn(a*3, b*3)
            return wrapper

        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, 'x'), isa.MyClass(3, 'xxx'))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(annotations.get_annotations(wrapped), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(wrapped, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(wrapped, eval_str=False), {'a': int, 'b': str, 'return': isa.MyClass})

    def test_get_annotations_with_stringized_annotations(self):
        isa = inspect_stringized_annotations
        self.assertEqual(annotations.get_annotations(isa), {'a': 'int', 'b': 'str'})
        self.assertEqual(annotations.get_annotations(isa.MyClass), {'a': 'int', 'b': 'str'})
        self.assertEqual(annotations.get_annotations(isa.function), {'a': 'int', 'b': 'str', 'return': 'MyClass'})
        self.assertEqual(annotations.get_annotations(isa.function2), {'a': 'int', 'b': "'str'", 'c': 'MyClass', 'return': 'MyClass'})
        self.assertEqual(annotations.get_annotations(isa.function3), {'a': "'int'", 'b': "'str'", 'c': "'MyClass'"})
        self.assertEqual(annotations.get_annotations(isa.UnannotatedClass), {})
        self.assertEqual(annotations.get_annotations(isa.unannotated_function), {})

        self.assertEqual(annotations.get_annotations(isa, eval_str=True), {'a': int, 'b': str})
        self.assertEqual(annotations.get_annotations(isa.MyClass, eval_str=True), {'a': int, 'b': str})
        self.assertEqual(annotations.get_annotations(isa.function, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(isa.function2, eval_str=True), {'a': int, 'b': 'str', 'c': isa.MyClass, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(isa.function3, eval_str=True), {'a': 'int', 'b': 'str', 'c': 'MyClass'})
        self.assertEqual(annotations.get_annotations(isa.UnannotatedClass, eval_str=True), {})
        self.assertEqual(annotations.get_annotations(isa.unannotated_function, eval_str=True), {})

        self.assertEqual(annotations.get_annotations(isa, eval_str=False), {'a': 'int', 'b': 'str'})
        self.assertEqual(annotations.get_annotations(isa.MyClass, eval_str=False), {'a': 'int', 'b': 'str'})
        self.assertEqual(annotations.get_annotations(isa.function, eval_str=False), {'a': 'int', 'b': 'str', 'return': 'MyClass'})
        self.assertEqual(annotations.get_annotations(isa.function2, eval_str=False), {'a': 'int', 'b': "'str'", 'c': 'MyClass', 'return': 'MyClass'})
        self.assertEqual(annotations.get_annotations(isa.function3, eval_str=False), {'a': "'int'", 'b': "'str'", 'c': "'MyClass'"})
        self.assertEqual(annotations.get_annotations(isa.UnannotatedClass, eval_str=False), {})
        self.assertEqual(annotations.get_annotations(isa.unannotated_function, eval_str=False), {})

        isa2 = inspect_stringized_annotations_2
        self.assertEqual(annotations.get_annotations(isa2), {})
        self.assertEqual(annotations.get_annotations(isa2, eval_str=True), {})
        self.assertEqual(annotations.get_annotations(isa2, eval_str=False), {})

        def times_three(fn):
            @functools.wraps(fn)
            def wrapper(a, b):
                return fn(a*3, b*3)
            return wrapper

        wrapped = times_three(isa.function)
        self.assertEqual(wrapped(1, 'x'), isa.MyClass(3, 'xxx'))
        self.assertIsNot(wrapped.__globals__, isa.function.__globals__)
        self.assertEqual(annotations.get_annotations(wrapped), {'a': 'int', 'b': 'str', 'return': 'MyClass'})
        self.assertEqual(annotations.get_annotations(wrapped, eval_str=True), {'a': int, 'b': str, 'return': isa.MyClass})
        self.assertEqual(annotations.get_annotations(wrapped, eval_str=False), {'a': 'int', 'b': 'str', 'return': 'MyClass'})

        # test that local namespace lookups work
        self.assertEqual(annotations.get_annotations(isa.MyClassWithLocalAnnotations), {'x': 'mytype'})
        self.assertEqual(annotations.get_annotations(isa.MyClassWithLocalAnnotations, eval_str=True), {'x': int})
