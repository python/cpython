from unittest import TestCase

from _pyrepl.render import (
    RenderLine,
    RenderedScreen,
    diff_render_lines,
    render_cells,
    with_active_prefix,
)


class TestRenderLine(TestCase):
    def test_from_rendered_text_groups_escape_with_visible_cells(self):
        line = RenderLine.from_rendered_text("\x1b[31ma\x1b[0mb")

        self.assertEqual(line.width, 2)
        self.assertEqual([cell.text for cell in line.cells], ["a", "b"])
        self.assertEqual([cell.prefix for cell in line.cells], ["\x1b[31m", "\x1b[0m"])
        self.assertEqual([cell.suffix for cell in line.cells], ["", ""])

    def test_from_rendered_text_keeps_trailing_escape_on_last_cell(self):
        line = RenderLine.from_rendered_text("\x1b[31ma\x1b[0m")

        self.assertEqual([cell.text for cell in line.cells], ["a", ""])
        self.assertEqual([cell.prefix for cell in line.cells], ["\x1b[31m", "\x1b[0m"])
        self.assertEqual([cell.suffix for cell in line.cells], ["", ""])

    def test_from_parts_normalizes_inline_trailing_escape(self):
        line = RenderLine.from_parts(
            ["\x1b[1;34md", "e", "f\x1b[0m", " "],
            [1, 1, 1, 1],
        )

        self.assertEqual([cell.text for cell in line.cells], ["d", "e", "f", "", " "])
        self.assertEqual(
            [cell.prefix for cell in line.cells],
            ["\x1b[1;34m", "", "", "\x1b[0m", ""],
        )
        self.assertEqual([cell.suffix for cell in line.cells], ["", "", "", "", ""])


class TestLineDiff(TestCase):
    def test_diff_render_lines_ignores_unchanged_ansi_prefix(self):
        old = RenderLine.from_rendered_text("\x1b[31ma\x1b[0mb")
        new = RenderLine.from_rendered_text("\x1b[31ma\x1b[0mc")

        diff = diff_render_lines(old, new)

        self.assertIsNotNone(diff)
        assert diff is not None
        self.assertEqual(diff.start_x, 1)
        self.assertEqual(diff.old_text, "\x1b[0mb")
        self.assertEqual(diff.new_text, "\x1b[0mc")

    def test_diff_render_lines_detects_single_cell_insertion(self):
        old = RenderLine.from_rendered_text("ab")
        new = RenderLine.from_rendered_text("acb")

        diff = diff_render_lines(old, new)

        self.assertIsNotNone(diff)
        assert diff is not None
        self.assertEqual(diff.start_x, 1)
        self.assertEqual(diff.old_text, "")
        self.assertEqual(diff.new_text, "c")

    def test_with_active_prefix_replays_color_for_mid_span_update(self):
        line = RenderLine.from_rendered_text("\x1b[31mabc\x1b[0m")

        replayed = with_active_prefix(line, 1, line.cells[1:2])

        self.assertEqual(replayed[0].terminal_text, "\x1b[31mb")

    def test_colored_append_only_emits_new_character_and_reset(self):
        old = RenderLine.from_rendered_text("\x1b[1mabc\x1b[0m")
        new = RenderLine.from_rendered_text("\x1b[1mabcd\x1b[0m")

        diff = diff_render_lines(old, new)

        self.assertIsNotNone(diff)
        assert diff is not None
        self.assertEqual(diff.start_x, 3)
        replayed = with_active_prefix(new, diff.start_cell, diff.new_cells)
        self.assertEqual(render_cells(replayed), "\x1b[1md\x1b[0m")

    def test_keyword_space_inserts_only_space_after_reset(self):
        old = RenderLine.from_parts(
            ["\x1b[1;34md", "e", "f\x1b[0m"],
            [1, 1, 1],
        )
        new = RenderLine.from_parts(
            ["\x1b[1;34md", "e", "f\x1b[0m", " "],
            [1, 1, 1, 1],
        )

        diff = diff_render_lines(old, new)

        self.assertIsNotNone(diff)
        assert diff is not None
        self.assertEqual(diff.start_x, 3)
        replayed = with_active_prefix(new, diff.start_cell, diff.new_cells)
        self.assertEqual(render_cells(replayed), " ")

    def test_rendered_screen_round_trips_screen_lines(self):
        screen = RenderedScreen.from_screen_lines(
            ["a", "\x1b[31mb\x1b[0m"],
            (0, 1),
        )

        self.assertEqual(screen.screen_lines, ("a", "\x1b[31mb\x1b[0m"))
