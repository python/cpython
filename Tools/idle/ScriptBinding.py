import tkMessageBox
import os
import imp
import sys

class ScriptBinding:

    def __init__(self, editwin):
        self.editwin = editwin
        text = editwin.text
        text.bind("<<run-module>>", self.run_module)
        text.bind("<<run-script>>", self.run_script)
        text.bind("<<new-shell>>", self.new_shell)

    def run_module(self, event=None):
        filename = self.editwin.io.filename
        if not filename:
            tkMessageBox.showerror("No file name",
                                   "This window has no file name",
                                   master=self.editwin.text)
            return
        modname, ext = os.path.splitext(os.path.basename(filename))
        try:
            mod = sys.modules[modname]
        except KeyError:
            mod = imp.new_module(modname)
            sys.modules[modname] = mod
        source = self.editwin.text.get("1.0", "end")
        exec source in mod.__dict__

    def run_script(self, event=None):
        pass

    def new_shell(self, event=None):
        import PyShell
        # XXX Not enough: each shell takes over stdin/stdout/stderr...
        pyshell = PyShell.PyShell(self.editwin.flist)
        pyshell.begin()
