import string
import sys
import os
from Tkinter import *
import linecache
from repr import Repr


class StackViewer:

    def __init__(self, root=None, flist=None):
        self.flist = flist
        # Create root and/or toplevel window
        if not root:
            import Tkinter
            root = Tkinter._default_root
        if not root:
            root = top = Tk()
        else:
            top = Toplevel(root)
        self.top.protocol("WM_DELETE_WINDOW", self.close)
        self.root = root
        self.top = top
        top.wm_title("Stack viewer")
        # Create help label
        self.helplabel = Label(top,
            text="Click once to view variables; twice for source",
            borderwidth=2, relief="groove")
        self.helplabel.pack(fill="x")
        # Create top frame, with scrollbar and listbox
        self.topframe = Frame(top)
        self.topframe.pack(fill="both", expand=1)
        self.vbar = Scrollbar(self.topframe, name="vbar")
        self.vbar.pack(side="right", fill="y")
        self.listbox = Listbox(self.topframe, exportselection=0, 
                               takefocus=1, width=60)
        self.listbox.pack(expand=1, fill="both")
        # Tie listbox and scrollbar together
        self.vbar["command"] = self.listbox.yview
        self.listbox["yscrollcommand"] = self.vbar.set
        # Bind events to the list box
        self.listbox.bind("<ButtonRelease-1>", self.click_event)
	self.listbox.bind("<Double-ButtonRelease-1>", self.double_click_event)
        self.listbox.bind("<ButtonPress-3>", self.popup_event)
        self.listbox.bind("<Key-Up>", self.up_event)
        self.listbox.bind("<Key-Down>", self.down_event)
        # Create status label
        self.statuslabel = Label(top, text="status")
        self.statuslabel.pack(fill="x")
        # Load the stack
        linecache.checkcache()
        stack = getstack()
        self.load_stack(stack)
        self.statuslabel.config(text=getexception())
    
    def close(self):
        self.top.destroy()

    def load_stack(self, stack):
        self.stack = stack
        l = self.listbox
        l.delete(0, END)
        if len(stack) > 10:
            l["height"] = 10
            self.topframe.pack(expand=1)
        else:
            l["height"] = len(stack)
            self.topframe.pack(expand=0)
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
            l.insert(END, item)
        l.focus_set()
        l.selection_clear(0, "end")
        l.activate("end")
        l.see("end")

    rmenu = None

    def click_event(self, event):
        self.listbox.activate("@%d,%d" % (event.x, event.y))
        self.show_stack_frame()
        return "break"

    def popup_event(self, event):
        if not self.rmenu:
            self.make_menu()
        rmenu = self.rmenu
        self.event = event
        self.listbox.activate("@%d,%d" % (event.x, event.y))
        rmenu.tk_popup(event.x_root, event.y_root)

    def make_menu(self):
        rmenu = Menu(self.top, tearoff=0)
        rmenu.add_command(label="Go to source line",
                          command=self.goto_source_line)
        rmenu.add_command(label="Show stack frame",
                          command=self.show_stack_frame)
        self.rmenu = rmenu

    def goto_source_line(self):
        index = self.listbox.index("active")
        self.show_source(index)
    
    def show_stack_frame(self):
        index = self.listbox.index("active")
        self.show_frame(index)

    def double_click_event(self, event):
        index = self.listbox.index("active")
        self.show_source(index)
        return "break"
    
    def up_event(self, event):
        index = self.listbox.index("active") - 1
        if index < 0:
            self.top.bell()
            return "break"
        self.show_frame(index)
        return "break"
        
    def down_event(self, event):
        index = self.listbox.index("active") + 1
        if index >= len(self.stack):
            self.top.bell()
            return "break"
        self.show_frame(index)
        return "break"
    
    def show_source(self, index):
        if not 0 <= index < len(self.stack):
            self.top.bell()
            return
        frame, lineno = self.stack[index]
        code = frame.f_code
        filename = code.co_filename
        if not self.flist:
	    self.top.bell()
	    return
        if not os.path.exists(filename):
	    self.top.bell()
	    return
        edit = self.flist.open(filename)
        edit.gotoline(lineno)
    
    localsframe = None
    localsviewer = None
    localsdict = None
    globalsframe = None
    globalsviewer = None
    globalsdict = None
    curframe = None

    def show_frame(self, index):
        if not 0 <= index < len(self.stack):
            self.top.bell()
            return
        self.listbox.selection_clear(0, "end")
        self.listbox.selection_set(index)
        self.listbox.activate(index)
        self.listbox.see(index)
        self.listbox.focus_set()
        frame, lineno = self.stack[index]
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


def getstack(t=None, f=None):
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
    
    def __init__(self, frame, title, dict):
        width = 0
        height = 20*len(dict) # XXX 20 == observed height of Entry widget
        self.frame = frame
        self.title = title
        self.dict = dict
        self.repr = Repr()
        self.repr.maxstring = 60
        self.repr.maxother = 60
        self.label = Label(frame, text=title, borderwidth=2, relief="groove")
        self.label.pack(fill="x")
        self.vbar = vbar = Scrollbar(frame, name="vbar")
        vbar.pack(side="right", fill="y")
        self.canvas = canvas = Canvas(frame,
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
        frame.update_idletasks() # Alas!
        width = subframe.winfo_reqwidth()
        height = subframe.winfo_reqheight()
        canvas["scrollregion"] = (0, 0, width, height)
        if height > 300:
            canvas["height"] = 300
            frame.pack(expand=1)
        else:
            canvas["height"] = height
            frame.pack(expand=0)

    def close(self):
        for c in self.subframe, self.label, self.vbar, self.canvas:
            try:
                c.destroy()
            except:
                pass
