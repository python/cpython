#! /usr/bin/env python

import os
import sys
import string

import linecache
from code import InteractiveInterpreter

from Tkinter import *
import tkMessageBox

from EditorWindow import fixwordbreaks
from FileList import FileList, MultiEditorWindow, MultiIOBinding
from ColorDelegator import ColorDelegator


class ModifiedIOBinding(MultiIOBinding):

    def defaultfilename(self, mode="open"):
        if self.filename:
            return MultiIOBinding.defaultfilename(self, mode)
        else:
            try:
                pwd = os.getcwd()
            except os.error:
                pwd = ""
            return pwd, ""

    def open(self, event):
        # Override base class method -- don't allow reusing this window
        filename = self.askopenfile()
        if filename:
            self.flist.open(filename)
        return "break"

    def maybesave(self):
        # Override base class method -- don't ask any questions
        if self.text.get_saved():
            return "yes"
        else:
            return "no"


class ModifiedColorDelegator(ColorDelegator):
    
    def recolorize_main(self):
        self.tag_remove("TODO", "1.0", "iomark")
        self.tag_add("SYNC", "1.0", "iomark")
        ColorDelegator.recolorize_main(self)


class ModifiedInterpreter(InteractiveInterpreter):
    
    def __init__(self, tkconsole):
        self.tkconsole = tkconsole
        InteractiveInterpreter.__init__(self)

    gid = 0

    def runsource(self, source):
        # Extend base class to stuff the source in the line cache
        filename = "<console#%d>" % self.gid
        self.gid = self.gid + 1
        lines = string.split(source, "\n")
        linecache.cache[filename] = len(source)+1, 0, lines, filename
        self.more = 0
        return InteractiveInterpreter.runsource(self, source, filename)

    def showsyntaxerror(self, filename=None):
        # Extend base class to color the offending position
        # (instead of printing it and pointing at it with a caret)
        text = self.tkconsole.text
        stuff = self.unpackerror()
        if not stuff:
            self.tkconsole.resetoutput()
            InteractiveInterpreter.showsyntaxerror(self, filename)
            return
        msg, lineno, offset, line = stuff
        if lineno == 1:
            pos = "iomark + %d chars" % (offset-1)
        else:
            pos = "iomark linestart + %d lines + %d chars" % (lineno-1,
                                                              offset-1)
        text.tag_add("ERROR", pos)
        text.see(pos)
        char = text.get(pos)
        if char in string.letters + string.digits + "_":
            text.tag_add("ERROR", pos + " wordstart", pos)
        self.tkconsole.resetoutput()
        self.write("SyntaxError: %s\n" % str(msg))

    def unpackerror(self):
        type, value, tb = sys.exc_info()
        ok = type == SyntaxError
        if ok:
            try:
                msg, (dummy_filename, lineno, offset, line) = value
            except:
                ok = 0
        if ok:
            return msg, lineno, offset, line
        else:
            return None

    def showtraceback(self):
        # Extend base class method to reset output properly
        text = self.tkconsole.text
        self.tkconsole.resetoutput()
        InteractiveInterpreter.showtraceback(self)

    def runcode(self, code):
        # Override base class method
        try:
            self.tkconsole.beginexecuting()
            try:
                exec code in self.locals
            except SystemExit:
                if tkMessageBox.askyesno(
                    "Exit?",
                    "Do you want to exit altogether?",
                    default="yes",
                    master=self.tkconsole.text):
                    raise
                else:
                    self.showtraceback()
            except:
                self.showtraceback()
        finally:
            self.tkconsole.endexecuting()
 
    def write(self, s):
        # Override base class write
        self.tkconsole.console.write(s)
       

class PyShell(MultiEditorWindow):

    # Override classes
    ColorDelegator = ModifiedColorDelegator
    IOBinding = ModifiedIOBinding
    
    # New class
    from History import History

    def __init__(self, flist=None):
        self.interp = ModifiedInterpreter(self)
        if flist is None:
            root = Tk()
            fixwordbreaks(root)
            root.withdraw()
            flist = FileList(root)

        MultiEditorWindow.__init__(self, flist, None, None)
        self.config_colors()

        import __builtin__
        __builtin__.quit = __builtin__.exit = "To exit, type Ctrl-D."

        self.auto.config(prefertabs=1)

        text = self.text
        text.bind("<<newline-and-indent>>", self.enter_callback)
        text.bind("<<plain-newline-and-indent>>", self.linefeed_callback)
        text.bind("<<interrupt-execution>>", self.cancel_callback)
        text.bind("<<beginning-of-line>>", self.home_callback)
        text.bind("<<end-of-file>>", self.eof_callback)

        sys.stdout = PseudoFile(self, "stdout")
        ##sys.stderr = PseudoFile(self, "stderr")
        sys.stdin = self
        self.console = PseudoFile(self, "console")

        self.history = self.History(self.text)

    tagdefs = {
        ##"stdin":   {"background": "yellow"},
        "stdout":  {"foreground": "blue"},
        "stderr":  {"foreground": "#007700"},
        "console": {"foreground": "red"},
        "ERROR":   {"background": "#FF7777"},
        None:      {"foreground": "purple"}, # default
    }

    def config_colors(self):
        for tag, cnf in self.tagdefs.items():
            if cnf:
                if not tag:
                    apply(self.text.configure, (), cnf)
                else:
                    apply(self.text.tag_configure, (tag,), cnf)

    reading = 0
    executing = 0
    canceled = 0
    endoffile = 0

    def beginexecuting(self):
        # Helper for ModifiedInterpreter
        self.resetoutput()
        self.executing = 1
        self._cancel_check = self.cancel_check
        ##sys.settrace(self._cancel_check)

    def endexecuting(self):
        # Helper for ModifiedInterpreter
        sys.settrace(None)
        self.executing = 0
        self.canceled = 0

    def close(self):
        # Extend base class method
        if self.executing:
            # XXX Need to ask a question here
            if not tkMessageBox.askokcancel(
                "Cancel?",
                "The program is still running; do you want to cancel it?",
                default="ok",
                master=self.text):
                return "cancel"
            self.canceled = 1
            if self.reading:
                self.top.quit()
            return "cancel"
        reply = MultiEditorWindow.close(self)
        if reply != "cancel":
            # Restore std streams
            sys.stdout = sys.__stdout__
            sys.stderr = sys.__stderr__
            sys.stdin = sys.__stdin__
            # Break cycles
            self.interp = None
            self.console = None
        return reply

    def ispythonsource(self, filename):
        # Override this so EditorWindow never removes the colorizer
        return 1

    def saved_change_hook(self):
        # Override this to get the title right
        title = "Python Shell"
        if self.io.filename:
            title = title + ": " + self.io.filename
            if not self.undo.get_saved():
                title = title + " *"
        self.top.wm_title(title)

    def interact(self):
        self.resetoutput()
        self.write("Python %s on %s\n%s\n" %
                   (sys.version, sys.platform, sys.copyright))
        try:
            sys.ps1
        except AttributeError:
            sys.ps1 = ">>> "
        self.showprompt()
        import Tkinter
        Tkinter._default_root = None
        self.top.mainloop()

    def readline(self):
        save = self.reading
        try:
            self.reading = 1
            self.top.mainloop()
        finally:
            self.reading = save
        line = self.text.get("iomark", "end-1c")
        self.resetoutput()
        if self.canceled:
            self.canceled = 0
            raise KeyboardInterrupt
        if self.endoffile:
            self.endoffile = 0
            return ""
        return line

    def cancel_callback(self, event):
        try:
            if self.text.compare("sel.first", "!=", "sel.last"):
                return # Active selection -- always use default binding
        except:
            pass
        if not (self.executing or self.reading):
            self.resetoutput()
            self.write("KeyboardInterrupt\n")
            self.showprompt()
            return "break"
        self.endoffile = 0
        self.canceled = 1
        if self.reading:
            self.top.quit()
        return "break"

    def eof_callback(self, event):
        if self.executing and not self.reading:
            return # Let the default binding (delete next char) take over
        if not (self.text.compare("iomark", "==", "insert") and
                self.text.compare("insert", "==", "end-1c")):
            return # Let the default binding (delete next char) take over
        if not self.executing:
##             if not tkMessageBox.askokcancel(
##                 "Exit?",
##                 "Are you sure you want to exit?",
##                 default="ok", master=self.text):
##                 return "break"
            self.resetoutput()
            self.close()
        else:
            self.canceled = 0
            self.endoffile = 1
            self.top.quit()
        return "break"

    def home_callback(self, event):
        if event.state != 0 and event.keysym == "Home":
            return # <Modifier-Home>; fall back to class binding
        if self.text.compare("iomark", "<=", "insert") and \
           self.text.compare("insert linestart", "<=", "iomark"):
            self.text.mark_set("insert", "iomark")
            self.text.tag_remove("sel", "1.0", "end")
            self.text.see("insert")
            return "break"

    def linefeed_callback(self, event):
        # Insert a linefeed without entering anything (still autoindented)
        if self.reading:
            self.text.insert("insert", "\n")
            self.text.see("insert")
        else:
            self.auto.autoindent(event)
        return "break"

    def enter_callback(self, event):
        if self.executing and not self.reading:
            return # Let the default binding (insert '\n') take over
        # If some text is selected, recall the selection
        try:
            sel = self.text.get("sel.first", "sel.last")
            if sel:
                self.recall(sel)
                return "break"
        except:
            pass
        # If we're strictly before the line containing iomark, recall
        # the current line, less a leading prompt, less leading or
        # trailing whitespace
        if self.text.compare("insert", "<", "iomark linestart"):
            # Check if there's a relevant stdin mark -- if so, use it
            prev = self.text.tag_prevrange("stdin", "insert")
            if prev and self.text.compare("insert", "<", prev[1]):
                self.recall(self.text.get(prev[0], prev[1]))
                return "break"
            next = self.text.tag_nextrange("stdin", "insert")
            if next and self.text.compare("insert lineend", ">=", next[0]):
                self.recall(self.text.get(next[0], next[1]))
                return "break"
            # No stdin mark -- just get the current line
            self.recall(self.text.get("insert linestart", "insert lineend"))
            return "break"
        # If we're anywhere in the current input (including in the
        # prompt) but not at the very end, move the cursor to the end.
        if self.text.compare("insert", "<", "end-1c"):
            self.text.mark_set("insert", "end-1c")
            self.text.see("insert")
            return "break"
        # OK, we're already at the end -- insert a newline and run it.
        if self.reading:
            self.text.insert("insert", "\n")
            self.text.see("insert")
        else:
            self.auto.autoindent(event)
        self.text.tag_add("stdin", "iomark", "end-1c")
        self.text.update_idletasks()
        if self.reading:
            self.top.quit() # Break out of recursive mainloop() in raw_input()
        else:
            self.runit()
        return "break"

    def recall(self, s):
        if self.history:
            self.history.recall(s)

    def runit(self):
        line = self.text.get("iomark", "end-1c")
        # Strip off last newline and surrounding whitespace.
        # (To allow you to hit return twice to end a statement.)
        i = len(line)
        while i > 0 and line[i-1] in " \t":
            i = i-1
        if i > 0 and line[i-1] == "\n":
            i = i-1
        while i > 0 and line[i-1] in " \t":
            i = i-1
        line = line[:i]
        more = self.interp.runsource(line)
        if not more:
            self.showprompt()

    def cancel_check(self, frame, what, args,
                     dooneevent=tkinter.dooneevent,
                     dontwait=tkinter.DONT_WAIT):
        # Hack -- use the debugger hooks to be able to handle events
        # and interrupt execution at any time.
        # This slows execution down quite a bit, so you may want to
        # disable this (by not calling settrace() in runcode() above)
        # for full-bore (uninterruptable) speed.
        # XXX This should become a user option.
        if self.canceled:
            return
        dooneevent(dontwait)
        if self.canceled:
            self.canceled = 0
            raise KeyboardInterrupt
        return self._cancel_check

    def showprompt(self):
        self.resetoutput()
        try:
            s = str(sys.ps1)
        except:
            s = ""
        self.console.write(s)
        self.text.mark_set("insert", "end-1c")

    def resetoutput(self):
        source = self.text.get("iomark", "end-1c")
        if self.history:
            self.history.history_store(source)
        if self.text.get("end-2c") != "\n":
            self.text.insert("end-1c", "\n")
        self.text.mark_set("iomark", "end-1c")
        sys.stdout.softspace = 0

    def write(self, s):
        # Overrides base class write
        self.console.write(s)

class PseudoFile:

    def __init__(self, interp, tags):
        self.interp = interp
        self.text = interp.text
        self.tags = tags

    def write(self, s):
        self.text.mark_gravity("iomark", "right")
        self.text.insert("iomark", str(s), self.tags)
        self.text.mark_gravity("iomark", "left")
        self.text.see("iomark")
        self.text.update()
        if self.interp.canceled:
            self.interp.canceled = 0
            raise KeyboardInterrupt

    def writelines(self, l):
        map(self.write, l)


def main():
    global flist, root
    root = Tk()
    fixwordbreaks(root)
    root.withdraw()
    flist = FileList(root)
    if sys.argv[1:]:
        for filename in sys.argv[1:]:
            flist.open(filename)
    t = PyShell(flist)
    t.interact()

if __name__ == "__main__":
    main()
