"""Extension to execute code outside the Python shell window.

This adds two commands (to the Edit menu, until there's a separate
Python menu):

- Import module (F5) is equivalent to either import or reload of the
current module.  The window must have been saved previously. The
module is added to sys.modules, and is also added to the __main__
namespace.  Output goes to the shell window.

- Run module (Control-F5) does the same but executes the module's
code in the __main__ namespace.

"""

import sys
import os
import imp
import tkMessageBox


class ScriptBinding:
    
    keydefs = {
        '<<import-module>>': ['<F5>'],
        '<<run-script>>': ['<Control-F5>'],
    }
    
    menudefs = [
        ('edit', [None,
                  ('Import module', '<<import-module>>'),
                  ('Run script', '<<run-script>>'),
                 ]
        ),
    ]

    def __init__(self, editwin):
        self.editwin = editwin
        # Provide instance variables referenced by Debugger
        # XXX This should be done differently
        self.flist = self.editwin.flist
        self.root = self.flist.root

    def import_module_event(self, event):
        filename = self.getfilename()
        if not filename:
            return

        modname, ext = os.path.splitext(os.path.basename(filename))
        if sys.modules.has_key(modname):
            mod = sys.modules[modname]
        else:
            mod = imp.new_module(modname)
            sys.modules[modname] = mod
        mod.__file__ = filename
        setattr(sys.modules['__main__'], modname, mod)

        dir = os.path.dirname(filename)
        dir = os.path.normpath(os.path.abspath(dir))
        if dir not in sys.path:
            sys.path.insert(0, dir)

        flist = self.editwin.flist
        shell = flist.open_shell()
        interp = shell.interp
        interp.runcode("reload(%s)" % modname)

    def run_script_event(self, event):
        filename = self.getfilename()
        if not filename:
            return

        flist = self.editwin.flist
        shell = flist.open_shell()
        interp = shell.interp
        interp.execfile(filename)

    def getfilename(self):
        # Logic to make sure we have a saved filename
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
        return filename
