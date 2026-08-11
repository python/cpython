from unittest import TestCase
from _pyrepl.content import (
    ContentFragment,
    ContentLine,
    PromptContent,
    SourceLine,
)
from _pyrepl.layout import (
    LayoutMap,
    LayoutRow,
    layout_content_lines,
)


def _source(text, lineno=0, start_offset=0, has_newline=True, cursor_index=None):
    return SourceLine(lineno, text, start_offset, has_newline, cursor_index)


def _prompt(text=">>> ", width=4):
    return PromptContent((), text, width)


def _body_from_text(text):
    return tuple(ContentFragment(ch, 1) for ch in text)


def _content_line(text, prompt=None, has_newline=True, start_offset=0):
    if prompt is None:
        prompt = _prompt()
    body = _body_from_text(text)
    source = _source(text, start_offset=start_offset, has_newline=has_newline)
    return ContentLine(source, prompt, body)


class TestLayoutRow(TestCase):
    def test_width_basic(self):
        row = LayoutRow(4, (1, 1, 1))
        self.assertEqual(row.width, 7)

    def test_width_with_suffix(self):
        row = LayoutRow(4, (1, 1), suffix_width=1)
        self.assertEqual(row.width, 7)

    def test_screeninfo_without_suffix(self):
        row = LayoutRow(4, (1, 1, 1))
        prompt_w, widths = row.screeninfo
        self.assertEqual(prompt_w, 4)
        self.assertEqual(widths, [1, 1, 1])

    def test_screeninfo_with_suffix(self):
        row = LayoutRow(4, (1, 1), suffix_width=1)
        prompt_w, widths = row.screeninfo
        self.assertEqual(prompt_w, 4)
        self.assertEqual(widths, [1, 1, 1])


class TestLayoutMap(TestCase):
    def test_empty(self):
        lm = LayoutMap.empty()
        self.assertEqual(len(lm.rows), 1)
        self.assertEqual(lm.max_row(), 0)
        self.assertEqual(lm.max_column(0), 0)

    def test_screeninfo(self):
        lm = LayoutMap((
            LayoutRow(4, (1, 1, 1)),
            LayoutRow(0, (1, 1)),
        ))
        info = lm.screeninfo
        self.assertEqual(len(info), 2)
        self.assertEqual(info[0], (4, [1, 1, 1]))
        self.assertEqual(info[1], (0, [1, 1]))

    def test_max_column(self):
        lm = LayoutMap((LayoutRow(4, (1, 1, 1)),))
        self.assertEqual(lm.max_column(0), 7)

    def test_max_row(self):
        lm = LayoutMap((LayoutRow(0, ()), LayoutRow(0, ())))
        self.assertEqual(lm.max_row(), 1)

    def test_pos_to_xy_empty_rows(self):
        lm = LayoutMap(())
        self.assertEqual(lm.pos_to_xy(0), (0, 0))

    def test_pos_to_xy_single_row(self):
        lm = LayoutMap((LayoutRow(4, (1, 1, 1), buffer_advance=3),))
        self.assertEqual(lm.pos_to_xy(0), (4, 0))
        self.assertEqual(lm.pos_to_xy(1), (5, 0))
        self.assertEqual(lm.pos_to_xy(2), (6, 0))
        self.assertEqual(lm.pos_to_xy(3), (7, 0))

    def test_pos_to_xy_multi_row(self):
        lm = LayoutMap((
            LayoutRow(4, (1, 1, 1), buffer_advance=4),
            LayoutRow(4, (1, 1), buffer_advance=2),
        ))
        # First row: pos 0-3
        self.assertEqual(lm.pos_to_xy(0), (4, 0))
        self.assertEqual(lm.pos_to_xy(3), (7, 0))
        # Second row: pos 4-5
        self.assertEqual(lm.pos_to_xy(4), (4, 1))
        self.assertEqual(lm.pos_to_xy(5), (5, 1))

    def test_pos_to_xy_past_end_clamps(self):
        lm = LayoutMap((LayoutRow(4, (1, 1), buffer_advance=2),))
        self.assertEqual(lm.pos_to_xy(99), (6, 0))

    def test_pos_to_xy_skips_prompt_only_leading_rows(self):
        lm = LayoutMap((
            LayoutRow(0, (), buffer_advance=0),  # leading prompt-only row
            LayoutRow(4, (1, 1), buffer_advance=3),
        ))
        # pos 0 should skip the leading row and land on the real row
        self.assertEqual(lm.pos_to_xy(0), (4, 1))

    def test_pos_to_xy_with_suffix(self):
        lm = LayoutMap((
            LayoutRow(4, (1, 1), suffix_width=1, buffer_advance=2),
            LayoutRow(0, (1,), buffer_advance=2),
        ))
        # pos=2 fits within first row's char_widths (len=2), cursor at end
        self.assertEqual(lm.pos_to_xy(2), (6, 0))
        # pos=3 exceeds first row, lands on second row
        self.assertEqual(lm.pos_to_xy(3), (1, 1))

    def test_xy_to_pos_empty_rows(self):
        lm = LayoutMap(())
        self.assertEqual(lm.xy_to_pos(0, 0), 0)

    def test_xy_to_pos_single_row(self):
        lm = LayoutMap((LayoutRow(4, (1, 1, 1), buffer_advance=3),))
        self.assertEqual(lm.xy_to_pos(4, 0), 0)
        self.assertEqual(lm.xy_to_pos(5, 0), 1)
        self.assertEqual(lm.xy_to_pos(6, 0), 2)
        self.assertEqual(lm.xy_to_pos(7, 0), 3)

    def test_xy_to_pos_multi_row(self):
        lm = LayoutMap((
            LayoutRow(4, (1, 1, 1), buffer_advance=4),
            LayoutRow(4, (1, 1), buffer_advance=2),
        ))
        self.assertEqual(lm.xy_to_pos(4, 0), 0)
        self.assertEqual(lm.xy_to_pos(4, 1), 4)
        self.assertEqual(lm.xy_to_pos(5, 1), 5)

    def test_xy_to_pos_before_prompt_returns_zero(self):
        lm = LayoutMap((LayoutRow(4, (1, 1), buffer_advance=2),))
        self.assertEqual(lm.xy_to_pos(0, 0), 0)

    def test_xy_to_pos_with_zero_width_chars(self):
        # Simulates combining characters (zero-width) after a base char
        lm = LayoutMap((LayoutRow(4, (1, 0, 1), buffer_advance=3),))
        # x=5 is past the first char; trailing zero-width combining is included
        self.assertEqual(lm.xy_to_pos(5, 0), 2)

    def test_xy_to_pos_zero_width_skipped(self):
        lm = LayoutMap((LayoutRow(0, (0, 1, 1), buffer_advance=3),))
        # x=0: the zero-width char at index 0 is skipped, pos advances
        self.assertEqual(lm.xy_to_pos(0, 0), 1)

    def test_xy_to_pos_zero_width_before_target(self):
        # Zero-width char between two normal chars; target x is past it
        lm = LayoutMap((LayoutRow(0, (1, 0, 1), buffer_advance=3),))
        # x=2: passes char at x=0 (w=1), skips zero-width at x=1, lands at x=2
        self.assertEqual(lm.xy_to_pos(2, 0), 3)

    def test_xy_to_pos_trailing_all_zero_width(self):
        # All remaining chars from cursor position are zero-width
        lm = LayoutMap((LayoutRow(0, (1, 0), buffer_advance=2),))
        # x=1: past first char, trailing loop exhausts (all remaining are 0-width)
        self.assertEqual(lm.xy_to_pos(1, 0), 2)


class TestLayoutContentLines(TestCase):
    def test_zero_width_returns_empty(self):
        result = layout_content_lines((), 0, 0)
        self.assertEqual(result.wrapped_rows, ())
        self.assertEqual(result.layout_map.rows, ())

    def test_negative_width_returns_empty(self):
        result = layout_content_lines((), -1, 0)
        self.assertEqual(result.wrapped_rows, ())

    def test_single_short_line(self):
        line = _content_line("abc")
        result = layout_content_lines((line,), 80, 0)

        self.assertEqual(len(result.wrapped_rows), 1)
        row = result.wrapped_rows[0]
        self.assertEqual(row.prompt_text, ">>> ")
        self.assertEqual(row.prompt_width, 4)
        self.assertEqual(row.suffix, "")
        self.assertEqual(row.buffer_advance, 4)  # 3 chars + newline

    def test_single_line_no_newline(self):
        line = _content_line("ab", has_newline=False)
        result = layout_content_lines((line,), 80, 0)

        self.assertEqual(len(result.wrapped_rows), 1)
        self.assertEqual(result.wrapped_rows[0].buffer_advance, 2)

    def test_empty_body(self):
        source = _source("", has_newline=True)
        prompt = _prompt()
        line = ContentLine(source, prompt, ())
        result = layout_content_lines((line,), 80, 0)

        self.assertEqual(len(result.wrapped_rows), 1)
        self.assertEqual(result.wrapped_rows[0].buffer_advance, 1)  # just newline

    def test_line_wraps(self):
        # prompt ">>> " is 4 wide, terminal is 10 wide, so 6 chars fit per row
        line = _content_line("abcdefgh")  # 8 chars, needs 2 rows
        result = layout_content_lines((line,), 10, 0)

        self.assertEqual(len(result.wrapped_rows), 2)
        first, second = result.wrapped_rows
        self.assertEqual(first.prompt_text, ">>> ")
        self.assertEqual(first.suffix, "\\")
        self.assertEqual(first.suffix_width, 1)
        # First row: 10 - 4(prompt) - 1(suffix) = 5 chars
        self.assertEqual(first.buffer_advance, 5)
        # Second row: continuation with no prompt
        self.assertEqual(second.prompt_text, "")
        self.assertEqual(second.prompt_width, 0)
        self.assertEqual(second.buffer_advance, 4)  # remaining 3 + newline
        self.assertEqual(second.suffix, "")

    def test_wrapping_forces_progress(self):
        # When a single character is wider than available space, force 1 char
        prompt = _prompt("P", 1)
        body = (ContentFragment("W", 1),)
        source = _source("W", has_newline=False)
        line = ContentLine(source, prompt, body)
        # width=2 means prompt(1) + char(1) = 2, which fits (< width would be
        # false for width=2 since 1+1 >= 2), so it wraps but forces progress
        result = layout_content_lines((line,), 2, 0)

        self.assertEqual(len(result.wrapped_rows), 1)
        self.assertEqual(result.wrapped_rows[0].buffer_advance, 1)

    def test_layout_map_matches_wrapped_rows(self):
        line = _content_line("abc")
        result = layout_content_lines((line,), 80, 0)

        self.assertEqual(len(result.layout_map.rows), len(result.wrapped_rows))
        self.assertEqual(result.layout_map.rows[0].prompt_width, 4)
        self.assertEqual(result.layout_map.rows[0].char_widths, (1, 1, 1))

    def test_line_end_offsets(self):
        line1 = _content_line("ab")
        line2 = _content_line("cd")
        result = layout_content_lines((line1, line2), 80, 0)

        self.assertEqual(len(result.line_end_offsets), 2)
        # line1: 2 chars + 1 newline = offset 3
        self.assertEqual(result.line_end_offsets[0], 3)
        # line2: offset 3 + 2 chars + 1 newline = 6
        self.assertEqual(result.line_end_offsets[1], 6)

    def test_start_offset_shifts_offsets(self):
        line = _content_line("ab")
        result = layout_content_lines((line,), 80, 10)

        self.assertEqual(result.line_end_offsets[0], 13)

    def test_multiple_lines(self):
        line1 = _content_line("abc")
        line2 = _content_line("de")
        result = layout_content_lines((line1, line2), 80, 0)

        self.assertEqual(len(result.wrapped_rows), 2)
        self.assertEqual(result.wrapped_rows[0].buffer_advance, 4)  # abc + \n
        self.assertEqual(result.wrapped_rows[1].buffer_advance, 3)  # de + \n

    def test_leading_prompt_lines(self):
        leading = (ContentFragment("header", 6),)
        prompt = PromptContent(leading, ">>> ", 4)
        body = _body_from_text("x")
        source = _source("x", has_newline=False)
        line = ContentLine(source, prompt, body)
        result = layout_content_lines((line,), 80, 0)

        # Leading line + body line
        self.assertEqual(len(result.wrapped_rows), 2)
        # Leading row has the fragment but no prompt
        self.assertEqual(result.wrapped_rows[0].fragments, leading)
        # Body row has prompt and content
        self.assertEqual(result.wrapped_rows[1].prompt_text, ">>> ")

    def test_wrapped_line_layout_rows_have_suffix(self):
        line = _content_line("abcdefgh")
        result = layout_content_lines((line,), 10, 0)

        first_layout = result.layout_map.rows[0]
        self.assertEqual(first_layout.suffix_width, 1)
        second_layout = result.layout_map.rows[1]
        self.assertEqual(second_layout.suffix_width, 0)

    def test_pos_to_xy_through_layout(self):
        line = _content_line("abc")
        result = layout_content_lines((line,), 80, 0)
        lm = result.layout_map

        self.assertEqual(lm.pos_to_xy(0), (4, 0))  # after prompt
        self.assertEqual(lm.pos_to_xy(1), (5, 0))
        self.assertEqual(lm.pos_to_xy(2), (6, 0))
        self.assertEqual(lm.pos_to_xy(3), (7, 0))  # end of line
