import os
import bdb
import traceback
from Tkinter import *
from WindowList import ListedToplevel

import StackViewer


class Debugger(bdb.Bdb):

    interacting = 0

    vstack = vsource = vlocals = vglobals = None

    def __init__(self, pyshell):
        bdb.Bdb.__init__(self)
        self.pyshell = pyshell
        self.make_gui()
    
    def canonic(self, filename):
        # Canonicalize filename -- called by Bdb
        return os.path.normcase(os.path.abspath(filename))

    def close(self, event=None):
        if self.interacting:
            self.top.bell()
            return
        if self.stackviewer:
            self.stackviewer.close(); self.stackviewer = None
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
        ##self.interaction(frame)
        pass

    def user_exception(self, frame, info):
        self.interaction(frame, info)

    def make_gui(self):
        pyshell = self.pyshell
        self.flist = pyshell.flist
        self.root = root = pyshell.root
        self.top = top =ListedToplevel(root)
        self.top.wm_title("Debug Control")
        self.top.wm_iconname("Debug")
        top.wm_protocol("WM_DELETE_WINDOW", self.close)
        self.top.bind("<Escape>", self.close)
        #
        self.bframe = bframe = Frame(top)
        self.bframe.pack(anchor="w")
        self.buttons = bl = []
        #
        self.bcont = b = Button(bframe, text="Go", command=self.cont)
        bl.append(b)
        self.bstep = b = Button(bframe, text="Step", command=self.step)
        bl.append(b)
        self.bnext = b = Button(bframe, text="Over", command=self.next)
        bl.append(b)
        self.bret = b = Button(bframe, text="Out", command=self.ret)
        bl.append(b)
        self.bret = b = Button(bframe, text="Quit", command=self.quit)
        bl.append(b)
        #
        for b in bl:
            b.configure(state="disabled")
            b.pack(side="left")
        #
        self.cframe = cframe = Frame(bframe)
        self.cframe.pack(side="left")
        #
        if not self.vstack:
            self.__class__.vstack = BooleanVar(top)
            self.vstack.set(1)
        self.bstack = Checkbutton(cframe,
            text="Stack", command=self.show_stack, variable=self.vstack)
        self.bstack.grid(row=0, column=0)
        if not self.vsource:
            self.__class__.vsource = BooleanVar(top)
            ##self.vsource.set(1)
        self.bsource = Checkbutton(cframe,
            text="Source", command=self.show_source, variable=self.vsource)
        self.bsource.grid(row=0, column=1)
        if not self.vlocals:
            self.__class__.vlocals = BooleanVar(top)
            self.vlocals.set(1)
        self.blocals = Checkbutton(cframe,
            text="Locals", command=self.show_locals, variable=self.vlocals)
        self.blocals.grid(row=1, column=0)
        if not self.vglobals:
            self.__class__.vglobals = BooleanVar(top)
            ##self.vglobals.set(1)
        self.bglobals = Checkbutton(cframe,
            text="Globals", command=self.show_globals, variable=self.vglobals)
        self.bglobals.grid(row=1, column=1)
        #
        self.status = Label(top, anchor="w")
        self.status.pack(anchor="w")
        self.error = Label(top, anchor="w")
        self.error.pack(anchor="w", fill="x")
        self.errorbg = self.error.cget("background")
        #
        self.fstack = Frame(top, height=1)
        self.fstack.pack(expand=1, fill="both")
        self.flocals = Frame(top)
        self.flocals.pack(expand=1, fill="both")
        self.fglobals = Frame(top, height=1)
        self.fglobals.pack(expand=1, fill="both")
        #
        if self.vstack.get():
            self.show_stack()
        if self.vlocals.get():
            self.show_locals()
        if self.vglobals.get():
            self.show_globals()

    frame = None

    def interaction(self, frame, info=None):
        self.frame = frame
        code = frame.f_code
        file = code.co_filename
        base = os.path.basename(file)
        lineno = frame.f_lineno
        #
        message = "%s:%s" % (base, lineno)
        if code.co_name != "?":
            message = "%s: %s()" % (message, code.co_name)
        self.status.configure(text=message)
        #
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
            bg = "yellow"
        else:
            m1 = ""
            tb = None
            bg = self.errorbg
        self.error.configure(text=m1, background=bg)
        #
        sv = self.stackviewer
        if sv:
            stack, i = self.get_stack(self.frame, tb)
            sv.load_stack(stack, i)
        #
        self.show_variables(1)
        #
        if self.vsource.get():
            self.sync_source_line()
        #
        for b in self.buttons:
            b.configure(state="normal")
        #
        self.top.tkraise()
        self.root.mainloop()
        #
        for b in self.buttons:
            b.configure(state="disabled")
        self.status.configure(text="")
        self.error.configure(text="", background=self.errorbg)
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

    def quit(self):
        self.set_quit()
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
        self.frame = frame
        self.show_variables()

    localsviewer = None
    globalsviewer = None

    def show_locals(self):
        lv = self.localsviewer
        if self.vlocals.get():
            if not lv:
                self.localsviewer = StackViewer.NamespaceViewer(
                    self.flocals, "Locals")
        else:
            if lv:
                self.localsviewer = None
                lv.close()
                self.flocals['height'] = 1
        self.show_variables()

    def show_globals(self):
        gv = self.globalsviewer
        if self.vglobals.get():
            if not gv:
                self.globalsviewer = StackViewer.NamespaceViewer(
                    self.fglobals, "Globals")
        else:
            if gv:
                self.globalsviewer = None
                gv.close()
                self.fglobals['height'] = 1
        self.show_variables()

    def show_variables(self, force=0):
        lv = self.localsviewer
        gv = self.globalsviewer
        frame = self.frame
        if not frame:
            ldict = gdict = None
        else:
            ldict = frame.f_locals
            gdict = frame.f_globals
            if lv and gv and ldict is gdict:
                ldict = None
        if lv:
            lv.load_dict(ldict, force)
        if gv:
            gv.load_dict(gdict, force)

    def set_breakpoint_here(self, edit):
        text = edit.text
        filename = edit.io.filename
        if not filename:
            text.bell()
            return
        lineno = int(float(text.index("insert")))
        msg = self.set_break(filename, lineno)
        if msg:
            text.bell()
            return
        text.tag_add("BREAK", "insert linestart", "insert lineend +1char")

    # A literal copy of Bdb.set_break() without the print statement at the end
    def set_break(self, filename, lineno, temporary=0, cond = None):
        import linecache # Import as late as possible
        line = linecache.getline(filename, lineno)
        if not line:
                return 'That line does not exist!'
        if not self.breaks.has_key(filename):
                self.breaks[filename] = []
        list = self.breaks[filename]
        if not lineno in list:
                list.append(lineno)
        bp = bdb.Breakpoint(filename, lineno, temporary, cond)
