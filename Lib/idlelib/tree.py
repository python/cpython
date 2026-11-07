"""A tree widget for IDLE, based on ttk.Treeview.

TreeWidget shows a tree of TreeItems (see below).  An item is asked for
its children only when its row is opened, so that a large tree costs
nothing until it is expanded.

A Treeview sizes its rows after the font, gets its expand/collapse
indicators and colors from the theme, and navigates with the keyboard.
"""
from tkinter import ttk
from tkinter import font

from idlelib.config import idleConf

# The ttk style of the tree, colored after IDLE's current configuration.
STYLE = "IDLE.Treeview"


class TreeItem:

    """Abstract class representing tree items.

    Methods should typically be overridden, otherwise a default action
    is used.

    """

    def __init__(self):
        """Constructor.  Do whatever you need to do."""

    def GetText(self):
        """Return text string to display."""

    def GetLabelText(self):
        """Return label text string to display in front of text (if any)."""

    expandable = None

    def _IsExpandable(self):
        """Do not override!  Called by TreeWidget."""
        if self.expandable is None:
            self.expandable = self.IsExpandable()
        return self.expandable

    def IsExpandable(self):
        """Return whether there are subitems."""
        return 1

    def _GetSubList(self):
        """Do not override!  Called by TreeWidget."""
        if not self.IsExpandable():
            return []
        sublist = self.GetSubList()
        if not sublist:
            self.expandable = 0
        return sublist

    def IsEditable(self):
        """Return whether the item's text may be edited."""

    def SetText(self, text):
        """Change the item's text (if it is editable)."""

    def GetSubList(self):
        """Return list of items forming sublist."""

    def GetTags(self):
        """Return the tags of the item's row, for its colors or its icon."""

    def GetValues(self):
        """Return the texts of the columns after the tree column.

        The default is the text of the item in the last of them, which
        is what a tree of names and values wants.
        """

    def OnDoubleClick(self):
        """Called on a double-click on the item."""


class ScrolledTreeview(ttk.Frame):

    """A ttk.Treeview with scrollbars, colored and sized after IDLE's config.

    The columns are left to ttk, which shares the width of the tree
    between them and lets the user drag the edge of a heading.

    Attributes:
        tree: The ttk.Treeview itself.
    """

    def __init__(self, master, *, columns=(), show="tree", headings=(), **kw):
        """Create the widget.

        master: The parent widget.
        columns: The names of the columns after the tree column.
        show: What the tree shows, as the ttk.Treeview option.
        headings: A heading for each column, the tree column first.
            Their edges can be dragged to widen a column.
        Other keyword arguments are passed to ttk.Frame.
        """
        super().__init__(master, **kw)
        self.tree_column = "tree" in show
        if headings:
            show = (show + " headings").strip()
        self.tree = ttk.Treeview(self, style=STYLE, show=show,
                                 selectmode="browse", columns=columns)
        for column, heading in enumerate(headings):
            self.tree.heading("#%d" % column, text=heading, anchor="w")
        vbar = ttk.Scrollbar(self, name="vbar", orient="vertical",
                             command=self.tree.yview)
        hbar = ttk.Scrollbar(self, name="hbar", orient="horizontal",
                             command=self.tree.xview)
        self.tree['yscrollcommand'] = vbar.set
        self.tree['xscrollcommand'] = hbar.set
        self.tree.grid(row=0, column=0, sticky="nsew")
        vbar.grid(row=0, column=1, sticky="ns")
        hbar.grid(row=1, column=0, sticky="ew")
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)
        self.configure_style()

    def focus_set(self):
        "Focus the tree itself, so that the keys work at once."
        self.tree.focus_set()

    def add_row(self, parent="", text="", values=(), **kw):
        "Add a row at the end of the parent row and return its id."
        return self.tree.insert(parent, "end", text=text, values=values, **kw)

    def clear(self):
        "Remove all rows."
        self.tree.delete(*self.tree.get_children())

    def configure_style(self):
        """Take the colors and the font of the tree from the configuration.

        The row height goes with the font, as the ttk default of 20
        pixels clips all but the smallest text.
        """
        theme = idleConf.CurrentTheme()
        normal = idleConf.GetHighlight(theme, 'normal')
        hilite = idleConf.GetHighlight(theme, 'hilite')
        text_font = idleConf.GetFont(self, 'main', 'EditorWindow')
        self.font = font.Font(root=self, font=text_font)
        style = ttk.Style(self)
        style.configure(STYLE, font=text_font,
                        rowheight=self.font.metrics("linespace") + 2,
                        fieldbackground=normal['background'], **normal)
        style.map(STYLE,
                  background=[('selected', hilite['background'])],
                  foreground=[('selected', hilite['foreground'])])


class TreeWidget(ScrolledTreeview):

    """A scrolled tree of TreeItems.

    Create the widget, give it the item to show as the root of the tree,
    and pack or grid it like any other widget:

        tree = TreeWidget(top, rootitem)
        tree.pack(expand=True, fill="both")

    An item is asked for its children only when its row is opened.
    Its label text goes in the tree column and its text in the last
    column, or after the label where there is only the tree column.
    See GetValues for filling more columns than one.

    Attributes:
        tree: The ttk.Treeview showing the items.
        items: Map of the row ids of the tree to their TreeItems.
        root: The row id of the root item, or '' if there is none.
    """

    def __init__(self, master, item=None, **kw):
        """Create the widget and, if item is given, show it as the root.

        master: The parent widget.
        item: The TreeItem to show as the root of the tree.
        Other keyword arguments are passed to ScrolledTreeview.
        """
        super().__init__(master, **kw)
        self.items = {}
        self.root = ''
        self.tree.bind("<<TreeviewOpen>>", self.opened)
        # A double click also opens or closes the row, as it did before.
        self.tree.bind("<Double-Button-1>", self.double_clicked, add="+")
        self.tree.bind("<Return>", self.activated)
        if item is not None:
            self.set_root(item)

    def set_root(self, item):
        "Show item as the root of the tree, replacing what the tree shows."
        self.clear()
        self.items.clear()
        self.root = self.add_item('', item)
        return self.root

    def add_item(self, parent, item):
        "Add a row for item as a child of the parent row and return its id."
        columns = self.tree['columns']
        text = item.GetLabelText()
        value = item.GetText() or ''
        if not text:
            text, value = value, ''
        elif not columns:
            text, value = f'{text} {value}', ''  # No column for it.
        values = item.GetValues() if columns else ()
        if values is None:
            values = [''] * (len(columns) - 1) + [value]
        values = (list(values) + [''] * len(columns))[:len(columns)]
        iid = self.add_row(parent, text=text, values=values,
                           tags=item.GetTags() or ())
        self.items[iid] = item
        if item._IsExpandable():
            # A placeholder gives the row its indicator; opening the row
            # replaces it with the children.
            self.tree.insert(iid, "end")
        return iid

    def expand(self, iid=None):
        "Open the given row, the root row by default, and fill it in."
        if iid is None:
            iid = self.root
        if iid:
            self.fill(iid)
            self.tree.item(iid, open=True)

    def fill(self, iid):
        "Replace the placeholder child of a row with rows for the children."
        children = self.tree.get_children(iid)
        if not children or children[0] in self.items:
            return  # A leaf, or already filled in.
        self.tree.delete(*children)
        for item in self.items[iid]._GetSubList():
            self.add_item(iid, item)

    def opened(self, event=None):
        "Fill in the row that the user has just opened."
        self.fill(self.tree.focus())

    def activated(self, event=None):
        "Call OnDoubleClick for the current row."
        item = self.items.get(self.tree.focus())
        if item is not None:
            item.OnDoubleClick()

    def double_clicked(self, event):
        "Call OnDoubleClick for the double-clicked row."
        item = self.items.get(self.tree.identify_row(event.y))
        if item is not None:
            item.OnDoubleClick()


def _tree_widget(parent):  # htest #
    from tkinter import Toplevel
    from idlelib.debugobj import make_objecttreeitem
    import sys

    top = Toplevel(parent)
    top.title("Test TreeWidget")
    x, y = map(int, parent.geometry().split('+')[1:])
    top.geometry("+%d+%d" % (x + 50, y + 175))
    tree = TreeWidget(top, make_objecttreeitem("sys", sys),
                      columns=("value",), headings=("Name", "Value"))
    tree.pack(expand=True, fill="both")
    tree.expand()


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_tree', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(_tree_widget)
