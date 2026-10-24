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
            (
                "case +1",
                [
                    ("case", "soft_keyword"),
                    ("+", "op"),
                    ("1", "number"),
                ],
            ),
            (
                "match +1",
                [
                    ("match", "soft_keyword"),
                    ("+", "op"),
                    ("1", "number"),
                ],
            ),
            (
                "match -1",
                [
                    ("match", "soft_keyword"),
                    ("-", "op"),
                    ("1", "number"),
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

    def test_gen_colors_soft_keywords_after_blank_lines(self):
        cases = [
            (
                "def f(): pass\n",
                "match -1:\n    case _: pass",
                ["match", "case", "_"],
            ),
            (
                "def f(): pass\n",
                "match value:\n    case _: pass",
                ["match", "case", "_"],
            ),
            (
                "match value:\n    case 0: pass\n",
                "    case -1: pass",
                ["match", "case", "case"],
            ),
            (
                "match value:\n    case 0: pass\n",
                "    case other: pass",
                ["match", "case", "case"],
            ),
            ("def f(): pass\n", "type Alias = int", ["type"]),
            ("def f(): pass\n", "lazy import mod", ["lazy"]),
            ("def f(): pass\n", "lazy from mod import name", ["lazy"]),
        ]
        for prefix, statement, expected in cases:
            for blank_line in ("\n", "    \n", "# comment\n"):
                code = prefix + blank_line + statement
                with self.subTest(code=code):
                    actual = [
                        code[color.span.start:color.span.end + 1]
                        for color in gen_colors(code)
                        if color.tag == "soft_keyword"
                    ]
                    self.assertEqual(actual, expected)

    def test_gen_colors_match_after_dedent(self):
        for subject in ("-1", "value"):
            for separator in ("", "\n", "# comment\n"):
                code = (
                    "if x:\n    pass\n"
                    f"{separator}match {subject}:\n    case _: pass"
                )
                with self.subTest(code=code):
                    actual = [
                        code[color.span.start:color.span.end + 1]
                        for color in gen_colors(code)
                        if color.tag == "soft_keyword"
                    ]
                    self.assertEqual(actual, ["match", "case", "_"])

    def test_gen_colors_soft_keyword_names_in_brackets(self):
        for opening, closing in ("()", "[]", "{}"):
            for name in ("match", "case"):
                code = f"{opening}\n{name} - 1\n{closing}"
                with self.subTest(code=code):
                    self.assertNotIn(
                        "soft_keyword", [color.tag for color in gen_colors(code)]
                    )

    def test_gen_colors_command_highlighting(self):
        cases = [
            # highlights bare command names (after stripping whitespaces)
            ("exit", [("exit", "command")]),
            ("quit", [("quit", "command")]),
            ("copyright", [("copyright", "command")]),
            ("help", [("help", "command")]),
            ("clear", [("clear", "command")]),
            ("  clear ", [("clear", "command")]),
            # no highlight when not the only token on the line
            ("x = exit", [("=", "op"), ("exit", "builtin")]),
            ("obj.exit", [(".", "op")]),
            # falls through to builtin when called as function or used in expression
            ("exit()", [("exit", "builtin"), ("(", "op"), (")", "op")]),
            ("quit(0)", [("quit", "builtin"), ("(", "op"), ("0", "number"), (")", "op")]),
            ("print(exit)", [("print", "builtin"), ("(", "op"), ("exit", "builtin"), (")", "op")]),
        ]
        for code, expected_highlights in cases:
            with self.subTest(code=code):
                colors = list(gen_colors(code))
                actual_highlights = []
                for color in colors:
                    span_text = code[color.span.start:color.span.end + 1]
                    actual_highlights.append((span_text, color.tag))
                self.assertEqual(actual_highlights, expected_highlights)
