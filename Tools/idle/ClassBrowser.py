"""Primitive class browser.

XXX TO DO:

- generalize the scrolling listbox with some behavior into a base class
- add popup menu with more options (e.g. doc strings, base classes, imports)
- show function argument list (have to do pattern matching on source)
- should the classes and methods lists also be in the module's menu bar?

"""

import os
import string
import pyclbr
from Tkinter import *
import tkMessageBox
from WindowList import ListedToplevel

from ScrolledList import ScrolledList


class ClassBrowser:

    def __init__(self, flist, name, path=[]):
        root = flist.root
        try:
            dict = pyclbr.readmodule(name, path)
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
        self.top = top = ListedToplevel(root)
        self.top.protocol("WM_DELETE_WINDOW", self.close)
        top.wm_title("Class Browser - " + name)
        top.wm_iconname("ClBrowser")
        self.leftframe = leftframe = Frame(top)
        self.leftframe.pack(side="left", fill="both", expand=1)
        # Create help label
        self.helplabel = Label(leftframe, text="Module %s" % name,
                               relief="groove", borderwidth=2)
        self.helplabel.pack(fill="x")
        # Create top frame, with scrollbar and listbox
        self.classviewer = ClassViewer(
            self.leftframe, self.flist, self)
        # Load the classes
        self.load_classes(dict, name)

    def close(self):
        self.classviewer = None
        self.methodviewer = None
        self.top.destroy()

    def load_classes(self, dict, module):
        self.classviewer.load_classes(dict, module)
        if self.botframe:
            self.botframe.destroy()
            self.botframe = None
        self.methodviewer = None

    botframe = None
    methodhelplabel = None
    methodviewer = None

    def show_methods(self, cl):
        if not self.botframe:
            self.botframe = Frame(self.top)
            self.botframe.pack(side="right", expand=1, fill="both")
            self.methodhelplabel = Label(self.botframe,
                               relief="groove", borderwidth=2)
            self.methodhelplabel.pack(fill="x")
            self.methodviewer = MethodViewer(self.botframe, self.flist)
        self.methodhelplabel.config(text="Class %s" % cl.name)
        self.methodviewer.load_methods(cl)


class ClassViewer(ScrolledList):

    def __init__(self, master, flist, browser):
        ScrolledList.__init__(self, master)
        self.flist = flist
        self.browser = browser

    def load_classes(self, dict, module):
        self.clear()
        self.dict = dict
        items = []
        for key, value in dict.items():
            if value.module == module:
                items.append((value.lineno, key, value))
        items.sort()
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
            self.append(s)

    def getname(self, index):
        name = self.listbox.get(index)
        i = string.find(name, '(')
        if i >= 0:
            name = name[:i]
        return name

    def getclass(self, index):
        return self.dict[self.getname(index)]

    def on_select(self, index):
        self.show_methods(index)

    def on_double(self, index):
        self.show_source(index)

    def show_methods(self, index):
        cl = self.getclass(index)
        self.browser.show_methods(cl)

    def show_source(self, index):
        cl = self.getclass(index)
        if os.path.isfile(cl.file):
            edit = self.flist.open(cl.file)
            edit.gotoline(cl.lineno)


class MethodViewer(ScrolledList):

    def __init__(self, master, flist):
        ScrolledList.__init__(self, master)
        self.flist = flist

    classinfo = None

    def load_methods(self, cl):
        self.classinfo = cl
        self.clear()
        items = []
        for name, lineno in cl.methods.items():
            items.append((lineno, name))
        items.sort()
        for item, name in items:
            self.append(name)

    def click_event(self, event):
        pass

    def on_double(self, index):
        self.show_source(self.get(index))

    def show_source(self, name):
        if os.path.isfile(self.classinfo.file):
            edit = self.flist.open(self.classinfo.file)
            edit.gotoline(self.classinfo.methods[name])
