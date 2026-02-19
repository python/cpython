from unittest import TestCase

from _pyrepl.utils import prev_next_window, gen_colors


class TestUtils(TestCase):
    def test_prev_next_window(self):
        def gen_normal():
            yield 1
            yield 2
            yield 3
            yield 4

        pnw = prev_next_window(gen_normal())
        self.assertEqual(next(pnw), (None, 1, 2))
        self.assertEqual(next(pnw), (1, 2, 3))
        self.assertEqual(next(pnw), (2, 3, 4))
        self.assertEqual(next(pnw), (3, 4, None))
        with self.assertRaises(StopIteration):
            next(pnw)

        def gen_short():
            yield 1

        pnw = prev_next_window(gen_short())
        self.assertEqual(next(pnw), (None, 1, None))
        with self.assertRaises(StopIteration):
            next(pnw)

        def gen_raise():
            yield from gen_normal()
            1/0

        pnw = prev_next_window(gen_raise())
        self.assertEqual(next(pnw), (None, 1, 2))
        self.assertEqual(next(pnw), (1, 2, 3))
        self.assertEqual(next(pnw), (2, 3, 4))
        self.assertEqual(next(pnw), (3, 4, None))
        with self.assertRaises(ZeroDivisionError):
            next(pnw)

    def test_gen_colors_keyword_highlighting(self):
        cases = [
            # no highlights
            ("a.set", [(".", "op")]),
            ("obj.list", [(".", "op")]),
            ("obj.match", [(".", "op")]),
            ("b. \\\n format", [(".", "op")]),
            ("lazy", []),
            ("lazy()", [('(', 'op'), (')', 'op')]),
            # highlights
            ("set", [("set", "builtin")]),
            ("list", [("list", "builtin")]),
            ("    \n dict", [("dict", "builtin")]),
            (
                "    lazy import",
                [("lazy", "soft_keyword"), ("import", "keyword")],
            ),
            (
                "lazy from cool_people import pablo",
                [
                    ("lazy", "soft_keyword"),
                    ("from", "keyword"),
                    ("import", "keyword"),
                ],
            ),
            (
                "if sad: lazy import happy",
                [
                    ("if", "keyword"),
                    (":", "op"),
                    ("lazy", "soft_keyword"),
                    ("import", "keyword"),
                ],
            ),
            (
                "pass; lazy import z",
                [
                    ("pass", "keyword"),
                    (";", "op"),
                    ("lazy", "soft_keyword"),
                    ("import", "keyword"),
                ],
            ),
        ]
        for code, expected_highlights in cases:
            with self.subTest(code=code):
                colors = list(gen_colors(code))
                # Extract (text, tag) pairs for comparison
                actual_highlights = []
                for color in colors:
                    span_text = code[color.span.start:color.span.end + 1]
                    actual_highlights.append((span_text, color.tag))
                self.assertEqual(actual_highlights, expected_highlights)
