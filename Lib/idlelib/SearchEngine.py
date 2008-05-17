import re
from tkinter import *
import tkinter.messagebox as tkMessageBox

def get(root):
    if not hasattr(root, "_searchengine"):
        root._searchengine = SearchEngine(root)
        # XXX This will never garbage-collect -- who cares
    return root._searchengine

class SearchEngine:

    def __init__(self, root):
        self.root = root
        # State shared by search, replace, and grep;
        # the search dialogs bind these to UI elements.
        self.patvar = StringVar(root)           # search pattern
        self.revar = BooleanVar(root)           # regular expression?
        self.casevar = BooleanVar(root)         # match case?
        self.wordvar = BooleanVar(root)         # match whole word?
        self.wrapvar = BooleanVar(root)         # wrap around buffer?
        self.wrapvar.set(1)                     # (on by default)
        self.backvar = BooleanVar(root)         # search backwards?

    # Access methods

    def getpat(self):
        return self.patvar.get()

    def setpat(self, pat):
        self.patvar.set(pat)

    def isre(self):
        return self.revar.get()

    def iscase(self):
        return self.casevar.get()

    def isword(self):
        return self.wordvar.get()

    def iswrap(self):
        return self.wrapvar.get()

    def isback(self):
        return self.backvar.get()

    # Higher level access methods

    def getcookedpat(self):
        pat = self.getpat()
        if not self.isre():
            pat = re.escape(pat)
        if self.isword():
            pat = r"\b%s\b" % pat
        return pat

    def getprog(self):
        pat = self.getpat()
        if not pat:
            self.report_error(pat, "Empty regular expression")
            return None
        pat = self.getcookedpat()
        flags = 0
        if not self.iscase():
            flags = flags | re.IGNORECASE
        try:
            prog = re.compile(pat, flags)
        except re.error as what:
            try:
                msg, col = what
            except:
                msg = str(what)
                col = -1
            self.report_error(pat, msg, col)
            return None
        return prog

    def report_error(self, pat, msg, col=-1):
        # Derived class could overrid this with something fancier
        msg = "Error: " + str(msg)
        if pat:
            msg = msg + "\np\Pattern: " + str(pat)
        if col >= 0:
            msg = msg + "\nOffset: " + str(col)
        tkMessageBox.showerror("Regular expression error",
                               msg, master=self.root)

    def setcookedpat(self, pat):
        if self.isre():
            pat = re.escape(pat)
        self.setpat(pat)

    def search_text(self, text, prog=None, ok=0):
        """Search a text widget for the pattern.

        If prog is given, it should be the precompiled pattern.
        Return a tuple (lineno, matchobj); None if not found.

        This obeys the wrap and direction (back) settings.

        The search starts at the selection (if there is one) or
        at the insert mark (otherwise).  If the search is forward,
        it starts at the right of the selection; for a backward
        search, it starts at the left end.  An empty match exactly
        at either end of the selection (or at the insert mark if
        there is no selection) is ignored  unless the ok flag is true
        -- this is done to guarantee progress.

        If the search is allowed to wrap around, it will return the
        original selection if (and only if) it is the only match.

        """
        if not prog:
            prog = self.getprog()
            if not prog:
                return None # Compilation failed -- stop
        wrap = self.wrapvar.get()
        first, last = get_selection(text)
        if self.isback():
            if ok:
                start = last
            else:
                start = first
            line, col = get_line_col(start)
            res = self.search_backward(text, prog, line, col, wrap, ok)
        else:
            if ok:
                start = first
            else:
                start = last
            line, col = get_line_col(start)
            res = self.search_forward(text, prog, line, col, wrap, ok)
        return res

    def search_forward(self, text, prog, line, col, wrap, ok=0):
        wrapped = 0
        startline = line
        chars = text.get("%d.0" % line, "%d.0" % (line+1))
        while chars:
            m = prog.search(chars[:-1], col)
            if m:
                if ok or m.end() > col:
                    return line, m
            line = line + 1
            if wrapped and line > startline:
                break
            col = 0
            ok = 1
            chars = text.get("%d.0" % line, "%d.0" % (line+1))
            if not chars and wrap:
                wrapped = 1
                wrap = 0
                line = 1
                chars = text.get("1.0", "2.0")
        return None

    def search_backward(self, text, prog, line, col, wrap, ok=0):
        wrapped = 0
        startline = line
        chars = text.get("%d.0" % line, "%d.0" % (line+1))
        while 1:
            m = search_reverse(prog, chars[:-1], col)
            if m:
                if ok or m.start() < col:
                    return line, m
            line = line - 1
            if wrapped and line < startline:
                break
            ok = 1
            if line <= 0:
                if not wrap:
                    break
                wrapped = 1
                wrap = 0
                pos = text.index("end-1c")
                line, col = map(int, pos.split("."))
            chars = text.get("%d.0" % line, "%d.0" % (line+1))
            col = len(chars) - 1
        return None

# Helper to search backwards in a string.
# (Optimized for the case where the pattern isn't found.)

def search_reverse(prog, chars, col):
    m = prog.search(chars)
    if not m:
        return None
    found = None
    i, j = m.span()
    while i < col and j <= col:
        found = m
        if i == j:
            j = j+1
        m = prog.search(chars, j)
        if not m:
            break
        i, j = m.span()
    return found

# Helper to get selection end points, defaulting to insert mark.
# Return a tuple of indices ("line.col" strings).

def get_selection(text):
    try:
        first = text.index("sel.first")
        last = text.index("sel.last")
    except TclError:
        first = last = None
    if not first:
        first = text.index("insert")
    if not last:
        last = first
    return first, last

# Helper to parse a text index into a (line, col) tuple.

def get_line_col(index):
    line, col = map(int, index.split(".")) # Fails on invalid index
    return line, col
