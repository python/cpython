from Tkinter import *
import string
import pyclbr

class ClassBrowser:
    
    def __init__(self, root, name):
        try:
            dict = pyclbr.readmodule(name)
        except ImportError, msg:
            tkMessageBox.showerror("Import error", str(msg), parent=root)
            return
        if not dict:
            tkMessageBox.showerror("Nothing to browse",
                "Module %s defines no classes" % name, parent=root)
            return
        self.root = root
        self.top = top = Toplevel(root)
        # Create help label
        self.helplabel = Label(top,
            text="Classes in module %s" % name,
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
        ##self.listbox.bind("<ButtonPress-3>", self.popup_event)
        self.listbox.bind("<Key-Up>", self.up_event)
        self.listbox.bind("<Key-Down>", self.down_event)
        # Load the classes
        self.loadclasses(dict)
        
    def loadclasses(self, dict):
        items = dict.items()
        items.sort()
        l = self.listbox
        for key, value in items:
            s = key
            if value.super:
                super = []
                for sup in value.super:
                    name = sup.name
                    if sup.module != value.module:
                        name = "%s.%s" % (sup.module, name)
                    super.append(name)
                s = s + "(%s)" % string.join(super, ", ")
            l.insert(END, s)

    def click_event(self, event):
        pass
    
    def double_click_event(self, event):
        pass
    
    def up_event(self, event):
        pass
    
    def down_event(self, event):
        pass
