import os
import bdb
import traceback
from Tkinter import *


class Debugger(bdb.Bdb):
    
    interacting = 0
    
    def __init__(self, pyshell):
        bdb.Bdb.__init__(self)
        self.pyshell = pyshell
        self.make_gui()
    
    def close(self):
        if self.interacting:
            self.top.bell()
            return
        self.pyshell.close_debugger()
        self.top.destroy()
        
    def run(self, *args):
        try:
            self.interacting = 1
            return apply(bdb.Bdb.run, (self,) + args)
        finally:
            self.interacting = 0

    def user_line(self, frame):
        self.interaction(frame)

    def user_return(self, frame, rv):
        # XXX show rv?
        self.interaction(frame)

    def user_exception(self, frame, info):
        self.interaction(frame, info)
    
    def make_gui(self):
        pyshell = self.pyshell
        self.flist = pyshell.flist
        self.root = root = pyshell.root
        self.top = top = Toplevel(root)
        top.wm_protocol("WM_DELETE_WINDOW", self.close)
        self.bframe = bframe = Frame(top)
        self.bframe.pack(anchor="w")
        self.buttons = bl = []
        self.bcont = b = Button(bframe, text="Go", command=self.cont)
        bl.append(b)
        self.bstep = b = Button(bframe, text="Step into", command=self.step)
        bl.append(b)
        self.bnext = b = Button(bframe, text="Step over", command=self.next)
        bl.append(b)
        self.bret = b = Button(bframe, text="Step out", command=self.ret)
        bl.append(b)
        for b in bl:
            b.configure(state="disabled")
            b.pack(side="left")
        self.status = Label(top, anchor="w")
        self.status.pack(anchor="w")
        self.error = Label(top, anchor="w")
        self.error.pack(anchor="w")
    
    def interaction(self, frame, info=None):
        self.top.pack_propagate(0)
        self.frame = frame
        code = frame.f_code
        file = code.co_filename
        base = os.path.basename(file)
        lineno = frame.f_lineno
        message = "%s:%s" % (base, lineno)
        if code.co_name != "?":
            message = "%s: %s()" % (message, code.co_name)
        self.status.configure(text=message)
        if info:
            type, value, tb = info
            try:
                m1 = type.__name__
            except AttributeError:
                m1 = "%s" % str(type)
            if value is not None:
                try:
                    m1 = "%s: %s" % (m1, str(value))
                except:
                    pass
        else:
            m1 = ""
        self.error.configure(text=m1)
        if file[:1] + file[-1:] != "<>" and os.path.exists(file):
            edit = self.flist.open(file)
            if edit:
                edit.gotoline(lineno)
        for b in self.buttons:
            b.configure(state="normal")
        self.top.tkraise()
        self.root.mainloop()
        for b in self.buttons:
            b.configure(state="disabled")
        self.status.configure(text="")
        self.error.configure(text="")
        self.frame = None
    
    def cont(self):
        self.set_continue()
        self.root.quit()
        
    def step(self):
        self.set_step()
        self.root.quit()
    
    def next(self):
        self.set_next(self.frame)
        self.root.quit()
    
    def ret(self):
        self.set_return(self.frame)
        self.root.quit()
