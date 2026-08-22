"""A simple Unicode character browser for IDLE.

Similar to kcharselect and gnome-characters: browse Unicode blocks in a
grid, view a character's details, search by name or code point, and
insert a character or copy it to the clipboard.

The Edit menu's "Character Browser" command (see open() below) opens a
CharSelectWindow.
"""
import re
import unicodedata
from html.entities import codepoint2name

from tkinter import Toplevel, StringVar, Canvas, Text, Label, Entry, TclError
from tkinter import TOP, BOTTOM, LEFT, RIGHT, X, Y, BOTH, END, NSEW, \
    VERTICAL, SUNKEN
from tkinter import ttk
from tkinter.font import Font

# Unicode blocks: (name, start, end).
BLOCKS = [
    ("Basic Latin", 0x0020, 0x007F),
    ("Latin-1 Supplement", 0x00A0, 0x00FF),
    ("Latin Extended-A", 0x0100, 0x017F),
    ("Latin Extended-B", 0x0180, 0x024F),
    ("IPA Extensions", 0x0250, 0x02AF),
    ("Greek and Coptic", 0x0370, 0x03FF),
    ("Cyrillic", 0x0400, 0x04FF),
    ("Hebrew", 0x0590, 0x05FF),
    ("Arabic", 0x0600, 0x06FF),
    ("Devanagari", 0x0900, 0x097F),
    ("Thai", 0x0E00, 0x0E7F),
    ("General Punctuation", 0x2000, 0x206F),
    ("Superscripts and Subscripts", 0x2070, 0x209F),
    ("Currency Symbols", 0x20A0, 0x20CF),
    ("Letterlike Symbols", 0x2100, 0x214F),
    ("Number Forms", 0x2150, 0x218F),
    ("Arrows", 0x2190, 0x21FF),
    ("Mathematical Operators", 0x2200, 0x22FF),
    ("Box Drawing", 0x2500, 0x257F),
    ("Block Elements", 0x2580, 0x259F),
    ("Geometric Shapes", 0x25A0, 0x25FF),
    ("Miscellaneous Symbols", 0x2600, 0x26FF),
    ("Dingbats", 0x2700, 0x27BF),
    ("Braille Patterns", 0x2800, 0x28FF),
    ("Enclosed Alphanumerics", 0x2460, 0x24FF),
    ("CJK Symbols and Punctuation", 0x3000, 0x303F),
    ("Hiragana", 0x3040, 0x309F),
    ("Katakana", 0x30A0, 0x30FF),
    ("Emoticons", 0x1F600, 0x1F64F),
    ("Miscellaneous Symbols and Pictographs", 0x1F300, 0x1F5FF),
    ("Transport and Map Symbols", 0x1F680, 0x1F6FF),
    ("Supplemental Symbols and Pictographs", 0x1F900, 0x1F9FF),
]

COLS = 16  # Characters per row in the grid.

# Grid cell background colors: normal, hovered, and selected.
CELL_BG, HOVER_BG, SELECT_BG = "white", "#dde8ff", "#a8c7ff"

# Foreground for click-to-copy values in the detail tabs.
LINK_FG = "#0645ad"

# Tk counts two clicks within its NEARBY_MS (500) as a double click.  Defer a
# single-click copy this long so a double-click (which inserts) can cancel it.
DOUBLE_CLICK_MS = 500

# Categories with nothing worth showing in the grid.
SKIP_CATEGORIES = {"Cc", "Cs", "Cn", "Co"}

# Codepoint ranges scanned by a name search (the assigned parts of the
# blocks above, plus the common pictograph ranges).
SEARCH_RANGES = [(0x20, 0x2FFF), (0x3000, 0x33FF),
                 (0x1F300, 0x1FAFF), (0x2460, 0x27BF)]
SEARCH_LIMIT = 600  # Stop a name search after this many matches.


# Descriptions of the Unicode general categories, for the Unicode tab.
CATEGORY_NAMES = {
    "Lu": "uppercase letter", "Ll": "lowercase letter",
    "Lt": "titlecase letter", "Lm": "modifier letter", "Lo": "other letter",
    "Mn": "nonspacing mark", "Mc": "spacing mark", "Me": "enclosing mark",
    "Nd": "decimal number", "Nl": "letter number", "No": "other number",
    "Pc": "connector punctuation", "Pd": "dash punctuation",
    "Ps": "open punctuation", "Pe": "close punctuation",
    "Pi": "initial punctuation", "Pf": "final punctuation",
    "Po": "other punctuation", "Sm": "math symbol", "Sc": "currency symbol",
    "Sk": "modifier symbol", "So": "other symbol", "Zs": "space separator",
    "Zl": "line separator", "Zp": "paragraph separator", "Cc": "control",
    "Cf": "format", "Cs": "surrogate", "Co": "private use", "Cn": "unassigned",
}

# The five entities XML predefines by name; anything else needs a numeric ref.
XML_ENTITIES = {0x26: "amp", 0x3C: "lt", 0x3E: "gt", 0x22: "quot", 0x27: "apos"}


# Codepoints with a short Python string escape (\n, \t, ...).
SIMPLE_ESCAPES = {
    0x00: "\\0", 0x07: "\\a", 0x08: "\\b", 0x09: "\\t",
    0x0A: "\\n", 0x0B: "\\v", 0x0C: "\\f", 0x0D: "\\r",
}
ESCAPE_CODEPOINTS = {escape: cp for cp, escape in SIMPLE_ESCAPES.items()}


def char_escape(cp):
    "Return a Python string escape for codepoint cp: \\n, \\xHH, \\uHHHH or \\U..."
    if cp in SIMPLE_ESCAPES:
        return SIMPLE_ESCAPES[cp]
    if cp <= 0xFF:
        return f"\\x{cp:02x}"
    elif cp <= 0xFFFF:
        return f"\\u{cp:04x}"
    else:
        return f"\\U{cp:08x}"


def char_name(ch):
    "Return the Unicode name of ch, or '' if it has none."
    try:
        return unicodedata.name(ch)
    except ValueError:
        return ""


def surrogate_pair(cp):
    "Return the UTF-16 surrogate pair escape for a non-BMP cp, else None."
    if cp <= 0xFFFF:
        return None
    v = cp - 0x10000
    return f"\\u{0xD800 + (v >> 10):04x}\\u{0xDC00 + (v & 0x3FF):04x}"


def literal(s):
    "Show a string literally, escaping any unprintable characters."
    return "".join(c if c.isprintable() else char_escape(ord(c)) for c in s)


def codepoints(s):
    "Format a string as a space-separated sequence of U+XXXX codepoints."
    return " ".join(f"U+{ord(c):04X}" for c in s)


def decode_escapes(query):
    "Decode a run of \\n, \\xHH, \\uHHHH and \\UHHHHHHHH escapes to codepoints."
    if re.fullmatch(r"(?:\\x[0-9A-Fa-f]{2})+", query):
        # A pure run of \xHH bytes is decoded together as UTF-8 (else Latin-1,
        # one character per byte), so that e.g. \xf0\x9f\xa6\x86 is one emoji.
        data = bytes(int(h, 16)
                     for h in re.findall(r"\\x([0-9A-Fa-f]{2})", query))
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError:
            text = data.decode("latin-1")
        return [ord(c) for c in text]
    # Otherwise each escape is one code unit; merge any surrogate pairs.
    units = []
    for esc in re.findall(r"\\U[0-9A-Fa-f]{8}|\\u[0-9A-Fa-f]{4}"
                          r"|\\x[0-9A-Fa-f]{2}|\\[0abtnvfr]", query):
        units.append(ESCAPE_CODEPOINTS[esc] if esc in ESCAPE_CODEPOINTS
                     else int(esc[2:], 16))
    text = "".join(map(chr, units))
    try:
        text = text.encode("utf-16-le", "surrogatepass").decode("utf-16-le")
    except UnicodeDecodeError:
        pass  # A lone surrogate; leave the code units as they are.
    return [ord(c) for c in text]


def parse_input(query):
    """Interpret query as an unambiguous character specification.

    Return a list of codepoints if query is a single character or a marked
    codepoint notation (\\N{...}, a \\x/\\u/\\U escape run, U+XXXX, 0xXXXX,
    or an &#...; reference).  Return None for a plain word or a bare number,
    which the caller resolves by hex/decimal value and by name search.
    """
    if len(query) == 1:                             # A character stands for itself.
        return [ord(query)]
    m = re.fullmatch(r"\\N\{([^}]+)\}", query)      # \N{NAME}
    if m:
        try:
            return [ord(unicodedata.lookup(m.group(1).upper()))]
        except KeyError:
            return []
    if re.fullmatch(r"(?:\\x[0-9A-Fa-f]{2}|\\u[0-9A-Fa-f]{4}"
                    r"|\\U[0-9A-Fa-f]{8}|\\[0abtnvfr])+", query):
        return decode_escapes(query)                # \n, \x, \u, \U escapes
    m = re.fullmatch(r"(?:[Uu]\+|0[xX]|&#[xX])([0-9A-Fa-f]+);?", query)
    if m:
        return [int(m.group(1), 16)]                # U+XXXX, 0xXXXX, &#xXXXX;
    m = re.fullmatch(r"&#([0-9]+);?", query)
    if m:
        return [int(m.group(1))]                    # &#NNNN;
    return None


def open(editwin):
    "Open a character browser, seeded with the editor's selection."
    text = editwin.text
    try:
        first, last = text.index("sel.first"), text.index("sel.last")
    except TclError:  # A plain Text raises with no selection.
        first = last = ""
    selection = text.get(first, last) if first and last else ""
    CharSelectWindow(editwin.top, text, selection)


class CharSelectWindow(Toplevel):
    "Browse and search Unicode characters, and copy them to the clipboard."

    last_block = 0  # Block index to reopen on; kept across browser windows.

    def __init__(self, parent, text=None, search="", *,
                 _htest=False, _utest=False):
        """Create the character browser as a child of parent.

        text - the editor Text to insert into, or None to disable Insert.
        search - initial search query (e.g. the editor selection).
        _htest - bool; change box location when running htest.
        _utest - bool; don't wait for user interaction when unit testing.
        """
        super().__init__(parent)
        self.editor_text = text
        self.title("Character Browser")
        self.protocol("WM_DELETE_WINDOW", self.close)
        self.bind("<Escape>", self.close)
        # Scroll the grid with the mouse wheel over this window only
        # (bind_all would steal the wheel from the rest of IDLE).
        self.bind("<MouseWheel>", self.wheel)
        self.bind("<Button-4>", self.wheel)
        self.bind("<Button-5>", self.wheel)
        self.bind("<<Copy>>", self.copy_event)  # Ctrl+C copies the character.

        self.cell_font = Font(self, family="DejaVu Sans", size=18)
        self.big_font = Font(self, family="DejaVu Sans", size=72)
        self.current_block = BLOCKS[0]
        self.selected_cp = None
        self.selected_cell = None    # The highlighted grid cell, if any.
        self.cells = {}              # codepoint -> grid cell widget.
        self.cell_index = 0
        self.pending_copy = None     # after() id of a deferred single click.

        self.create_widgets()
        # Size the window from a full 16-column block before showing the
        # remembered block or a seeded search, whose few results would
        # otherwise size the grid too narrow.
        self.show_block(BLOCKS[0])
        self.set_geometry(_htest)
        if CharSelectWindow.last_block:
            self.block_combo.current(CharSelectWindow.last_block)
            self.show_block(BLOCKS[CharSelectWindow.last_block])
        if search.strip():
            self.search_var.set(search)
            self.search()
        if not _utest:
            self.deiconify()

    def set_geometry(self, _htest=False):
        "Size the window so that all 16 columns and the buttons are visible."
        self.update_idletasks()
        # Make the grid canvas as wide as the whole 16-column grid.  The paned
        # window under-requests, so compute the total width from its panes and
        # put the sash where the grid pane gets its full width; otherwise the
        # grid is squeezed and the rightmost columns are clipped.
        self.canvas.configure(width=self.grid_inner.winfo_reqwidth())
        self.update_idletasks()
        grid_pane, detail_pane = self.paned.panes()
        grid_width = self.nametowidget(grid_pane).winfo_reqwidth()
        detail_width = self.nametowidget(detail_pane).winfo_reqwidth()
        width = grid_width + detail_width + 16  # Allow for the sash.
        height = self.winfo_reqheight()
        self.minsize(width, height)
        # Place the window below and to the right of the parent.
        parent = self.master
        x = parent.winfo_rootx() + 20
        y = parent.winfo_rooty() + (100 if _htest else 20)
        self.geometry(f"{width}x{height}+{x}+{y}")
        self.update_idletasks()
        self.paned.sashpos(0, grid_width)

    def create_widgets(self):
        # Top bar: block selector and search box.
        top = ttk.Frame(self, padding=6)
        top.pack(side=TOP, fill=X)

        ttk.Label(top, text="Block:").pack(side=LEFT)
        self.block_combo = ttk.Combobox(
            top, state="readonly", width=34,
            values=[name for name, start, end in BLOCKS])
        self.block_combo.current(0)
        self.block_combo.pack(side=LEFT, padx=(4, 12))
        self.block_combo.bind("<<ComboboxSelected>>", self.block_selected)

        ttk.Label(top, text="Search:").pack(side=LEFT)
        self.search_var = StringVar(self)
        search_entry = ttk.Entry(top, textvariable=self.search_var, width=22)
        search_entry.pack(side=LEFT, padx=4)
        search_entry.bind("<Return>", self.search)
        ttk.Button(top, text="Go", command=self.search).pack(side=LEFT)
        ttk.Button(top, text="Clear",
                   command=self.clear_search).pack(side=LEFT, padx=4)

        # Main area: character grid on the left, detail panel on the right.
        paned = self.paned = ttk.PanedWindow(self, orient="horizontal")
        paned.pack(fill=BOTH, expand=True)

        grid_frame = ttk.Frame(paned)
        self.canvas = Canvas(grid_frame, highlightthickness=0, bg="white")
        vbar = ttk.Scrollbar(grid_frame, orient=VERTICAL,
                             command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=vbar.set)
        vbar.pack(side=RIGHT, fill=Y)
        self.canvas.pack(side=LEFT, fill=BOTH, expand=True)
        self.grid_inner = ttk.Frame(self.canvas)
        self.canvas.create_window((0, 0), window=self.grid_inner, anchor="nw")
        self.grid_inner.bind(
            "<Configure>",
            lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        paned.add(grid_frame, weight=0)  # Keep the grid fixed on resize;

        # Detail panel: the character above a notebook of detail tabs.
        detail = ttk.Frame(paned, padding=10)
        self.big_var = StringVar(self)
        ttk.Label(detail, textvariable=self.big_var, font=self.big_font,
                  anchor="center").pack(fill=X, pady=(0, 8))
        notebook = ttk.Notebook(detail)
        notebook.pack(fill=BOTH, expand=True)
        self.overview = self.make_info_tab(notebook, "Overview")
        self.reprs = self.make_info_tab(notebook, "Repr")
        self.unidata = self.make_info_tab(notebook, "Unicode")
        button_row = ttk.Frame(detail)
        button_row.pack(fill=X, pady=(6, 0))
        ttk.Button(button_row, text="Copy character",
                   command=self.copy_char).pack(side=LEFT)
        self.insert_button = ttk.Button(button_row, text="Insert",
                                        command=self.insert_char)
        self.insert_button.pack(side=LEFT, padx=(6, 0))
        if self.editor_text is None:  # Nothing to insert into.
            self.insert_button.state(["disabled"])
        paned.add(detail, weight=1)      # expand the detail panel instead.

        # Status bar.
        self.status = ttk.Label(self, text="", anchor="w", relief=SUNKEN,
                                padding=3)
        self.status.pack(side=BOTTOM, fill=X)

    # Grid population.

    def clear_grid(self):
        for child in self.grid_inner.winfo_children():
            child.destroy()
        self.cells = {}
        self.selected_cell = None
        self.cell_index = 0

    def add_cell(self, cp, force=False):
        "Add a grid cell for codepoint cp; return True if it was shown."
        ch = chr(cp)
        category = unicodedata.category(ch)
        if category == "Cs":
            return False        # A lone surrogate cannot be displayed.
        if not force and category in SKIP_CATEGORIES:
            return False        # force shows an explicitly requested codepoint.
        row, col = divmod(self.cell_index, COLS)
        cell = Label(self.grid_inner, text=ch, font=self.cell_font,
                     width=2, height=1, relief="ridge", borderwidth=1,
                     bg=CELL_BG)
        cell.grid(row=row, column=col, sticky=NSEW, padx=1, pady=1)
        cell.bind("<Button-1>", lambda e, p=cp: self.select(p))
        cell.bind("<Double-Button-1>", lambda e, p=cp: self.activate_cell(p))
        cell.bind("<Enter>", self.hover_cell)
        cell.bind("<Leave>", self.unhover_cell)
        self.cells[cp] = cell
        self.cell_index += 1
        return True

    def hover_cell(self, event):
        if event.widget is not self.selected_cell:
            event.widget.configure(bg=HOVER_BG)

    def unhover_cell(self, event):
        if event.widget is not self.selected_cell:
            event.widget.configure(bg=CELL_BG)

    def highlight_cell(self, cp):
        "Mark cp's cell as selected, deselecting any previous one."
        if self.selected_cell is not None:
            try:
                self.selected_cell.configure(bg=CELL_BG)
            except TclError:
                pass            # The old cell was destroyed by clear_grid.
        self.selected_cell = self.cells.get(cp)
        if self.selected_cell is not None:
            self.selected_cell.configure(bg=SELECT_BG)

    def restore_highlight(self):
        "Re-mark the selected character's cell after the grid is rebuilt."
        if self.selected_cp in self.cells:
            self.highlight_cell(self.selected_cp)

    def show_block(self, block):
        name, start, end = block
        self.current_block = block
        self.clear_grid()
        for cp in range(start, end + 1):
            self.add_cell(cp)
        self.canvas.yview_moveto(0)
        self.status.configure(
            text=f"{name}  (U+{start:04X}–U+{end:04X})"
                 f"  •  {self.cell_index} characters")
        self.restore_highlight()

    # Selection and detail.

    def make_info_tab(self, notebook, title):
        "Add a read-only text tab to notebook and return its Text widget."
        text = Text(notebook, width=30, height=12, wrap="word",
                    state="disabled", relief="flat", bg=self["bg"])
        text.tag_configure("copy", foreground=LINK_FG, underline=True)
        text.tag_bind("copy", "<Enter>",
                      lambda e: text.configure(cursor="hand2"))
        text.tag_bind("copy", "<Leave>",
                      lambda e: text.configure(cursor=""))
        notebook.add(text, text=title)
        return text

    def activate_cell(self, cp):
        "Select cp then insert it and close (the double-click action)."
        self.select(cp)
        self.insert_char()

    def select(self, cp):
        "Show codepoint cp in the big label, the detail tabs, and the grid."
        self.selected_cp = cp
        self.big_var.set(chr(cp))
        self.fill_info(self.overview, self.overview_pairs(cp))
        self.fill_info(self.reprs, self.repr_pairs(cp))
        self.fill_info(self.unidata, self.unidata_pairs(cp))
        self.highlight_cell(cp)

    def fill_info(self, text, pairs):
        """Show (label, value) or (label, value, copy) rows in the tab.

        A row with a copy string is drawn as a link that puts it on the
        clipboard when clicked.
        """
        width = max(len(label) for label, *_ in pairs) + 2
        text.configure(state="normal")
        text.delete("1.0", END)
        for i, (label, value, *rest) in enumerate(pairs):
            if i:
                text.insert(END, "\n")
            text.insert(END, f"{label + ':':<{width}}")
            if rest:  # A value: click to copy it, double-click to insert it.
                tag = f"copy-{i}"
                text.insert(END, str(value), ("copy", tag))
                text.tag_bind(tag, "<Button-1>",
                              lambda e, s=rest[0]: self.copy_later(s))
                text.tag_bind(tag, "<Double-Button-1>",
                              lambda e, s=rest[0]: self.insert_value(s))
            else:
                text.insert(END, str(value))
        text.configure(state="disabled")

    def overview_pairs(self, cp):
        "The identity of the character: name, codepoint, decimal value."
        ch = chr(cp)
        return [
            ("Name", char_name(ch) or "<unnamed>"),
            ("Codepoint", f"U+{cp:04X}"),
            ("Decimal", cp),
        ]

    def repr_pairs(self, cp):
        "Ways to write the character: literal, escapes, UTF-8, XML, HTML."
        ch = chr(cp)
        utf8 = "".join(f"\\x{b:02x}" for b in ch.encode("utf-8"))  # UTF-8 bytes
        # A named HTML entity if there is one, else a decimal reference.
        entity = codepoint2name.get(cp)
        html = f"&{entity};" if entity else f"&#{cp};"
        # XML predefines five entities by name; the rest use a decimal
        # reference, as str.encode(..., "xmlcharrefreplace") does.
        xml_entity = XML_ENTITIES.get(cp)
        xml = f"&{xml_entity};" if xml_entity else f"&#{cp};"
        pairs = [
            ("Character", ch),
            ("Escaped", char_escape(cp)),
            ("UTF-8", utf8),
        ]
        surrogates = surrogate_pair(cp)
        if surrogates:  # Only a non-BMP character has a surrogate pair.
            pairs.append(("Surrogates", surrogates))
        pairs += [
            ("XML", xml),
            ("HTML", html),
        ]
        name = char_name(ch)
        if name:  # Only a named character has a \N{...} escape.
            pairs.append(("Named", f"\\N{{{name}}}"))
        # Every representation is click-to-copy (copy the value itself).
        return [(label, value, value) for label, value in pairs]

    def unidata_pairs(self, cp):
        "Character properties from the Unicode database."
        ch = chr(cp)
        category = unicodedata.category(ch)
        cat_name = CATEGORY_NAMES.get(category, "")
        block = next((bname for bname, start, end in BLOCKS
                      if start <= cp <= end), "—")
        pairs = [
            ("Name", char_name(ch) or "<unnamed>"),
            ("Block", block),
            ("Category", f"{category} ({cat_name})" if cat_name else category),
            ("Bidirectional", unicodedata.bidirectional(ch) or "—"),
            ("Combining", unicodedata.combining(ch)),
            ("East Asian", unicodedata.east_asian_width(ch)),
            ("Mirrored", "yes" if unicodedata.mirrored(ch) else "no"),
        ]
        decomposition = unicodedata.decomposition(ch)
        if decomposition:
            pairs.append(("Decomposition", decomposition))
        for label, func in (("Decimal", unicodedata.decimal),
                            ("Digit", unicodedata.digit),
                            ("Numeric", unicodedata.numeric)):
            value = func(ch, None)
            if value is not None:
                pairs.append((label, value))
        # Normalization forms, shown only when they change the character.
        for form in ("NFC", "NFD", "NFKC", "NFKD"):
            normalized = unicodedata.normalize(form, ch)
            if normalized != ch:
                # Click-to-copy the literal string, without the U+ codepoints.
                lit = literal(normalized)
                pairs.append((form, f"{lit}  ({codepoints(normalized)})", lit))
        return pairs

    # Search.

    def search(self, event=None):
        "Show the specified character(s) and/or characters whose name matches."
        query = self.search_var.get().strip()
        if not query:
            return
        cps = parse_input(query)
        if cps is not None:
            # A single character or a marked codepoint notation names exactly
            # these characters (shown even if normally filtered out); a name
            # search would only add noise.
            valid = [cp for cp in cps if 0 <= cp <= 0x10FFFF]
            self.show_results(valid, query, forced=set(valid))
            return
        # A bare token may name a codepoint as hexadecimal (and, if all
        # digits, also as decimal), and may still occur in character names.
        forced = []
        if re.fullmatch("[0-9A-Fa-f]+", query) and int(query, 16) <= 0x10FFFF:
            forced.append(int(query, 16))
        if query.isdigit() and int(query) <= 0x10FFFF and int(query) not in forced:
            forced.append(int(query))
        results = forced + self.name_matches(query)
        self.show_results(results, query, forced=set(forced))

    def name_matches(self, query):
        "Return codepoints whose name contains query, or [] if it cannot occur."
        upper = query.upper()
        if not re.fullmatch("[A-Z0-9 -]+", upper):
            return []       # A name has only letters, digits, spaces, hyphens.
        results = []
        for start, end in SEARCH_RANGES:
            for cp in range(start, end + 1):
                if upper in char_name(chr(cp)):
                    results.append(cp)
            if len(results) > SEARCH_LIMIT:
                break
        return results

    def show_results(self, cps, query, forced=frozenset()):
        "Show cps in the grid; those in forced bypass the category filter."
        self.clear_grid()
        shown = []
        seen = set()
        for cp in cps:
            if cp not in seen:
                seen.add(cp)
                if self.add_cell(cp, force=cp in forced):
                    shown.append(cp)
        self.canvas.yview_moveto(0)
        self.status.configure(text=f"Search '{query}'"
                                   f"  •  {self.cell_index} results")
        # Select the sole result, or the first one when no character is shown
        # yet; otherwise keep the current detail and re-mark its cell.
        if len(shown) == 1 or (shown and self.selected_cp is None):
            self.select(shown[0])
        else:
            self.restore_highlight()

    def clear_search(self):
        self.search_var.set("")
        self.show_block(self.current_block)

    # Other handlers.

    def block_selected(self, event=None):
        CharSelectWindow.last_block = self.block_combo.current()
        self.show_block(BLOCKS[CharSelectWindow.last_block])

    def wheel(self, event):
        if event.num == 5 or event.delta < 0:
            self.canvas.yview_scroll(1, "units")
        elif event.num == 4 or event.delta > 0:
            self.canvas.yview_scroll(-1, "units")

    def copy_event(self, event=None):
        "Copy the character, unless a text widget has its own selection to copy."
        focus = self.focus_get()
        if isinstance(focus, Entry) and focus.selection_present():
            return              # Let the search box copy its selected text.
        if isinstance(focus, Text) and focus.tag_ranges("sel"):
            return              # Let a detail pane copy its selected text.
        self.copy_char()
        return "break"

    def copy_later(self, s):
        "Copy s after a delay, so a following double-click can cancel it."
        self.cancel_copy()
        self.pending_copy = self.after(DOUBLE_CLICK_MS, self.copy_text, s)

    def cancel_copy(self):
        if self.pending_copy is not None:
            self.after_cancel(self.pending_copy)
            self.pending_copy = None

    def insert_value(self, s):
        "Insert a detail value, cancelling the pending single-click copy."
        self.cancel_copy()
        self.insert_text(s)

    def copy_text(self, s):
        "Put s on the clipboard and report it in the status bar."
        self.pending_copy = None
        self.clipboard_clear()
        self.clipboard_append(s)
        self.status.configure(text=f"Copied {s!r} to the clipboard")

    def copy_char(self):
        if self.selected_cp is not None:
            self.copy_text(chr(self.selected_cp))

    def insert_text(self, s):
        "Insert string s into the editor, replacing any selection, then close."
        if self.editor_text is None:
            self.bell()              # Nowhere to insert.
            return
        text = self.editor_text
        try:
            first, last = text.index("sel.first"), text.index("sel.last")
        except TclError:
            first = last = ""
        # In the Shell, keep the edit within the input area after "iomark",
        # as PyShell does for typing, cut, and paste.
        try:
            if text.compare("insert", "<", "iomark"):
                text.mark_set("insert", "iomark")
            if first and text.compare(first, "<", "iomark"):
                first = last = ""    # Don't overwrite output before the prompt.
        except TclError:
            pass                     # Not the Shell: there is no "iomark".
        if first and last:           # Replace the selection.
            text.delete(first, last)
            text.insert(first, s)
        else:
            text.insert("insert", s)
        text.focus_set()
        self.close()

    def insert_char(self):
        "Insert the selected character (Insert button and grid double-click)."
        if self.selected_cp is None:
            self.bell()              # Nothing selected.
            return
        self.insert_text(chr(self.selected_cp))

    def close(self, event=None):
        self.cancel_copy()  # Don't let a deferred copy fire after destroy.
        self.destroy()


if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_charselect', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(CharSelectWindow)
