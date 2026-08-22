"Test charselect, coverage 97%."
from idlelib import charselect
from test.support import requires

import unittest
from tkinter import Tk, Text
from idlelib.idle_test.mock_idle import Func


class CharSelectOpenTest(unittest.TestCase):
    "Test the open() entry point (no gui needed)."

    def test_open_with_selection(self):
        editwin = Func()
        editwin.top = 'toplevel'
        editwin.text = text = Func()      # A selection to seed the search with.
        text.index = Func(result='1.0')
        text.get = Func(result='ab')
        orig = charselect.CharSelectWindow
        mock = charselect.CharSelectWindow = Func()
        try:
            charselect.open(editwin)
        finally:
            charselect.CharSelectWindow = orig
        self.assertTrue(mock.called)
        self.assertEqual(mock.args, ('toplevel', text, 'ab'))

    def test_open_no_selection(self):
        editwin = Func()
        editwin.top = 'toplevel'
        editwin.text = text = Func()      # IDLE returns '' for a missing mark.
        text.index = Func(result='')
        orig = charselect.CharSelectWindow
        mock = charselect.CharSelectWindow = Func()
        try:
            charselect.open(editwin)
        finally:
            charselect.CharSelectWindow = orig
        self.assertEqual(mock.args, ('toplevel', text, ''))


class CharSelectWindowTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.dialog = charselect.CharSelectWindow(cls.root, _utest=True)

    @classmethod
    def tearDownClass(cls):
        cls.dialog.close()
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.dialog, cls.root

    def setUp(self):
        charselect.CharSelectWindow.last_block = 0   # isolate remembered state
        self.dialog.show_block(charselect.BLOCKS[0])
        self.dialog.selected_cp = None

    def editor_dialog(self, search=""):
        "A browser bound to a fresh editor Text, torn down after the test."
        text = Text(self.root)
        dialog = charselect.CharSelectWindow(self.root, text, search,
                                             _utest=True)
        self.addCleanup(text.destroy)
        self.addCleanup(lambda: dialog.winfo_exists() and dialog.destroy())
        return dialog, text

    def expect_bell(self, dialog):
        "Record dialog.bell() calls in the returned list."
        beeps = []
        dialog.bell = lambda: beeps.append(True)
        self.addCleanup(lambda: dialog.__dict__.pop("bell", None))
        return beeps

    def set_focus(self, dialog, widget):
        "Make dialog.focus_get() return widget for this test."
        dialog.focus_get = lambda: widget
        self.addCleanup(lambda: dialog.__dict__.pop("focus_get", None))

    def test_show_block(self):
        dialog = self.dialog
        dialog.show_block(("Arrows", 0x2190, 0x21FF))
        self.assertEqual(dialog.current_block[0], "Arrows")
        # One grid cell per assigned, non-skipped codepoint.
        self.assertEqual(dialog.cell_index, len(dialog.grid_inner.winfo_children()))
        self.assertGreater(dialog.cell_index, 0)
        self.assertIn("Arrows", dialog.status.cget("text"))

    def test_add_cell_skips_unassigned(self):
        dialog = self.dialog
        dialog.clear_grid()
        dialog.add_cell(0x0041)          # LATIN CAPITAL LETTER A
        dialog.add_cell(0x0007)          # BELL (Cc), skipped
        dialog.add_cell(0x0378)          # unassigned (Cn), skipped
        self.assertEqual(dialog.cell_index, 1)

    def test_select(self):
        dialog = self.dialog
        dialog.select(0x2764)            # HEAVY BLACK HEART
        self.assertEqual(dialog.selected_cp, 0x2764)
        self.assertEqual(dialog.big_var.get(), '❤')
        overview = dialog.overview.get("1.0", "end")
        self.assertIn("HEAVY BLACK HEART", overview)
        self.assertIn("U+2764", overview)
        self.assertIn("10084", overview)     # decimal
        reprs = dialog.reprs.get("1.0", "end")
        self.assertIn(r"\u2764", reprs)   # Python escape
        utf8 = "".join(charselect.char_escape(b) for b in '❤'.encode())
        self.assertIn(utf8, reprs)        # UTF-8 as \x escapes
        self.assertIn("&#10084;", reprs)  # numeric HTML entity

    def test_detail_escape_and_entity(self):
        dialog = self.dialog
        cases = {
            0x41: (r"\x41", "&#65;"),       # no named entity
            0xA0: (r"\xa0", "&nbsp;"),      # named entity
            0x26: (r"\x26", "&amp;"),       # named entity
            0x2764: (r"\u2764", "&#10084;"),
            0x1F600: (r"\U0001f600", "&#128512;"),
        }
        for cp, (escape, entity) in cases.items():
            with self.subTest(cp=cp):
                dialog.select(cp)
                reprs = dialog.reprs.get("1.0", "end")
                self.assertIn(escape, reprs)
                self.assertIn(entity, reprs)

    def test_select_unnamed(self):
        dialog = self.dialog
        dialog.select(0xE000)            # private use, no name
        self.assertIn("<unnamed>", dialog.overview.get("1.0", "end"))

    def test_repr_tab_non_bmp(self):
        dialog = self.dialog
        cp = 0x1F600                     # GRINNING FACE (non-BMP)
        dialog.select(cp)
        reprs = dialog.reprs.get("1.0", "end")
        self.assertIn(chr(cp), reprs)                        # literal character
        self.assertIn(charselect.char_escape(cp), reprs)    # Python escape
        self.assertIn(charselect.surrogate_pair(cp), reprs)  # UTF-16 surrogates
        self.assertIn("&#128512;", reprs)                   # XML/HTML reference
        self.assertIn("GRINNING FACE", reprs)               # in the \\N{...} escape

        utf8 = "".join(charselect.char_escape(b) for b in chr(cp).encode())
        self.assertIn(utf8, reprs)                          # UTF-8 as hex escapes

    def test_repr_tab_xml_entity(self):
        dialog = self.dialog
        dialog.select(0x27)              # APOSTROPHE: &apos; in XML, numeric HTML
        reprs = dialog.reprs.get("1.0", "end")
        self.assertIn("&apos;", reprs)   # XML predefines this entity
        self.assertIn("&#39;", reprs)    # but HTML has no name for it
        dialog.select(0x2764)            # not predefined -> decimal XML reference
        self.assertIn("&#10084;", dialog.reprs.get("1.0", "end"))

    def test_repr_tab_bmp_has_no_surrogates(self):
        dialog = self.dialog
        dialog.select(0x0041)            # LATIN CAPITAL LETTER A (BMP)
        self.assertNotIn("Surrogates", dialog.reprs.get("1.0", "end"))

    def test_surrogate_pair(self):
        self.assertIsNone(charselect.surrogate_pair(0x41))   # BMP has none
        # U+1F600 GRINNING FACE decomposes to high D83D, low DE00.
        self.assertEqual(charselect.surrogate_pair(0x1F600),
                         chr(92) + "ud83d" + chr(92) + "ude00")

    def test_unicode_tab(self):
        dialog = self.dialog
        dialog.select(0x0041)            # LATIN CAPITAL LETTER A
        data = dialog.unidata.get("1.0", "end")
        self.assertIn("LATIN CAPITAL LETTER A", data)  # name
        self.assertIn("Basic Latin", data)             # block
        self.assertIn("Lu", data)                      # category
        self.assertIn("uppercase letter", data)        # category description
        self.assertIn("Bidirectional", data)
        self.assertIn("Mirrored", data)

    def test_unicode_tab_numeric(self):
        dialog = self.dialog
        dialog.select(0x00BD)            # VULGAR FRACTION ONE HALF
        data = dialog.unidata.get("1.0", "end")
        self.assertIn("Numeric", data)
        self.assertIn("0.5", data)
        self.assertIn("(other number)", data)    # category name in parens
        self.assertIn("Decomposition", data)     # <fraction> 0031 2044 0032
        # Normalization forms show the literal string and the U+ codepoints.
        self.assertIn("1" + chr(0x2044) + "2", data)   # NFKC/NFKD literal
        self.assertIn("U+2044", data)                  # ... with codepoints

    def test_search_by_name(self):
        dialog = self.dialog
        dialog.search_var.set("heart")
        dialog.search()
        self.assertGreater(dialog.cell_index, 0)
        self.assertIn("heart", dialog.status.cget("text"))

    def test_search_by_codepoint(self):
        dialog = self.dialog
        # Every notation for U+1F600 GRINNING FACE shows exactly that cell.
        for query in ("U+1F600", "0x1F600", "&#x1F600;", "&#128512;",
                      "128512", charselect.char_escape(0x1F600)):
            with self.subTest(query=query):
                dialog.search_var.set(query)
                dialog.search()
                self.assertEqual(dialog.cell_index, 1)

    def test_search_single_character(self):
        dialog = self.dialog
        dialog.search_var.set("A")       # A single character shows only itself.
        dialog.search()
        self.assertEqual(dialog.cell_index, 1)

    def grid_chars(self):
        return [w.cget("text") for w in self.dialog.grid_inner.winfo_children()]

    def test_search_bare_hex(self):
        dialog = self.dialog
        dialog.search_var.set("FACE")    # bare hex 0xFACE, plus names with FACE
        dialog.search()
        self.assertIn(chr(0xFACE), self.grid_chars())   # the hex codepoint
        self.assertIn("☺", self.grid_chars())            # WHITE SMILING FACE
        self.assertGreater(dialog.cell_index, 1)

    def test_search_decimal_is_also_hex(self):
        dialog = self.dialog
        dialog.search_var.set("65")      # decimal 65 = 'A', hex 0x65 = 'e'
        dialog.search()
        self.assertIn("A", self.grid_chars())    # decimal value
        self.assertIn("e", self.grid_chars())    # hexadecimal value

    def test_search_forced_shows_filtered_char(self):
        dialog = self.dialog
        dialog.search_var.set("U+0007")  # BELL: a control, normally filtered out
        dialog.search()
        self.assertEqual(dialog.cell_index, 1)   # shown because it is explicit

    def test_search_single_result_selects(self):
        dialog = self.dialog
        dialog.selected_cp = None
        dialog.search_var.set("U+2764")  # exactly one result
        dialog.search()
        self.assertEqual(dialog.selected_cp, 0x2764)
        self.assertIn("HEAVY BLACK HEART", dialog.overview.get("1.0", "end"))
        self.assertEqual(dialog.selected_cell.cget("bg"), charselect.SELECT_BG)

    def test_search_selects_first_when_detail_empty(self):
        dialog = self.dialog
        dialog.selected_cp = None
        dialog.search_var.set("heart")   # many results, nothing selected yet
        dialog.search()
        self.assertIsNotNone(dialog.selected_cp)

    def test_search_keeps_detail_when_not_empty(self):
        dialog = self.dialog
        dialog.select(0x2764)            # a character is already shown
        dialog.search_var.set("heart")   # many results
        dialog.search()
        self.assertEqual(dialog.selected_cp, 0x2764)   # detail unchanged

    def test_select_highlights_grid_cell(self):
        dialog = self.dialog
        dialog.show_block(charselect.BLOCKS[0])   # Basic Latin, includes 'A'
        dialog.select(0x41)
        self.assertIs(dialog.selected_cell, dialog.cells[0x41])
        self.assertEqual(dialog.selected_cell.cget("bg"), charselect.SELECT_BG)

    def test_simple_escape_in_repr(self):
        dialog = self.dialog
        dialog.select(0x0A)              # LINE FEED
        reprs = dialog.reprs.get("1.0", "end")
        self.assertIn("Escaped:   " + chr(92) + "n", reprs)   # \n, not \x0a
        self.assertIn(chr(92) + "x0a", reprs)                 # UTF-8 keeps \xHH

    def test_seed_search(self):
        dialog, text = self.editor_dialog("U+1F600")
        self.assertEqual(dialog.search_var.get(), "U+1F600")
        self.assertEqual(dialog.cell_index, 1)
        self.assertEqual(dialog.selected_cp, 0x1F600)   # sole result selected

    def test_seed_blank_shows_first_block(self):
        dialog, text = self.editor_dialog("  ")
        self.assertEqual(dialog.current_block, charselect.BLOCKS[0])

    def test_search_empty(self):
        dialog = self.dialog
        dialog.show_block(("Arrows", 0x2190, 0x21FF))
        dialog.search_var.set("   ")
        dialog.search()                  # Should do nothing.
        self.assertEqual(dialog.current_block[0], "Arrows")

    def test_clear_search(self):
        dialog = self.dialog
        dialog.search_var.set("heart")
        dialog.search()
        dialog.clear_search()
        self.assertEqual(dialog.search_var.get(), "")
        self.assertEqual(dialog.current_block, charselect.BLOCKS[0])

    def test_block_selected(self):
        dialog = self.dialog
        dialog.block_combo.current(2)
        dialog.block_selected()
        self.assertEqual(dialog.current_block, charselect.BLOCKS[2])

    def test_block_remembered_on_reopen(self):
        first, _ = self.editor_dialog()
        first.block_combo.current(2)
        first.block_selected()
        first.destroy()
        again, _ = self.editor_dialog()
        self.assertEqual(again.current_block, charselect.BLOCKS[2])
        self.assertEqual(again.block_combo.current(), 2)

    def test_copy_char(self):
        dialog = self.dialog
        dialog.select(0x2764)
        dialog.clipboard_clear()
        dialog.copy_char()
        self.assertEqual(dialog.clipboard_get(), '❤')
        self.assertIn("Copied", dialog.status.cget("text"))

    def test_copy_without_selection(self):
        dialog = self.dialog
        dialog.selected_cp = None
        dialog.status.configure(text="unchanged")
        dialog.copy_char()
        self.assertEqual(dialog.status.cget("text"), "unchanged")

    def test_copy_text(self):
        dialog = self.dialog
        dialog.clipboard_clear()
        dialog.copy_text("hello")
        self.assertEqual(dialog.clipboard_get(), "hello")
        self.assertIn("Copied", dialog.status.cget("text"))

    def test_copy_later_defers_copy(self):
        dialog = self.dialog
        dialog.copy_later("hi")                     # a single click on a value
        self.assertIsNotNone(dialog.pending_copy)   # scheduled, not immediate
        self.addCleanup(dialog.cancel_copy)
        dialog.cancel_copy()
        self.assertIsNone(dialog.pending_copy)

    def test_double_click_value_cancels_copy(self):
        dialog, text = self.editor_dialog()
        dialog.copy_later("x")                      # single click scheduled copy
        self.assertIsNotNone(dialog.pending_copy)
        dialog.insert_value("x")                    # the double-click action
        self.assertIsNone(dialog.pending_copy)      # copy was cancelled
        self.assertEqual(text.get("1.0", "end-1c"), "x")
        self.assertFalse(dialog.winfo_exists())     # inserted and closed

    def test_repr_values_are_clickable(self):
        dialog = self.dialog
        dialog.select(0x2764)
        # Every Repr row carries a copy string equal to its displayed value.
        for label, value, copy in dialog.repr_pairs(0x2764):
            self.assertEqual(copy, value)
        self.assertTrue(dialog.reprs.tag_ranges("copy"))      # click targets
        self.assertFalse(dialog.overview.tag_ranges("copy"))  # plain text

    def test_normalization_click_copies_literal(self):
        dialog = self.dialog
        dialog.select(0x00BD)            # VULGAR FRACTION ONE HALF
        rows = {p[0]: p for p in dialog.unidata_pairs(0x00BD)}
        label, display, copy = rows["NFKD"]
        self.assertIn("U+2044", display)                 # shown with codepoints
        self.assertEqual(copy, "1" + chr(0x2044) + "2")  # copied without them
        self.assertTrue(dialog.unidata.tag_ranges("copy"))

    def test_copy_event(self):
        dialog = self.dialog
        dialog.select(0x2764)
        dialog.clipboard_clear()
        self.set_focus(dialog, None)          # focus not in a text widget
        self.assertEqual(dialog.copy_event(), "break")
        self.assertEqual(dialog.clipboard_get(), '❤')

    def test_copy_event_defers_to_text_selection(self):
        dialog = self.dialog
        dialog.select(0x2764)
        dialog.clipboard_clear()
        dialog.clipboard_append("kept")
        dialog.overview.tag_add("sel", "1.0", "1.3")   # a pane with a selection
        self.addCleanup(dialog.overview.tag_remove, "sel", "1.0", "end")
        self.set_focus(dialog, dialog.overview)
        self.assertIsNone(dialog.copy_event())         # deferred, not copied
        self.assertEqual(dialog.clipboard_get(), "kept")

    def test_insert_disabled_without_editor(self):
        # self.dialog was created with no editor Text to insert into.
        self.assertIn("disabled", self.dialog.insert_button.state())

    def test_insert_char(self):
        dialog, text = self.editor_dialog()
        self.assertNotIn("disabled", dialog.insert_button.state())
        dialog.select(0x2764)
        text.insert("insert", "ab")
        text.mark_set("insert", "1.1")     # Cursor between a and b.
        dialog.insert_char()
        self.assertEqual(text.get("1.0", "end-1c"), "a❤b")
        self.assertFalse(dialog.winfo_exists())  # insert_char closed it.

    def test_double_click_inserts_and_closes(self):
        dialog, text = self.editor_dialog()
        dialog.activate_cell(0x41)                 # the <Double-Button-1> action
        self.assertEqual(text.get("1.0", "end-1c"), "A")
        self.assertFalse(dialog.winfo_exists())   # inserted and closed

    def test_insert_text_inserts_and_closes(self):
        # The double-click action on a detail value inserts its string.
        dialog, text = self.editor_dialog()
        dialog.insert_text(r"\U0001f600")
        self.assertEqual(text.get("1.0", "end-1c"), r"\U0001f600")
        self.assertFalse(dialog.winfo_exists())   # inserted and closed

    def test_insert_text_beeps_without_editor(self):
        dialog = self.dialog                       # opened with no editor Text
        beeped = self.expect_bell(dialog)
        dialog.insert_text("x")
        self.assertTrue(beeped)                    # Beeped: nowhere to insert.

    def test_insert_replaces_selection(self):
        dialog, text = self.editor_dialog()
        dialog.select(0x2764)
        text.insert("1.0", "abc")
        text.tag_add("sel", "1.1", "1.2")     # Select 'b'.
        dialog.insert_char()
        self.assertEqual(text.get("1.0", "end-1c"), "a❤c")

    def test_insert_clamped_to_shell_input(self):
        # An "iomark" mark makes a plain Text behave like the Shell.
        dialog, text = self.editor_dialog()
        dialog.select(0x2764)
        text.insert("1.0", "output\n>>> ")
        text.mark_set("iomark", "2.4")   # Input starts after the prompt.
        text.mark_set("insert", "1.2")   # Cursor up in the output area.
        dialog.insert_char()
        self.assertEqual(text.get("1.0", "2.4"), "output\n>>> ")  # untouched
        self.assertEqual(text.get("2.4", "2.5"), "❤")   # inserted in input

    def test_insert_keeps_shell_output_on_selection(self):
        dialog, text = self.editor_dialog()
        dialog.select(0x2764)
        text.insert("1.0", "output\n>>> ")
        text.mark_set("iomark", "2.4")
        text.tag_add("sel", "1.0", "1.6")   # Select 'output' (before iomark).
        text.mark_set("insert", "1.6")
        dialog.insert_char()
        self.assertEqual(text.get("1.0", "1.6"), "output")   # not overwritten

    def test_insert_without_selection(self):
        dialog, text = self.editor_dialog()
        dialog.selected_cp = None
        beeped = self.expect_bell(dialog)
        dialog.insert_char()
        self.assertEqual(text.get("1.0", "end-1c"), "")
        self.assertTrue(dialog.winfo_exists())   # Not closed.
        self.assertTrue(beeped)                  # Beeped: nothing to insert.

    def test_insert_beeps_without_editor(self):
        dialog = self.dialog                     # opened with no editor Text
        dialog.select(0x41)
        beeped = self.expect_bell(dialog)
        dialog.insert_char()
        self.assertTrue(beeped)                  # Beeped: nowhere to insert.


class ParseInputTest(unittest.TestCase):
    "Test charselect.parse_input (no gui needed)."

    def test_single_character(self):
        self.assertEqual(charselect.parse_input("A"), [0x41])
        self.assertEqual(charselect.parse_input("🦆"), [0x1F986])

    def test_marked_codepoints(self):
        duck = 0x1F986
        for query in ("U+1F986", "u+1f986", "0x1F986", "0X1f986",
                      "&#x1F986;", "&#x1f986", "&#129414;", "&#129414"):
            with self.subTest(query=query):
                self.assertEqual(charselect.parse_input(query), [duck])

    def test_escape_forms(self):
        duck = 0x1F986
        forms = [
            charselect.char_escape(duck),          # \U0001f986
            charselect.surrogate_pair(duck),       # 🦆
            "".join(charselect.char_escape(b)      # \xf0\x9f\xa6\x86 (UTF-8)
                    for b in chr(duck).encode()),
        ]
        for query in forms:
            with self.subTest(query=query):
                self.assertEqual(charselect.parse_input(query), [duck])

    def test_named_form(self):
        query = chr(92) + "N{DUCK}"                # \N{DUCK}
        self.assertEqual(charselect.parse_input(query), [0x1F986])

    def test_simple_escapes(self):
        bs = chr(92)
        for esc, cp in ((bs+"n", 0x0A), (bs+"t", 0x09), (bs+"r", 0x0D),
                        (bs+"0", 0x00), (bs+"a", 0x07)):
            with self.subTest(esc=esc):
                self.assertEqual(charselect.parse_input(esc), [cp])

    def test_char_escape_prefers_simple(self):
        bs = chr(92)
        self.assertEqual(charselect.char_escape(0x0A), bs + "n")
        self.assertEqual(charselect.char_escape(0x09), bs + "t")
        self.assertEqual(charselect.char_escape(0x41), bs + "x41")
        bad = chr(92) + "N{NOT A CHARACTER NAME}"
        self.assertEqual(charselect.parse_input(bad), [])

    def test_literal(self):
        bs = chr(92)
        self.assertEqual(charselect.literal("abc"), "abc")
        self.assertEqual(charselect.literal("é"), "é")     # combining kept
        self.assertEqual(charselect.literal("a" + chr(0) + chr(9)),
                         "a" + bs + "0" + bs + "t")         # NUL, TAB escaped

    def test_bare_tokens_are_not_marked(self):
        # A word or a bare number is not a marked notation; parse_input
        # returns None and the caller resolves it by value and by name.
        self.assertIsNone(charselect.parse_input("heart"))
        self.assertIsNone(charselect.parse_input("1F600"))   # bare hex
        self.assertIsNone(charselect.parse_input("129414"))  # bare decimal


if __name__ == '__main__':
    unittest.main(verbosity=2)
