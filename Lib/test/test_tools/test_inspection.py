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
        flat_a = [
            frame("leaf_a", lineno=flat_lines["leaf_a"]),
            frame("hot_a", lineno=flat_lines["hot_a"]),
        ]
        flat_crossed = [
            frame("hot_a", lineno=flat_lines["hot_a"]),
            frame("hot_b", lineno=flat_lines["hot_b"]),
        ]
        shared_a = [
            frame("shared_leaf", lineno=short_line),
            frame("a_wrapper"),
        ]
        shared_crossed = [
            frame("shared_leaf", lineno=short_line),
            frame("b_wrapper"),
        ]
        cases = [
            (snippets.classify_flat, flat_a, False),
            (snippets.classify_flat, flat_crossed, True),
            (
                snippets.classify_nested,
                frames("burn_a", "a_leaf", "a_parent"),
                False,
            ),
            (
                snippets.classify_nested,
                frames("a_parent", "a_leaf", "burn_a"),
                True,
            ),
            (snippets.classify_shared, shared_a, False),
            (snippets.classify_shared, shared_crossed, True),
            (snippets.classify_gen, frames("agen", "drv_a"), False),
            (snippets.classify_gen, frames("agen", "drv_b"), True),
            (snippets.classify_gen, frames("bgen", "drv_a"), True),
            (snippets.classify_gen, frames("agen"), False),
            (
                snippets.classify_gen,
                frames("agen", "agen", "drv_a"),
                False,
            ),

            (snippets.classify_recursion, frames("a", "a"), False),
            (snippets.classify_recursion, frames("a", "b"), True),
            (
                snippets.classify_async_running_task,
                (None, "hot", None, frames("leaf_hot")),
                False,
            ),
            (
                snippets.classify_async_running_task,
                (None, "hot", None, frames("leaf_rare")),
                True,
            ),
            (
                snippets.classify_code_object_reuse,
                [frame("func_a", filename="A_file.py")],
                False,
            ),
            (
                snippets.classify_code_object_reuse,
                [frame("func_a", filename="B_file.py")],
                True,
            ),
            (
                snippets.classify_oversized_chunk,
                [frame("big_a", filename="a.py")],
                False,
            ),
            (
                snippets.classify_oversized_chunk,
                [frame("big_b", filename="a.py")],
                True,
            ),
        ]
        for classifier, sample, expected in cases:
            with self.subTest(classifier=classifier.__name__, sample=sample):
                self.assertEqual(classifier(sample), expected)


if __name__ == "__main__":
    unittest.main()
