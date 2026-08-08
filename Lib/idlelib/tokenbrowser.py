"""A token browser for IDLE.

The Browse menu's "Token Browser" command (see open() below) opens a
window listing the Python tokens of the editor content (or, in the Shell,
the current input), or of the selection if there is one.  Selecting rows
highlights the matching regions in the editor and moves the editor cursor
there; selecting text (or moving the cursor) in the editor selects the
matching rows.  Double-clicking a row hides the browser (as does Escape),
revealing the editor at the token.
"""
import io
import token
import tokenize

from tkinter import Toplevel, TclError
from tkinter import TOP, BOTTOM, LEFT, RIGHT, X, Y, BOTH, W, END, VERTICAL
from tkinter import ttk

from idlelib.config import idleConf

# The editor tag that highlights the tokens of the selected rows.
TAG = "TOKENBROWSER"

# Row colors per token group, mirroring the "python -m tokenize" CLI
# (see tokenize._get_token_colors and _colorize.Syntax/Tokenize).  Token
# groups not listed here (NAME, OP) keep the default foreground.
GROUP_COLORS = {
    'comment': '#cc0000',       # RED
    'string': '#008700',        # GREEN
    'number': '#a67c00',        # YELLOW
    'soft_keyword': '#0000cc',  # BOLD_BLUE
    'whitespace': '#808080',    # GREY
    'error': '#e40000',         # BOLD_RED
}


def token_groups():
    "Map token type numbers to a color group name (mirrors the CLI)."
    groups = {}
    for group, names in (
            ('comment', ['COMMENT']),
            ('whitespace', ['DEDENT', 'ENCODING', 'ENDMARKER', 'INDENT',
                            'NEWLINE', 'NL']),
            ('error', ['ERRORTOKEN']),
            ('string', ['STRING', 'FSTRING_START', 'FSTRING_MIDDLE',
                        'FSTRING_END', 'TSTRING_START', 'TSTRING_MIDDLE',
                        'TSTRING_END']),
            ('number', ['NUMBER']),
            ('soft_keyword', ['SOFT_KEYWORD'])):
        for name in names:
            value = getattr(token, name, None)
            if value is not None:  # Some token types are version-specific.
                groups[value] = group
    return groups


TOKEN_GROUPS = token_groups()


def open(editwin):
    "Open the token browser for editwin, reusing one already open."
    window = getattr(editwin, "token_browser", None)
    if window is not None and window.winfo_exists():
        window.refresh()
    else:
        editwin.token_browser = TokenBrowserWindow(editwin.top, editwin.text)


class TokenBrowserWindow(Toplevel):
    "List the Python tokens of a Text widget's content or selection."

    def __init__(self, parent, text, *, _htest=False, _utest=False):
        """Create the token browser.

        parent - the master widget of this window.
        text - the editor Text widget to browse and drive.
        _htest - bool; change box location when running htest.
        _utest - bool; don't wait for user interaction when unit testing.
        """
        super().__init__(parent)
        self.text = text
        self.base = (1, 0)    # Editor index of the tokenized region's start.
        self.ranges = {}      # Tree item id -> (start index, end index).
        self.focused = False  # Whether the browser currently has the focus.
        self.title("Token Browser")
        self.protocol("WM_DELETE_WINDOW", self.hide)
        self.bind("<Escape>", self.hide)
        x = parent.winfo_rootx() + 20
        y = parent.winfo_rooty() + (100 if _htest else 20)
        self.geometry(f"640x480+{x}+{y}")
        self.minsize(400, 300)

        self.create_widgets()
        self.configure_tag()
        self.populate()
        # Follow the editor and select the matching rows.  <<Selection>>
        # covers selection changes by keyboard or mouse (a generic <KeyRelease>
        # is shadowed by IDLE's specific key bindings); the release events
        # cover plain cursor moves that leave no selection.  These bindings
        # live as long as the editor Text and are torn down together with it
        # (and with this child window), so there is nothing to unbind.
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
        self.tree = ttk.Treeview(frame, columns=("type", "string"),
                                 show="headings", selectmode="extended")
        for name, title, width, stretch in (
                ("type", "Type", 120, False),
                ("string", "String", 260, True)):
            self.tree.heading(name, text=title)
            self.tree.column(name, width=width, stretch=stretch, anchor=W)
        for group, color in GROUP_COLORS.items():
            self.tree.tag_configure(group, foreground=color)
        vbar = ttk.Scrollbar(frame, orient=VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=vbar.set)
        vbar.pack(side=RIGHT, fill=Y)
        self.tree.pack(side=LEFT, fill=BOTH, expand=True)
        self.tree.bind("<<TreeviewSelect>>", self.select_tokens)
        self.tree.bind("<Double-Button-1>", self.goto_token)
        # Shift + Up/Down extends the selection with the keyboard.
        self.tree.bind("<Shift-Up>", lambda e: self.extend_selection(-1))
        self.tree.bind("<Shift-Down>", lambda e: self.extend_selection(1))
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

    def editor_index(self, row, col):
        "Map a token (row, col) to an editor index, honoring the selection."
        base_row, base_col = self.base
        if row == 1:
            col += base_col
        return f"{base_row + row - 1}.{col}"

    def editor_selection(self):
        "Return the editor's (first, last) selection, or ('', '') if none."
        try:
            # A plain Text raises without a selection; the IDLE editor
            # returns an empty string instead.
            return self.text.index("sel.first"), self.text.index("sel.last")
        except TclError:
            return "", ""

    def populate(self, event=None):
        "Tokenize the content (or selection) and fill the table."
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
        if not source.endswith("\n"):
            source += "\n"
        error = None
        try:
            for tok in tokenize.generate_tokens(io.StringIO(source).readline):
                self.add_token(tok)
        except (tokenize.TokenError, IndentationError, SyntaxError) as exc:
            error = exc.args[0] if exc.args else type(exc).__name__
        status = f"{len(self.ranges)} tokens in {scope}"
        if error:
            status += f"  —  incomplete: {error}"
        self.status.configure(text=status)
        self.sync_from_editor()

    def refresh(self):
        "Re-tokenize the current range and bring the browser to the front."
        self.populate()
        self.deiconify()
        self.lift()
        self.focus_set()

    def sync_from_editor(self, event=None):
        "Select the rows matching the editor's selection, or the cursor's row."
        first, last = self.editor_selection()
        if first and last:
            # Select every token whose range overlaps the editor selection.
            text = self.text
            self.select_rows(
                [item for item, (start, end) in self.ranges.items()
                 if text.compare(start, "<", last)
                 and text.compare(end, ">", first)])
        else:
            self.select_cursor_row()

    def select_cursor_row(self):
        "Select the row of the token that contains the editor's cursor."
        insert = self.text.index("insert")
        chosen = None
        for item, (start, end) in self.ranges.items():
            if self.text.compare(start, "<=", insert):
                chosen = item          # Last token starting at or before it.
                if self.text.compare(insert, "<", end):
                    break              # The cursor is inside this token.
        self.select_rows([chosen] if chosen else [])

    def select_rows(self, items):
        "Select the given tree rows and reveal the first."
        if items:
            self.tree.selection_set(items)
            self.tree.focus(items[0])
            self.tree.see(items[0])

    def add_token(self, tok):
        name = token.tok_name[tok.exact_type]
        start = self.editor_index(*tok.start)
        end = self.editor_index(*tok.end)
        group = TOKEN_GROUPS.get(tok.type, '')   # '' means the default color.
        item = self.tree.insert("", END, values=(name, repr(tok.string)),
                                tags=(group,) if group else ())
        self.ranges[item] = (start, end)

    def select_tokens(self, event=None):
        "Highlight the selected rows and, while focused, follow with the cursor."
        self.show_highlight(see=True)
        # Move the editor cursor only when the browser drives the selection
        # (it has the focus).  When the editor drives it, the browser is not
        # focused, so the cursor is left alone and there is no feedback loop.
        if self.focused:
            self.move_cursor()

    def show_highlight(self, see=False):
        "Highlight the selected rows' tokens while the browser has focus."
        if not self.focused:  # Keep the editor clean while it is in use.
            return
        text = self.text
        self.hide_highlight()
        first = None
        for item in self.tree.selection():
            start, end = self.ranges[item]
            if start != end:  # Skip zero-width tokens (NEWLINE, INDENT, ...).
                text.tag_add(TAG, start, end)
                if first is None:
                    first = start
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

    def extend_selection(self, direction):
        "Extend the selection to the previous or next row (Shift+Up/Down)."
        tree = self.tree
        item = tree.next(tree.focus()) if direction > 0 else tree.prev(tree.focus())
        if item:
            tree.selection_add(item)
            tree.focus(item)
            tree.see(item)
        return "break"

    def goto_token(self, event=None):
        "Move the cursor to the double-clicked token and hide the browser."
        self.move_cursor(self.tree.identify_row(event.y))
        self.hide()
        return "break"      # Suppress the default double-click handling.

    def move_cursor(self, item=None):
        "Move the editor cursor to a token (the first selected row by default)."
        if item is None:
            selection = self.tree.selection()
            item = selection[0] if selection else None
        if not item:
            return
        start, end = self.ranges[item]
        self.text.mark_set("insert", start)
        self.text.see(start)

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


def _token_browser(parent):  # htest #
    "Set up a sample editor Text and open a token browser on it."
    from tkinter import Text
    top = Toplevel(parent)
    top.title("Sample editor")
    text = Text(top, width=40, height=8)
    text.insert("1.0", "import sys\n\ndef f(x):\n    return x + 1  # add one\n")
    text.pack(fill=BOTH, expand=True)
    return TokenBrowserWindow(top, text, _htest=True)


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_tokenbrowser', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(_token_browser)
