"""Extension to execute code outside the Python shell window.

This adds the following commands:

- Check module does a full syntax check of the current module.
It also runs the tabnanny to catch any inconsistent tabs.

- Import module is equivalent to either import or reload of the
current module.  The window must have been saved previously. The
module is added to sys.modules, and is also added to the __main__
namespace.  Output goes to the shell window.

- Run module does the same but executes the module's
code in the __main__ namespace.

XXX Redesign this interface (yet again) as follows:

- Present a dialog box for ``Run script''

- Allow specify command line arguments in the dialog box

- Restart the interpreter when running a script

"""

import sys
import os
import imp
import tkMessageBox

indent_message = """Error: Inconsistent indentation detected!

This means that either:

1) your indentation is outright incorrect (easy to fix), or

2) your indentation mixes tabs and spaces in a way that depends on \
how many spaces a tab is worth.

To fix case 2, change all tabs to spaces by using Select All followed \
by Untabify Region (both in the Edit menu)."""


# XXX TBD Implement stop-execution  KBK 11Jun02
class ScriptBinding:

    menudefs = [
        ('run', [None,
#                 ('Check module', '<<check-module>>'),
#                 ('Import module', '<<import-module>>'),
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
        flist = self.editwin.flist
        shell = flist.open_shell()
        interp = shell.interp

        filename = self.getfilename()
        if not filename:
            return

        modname, ext = os.path.splitext(os.path.basename(filename))

        dir = os.path.dirname(filename)
        dir = os.path.normpath(os.path.abspath(dir))

        interp.runcode("""if 1:
            import sys as _sys
            if %s not in _sys.path:
                _sys.path.insert(0, %s)
            if _sys.modules.get(%s):
                del _sys
                import %s
                reload(%s)
            else:
                del _sys
                import %s
                \n""" % (`dir`, `dir`, `modname`, modname, modname, modname))

    def run_script_event(self, event):
        filename = self.getfilename()
        if not filename:
            return
        flist = self.editwin.flist
        shell = flist.open_shell()
        interp = shell.interp
        # XXX Too often this discards arguments the user just set...
        interp.runcommand("""if 1:
            _filename = %s
            import sys as _sys
            from os.path import basename as _basename
            if (not _sys.argv or
                _basename(_sys.argv[0]) != _basename(_filename)):
                # XXX 25 July 2002 KBK should this be sys.argv not _sys.argv?
                _sys.argv = [_filename]
            del _filename, _sys, _basename
                \n""" % `filename`)
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
