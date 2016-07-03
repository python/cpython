"""
Dialogs that query users and verify the answer before accepting.
Use ttk widgets, limiting use to tcl/tk 8.5+, as in IDLE 3.6+.

Query is the generic base class for a popup dialog.
The user must either enter a valid answer or close the dialog.
Entries are validated when <Return> is entered or [Ok] is clicked.
Entries are ignored when [Cancel] or [X] are clicked.
The 'return value' is .result set to either a valid answer or None.

Subclass SectionName gets a name for a new config file section.
Configdialog uses it for new highlight theme and keybinding set names.
"""
# Query and Section name result from splitting GetCfgSectionNameDialog
# of configSectionNameDialog.py (temporarily config_sec.py) into
# generic and specific parts.

import importlib
from tkinter import Toplevel, StringVar
from tkinter.messagebox import showerror
from tkinter.ttk import Frame, Button, Entry, Label

class Query(Toplevel):
    """Base class for getting verified answer from a user.

    For this base class, accept any non-blank string.
    """
    def __init__(self, parent, title, message, text0='',
                 *, _htest=False, _utest=False):
        """Create popup, do not return until tk widget destroyed.

        Additional subclass init must be done before calling this
        unless  _utest=True is passed to suppress wait_window().

        title - string, title of popup dialog
        message - string, informational message to display
        text0 - initial value for entry
        _htest - bool, change box location when running htest
        _utest - bool, leave window hidden and not modal
        """
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        self.resizable(height=False, width=False)
        self.title(title)
        self.transient(parent)
        self.grab_set()
        self.bind('<Key-Return>', self.ok)
        self.protocol("WM_DELETE_WINDOW", self.cancel)
        self.parent = parent
        self.message = message
        self.text0 = text0
        self.create_widgets()
        self.update_idletasks()
        #needs to be done here so that the winfo_reqwidth is valid
        self.withdraw()  # Hide while configuring, especially geometry.
        self.geometry(
                "+%d+%d" % (
                    parent.winfo_rootx() +
                    (parent.winfo_width()/2 - self.winfo_reqwidth()/2),
                    parent.winfo_rooty() +
                    ((parent.winfo_height()/2 - self.winfo_reqheight()/2)
                    if not _htest else 150)
                ) )  #centre dialog over parent (or below htest box)
        if not _utest:
            self.deiconify()  #geometry set, unhide
            self.wait_window()

    def create_widgets(self):  # Call from override, if any.
        # Bind widgets needed for entry_ok or unittest to self.
        frame = Frame(self, borderwidth=2, relief='sunken', )
        label = Label(frame, anchor='w', justify='left',
                    text=self.message)
        self.entryvar = StringVar(self, self.text0)
        self.entry = Entry(frame, width=30, textvariable=self.entryvar)
        self.entry.focus_set()

        buttons = Frame(self)
        self.button_ok = Button(buttons, text='Ok',
                width=8, command=self.ok)
        self.button_cancel = Button(buttons, text='Cancel',
                width=8, command=self.cancel)

        frame.pack(side='top', expand=True, fill='both')
        label.pack(padx=5, pady=5)
        self.entry.pack(padx=5, pady=5)
        buttons.pack(side='bottom')
        self.button_ok.pack(side='left', padx=5)
        self.button_cancel.pack(side='right', padx=5)

    def entry_ok(self):  # Example: usually replace.
        "Return non-blank entry or None."
        entry = self.entry.get().strip()
        if not entry:
            showerror(title='Entry Error',
                    message='Blank line.', parent=self)
            return
        return entry

    def ok(self, event=None):  # Do not replace.
        '''If entry is valid, bind it to 'result' and destroy tk widget.

        Otherwise leave dialog open for user to correct entry or cancel.
        '''
        entry = self.entry_ok()
        if entry is not None:
            self.result = entry
            self.destroy()
        else:
            # [Ok] (but not <Return>) moves focus.  Move it back.
            self.entry.focus_set()

    def cancel(self, event=None):  # Do not replace.
        "Set dialog result to None and destroy tk widget."
        self.result = None
        self.destroy()


class SectionName(Query):
    "Get a name for a config file section name."

    def __init__(self, parent, title, message, used_names,
                 *, _htest=False, _utest=False):
        "used_names - collection of strings already in use"
        self.used_names = used_names
        Query.__init__(self, parent, title, message,
                 _htest=_htest, _utest=_utest)

    def entry_ok(self):
        "Return sensible ConfigParser section name or None."
        name = self.entry.get().strip()
        if not name:
            showerror(title='Name Error',
                    message='No name specified.', parent=self)
            return
        elif len(name)>30:
            showerror(title='Name Error',
                    message='Name too long. It should be no more than '+
                    '30 characters.', parent=self)
            return
        elif name in self.used_names:
            showerror(title='Name Error',
                    message='This name is already in use.', parent=self)
            return
        return name


class ModuleName(Query):
    "Get a module name for Open Module menu entry."
    # Used in open_module (editor.EditorWindow until move to iobinding).

    def __init__(self, parent, title, message, text0='',
                 *, _htest=False, _utest=False):
        """text0 - name selected in text before Open Module invoked"
        """
        Query.__init__(self, parent, title, message, text0=text0,
                 _htest=_htest, _utest=_utest)

    def entry_ok(self):
        "Return entered module name as file path or None."
        # Moved here from Editor_Window.load_module 2016 July.
        name = self.entry.get().strip()
        if not name:
            showerror(title='Name Error',
                    message='No name specified.', parent=self)
            return
        # XXX Ought to insert current file's directory in front of path
        try:
            spec = importlib.util.find_spec(name)
        except (ValueError, ImportError) as msg:
            showerror("Import Error", str(msg), parent=self)
            return
        if spec is None:
            showerror("Import Error", "module not found",
                      parent=self)
            return
        if not isinstance(spec.loader, importlib.abc.SourceLoader):
            showerror("Import Error", "not a source-based module",
                      parent=self)
            return
        try:
            file_path = spec.loader.get_filename(name)
        except AttributeError:
            showerror("Import Error",
                      "loader does not support get_filename",
                      parent=self)
            return
        return file_path


if __name__ == '__main__':
    import unittest
    unittest.main('idlelib.idle_test.test_query', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(Query)
