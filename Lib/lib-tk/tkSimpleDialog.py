#
# An Introduction to Tkinter
# tkSimpleDialog.py
#
# Copyright (c) 1997 by Fredrik Lundh
#
# fredrik@pythonware.com
# http://www.pythonware.com
#

# --------------------------------------------------------------------
# dialog base class

from Tkinter import *
import os

class Dialog(Toplevel):

    def __init__(self, parent, title = None):

        Toplevel.__init__(self, parent)
        self.transient(parent)

        if title:
            self.title(title)

        self.parent = parent

        self.result = None

        body = Frame(self)
        self.initial_focus = self.body(body)
        body.pack(padx=5, pady=5)

        self.buttonbox()

        self.grab_set()

        if not self.initial_focus:
            self.initial_focus = self

        self.protocol("WM_DELETE_WINDOW", self.cancel)

        self.geometry("+%d+%d" % (parent.winfo_rootx()+50,
                                  parent.winfo_rooty()+50))

        self.initial_focus.focus_set()

        self.wait_window(self)

    #
    # construction hooks

    def body(self, master):
        # create dialog body.  return widget that should have
        # initial focus.  this method should be overridden

        pass

    def buttonbox(self):
        # add standard button box. override if you don't want the
        # standard buttons
        
        box = Frame(self)

        w = Button(box, text="OK", width=10, command=self.ok, default=ACTIVE)
        w.pack(side=LEFT, padx=5, pady=5)
        w = Button(box, text="Cancel", width=10, command=self.cancel)
        w.pack(side=LEFT, padx=5, pady=5)

        self.bind("<Return>", self.ok)
        self.bind("<Escape>", self.cancel)

        box.pack()

    #
    # standard button semantics

    def ok(self, event=None):

        if not self.validate():
            self.initial_focus.focus_set() # put focus back
            return

        self.withdraw()
        self.update_idletasks()

        self.apply()

        self.cancel()

    def cancel(self, event=None):

        # put focus back to the parent window
        self.parent.focus_set()
        self.destroy()

    #
    # command hooks

    def validate(self):

        return 1 # override

    def apply(self):

        pass # override


# --------------------------------------------------------------------
# convenience dialogues

import string

class _QueryDialog(Dialog):

    def __init__(self, title, prompt,
                 initialvalue=None,
                 minvalue = None, maxvalue = None,
                 parent = None):

        if not parent:
            import Tkinter
            parent = Tkinter._default_root

        self.prompt   = prompt
        self.minvalue = minvalue
        self.maxvalue = maxvalue

        self.initialvalue = initialvalue

        Dialog.__init__(self, parent, title)

    def body(self, master):

        w = Label(master, text=self.prompt, justify=LEFT)
        w.grid(row=0, padx=5, sticky=W)

        self.entry = Entry(master, name="entry")
        self.entry.grid(row=1, padx=5, sticky=W+E)

        if self.initialvalue:
            self.entry.insert(0, self.initialvalue)
            self.entry.select_range(0, END)

        return self.entry

    def validate(self):

        import tkMessageBox

        try:
            result = self.getresult()
        except ValueError:
            tkMessageBox.showwarning(
                "Illegal value",
                self.errormessage + "\nPlease try again",
                parent = self
            )
            return 0

        if self.minvalue is not None and result < self.minvalue:
            tkMessageBox.showwarning(
                "Too small",
                "The allowed minimum value is %s. "
                "Please try again." % self.minvalue,
                parent = self
            )
            return 0

        if self.maxvalue is not None and result > self.maxvalue:
            tkMessageBox.showwarning(
                "Too large",
                "The allowed maximum value is %s. "
                "Please try again." % self.maxvalue,
                parent = self
            )
            return 0
                
        self.result = result

        return 1


class _QueryInteger(_QueryDialog):
    errormessage = "Not an integer."
    def getresult(self):
        return string.atoi(self.entry.get())

def askinteger(title, prompt, **kw):
    d = apply(_QueryInteger, (title, prompt), kw)
    return d.result

class _QueryFloat(_QueryDialog):
    errormessage = "Not a floating point value."
    def getresult(self):
        return string.atof(self.entry.get())

def askfloat(title, prompt, **kw):
    d = apply(_QueryFloat, (title, prompt), kw)
    return d.result

class _QueryString(_QueryDialog):
    def getresult(self):
        return self.entry.get()

def askstring(title, prompt, **kw):
    d = apply(_QueryString, (title, prompt), kw)
    return d.result

if __name__ == "__main__": 

    root = Tk()
    root.update()

    print askinteger("Spam", "Egg count", initialvalue=12*12)
    print askfloat("Spam", "Egg weight\n(in tons)", minvalue=1, maxvalue=100)
    print askstring("Spam", "Egg label")

