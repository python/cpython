"""Simple text browser for IDLE

"""
from tkinter import Toplevel, Text, TclError,\
    HORIZONTAL, VERTICAL, NS, EW, NSEW, NONE, WORD, SUNKEN
from tkinter.ttk import Frame, Scrollbar, Button
from tkinter.messagebox import showerror

from functools import update_wrapper
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


def add_config_callback(config_callback):
    """Class decorator adding a configuration callback for Tk widgets"""
    def decorator(cls):
        class WidgetWithConfigHook(cls):
            def __setitem__(self, key, value):
                config_callback({key: value})
                super().__setitem__(key, value)

            def configure(self, cnf=None, **kw):
                if cnf is not None:
                    config_callback(cnf)
                if kw:
                    config_callback(kw)
                super().configure(cnf=cnf, **kw)

            config = configure

        update_wrapper(WidgetWithConfigHook, cls, updated=[])
        return WidgetWithConfigHook

    return decorator


class ScrollableTextFrame(Frame):
    """Display text with scrollbar(s)."""

    def __init__(self, master, **kwargs):
        """Create a frame for Textview.

        master - master widget for this frame

        The Text widget is accessible via the 'text' attribute.
        """
        super().__init__(master, **kwargs)

        @add_config_callback(self._handle_wrap)
        class TextWithConfigHook(Text):
            pass
        self.text = TextWithConfigHook(self)
        self.text.grid(row=0, column=0, sticky=NSEW)
        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)

        # vertical scrollbar
        self.yscroll = AutoHiddenScrollbar(self, orient=VERTICAL,
                                           takefocus=False,
                                           command=self.text.yview)
        self.yscroll.grid(row=0, column=1, sticky=NS)
        self.text['yscrollcommand'] = self.yscroll.set

        # horizontal scrollbar
        self.xscroll = None
        self._set_xscroll(self.text.cget('wrap'))

    def _handle_wrap(self, cnf):
        if 'wrap' in cnf:
            self._set_xscroll(cnf['wrap'])

    def _set_xscroll(self, wrap):
        """show/hide the horizontal scrollbar as per the 'wrap' setting"""
        # show the scrollbar only when wrap is set to NONE
        if wrap == NONE and self.xscroll is None:
            self.xscroll = AutoHiddenScrollbar(self, orient=HORIZONTAL,
                                               takefocus=False,
                                               command=self.text.xview)
            self.xscroll.grid(row=1, column=0, sticky=EW)
            self.text['xscrollcommand'] = self.xscroll.set
        elif wrap != NONE and self.xscroll is not None:
            self.text['xscrollcommand'] = ''
            self.xscroll.grid_forget()
            self.xscroll.destroy()
            self.xscroll = None


class ViewFrame(Frame):
    "Display TextFrame and Close button."
    def __init__(self, parent, contents, wrap='word'):
        """Create a frame for viewing text with a "Close" button.

        parent - parent widget for this frame
        contents - text to display
        wrap - type of text wrapping to use ('word', 'char' or 'none')

        The Text widget is accessible via the 'text' attribute.
        """
        super().__init__(parent)
        self.parent = parent
        self.bind('<Return>', self.ok)
        self.bind('<Escape>', self.ok)
        self.textframe = ScrollableTextFrame(self, relief=SUNKEN, height=700)

        text = self.text = self.textframe.text
        text.configure(wrap=wrap, highlightthickness=0)
        text.insert('1.0', contents)
        color_config(text)
        text['state'] = 'disabled'
        text.focus_set()

        self.button_ok = button_ok = Button(
                self, text='Close', command=self.ok, takefocus=False)
        self.textframe.pack(side='top', expand=True, fill='both')
        button_ok.pack(side='bottom')

    def ok(self, event=None):
        """Dismiss text viewer dialog."""
        self.parent.destroy()


class ViewWindow(Toplevel):
    "A simple text viewer dialog for IDLE."

    def __init__(self, parent, title, contents, modal=True, wrap=WORD,
                 *, _htest=False, _utest=False):
        """Show the given text in a scrollable window with a 'close' button.

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
        self.geometry(f'=750x500+{x}+{y}')

        self.title(title)
        self.viewframe = ViewFrame(self, contents, wrap=wrap)
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
