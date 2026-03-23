from unittest import TestCase

from _pyrepl.render import RenderLine, RenderedScreen, diff_render_lines


class TestRenderLine(TestCase):
    def test_from_rendered_text_groups_escape_with_visible_cells(self):
        line = RenderLine.from_rendered_text("\x1b[31ma\x1b[0mb")

        self.assertEqual(line.width, 2)
        self.assertEqual(
            [cell.text for cell in line.cells],
            ["\x1b[31ma", "\x1b[0mb"],
        )

    def test_from_rendered_text_keeps_trailing_escape_on_last_cell(self):
        line = RenderLine.from_rendered_text("\x1b[31ma\x1b[0m")

        self.assertEqual([cell.text for cell in line.cells], ["\x1b[31ma\x1b[0m"])


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

    def test_rendered_screen_round_trips_screen_lines(self):
        screen = RenderedScreen.from_screen_lines(
            ["a", "\x1b[31mb\x1b[0m"],
            (0, 1),
        )

        self.assertEqual(screen.screen_lines, ("a", "\x1b[31mb\x1b[0m"))
