"""Simple text browser for IDLE

"""
from tkinter import Toplevel, Frame, Button, Text
from tkinter import DISABLED, SUNKEN, VERTICAL, WORD
from tkinter import RIGHT, LEFT, TOP, BOTTOM, BOTH, X, Y
from tkinter.ttk import Scrollbar
from tkinter.messagebox import showerror


class TextViewer(Toplevel):
    "A simple text viewer dialog for IDLE."

    def __init__(self, parent, title, text, modal=True,
                 _htest=False, _utest=False):
        """Show the given text in a scrollable window with a 'close' button.

        If modal is left True, users cannot interact with other windows
        until the textview window is closed.

        parent - parent of this dialog
        title - string which is title of popup dialog
        text - text to display in dialog
        _htest - bool; change box location when running htest.
        _utest - bool; don't wait_window when running unittest.
        """
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        # Place dialog below parent if running htest.
        self.geometry("=%dx%d+%d+%d" % (750, 500,
                           parent.winfo_rootx() + 10,
                           parent.winfo_rooty() + (10 if not _htest else 100)))
        # TODO: get fg/bg from theme.
        self.bg = '#ffffff'
        self.fg = '#000000'

        self.create_widgets()
        self.title(title)
        self.protocol("WM_DELETE_WINDOW", self.ok)
        self.parent = parent
        self.text.focus_set()
        # Bind keys for closing this dialog.
        self.bind('<Return>', self.ok)
        self.bind('<Escape>', self.ok)
        self.text.insert(0.0, text)
        self.text.config(state=DISABLED)

        if modal:
            self.transient(parent)
            self.grab_set()
            if not _utest:
                self.wait_window()

    def create_widgets(self):
        "Create Frame with Text (with vertical Scrollbar) and Button."
        frame = Frame(self, relief=SUNKEN, height=700)
        frame_buttons = Frame(self)
        self.button_ok = Button(frame_buttons, text='Close',
                                command=self.ok, takefocus=False)
        self.scrollbar = Scrollbar(frame, orient=VERTICAL, takefocus=False)
        self.text = Text(frame, wrap=WORD, highlightthickness=0,
                             fg=self.fg, bg=self.bg)
        self.scrollbar.config(command=self.text.yview)
        self.text.config(yscrollcommand=self.scrollbar.set)
        
        self.button_ok.pack()
        self.scrollbar.pack(side=RIGHT, fill=Y)
        self.text.pack(side=LEFT, expand=True, fill=BOTH)
        frame_buttons.pack(side=BOTTOM, fill=X)
        frame.pack(side=TOP, expand=True, fill=BOTH)

    def ok(self, event=None):
        """Dismiss text viewer dialog."""
        self.destroy()


def view_text(parent, title, text, modal=True, _utest=False):
    """Create TextViewer for given text.

    parent - parent of this dialog
    title - string which is the title of popup dialog
    text - text to display in this dialog
    modal - controls if users can interact with other windows while this
            dialog is displayed
    _utest - bool; controls wait_window on unittest
    """
    return TextViewer(parent, title, text, modal, _utest=_utest)


def view_file(parent, title, filename, encoding=None, modal=True, _utest=False):
    """Create TextViewer for text in filename.

    Return error message if file cannot be read.  Otherwise calls view_text
    with contents of the file.
    """
    try:
        with open(filename, 'r', encoding=encoding) as file:
            contents = file.read()
    except OSError:
        showerror(title='File Load Error',
                  message='Unable to load file %r .' % filename,
                  parent=parent)
    except UnicodeDecodeError as err:
        showerror(title='Unicode Decode Error',
                  message=str(err),
                  parent=parent)
    else:
        return view_text(parent, title, contents, modal, _utest=_utest)
    return None


if __name__ == '__main__':
    import unittest
    unittest.main('idlelib.idle_test.test_textview', verbosity=2, exit=False)
    from idlelib.idle_test.htest import run
    run(TextViewer)
