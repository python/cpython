#! /usr/bin/env python

import os
import os.path
import sys
import string
import getopt
import re
import socket
import time
import traceback
import types
import exceptions

import linecache
from code import InteractiveInterpreter

from Tkinter import *
import tkMessageBox

from EditorWindow import EditorWindow, fixwordbreaks
from FileList import FileList
from ColorDelegator import ColorDelegator
from UndoDelegator import UndoDelegator
from OutputWindow import OutputWindow
from configHandler import idleConf
import idlever

import rpc
import RemoteDebugger

IDENTCHARS = string.ascii_letters + string.digits + "_"

# Change warnings module to write to sys.__stderr__
try:
    import warnings
except ImportError:
    pass
else:
    def idle_showwarning(message, category, filename, lineno):
        file = sys.__stderr__
        file.write(warnings.formatwarning(message, category, filename, lineno))
    warnings.showwarning = idle_showwarning

def extended_linecache_checkcache(orig_checkcache=linecache.checkcache):
    """Extend linecache.checkcache to preserve the <pyshell#...> entries

    Rather than repeating the linecache code, patch it to save the pyshell#
    entries, call the original linecache.checkcache(), and then restore the
    saved entries.  Assigning the orig_checkcache keyword arg freezes its value
    at definition time to the (original) method linecache.checkcache(), i.e.
    makes orig_checkcache lexical.

    """
    cache = linecache.cache
    save = {}
    for filename in cache.keys():
        if filename[:1] + filename[-1:] == '<>':
            save[filename] = cache[filename]
    orig_checkcache()
    cache.update(save)
    
# Patch linecache.checkcache():
linecache.checkcache = extended_linecache_checkcache


class PyShellEditorWindow(EditorWindow):
    "Regular text edit window when a shell is present"

    def __init__(self, *args):
        self.breakpoints = []
        apply(EditorWindow.__init__, (self,) + args)
        self.text.bind("<<set-breakpoint-here>>", self.set_breakpoint_here)
        self.text.bind("<<clear-breakpoint-here>>", self.clear_breakpoint_here)
        self.text.bind("<<open-python-shell>>", self.flist.open_shell)

        self.breakpointPath = os.path.join(idleConf.GetUserCfgDir(),
                                           'breakpoints.lst')
        # whenever a file is changed, restore breakpoints
        if self.io.filename: self.restore_file_breaks()
        def filename_changed_hook(old_hook=self.io.filename_change_hook,
                                  self=self):
            self.restore_file_breaks()
            old_hook()
        self.io.set_filename_change_hook(filename_changed_hook)

    rmenu_specs = [("Set Breakpoint", "<<set-breakpoint-here>>"),
                   ("Clear Breakpoint", "<<clear-breakpoint-here>>")]

    def set_breakpoint(self, lineno):
        text = self.text
        filename = self.io.filename
        text.tag_add("BREAK", "%d.0" % lineno, "%d.0" % (lineno+1))
        try:
            i = self.breakpoints.index(lineno)
        except ValueError:  # only add if missing, i.e. do once
            self.breakpoints.append(lineno)
        try:    # update the subprocess debugger
            debug = self.flist.pyshell.interp.debugger
            debug.set_breakpoint_here(filename, lineno)
        except: # but debugger may not be active right now....
            pass

    def set_breakpoint_here(self, event=None):
        text = self.text
        filename = self.io.filename
        if not filename:
            text.bell()
            return
        lineno = int(float(text.index("insert")))
        self.set_breakpoint(lineno)

    def clear_breakpoint_here(self, event=None):
        text = self.text
        filename = self.io.filename
        if not filename:
            text.bell()
            return
        lineno = int(float(text.index("insert")))
        try:
            self.breakpoints.remove(lineno)
        except:
            pass
        text.tag_remove("BREAK", "insert linestart",\
                        "insert lineend +1char")
        try:
            debug = self.flist.pyshell.interp.debugger
            debug.clear_breakpoint_here(filename, lineno)
        except:
            pass

    def clear_file_breaks(self):
        if self.breakpoints:
            text = self.text
            filename = self.io.filename
            if not filename:
                text.bell()
                return
            self.breakpoints = []
            text.tag_remove("BREAK", "1.0", END)
            try:
                debug = self.flist.pyshell.interp.debugger
                debug.clear_file_breaks(filename)
            except:
                pass

    def store_file_breaks(self):
        "Save breakpoints when file is saved"
        # XXX 13 Dec 2002 KBK Currently the file must be saved before it can
        #     be run.  The breaks are saved at that time.  If we introduce
        #     a temporary file save feature the save breaks functionality
        #     needs to be re-verified, since the breaks at the time the
        #     temp file is created may differ from the breaks at the last
        #     permanent save of the file.  A break introduced after a save
        #     will be effective,  but not persistent.  This is necessary to
        #     keep the saved breaks synched with the saved file.
        #
        #     Breakpoints are set as tagged ranges in the text.  Certain
        #     kinds of edits cause these ranges to be deleted: Inserting
        #     or deleting a line just before a breakpoint, and certain
        #     deletions prior to a breakpoint.  These issues need to be
        #     investigated and understood.  It's not clear if they are
        #     Tk issues or IDLE issues, or whether they can actually
        #     be fixed.  Since a modified file has to be saved before it is
        #     run, and since self.breakpoints (from which the subprocess
        #     debugger is loaded) is updated during the save, the visible
        #     breaks stay synched with the subprocess even if one of these
        #     unexpected breakpoint deletions occurs.
        breaks = self.breakpoints
        filename = self.io.filename
        try:
            lines = open(self.breakpointPath,"r").readlines()
        except IOError:
            lines = []
        new_file = open(self.breakpointPath,"w")
        for line in lines:
            if not line.startswith(filename + '='):
                new_file.write(line)
        self.update_breakpoints()
        breaks = self.breakpoints
        if breaks:
            new_file.write(filename + '=' + str(breaks) + '\n')
        new_file.close()

    def restore_file_breaks(self):
        self.text.update()   # this enables setting "BREAK" tags to be visible
        filename = self.io.filename
        if filename is None:
            return
        if os.path.isfile(self.breakpointPath):
            lines = open(self.breakpointPath,"r").readlines()
            for line in lines:
                if line.startswith(filename + '='):
                    breakpoint_linenumbers = eval(line[len(filename)+1:]) 
                    for breakpoint_linenumber in breakpoint_linenumbers:
                        self.set_breakpoint(breakpoint_linenumber)

    def update_breakpoints(self):
        "Retrieves all the breakpoints in the current window"
        text = self.text
        ranges = text.tag_ranges("BREAK")
        linenumber_list = self.ranges_to_linenumbers(ranges)
        self.breakpoints = linenumber_list

    def ranges_to_linenumbers(self, ranges):
        lines = []
        for index in range(0, len(ranges), 2):
            lineno = int(float(ranges[index]))
            end = int(float(ranges[index+1]))
            while lineno < end:
                lines.append(lineno)
                lineno += 1
        return lines

# XXX 13 Dec 2020 KBK Not used currently
#    def saved_change_hook(self):
#        "Extend base method - clear breaks if module is modified"
#        if not self.get_saved():
#            self.clear_file_breaks()
#        EditorWindow.saved_change_hook(self)

    def _close(self):
        "Extend base method - clear breaks when module is closed"
        self.clear_file_breaks()
        EditorWindow._close(self)
                                

class PyShellFileList(FileList):
    "Extend base class: file list when a shell is present"

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
    "Extend base class: colorizer for the shell window itself"
    
    def __init__(self):
        ColorDelegator.__init__(self)
        self.LoadTagDefs()

    def recolorize_main(self):
        self.tag_remove("TODO", "1.0", "iomark")
        self.tag_add("SYNC", "1.0", "iomark")
        ColorDelegator.recolorize_main(self)
    
    def LoadTagDefs(self):
        ColorDelegator.LoadTagDefs(self)
        theme = idleConf.GetOption('main','Theme','name')
        self.tagdefs.update({
            "stdin": {'background':None,'foreground':None},
            "stdout": idleConf.GetHighlight(theme, "stdout"),
            "stderr": idleConf.GetHighlight(theme, "stderr"),
            "console": idleConf.GetHighlight(theme, "console"),
            "ERROR": idleConf.GetHighlight(theme, "error"),
            None: idleConf.GetHighlight(theme, "normal"),
        })

class ModifiedUndoDelegator(UndoDelegator):
    "Extend base class: forbid insert/delete before the I/O mark"

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
        self.save_warnings_filters = None

    port = 8833
    rpcclt = None
    rpcpid = None

    def spawn_subprocess(self):         
        w = ['-W' + s for s in sys.warnoptions]        
        args = [self.find_executable()] + w \
             + ["-c", "__import__('run').main()", str(self.port)]
        self.rpcpid = os.spawnv(os.P_NOWAIT, args[0], args)

    def find_executable(self):
        if sys.platform == 'darwin' and sys.executable.count('.app'):
            # On Mac OS X, avoid calling sys.executable because it ignores
            # command-line options (sys.executable is an applet)
            #
            # Instead, find the executable by looking relative to
            # sys.prefix.
            executable = os.path.join(sys.prefix, 'Resources', 
                                      'Python.app', 'Contents',
                                      'MacOS', 'python')
            return executable
        else:
            return sys.executable 

    def start_subprocess(self):
        addr = ("localhost", self.port)
        self.spawn_subprocess()
        # Idle starts listening for connection on localhost
        for i in range(6):
            time.sleep(i)
            try:
                self.rpcclt = rpc.RPCClient(addr)
                break
            except socket.error, err:
                if i < 3:
                    print>>sys.__stderr__, ". ",
                else:
                    print>>sys.__stderr__,"\nIdle socket error: " + err[1]\
                                                    + ", retrying..."
        else:
            display_port_binding_error()
            return
        # Accept the connection from the Python execution server
        self.rpcclt.accept()
        self.rpcclt.register("stdin", self.tkconsole)
        self.rpcclt.register("stdout", self.tkconsole.stdout)
        self.rpcclt.register("stderr", self.tkconsole.stderr)
        self.rpcclt.register("flist", self.tkconsole.flist)
        self.transfer_path()
        self.poll_subprocess()

    def restart_subprocess(self):
        # close only the subprocess debugger
        debug = self.getdebugger()
        if debug:
            RemoteDebugger.close_subprocess_debugger(self.rpcclt)           
        # kill subprocess, spawn a new one, accept connection
        self.rpcclt.close()
        self.spawn_subprocess()
        self.rpcclt.accept()
        self.transfer_path()
        # restart remote debugger
        if debug:
            gui = RemoteDebugger.restart_subprocess_debugger(self.rpcclt)
            # reload remote debugger breakpoints for all PyShellEditWindows
            debug.load_breakpoints()

    def transfer_path(self):
        self.runcommand("""if 1:
        import sys as _sys
        _sys.path = %s
        del _sys
        \n""" % `sys.path`)

    active_seq = None

    def poll_subprocess(self):
        clt = self.rpcclt
        if clt is None:
            return
        response = clt.pollresponse(self.active_seq)
        # Reschedule myself in 50 ms
        self.tkconsole.text.after(50, self.poll_subprocess)
        if response:
            self.tkconsole.resetoutput()
            self.active_seq = None
            how, what = response
            file = self.tkconsole.console
            if how == "OK":
                if what is not None:
                    print >>file, `what`
            elif how == "EXCEPTION":
                mod, name, args, tb = what
                print >>file, 'Traceback (most recent call last):'
                while tb and tb[0][0] in ("run.py", "rpc.py"):
                    del tb[0]
                while tb and tb[-1][0] in ("run.py", "rpc.py"):
                    del tb[-1]
                for i in range(len(tb)):
                    fn, ln, nm, line = tb[i]
                    if not line and fn.startswith("<pyshell#"):
                        line = linecache.getline(fn, ln)
                        tb[i] = fn, ln, nm, line
                traceback.print_list(tb, file=file)
                # try to reinstantiate the exception, stuff in the args:
                try:
                    etype = eval(mod + '.' + name)
                    val = etype()
                    val.args = args
                except TypeError:  # string exception!
                    etype = name
                    val = args
                lines = traceback.format_exception_only(etype, val)
                for line in lines[:-1]:
                    traceback._print(file, line, '')
                traceback._print(file, lines[-1], '')
                if self.tkconsole.getvar("<<toggle-jit-stack-viewer>>"):
                    self.remote_stack_viewer()
            elif how == "ERROR":
                errmsg = "PyShell.ModifiedInterpreter: Subprocess ERROR:\n"
                print >>sys.__stderr__, errmsg, what
                print >>file, errmsg, what
            self.tkconsole.endexecuting()

    def kill_subprocess(self):
        clt = self.rpcclt
        self.rpcclt = None
        if clt is not None:
            clt.close()

    debugger = None

    def setdebugger(self, debugger):
        self.debugger = debugger

    def getdebugger(self):
        return self.debugger

    def remote_stack_viewer(self):
        import RemoteObjectBrowser
        oid = self.rpcclt.remotecall("exec", "stackviewer", ("flist",), {})
        if oid is None:
            self.tkconsole.root.bell()
            return
        item = RemoteObjectBrowser.StubObjectTreeItem(self.rpcclt, oid)
        from TreeWidget import ScrolledCanvas, TreeNode
        top = Toplevel(self.tkconsole.root)
        sc = ScrolledCanvas(top, bg="white", highlightthickness=0)
        sc.frame.pack(expand=1, fill="both")
        node = TreeNode(sc.canvas, None, item)
        node.expand()
        # XXX Should GC the remote tree when closing the window

    gid = 0

    def execsource(self, source):
        "Like runsource() but assumes complete exec source"
        filename = self.stuffsource(source)
        self.execfile(filename, source)

    def execfile(self, filename, source=None):
        "Execute an existing file"
        if source is None:
            source = open(filename, "r").read()
        try:
            code = compile(source, filename, "exec")
        except (OverflowError, SyntaxError):
            self.tkconsole.resetoutput()
            console = self.tkconsole.console
            print >>console, 'Traceback (most recent call last):'
            InteractiveInterpreter.showsyntaxerror(self, filename)
            self.tkconsole.showprompt()
        else:
            self.runcode(code)

    def runsource(self, source):
        "Extend base class method: Stuff the source in the line cache first"
        filename = self.stuffsource(source)
        self.more = 0
        self.save_warnings_filters = warnings.filters[:]
        warnings.filterwarnings(action="error", category=SyntaxWarning)
        if isinstance(source, types.UnicodeType):
            import IOBinding
            try:
                source = source.encode(IOBinding.encoding)
            except UnicodeError:
                self.tkconsole.resetoutput()
                self.write("Unsupported characters in input")
                return
        try:
            return InteractiveInterpreter.runsource(self, source, filename)
        finally:
            if self.save_warnings_filters is not None:
                warnings.filters[:] = self.save_warnings_filters
                self.save_warnings_filters = None

    def stuffsource(self, source):
        "Stuff source in the filename cache"
        filename = "<pyshell#%d>" % self.gid
        self.gid = self.gid + 1
        lines = source.split("\n")
        linecache.cache[filename] = len(source)+1, 0, lines, filename
        return filename
    
    def showsyntaxerror(self, filename=None):
        """Extend base class method: Add Colorizing

        Color the offending position instead of printing it and pointing at it
        with a caret.

        """
        text = self.tkconsole.text
        stuff = self.unpackerror()
        if stuff:
            msg, lineno, offset, line = stuff
            if lineno == 1:
                pos = "iomark + %d chars" % (offset-1)
            else:
                pos = "iomark linestart + %d lines + %d chars" % \
                      (lineno-1, offset-1)
            text.tag_add("ERROR", pos)
            text.see(pos)
            char = text.get(pos)
            if char and char in IDENTCHARS:
                text.tag_add("ERROR", pos + " wordstart", pos)
            self.tkconsole.resetoutput()
            self.write("SyntaxError: %s\n" % str(msg))
        else:
            self.tkconsole.resetoutput()
            InteractiveInterpreter.showsyntaxerror(self, filename)
        self.tkconsole.showprompt()

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
        "Extend base class method to reset output properly"
        self.tkconsole.resetoutput()
        self.checklinecache()
        InteractiveInterpreter.showtraceback(self)
        if self.tkconsole.getvar("<<toggle-jit-stack-viewer>>"):
            self.tkconsole.open_stack_viewer()

    def checklinecache(self):
        c = linecache.cache
        for key in c.keys():
            if key[:1] + key[-1:] != "<>":
                del c[key]

    def display_executing_dialog(self):
        tkMessageBox.showerror(
            "Already executing",
            "The Python Shell window is already executing a command; "
            "please wait until it is finished.",
            master=self.tkconsole.text)
        
    def runcommand(self, code):
        "Run the code without invoking the debugger"
        # The code better not raise an exception!
        if self.tkconsole.executing:
            self.display_executing_dialog()
            return 0
        if self.rpcclt:
            self.rpcclt.remotecall("exec", "runcode", (code,), {})
        else:
            exec code in self.locals
        return 1

    def runcode(self, code):
        "Override base class method"
        if self.tkconsole.executing:
            self.display_executing_dialog()
            return
        self.checklinecache()
        if self.save_warnings_filters is not None:
            warnings.filters[:] = self.save_warnings_filters
            self.save_warnings_filters = None
        debugger = self.debugger
        if not debugger and self.rpcclt is not None:
            self.tkconsole.beginexecuting()
            self.active_seq = self.rpcclt.asynccall("exec", "runcode",
                                                    (code,), {})
            return
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
            except:
                self.showtraceback()
        finally:
            self.tkconsole.endexecuting()

    def write(self, s):
        "Override base class method"
        self.tkconsole.console.write(s)

class PyShell(OutputWindow):

    shell_title = "Python Shell"

    # Override classes
    ColorDelegator = ModifiedColorDelegator
    UndoDelegator = ModifiedUndoDelegator

    # Override menus
    menu_specs = [
        ("file", "_File"),
        ("edit", "_Edit"),
        ("debug", "_Debug"),
        ("settings", "_Settings"),
        ("windows", "_Windows"),
        ("help", "_Help"),
    ]

    # New classes
    from IdleHistory import History

    def __init__(self, flist=None):
        self.interp = ModifiedInterpreter(self)
        if flist is None:
            root = Tk()
            fixwordbreaks(root)
            root.withdraw()
            flist = PyShellFileList(root)
        #
        OutputWindow.__init__(self, flist, None, None)
        #
        import __builtin__
        __builtin__.quit = __builtin__.exit = "To exit, type Ctrl-D."
        #
        self.config(usetabs=1, indentwidth=8, context_use_ps1=1)
        #
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
        #
        self.save_stdout = sys.stdout
        self.save_stderr = sys.stderr
        self.save_stdin = sys.stdin
        self.stdout = PseudoFile(self, "stdout")
        self.stderr = PseudoFile(self, "stderr")
        self.console = PseudoFile(self, "console")
        if not use_subprocess:
            sys.stdout = self.stdout
            sys.stderr = self.stderr
            sys.stdin = self
        #
        self.history = self.History(self.text)
        #
        if use_subprocess:
            self.interp.start_subprocess()

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
            if self.interp.rpcclt:
                RemoteDebugger.close_remote_debugger(self.interp.rpcclt)
            self.resetoutput()
            self.console.write("[DEBUG OFF]\n")
            sys.ps1 = ">>> "
            self.showprompt()
        self.set_debugger_indicator()

    def open_debugger(self):
        # XXX KBK 13Jun02 An RPC client always exists now? Open remote
        # debugger and return...dike the rest of this fcn and combine
        # with open_remote_debugger?
        if self.interp.rpcclt:
            return self.open_remote_debugger()
        import Debugger
        self.interp.setdebugger(Debugger.Debugger(self))
        sys.ps1 = "[DEBUG ON]\n>>> "
        self.showprompt()
        self.set_debugger_indicator()

    def open_remote_debugger(self):
        gui = RemoteDebugger.start_remote_debugger(self.interp.rpcclt, self)
        self.interp.setdebugger(gui)
        # Load all PyShellEditorWindow breakpoints:
        gui.load_breakpoints()
        sys.ps1 = "[DEBUG ON]\n>>> "
        self.showprompt()
        self.set_debugger_indicator()

    def beginexecuting(self):
        "Helper for ModifiedInterpreter"
        self.resetoutput()
        self.executing = 1
        ##self._cancel_check = self.cancel_check
        ##sys.settrace(self._cancel_check)

    def endexecuting(self):
        "Helper for ModifiedInterpreter"
        ##sys.settrace(None)
        ##self._cancel_check = None
        self.executing = 0
        self.canceled = 0
        self.showprompt()

    def close(self):
        "Extend EditorWindow.close()"
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
        return EditorWindow.close(self)

    def _close(self):
        "Extend EditorWindow._close(), shut down debugger and execution server"
        self.close_debugger()
        self.interp.kill_subprocess()
        # Restore std streams
        sys.stdout = self.save_stdout
        sys.stderr = self.save_stderr
        sys.stdin = self.save_stdin
        # Break cycles
        self.interp = None
        self.console = None
        self.flist.pyshell = None
        self.history = None
        EditorWindow._close(self)

    def ispythonsource(self, filename):
        "Override EditorWindow method: never remove the colorizer"
        return True

    def short_title(self):
        return self.shell_title

    COPYRIGHT = \
              'Type "copyright", "credits" or "license" for more information.'

    def begin(self):
        self.resetoutput()
        self.write("Python %s on %s\n%s\nGRPC IDLE Fork %s\n" %
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
        return True

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
        if self.reading:
            self.canceled = 1
            self.top.quit()
        elif (self.executing and self.interp.rpcclt and
              self.interp.rpcpid and hasattr(os, "kill")):
            try:
                from signal import SIGINT
            except ImportError:
                SIGINT = 2
            os.kill(self.interp.rpcpid, SIGINT)
        else:
            self.canceled = 1
        return "break"

    def eof_callback(self, event):
        if self.executing and not self.reading:
            return # Let the default binding (delete next char) take over
        if not (self.text.compare("iomark", "==", "insert") and
                self.text.compare("insert", "==", "end-1c")):
            return # Let the default binding (delete next char) take over
        if not self.executing:
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
            self.newline_and_indent_event(event)
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
        # If we're between the beginning of the line and the iomark, i.e.
        # in the prompt area, complain.
        if self.text.compare("insert", "<", "iomark"):
            self.text.bell()
            return "break"
        # If we're in the current input and there's only whitespace
        # beyond the cursor, erase that whitespace first
        s = self.text.get("insert", "end-1c")
        if s and not s.strip():
            self.text.delete("insert", "end-1c")
        # If we're in the current input before its last line,
        # insert a newline right at the insert point
        if self.text.compare("insert", "<", "end-1c linestart"):
            self.newline_and_indent_event(event)
            return "break"
        # We're in the last line; append a newline and submit it
        self.text.mark_set("insert", "end-1c")
        if self.reading:
            self.text.insert("insert", "\n")
            self.text.see("insert")
        else:
            self.newline_and_indent_event(event)
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
        if self.interp.rpcclt:
            return self.interp.remote_stack_viewer()
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
        self.set_line_and_column()
        self.io.reset_undo()

    def resetoutput(self):
        source = self.text.get("iomark", "end-1c")
        if self.history:
            self.history.history_store(source)
        if self.text.get("end-2c") != "\n":
            self.text.insert("end-1c", "\n")
        self.text.mark_set("iomark", "end-1c")
        self.set_line_and_column()
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
        self.softspace = 0

    def write(self, s):
        self.shell.write(s, self.tags)

    def writelines(self, l):
        map(self.write, l)

    def flush(self):
        pass

    def isatty(self):
        return True


usage_msg = """\

USAGE: idle  [-deis] [-t title] [file]*             
       idle  [-ds] [-t title] (-c cmd | -r file) [arg]*
       idle  [-ds] [-t title] - [arg]*
       
  -h         print this help message and exit

The following options will override the IDLE 'settings' configuration:

  -e         open an edit window
  -i         open a shell window

The following options imply -i and will open a shell:

  -c cmd     run the command in a shell, or
  -r file    run script from file

  -d         enable the debugger
  -s         run $IDLESTARTUP or $PYTHONSTARTUP before anything else
  -t title   set title of shell window

A default edit window will be bypassed when -c, -r, or - are used.

[arg]* are passed to the command (-c) or script (-r) in sys.argv[1:].

Examples:

idle
        Open an edit window or shell depending on IDLE's configuration.

idle foo.py foobar.py
        Edit the files, also open a shell if configured to start with shell.

idle -est "Baz" foo.py
        Run $IDLESTARTUP or $PYTHONSTARTUP, edit foo.py, and open a shell
        window with the title "Baz".

idle -c "import sys; print sys.argv" "foo"
        Open a shell window and run the command, passing "-c" in sys.argv[0]
        and "foo" in sys.argv[1].

idle -d -s -r foo.py "Hello World"
        Open a shell window, run a startup script, enable the debugger, and
        run foo.py, passing "foo.py" in sys.argv[0] and "Hello World" in
        sys.argv[1].

echo "import sys; print sys.argv" | idle - "foobar"
        Open a shell window, run the script piped in, passing '' in sys.argv[0]
        and "foobar" in sys.argv[1].
"""

def main():
    global flist, root, use_subprocess

    enable_shell = False
    enable_edit = False
    debug = False
    cmd = None
    script = None
    startup = False
    try:
        opts, args = getopt.getopt(sys.argv[1:], "c:deihr:st:")
    except getopt.error, msg:
        sys.stderr.write("Error: %s\n" % str(msg))
        sys.stderr.write(usage_msg)
        sys.exit(2)
    for o, a in opts:
        if o == '-c':
            cmd = a
            enable_shell = True
        if o == '-d':
            debug = True
            enable_shell = True
        if o == '-e':
            enable_edit = True
        if o == '-h':
            sys.stdout.write(usage_msg)
            sys.exit()
        if o == '-i':
            enable_shell = True
        if o == '-r':
            script = a
            if os.path.isfile(script):
                pass
            else:
                print "No script file: ", script
                sys.exit()
            enable_shell = True
        if o == '-s':
            startup = True
            enable_shell = True
        if o == '-t':
            PyShell.shell_title = a
            enable_shell = True
    if args and args[0] == '-':
        cmd = sys.stdin.read()
        enable_shell = True
        
    use_subprocess = True

    # process sys.argv and sys.path:
    for i in range(len(sys.path)):
        sys.path[i] = os.path.abspath(sys.path[i])
    if args and args[0] == '-':
        sys.argv = [''] + args[1:]
    elif cmd:
        sys.argv = ['-c'] + args
    elif script:
        sys.argv = [script] + args
    elif args:
        enable_edit = True
        pathx = []
        for filename in args:
            pathx.append(os.path.dirname(filename))
        for dir in pathx:
            dir = os.path.abspath(dir)
            if not dir in sys.path:
                sys.path.insert(0, dir)
    # check the IDLE settings configuration (but command line overrides)
    edit_start = idleConf.GetOption('main', 'General',
                                    'editor-on-startup', type='bool') 
    enable_edit = enable_edit or edit_start
    enable_shell = enable_shell or not edit_start  
    # start editor and/or shell windows:
    root = Tk(className="Idle")
    fixwordbreaks(root)
    root.withdraw()
    flist = PyShellFileList(root)
    if enable_edit:
        if not (cmd or script):
            for filename in args:
                flist.open(filename)
            if not args:
                flist.new()
        if enable_shell:
            flist.open_shell()
    elif enable_shell:
        flist.pyshell = PyShell(flist)
        flist.pyshell.begin()
    shell = flist.pyshell
    # handle remaining options:
    if debug:
        shell.open_debugger()
    if startup:
        filename = os.environ.get("IDLESTARTUP") or \
                   os.environ.get("PYTHONSTARTUP")
        if filename and os.path.isfile(filename):
            shell.interp.execfile(filename)
    if cmd or script:
        shell.interp.runcommand("""if 1:
            import sys as _sys
            _sys.argv = %s
            del _sys
            \n""" % `sys.argv`)
        if cmd:
            shell.interp.execsource(cmd)
        elif script:
            shell.interp.execfile(script)
    root.mainloop()
    root.destroy()


def display_port_binding_error():
    print """\
IDLE cannot run.

IDLE needs to use a specific TCP/IP port (8833) in order to execute and
debug programs. IDLE is unable to bind to this port, and so cannot
start. Here are some possible causes of this problem:

  1. TCP/IP networking is not installed or not working on this computer
  2. Another program is running that uses this port
  3. Personal firewall software is preventing IDLE from using this port

IDLE makes and accepts connections only with this computer, and does not
communicate over the internet in any way. Its use of port 8833 should not 
be a security risk on a single-user machine.
"""

if __name__ == "__main__":
    main()
