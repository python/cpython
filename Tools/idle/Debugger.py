import os
import bdb
import traceback
from Tkinter import *

import StackViewer


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
        #
        self.bframe = bframe = Frame(top)
        self.bframe.pack(anchor="w")
        self.buttons = bl = []
        #
        self.bcont = b = Button(bframe, text="Go", command=self.cont)
        bl.append(b)
        self.bstep = b = Button(bframe, text="Into", command=self.step)
        bl.append(b)
        self.bnext = b = Button(bframe, text="Over", command=self.next)
        bl.append(b)
        self.bret = b = Button(bframe, text="Out", command=self.ret)
        bl.append(b)
        #
        for b in bl:
            b.configure(state="disabled")
            b.pack(side="left")
        #
        self.cframe = cframe = Frame(bframe)
        self.cframe.pack(side="left")
        #
        self.vstack = BooleanVar(top)
        self.bstack = Checkbutton(cframe,
            text="Stack", command=self.show_stack, variable=self.vstack)
        self.bstack.grid(row=0, column=0)
        self.vsource = BooleanVar(top)
        self.bsource = Checkbutton(cframe,
            text="Source", command=self.show_source, variable=self.vsource)
        self.bsource.grid(row=0, column=1)
        self.vlocals = BooleanVar(top)
        self.blocals = Checkbutton(cframe,
            text="Locals", command=self.show_locals, variable=self.vlocals)
        self.blocals.grid(row=1, column=0)
        self.vglobals = BooleanVar(top)
        self.bglobals = Checkbutton(cframe,
            text="Globals", command=self.show_globals, variable=self.vglobals)
        self.bglobals.grid(row=1, column=1)
        #
        self.status = Label(top, anchor="w")
        self.status.pack(anchor="w")
        self.error = Label(top, anchor="w")
        self.error.pack(anchor="w")
        #
        self.fstack = Frame(top, height=1)
        self.fstack.pack(expand=1, fill="both")
        self.flocals = Frame(top)
        self.flocals.pack(expand=1, fill="both")
        self.fglobals = Frame(top, height=1)
        self.fglobals.pack(expand=1, fill="both")
    
    def interaction(self, frame, info=None):
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
            tb = None
        self.error.configure(text=m1)
        sv = self.stackviewer
        if sv:
            stack, i = self.get_stack(self.frame, tb)
            sv.load_stack(stack, i)
        lv = self.localsviewer
        gv = self.globalsviewer
        if lv:
            if not gv or self.frame.f_locals is not self.frame.f_globals:
                lv.load_dict(self.frame.f_locals)
            else:
                lv.load_dict(None)
        if gv:
            gv.load_dict(self.frame.f_globals)
        if self.vsource.get():
            self.sync_source_line()
        for b in self.buttons:
            b.configure(state="normal")
        self.top.tkraise()
        self.root.mainloop()
        for b in self.buttons:
            b.configure(state="disabled")
        self.status.configure(text="")
        self.error.configure(text="")
        self.frame = None
    
    def sync_source_line(self):
        frame = self.frame
        if not frame:
            return
        code = frame.f_code
        file = code.co_filename
        lineno = frame.f_lineno
        if file[:1] + file[-1:] != "<>" and os.path.exists(file):
            edit = self.flist.open(file)
            if edit:
                edit.gotoline(lineno)
    
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

    stackviewer = None

    def show_stack(self):
        if not self.stackviewer and self.vstack.get():
            self.stackviewer = sv = StackViewer.StackViewer(
                self.fstack, self.flist, self)
            if self.frame:
                stack, i = self.get_stack(self.frame, None)
                sv.load_stack(stack, i)
        else:
            sv = self.stackviewer
            if sv and not self.vstack.get():
                self.stackviewer = None
                sv.close()
            self.fstack['height'] = 1
    
    def show_source(self):
        if self.vsource.get():
            self.sync_source_line()

    def show_frame(self, (frame, lineno)):
        pass

    localsviewer = None

    def show_locals(self):
        lv = self.localsviewer
        if not lv and self.vlocals.get():
            self.localsviewer = lv = StackViewer.NamespaceViewer(
                self.flocals, "Locals")
            if self.frame:
                lv.load_dict(self.frame.f_locals)
        elif lv and not self.vlocals.get():
            self.localsviewer = None
            lv.close()

    globalsviewer = None

    def show_globals(self):
        lv = self.globalsviewer
        if not lv and self.vglobals.get():
            self.globalsviewer = lv = StackViewer.NamespaceViewer(
                self.fglobals, "Locals")
            if self.frame:
                lv.load_dict(self.frame.f_globals)
        elif lv and not self.vglobals.get():
            self.globalsviewer = None
            lv.close()
