"""Tests for scripts in Tools/inspection."""

import unittest
from types import SimpleNamespace

from test.test_tools import imports_under_tool, skip_if_missing


skip_if_missing("inspection")
with imports_under_tool("inspection"):
    import snippets


def frames(*names):
    return [SimpleNamespace(funcname=name) for name in names]


class ClassifierTests(unittest.TestCase):
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
