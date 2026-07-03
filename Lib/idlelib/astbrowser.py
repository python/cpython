"""An AST browser for IDLE.

The Browse menu's "AST Browser" command (see open() below) opens a window
showing the abstract syntax tree of the editor content (or, in the Shell,
the current input), or of the selection if there is one.  Selecting a node
highlights the matching region in the editor and moves the editor cursor
there; selecting text or moving the cursor in the editor selects the
innermost matching node.  Double-clicking a node hides the browser (as
does Escape), revealing the editor at the node.
"""
import ast

from tkinter import Toplevel, TclError
from tkinter import TOP, BOTTOM, LEFT, RIGHT, X, Y, BOTH, W, END, VERTICAL
from tkinter import ttk

from idlelib.config import idleConf

# The editor tag that highlights the source of the selected nodes.
TAG = "ASTBROWSER"


def open(editwin):
    "Open the AST browser for editwin, reusing one already open."
    window = getattr(editwin, "ast_browser", None)
    if window is not None and window.winfo_exists():
        window.refresh()
    else:
        editwin.ast_browser = ASTBrowserWindow(editwin.top, editwin.text)


class ASTBrowserWindow(Toplevel):
    "Show the abstract syntax tree of a Text widget's content or selection."

    def __init__(self, parent, text, *, _htest=False, _utest=False):
        """Create the AST browser.

        parent - the master widget of this window.
        text - the editor Text widget to browse and drive.
        _htest - bool; change box location when running htest.
        _utest - bool; don't wait for user interaction when unit testing.
        """
        super().__init__(parent)
        self.text = text
        self.base = (1, 0)      # Editor index of the parsed region's start.
        self.source_lines = []  # Lines of the parsed source (for byte->char).
        self.ranges = {}        # Tree item id -> (start index, end index).
        self.focused = False    # Whether the browser currently has the focus.
        self.title("AST Browser")
        self.protocol("WM_DELETE_WINDOW", self.hide)
        self.bind("<Escape>", self.hide)
        x = parent.winfo_rootx() + 20
        y = parent.winfo_rooty() + (100 if _htest else 20)
        self.geometry(f"640x480+{x}+{y}")
        self.minsize(400, 300)

        self.create_widgets()
        self.configure_tag()
        self.populate()
        # Follow the editor and select the matching node.  <<Selection>> covers
        # selection changes by keyboard or mouse (a generic <KeyRelease> is
        # shadowed by IDLE's specific key bindings); the release events cover
        # plain cursor moves that leave no selection.  These bindings live as
        # long as the editor Text and are torn down together with it (and with
        # this child window), so there is nothing to unbind.
        text.bind("<<Selection>>", self.sync_from_editor, add="+")
        text.bind("<KeyRelease>", self.sync_from_editor, add="+")
        text.bind("<ButtonRelease-1>", self.sync_from_editor, add="+")
        if not _utest:
            self.deiconify()

    def create_widgets(self):
        bar = ttk.Frame(self, padding=(6, 6, 6, 0))
        bar.pack(side=TOP, fill=X)
        ttk.Button(bar, text="Refresh", command=self.populate).pack(side=LEFT)

        self.status = ttk.Label(self, anchor=W, relief="sunken", padding=3)
        self.status.pack(side=BOTTOM, fill=X)

        frame = ttk.Frame(self, padding=6)
        frame.pack(side=TOP, fill=BOTH, expand=True)
        self.tree = ttk.Treeview(frame, show="tree", selectmode="extended")
        vbar = ttk.Scrollbar(frame, orient=VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=vbar.set)
        vbar.pack(side=RIGHT, fill=Y)
        self.tree.pack(side=LEFT, fill=BOTH, expand=True)
        self.tree.bind("<<TreeviewSelect>>", self.select_nodes)
        self.tree.bind("<Double-Button-1>", self.goto_node)
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
        "Map an AST (lineno, byte col) to an editor index, honoring the base."
        if lineno <= len(self.source_lines):
            # col_offset is a UTF-8 byte offset; convert it to a character one.
            col = len(self.source_lines[lineno - 1].encode()[:col]
                      .decode(errors="replace"))
        base_row, base_col = self.base
        if lineno == 1:
            col += base_col
        return f"{base_row + lineno - 1}.{col}"

    def node_range(self, node):
        "Return the (start, end) editor indices of a node, or None."
        if getattr(node, "lineno", None) is None or node.end_lineno is None:
            return None
        return (self.editor_index(node.lineno, node.col_offset),
                self.editor_index(node.end_lineno, node.end_col_offset))

    def populate(self, event=None):
        "Parse the content (or selection) and fill the tree."
        self.hide_highlight()
        self.tree.delete(*self.tree.get_children())
        self.ranges.clear()
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
        self.source_lines = source.splitlines()
        error = count = None
        try:
            tree = ast.parse(source)
        except SyntaxError as exc:
            error = exc.msg
        else:
            count = self.add_node("", "", tree)
        status = f"{count or 0} nodes in {scope}"
        if error:
            status += f"  —  incomplete: {error}"
        self.status.configure(text=status)
        self.sync_from_editor()

    def add_node(self, parent_item, field, node):
        "Insert a node and its descendants; return the number of nodes added."
        inline = []           # Fields shown in this row: 'name=value'.
        children = []         # Fields shown as child rows: (label, node).
        for name, value in ast.iter_fields(node):
            if isinstance(value, ast.AST):
                if value._fields:
                    children.append((name, value))
                else:         # An operator or context, e.g. Add, Load.
                    inline.append(f"{name}={type(value).__name__}")
            elif isinstance(value, list):
                nodes = [(f"{name}[{i}]", elt) for i, elt in enumerate(value)
                         if isinstance(elt, ast.AST)]
                if nodes:
                    children += nodes
                elif value:   # A non-empty list of scalars; drop empty ones.
                    inline.append(f"{name}={value!r}")
            elif value is not None or name == "value":  # Keep the None literal.
                inline.append(f"{name}={value!r}")

        label = type(node).__name__
        if inline:
            label += "(" + ", ".join(inline) + ")"
        if field:
            label = f"{field}: {label}"
        item = self.tree.insert(parent_item, END, text=label, open=True)
        if rng := self.node_range(node):
            self.ranges[item] = rng

        count = 1
        for name, child in children:
            count += self.add_node(item, name, child)
        return count

    def refresh(self):
        "Re-parse the current range and bring the browser to the front."
        self.populate()
        self.deiconify()
        self.lift()
        self.focus_set()

    def sync_from_editor(self, event=None):
        "Select the innermost node matching the editor's selection or cursor."
        first, last = self.editor_selection()
        if not (first and last):
            first = last = self.text.index("insert")
        self.select_rows(self.enclosing_node(first, last))

    def enclosing_node(self, first, last):
        "Return [item] of the smallest node covering [first, last], or []."
        best = None
        for item, (start, end) in self.ranges.items():
            if (self.text.compare(start, "<=", first)
                    and self.text.compare(last, "<=", end)):
                # Covering nodes are nested; keep the tightest (deepest) one.
                if best is None or (self.text.compare(start, ">=", best[1])
                                    and self.text.compare(end, "<=", best[2])):
                    best = (item, start, end)
        return [best[0]] if best else []

    def select_rows(self, items):
        "Select the given tree rows and reveal the first."
        if items:
            self.tree.selection_set(items)
            self.tree.focus(items[0])
            self.tree.see(items[0])

    def select_nodes(self, event=None):
        "Highlight the selected nodes and, while focused, follow with the cursor."
        self.show_highlight(see=True)
        # Move the editor cursor only when the browser drives the selection
        # (it has the focus).  When the editor drives it, the browser is not
        # focused, so the cursor is left alone and there is no feedback loop.
        if self.focused:
            self.move_cursor()

    def show_highlight(self, see=False):
        "Highlight the selected nodes' source while the browser has focus."
        if not self.focused:  # Keep the editor clean while it is in use.
            return
        text = self.text
        self.hide_highlight()
        first = None
        for item in self.tree.selection():
            rng = self.ranges.get(item)
            if rng and rng[0] != rng[1]:  # Skip nodes with no source span.
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

    def goto_node(self, event=None):
        "Move the cursor to the double-clicked node and hide the browser."
        self.move_cursor(self.tree.identify_row(event.y))
        self.hide()
        return "break"      # Suppress the default double-click handling.

    def move_cursor(self, item=None):
        "Move the editor cursor to a node (the first selected row by default)."
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


def _ast_browser(parent):  # htest #
    "Set up a sample editor Text and open an AST browser on it."
    from tkinter import Text
    top = Toplevel(parent)
    top.title("Sample editor")
    text = Text(top, width=40, height=8)
    text.insert("1.0", "import sys\n\ndef f(x):\n    return x + 1  # add one\n")
    text.pack(fill=BOTH, expand=True)
    return ASTBrowserWindow(top, text, _htest=True)


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_astbrowser', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(_ast_browser)
