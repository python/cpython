from unittest import TestCase

from _pyrepl.utils import (
    str_width, wlen, prev_next_window, gen_colors,
    find_matching_delimiter,
)


class TestUtils(TestCase):
    def test_incomplete_input_colors_and_delimiters(self):
        cases = [
            ('"unfinished', [(0, 10, "string")]),
            ('f"unfinished', [(0, 1, "string"), (2, 11, "string")]),
            ('t"unfinished', [(0, 1, "string"), (2, 11, "string")]),
            ('"""unfinished', [(0, 12, "string")]),
            ('f"""unfinished', [(0, 3, "string"), (4, 13, "string")]),
            ('t"""unfinished', [(0, 3, "string"), (4, 13, "string")]),
        ]
        for buffer, expected_colors in cases:
            with self.subTest(buffer=buffer):
                self.assertEqual(
                    [(c.span.start, c.span.end, c.tag)
                     for c in gen_colors(buffer)],
                    expected_colors,
                )
                quote = buffer.index('"')
                self.assertIsNone(find_matching_delimiter(buffer, quote))

        buffer = '(value) """unfinished'
        self.assertEqual(
            [(c.span.start, c.span.end, c.tag)
             for c in gen_colors(buffer)],
            [(0, 0, "op"), (6, 6, "op"), (8, 20, "string")],
        )
        self.assertEqual(find_matching_delimiter(buffer, 0), (6, 7))
        quote = buffer.index('"')
        self.assertIsNone(find_matching_delimiter(buffer, quote))

    def test_find_matching_delimiter(self):
        cases = [
            ("(value)", 0, (6, 7)),
            ("(value)", 6, (0, 1)),
            ("(樂)", 0, (2, 3)),
            ("(樂)", 2, (0, 1)),
            ("[({value})]", 2, (8, 9)),
            ("[({value})]", 8, (2, 3)),
            ("call(\n    value\n)", 4, (16, 17)),
            # The cursor may be just after a bracket.
            ("(value)", 7, (0, 1)),
            # Prefer a bracket at the cursor over one immediately before it.
            ("()[]", 1, (0, 1)),
            ("()[]", 2, (3, 4)),
            # An unmatched bracket at the cursor takes precedence over one
            # immediately before it.
            ("()(", 2, None),
            # A later lexical error or mismatched bracket does not discard an
            # earlier complete pair.
            ("(value) [", 0, (6, 7)),
            ("(value) ([)]", 0, (6, 7)),
        ]
        for buffer, pos, expected in cases:
            with self.subTest(buffer=buffer, pos=pos):
                self.assertEqual(find_matching_delimiter(buffer, pos), expected)

    def test_find_matching_delimiter_ignores_strings_and_comments(self):
        cases = [
            ('"( text )"', 1, None),
            ("# ( comment )", 3, None),
            ('"a \\\" (still string)"', 8, None),
            ("(\"[\" + value)", 0, (12, 13)),
        ]
        for buffer, pos, expected in cases:
            with self.subTest(buffer=buffer, pos=pos):
                self.assertEqual(find_matching_delimiter(buffer, pos), expected)

    def test_find_matching_delimiter_f_string_expression(self):
        buffer = 'f"{mapping[\'key\']}"'
        self.assertEqual(find_matching_delimiter(buffer, 16), (10, 11))

        # Python 3.12+ permits a string with the same quote style inside an
        # f-string expression; the inner pair and outer pair stay distinct.
        buffer = 'f"{mapping["key"]}"'
        inner_open = buffer.index('"', 2)
        inner_close = buffer.index('"', inner_open + 1)
        outer_close = buffer.rindex('"')
        self.assertEqual(find_matching_delimiter(buffer, inner_open),
                         (inner_close, inner_close + 1))
        self.assertEqual(find_matching_delimiter(buffer, 1),
                         (outer_close, outer_close + 1))

    def test_find_matching_delimiter_f_and_t_string_braces(self):
        for buffer in ('f"{{literal}}"', 't"{{literal}}"'):
            with self.subTest(buffer=buffer):
                self.assertIsNone(find_matching_delimiter(buffer, 2))

        buffer = 't"{mapping[1]}"'
        self.assertEqual(
            find_matching_delimiter(buffer, buffer.index("[")),
            (buffer.index("]"), buffer.index("]") + 1),
        )

    def test_find_matching_delimiter_t_string_quotes(self):
        buffer = 't"{mapping[1]}"'
        opening = buffer.index('"')
        closing = buffer.rindex('"')
        self.assertEqual(find_matching_delimiter(buffer, opening),
                         (closing, closing + 1))
        self.assertEqual(find_matching_delimiter(buffer, closing),
                         (opening, opening + 1))

    def test_find_matching_delimiter_formatted_string_quotes(self):
        for prefix in ("f", "t", "fr", "rt"):
            for quote in ("'", '"', "'" * 3, '"' * 3):
                buffer = prefix + quote + "value" + quote
                width = len(quote)
                opening = len(prefix)
                closing = len(buffer) - width
                with self.subTest(buffer=buffer):
                    for pos in range(opening, opening + width + 1):
                        self.assertEqual(find_matching_delimiter(buffer, pos),
                                         (closing, closing + width))
                    for pos in range(closing, closing + width + 1):
                        self.assertEqual(find_matching_delimiter(buffer, pos),
                                         (opening, opening + width))

        for prefix in ("f", "t"):
            buffer = prefix + '"{' + prefix + '"value"}"'
            with self.subTest(buffer=buffer):
                self.assertEqual(find_matching_delimiter(buffer, 1),
                                 (len(buffer) - 1, len(buffer)))
                self.assertEqual(find_matching_delimiter(buffer, 4), (10, 11))
                self.assertEqual(find_matching_delimiter(buffer, 10), (4, 5))

    def test_find_matching_delimiter_complete_quote_before_error(self):
        for buffer in ('"value" "unfinished', 'f"value" "unfinished'):
            opening = buffer.index('"')
            closing = buffer.index('"', opening + 1)
            with self.subTest(buffer=buffer):
                self.assertEqual(find_matching_delimiter(buffer, opening),
                                 (closing, closing + 1))

    def test_find_matching_delimiter_ignores_unterminated_strings(self):
        cases = [
            ('"(fake)', 2),
            ('"""(fake)', 3),
            ("'''[fake]", 3),
        ]
        for buffer, pos in cases:
            with self.subTest(buffer=buffer):
                self.assertIsNone(find_matching_delimiter(buffer, pos))

    def test_find_matching_delimiter_string_vertical_tab_and_form_feed(self):
        for control in ("\v", "\f"):
            buffer = f'"before{control}after" + (value)'
            opening = buffer.index("(")
            with self.subTest(control=repr(control)):
                self.assertEqual(
                    find_matching_delimiter(buffer, opening),
                    (buffer.index(")"), buffer.index(")") + 1),
                )

    def test_find_matching_delimiter_unmatched_or_mismatched(self):
        for buffer, pos in [("(", 0), ("([)]", 0), ("x)", 1), ("()(", 2)]:
            with self.subTest(buffer=buffer, pos=pos):
                self.assertIsNone(find_matching_delimiter(buffer, pos))

    def test_find_matching_delimiter_string_quotes(self):
        for quote in ("'", '"', "'''", '"""'):
            buffer = quote + "value" + quote
            width = len(quote)
            opening = 0
            closing = len(buffer) - width
            with self.subTest(buffer=buffer):
                for pos in range(opening, opening + width):
                    self.assertEqual(find_matching_delimiter(buffer, pos),
                                     (closing, closing + width))
                self.assertEqual(find_matching_delimiter(buffer, opening + width),
                                 (closing, closing + width))
                for pos in range(closing, closing + width):
                    self.assertEqual(find_matching_delimiter(buffer, pos),
                                     (opening, opening + width))
                self.assertEqual(find_matching_delimiter(buffer, closing + width),
                                 (opening, opening + width))

    def test_find_matching_delimiter_complete_string_tokens(self):
        for token in (
            'r"raw value"', 'b"bytes"', 'u"unicode"',
            'br"raw bytes"', 'Rb"raw bytes"', '"a"',
        ):
            with self.subTest(token=token):
                # Prefixes do not change the quoted token's delimiter span.
                opening = token.index('"')
                closing = token.rindex('"')
                for pos in range(opening, opening + 2):
                    self.assertEqual(find_matching_delimiter(token, pos),
                                     (closing, closing + 1))
                for pos in range(closing, closing + 2):
                    self.assertEqual(find_matching_delimiter(token, pos),
                                     (opening, opening + 1))

        for token in ('""', "''"):
            with self.subTest(token=token):
                self.assertEqual(find_matching_delimiter(token, 0), (1, 2))
                self.assertEqual(find_matching_delimiter(token, 2), (0, 1))

        adjacent = '"first" "second"'
        with self.subTest(token=adjacent):
            self.assertEqual(find_matching_delimiter(adjacent, 0), (6, 7))
            self.assertEqual(find_matching_delimiter(adjacent, 8), (15, 16))

    def test_find_matching_delimiter_raw_candidate_wins_previous(self):
        # The raw delimiter under the cursor takes precedence even when it is
        # unmatched or belongs to a string literal.
        self.assertIsNone(find_matching_delimiter('()"', 2))
        self.assertEqual(find_matching_delimiter('("x"', 1), (3, 4))

    def test_find_matching_delimiter_unfinished_or_escaped_quotes(self):
        for buffer in ('"unfinished', "'unfinished", '"escaped \\\" quote'):
            with self.subTest(buffer=buffer):
                self.assertIsNone(find_matching_delimiter(buffer, 0))

        buffer = '"escaped \\\" quote"'
        escaped_quote = buffer.index('"', 1)
        with self.subTest(buffer=buffer, pos=escaped_quote):
            self.assertIsNone(find_matching_delimiter(buffer, escaped_quote))

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
