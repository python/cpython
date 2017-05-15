"""Extension to execute code outside the Python shell window.

This adds the following commands:

- Check module does a full syntax check of the current module.
  It also runs the tabnanny to catch any inconsistent tabs.

- Run module executes the module's code in the __main__ namespace.  The window
  must have been saved previously. The module is added to sys.modules, and is
  also added to the __main__ namespace.

XXX GvR Redesign this interface (yet again) as follows:

- Present a dialog box for ``Run Module''

- Allow specify command line arguments in the dialog box

"""

import os
import tabnanny
import tokenize

import tkinter.messagebox as tkMessageBox
from tkinter import Frame, Button, Label, Entry, Toplevel
from tkinter import TOP, TRUE, FALSE, BOTH, LEFT, BOTTOM

from idlelib.config import idleConf
from idlelib import macosx
from idlelib import pyshell

indent_message = """Error: Inconsistent indentation detected!

1) Your indentation is outright incorrect (easy to fix), OR

2) Your indentation mixes tabs and spaces.

To fix case 2, change all tabs to spaces by using Edit->Select All followed \
by Format->Untabify Region and specify the number of columns used by each tab.
"""


class ArgumentsInputBox(Toplevel):

    def __init__(self, parent, title, _htest=False):
        """
        _htest - bool, change box location when running htest
        """
        Toplevel.__init__(self, parent)
        self.parent = parent
        self.title(title)
        self.resizable(height=False, width=True)
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.cancel)

        self.geometry(
                "+%d+%d" % (parent.winfo_rootx() + parent.winfo_width() / 2 - 150,
                parent.winfo_rooty() + (30 if not _htest else 150)))
        self.minsize(width=300, height=50)

    def set_callback(self, callback):
        self.callback = callback

    def show_input_box(self):
        self.create_input_box()
        self.create_action_buttons().pack(side=BOTTOM)

    def create_input_box(self):
        self.frameMain = frameMain = Frame(self)
        frameMain.pack(side=TOP, expand=TRUE, fill=BOTH)

        self.label = label = Label(frameMain, text="Command-line arguments: ")
        label.pack(expand=TRUE, fill=BOTH)

        self.entry = entry = Entry(frameMain)
        entry.bind("<Return>", lambda event: self.run())
        entry.pack(expand=TRUE, fill=BOTH)
        entry.focus_set()

    def create_action_buttons(self):
        if macosx.isAquaTk():
            # Changing the default padding on OSX results in unreadable
            # text in the buttons
            paddingArgs = {}
        else:
            paddingArgs = {'padx': 6, 'pady': 3}
        outer = Frame(self, pady=2)
        buttons = Frame(outer, pady=2)
        for txt, cmd in (
            ('Run', self.run),
            ('Cancel', self.cancel)):
            Button(buttons, text=txt, command=cmd, takefocus=FALSE,
                   **paddingArgs).pack(side=LEFT, padx=5)
        # add space above buttons
        Frame(outer, height=2, borderwidth=0).pack(side=TOP)
        buttons.pack(side=BOTTOM)
        return outer

    def run(self):
        args = self.entry.get()
        self.destroy()
        self.callback(None, args)

    def cancel(self):
        self.destroy()


class ScriptBinding:

    menudefs = [
        ('run', [None,
                 ('Check Module', '<<check-module>>'),
                 ('Run Module', '<<run-module>>'),
                 ('Run Module with Arguments', '<<run-module-arguments>>'), ]), ]

    def __init__(self, editwin):
        self.editwin = editwin
        # Provide instance variables referenced by debugger
        # XXX This should be done differently
        self.flist = self.editwin.flist
        self.root = self.editwin.root

        if macosx.isCocoaTk():
            self.editwin.text_frame.bind('<<run-module-event-2>>', self._run_module_event)

    def run_module_arguments_event(self, event):
        aw = ArgumentsInputBox(event.widget, 'Run with arguments')
        aw.set_callback(self.run_module_event)
        aw.show_input_box()

    def check_module_event(self, event):
        filename = self.getfilename()
        if not filename:
            return 'break'
        if not self.checksyntax(filename):
            return 'break'
        if not self.tabnanny(filename):
            return 'break'

    def tabnanny(self, filename):
        # XXX: tabnanny should work on binary files as well
        with tokenize.open(filename) as f:
            try:
                tabnanny.process_tokens(tokenize.generate_tokens(f.readline))
            except tokenize.TokenError as msg:
                msgtxt, (lineno, start) = msg.args
                self.editwin.gotoline(lineno)
                self.errorbox("Tabnanny Tokenizing Error",
                              "Token Error: %s" % msgtxt)
                return False
            except tabnanny.NannyNag as nag:
                # The error messages from tabnanny are too confusing...
                self.editwin.gotoline(nag.get_lineno())
                self.errorbox("Tab/space error", indent_message)
                return False
        return True

    def checksyntax(self, filename):
        self.shell = shell = self.flist.open_shell()
        saved_stream = shell.get_warning_stream()
        shell.set_warning_stream(shell.stderr)
        with open(filename, 'rb') as f:
            source = f.read()
        if b'\r' in source:
            source = source.replace(b'\r\n', b'\n')
            source = source.replace(b'\r', b'\n')
        if source and source[-1] != ord(b'\n'):
            source = source + b'\n'
        editwin = self.editwin
        text = editwin.text
        text.tag_remove("ERROR", "1.0", "end")
        try:
            # If successful, return the compiled code
            return compile(source, filename, "exec")
        except (SyntaxError, OverflowError, ValueError) as value:
            msg = getattr(value, 'msg', '') or value or "<no detail available>"
            lineno = getattr(value, 'lineno', '') or 1
            offset = getattr(value, 'offset', '') or 0
            if offset == 0:
                lineno += 1  #mark end of offending line
            pos = "0.0 + %d lines + %d chars" % (lineno-1, offset-1)
            editwin.colorize_syntax_error(text, pos)
            self.errorbox("SyntaxError", "%-20s" % msg)
            return False
        finally:
            shell.set_warning_stream(saved_stream)

    def run_module_event(self, event, args=None):
        if macosx.isCocoaTk():
            # Tk-Cocoa in MacOSX is broken until at least
            # Tk 8.5.9, and without this rather
            # crude workaround IDLE would hang when a user
            # tries to run a module using the keyboard shortcut
            # (the menu item works fine).
            self.editwin.text_frame.after(200,
                lambda: self.editwin.text_frame.event_generate('<<run-module-event-2>>'))
            return 'break'
        else:
            return self._run_module_event(event, args)

    def _run_module_event(self, event, args=None):
        """Run the module after setting up the environment.

        First check the syntax.  If OK, make sure the shell is active and
        then transfer the arguments, set the run environment's working
        directory to the directory of the module being executed and also
        add that directory to its sys.path if not already included.
        """

        filename = self.getfilename()
        if not filename:
            return 'break'
        code = self.checksyntax(filename)
        if not code:
            return 'break'
        if not self.tabnanny(filename):
            return 'break'
        interp = self.shell.interp
        if pyshell.use_subprocess:
            interp.restart_subprocess(with_cwd=False, filename=
                        self.editwin._filename_to_unicode(filename))
        dirname = os.path.dirname(filename)
        # XXX Too often this discards arguments the user just set...
        command = """if 1:
            __file__ = {filename!r}
            import sys as _sys
            from os.path import basename as _basename
            if (not _sys.argv or
                _basename(_sys.argv[0]) != _basename(__file__)):
                _sys.argv = [__file__]
            import os as _os
            _os.chdir({dirname!r})
            \n""".format(filename=filename, dirname=dirname)
        if args:
            for arg in args.split():
                command += '_sys.argv.append({arg})\n'.format(arg=repr(arg))
        command += 'del _sys, _basename, _os\n'
        interp.runcommand(command)
        interp.prepend_syspath(filename)
        # XXX KBK 03Jul04 When run w/o subprocess, runtime warnings still
        #         go to __stderr__.  With subprocess, they go to the shell.
        #         Need to change streams in pyshell.ModifiedInterpreter.
        interp.runcode(code)
        return 'break'

    def getfilename(self):
        """Get source filename.  If not saved, offer to save (or create) file

        The debugger requires a source file.  Make sure there is one, and that
        the current version of the source buffer has been saved.  If the user
        declines to save or cancels the Save As dialog, return None.

        If the user has configured IDLE for Autosave, the file will be
        silently saved if it already exists and is dirty.

        """
        filename = self.editwin.io.filename
        if not self.editwin.get_saved():
            autosave = idleConf.GetOption('main', 'General',
                                          'autosave', type='bool')
            if autosave and filename:
                self.editwin.io.save(None)
            else:
                confirm = self.ask_save_dialog()
                self.editwin.text.focus_set()
                if confirm:
                    self.editwin.io.save(None)
                    filename = self.editwin.io.filename
                else:
                    filename = None
        return filename

    def ask_save_dialog(self):
        msg = "Source Must Be Saved\n" + 5*' ' + "OK to Save?"
        confirm = tkMessageBox.askokcancel(title="Save Before Run or Check",
                                           message=msg,
                                           default=tkMessageBox.OK,
                                           parent=self.editwin.text)
        return confirm

    def errorbox(self, title, message):
        # XXX This should really be a function of EditorWindow...
        tkMessageBox.showerror(title, message, parent=self.editwin.text)
        self.editwin.text.focus_set()
