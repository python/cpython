import string
import sys
import os
from Tkinter import *
import linecache
from repr import Repr

from ScrolledList import ScrolledList


class StackBrowser:
    
    def __init__(self, root, flist, stack=None):
        self.top = top = Toplevel(root)
        top.protocol("WM_DELETE_WINDOW", self.close)
        top.wm_title("Stack viewer")
        # Create help label
        self.helplabel = Label(top,
            text="Click once to view variables; twice for source",
            borderwidth=2, relief="groove")
        self.helplabel.pack(fill="x")
        #
        self.sv = StackViewer(top, flist, self)
        if stack is None:
            stack = get_stack()
        self.sv.load_stack(stack)
    
    def close(self):
        self.top.destroy()

    localsframe = None
    localsviewer = None
    localsdict = None
    globalsframe = None
    globalsviewer = None
    globalsdict = None
    curframe = None

    def show_frame(self, (frame, lineno)):
        if frame is self.curframe:
            return
        self.curframe = None
        if frame.f_globals is not self.globalsdict:
            self.show_globals(frame)
        self.show_locals(frame)
        self.curframe = frame
    
    def show_globals(self, frame):
        title = "Global Variables"
        if frame.f_globals.has_key("__name__"):
            try:
                name = str(frame.f_globals["__name__"]) + ""
            except:
                name = ""
            if name:
                title = title + " in module " + name
        self.globalsdict = None
        if self.globalsviewer:
            self.globalsviewer.close()
        self.globalsviewer = None
        if not self.globalsframe:
            self.globalsframe = Frame(self.top)
        self.globalsdict = frame.f_globals
        self.globalsviewer = NamespaceViewer(
            self.globalsframe,
            title,
            self.globalsdict)
        self.globalsframe.pack(fill="both", side="bottom")
    
    def show_locals(self, frame):
        self.localsdict = None
        if self.localsviewer:
            self.localsviewer.close()
        self.localsviewer = None
        if frame.f_locals is not frame.f_globals:
            title = "Local Variables"
            code = frame.f_code
            funcname = code.co_name
            if funcname not in ("?", "", None):
                title = title + " in " + funcname
            if not self.localsframe:
                self.localsframe = Frame(self.top)
            self.localsdict = frame.f_locals
            self.localsviewer = NamespaceViewer(
                self.localsframe,
                title,
                self.localsdict)
            self.localsframe.pack(fill="both", side="top")
        else:
            if self.localsframe:
                self.localsframe.forget()


class StackViewer(ScrolledList):
    
    def __init__(self, master, flist, browser):
        ScrolledList.__init__(self, master)
        self.flist = flist
        self.browser = browser

    def load_stack(self, stack, index=None):
        self.stack = stack
        self.clear()
##        if len(stack) > 10:
##            l["height"] = 10
##            self.topframe.pack(expand=1)
##        else:
##            l["height"] = len(stack)
##            self.topframe.pack(expand=0)
        for frame, lineno in stack:
            try:
                modname = frame.f_globals["__name__"]
            except:
                modname = "?"
            code = frame.f_code
            filename = code.co_filename
            funcname = code.co_name
            sourceline = linecache.getline(filename, lineno)
            sourceline = string.strip(sourceline)
            if funcname in ("?", "", None):
                item = "%s, line %d: %s" % (modname, lineno, sourceline)
            else:
                item = "%s.%s(), line %d: %s" % (modname, funcname,
                                                 lineno, sourceline)
            self.append(item)
        if index is not None:
            self.select(index)

    def fill_menu(self):
        menu = self.menu
        menu.add_command(label="Go to source line",
                         command=self.goto_source_line)
        menu.add_command(label="Show stack frame",
                         command=self.show_stack_frame)

    def on_select(self, index):
        self.browser.show_frame(self.stack[index])

    def on_double(self, index):
        self.show_source(index)

    def goto_source_line(self):
        index = self.listbox.index("active")
        self.show_source(index)

    def show_stack_frame(self):
        index = self.listbox.index("active")
        self.browser.show_frame(self.stack[index])
    
    def show_source(self, index):
        frame, lineno = self.stack[index]
        code = frame.f_code
        filename = code.co_filename
        if os.path.isfile(filename):
            edit = self.flist.open(filename)
            if edit:
                edit.gotoline(lineno)


def get_stack(t=None, f=None):
    if t is None:
        t = sys.last_traceback
    stack = []
    if t and t.tb_frame is f:
        t = t.tb_next
    while f is not None:
        stack.append((f, f.f_lineno))
        if f is self.botframe:
	    break
        f = f.f_back
    stack.reverse()
    while t is not None:
        stack.append((t.tb_frame, t.tb_lineno))
        t = t.tb_next
    return stack


def getexception(type=None, value=None):
    if type is None:
        type = sys.last_type
        value = sys.last_value
    if hasattr(type, "__name__"):
        type = type.__name__
    s = str(type)
    if value is not None:
        s = s + ": " + str(value)
    return s


class NamespaceViewer:
    
    def __init__(self, master, title, dict):
        width = 0
        height = 20*len(dict) # XXX 20 == observed height of Entry widget
        self.master = master
        self.title = title
        self.dict = dict
        self.repr = Repr()
        self.repr.maxstring = 60
        self.repr.maxother = 60
        self.label = Label(master, text=title, borderwidth=2, relief="groove")
        self.label.pack(fill="x")
        self.vbar = vbar = Scrollbar(master, name="vbar")
        vbar.pack(side="right", fill="y")
        self.canvas = canvas = Canvas(master,
                                      height=min(300, max(40, height)),
                                      scrollregion=(0, 0, width, height))
        canvas.pack(side="left", fill="both", expand=1)
        vbar["command"] = canvas.yview
        canvas["yscrollcommand"] = vbar.set
        self.subframe = subframe = Frame(canvas)
        self.sfid = canvas.create_window(0, 0, window=subframe, anchor="nw")
        names = dict.keys()
        names.sort()
        row = 0
        for name in names:
            value = dict[name]
            svalue = self.repr.repr(value) # repr(value)
            l = Label(subframe, text=name)
            l.grid(row=row, column=0, sticky="nw")
##            l = Label(subframe, text=svalue, justify="l", wraplength=300)
            l = Entry(subframe, width=0, borderwidth=0)
            l.insert(0, svalue)
##            l["state"] = "disabled"
            l.grid(row=row, column=1, sticky="nw")
            row = row+1
        subframe.update_idletasks() # Alas!
        width = subframe.winfo_reqwidth()
        height = subframe.winfo_reqheight()
        canvas["scrollregion"] = (0, 0, width, height)
##        if height > 300:
##            canvas["height"] = 300
##            master.pack(expand=1)
##        else:
##            canvas["height"] = height
##            master.pack(expand=0)

    def close(self):
        for c in self.subframe, self.label, self.vbar, self.canvas:
            try:
                c.destroy()
            except:
                pass
