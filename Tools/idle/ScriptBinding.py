"""Extension to execute code outside the Python shell window.

This adds two commands (to the Edit menu, until there's a separate
Python menu):

- Run module (F5) is equivalent to either import or reload of the
current module.  The window must have been saved previously. The
module only gets added to sys.modules, it doesn't get added to
anyone's namespace; you can import it in the shell if you need to.  If
this generates any output to sys.stdout or sys.stderr, a new output
window is created to display that output. The two streams are
distinguished by their text color.

- Debug module (Control-F5) does the same but executes the module's
code in the debugger.

When an unhandled exception occurs in Run module, the stack viewer is
popped up.  This is not done in Debug module, because you've already
had an opportunity to view the stack.  In either case, the variables
sys.last_type, sys.last_value, sys.last_traceback are set to the
exception info.

"""

import sys
import os
import imp
import linecache
import traceback
import tkMessageBox
from OutputWindow import OutputWindow

# XXX These helper classes are more generally usable!

class OnDemandOutputWindow:

    tagdefs = {
        "stdout":  {"foreground": "blue"},
        "stderr":  {"foreground": "#007700"},
    }   
    
    def __init__(self, flist):
        self.flist = flist
        self.owin = None
    
    def write(self, s, tags, mark):
        if not self.owin:
            self.setup()
        self.owin.write(s, tags, mark)
    
    def setup(self):
        self.owin = owin = OutputWindow(self.flist)
        text = owin.text
        for tag, cnf in self.tagdefs.items():
            if cnf:
                apply(text.tag_configure, (tag,), cnf)
        text.tag_raise('sel')
        self.write = self.owin.write

class PseudoFile:

    def __init__(self, owin, tags, mark="end"):
        self.owin = owin
        self.tags = tags
        self.mark = mark

    def write(self, s):
        self.owin.write(s, self.tags, self.mark)

    def writelines(self, l):
        map(self.write, l)


class ScriptBinding:
    
    keydefs = {
        '<<run-module>>': ['<F5>'],
        '<<debug-module>>': ['<Control-F5>'],
    }
    
    menudefs = [
        ('edit', [None,
                  ('Run module', '<<run-module>>'),
                  ('Debug module', '<<debug-module>>'),
                 ]
        ),
    ]

    def __init__(self, editwin):
        self.editwin = editwin
        # Provide instance variables referenced by Debugger
        # XXX This should be done differently
        self.flist = self.editwin.flist
        self.root = self.flist.root

    def run_module_event(self, event, debugger=None):
        if not self.editwin.get_saved():
            tkMessageBox.showerror("Not saved",
                                   "Please save first!",
                                   master=self.editwin.text)
            self.editwin.text.focus_set()
            return
        filename = self.editwin.io.filename
        if not filename:
            tkMessageBox.showerror("No file name",
                                   "This window has no file name",
                                   master=self.editwin.text)
            self.editwin.text.focus_set()
            return
        modname, ext = os.path.splitext(os.path.basename(filename))
        if sys.modules.has_key(modname):
            mod = sys.modules[modname]
        else:
            mod = imp.new_module(modname)
            sys.modules[modname] = mod
        mod.__file__ = filename
        saveout = sys.stdout
        saveerr = sys.stderr
        owin = OnDemandOutputWindow(self.editwin.flist)
        try:
            sys.stderr = PseudoFile(owin, "stderr")
            try:
                sys.stdout = PseudoFile(owin, "stdout")
                try:
                    if debugger:
                        debugger.run("execfile(%s)" % `filename`, mod.__dict__)
                    else:
                        execfile(filename, mod.__dict__)
                except:
                    (sys.last_type, sys.last_value,
                     sys.last_traceback) = sys.exc_info()
##                    linecache.checkcache()
##                    traceback.print_exc()
                    if not debugger:
                        from StackViewer import StackBrowser
                        sv = StackBrowser(self.root, self.flist)
            finally:
                sys.stdout = saveout
        finally:
            sys.stderr = saveerr

    def debug_module_event(self, event):
        import Debugger
        debugger = Debugger.Debugger(self)
        self.run_module_event(event, debugger)

    def close_debugger(self):
        # Method called by Debugger
        # XXX This should be done differently
        pass
