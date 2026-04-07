from unittest import TestCase
from _pyrepl.render import (
    RenderLine,
    RenderedScreen,
    StyleRef,
    diff_render_lines,
    render_cells,
)


class TestRenderLine(TestCase):
    def test_from_rendered_text_parses_style_state(self):
        line = RenderLine.from_rendered_text("\x1b[31ma\x1b[0mb")

        self.assertEqual(line.width, 2)
        self.assertEqual([cell.text for cell in line.cells], ["a", "b"])
        self.assertEqual([cell.style.sgr for cell in line.cells], ["\x1b[31m", ""])
        self.assertEqual([cell.controls for cell in line.cells], [(), ()])

    def test_from_rendered_text_round_trips_trailing_reset(self):
        line = RenderLine.from_rendered_text("\x1b[31ma\x1b[0m")

        self.assertEqual([cell.text for cell in line.cells], ["a"])
        self.assertEqual([cell.style.sgr for cell in line.cells], ["\x1b[31m"])
        self.assertEqual(line.text, "\x1b[31ma\x1b[0m")

    def test_from_parts_accepts_style_refs(self):
        line = RenderLine.from_parts(
            ["d", "e", "f", " "],
            [1, 1, 1, 1],
            [StyleRef.from_sgr("\x1b[1;34m")] * 3 + [None],
        )

        self.assertEqual([cell.text for cell in line.cells], ["d", "e", "f", " "])
        self.assertEqual(
            [cell.style.sgr for cell in line.cells],
            ["\x1b[1;34m", "\x1b[1;34m", "\x1b[1;34m", ""],
        )
        self.assertEqual(line.text, "\x1b[1;34mdef\x1b[0m ")


class TestLineDiff(TestCase):
    def test_diff_render_lines_ignores_unchanged_ansi_prefix(self):
        old = RenderLine.from_rendered_text("\x1b[31ma\x1b[0mb")
        new = RenderLine.from_rendered_text("\x1b[31ma\x1b[0mc")

        diff = diff_render_lines(old, new)

        self.assertIsNotNone(diff)
        assert diff is not None
        self.assertEqual(diff.start_x, 1)
        self.assertEqual(diff.old_text, "b")
        self.assertEqual(diff.new_text, "c")

    def test_diff_render_lines_detects_single_cell_insertion(self):
        old = RenderLine.from_rendered_text("ab")
        new = RenderLine.from_rendered_text("acb")

        diff = diff_render_lines(old, new)

        self.assertIsNotNone(diff)
        assert diff is not None
        self.assertEqual(diff.start_x, 1)
        self.assertEqual(diff.old_text, "")
        self.assertEqual(diff.new_text, "c")

    def test_colored_append_only_emits_new_character_and_reset(self):
        old = RenderLine.from_rendered_text("\x1b[1mabc\x1b[0m")
        new = RenderLine.from_rendered_text("\x1b[1mabcd\x1b[0m")

        diff = diff_render_lines(old, new)

        self.assertIsNotNone(diff)
        assert diff is not None
        self.assertEqual(diff.start_x, 3)
        self.assertEqual(render_cells(diff.new_cells), "\x1b[1md\x1b[0m")

    def test_keyword_space_inserts_only_space_after_reset(self):
        old = RenderLine.from_parts(
            ["d", "e", "f"],
            [1, 1, 1],
            ["keyword", "keyword", "keyword"],
        )
        new = RenderLine.from_parts(
            ["d", "e", "f", " "],
            [1, 1, 1, 1],
            ["keyword", "keyword", "keyword", None],
        )

        diff = diff_render_lines(old, new)

        self.assertIsNotNone(diff)
        assert diff is not None
        self.assertEqual(diff.start_x, 3)
        self.assertEqual(render_cells(diff.new_cells), " ")

    def test_rendered_screen_round_trips_screen_lines(self):
        screen = RenderedScreen.from_screen_lines(
            ["a", "\x1b[31mb\x1b[0m"],
            (0, 1),
        )

        self.assertEqual(screen.screen_lines, ("a", "\x1b[31mb\x1b[0m"))
