import sys
import os
import string
import imp
from Tkinter import *
import tkSimpleDialog
import tkMessageBox

about_title = "About IDLE"
about_text = """\
IDLE 0.1

A not totally unintegrated development environment for Python

by Guido van Rossum
"""

class EditorWindow:

    from Percolator import Percolator
    from ColorDelegator import ColorDelegator
    from UndoDelegator import UndoDelegator
    from IOBinding import IOBinding
    from SearchBinding import SearchBinding
    from AutoIndent import AutoIndent
    from AutoExpand import AutoExpand
    import Bindings
    
    about_title = about_title
    about_text = about_text    

    def __init__(self, root, filename=None):
        self.root = root
        self.menubar = Menu(root)
        self.top = top = Toplevel(root, menu=self.menubar)
        self.vbar = vbar = Scrollbar(top, name='vbar')
        self.text = text = Text(top, name='text')

        self.createmenubar()
        self.Bindings.apply_bindings(text)

        self.top.protocol("WM_DELETE_WINDOW", self.close)
        self.top.bind("<<close-window>>", self.close_event)
        self.text.bind("<<center-insert>>", self.center_insert_event)
        self.text.bind("<<help>>", self.help_dialog)
        self.text.bind("<<about-idle>>", self.about_dialog)
        self.text.bind("<<open-module>>", self.open_module)
        self.text.bind("<<do-nothing>>", lambda event: "break")

        vbar['command'] = text.yview
        vbar.pack(side=RIGHT, fill=Y)

        text['yscrollcommand'] = vbar.set
        text['background'] = 'white'
        if sys.platform[:3] == 'win':
            text['font'] = ("lucida console", 8)
        text.pack(side=LEFT, fill=BOTH, expand=1)
        text.focus_set()

        self.auto = auto = self.AutoIndent(text)
        self.autoex = self.AutoExpand(text)
        self.per = per = self.Percolator(text)
        if self.ispythonsource(filename):
            self.color = color = self.ColorDelegator(); per.insertfilter(color)
            ##print "Initial colorizer"
        else:
            ##print "No initial colorizer"
            self.color = None
        self.undo = undo = self.UndoDelegator(); per.insertfilter(undo)
        self.search = search = self.SearchBinding(undo)
        self.io = io = self.IOBinding(undo)

        undo.set_saved_change_hook(self.saved_change_hook)
        io.set_filename_change_hook(self.filename_change_hook)

        if filename:
            if os.path.exists(filename):
                io.loadfile(filename)
            else:
                io.set_filename(filename)

        self.saved_change_hook()

    menu_specs = [
        ("file", "_File"),
        ("edit", "_Edit"),
        ("help", "_Help"),
    ]

    def createmenubar(self):
        mbar = self.menubar
        self.menudict = mdict = {}
        for name, label in self.menu_specs:
            underline, label = self.Bindings.prepstr(label)
            mdict[name] = menu = Menu(mbar, name=name)
            mbar.add_cascade(label=label, menu=menu, underline=underline)
        self.Bindings.fill_menus(self.text, mdict)

    def about_dialog(self, event=None):
        tkMessageBox.showinfo(self.about_title, self.about_text,
                              master=self.text)

    def help_dialog(self, event=None):
        from HelpWindow import HelpWindow
        HelpWindow(root=self.root)
    
    def open_module(self, event=None):
        try:
            name = self.text.get("sel.first", "sel.last")
        except TclError:
            name = ""
        else:
            name = string.strip(name)
        if not name:
            name = tkSimpleDialog.askstring("Module",
                     "Enter the name of a Python module\n"
                     "to search on sys.path and open:",
                     parent=self.text)
            if name:
                name = string.strip(name)
            if not name:
                return
        try:
            (f, file, (suffix, mode, type)) = imp.find_module(name)
        except ImportError, msg:
            tkMessageBox.showerror("Import error", str(msg), parent=self.text)
            return
        if type != imp.PY_SOURCE:
            tkMessageBox.showerror("Unsupported type",
                "%s is not a source module" % name, parent=self.text)
            return
        if f:
            f.close()
        self.flist.open(file, self)

    def gotoline(self, lineno):
        if lineno is not None and lineno > 0:
            self.text.mark_set("insert", "%d.0" % lineno)
            self.text.tag_remove("sel", "1.0", "end")
            self.text.tag_add("sel", "insert", "insert +1l")
            self.center()

    def ispythonsource(self, filename):
        if not filename:
            return 1
        if os.path.normcase(filename[-3:]) == ".py":
            return 1
        try:
            f = open(filename)
            line = f.readline()
            f.close()
        except IOError:
            return 0
        return line[:2] == '#!' and string.find(line, 'python') >= 0

    close_hook = None

    def set_close_hook(self, close_hook):
        self.close_hook = close_hook

    def filename_change_hook(self):
        self.saved_change_hook()
        if self.ispythonsource(self.io.filename):
            self.addcolorizer()
        else:
            self.rmcolorizer()

    def addcolorizer(self):
        if self.color:
            return
        ##print "Add colorizer"
        self.per.removefilter(self.undo)
        self.color = self.ColorDelegator()
        self.per.insertfilter(self.color)
        self.per.insertfilter(self.undo)

    def rmcolorizer(self):
        if not self.color:
            return
        ##print "Remove colorizer"
        self.per.removefilter(self.undo)
        self.per.removefilter(self.color)
        self.color = None
        self.per.insertfilter(self.undo)

    def saved_change_hook(self):
        if self.io.filename:
            title = self.io.filename
        else:
            title = "(Untitled)"
        if not self.undo.get_saved():
            title = title + " *"
        self.top.wm_title(title)

    def center_insert_event(self, event):
        self.center()

    def center(self, mark="insert"):
        insert = float(self.text.index(mark + " linestart"))
        end = float(self.text.index("end"))
        if insert > end-insert:
            self.text.see("1.0")
        else:
            self.text.see("end")
        self.text.see(mark)

    def close_event(self, event):
        self.close()

    def close(self):
        self.top.wm_deiconify()
        self.top.tkraise()
        reply = self.io.maybesave()
        if reply != "cancel":
            if self.color and self.color.colorizing:
                self.color.close()
                self.top.bell()
                return "cancel"
            if self.close_hook:
                self.close_hook()
            if self.color:
                self.color.close()          # Cancel colorization
            self.top.destroy()
        return reply


def fixwordbreaks(root):
    tk = root.tk
    tk.call('tcl_wordBreakAfter', 'a b', 0) # make sure word.tcl is loaded
    tk.call('set', 'tcl_wordchars', '[a-zA-Z0-9_]')
    tk.call('set', 'tcl_nonwordchars', '[^a-zA-Z0-9_]')


def test():
    root = Tk()
    fixwordbreaks(root)
    root.withdraw()
    if sys.argv[1:]:
        filename = sys.argv[1]
    else:
        filename = None
    edit = EditorWindow(root, filename)
    edit.set_close_hook(root.quit)
    root.mainloop()
    root.destroy()

if __name__ == '__main__':
    test()
