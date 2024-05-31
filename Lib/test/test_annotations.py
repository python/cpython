"""Tests for the annotations module."""

import annotations
import pickle
import unittest


class TestForwardRefFormat(unittest.TestCase):
    def test_closure(self):
        def inner(arg: x):
            pass

        anno = annotations.get_annotations(inner, format=annotations.Format.FORWARDREF)
        fwdref = anno["arg"]
        self.assertIsInstance(fwdref, annotations.ForwardRef)
        self.assertEqual(fwdref.__forward_arg__, 'x')
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
        fr = annotations.ForwardRef('set[Any]')
        self.assertFalse(hasattr(fr, '__name__'))
        self.assertFalse(hasattr(fr, '__qualname__'))
        self.assertEqual(fr.__module__, 'typing')
        # Forward refs are currently unpicklable.
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises(TypeError):
                pickle.dumps(fr, proto)
