"""Primitive class browser.

XXX TO DO:
    
- generalize the scrolling listbox with some behavior into a base class
- add popup menu with more options (e.g. doc strings, base classes, imports)
- show function argument list (have to do pattern matching on source)
- should the classes and methods lists also be in the module's menu bar?

"""

import string
import pyclbr
from Tkinter import *
import tkMessageBox

class ClassBrowser:
    
    def __init__(self, flist, name):
        root = flist.root
        try:
            dict = pyclbr.readmodule(name)
        except ImportError, msg:
            tkMessageBox.showerror("Import error", str(msg), parent=root)
            return
        if not dict:
            tkMessageBox.showerror("Nothing to browse",
                "Module %s defines no classes" % name, parent=root)
            return
        self.flist = flist
        self.dict = dict
        self.root = root
        self.top = top = Toplevel(root)
        self.top.protocol("WM_DELETE_WINDOW", self.close)
        self.leftframe = leftframe = Frame(top)
        self.leftframe.pack(side="left", fill="both", expand=1)
        top.wm_title("Class browser")
        # Create help label
        self.helplabel = Label(leftframe,
            text="Classes in module %s" % name,
            borderwidth=2, relief="groove")
        self.helplabel.pack(fill="x")
        # Create top frame, with scrollbar and listbox
        self.topframe = Frame(leftframe)
        self.topframe.pack(fill="both", expand=1)
        self.vbar = Scrollbar(self.topframe, name="vbar")
        self.vbar.pack(side="right", fill="y")
        self.listbox = Listbox(self.topframe, exportselection=0, 
                               takefocus=1, width=30)
        self.listbox.pack(expand=1, fill="both")
        # Tie listbox and scrollbar together
        self.vbar["command"] = self.listbox.yview
        self.listbox["yscrollcommand"] = self.vbar.set
        # Bind events to the list box
        self.listbox.bind("<ButtonRelease-1>", self.click_event)
	self.listbox.bind("<Double-ButtonRelease-1>", self.double_click_event)
        ##self.listbox.bind("<ButtonPress-3>", self.popup_event)
        self.listbox.bind("<Key-Up>", self.up_event)
        self.listbox.bind("<Key-Down>", self.down_event)
        # Load the classes
        self.loadclasses(dict, name)
    
    def close(self):
        self.top.destroy()
        
    def loadclasses(self, dict, module):
        items = []
        for key, value in dict.items():
            if value.module == module:
                items.append((value.lineno, key, value))
        items.sort()
        l = self.listbox
        for lineno, key, value in items:
            s = key
            if value.super:
                super = []
                for sup in value.super:
                    name = sup.name
                    if sup.module != value.module:
                        name = "%s.%s" % (sup.module, name)
                    super.append(name)
                s = s + "(%s)" % string.join(super, ", ")
            l.insert("end", s)
        l.focus_set()
        l.selection_clear(0, "end")
        if self.botframe:
            self.botframe.destroy()
            self.botframe = None
        self.methodviewer = None

    def click_event(self, event):
        self.listbox.activate("@%d,%d" % (event.x, event.y))
        index = self.listbox.index("active")
        self.show_methods(index)
    
    def double_click_event(self, event):
        self.listbox.activate("@%d,%d" % (event.x, event.y))
        index = self.listbox.index("active")
        self.show_source(index)
    
    def up_event(self, event):
        index = self.listbox.index("active") - 1
        if index < 0:
            self.top.bell()
            return "break"
        self.show_methods(index)
        return "break"
    
    def down_event(self, event):
        index = self.listbox.index("active") + 1
        if index >= self.listbox.index("end"):
            self.top.bell()
            return "break"
        self.show_methods(index)
        return "break"
        
    def show_source(self, index):
        name = self.listbox.get(index)
        i = string.find(name, '(')
        if i >= 0:
            name = name[:i]
        cl = self.dict[name]
        edit = self.flist.open(cl.file)
        edit.gotoline(cl.lineno)

    botframe = None
    methodviewer = None
    
    def show_methods(self, index):
        self.listbox.selection_clear(0, "end")
        self.listbox.selection_set(index)
        self.listbox.activate(index)
        self.listbox.see(index)
        self.listbox.focus_set()
        name = self.listbox.get(index)
        i = string.find(name, '(')
        if i >= 0:
            name = name[:i]
        cl = self.dict[name]
        if not self.botframe:
            self.botframe = Frame(self.top)
            self.botframe.pack(expand=1, fill="both")
        if not self.methodviewer:
            self.methodviewer = MethodViewer(self.botframe, self.flist)
        self.methodviewer.loadmethods(cl)

class MethodViewer:
    
    # XXX There's a pattern emerging here...
    
    def __init__(self, frame, flist):
        self.frame = frame
        self.flist = flist
        # Create help label
        self.helplabel = Label(frame,
            text="Methods", borderwidth=2, relief="groove")
        self.helplabel.pack(fill="x")
        # Create top frame, with scrollbar and listbox
        self.topframe = Frame(frame)
        self.topframe.pack(fill="both", expand=1)
        self.vbar = Scrollbar(self.topframe, name="vbar")
        self.vbar.pack(side="right", fill="y")
        self.listbox = Listbox(self.topframe, exportselection=0, 
                               takefocus=1, width=30)
        self.listbox.pack(expand=1, fill="both")
        # Tie listbox and scrollbar together
        self.vbar["command"] = self.listbox.yview
        self.listbox["yscrollcommand"] = self.vbar.set
        # Bind events to the list box
        self.listbox.bind("<ButtonRelease-1>", self.click_event)
	self.listbox.bind("<Double-ButtonRelease-1>", self.double_click_event)
        ##self.listbox.bind("<ButtonPress-3>", self.popup_event)
        self.listbox.bind("<Key-Up>", self.up_event)
        self.listbox.bind("<Key-Down>", self.down_event)
        
    classinfo = None
    
    def loadmethods(self, cl):
        self.classinfo = cl
        self.helplabel.config(text="Methods of class %s" % cl.name)
        l = self.listbox
        l.delete(0, "end")
        l.selection_clear(0, "end")
        items = []
        for name, lineno in cl.methods.items():
            items.append((lineno, name))
        items.sort()
        for item, name in items:
            l.insert("end", name)

    def click_event(self, event):
        pass
    
    def double_click_event(self, event):
        self.listbox.activate("@%d,%d" % (event.x, event.y))
        index = self.listbox.index("active")
        self.show_source(index)
    
    def up_event(self, event):
        pass
    
    def down_event(self, event):
        pass
    
    def show_source(self, index):
        name = self.listbox.get(index)
        i = string.find(name, '(')
        if i >= 0:
            name = name[:i]
        edit = self.flist.open(self.classinfo.file)
        edit.gotoline(self.classinfo.methods[name])
        
