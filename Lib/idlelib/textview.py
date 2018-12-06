"""Simple text browser for IDLE

"""
from tkinter import Toplevel, Text, TclError,\
    HORIZONTAL, VERTICAL, N, S, E, W
from tkinter.ttk import Frame, Scrollbar, Button
from tkinter.messagebox import showerror

from idlelib.colorizer import color_config


class AutoHiddenScrollbar(Scrollbar):
    """A scrollbar that is automatically hidden when not needed.

    Only the grid geometry manager is supported.
    """
    def set(self, lo, hi):
        if float(lo) > 0.0 or float(hi) < 1.0:
            self.grid()
        else:
            self.grid_remove()
        super().set(lo, hi)

    def pack(self, **kwargs):
        raise TclError(f'{self.__class__.__name__} does not support "pack"')

    def place(self, **kwargs):
        raise TclError(f'{self.__class__.__name__} does not support "place"')


class TextFrame(Frame):
    "Display text with scrollbar."

    def __init__(self, parent, rawtext, wrap='word'):
        """Create a frame for Textview.

        parent - parent widget for this frame
        rawtext - text to display
        """
        super().__init__(parent)
        self['relief'] = 'sunken'
        self['height'] = 700

        self.text = text = Text(self, wrap=wrap, highlightthickness=0)
        color_config(text)
        text.grid(row=0, column=0, sticky=N+S+E+W)
        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)
        text.insert(0.0, rawtext)
        text['state'] = 'disabled'
        text.focus_set()

        # vertical scrollbar
        self.yscroll = yscroll = AutoHiddenScrollbar(self, orient=VERTICAL,
                                                     takefocus=False,
                                                     command=text.yview)
        text['yscrollcommand'] = yscroll.set
        yscroll.grid(row=0, column=1, sticky=N+S)

        if wrap == 'none':
            # horizontal scrollbar
            self.xscroll = xscroll = AutoHiddenScrollbar(self, orient=HORIZONTAL,
                                                         takefocus=False,
                                                         command=text.xview)
            text['xscrollcommand'] = xscroll.set
            xscroll.grid(row=1, column=0, sticky=E+W)


class ViewFrame(Frame):
    "Display TextFrame and Close button."
    def __init__(self, parent, text, wrap='word'):
        super().__init__(parent)
        self.parent = parent
        self.bind('<Return>', self.ok)
        self.bind('<Escape>', self.ok)
        self.textframe = TextFrame(self, text, wrap=wrap)
        self.button_ok = button_ok = Button(
                self, text='Close', command=self.ok, takefocus=False)
        self.textframe.pack(side='top', expand=True, fill='both')
        button_ok.pack(side='bottom')

    def ok(self, event=None):
        """Dismiss text viewer dialog."""
        self.parent.destroy()


class ViewWindow(Toplevel):
    "A simple text viewer dialog for IDLE."

    def __init__(self, parent, title, text, modal=True, wrap='word',
                 *, _htest=False, _utest=False):
        """Show the given text in a scrollable window with a 'close' button.

        If modal is left True, users cannot interact with other windows
        until the textview window is closed.

        parent - parent of this dialog
        title - string which is title of popup dialog
        text - text to display in dialog
        wrap - type of text wrapping to use ('word', 'char' or 'none')
        _htest - bool; change box location when running htest.
        _utest - bool; don't wait_window when running unittest.
        """
        super().__init__(parent)
        self['borderwidth'] = 5
        # Place dialog below parent if running htest.
        x = parent.winfo_rootx() + 10
        y = parent.winfo_rooty() + (10 if not _htest else 100)
        self.geometry(f'=750x500+{x}+{y}')

        self.title(title)
        self.viewframe = ViewFrame(self, text, wrap=wrap)
        self.protocol("WM_DELETE_WINDOW", self.ok)
        self.button_ok = button_ok = Button(self, text='Close',
                                            command=self.ok, takefocus=False)
        self.viewframe.pack(side='top', expand=True, fill='both')

        self.is_modal = modal
        if self.is_modal:
            self.transient(parent)
            self.grab_set()
            if not _utest:
                self.wait_window()

    def ok(self, event=None):
        """Dismiss text viewer dialog."""
        if self.is_modal:
            self.grab_release()
        self.destroy()


def view_text(parent, title, text, modal=True, wrap='word', _utest=False):
    """Create text viewer for given text.

    parent - parent of this dialog
    title - string which is the title of popup dialog
    text - text to display in this dialog
    wrap - type of text wrapping to use ('word', 'char' or 'none')
    modal - controls if users can interact with other windows while this
            dialog is displayed
    _utest - bool; controls wait_window on unittest
    """
    return ViewWindow(parent, title, text, modal, wrap=wrap, _utest=_utest)


def view_file(parent, title, filename, encoding, modal=True, wrap='word',
              _utest=False):
    """Create text viewer for text in filename.

    Return error message if file cannot be read.  Otherwise calls view_text
    with contents of the file.
    """
    try:
        with open(filename, 'r', encoding=encoding) as file:
            contents = file.read()
    except OSError:
        showerror(title='File Load Error',
                  message=f'Unable to load file {filename!r} .',
                  parent=parent)
    except UnicodeDecodeError as err:
        showerror(title='Unicode Decode Error',
                  message=str(err),
                  parent=parent)
    else:
        return view_text(parent, title, contents, modal, wrap=wrap,
                         _utest=_utest)
    return None


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_textview', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(ViewWindow)
