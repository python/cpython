import os
import fnmatch
import sys
from Tkinter import *
from idlelib import SearchEngine
from idlelib.SearchDialogBase import SearchDialogBase

def grep(text, io=None, flist=None):
    root = text._root()
    engine = SearchEngine.get(root)
    if not hasattr(engine, "_grepdialog"):
        engine._grepdialog = GrepDialog(root, engine, flist)
    dialog = engine._grepdialog
    searchphrase = text.get("sel.first", "sel.last")
    dialog.open(text, searchphrase, io)

class GrepDialog(SearchDialogBase):

    title = "Find in Files Dialog"
    icon = "Grep"
    needwrapbutton = 0

    def __init__(self, root, engine, flist):
        SearchDialogBase.__init__(self, root, engine)
        self.flist = flist
        self.globvar = StringVar(root)
        self.recvar = BooleanVar(root)

    def open(self, text, searchphrase, io=None):
        SearchDialogBase.open(self, text, searchphrase)
        if io:
            path = io.filename or ""
        else:
            path = ""
        dir, base = os.path.split(path)
        head, tail = os.path.splitext(base)
        if not tail:
            tail = ".py"
        self.globvar.set(os.path.join(dir, "*" + tail))

    def create_entries(self):
        SearchDialogBase.create_entries(self)
        self.globent = self.make_entry("In files:", self.globvar)

    def create_other_buttons(self):
        f = self.make_frame()

        btn = Checkbutton(f, anchor="w",
                variable=self.recvar,
                text="Recurse down subdirectories")
        btn.pack(side="top", fill="both")
        btn.select()

    def create_command_buttons(self):
        SearchDialogBase.create_command_buttons(self)
        self.make_button("Search Files", self.default_command, 1)

    def default_command(self, event=None):
        prog = self.engine.getprog()
        if not prog:
            return
        path = self.globvar.get()
        if not path:
            self.top.bell()
            return
        from idlelib.OutputWindow import OutputWindow
        save = sys.stdout
        try:
            sys.stdout = OutputWindow(self.flist)
            self.grep_it(prog, path)
        finally:
            sys.stdout = save

    def grep_it(self, prog, path):
        dir, base = os.path.split(path)
        list = self.findfiles(dir, base, self.recvar.get())
        list.sort()
        self.close()
        pat = self.engine.getpat()
        print("Searching %r in %s ..." % (pat, path))
        hits = 0
        for fn in list:
            try:
                f = open(fn)
            except IOError as msg:
                print(msg)
                continue
            lineno = 0
            while 1:
                block = f.readlines(100000)
                if not block:
                    break
                for line in block:
                    lineno = lineno + 1
                    if line[-1:] == '\n':
                        line = line[:-1]
                    if prog.search(line):
                        sys.stdout.write("%s: %s: %s\n" % (fn, lineno, line))
                        hits = hits + 1
        if hits:
            if hits == 1:
                s = ""
            else:
                s = "s"
            print("Found", hits, "hit%s." % s)
            print("(Hint: right-click to open locations.)")
        else:
            print("No hits.")

    def findfiles(self, dir, base, rec):
        try:
            names = os.listdir(dir or os.curdir)
        except os.error as msg:
            print(msg)
            return []
        list = []
        subdirs = []
        for name in names:
            fn = os.path.join(dir, name)
            if os.path.isdir(fn):
                subdirs.append(fn)
            else:
                if fnmatch.fnmatch(name, base):
                    list.append(fn)
        if rec:
            for subdir in subdirs:
                list.extend(self.findfiles(subdir, base, rec))
        return list

    def close(self, event=None):
        if self.top:
            self.top.grab_release()
            self.top.withdraw()
