#! /usr/bin/env python

import os
import sys
import string
import getopt
import re

import linecache
from code import InteractiveInterpreter

from Tkinter import *
import tkMessageBox

from EditorWindow import EditorWindow, fixwordbreaks
from FileList import FileList
from ColorDelegator import ColorDelegator
from UndoDelegator import UndoDelegator
from OutputWindow import OutputWindow
from IdleConf import idleconf
import idlever

# We need to patch linecache.checkcache, because we don't want it
# to throw away our <pyshell#...> entries.
# Rather than repeating its code here, we save those entries,
# then call the original function, and then restore the saved entries.
def linecache_checkcache(orig_checkcache=linecache.checkcache):
    cache = linecache.cache
    save = {}
    for filename in cache.keys():
        if filename[:1] + filename[-1:] == '<>':
            save[filename] = cache[filename]
    orig_checkcache()
    cache.update(save)
linecache.checkcache = linecache_checkcache


# Note: <<newline-and-indent>> event is defined in AutoIndent.py

#$ event <<plain-newline-and-indent>>
#$ win <Control-j>
#$ unix <Control-j>

#$ event <<beginning-of-line>>
#$ win <Control-a>
#$ win <Home>
#$ unix <Control-a>
#$ unix <Home>

#$ event <<history-next>>
#$ win <Alt-n>
#$ unix <Alt-n>

#$ event <<history-previous>>
#$ win <Alt-p>
#$ unix <Alt-p>

#$ event <<interrupt-execution>>
#$ win <Control-c>
#$ unix <Control-c>

#$ event <<end-of-file>>
#$ win <Control-d>
#$ unix <Control-d>

#$ event <<open-stack-viewer>>

#$ event <<toggle-debugger>>


class PyShellEditorWindow(EditorWindow):

    # Regular text edit window when a shell is present
    # XXX ought to merge with regular editor window

    def __init__(self, *args):
        apply(EditorWindow.__init__, (self,) + args)
        self.text.bind("<<set-breakpoint-here>>", self.set_breakpoint_here)
        self.text.bind("<<open-python-shell>>", self.flist.open_shell)

    rmenu_specs = [
        ("Set breakpoint here", "<<set-breakpoint-here>>"),
    ]

    def set_breakpoint_here(self, event=None):
        if not self.flist.pyshell or not self.flist.pyshell.interp.debugger:
            self.text.bell()
            return
        self.flist.pyshell.interp.debugger.set_breakpoint_here(self)


class PyShellFileList(FileList):

    # File list when a shell is present

    EditorWindow = PyShellEditorWindow

    pyshell = None

    def open_shell(self, event=None):
        if self.pyshell:
            self.pyshell.wakeup()
        else:
            self.pyshell = PyShell(self)
            self.pyshell.begin()
        return self.pyshell


class ModifiedColorDelegator(ColorDelegator):

    # Colorizer for the shell window itself

    def recolorize_main(self):
        self.tag_remove("TODO", "1.0", "iomark")
        self.tag_add("SYNC", "1.0", "iomark")
        ColorDelegator.recolorize_main(self)

    tagdefs = ColorDelegator.tagdefs.copy()
    cconf = idleconf.getsection('Colors')

    tagdefs.update({
        "stdin": cconf.getcolor("stdin"),
        "stdout": cconf.getcolor("stdout"),
        "stderr": cconf.getcolor("stderr"),
        "console": cconf.getcolor("console"),
        "ERROR": cconf.getcolor("ERROR"),
	None: cconf.getcolor("normal"),
    })


class ModifiedUndoDelegator(UndoDelegator):

    # Forbid insert/delete before the I/O mark

    def insert(self, index, chars, tags=None):
        try:
            if self.delegate.compare(index, "<", "iomark"):
                self.delegate.bell()
                return
        except TclError:
            pass
        UndoDelegator.insert(self, index, chars, tags)

    def delete(self, index1, index2=None):
        try:
            if self.delegate.compare(index1, "<", "iomark"):
                self.delegate.bell()
                return
        except TclError:
            pass
        UndoDelegator.delete(self, index1, index2)

class ModifiedInterpreter(InteractiveInterpreter):

    def __init__(self, tkconsole):
        self.tkconsole = tkconsole
        locals = sys.modules['__main__'].__dict__
        InteractiveInterpreter.__init__(self, locals=locals)

    gid = 0

    def execsource(self, source):
        # Like runsource() but assumes complete exec source
        filename = self.stuffsource(source)
        self.execfile(filename, source)

    def execfile(self, filename, source=None):
        # Execute an existing file
        if source is None:
            source = open(filename, "r").read()
        try:
            code = compile(source, filename, "exec")
        except (OverflowError, SyntaxError):
            self.tkconsole.resetoutput()
            InteractiveInterpreter.showsyntaxerror(self, filename)
        else:
            self.runcode(code)

    def runsource(self, source):
        # Extend base class to stuff the source in the line cache first
        filename = self.stuffsource(source)
        self.more = 0
        return InteractiveInterpreter.runsource(self, source, filename)

    def stuffsource(self, source):
        # Stuff source in the filename cache
        filename = "<pyshell#%d>" % self.gid
        self.gid = self.gid + 1
        lines = string.split(source, "\n")
        linecache.cache[filename] = len(source)+1, 0, lines, filename
        return filename

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
        if char and char in string.letters + string.digits + "_":
            text.tag_add("ERROR", pos + " wordstart", pos)
        self.tkconsole.resetoutput()
        self.write("SyntaxError: %s\n" % str(msg))

    def unpackerror(self):
        type, value, tb = sys.exc_info()
        ok = type is SyntaxError
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
        self.checklinecache()
        InteractiveInterpreter.showtraceback(self)

    def checklinecache(self):
        c = linecache.cache
        for key in c.keys():
            if key[:1] + key[-1:] != "<>":
                del c[key]

    debugger = None

    def setdebugger(self, debugger):
        self.debugger = debugger

    def getdebugger(self):
        return self.debugger

    def runcode(self, code):
        # Override base class method
        debugger = self.debugger
        try:
            self.tkconsole.beginexecuting()
            try:
                if debugger:
                    debugger.run(code, self.locals)
                else:
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
                    if self.tkconsole.getvar("<<toggle-jit-stack-viewer>>"):
                        self.tkconsole.open_stack_viewer()
            except:
                self.showtraceback()
                if self.tkconsole.getvar("<<toggle-jit-stack-viewer>>"):
                    self.tkconsole.open_stack_viewer()

        finally:
            self.tkconsole.endexecuting()

    def write(self, s):
        # Override base class write
        self.tkconsole.console.write(s)


class PyShell(OutputWindow):

    shell_title = "Python Shell"

    # Override classes
    ColorDelegator = ModifiedColorDelegator
    UndoDelegator = ModifiedUndoDelegator

    # Override menu bar specs
    menu_specs = PyShellEditorWindow.menu_specs[:]
    menu_specs.insert(len(menu_specs)-2, ("debug", "_Debug"))

    # New classes
    from IdleHistory import History

    def __init__(self, flist=None):
        self.interp = ModifiedInterpreter(self)
        if flist is None:
            root = Tk()
            fixwordbreaks(root)
            root.withdraw()
            flist = PyShellFileList(root)

        OutputWindow.__init__(self, flist, None, None)

        import __builtin__
        __builtin__.quit = __builtin__.exit = "To exit, type Ctrl-D."

        self.auto = self.extensions["AutoIndent"] # Required extension
        self.auto.config(usetabs=1, indentwidth=8, context_use_ps1=1)

        text = self.text
        text.configure(wrap="char")
        text.bind("<<newline-and-indent>>", self.enter_callback)
        text.bind("<<plain-newline-and-indent>>", self.linefeed_callback)
        text.bind("<<interrupt-execution>>", self.cancel_callback)
        text.bind("<<beginning-of-line>>", self.home_callback)
        text.bind("<<end-of-file>>", self.eof_callback)
        text.bind("<<open-stack-viewer>>", self.open_stack_viewer)
        text.bind("<<toggle-debugger>>", self.toggle_debugger)
        text.bind("<<open-python-shell>>", self.flist.open_shell)
        text.bind("<<toggle-jit-stack-viewer>>", self.toggle_jit_stack_viewer)

        self.save_stdout = sys.stdout
        self.save_stderr = sys.stderr
        self.save_stdin = sys.stdin
        sys.stdout = PseudoFile(self, "stdout")
        sys.stderr = PseudoFile(self, "stderr")
        sys.stdin = self
        self.console = PseudoFile(self, "console")

        self.history = self.History(self.text)

    reading = 0
    executing = 0
    canceled = 0
    endoffile = 0

    def toggle_debugger(self, event=None):
        if self.executing:
            tkMessageBox.showerror("Don't debug now",
                "You can only toggle the debugger when idle",
                master=self.text)
            self.set_debugger_indicator()
            return "break"
        else:
            db = self.interp.getdebugger()
            if db:
                self.close_debugger()
            else:
                self.open_debugger()

    def set_debugger_indicator(self):
        db = self.interp.getdebugger()
        self.setvar("<<toggle-debugger>>", not not db)

    def toggle_jit_stack_viewer( self, event=None):
        pass # All we need is the variable

    def close_debugger(self):
        db = self.interp.getdebugger()
        if db:
            self.interp.setdebugger(None)
            db.close()
            self.resetoutput()
            self.console.write("[DEBUG OFF]\n")
            sys.ps1 = ">>> "
            self.showprompt()
        self.set_debugger_indicator()

    def open_debugger(self):
        import Debugger
        self.interp.setdebugger(Debugger.Debugger(self))
        sys.ps1 = "[DEBUG ON]\n>>> "
        self.showprompt()
        self.set_debugger_indicator()

    def beginexecuting(self):
        # Helper for ModifiedInterpreter
        self.resetoutput()
        self.executing = 1
        ##self._cancel_check = self.cancel_check
        ##sys.settrace(self._cancel_check)

    def endexecuting(self):
        # Helper for ModifiedInterpreter
        ##sys.settrace(None)
        ##self._cancel_check = None
        self.executing = 0
        self.canceled = 0

    def close(self):
        # Extend base class method
        if self.executing:
            # XXX Need to ask a question here
            if not tkMessageBox.askokcancel(
                "Kill?",
                "The program is still running; do you want to kill it?",
                default="ok",
                master=self.text):
                return "cancel"
            self.canceled = 1
            if self.reading:
                self.top.quit()
            return "cancel"
        return PyShellEditorWindow.close(self)

    def _close(self):
        self.close_debugger()
        # Restore std streams
        sys.stdout = self.save_stdout
        sys.stderr = self.save_stderr
        sys.stdin = self.save_stdin
        # Break cycles
        self.interp = None
        self.console = None
        self.auto = None
        self.flist.pyshell = None
        self.history = None
        OutputWindow._close(self) # Really EditorWindow._close

    def ispythonsource(self, filename):
        # Override this so EditorWindow never removes the colorizer
        return 1

    def short_title(self):
        return self.shell_title

    COPYRIGHT = \
              'Type "copyright", "credits" or "license" for more information.'

    def begin(self):
        self.resetoutput()
        self.write("Python %s on %s\n%s\nIDLE %s -- press F1 for help\n" %
                   (sys.version, sys.platform, self.COPYRIGHT,
                    idlever.IDLE_VERSION))
        try:
            sys.ps1
        except AttributeError:
            sys.ps1 = ">>> "
        self.showprompt()
        import Tkinter
        Tkinter._default_root = None

    def interact(self):
        self.begin()
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

    def isatty(self):
        return 1

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
            self.auto.auto_indent(event)
        return "break"

    def enter_callback(self, event):
        if self.executing and not self.reading:
            return # Let the default binding (insert '\n') take over
        # If some text is selected, recall the selection
        # (but only if this before the I/O mark)
        try:
            sel = self.text.get("sel.first", "sel.last")
            if sel:
                if self.text.compare("sel.last", "<=", "iomark"):
                    self.recall(sel)
                    return "break"
        except:
            pass
        # If we're strictly before the line containing iomark, recall
        # the current line, less a leading prompt, less leading or
        # trailing whitespace
        if self.text.compare("insert", "<", "iomark linestart"):
            # Check if there's a relevant stdin range -- if so, use it
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
        # If we're in the current input and there's only whitespace
        # beyond the cursor, erase that whitespace first
        s = self.text.get("insert", "end-1c")
        if s and not string.strip(s):
            self.text.delete("insert", "end-1c")
        # If we're in the current input before its last line,
        # insert a newline right at the insert point
        if self.text.compare("insert", "<", "end-1c linestart"):
            self.auto.auto_indent(event)
            return "break"
        # We're in the last line; append a newline and submit it
        self.text.mark_set("insert", "end-1c")
        if self.reading:
            self.text.insert("insert", "\n")
            self.text.see("insert")
        else:
            self.auto.auto_indent(event)
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

    def open_stack_viewer(self, event=None):
        try:
            sys.last_traceback
        except:
            tkMessageBox.showerror("No stack trace",
                "There is no stack trace yet.\n"
                "(sys.last_traceback is not defined)",
                master=self.text)
            return
        from StackViewer import StackBrowser
        sv = StackBrowser(self.root, self.flist)

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

    def write(self, s, tags=()):
        self.text.mark_gravity("iomark", "right")
        OutputWindow.write(self, s, tags, "iomark")
        self.text.mark_gravity("iomark", "left")
        if self.canceled:
            self.canceled = 0
            raise KeyboardInterrupt

class PseudoFile:

    def __init__(self, shell, tags):
        self.shell = shell
        self.tags = tags

    def write(self, s):
        self.shell.write(s, self.tags)

    def writelines(self, l):
        map(self.write, l)

    def flush(self):
        pass

    def isatty(self):
        return 1


usage_msg = """\
usage: idle.py [-c command] [-d] [-e] [-s] [-t title] [arg] ...

-c command  run this command
-d          enable debugger
-e          edit mode; arguments are files to be edited
-s          run $IDLESTARTUP or $PYTHONSTARTUP before anything else
-t title    set title of shell window

When neither -c nor -e is used, and there are arguments, and the first
argument is not '-', the first argument is run as a script.  Remaining
arguments are arguments to the script or to the command run by -c.
"""

def main():
    cmd = None
    edit = 0
    debug = 0
    startup = 0

    try:
        opts, args = getopt.getopt(sys.argv[1:], "c:deist:")
    except getopt.error, msg:
        sys.stderr.write("Error: %s\n" % str(msg))
        sys.stderr.write(usage_msg)
        sys.exit(2)

    for o, a in opts:
        if o == '-c':
            cmd = a
        if o == '-d':
            debug = 1
        if o == '-e':
            edit = 1
        if o == '-s':
            startup = 1
        if o == '-t':
            PyShell.shell_title = a

    for i in range(len(sys.path)):
        sys.path[i] = os.path.abspath(sys.path[i])

    pathx = []
    if edit:
        for filename in args:
            pathx.append(os.path.dirname(filename))
    elif args and args[0] != "-":
        pathx.append(os.path.dirname(args[0]))
    else:
        pathx.append(os.curdir)
    for dir in pathx:
        dir = os.path.abspath(dir)
        if not dir in sys.path:
            sys.path.insert(0, dir)

    global flist, root
    root = Tk(className="Idle")
    fixwordbreaks(root)
    root.withdraw()
    flist = PyShellFileList(root)

    if edit:
        for filename in args:
            flist.open(filename)
    else:
        if cmd:
            sys.argv = ["-c"] + args
        else:
            sys.argv = args or [""]


    shell = PyShell(flist)
    interp = shell.interp
    flist.pyshell = shell

    if startup:
        filename = os.environ.get("IDLESTARTUP") or \
                   os.environ.get("PYTHONSTARTUP")
        if filename and os.path.isfile(filename):
            interp.execfile(filename)

    if debug:
        shell.open_debugger()
    if cmd:
        interp.execsource(cmd)
    elif not edit and args and args[0] != "-":
        interp.execfile(args[0])

    shell.begin()
    root.mainloop()
    root.destroy()


if __name__ == "__main__":
    main()
