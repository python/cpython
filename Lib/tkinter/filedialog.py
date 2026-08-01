"""File selection dialog classes.

Classes:

- FileDialog
- LoadFileDialog
- SaveFileDialog

This module also presents tk common file dialogues, it provides interfaces
to the native file dialogues available in Tk 4.2 and newer, and the
directory dialogue available in Tk 8.3 and newer.
These interfaces were written by Fredrik Lundh, May 1997.
"""
__all__ = ["FileDialog", "LoadFileDialog", "SaveFileDialog",
           "Open", "SaveAs", "Directory",
           "askopenfilename", "asksaveasfilename", "askopenfilenames",
           "askopenfile", "askopenfiles", "asksaveasfile", "askdirectory"]

import fnmatch
import os
import tkinter
from tkinter import (
    ACTIVE, CENTER, EW, NORMAL, NS, NSEW, RAISED, W, YES, BOTTOM, TOP, Tk, X,
    Toplevel, END, Listbox, BOTH,
)
from tkinter import ttk
from tkinter import messagebox
from tkinter import commondialog
from tkinter.simpledialog import (_setup_dialog, _place_window, _temp_grab_focus,
                                  _underline_ampersand, _find_alt_key_target)


dialogstates = {}


class FileDialog:

    """Standard file selection dialog -- no checks on selected file.

    The layout and behavior follow the classic Motif file selection dialog.

    Usage:

        d = FileDialog(master)
        fname = d.go(dir_or_file, pattern, default, key)
        if fname is None: ...canceled...
        else: ...open file...

    All arguments to go() are optional.

    The 'key' argument specifies a key in the global dictionary
    'dialogstates', which keeps track of the values for the directory
    and pattern arguments, overriding the values passed in (it does
    not keep track of the default argument!).  If no key is specified,
    the dialog keeps no memory of previous state.  Note that memory is
    kept even when the dialog is canceled.  (All this emulates the
    behavior of the Macintosh file selection dialogs.)

    """

    title = "File Selection Dialog"

    def _widget(self, klass, master, **kw):
        # Create a themed (ttk) or classic (tkinter) widget.  ttk has no
        # Listbox, so the directory and file lists stay classic.
        return getattr(ttk if self.use_ttk else tkinter, klass)(master, **kw)

    def _frame(self, master):
        # A structural frame.  The classic file dialog gives its frames a
        # raised border on X11; the themed one does not (cf. tk_dialog).
        frame = self._widget('Frame', master)
        if not self.use_ttk and self.top._windowingsystem == 'x11':
            frame.configure(relief=RAISED, bd=1)
        return frame

    def _make_list(self, column, label, browse, activate):
        # Create a labelled listbox with vertical and horizontal scrollbars in
        # the middle frame, like the Motif file dialog (cf. xmfbox.tcl
        # MotifFDialog_MakeSList).
        frame = self._widget('Frame', self.midframe)
        frame.grid(row=0, column=column, sticky=NSEW, padx='3p', pady='3p')
        frame.grid_rowconfigure(1, weight=1)
        frame.grid_columnconfigure(0, weight=1)
        vbar = self._widget('Scrollbar', frame, takefocus=0)
        hbar = self._widget('Scrollbar', frame, orient='horizontal', takefocus=0)
        listbox = Listbox(frame, exportselection=0,
                          xscrollcommand=(hbar, 'set'),
                          yscrollcommand=(vbar, 'set'))
        vbar.configure(command=(listbox, 'yview'))
        hbar.configure(command=(listbox, 'xview'))
        self._label(frame, label, listbox).grid(
            row=0, column=0, columnspan=2, sticky=EW, padx='1.5p', pady='1.5p')
        listbox.grid(row=1, column=0, sticky=NSEW)
        vbar.grid(row=1, column=1, sticky=NS)
        hbar.grid(row=2, column=0, sticky=EW)
        # Instance bindings fire after the Listbox class bindings, which have
        # already updated the selection.
        btags = listbox.bindtags()
        listbox.bindtags(btags[1:] + btags[:1])
        listbox.bind('<<ListboxSelect>>', browse)
        listbox.bind('<Double-ButtonRelease-1>', activate)
        listbox.bind('<Return>', lambda e: (browse(e), activate(e)))
        # Type a few characters to jump to a matching entry, like the Motif
        # file dialog (cf. xmfbox.tcl ListBoxKeyAccel).
        listbox.bind('<Key>', self._listbox_keyaccel)
        return listbox, vbar, hbar

    def __init__(self, master, title=None, *, use_ttk=True):
        if title is None: title = self.title
        self.master = master
        self.use_ttk = use_ttk
        self.directory = None
        self._keyaccel = {}        # listbox -> recently typed prefix
        self._keyaccel_after = {}  # listbox -> pending reset callback id

        self.top = Toplevel(master)
        self.top.withdraw()  # remain invisible until placed by go()
        self.top.title(title)
        self.top.iconname(title)
        # Keep the dialog above its parent, like SimpleDialog and Dialog (cf.
        # xmfbox.tcl).  Skip it when the master is not viewable, or the dialog
        # would itself be opened withdrawn.
        if master.winfo_viewable():
            self.top.transient(master)
        _setup_dialog(self.top)
        if self.use_ttk:
            # Use a single themed background for the whole dialog so it blends
            # with the ttk widgets (cf. tk::MessageBox).
            self.top.configure(
                background=ttk.Style(self.top).lookup('.', 'background'))

        # Tk traverses focus in widget-creation order, so the widgets are
        # created top to bottom -- filter, lists, selection, buttons -- to make
        # Tab follow the visual layout, like the Motif file dialog (cf.
        # xmfbox.tcl).
        self.filter = self._widget('Entry', self.top)
        self.filter.bind('<Return>', self.filter_command)
        self.filter_label = self._label(self.top, 'Fil&ter:', self.filter)
        self.filter_label.pack(side=TOP, fill=X, padx='4.5p', pady='3p')
        self.filter.pack(side=TOP, fill=X, padx='3p')

        self.midframe = self._frame(self.top)
        self.midframe.pack(side=TOP, expand=YES, fill=BOTH)

        # Directory list (left) and file list (right), each with a label and
        # vertical and horizontal scrollbars, like the Motif file dialog (cf.
        # xmfbox.tcl).
        self.dirs, self.dirsbar, self.dirshbar = self._make_list(
            0, '&Directory:', self.dirs_select_event, self.dirs_double_event)
        self.files, self.filesbar, self.fileshbar = self._make_list(
            1, 'Fi&les:', self.files_select_event, self.files_double_event)

        # Give the file list twice the width of the directory list, like the
        # Motif file dialog (cf. xmfbox.tcl).
        self.midframe.grid_rowconfigure(0, weight=1)
        self.midframe.grid_columnconfigure(0, weight=1)
        self.midframe.grid_columnconfigure(1, minsize=150, weight=2)

        self.selection = self._widget('Entry', self.top)
        self.selection.bind('<Return>', self.ok_event)
        self.selection_label = self._label(self.top, '&Selection:', self.selection)
        self.selection.pack(side=BOTTOM, fill=X, padx='3p', pady='3p')
        self.selection_label.pack(side=BOTTOM, fill=X, padx='4.5p')

        # Created last so the buttons traverse last, but packed below the
        # selection field.
        self.botframe = self._frame(self.top)
        self.botframe.pack(side=BOTTOM, fill=X, before=self.selection)

        # tk::MessageBox and tk_dialog space the buttons differently.
        padx, pady = ('3m', '2m') if self.use_ttk else ('7.5p', '3p')
        for i, (name, label, command) in enumerate((
                ('ok', '&OK', self.ok_command),
                ('filter', '&Filter', self.filter_command),
                ('cancel', '&Cancel', self.cancel_command))):
            # Create a button with an accelerator key marked by "&" in the text,
            # like SimpleDialog and Dialog (cf. tk::AmpWidget).
            text, underline = _underline_ampersand(label)
            button = self._widget('Button', self.botframe, name=name, text=text,
                                  underline=underline, command=command,
                                  default=ACTIVE if name == 'ok' else NORMAL)
            button.bind('<<AltUnderlined>>', lambda e: e.widget.invoke())
            button.grid(column=i, row=0, sticky=EW, padx=padx, pady=pady)
            # tk::MessageBox makes the buttons equal width; tk_dialog does not.
            self.botframe.grid_columnconfigure(
                i, uniform='buttons' if self.use_ttk else '')
            if self.top._windowingsystem == 'aqua':
                self.botframe.grid_columnconfigure(i, minsize=90)
                button.grid_configure(pady=7)
            setattr(self, name + '_button', button)
        self.botframe.grid_anchor(CENTER)

        self.top.protocol('WM_DELETE_WINDOW', self.cancel_command)
        # Alt + an underlined character invokes the matching button (cf.
        # ::tk::AltKeyInDialog), like SimpleDialog and Dialog.
        self.top.bind('<Alt-Key>', self._alt_key)
        # The default ring follows the keyboard focus among the buttons (cf.
        # tk::MessageBox), like SimpleDialog and Dialog.
        self.top.bind('<FocusIn>', lambda e: self._set_default(e.widget, ACTIVE))
        self.top.bind('<FocusOut>', lambda e: self._set_default(e.widget, NORMAL))
        self.top.bind('<Escape>', self.cancel_command)

    def go(self, dir_or_file=os.curdir, pattern="*", default="", key=None):
        if key and key in dialogstates:
            self.directory, pattern = dialogstates[key]
        else:
            dir_or_file = os.path.expanduser(dir_or_file)
            if os.path.isdir(dir_or_file):
                self.directory = dir_or_file
            else:
                self.directory, default = os.path.split(dir_or_file)
        self.set_filter(self.directory, pattern)
        self.set_selection(default)
        self.filter_command()
        # Center the dialog over its parent and make it visible, like
        # SimpleDialog and Dialog (cf. xmfbox.tcl).
        _place_window(self.top, self.master)
        self.selection.select_range(0, END)  # so the user can type a new name
        self.top.wait_visibility() # window needs to be visible for the grab
        self.how = None
        # Grab the input and restore the previous focus and grab afterwards,
        # like SimpleDialog and Dialog.  go() destroys the window itself below,
        # so leave it in place here.
        with _temp_grab_focus(self.top, self.selection, destroy=False):
            self.master.mainloop()          # Exited by self.quit(how)
        if key:
            directory, pattern = self.get_filter()
            if self.how:
                directory = os.path.dirname(self.how)
            dialogstates[key] = directory, pattern
        self.top.destroy()
        return self.how

    def quit(self, how=None):
        self.how = how
        self.master.quit()              # Exit mainloop()

    def _alt_key(self, event):
        # Invoke the button whose accelerator matches the Alt key (cf.
        # SimpleDialog and Dialog).
        target = _find_alt_key_target(self.top, event.char)
        if target is not None:
            target.event_generate('<<AltUnderlined>>')

    def _set_default(self, widget, state):
        # Set a button's default ring.
        if widget.winfo_class() in ('Button', 'TButton'):
            widget.configure(default=state)

    def _label(self, master, label, target):
        # Create a field label whose "&" accelerator focuses the target widget,
        # like the labels of the Motif file dialog (cf. xmfbox.tcl).
        text, underline = _underline_ampersand(label)
        widget = self._widget('Label', master, text=text, underline=underline,
                              anchor=W)
        widget.bind('<<AltUnderlined>>', lambda e: target.focus_set())
        return widget

    def _listbox_keyaccel(self, event):
        # Append the typed character and jump to a matching entry, resetting
        # after a short pause (cf. xmfbox.tcl ListBoxKeyAccel_Key).
        char = event.char
        if not char or not char.isprintable():
            return
        listbox = event.widget
        prefix = self._keyaccel.get(listbox, '') + char
        self._keyaccel[listbox] = prefix
        self._listbox_keyaccel_goto(listbox, prefix)
        after_id = self._keyaccel_after.get(listbox)
        if after_id is not None:
            listbox.after_cancel(after_id)
        self._keyaccel_after[listbox] = listbox.after(
            500, lambda: self._keyaccel.pop(listbox, None))

    def _listbox_keyaccel_goto(self, listbox, prefix):
        # Select the first entry not preceding the typed prefix (cf. xmfbox.tcl
        # ListBoxKeyAccel_Goto).
        prefix = prefix.lower()
        index = -1
        for i in range(listbox.size()):
            item = listbox.get(i).lower()
            if prefix >= item:
                index = i
            if prefix <= item:
                index = i
                break
        if index >= 0:
            listbox.selection_clear(0, END)
            listbox.selection_set(index)
            listbox.activate(index)
            listbox.see(index)
            listbox.event_generate('<<ListboxSelect>>')

    def dirs_double_event(self, event):
        self.filter_command()
        # Highlight an entry so the directory list stays keyboard-navigable
        # after entering a directory, like the Motif file dialog (cf.
        # xmfbox.tcl); prefer the first subdirectory over the ".." entry.
        index = 1 if self.dirs.size() > 1 else 0
        self.dirs.selection_set(index)
        self.dirs.activate(index)

    def dirs_select_event(self, event):
        # Show a selection in only one list at a time (cf. xmfbox.tcl).
        self.files.selection_clear(0, END)
        dir, pat = self.get_filter()
        subdir = self.dirs.get('active')
        dir = os.path.normpath(os.path.join(self.directory, subdir))
        self.set_filter(dir, pat)
        # Show the end of a long path, like the Motif file dialog (cf.
        # xmfbox.tcl).
        self.filter.xview(END)

    def files_double_event(self, event):
        self.ok_command()

    def files_select_event(self, event):
        # Show a selection in only one list at a time (cf. xmfbox.tcl).
        self.dirs.selection_clear(0, END)
        file = self.files.get('active')
        self.set_selection(file)
        self.selection.xview(END)

    def ok_event(self, event):
        self.ok_command()

    def ok_command(self):
        self.quit(self.get_selection())

    def filter_command(self, event=None):
        dir, pat = self.get_filter()
        try:
            names = os.listdir(dir)
        except OSError:
            self.master.bell()
            return
        self.directory = dir
        self.set_filter(dir, pat)
        names.sort()
        subdirs = [os.pardir]
        matchingfiles = []
        for name in names:
            fullname = os.path.join(dir, name)
            if os.path.isdir(fullname):
                subdirs.append(name)
            elif fnmatch.fnmatch(name, pat):
                matchingfiles.append(name)
        self.dirs.delete(0, END)
        for name in subdirs:
            self.dirs.insert(END, name)
        self.files.delete(0, END)
        for name in matchingfiles:
            self.files.insert(END, name)
        head, tail = os.path.split(self.get_selection())
        if tail == os.curdir: tail = ''
        self.set_selection(tail)

    def get_filter(self):
        filter = self.filter.get()
        filter = os.path.expanduser(filter)
        if filter[-1:] == os.sep or os.path.isdir(filter):
            filter = os.path.join(filter, "*")
        return os.path.split(filter)

    def get_selection(self):
        file = self.selection.get()
        file = os.path.expanduser(file)
        return file

    def cancel_command(self, event=None):
        self.quit()

    def set_filter(self, dir, pat):
        if not os.path.isabs(dir):
            try:
                pwd = os.getcwd()
            except OSError:
                pwd = None
            if pwd:
                dir = os.path.join(pwd, dir)
                dir = os.path.normpath(dir)
        self.filter.delete(0, END)
        self.filter.insert(END, os.path.join(dir or os.curdir, pat or "*"))

    def set_selection(self, file):
        self.selection.delete(0, END)
        self.selection.insert(END, os.path.join(self.directory, file))


class LoadFileDialog(FileDialog):

    """File selection dialog which checks that the file exists."""

    title = "Load File Selection Dialog"

    def ok_command(self):
        file = self.get_selection()
        if not os.path.isfile(file):
            self.master.bell()
        else:
            self.quit(file)


class SaveFileDialog(FileDialog):

    """File selection dialog which checks that the file may be created."""

    title = "Save File Selection Dialog"

    def ok_command(self):
        file = self.get_selection()
        if os.path.exists(file):
            if os.path.isdir(file):
                self.master.bell()
                return
            if not messagebox.askyesno(
                    "Overwrite Existing File",
                    "Overwrite existing file %r?" % (file,),
                    icon=messagebox.WARNING, parent=self.top):
                return
        else:
            head, tail = os.path.split(file)
            if not os.path.isdir(head):
                self.master.bell()
                return
        self.quit(file)


# For the following classes and modules:
#
# options (all have default values):
#
# - defaultextension: added to filename if not explicitly given
#
# - filetypes: sequence of (label, pattern) tuples.  the same pattern
#   may occur with several patterns.  use "*" as pattern to indicate
#   all files.
#
# - initialdir: initial directory.  preserved by dialog instance.
#
# - initialfile: initial file (ignored by the open dialog).  preserved
#   by dialog instance.
#
# - parent: which window to place the dialog on top of
#
# - title: dialog title
#
# - multiple: if true user may select more than one file
#
# options for the directory chooser:
#
# - initialdir, parent, title: see above
#
# - mustexist: if true, user must pick an existing directory
#


class _Dialog(commondialog.Dialog):

    def _fixoptions(self):
        try:
            # make sure "filetypes" is a tuple
            self.options["filetypes"] = tuple(self.options["filetypes"])
        except KeyError:
            pass

    def _fixresult(self, widget, result):
        if not result:
            result = ''  # normalize the cancelled result (gh-103878)
        else:
            # keep directory and filename until next time
            # convert Tcl path objects to strings
            try:
                result = result.string
            except AttributeError:
                # it already is a string
                pass
            path, file = os.path.split(result)
            self.options["initialdir"] = path
            self.options["initialfile"] = file
        self.filename = result # compatibility
        return result


#
# file dialogs

class Open(_Dialog):
    "Ask for a filename to open"

    command = "tk_getOpenFile"

    def _fixresult(self, widget, result):
        if self.options.get("multiple"):
            # multiple results: a tuple of filenames
            if not isinstance(result, tuple):
                result = widget.tk.splitlist(result)
            result = tuple([getattr(r, "string", r) for r in result])
            if result:
                path, file = os.path.split(result[0])
                self.options["initialdir"] = path
                # don't set initialfile or filename, as we have multiple of these
            return result
        return _Dialog._fixresult(self, widget, result)


class SaveAs(_Dialog):
    "Ask for a filename to save as"

    command = "tk_getSaveFile"


# the directory dialog has its own _fix routines.
class Directory(commondialog.Dialog):
    "Ask for a directory"

    command = "tk_chooseDirectory"

    def _fixresult(self, widget, result):
        if not result:
            result = ''  # normalize the cancelled result (gh-103878)
        else:
            # convert Tcl path objects to strings
            try:
                result = result.string
            except AttributeError:
                # it already is a string
                pass
            # keep directory until next time
            self.options["initialdir"] = result
        self.directory = result # compatibility
        return result

#
# convenience stuff


def askopenfilename(**options):
    "Ask for a filename to open"

    return Open(**options).show()


def asksaveasfilename(**options):
    "Ask for a filename to save as"

    return SaveAs(**options).show()


def askopenfilenames(**options):
    """Ask for multiple filenames to open

    Returns a list of filenames or empty list if
    cancel button selected
    """
    options["multiple"]=1
    return Open(**options).show()

# FIXME: are the following  perhaps a bit too convenient?


def askopenfile(mode = "r", **options):
    "Ask for a filename to open, and returned the opened file"

    filename = Open(**options).show()
    if filename:
        return open(filename, mode)
    return None


def askopenfiles(mode = "r", **options):
    """Ask for multiple filenames and return the open file
    objects

    returns a list of open file objects or an empty list if
    cancel selected
    """
    import warnings
    warnings._deprecated(
        "tkinter.filedialog.askopenfiles",
        message=f"{warnings._DEPRECATED_MSG}; iterate over the names returned "
                "by askopenfilenames() and open them instead",
        remove=(3, 19))

    files = askopenfilenames(**options)
    return [open(filename, mode) for filename in files]


def asksaveasfile(mode = "w", **options):
    "Ask for a filename to save as, and returned the opened file"

    filename = SaveAs(**options).show()
    if filename:
        return open(filename, mode)
    return None


def askdirectory (**options):
    "Ask for a directory, and return the file name"
    return Directory(**options).show()


# --------------------------------------------------------------------
# test stuff

def test():
    """Simple test program."""
    root = Tk()
    use_ttk = tkinter.BooleanVar(root, value=True)

    def test_load():
        print('load:', LoadFileDialog(root, use_ttk=use_ttk.get()).go(key='test'))

    def test_save():
        print('save:', SaveFileDialog(root, use_ttk=use_ttk.get()).go(key='test'))

    def test_open():
        print('open:', askopenfilename(filetypes=[('all files', '*')]))

    def test_saveas():
        print('saveas:', asksaveasfilename())

    def test_directory():
        print('directory:', askdirectory())

    def test_directory_mustexist():
        print('directory (mustexist):', askdirectory(mustexist=True))

    tkinter.Checkbutton(root, text='Use themed (ttk) widgets',
                        variable=use_ttk).pack(fill=X)
    tkinter.Button(root, text='LoadFileDialog', command=test_load).pack(fill=X)
    tkinter.Button(root, text='SaveFileDialog', command=test_save).pack(fill=X)
    tkinter.Button(root, text='askopenfilename', command=test_open).pack(fill=X)
    tkinter.Button(root, text='asksaveasfilename', command=test_saveas).pack(fill=X)
    tkinter.Button(root, text='askdirectory', command=test_directory).pack(fill=X)
    tkinter.Button(root, text='askdirectory (mustexist)',
                   command=test_directory_mustexist).pack(fill=X)
    tkinter.Button(root, text='Quit', command=root.quit).pack(fill=X)
    root.mainloop()


if __name__ == '__main__':
    test()
