"""Extension to execute code outside the Python shell window.

This adds the following commands (to the Edit menu, until there's a
separate Python menu):

- Check module (Alt-F5) does a full syntax check of the current module.
It also runs the tabnanny to catch any inconsistent tabs.

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

indent_message = """Error: Inconsistent indentation detected!

This means that either:

(1) your indentation is outright incorrect (easy to fix), or

(2) your indentation mixes tabs and spaces in a way that depends on \
how many spaces a tab is worth.

To fix case 2, change all tabs to spaces by using Select All followed \
by Untabify Region (both in the Edit menu)."""

class ScriptBinding:
    
    keydefs = {
        '<<check-module>>': ['<Alt-F5>', '<Meta-F5>'],
        '<<import-module>>': ['<F5>'],
        '<<run-script>>': ['<Control-F5>'],
    }
    
    menudefs = [
        ('edit', [None,
                  ('Check module', '<<check-module>>'),
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

    def check_module_event(self, event):
        filename = self.getfilename()
        if not filename:
            return
        if not self.tabnanny(filename):
            return
        if not self.checksyntax(filename):
            return

    def tabnanny(self, filename):
        import tabnanny
        import tokenize
        tabnanny.reset_globals()
        f = open(filename, 'r')
        try:
            tokenize.tokenize(f.readline, tabnanny.tokeneater)
        except tokenize.TokenError, msg:
            self.errorbox("Token error",
                          "Token error:\n%s" % str(msg))
            return 0
        except tabnanny.NannyNag, nag:
            # The error messages from tabnanny are too confusing...
            self.editwin.gotoline(nag.get_lineno())
            self.errorbox("Tab/space error", indent_message)
            return 0
        return 1

    def checksyntax(self, filename):
        f = open(filename, 'r')
        source = f.read()
        f.close()
        if '\r' in source:
            import re
            source = re.sub(r"\r\n", "\n", source)
        if source and source[-1] != '\n':
            source = source + '\n'
        try:
            compile(source, filename, "exec")
        except (SyntaxError, OverflowError), err:
            try:
                msg, (errorfilename, lineno, offset, line) = err
                if not errorfilename:
                    err.args = msg, (filename, lineno, offset, line)
                    err.filename = filename
            except:
                lineno = None
                msg = "*** " + str(err)
            if lineno:
                self.editwin.gotoline(lineno)
            self.errorbox("Syntax error",
                          "There's an error in your program:\n" + msg)
        return 1

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
        if (not sys.argv or
            os.path.basename(sys.argv[0]) != os.path.basename(filename)):
            # XXX Too often this discards arguments the user just set...
            sys.argv = [filename]
        interp.execfile(filename)

    def getfilename(self):
        # Logic to make sure we have a saved filename
        # XXX Better logic would offer to save!
        if not self.editwin.get_saved():
            name = (self.editwin.short_title() or
                    self.editwin.long_title() or
                    "Untitled")
            self.errorbox("Not saved",
                          "The buffer for %s is not saved.\n" % name +
                          "Please save it first!")
            self.editwin.text.focus_set()
            return
        filename = self.editwin.io.filename
        if not filename:
            self.errorbox("No file name",
                          "This window has no file name")
            return
        return filename

    def errorbox(self, title, message):
        # XXX This should really be a function of EditorWindow...
        tkMessageBox.showerror(title, message, master=self.editwin.text)
        self.editwin.text.focus_set()
