"""Tests for snippets in Tools/inspection."""

import unittest
from types import SimpleNamespace

from test.test_tools import imports_under_tool, skip_if_missing


skip_if_missing("inspection")
with imports_under_tool("inspection"):
    import snippets


def frame(funcname, *, filename="", lineno=None):
    return SimpleNamespace(
        funcname=funcname,
        filename=filename,
        location=SimpleNamespace(lineno=lineno),
    )


class ClassifierTests(unittest.TestCase):
    def test_recognized_impossible_patterns(self):
        lines = snippets.FLAT_ALTERNATING_LINES
        cases = [
            (
                snippets.classify_flat,
                [
                    frame("hot_a", lineno=lines["hot_a"]),
                    frame("hot_b", lineno=lines["hot_b"]),
                ],
            ),
            (snippets.classify_nested, [frame("a_leaf")]),
            (snippets.classify_shared, [frame("shared_leaf")]),
            (
                snippets.classify_gen,
                [frame("agen"), frame("drv_b")],
            ),
            (
                snippets.classify_recursion,
                [frame("a"), frame("b")],
            ),
            (
                snippets.classify_async_running_task,
                (None, "hot", None, [frame("leaf_rare")]),
            ),
            (
                snippets.classify_code_object_reuse,
                [frame("func_a", filename="B_file.py")],
            ),
            (
                snippets.classify_oversized_chunk,
                [frame("big_b", filename="a.py")],
            ),
        ]
        for classifier, sample in cases:
            with self.subTest(classifier=classifier.__name__):
                self.assertTrue(classifier(sample))

    def test_unrecognized_gen_patterns(self):
        for names in [("agen",), ("agen", "agen", "drv_a")]:
            with self.subTest(names=names):
                self.assertFalse(
                    snippets.classify_gen([frame(name) for name in names])
                )


if __name__ == "__main__":
    unittest.main()
