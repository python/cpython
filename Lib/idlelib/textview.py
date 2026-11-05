"""Simple text browser for IDLE

"""
from tkinter import Toplevel, Text, TclError,\
    HORIZONTAL, VERTICAL, NS, EW, NSEW, NONE, WORD, SUNKEN
from tkinter.ttk import Frame, Scrollbar, Button
from tkinter.messagebox import showerror

from idlelib import search
from idlelib.colorizer import color_config
from idlelib.config import idleConf


class AutoHideScrollbar(Scrollbar):
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


class ScrollableTextFrame(Frame):
    """Display text with scrollbar(s)."""

    def __init__(self, master, wrap=NONE, **kwargs):
        """Create a frame for Textview.

        master - master widget for this frame
        wrap - type of text wrapping to use ('word', 'char' or 'none')

        All parameters except for 'wrap' are passed to Frame.__init__().

        The Text widget is accessible via the 'text' attribute.

        Note: Changing the wrapping mode of the text widget after
        instantiation is not supported.
        """
        super().__init__(master, **kwargs)

        text = self.text = Text(self, wrap=wrap)
        text.grid(row=0, column=0, sticky=NSEW)
        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)

        # vertical scrollbar
        self.yscroll = AutoHideScrollbar(self, orient=VERTICAL,
                                         takefocus=False,
                                         command=text.yview)
        self.yscroll.grid(row=0, column=1, sticky=NS)
        text['yscrollcommand'] = self.yscroll.set

        # horizontal scrollbar - only when wrap is set to NONE
        if wrap == NONE:
            self.xscroll = AutoHideScrollbar(self, orient=HORIZONTAL,
                                             takefocus=False,
                                             command=text.xview)
            self.xscroll.grid(row=1, column=0, sticky=EW)
            text['xscrollcommand'] = self.xscroll.set
        else:
            self.xscroll = None


class ViewFrame(Frame):
    """Display a TextFrame and "Close" and "Search" buttons."""
    def __init__(self, parent, contents, wrap='word'):
        """Create a frame for viewing text.

        parent - parent widget for this frame
        contents - text to display
        wrap - type of text wrapping to use ('word', 'char' or 'none')

        The Text widget is accessible via the 'text' attribute.
        The frame has "Close" and "Search" buttons.
        """
        super().__init__(parent)
        self.parent = parent
        self.bind('<Return>', self.ok)
        self.bind('<Escape>', self.ok)
        self.textframe = ScrollableTextFrame(self, relief=SUNKEN, height=900)

        text = self.text = self.textframe.text
        text.insert('1.0', contents)
        text.configure(wrap=wrap, highlightthickness=0, state='disabled')
        color_config(text)
        text.focus_set()

        buttons = Frame(self, padding=2)
        self.button_ok = button_ok = Button(
            buttons, text='Close', command=self.ok, takefocus=False,
        )
        self.textframe.pack(side='top', expand=True, fill='both')
        button_ok.pack(side='left', padx=5)

        self.button_search = button_search = Button(
            buttons, text='Search',
            command=lambda: self.find_event(None),
            takefocus=False,
        )
        button_search.pack(side='left', padx=5)

        keydefs = idleConf.GetCurrentKeySet()
        for pseudoevent, handler in [
            ('<<find>>', self.find_event),
            ('<<find-again>>', self.find_again_event),
            ('<<find-selection>>', self.find_selection_event),
        ]:
            keylist = keydefs[pseudoevent]
            if keylist:
                text.event_add(pseudoevent, *keylist)
            text.bind(pseudoevent, handler)

        buttons.pack(side='bottom')

    def ok(self):
        """Dismiss text viewer dialog."""
        self.parent.destroy()
        return "break"

    def find_event(self, event):
        """Open the search dialog."""
        search.find(self.text)
        return "break"

    def find_again_event(self, event):
        """Search for the previously searched pattern again."""
        search.find_again(self.text)
        return "break"

    def find_selection_event(self, event):
        """Search for the currently selected text."""
        search.find_selection(self.text)
        return "break"


class ViewWindow(Toplevel):
    "A simple text viewer dialog for IDLE."

    def __init__(self, parent, title, contents, modal=True, wrap=WORD,
                 *, _htest=False, _utest=False):
        """Show the given text in a scrollable window.

        If modal is left True, users cannot interact with other windows
        until the textview window is closed.

        parent - parent of this dialog
        title - string which is title of popup dialog
        contents - text to display in dialog
        wrap - type of text wrapping to use ('word', 'char' or 'none')
        _htest - bool; change box location when running htest.
        _utest - bool; don't wait_window when running unittest.
        """
        super().__init__(parent)
        self['borderwidth'] = 5
        # Place dialog below parent if running htest.
        x = parent.winfo_rootx() + 10
        y = parent.winfo_rooty() + (10 if not _htest else 100)
        self.geometry(f'=800x600+{x}+{y}')

        self.title(title)
        self.viewframe = ViewFrame(self, contents, wrap=wrap)
        self.protocol("WM_DELETE_WINDOW", self.ok)
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


def view_text(parent, title, contents, modal=True, wrap='word', _utest=False):
    """Create text viewer for given text.

    parent - parent of this dialog
    title - string which is the title of popup dialog
    contents - text to display in this dialog
    wrap - type of text wrapping to use ('word', 'char' or 'none')
    modal - controls if users can interact with other windows while this
            dialog is displayed
    _utest - bool; controls wait_window on unittest
    """
    return ViewWindow(parent, title, contents, modal, wrap=wrap, _utest=_utest)


def view_file(parent, title, filename, encoding, modal=True, wrap='word',
              _utest=False):
    """Create text viewer for text in filename.

    Return error message if file cannot be read.  Otherwise calls view_text
    with contents of the file.
    """
    try:
        with open(filename, encoding=encoding) as file:
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
