import os
import bdb
import traceback
from Tkinter import *


class Debugger(bdb.Bdb):
    
    def __init__(self, pyshell):
        bdb.Bdb.__init__(self)
        self.pyshell = pyshell
        self.make_gui()
    
    def close(self):
        self.top.destroy()

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
        self.bframe = bframe = Frame(top)
        self.bframe.pack()
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
        self.status = Label(top)
        self.status.pack()
    
    def interaction(self, frame, info=None):
        self.frame = frame
        code = frame.f_code
        file = code.co_filename
        lineno = frame.f_lineno
        message = "file=%s, name=%s, line=%s" % (file, code.co_name, lineno)
        if info:
            type, value, tb = info
            m1 = "%s" % str(type)
##            if value is not None:
##                try:
##                    m1 = "%s: %s" % (m1, str(value))
##                except:
##                    pass
            message = "%s\n%s" % (message, m1)
        self.status.configure(text=message)
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
