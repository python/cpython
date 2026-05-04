from unittest import TestCase

from _pyrepl.utils import str_width, wlen, prev_next_window, gen_colors


class TestUtils(TestCase):
    def test_str_width(self):
        characters = [
            'a',
            '1',
            '_',
            '!',
            '\x1a',
            '\u263A',
            '\uffb9',
            '\N{LATIN SMALL LETTER E WITH ACUTE}',  # é
            '\N{LATIN SMALL LETTER E WITH CEDILLA}', # ȩ
            '\u00ad',
        ]
        for c in characters:
            self.assertEqual(str_width(c), 1)

        zero_width_characters = [
            '\N{COMBINING ACUTE ACCENT}',
            '\N{ZERO WIDTH JOINER}',
        ]
        for c in zero_width_characters:
            with self.subTest(character=c):
                self.assertEqual(str_width(c), 0)

        characters = [chr(99989), chr(99999)]
        for c in characters:
            self.assertEqual(str_width(c), 2)

    def test_wlen(self):
        for c in ['a', 'b', '1', '!', '_']:
            self.assertEqual(wlen(c), 1)
        self.assertEqual(wlen('\x1a'), 2)

        char_east_asian_width_N = chr(3800)
        self.assertEqual(wlen(char_east_asian_width_N), 1)
        char_east_asian_width_W = chr(4352)
        self.assertEqual(wlen(char_east_asian_width_W), 2)

        self.assertEqual(wlen('hello'), 5)
        self.assertEqual(wlen('hello' + '\x1a'), 7)
        self.assertEqual(wlen('e\N{COMBINING ACUTE ACCENT}'), 1)
        self.assertEqual(wlen('a\N{ZERO WIDTH JOINER}b'), 2)

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
