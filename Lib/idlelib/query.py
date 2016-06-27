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

from tkinter import FALSE, TRUE, Toplevel
from tkinter.messagebox import showerror
from tkinter.ttk import Frame, Button, Entry, Label

class Query(Toplevel):
    """Base class for getting verified answer from a user.

    For this base class, accept any non-blank string.
    """
    def __init__(self, parent, title, message,
                 *, _htest=False, _utest=False):  # Call from override.
        """Create popup, do not return until tk widget destroyed.

        Additional subclass init must be done before calling this.

        title - string, title of popup dialog
        message - string, informational message to display
        _htest - bool, change box location when running htest
        _utest - bool, leave window hidden and not modal
        """
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        self.resizable(height=FALSE, width=FALSE)
        self.title(title)
        self.transient(parent)
        self.grab_set()
        self.bind('<Key-Return>', self.ok)
        self.protocol("WM_DELETE_WINDOW", self.cancel)
        self.parent = parent
        self.message = message
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
        frame = Frame(self, borderwidth=2, relief='sunken', )
        label = Label(frame, anchor='w', justify='left',
                    text=self.message)
        self.entry = Entry(frame, width=30)  # Bind name for entry_ok.
        self.entry.focus_set()

        buttons = Frame(self)  # Bind buttons for invoke in unittest.
        self.button_ok = Button(buttons, text='Ok',
                width=8, command=self.ok)
        self.button_cancel = Button(buttons, text='Cancel',
                width=8, command=self.cancel)

        frame.pack(side='top', expand=TRUE, fill='both')
        label.pack(padx=5, pady=5)
        self.entry.pack(padx=5, pady=5)
        buttons.pack(side='bottom')
        self.button_ok.pack(side='left', padx=5)
        self.button_cancel.pack(side='right', padx=5)

    def entry_ok(self):  # Usually replace.
        "Check that entry not blank."
        entry = self.entry.get().strip()
        if not entry:
            showerror(title='Entry Error',
                    message='Blank line.', parent=self)
        return entry

    def ok(self, event=None):  # Do not replace.
        '''If entry is valid, bind it to 'result' and destroy tk widget.

        Otherwise leave dialog open for user to correct entry or cancel.
        '''
        entry = self.entry_ok()
        if entry:
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
        # This call does ot return until tk widget is destroyed.

    def entry_ok(self):
        '''Stripping entered name, check that it is a  sensible
        ConfigParser file section name. Return it if it is, '' if not.
        '''
        name = self.entry.get().strip()
        if not name:
            showerror(title='Name Error',
                    message='No name specified.', parent=self)
        elif len(name)>30:
            showerror(title='Name Error',
                    message='Name too long. It should be no more than '+
                    '30 characters.', parent=self)
            name = ''
        elif name in self.used_names:
            showerror(title='Name Error',
                    message='This name is already in use.', parent=self)
            name = ''
        return name


if __name__ == '__main__':
    import unittest
    unittest.main('idlelib.idle_test.test_query', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(Query)
