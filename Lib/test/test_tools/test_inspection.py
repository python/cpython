"""Tests for snippets in Tools/inspection."""

import unittest
from types import SimpleNamespace

from test.test_tools import imports_under_tool, skip_if_missing


skip_if_missing("inspection")
with imports_under_tool("inspection"):
    import snippets


def frame(funcname, *, filename="", lineno=None):
    location = SimpleNamespace(lineno=lineno) if lineno is not None else None
    return SimpleNamespace(
        funcname=funcname,
        filename=filename,
        location=location,
    )


def frames(*names):
    return [frame(name) for name in names]


class ClassifierTests(unittest.TestCase):
    def test_classifiers(self):
        flat_lines = snippets.FLAT_ALTERNATING_LINES
        short_line = min(snippets.SHARED_LEAF_SHORT_LINES)
        cases = [
            (
                snippets.classify_flat,
                [
                    frame("leaf_a", lineno=flat_lines["leaf_a"]),
                    frame("hot_a", lineno=flat_lines["hot_a"]),
                ],
                [
                    frame("hot_a", lineno=flat_lines["hot_a"]),
                    frame("hot_b", lineno=flat_lines["hot_b"]),
                ],
            ),
            (
                snippets.classify_nested,
                frames("burn_a", "a_leaf", "a_parent"),
                frames("a_parent", "a_leaf", "burn_a"),
            ),
            (
                snippets.classify_shared,
                [
                    frame("shared_leaf", lineno=short_line),
                    frame("a_wrapper"),
                ],
                [
                    frame("shared_leaf", lineno=short_line),
                    frame("b_wrapper"),
                ],
            ),
            (
                snippets.classify_recursion,
                frames("a", "a"),
                frames("a", "b"),
            ),
            (
                snippets.classify_async_running_task,
                (None, "hot", None, frames("leaf_hot")),
                (None, "hot", None, frames("leaf_rare")),
            ),
            (
                snippets.classify_code_object_reuse,
                [frame("func_a", filename="A_file.py")],
                [frame("func_a", filename="B_file.py")],
            ),
            (
                snippets.classify_oversized_chunk,
                [frame("big_a", filename="a.py")],
                [frame("big_b", filename="a.py")],
            ),
        ]
        for classifier, unrecognized, recognized in cases:
            with self.subTest(classifier=classifier.__name__):
                self.assertFalse(classifier(unrecognized))
                self.assertTrue(classifier(recognized))

    def test_classify_gen(self):
        cases = [
            (("agen", "drv_a"), False),
            (("agen", "drv_b"), True),
            (("bgen", "drv_a"), True),
            (("agen",), False),
            (("agen", "agen", "drv_a"), False),
        ]
        for names, impossible in cases:
            with self.subTest(names=names):
                self.assertEqual(
                    snippets.classify_gen(frames(*names)), impossible
                )


if __name__ == "__main__":
    unittest.main()
