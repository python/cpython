"""STRING-format edge cases for annotationlib (gh-157056)."""

import inspect
import unittest
from unittest.mock import patch

from annotationlib import Format, get_annotations, type_repr


class TestStringFormatSpecialAnnotations(unittest.TestCase):
    def test_dict_comprehension_annotation(self):
        def f(x: {k: v for k, v in items}):
            pass

        self.assertEqual(
            get_annotations(f, format=Format.STRING),
            {"x": "{k: v for k, v in items}"},
        )

    def test_lambda_annotation(self):
        def g(x: lambda q: q):
            pass

        g_anno = get_annotations(g, format=Format.STRING)
        self.assertEqual(g_anno, {"x": "lambda q: q"})
        self.assertNotIn("0x", g_anno["x"].lower())

    def test_generator_expression_annotation(self):
        def h(x: (w for w in seq)):
            pass

        h_anno = get_annotations(h, format=Format.STRING)
        self.assertEqual(h_anno, {"x": "(w for w in seq)"})
        self.assertNotIn("0x", h_anno["x"].lower())

    def test_mixed_annotations(self):
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

    def test_quoted_annotation_not_requoted_on_unpack_fallback(self):
        def f(a: "int", b: {k: v for k, v in items}):
            pass

        self.assertEqual(
            get_annotations(f, format=Format.STRING),
            {"a": "int", "b": "{k: v for k, v in items}"},
        )

    def test_nested_lambda_uses_source_and_has_no_address(self):
        def f(x: [lambda q: q]):
            pass

        anno = get_annotations(f, format=Format.STRING)
        self.assertEqual(anno, {"x": "[lambda q: q]"})
        self.assertNotIn("0x", anno["x"].lower())

    def test_class_dictcomp_keeps_conditional_annotations(self):
        class C:
            a: int
            b: {k: v for k, v in items}
            if True:
                c: str

        self.assertEqual(
            get_annotations(C, format=Format.STRING),
            {"a": "int", "b": "{k: v for k, v in items}", "c": "str"},
        )

    def test_simple_string_format_does_not_read_source(self):
        def f(x: int) -> str:
            pass

        with patch.object(inspect, "getsource") as mocked:
            self.assertEqual(
                get_annotations(f, format=Format.STRING),
                {"x": "int", "return": "str"},
            )
            mocked.assert_not_called()

    def test_exec_without_source_still_raises_on_dictcomp(self):
        ns = {}
        exec("def f(x: {k: v for k, v in items}): pass", ns)
        with self.assertRaises(ValueError):
            get_annotations(ns["f"], format=Format.STRING)

    def test_type_repr_lambda_and_genexpr_have_no_address(self):
        lam = (lambda q: q)
        self.assertTrue(type_repr(lam).endswith("<lambda>"))
        self.assertNotIn("0x", type_repr(lam).lower())
        gen = (w for w in ())
        self.assertTrue(type_repr(gen).endswith("<genexpr>"))
        self.assertNotIn("0x", type_repr(gen).lower())


if __name__ == "__main__":
    unittest.main()
