import sys
import os
import string
from Tkinter import *


class EditorWindow:

    from Percolator import Percolator
    from ColorDelegator import ColorDelegator
    from UndoDelegator import UndoDelegator
    from IOBinding import IOBinding
    from SearchBinding import SearchBinding
    from AutoIndent import AutoIndent
    from AutoExpand import AutoExpand
    import Bindings

    def __init__(self, root, filename=None):
        self.top = top = Toplevel(root)
        self.vbar = vbar = Scrollbar(top, name='vbar')
        self.text = text = Text(top, name='text')

        self.Bindings.apply_bindings(text)

        self.top.protocol("WM_DELETE_WINDOW", self.close)
        self.top.bind("<<close-window>>", self.close_event)
        self.text.bind("<<center-insert>>", self.center_insert_event)

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
