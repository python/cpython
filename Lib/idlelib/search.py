from tkinter import TclError

from idlelib import searchengine
from idlelib.searchbase import SearchDialogBase

def _setup(text):
    "Create or find the singleton SearchDialog instance."
    root = text._root()
    engine = searchengine.get(root)
    if not hasattr(engine, "_searchdialog"):
        engine._searchdialog = SearchDialog(root, engine)
    return engine._searchdialog

def find(text):
    "Handle the editor edit menu item and corresponding event."
    pat = text.get("sel.first", "sel.last")
    return _setup(text).open(text, pat)  # Open is inherited from SDBase.

def find_again(text):
    "Handle the editor edit menu item and corresponding event."
    return _setup(text).find_again(text)

def find_selection(text):
    "Handle the editor edit menu item and corresponding event."
    return _setup(text).find_selection(text)


class SearchDialog(SearchDialogBase):

    def create_widgets(self):
        SearchDialogBase.create_widgets(self)
        self.make_button("Find Next", self.default_command, 1)

    def default_command(self, event=None):
        if not self.engine.getprog():
            return
        self.find_again(self.text)

    def find_again(self, text):
        if not self.engine.getpat():
            self.open(text)
            return False
        if not self.engine.getprog():
            return False
        res = self.engine.search_text(text)
        if res:
            line, m = res
            i, j = m.span()
            first = "%d.%d" % (line, i)
            last = "%d.%d" % (line, j)
            try:
                selfirst = text.index("sel.first")
                sellast = text.index("sel.last")
                if selfirst == first and sellast == last:
                    self.bell()
                    return False
            except TclError:
                pass
            text.tag_remove("sel", "1.0", "end")
            text.tag_add("sel", first, last)
            text.mark_set("insert", self.engine.isback() and first or last)
            text.see("insert")
            return True
        else:
            self.bell()
            return False

    def find_selection(self, text):
        pat = text.get("sel.first", "sel.last")
        if pat:
            self.engine.setcookedpat(pat)
        return self.find_again(text)


def _search_dialog(parent):  # htest #
    "Display search test box."
    from tkinter import Toplevel, Text
    from tkinter.ttk import Button

    box = Toplevel(parent)
    box.title("Test SearchDialog")
    x, y = map(int, parent.geometry().split('+')[1:])
    box.geometry("+%d+%d" % (x, y + 175))
    text = Text(box, inactiveselectbackground='gray')
    text.pack()
    text.insert("insert","This is a sample string.\n"*5)

    def show_find():
        text.tag_add('sel', '1.0', 'end')
        _setup(text).open(text)
        text.tag_remove('sel', '1.0', 'end')

    button = Button(box, text="Search (selection ignored)", command=show_find)
    button.pack()

if __name__ == '__main__':
    import unittest
    unittest.main('idlelib.idle_test.test_search',
                  verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(_search_dialog)
