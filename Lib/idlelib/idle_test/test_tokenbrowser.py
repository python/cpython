"Test tokenbrowser, coverage 95%."
from idlelib import tokenbrowser
from test.support import requires

import unittest
from unittest import mock
from tkinter import Tk, Text
from idlelib.idle_test.mock_idle import Func

code_sample = "import sys\n\ndef f(x):\n    return x + 1\n"


class TokenBrowserOpenTest(unittest.TestCase):
    "Test the open() entry point (no gui needed)."

    def make_editwin(self):
        editwin = Func()          # Only .top and .text are used.
        editwin.top = 'toplevel'
        editwin.text = 'text'
        return editwin

    def test_open_creates_window(self):
        editwin = self.make_editwin()
        with mock.patch.object(tokenbrowser, 'TokenBrowserWindow',
                               Func(result='window')) as window:
            tokenbrowser.open(editwin)
        self.assertEqual(window.args, ('toplevel', 'text'))
        self.assertEqual(editwin.token_browser, 'window')

    def test_open_reuses_window(self):
        editwin = self.make_editwin()
        editwin.token_browser = existing = Func()   # A live window.
        existing.winfo_exists = Func(result=1)
        existing.refresh = Func()
        with mock.patch.object(tokenbrowser, 'TokenBrowserWindow',
                               Func()) as new_window:
            tokenbrowser.open(editwin)
        self.assertTrue(existing.refresh.called)   # Refreshed, not recreated.
        self.assertFalse(new_window.called)


class TokenBrowserWindowTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.text = Text(cls.root)
        cls.window = tokenbrowser.TokenBrowserWindow(
            cls.root, cls.text, _utest=True)

    @classmethod
    def tearDownClass(cls):
        cls.window.destroy()
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.window, cls.text, cls.root

    def setUp(self):
        self.text.delete("1.0", "end")
        self.text.insert("1.0", code_sample)
        self.window.populate()

    def find(self, type=None, string=None):
        "Return the first tree item matching a token type and/or string."
        tree = self.window.tree
        for item in tree.get_children():
            typ, s = tree.item(item, "values")
            if (type is None or typ == type) and \
               (string is None or s == repr(string)):
                return item
        self.fail(f"no token {type} {string!r}")

    def test_populate_text(self):
        window = self.window
        self.assertGreater(len(window.ranges), 0)
        self.assertEqual(len(window.ranges), len(window.tree.get_children()))
        self.assertIn("in text", window.status.cget("text"))
        self.assertEqual(window.base, (1, 0))

    def test_token_row_values(self):
        tree = self.window.tree
        item = tree.get_children()[0]
        # First token is NAME 'import', shown as two columns, mapped to 1.0-1.6.
        self.assertEqual(tree.item(item, "values"), ("NAME", repr("import")))
        self.assertEqual(self.window.ranges[item], ("1.0", "1.6"))
        # Operators show their exact type.
        self.find(type="PLUS", string="+")

    def test_token_colors(self):
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "x = 'a' + 1  # c\n")
        self.window.populate()
        tree = self.window.tree
        tags = {tree.item(item, "values")[1]: tree.item(item, "tags")
                for item in tree.get_children()}
        self.assertIn("string", tags[repr("'a'")])
        self.assertIn("number", tags[repr("1")])
        self.assertIn("comment", tags[repr("# c")])
        self.assertNotIn("string", tags[repr("x")])   # NAME: default color.

    def test_editor_index(self):
        window = self.window
        window.base = (1, 0)
        self.assertEqual(window.editor_index(1, 0), "1.0")
        self.assertEqual(window.editor_index(3, 4), "3.4")

    def test_selection_scope(self):
        self.text.tag_add("sel", "4.11", "4.16")   # 'x + 1' on line 4.
        self.window.populate()
        window = self.window
        self.assertEqual(window.base, (4, 11))
        self.assertIn("in selection", window.status.cget("text"))
        # Tokens map back to editor coordinates.
        item = self.find(type="NAME", string="x")
        self.assertEqual(window.ranges[item], ("4.11", "4.12"))

    def test_focused_highlights_and_moves_cursor(self):
        # Browser drives the selection (it has focus): highlight the token
        # in the editor and move the cursor to it.
        window = self.window
        window.focused = True
        window.tree.selection_set(self.find(type="NAME", string="sys"))
        window.select_tokens()
        ranges = [str(i) for i in self.text.tag_ranges(tokenbrowser.TAG)]
        self.assertEqual(ranges, ["1.7", "1.10"])
        self.assertEqual(self.text.index("insert"), "1.7")

    def test_not_focused_keeps_editor_clean(self):
        # Editor drives the selection (browser not focused): select_tokens
        # neither highlights the editor nor moves its cursor.
        window = self.window
        window.focused = False
        self.text.mark_set("insert", "1.0")
        window.tree.selection_set(self.find(type="NAME", string="sys"))
        window.select_tokens()
        self.assertEqual(self.text.tag_ranges(tokenbrowser.TAG), ())
        self.assertEqual(self.text.index("insert"), "1.0")

    def test_select_multiple_highlights(self):
        window = self.window
        window.focused = True
        items = [self.find(type="NAME", string="import"),
                 self.find(type="NAME", string="sys")]
        window.tree.selection_set(items)
        window.select_tokens()
        ranges = self.text.tag_ranges(tokenbrowser.TAG)
        self.assertEqual(len(ranges), 4)            # Two (start, end) pairs.

    def test_highlight_follows_focus(self):
        window = self.window
        window.tree.selection_set(self.find(type="NAME", string="sys"))
        window.on_focus_in()           # The browser has focus.
        self.assertNotEqual(self.text.tag_ranges(tokenbrowser.TAG), ())
        window.on_focus_out()          # Focus moves to the editor.
        self.assertEqual(self.text.tag_ranges(tokenbrowser.TAG), ())
        window.on_focus_in()           # Focus returns to the browser.
        self.assertNotEqual(self.text.tag_ranges(tokenbrowser.TAG), ())

    def test_extend_selection(self):
        tree = self.window.tree
        rows = tree.get_children()
        tree.selection_set(rows[0])
        tree.focus(rows[0])
        self.window.extend_selection(1)
        self.assertEqual(set(tree.selection()), {rows[0], rows[1]})
        self.window.extend_selection(1)
        self.assertEqual(set(tree.selection()), {rows[0], rows[1], rows[2]})

    def test_extend_selection_at_edge(self):
        tree = self.window.tree
        last = tree.get_children()[-1]
        tree.selection_set(last)
        tree.focus(last)
        self.window.extend_selection(1)     # No next row to add.
        self.assertEqual(tree.selection(), (last,))

    def test_zero_width_not_highlighted(self):
        window = self.window
        window.focused = True
        item = self.find(type="ENDMARKER")
        start, end = window.ranges[item]
        self.assertEqual(start, end)
        window.tree.selection_set(item)
        window.select_tokens()
        self.assertEqual(self.text.tag_ranges(tokenbrowser.TAG), ())

    def test_sync_cursor_row(self):
        # With no editor selection, sync selects the single row of the
        # token under the cursor, without moving the cursor.
        window = self.window
        self.text.mark_set("insert", "1.8")   # Inside 'sys' (1.7-1.10).
        window.sync_from_editor()
        selection = window.tree.selection()
        self.assertEqual(len(selection), 1)
        self.assertEqual(window.tree.item(selection[0], "values"),
                         ("NAME", repr("sys")))
        self.assertEqual(self.text.index("insert"), "1.8")

    def test_sync_selection_selects_rows(self):
        # An editor selection selects every overlapping token's row.
        window = self.window
        self.text.tag_add("sel", "4.11", "4.16")   # 'x + 1' on line 4.
        window.sync_from_editor()
        values = {window.tree.item(item, "values")
                  for item in window.tree.selection()}
        self.assertEqual(values, {("NAME", repr("x")),
                                  ("PLUS", repr("+")),
                                  ("NUMBER", repr("1"))})

    def test_refresh(self):
        window = self.window
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "spam = 1\n")
        window.refresh()
        strings = [window.tree.item(i, "values")[1]
                   for i in window.tree.get_children()]
        self.assertIn(repr("spam"), strings)

    def test_move_cursor(self):
        window = self.window
        item = self.find(type="NAME", string="return")
        window.move_cursor(item)
        self.assertEqual(self.text.index("insert"), window.ranges[item][0])

    def test_move_cursor_no_item(self):
        self.window.move_cursor("")     # identify_row returns "" off a row.

    def test_hide(self):
        text = Text(self.root)
        text.insert("1.0", code_sample)
        window = tokenbrowser.TokenBrowserWindow(self.root, text, _utest=True)
        window.deiconify()
        window.focused = True
        window.tree.selection_set(window.tree.get_children()[0])
        window.select_tokens()
        self.assertNotEqual(text.tag_ranges(tokenbrowser.TAG), ())
        window.hide()                   # Double-click (or Escape) hides it.
        self.assertEqual(window.wm_state(), "withdrawn")   # Not destroyed.
        self.assertTrue(window.winfo_exists())
        self.assertEqual(text.tag_ranges(tokenbrowser.TAG), ())
        window.destroy()
        text.destroy()

    def test_shell_input_scope(self):
        # In the Shell (a Text with an "iomark"), browse only the current
        # input, which starts after the prompt at the iomark.
        text = Text(self.root)
        text.insert("1.0", ">>> x = 1\n")
        text.mark_set("iomark", "1.4")     # After the ">>> " prompt.
        window = tokenbrowser.TokenBrowserWindow(self.root, text, _utest=True)
        self.assertEqual(window.base, (1, 4))
        self.assertIn("in input", window.status.cget("text"))
        # The prompt is not tokenized; the first token is NAME 'x' at 1.4.
        first = window.tree.get_children()[0]
        self.assertEqual(window.tree.item(first, "values"), ("NAME", repr("x")))
        self.assertEqual(window.ranges[first], ("1.4", "1.5"))
        window.destroy()
        text.destroy()

    def test_no_selection_empty_index(self):
        # The IDLE editor returns '' (not a TclError) for a missing selection
        # or mark; that must be treated as "browse the whole text", not crash.
        class EditorText(Text):
            def index(self, spec):
                if spec.startswith("sel.") or spec == "iomark":
                    return ""
                return super().index(spec)
        text = EditorText(self.root)
        text.insert("1.0", code_sample)
        window = tokenbrowser.TokenBrowserWindow(self.root, text, _utest=True)
        self.assertEqual(window.base, (1, 0))
        self.assertIn("in text", window.status.cget("text"))
        window.destroy()
        text.destroy()

    def test_incomplete_source(self):
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "def f(:\n")    # Unbalanced/invalid.
        self.window.populate()
        self.assertIn("incomplete", self.window.status.cget("text"))


if __name__ == '__main__':
    unittest.main(verbosity=2)
