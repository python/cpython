"Test disbrowser, coverage 95%."
from idlelib import disbrowser
from test.support import requires

import dis
import types
import unittest
from unittest import mock
from tkinter import Tk, Text
from idlelib.idle_test.mock_idle import Func

code_sample = "import sys\n\ndef f(x):\n    return x + 1\n"


def walk(tree, parent=""):
    "Yield every tree item, depth first."
    for item in tree.get_children(parent):
        yield item
        yield from walk(tree, item)


def find_instr(window, rng):
    "The single instruction row in window whose editor range is rng."
    items = [it for it in window.instr_items if window.ranges[it] == rng]
    assert len(items) == 1, f"no unique instruction at {rng}"
    return items[0]


class DisBrowserOpenTest(unittest.TestCase):
    "Test the open() entry point (no gui needed)."

    def make_editwin(self):
        editwin = Func()          # Only .top and .text are used.
        editwin.top = 'toplevel'
        editwin.text = 'text'
        return editwin

    def test_open_creates_window(self):
        editwin = self.make_editwin()
        with mock.patch.object(disbrowser, 'DisBrowserWindow',
                               Func(result='window')) as window:
            disbrowser.open(editwin)
        self.assertEqual(window.args, ('toplevel', 'text', editwin))
        self.assertEqual(editwin.disassembly_browser, 'window')

    def test_open_reuses_window(self):
        editwin = self.make_editwin()
        editwin.disassembly_browser = existing = Func()   # A live window.
        existing.winfo_exists = Func(result=1)
        existing.refresh = Func()
        with mock.patch.object(disbrowser, 'DisBrowserWindow',
                               Func()) as new_window:
            disbrowser.open(editwin)
        self.assertTrue(existing.refresh.called)   # Refreshed, not recreated.
        self.assertFalse(new_window.called)


class GuiTest(unittest.TestCase):
    "Common Tk fixture: a hidden root and a browser on an editor Text."

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = root = Tk()
        root.withdraw()
        cls.text = Text(root)
        cls.window = disbrowser.DisBrowserWindow(root, cls.text, _utest=True)

    @classmethod
    def tearDownClass(cls):
        cls.window.destroy()
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.window, cls.text, cls.root

    def setUp(self):
        self.window.editwin = None
        self.text.delete("1.0", "end")
        self.text.insert("1.0", code_sample)
        self.window.populate()


class DisBrowserWindowTest(GuiTest):

    def find_op(self, opname):
        "Return the first instruction row whose Instruction column is opname."
        tree = self.window.tree
        for item in walk(tree):
            if tree.set(item, "opname") == opname:
                return item
        self.fail(f"no instruction {opname!r}")

    def code_nodes(self):
        "Return the top-level code-object rows, in order."
        tree = self.window.tree
        return [it for it in tree.get_children("")
                if "code" in tree.item(it, "tags")]

    def instr_at(self, rng):
        "Return the single instruction item with the given editor range."
        return find_instr(self.window, rng)

    def test_populate_text(self):
        window = self.window
        first = window.tree.get_children("")[0]   # The <module> code row.
        self.assertEqual(window.tree.item(first, "text"), "<module>")
        self.assertIn("code", window.tree.item(first, "tags"))
        self.assertIn("instructions in text", window.status.cget("text"))
        self.assertEqual(window.base, (1, 0))

    def test_code_objects_at_top_level(self):
        # Code objects are collapsible top-level rows (module then f), each
        # with its own instruction children.
        window = self.window
        self.assertEqual([window.tree.item(it, "text") for it in
                          self.code_nodes()], ["<module>", "f"])
        module = self.code_nodes()[0]
        self.assertTrue(any(c in window.instr_items
                            for c in window.tree.get_children(module)))

    def test_instruction_range(self):
        # The load of 'x' in 'return x + 1' maps to editor coordinates.
        item = self.instr_at(("4.11", "4.12"))
        self.assertEqual(self.window.tree.set(item, "arg"), "x")

    def test_form_feed_does_not_shift_lines(self):
        # gh-152966: a form feed breaks str.splitlines() but not the compiler,
        # so it must not shift source_lines and misplace later instructions.
        window = self.window
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "a = 1\f\nb = 22\n")   # form feed ends line 1.
        window.populate()
        ranges = [window.ranges[it] for it in window.instr_items]
        self.assertIn(("2.4", "2.6"), ranges)          # the load of 22.

    def code_node(self, qualname):
        "Return the top-level code-object row named qualname."
        tree = self.window.tree
        for item in self.code_nodes():
            if tree.item(item, "text") == qualname:
                return item
        self.fail(f"no code object {qualname!r}")

    def test_code_object_span(self):
        # The 'f' code row spans from its def to the end of its body.
        f_node = self.code_node("f")
        self.assertEqual(self.window.ranges[f_node], ("3.0", "4.16"))

    def test_open_cursor_code_object(self):
        # On open, everything is collapsed and the code object holding the
        # cursor is opened (the rest stay collapsed).
        window = self.window
        self.text.mark_set("insert", "4.11")       # Inside f's body.
        window.populate()
        self.assertTrue(window.tree.item(self.code_node("f"), "open"))
        self.assertFalse(window.tree.item(self.code_node("<module>"), "open"))

    def test_open_module_at_module_scope(self):
        window = self.window
        self.text.mark_set("insert", "1.0")        # On 'import sys'.
        window.populate()
        self.assertTrue(window.tree.item(self.code_node("<module>"), "open"))
        self.assertFalse(window.tree.item(self.code_node("f"), "open"))

    def test_all_collapsed_when_cursor_off_code(self):
        # A cursor on no instruction opens nothing and selects nothing.
        window = self.window
        self.text.mark_set("insert", "5.0")        # Past the last line.
        window.populate()
        self.assertFalse(any(window.tree.item(n, "open")
                             for n in self.code_nodes()))
        self.assertEqual(window.tree.selection(), ())

    def test_selection_scope(self):
        self.text.tag_add("sel", "4.11", "4.16")   # 'x + 1' on line 4.
        self.window.populate()
        self.assertEqual(self.window.base, (4, 11))
        self.assertIn("in selection", self.window.status.cget("text"))
        # The load of 'x' maps back to the selected editor coordinates.
        self.assertEqual(self.window.tree.set(
            self.instr_at(("4.11", "4.12")), "arg"), "x")

    def test_shell_input_scope(self):
        text = Text(self.root)
        text.insert("1.0", ">>> x = 1\n")
        text.mark_set("iomark", "1.4")            # After the ">>> " prompt.
        window = disbrowser.DisBrowserWindow(self.root, text, _utest=True)
        self.assertEqual(window.base, (1, 4))
        self.assertIn("in input", window.status.cget("text"))
        # The store of 'x' maps back past the ">>> " prompt to the input.
        items = [it for it in window.instr_items
                 if window.ranges[it] == ("1.4", "1.5")]
        self.assertTrue(items)
        self.assertTrue(any(window.tree.set(it, "arg") == "x"
                            for it in items))
        window.destroy()
        text.destroy()

    def test_jump_target_marked(self):
        window = self.window
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "while x:\n    pass\n")   # A loop has a target.
        window.populate()
        marks = [window.tree.set(it, "mark") for it in walk(window.tree)]
        self.assertIn(">>", marks)

    def test_colorize_groups(self):
        window = self.window
        # Code-object rows carry the 'code' tag and its configured color.
        module = self.code_node("<module>")
        self.assertIn("code", window.tree.item(module, "tags"))
        self.assertEqual(
            str(window.tree.tag_configure("code", "foreground")),
            disbrowser.GROUP_COLORS["code"])
        # Instructions are grouped by operand kind: 'x' is a local; the
        # loads/stores of names ('sys', 'f') are names; LOAD_CONST is a const.
        self.assertIn("local", window.tree.item(
            self.instr_at(("4.11", "4.12")), "tags"))
        tags = [window.tree.item(self.find_op(a), "tags")
                for a in ("STORE_NAME", "LOAD_CONST")]
        self.assertIn("name", tags[0])
        self.assertIn("const", tags[1])

    def test_colorize_jump(self):
        window = self.window
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "while x:\n    pass\n")   # A loop jumps.
        window.populate()
        self.assertTrue(any("jump" in window.tree.item(it, "tags")
                            for it in walk(window.tree)))

    def test_syntax_error(self):
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "def f(:\n")       # Invalid syntax.
        self.window.populate()
        self.assertIn("incomplete", self.window.status.cget("text"))
        self.assertEqual(self.window.tree.get_children(), ())

    def test_sync_cursor_selects_instructions(self):
        window = self.window
        self.text.mark_set("insert", "4.11")       # At 'x' in 'return x + 1'.
        window.sync_from_editor()
        selection = window.tree.selection()
        self.assertIn(self.instr_at(("4.11", "4.12")), selection)
        self.assertEqual(self.text.index("insert"), "4.11")

    def test_sync_selection_selects_overlap(self):
        window = self.window
        self.text.tag_add("sel", "4.11", "4.16")   # 'x + 1'.
        window.sync_from_editor()
        selection = window.tree.selection()
        self.assertIn(self.instr_at(("4.11", "4.12")), selection)

    def test_sync_selects_only_target_code_object(self):
        # A cursor inside f selects rows in f, not the module's def-building
        # instructions (which span the whole def), and opens f.
        window = self.window
        self.text.mark_set("insert", "4.11")       # Inside f's body.
        window.sync_from_editor()
        selection = window.tree.selection()
        f_node = self.code_node("f")
        self.assertTrue(selection)
        for item in selection:
            self.assertEqual(window.tree.parent(item), f_node)
        self.assertTrue(window.tree.item(f_node, "open"))

    def test_sync_cursor_at_line_boundaries(self):
        # At the start (indentation) or end of a line, no instruction range
        # contains the point, but its line's instructions are still selected.
        window = self.window
        f_node = self.code_node("f")
        for where in ("4.0", "4.16"):     # Start (indent) and end of line 4.
            self.text.mark_set("insert", where)
            window.sync_from_editor()
            selection = window.tree.selection()
            self.assertTrue(selection, f"nothing selected at {where}")
            for item in selection:
                self.assertEqual(window.tree.parent(item), f_node)
            self.assertTrue(window.tree.item(f_node, "open"))

    def test_sync_module_level_selects_module_rows(self):
        window = self.window
        self.text.mark_set("insert", "1.4")        # In 'import sys'.
        window.sync_from_editor()
        selection = window.tree.selection()
        module = self.code_node("<module>")
        self.assertTrue(selection)
        for item in selection:
            self.assertEqual(window.tree.parent(item), module)

    def test_focused_highlights_and_moves_cursor(self):
        # Browser drives the selection (it has focus): highlight the source
        # and move the cursor to it.
        window = self.window
        window.focused = True
        window.tree.selection_set(self.instr_at(("4.11", "4.12")))
        window.select_instrs()
        ranges = [str(i) for i in self.text.tag_ranges(disbrowser.TAG)]
        self.assertEqual(ranges, ["4.11", "4.12"])
        self.assertEqual(self.text.index("insert"), "4.11")

    def test_not_focused_keeps_editor_clean(self):
        # Editor drives the selection (browser not focused): select_instrs
        # neither highlights the editor nor moves its cursor.
        window = self.window
        window.focused = False
        self.text.mark_set("insert", "1.0")
        window.tree.selection_set(self.instr_at(("4.11", "4.12")))
        window.select_instrs()
        self.assertEqual(self.text.tag_ranges(disbrowser.TAG), ())
        self.assertEqual(self.text.index("insert"), "1.0")

    def test_highlight_follows_focus(self):
        window = self.window
        window.tree.selection_set(self.instr_at(("4.11", "4.12")))
        window.on_focus_in()           # The browser has focus.
        self.assertNotEqual(self.text.tag_ranges(disbrowser.TAG), ())
        window.on_focus_out()          # Focus moves to the editor.
        self.assertEqual(self.text.tag_ranges(disbrowser.TAG), ())
        window.on_focus_in()           # Focus returns to the browser.
        self.assertNotEqual(self.text.tag_ranges(disbrowser.TAG), ())

    def test_instruction_without_location_not_highlighted(self):
        # The module's leading RESUME has no real source position: it is
        # absent from ranges and selecting it produces no highlight.
        window = self.window
        window.focused = True
        item = self.find_op("RESUME")  # The module's, reached first.
        self.assertNotIn(item, window.ranges)
        window.tree.selection_set(item)
        window.select_instrs()
        self.assertEqual(self.text.tag_ranges(disbrowser.TAG), ())

    def test_refresh(self):
        window = self.window
        self.text.delete("1.0", "end")
        self.text.insert("1.0", "spam = 1\n")
        window.refresh()
        # The tree was rebuilt: some instruction now stores 'spam'.
        self.assertTrue(any(window.tree.set(it, "arg") == "spam"
                            for it in window.instr_items))

    def test_move_cursor(self):
        window = self.window
        window.move_cursor(self.instr_at(("4.11", "4.12")))
        self.assertEqual(self.text.index("insert"), "4.11")
        # A row without a source range (the module RESUME) leaves it put.
        window.move_cursor(self.find_op("RESUME"))
        self.assertEqual(self.text.index("insert"), "4.11")

    def test_goto_instr(self):
        # Double-clicking a row moves the cursor there and hides the browser.
        text = Text(self.root)
        text.insert("1.0", code_sample)
        window = disbrowser.DisBrowserWindow(self.root, text, _utest=True)
        item = find_instr(window, ("4.11", "4.12"))
        event = Func()
        event.y = 5
        with mock.patch.object(window.tree, 'identify_row', Func(result=item)):
            result = window.goto_instr(event)
        self.assertEqual(text.index("insert"), "4.11")
        self.assertEqual(window.wm_state(), "withdrawn")
        self.assertEqual(result, "break")
        window.destroy()
        text.destroy()

    def test_hide(self):
        text = Text(self.root)
        text.insert("1.0", code_sample)
        window = disbrowser.DisBrowserWindow(self.root, text, _utest=True)
        window.deiconify()
        window.focused = True
        item = find_instr(window, ("4.11", "4.12"))
        window.tree.selection_set(item)
        window.select_instrs()
        self.assertNotEqual(text.tag_ranges(disbrowser.TAG), ())
        window.hide()                  # Double-click (or Escape) hides it.
        self.assertEqual(window.wm_state(), "withdrawn")   # Not destroyed.
        self.assertTrue(window.winfo_exists())
        self.assertEqual(text.tag_ranges(disbrowser.TAG), ())
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
        window = disbrowser.DisBrowserWindow(self.root, text, _utest=True)
        self.assertEqual(window.base, (1, 0))
        self.assertIn("in text", window.status.cget("text"))
        window.destroy()
        text.destroy()


class DisBrowserDebugTest(GuiTest):
    "Test following the debugger instead of the editor (debug mode)."

    def module_code(self):
        "Compile the sample as a module, with a definite filename."
        return compile(code_sample, "/dir/sample.py", "exec")

    def func_code(self):
        "The code object of f() nested in the sample."
        return next(c for c in self.module_code().co_consts
                    if hasattr(c, "co_code"))

    def an_offset(self, code):
        "A real instruction offset in code (past the leading RESUME)."
        return list(dis.get_instructions(code))[1].offset

    def test_show_running_builds_from_code(self):
        w = self.window
        code = self.func_code()
        w.show_running(code, self.an_offset(code))
        self.assertTrue(w.debugging)
        roots = w.tree.get_children("")
        self.assertEqual(w.tree.item(roots[0], "text"), code.co_qualname)

    def test_show_running_marks_current_offset(self):
        w = self.window
        code = self.func_code()
        offset = self.an_offset(code)
        w.show_running(code, offset)
        selection = w.tree.selection()
        self.assertEqual(len(selection), 1)
        item = selection[0]
        self.assertEqual(w.tree.item(item, "text"), str(offset))
        self.assertIn("current", w.tree.item(item, "tags"))
        self.assertTrue(w.tree.item(w.tree.parent(item), "open"))  # Revealed.

    def test_show_running_unknown_offset_marks_nothing(self):
        w = self.window
        code = self.func_code()
        w.show_running(code, -1)      # No instruction lives at offset -1.
        self.assertFalse(w.tree.selection())

    def test_leave_debug_mode_reverts_to_editor(self):
        w = self.window
        code = self.func_code()
        w.show_running(code, self.an_offset(code))
        w.leave_debug_mode()
        self.assertFalse(w.debugging)
        roots = w.tree.get_children("")
        self.assertEqual(w.tree.item(roots[0], "text"), "<module>")

    def test_leave_debug_mode_when_not_debugging_is_noop(self):
        w = self.window
        before = w.tree.get_children("")
        w.leave_debug_mode()
        self.assertEqual(w.tree.get_children(""), before)

    def test_sync_from_editor_inert_while_debugging(self):
        w = self.window
        code = self.func_code()
        w.show_running(code, self.an_offset(code))
        current = w.tree.selection()
        w.text.mark_set("insert", "1.0")
        w.sync_from_editor()
        self.assertEqual(w.tree.selection(), current)   # Unchanged.

    def test_map_source_off_when_file_differs(self):
        w = self.window            # editwin is None: no file to map onto.
        code = self.module_code()
        w.show_running(code, self.an_offset(code))
        self.assertFalse(w.map_source)
        self.assertFalse(w.ranges)

    def test_map_source_on_when_file_matches(self):
        w = self.window
        w.editwin = types.SimpleNamespace(
            io=types.SimpleNamespace(filename="/dir/sample.py"))
        code = self.module_code()  # Compiled with that same filename.
        w.show_running(code, self.an_offset(code))
        self.assertTrue(w.map_source)
        self.assertTrue(w.ranges)  # Instruction positions mapped to the editor.

    def test_sync_from_debugger_shows_stopped_frame(self):
        w = self.window
        code = self.func_code()
        offset = self.an_offset(code)
        debugger = types.SimpleNamespace(
            current_frame_code=lambda: (code, offset))
        w.active_debugger = lambda: debugger
        try:
            w.sync_from_debugger()
        finally:
            del w.active_debugger
        self.assertTrue(w.debugging)
        self.assertEqual(w.tree.item(w.tree.selection()[0], "text"), str(offset))

    def test_sync_from_debugger_none_leaves_debug_mode(self):
        w = self.window
        code = self.func_code()
        w.show_running(code, self.an_offset(code))
        w.active_debugger = lambda: None
        try:
            w.sync_from_debugger()
        finally:
            del w.active_debugger
        self.assertFalse(w.debugging)


if __name__ == '__main__':
    unittest.main(verbosity=2)
