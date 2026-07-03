"Test astbrowser, coverage 95%."
from idlelib import astbrowser
from test.support import requires

import unittest
from unittest import mock
from tkinter import Tk, Text
from idlelib.idle_test.mock_idle import Func

code_sample = "import sys\n\ndef f(x):\n    return x + 1\n"


def find_item(tree, label):
    "Return the first tree item whose text contains label, or None."
    stack = list(tree.get_children())
    while stack:
        item = stack.pop(0)
        if label in tree.item(item, "text"):
            return item
        stack[:0] = tree.get_children(item)
    return None


class ASTBrowserOpenTest(unittest.TestCase):
    "Test the open() entry point (no gui needed)."

    def make_editwin(self):
        editwin = Func()          # Only .top and .text are used.
        editwin.top = 'toplevel'
        editwin.text = 'text'
        return editwin

    def test_open_creates_window(self):
        editwin = self.make_editwin()
        with mock.patch.object(astbrowser, 'ASTBrowserWindow',
                               Func(result='window')) as window:
            astbrowser.open(editwin)
        self.assertEqual(window.args, ('toplevel', 'text'))
        self.assertEqual(editwin.ast_browser, 'window')

    def test_open_reuses_window(self):
        editwin = self.make_editwin()
        editwin.ast_browser = existing = Func()   # A live window.
        existing.winfo_exists = Func(result=1)
        existing.refresh = Func()
        with mock.patch.object(astbrowser, 'ASTBrowserWindow',
                               Func()) as new_window:
            astbrowser.open(editwin)
        self.assertTrue(existing.refresh.called)   # Refreshed, not recreated.
        self.assertFalse(new_window.called)


class ASTBrowserWindowTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.text = Text(cls.root)
        cls.window = astbrowser.ASTBrowserWindow(cls.root, cls.text, _utest=True)

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

    def find(self, label):
        "Return the first tree item whose text contains label."
        item = find_item(self.window.tree, label)
        if item is None:
            self.fail(f"no node {label!r}")
        return item

    def test_populate_text(self):
        window = self.window
        roots = window.tree.get_children()
        self.assertEqual(len(roots), 1)
        self.assertEqual(window.tree.item(roots[0], "text"), "Module")
        self.assertIn("in text", window.status.cget("text"))
        self.assertEqual(window.base, (1, 0))

    def test_tree_structure(self):
        # The sample parses to a tree with these nodes at some depth.
        for label in ("Import", "FunctionDef(name='f')", "Return",
                      "BinOp(op=Add)", "Name(id='x'", "Constant(value=1)"):
            self.find(label)

    def test_node_range(self):
        # The Name 'x' in 'return x + 1' maps to editor coordinates.
        item = self.find("Name(id='x'")
        self.assertEqual(self.window.ranges[item], ("4.11", "4.12"))

    def test_selection_scope(self):
        self.text.tag_add("sel", "4.11", "4.16")   # 'x + 1' on line 4.
        self.window.populate()
        self.assertEqual(self.window.base, (4, 11))
        self.assertIn("in selection", self.window.status.cget("text"))
        # The BinOp maps back to the selected editor coordinates.
        item = self.find("BinOp")
        self.assertEqual(self.window.ranges[item], ("4.11", "4.16"))

    def test_shell_input_scope(self):
        text = Text(self.root)
        text.insert("1.0", ">>> x = 1\n")
        text.mark_set("iomark", "1.4")            # After the ">>> " prompt.
        window = astbrowser.ASTBrowserWindow(self.root, text, _utest=True)
        self.assertEqual(window.base, (1, 4))
        self.assertIn("in input", window.status.cget("text"))
        item = find_item(window.tree, "Name(id='x'")
        self.assertEqual(window.ranges[item], ("1.4", "1.5"))
        window.destroy()
        text.destroy()

    def test_syntax_error(self):
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "def f(:\n")       # Invalid syntax.
        self.window.populate()
        self.assertIn("incomplete", self.window.status.cget("text"))
        self.assertEqual(self.window.tree.get_children(), ())

    def test_sync_cursor_selects_innermost(self):
        window = self.window
        self.text.mark_set("insert", "4.11")       # At 'x' in 'return x + 1'.
        window.sync_from_editor()
        selection = window.tree.selection()
        self.assertEqual(len(selection), 1)
        self.assertIn("Name(id='x'", window.tree.item(selection[0], "text"))
        self.assertEqual(self.text.index("insert"), "4.11")

    def test_sync_selection_selects_enclosing(self):
        window = self.window
        self.text.tag_add("sel", "4.11", "4.16")   # 'x + 1'.
        window.sync_from_editor()
        selection = window.tree.selection()
        self.assertEqual(len(selection), 1)
        self.assertIn("BinOp", window.tree.item(selection[0], "text"))

    def test_focused_highlights_and_moves_cursor(self):
        # Browser drives the selection (it has focus): highlight the node's
        # source and move the cursor to it.
        window = self.window
        window.focused = True
        window.tree.selection_set(self.find("Name(id='x'"))
        window.select_nodes()
        ranges = [str(i) for i in self.text.tag_ranges(astbrowser.TAG)]
        self.assertEqual(ranges, ["4.11", "4.12"])
        self.assertEqual(self.text.index("insert"), "4.11")

    def test_not_focused_keeps_editor_clean(self):
        # Editor drives the selection (browser not focused): select_nodes
        # neither highlights the editor nor moves its cursor.
        window = self.window
        window.focused = False
        self.text.mark_set("insert", "1.0")
        window.tree.selection_set(self.find("Name(id='x'"))
        window.select_nodes()
        self.assertEqual(self.text.tag_ranges(astbrowser.TAG), ())
        self.assertEqual(self.text.index("insert"), "1.0")

    def test_highlight_follows_focus(self):
        window = self.window
        window.tree.selection_set(self.find("Name(id='x'"))
        window.on_focus_in()           # The browser has focus.
        self.assertNotEqual(self.text.tag_ranges(astbrowser.TAG), ())
        window.on_focus_out()          # Focus moves to the editor.
        self.assertEqual(self.text.tag_ranges(astbrowser.TAG), ())
        window.on_focus_in()           # Focus returns to the browser.
        self.assertNotEqual(self.text.tag_ranges(astbrowser.TAG), ())

    def test_node_without_location_not_highlighted(self):
        # Module has no source location: it is absent from ranges and
        # selecting it produces no highlight.
        window = self.window
        window.focused = True
        item = self.find("Module")
        self.assertNotIn(item, window.ranges)
        window.tree.selection_set(item)
        window.select_nodes()
        self.assertEqual(self.text.tag_ranges(astbrowser.TAG), ())

    def test_refresh(self):
        window = self.window
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "spam = 1\n")
        window.refresh()
        self.find("Name(id='spam'")    # The tree was rebuilt.

    def test_move_cursor(self):
        window = self.window
        item = self.find("Name(id='x'")
        window.move_cursor(item)
        self.assertEqual(self.text.index("insert"), "4.11")
        # A node without a source range (Module) leaves the cursor put.
        window.move_cursor(self.find("Module"))
        self.assertEqual(self.text.index("insert"), "4.11")

    def test_hide(self):
        text = Text(self.root)
        text.insert("1.0", code_sample)
        window = astbrowser.ASTBrowserWindow(self.root, text, _utest=True)
        window.deiconify()
        window.focused = True
        window.tree.selection_set(find_item(window.tree, "Name(id='x'"))
        window.select_nodes()
        self.assertNotEqual(text.tag_ranges(astbrowser.TAG), ())
        window.hide()                  # Double-click (or Escape) hides it.
        self.assertEqual(window.wm_state(), "withdrawn")   # Not destroyed.
        self.assertTrue(window.winfo_exists())
        self.assertEqual(text.tag_ranges(astbrowser.TAG), ())
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
        window = astbrowser.ASTBrowserWindow(self.root, text, _utest=True)
        self.assertEqual(window.base, (1, 0))
        self.assertIn("in text", window.status.cget("text"))
        window.destroy()
        text.destroy()


if __name__ == '__main__':
    unittest.main(verbosity=2)
