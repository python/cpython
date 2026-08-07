"""A disassembly browser for IDLE.

The Browse menu's "Disassembly Browser" command (see open() below) shows
the disassembled bytecode of the editor content, the Shell input, or the
selection, as collapsible code objects.  Selecting an instruction highlights
the matching source and moves the cursor there; selecting source selects the
instructions built from it.  Double-clicking a row, or Escape, hides the
browser.

While the debugger is stopped, the browser follows it instead of the editor,
showing the code object that is executing and marking the current instruction.
"""
import dis
import os
import types

from tkinter import Toplevel, TclError
from tkinter import TOP, BOTTOM, LEFT, RIGHT, X, Y, BOTH, W, END, VERTICAL
from tkinter import ttk

from idlelib.config import idleConf

# The editor tag that highlights the source of the selected instructions.
TAG = "DISBROWSER"

# Background of the instruction the debugger is stopped at (see show_running).
CURRENT_BG = "#ffe9a8"

# Row colors for code-object headers and for instructions grouped by the
# operand they act on (mirroring the opcode collections in the dis module).
GROUP_COLORS = {
    'code': '#7f0055',   # A code-object header (BOLD MAGENTA).
    'jump': '#0000cc',   # Control flow (BOLD BLUE).
    'name': '#008700',   # Globals, attributes, imports (GREEN).
    'local': '#0086b3',  # Locals and cells (CYAN).
    'const': '#a67c00',  # Constants (YELLOW).
    'exc': '#cc0000',    # Exception setup (RED).
}


def opcode_groups():
    "Map opcode numbers to a color group name (mirrors the dis collections)."
    groups = {}
    for group, opcodes in (
            ('jump', dis.hasjump),
            ('name', dis.hasname),
            ('local', dis.haslocal + dis.hasfree),
            ('const', dis.hasconst),
            ('exc', dis.hasexc)):
        for op in opcodes:
            groups.setdefault(op, group)
    return groups


OPCODE_GROUPS = opcode_groups()


def open(editwin):
    "Open the disassembly browser for editwin, reusing one already open."
    window = getattr(editwin, "disassembly_browser", None)
    if window is not None and window.winfo_exists():
        window.refresh()
    else:
        editwin.disassembly_browser = DisBrowserWindow(
            editwin.top, editwin.text, editwin)


class DisBrowserWindow(Toplevel):
    "Show the disassembly of a Text widget's content or selection."

    def __init__(self, parent, text, editwin=None, *,
                 _htest=False, _utest=False):
        """Create the disassembly browser.

        parent - the master widget of this window.
        text - the editor Text widget to browse and drive.
        editwin - the owning editor window (for debugger integration), or None.
        _htest - bool; change box location when running htest.
        _utest - bool; don't wait for user interaction when unit testing.
        """
        super().__init__(parent)
        self.text = text
        self.editwin = editwin
        self.base = (1, 0)      # Editor index of the compiled region's start.
        self.source_lines = []  # Lines of the compiled source (for byte->char).
        self.ranges = {}        # Tree item id -> (start index, end index).
        self.instr_items = set()  # Tree items that are instructions (not code).
        self.focused = False    # Whether the browser currently has the focus.
        self.debugging = False  # Whether it is showing a stopped debug frame.
        self.map_source = True  # Whether instruction ranges map to the editor.
        self.title("Disassembly Browser")
        self.protocol("WM_DELETE_WINDOW", self.hide)
        self.bind("<Escape>", self.hide)
        x = parent.winfo_rootx() + 20
        y = parent.winfo_rooty() + (100 if _htest else 20)
        self.geometry(f"640x480+{x}+{y}")
        self.minsize(400, 300)

        self.create_widgets()
        self.configure_tag()
        self.populate()
        # Follow the editor and select the matching instructions.  <<Selection>>
        # covers selection changes by keyboard or mouse (a generic <KeyRelease>
        # is shadowed by IDLE's specific key bindings); the release events cover
        # plain cursor moves that leave no selection.  These bindings live as
        # long as the editor Text and are torn down together with it (and with
        # this child window), so there is nothing to unbind.
        text.bind("<<Selection>>", self.sync_from_editor, add="+")
        text.bind("<KeyRelease>", self.sync_from_editor, add="+")
        text.bind("<ButtonRelease-1>", self.sync_from_editor, add="+")
        # Follow the debugger: it fires these on the Shell text when it stops
        # at, or steps to, a frame, and when it is closed.  Binding on the
        # long-lived Shell text works whichever opens first.
        self._shell_text = None
        shell = self.shell()
        if shell is not None:
            self._shell_text = shell.text
            self._stop_bind = shell.text.bind(
                "<<debugger-stopped>>", self.sync_from_debugger, add="+")
            self._off_bind = shell.text.bind(
                "<<debugger-off>>", self.leave_debug_mode, add="+")
            self.bind("<Destroy>", self.unbind_debugger)
        if not _utest:
            self.deiconify()
        self.sync_from_debugger()   # Show a frame if already stopped.

    def create_widgets(self):
        bar = ttk.Frame(self, padding=(6, 6, 6, 0))
        bar.pack(side=TOP, fill=X)
        ttk.Button(bar, text="Refresh", command=self.populate).pack(side=LEFT)

        self.status = ttk.Label(self, anchor=W, relief="sunken", padding=3)
        self.status.pack(side=BOTTOM, fill=X)

        frame = ttk.Frame(self, padding=6)
        frame.pack(side=TOP, fill=BOTH, expand=True)
        # Each code object is a collapsible top-level row; the tree column
        # (#0) holds its name and, for its instruction children, the offset.
        self.tree = ttk.Treeview(frame, columns=("mark", "opname", "arg"),
                                 show="tree headings", selectmode="extended")
        self.tree.heading("#0", text="Code / offset", anchor=W)
        self.tree.column("#0", width=170, stretch=False, anchor=W)
        for name, title, width, stretch in (
                ("mark", "", 32, False),        # ">>" for jump targets.
                ("opname", "Instruction", 200, False),
                ("arg", "Argument", 200, True)):
            self.tree.heading(name, text=title, anchor=W)
            self.tree.column(name, width=width, stretch=stretch, anchor=W)
        for group, color in GROUP_COLORS.items():
            self.tree.tag_configure(group, foreground=color)
        self.tree.tag_configure("current", background=CURRENT_BG)
        vbar = ttk.Scrollbar(frame, orient=VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=vbar.set)
        vbar.pack(side=RIGHT, fill=Y)
        self.tree.pack(side=LEFT, fill=BOTH, expand=True)
        self.tree.bind("<<TreeviewSelect>>", self.select_instrs)
        self.tree.bind("<Double-Button-1>", self.goto_instr)
        # The highlight is shown only while the browser has the focus.
        self.bind("<FocusIn>", self.on_focus_in)
        self.bind("<FocusOut>", self.on_focus_out)

    def configure_tag(self):
        "Give the highlight tag the theme's 'hit' colors."
        try:
            colors = idleConf.GetHighlight(idleConf.CurrentTheme(), 'hit')
        except Exception:
            colors = {'foreground': '#000000', 'background': '#ffff80'}
        self.text.tag_configure(TAG, **colors)

    def editor_selection(self):
        "Return the editor's (first, last) selection, or ('', '') if none."
        try:
            # A plain Text raises without a selection; the IDLE editor
            # returns an empty string instead.
            return self.text.index("sel.first"), self.text.index("sel.last")
        except TclError:
            return "", ""

    def editor_index(self, lineno, col):
        "Map a (lineno, byte col) source position to an editor index."
        if lineno <= len(self.source_lines):
            # Position columns are UTF-8 byte offsets; convert them to chars.
            col = len(self.source_lines[lineno - 1].encode()[:col]
                      .decode(errors="replace"))
        base_row, base_col = self.base
        if lineno == 1:
            col += base_col
        return f"{base_row + lineno - 1}.{col}"

    def clear(self):
        "Empty the tree and the source-position bookkeeping."
        self.hide_highlight()
        self.tree.delete(*self.tree.get_children())
        self.ranges.clear()
        self.instr_items.clear()

    def populate(self, event=None):
        "Compile the content (or selection) and fill the tree."
        self.debugging = False   # Editor view: leave any debug frame behind.
        self.map_source = True   # Instruction positions map to the editor.
        self.clear()
        text = self.text
        first, last = self.editor_selection()
        if first and last:
            scope = "selection"
        else:
            last = text.index("end-1c")
            # In the Shell, browse just the current input, which starts at the
            # "iomark"; a plain editor has no such mark.  IDLE's editor returns
            # '' for a missing mark, while a plain Text raises TclError.
            try:
                first = text.index("iomark")
            except TclError:
                first = ""
            if first:
                scope = "input"
            else:
                first, scope = "1.0", "text"
        self.base = tuple(int(i) for i in first.split("."))
        source = text.get(first, last)
        # Split at "\n" only; str.splitlines() would also break at "\f" etc.
        self.source_lines = source.split("\n")
        error = None
        try:
            code = compile(source, "<disassembly>", "exec")
        except (SyntaxError, ValueError) as exc:
            error = exc.msg if isinstance(exc, SyntaxError) else str(exc)
        else:
            self.add_code(code)
        status = f"{len(self.instr_items)} instructions in {scope}"
        if error:
            status += f"  —  incomplete: {error}"
        self.status.configure(text=status)
        # Every code object starts collapsed (add_code); syncing to the cursor
        # opens and selects the one it is in, leaving the rest collapsed.
        self.sync_from_editor()

    def add_code(self, code):
        "Add a top-level code-object row with its instructions as children."
        item = self.tree.insert("", END, text=code.co_qualname,
                                tags=("code",), open=False)
        starts, ends = [], []    # Position tuples, for the code row's span.
        for instr in dis.get_instructions(code):
            group = OPCODE_GROUPS.get(instr.opcode)   # None: the default color.
            child = self.tree.insert(
                item, END, text=str(instr.offset),
                values=(">>" if instr.is_jump_target else "",
                        instr.opname, instr.argrepr),
                tags=(group,) if group else ())
            pos = instr.positions
            # Some instructions (e.g. a module's RESUME) carry no real source
            # position: a missing field or a line before the first one.  In a
            # debug frame whose file is not the editor's, positions do not map
            # to it, so self.map_source suppresses the (bogus) ranges.
            if self.map_source and pos and None not in pos and pos.lineno >= 1:
                self.ranges[child] = (
                    self.editor_index(pos.lineno, pos.col_offset),
                    self.editor_index(pos.end_lineno, pos.end_col_offset))
                self.instr_items.add(child)
                starts.append((pos.lineno, pos.col_offset))
                ends.append((pos.end_lineno, pos.end_col_offset))
        # Give the header row the span of its own instructions (compared as
        # numeric positions, not editor-index strings), so selecting it
        # highlights where the code lives.
        if starts:
            self.ranges[item] = (self.editor_index(*min(starts)),
                                 self.editor_index(*max(ends)))
        # Nested code objects (functions, classes, comprehensions, ...) follow.
        for const in code.co_consts:
            if isinstance(const, types.CodeType):
                self.add_code(const)

    def refresh(self):
        "Re-compile the current range and bring the browser to the front."
        if self.debugging:
            self.sync_from_debugger()   # Stay on the stopped frame.
        else:
            self.populate()
        self.deiconify()
        self.lift()
        self.focus_set()

    # -- Debugger integration ------------------------------------------------
    #
    # While the debugger is stopped, show the frame it is stopped in: the code
    # object actually executing (marshalled from the subprocess through the
    # debugger RPC), with the instruction at f_lasti marked.  The editor drives
    # the view only when no debugger is stopped (see sync_from_editor).

    def shell(self):
        "Return the Shell window that runs and debugs code, or None."
        try:
            return self.editwin.flist.pyshell
        except AttributeError:
            return None

    def active_debugger(self):
        "Return the Debugger currently attached to the Shell, or None."
        try:
            return self.shell().interp.debugger
        except AttributeError:
            return None

    def unbind_debugger(self, event=None):
        "Drop the Shell-text bindings when this window is destroyed."
        if event is not None and event.widget is not self:
            return   # A child widget's <Destroy>, not the window's.
        text, self._shell_text = self._shell_text, None
        if text is not None:
            try:
                text.unbind("<<debugger-stopped>>", self._stop_bind)
                text.unbind("<<debugger-off>>", self._off_bind)
            except TclError:
                pass   # The Shell is already gone.

    def sync_from_debugger(self, event=None):
        "Show the frame the debugger is stopped in, or leave debug mode."
        debugger = self.active_debugger()
        info = debugger.current_frame_code() if debugger is not None else None
        if info is None:
            self.leave_debug_mode()
        else:
            self.show_running(*info)

    def leave_debug_mode(self, event=None):
        "Return from a stopped frame to the editor's own disassembly."
        if self.debugging:
            self.populate()   # Clears self.debugging and rebuilds from editor.

    def editor_shows(self, filename):
        "Whether the editor's file is the given code object's file."
        try:
            editor_file = self.editwin.io.filename
        except AttributeError:
            editor_file = None
        if not editor_file or not filename:
            return False
        return (os.path.normcase(os.path.abspath(editor_file))
                == os.path.normcase(os.path.abspath(filename)))

    def show_running(self, code, offset):
        "Disassemble a running code object; mark the instruction at offset."
        self.debugging = True
        self.clear()
        self.base = (1, 0)
        self.source_lines = []
        # Positions map to the editor only when it shows this frame's file.
        self.map_source = self.editor_shows(code.co_filename)
        if self.map_source:
            self.source_lines = self.text.get("1.0", "end-1c").split("\n")
        self.add_code(code)   # The frame's own code is the first top-level row.
        self.mark_current(offset)
        self.status.configure(
            text=f"stopped in {code.co_qualname} at offset {offset}")

    def mark_current(self, offset):
        "Open, tag and reveal the instruction at offset in the frame's code."
        roots = self.tree.get_children("")
        if not roots:
            return
        code_item = roots[0]   # add_code added the frame's own code first.
        for child in self.tree.get_children(code_item):
            if self.tree.item(child, "text") == str(offset):
                # An untagged row reports its tags as '' (not an empty tuple).
                tags = self.tree.item(child, "tags")
                if isinstance(tags, str):
                    tags = (tags,) if tags else ()
                self.tree.item(child, tags=(*tags, "current"))
                self.tree.item(code_item, open=True)
                self.tree.selection_set(child)
                self.tree.see(child)
                break

    def editor_location(self):
        "Return the (first, last) editor range to sync to (selection or cursor)."
        first, last = self.editor_selection()
        if not (first and last):
            first = last = self.text.index("insert")
        return first, last

    def code_object_at(self, first, last):
        "Return the innermost top-level code row whose span covers [first, last]."
        text = self.text
        best = None
        for item in self.tree.get_children(""):
            rng = self.ranges.get(item)
            if (rng and "code" in self.tree.item(item, "tags")
                    and text.compare(rng[0], "<=", first)
                    and text.compare(last, "<=", rng[1])):
                # Code-object spans nest; keep the tightest (innermost) one.
                if best is None or (
                        text.compare(rng[0], ">=", self.ranges[best][0])
                        and text.compare(rng[1], "<=", self.ranges[best][1])):
                    best = item
        return best

    def sync_from_editor(self, event=None):
        "Select the matching rows within the code object holding the location."
        if self.debugging:
            return   # The debugger drives the view; ignore editor moves.
        text = self.text
        first, last = self.editor_location()
        # Only the code object that the cursor/selection is in should respond;
        # its own instructions, not a caller's code that merely spans the def.
        target = self.code_object_at(first, last)
        if target is None:
            return self.select_rows([])
        rows = [it for it in self.tree.get_children(target)
                if it in self.instr_items]
        if first == last:   # A bare cursor: rows whose range contains it.
            items = [it for it in rows
                     if text.compare(self.ranges[it][0], "<=", first)
                     and text.compare(first, "<", self.ranges[it][1])]
            if not items:
                # At the start (indentation) or end of a line the point is in
                # no half-open range; use the instructions on that line.
                line = int(first.split(".")[0])
                first, last = f"{line}.0", f"{line + 1}.0"
        if first != last:   # A selection or a whole line: overlapping rows.
            items = [it for it in rows
                     if text.compare(self.ranges[it][0], "<", last)
                     and text.compare(self.ranges[it][1], ">", first)]
        if items:
            self.tree.item(target, open=True)   # Open it to reveal the rows.
        self.select_rows(items)

    def select_rows(self, items):
        "Select the given tree rows and reveal the first."
        if items:
            self.tree.selection_set(items)
            self.tree.focus(items[0])
            self.tree.see(items[0])

    def select_instrs(self, event=None):
        "Highlight the selection and, while focused, follow with the cursor."
        self.show_highlight(see=True)
        # Move the editor cursor only when the browser drives the selection
        # (it has the focus).  When the editor drives it, the browser is not
        # focused, so the cursor is left alone and there is no feedback loop.
        if self.focused:
            self.move_cursor()

    def show_highlight(self, see=False):
        "Highlight the selected rows' source while the browser has focus."
        if not self.focused:  # Keep the editor clean while it is in use.
            return
        text = self.text
        self.hide_highlight()
        first = None
        for item in self.tree.selection():
            rng = self.ranges.get(item)
            if rng and rng[0] != rng[1]:  # Skip rows with no source span.
                text.tag_add(TAG, *rng)
                if first is None:
                    first = rng[0]
        text.tag_raise(TAG)
        if see and first is not None:
            text.see(first)

    def on_focus_in(self, event=None):
        "Restore the highlight when the browser regains focus."
        self.focused = True
        self.show_highlight()

    def on_focus_out(self, event=None):
        "Hide the highlight while the editor (or another window) has focus."
        self.focused = False
        self.hide_highlight()

    def goto_instr(self, event=None):
        "Move the cursor to the double-clicked row and hide the browser."
        self.move_cursor(self.tree.identify_row(event.y))
        self.hide()
        return "break"      # Suppress the default double-click handling.

    def move_cursor(self, item=None):
        "Move the editor cursor to a row (the first selected row by default)."
        if item is None:
            selection = self.tree.selection()
            item = selection[0] if selection else None
        rng = self.ranges.get(item)
        if rng:
            self.text.mark_set("insert", rng[0])
            self.text.see(rng[0])

    def hide(self, event=None):
        """Withdraw the browser, revealing the editor and giving it focus.

        Hiding our own window sidesteps the window manager's focus-stealing
        prevention, which blocks a background editor window from being raised.
        """
        self.hide_highlight()
        self.withdraw()
        self.text.focus_set()

    def hide_highlight(self, event=None):
        try:
            self.text.tag_remove(TAG, "1.0", "end")
        except TclError:  # The editor may already be gone.
            pass


def _disassembly_browser(parent):  # htest #
    "Set up a sample editor Text and open a disassembly browser on it."
    from tkinter import Text
    top = Toplevel(parent)
    top.title("Sample editor")
    text = Text(top, width=40, height=8)
    text.insert("1.0", "import sys\n\ndef f(x):\n    return x + 1  # add one\n")
    text.pack(fill=BOTH, expand=True)
    return DisBrowserWindow(top, text, _htest=True)


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_disbrowser', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(_disassembly_browser)
