"""File selection dialog classes.

Classes:

- FileDialog
- LoadFileDialog
- SaveFileDialog

XXX Bugs:

- The fields are not labeled
- Default doesn't have absolute pathname
- Each FileDialog instance can be used only once
- There is no easy way for an application to add widgets of its own

"""

from Tkinter import *
from Dialog import Dialog

ANCHOR = 'anchor'

import os
import fnmatch


class FileDialog:

    """Standard file selection dialog -- no checks on selected file.

    Usage:

        d = FileDialog(master)
        file = d.go(directory, pattern, default)
        if file is None: ...canceled...

    """

    title = "File Selection Dialog"

    def __init__(self, master):
	self.master = master
	self.directory = None
	self.top = Toplevel(master)
	self.top.title(self.title)
	self.filter = Entry(self.top)
	self.filter.pack(fill=X)
	self.filter.bind('<Return>', self.filter_command)
	self.midframe = Frame(self.top)
	self.midframe.pack(expand=YES, fill=BOTH)
	self.dirs = Listbox(self.midframe)
	self.dirs.pack(side=LEFT, expand=YES, fill=BOTH)
	self.dirs.bind('<ButtonRelease-1>', self.dirs_select_event)
	self.dirs.bind('<Double-ButtonRelease-1>', self.dirs_double_event)
	self.files = Listbox(self.midframe)
	self.files.pack(side=RIGHT, expand=YES, fill=BOTH)
	self.files.bind('<ButtonRelease-1>', self.files_select_event)
	self.files.bind('<Double-ButtonRelease-1>', self.files_double_event)
	self.selection = Entry(self.top)
	self.selection.pack(fill=X)
	self.selection.bind('<Return>', self.ok_event)
	self.botframe = Frame(self.top)
	self.botframe.pack(fill=X)
	self.ok_button = Button(self.botframe,
				 text="OK",
				 command=self.ok_command)
	self.ok_button.pack(side=LEFT)
	self.filter_button = Button(self.botframe,
				    text="Filter",
				    command=self.filter_command)
	self.filter_button.pack(side=LEFT, expand=YES)
	self.cancel_button = Button(self.botframe,
				    text="Cancel",
				    command=self.cancel_command)
	self.cancel_button.pack(side=RIGHT)

    def go(self, directory=os.curdir, pattern="*", default=""):
	self.directory = directory
	self.set_filter(directory, pattern)
	self.filter_command()
	self.set_selection(default)
	self.selection.focus_set()
	self.top.grab_set()
	try:
	    self.master.mainloop()
	except SystemExit, how:
	    self.top.destroy()
	    return how

    def dirs_double_event(self, event):
##	self.dirs_select_event(event)
	self.filter_command()

    def dirs_select_event(self, event):
	dir, pat = self.get_filter()
	subdir = self.dirs.get(ANCHOR)
	dir = os.path.normpath(os.path.join(self.directory, subdir))
	self.set_filter(dir, pat)

    def files_double_event(self, event):
##	self.files_select_event(event)
##	self.master.update_idletasks()
	self.ok_command()

    def files_select_event(self, event):
	file = self.files.get(ANCHOR)
	self.set_selection(file)

    def ok_event(self, event):
	self.ok_command()

    def ok_command(self):
	raise SystemExit, self.selection.get()

    def filter_command(self, event=None):
	dir, pat = self.get_filter()
	try:
	    names = os.listdir(dir)
	except os.error:
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
	head, tail = os.path.split(self.selection.get())
	if tail == os.curdir: tail = ''
	self.set_selection(tail)

    def get_filter(self):
	filter = self.filter.get()
	if filter[-1:] == os.sep:
	    filter = filter + "*"
	return os.path.split(filter)

    def cancel_command(self):
	raise SystemExit, None

    def set_filter(self, dir, pat):
	self.filter.delete(0, END)
	self.filter.insert(END, os.path.join(dir or os.curdir, pat or "*"))

    def set_selection(self, file):
	self.selection.delete(0, END)
	self.selection.insert(END, os.path.join(self.directory, file))


class LoadFileDialog(FileDialog):

    """File selection dialog which checks that the file exists."""

    title = "Load File Selection Dialog"

    def ok_command(self):
	file = self.selection.get()
	if not os.path.isfile(file):
	    self.master.bell()
	else:
	    raise SystemExit, file


class SaveFileDialog(FileDialog):

    """File selection dialog which checks that the file may be created."""

    title = "Save File Selection Dialog"

    def ok_command(self):
	file = self.selection.get()
	if os.path.exists(file):
	    if os.path.isdir(file):
		self.master.bell()
		return
	    d = Dialog(self.master,
		       title="Overwrite Existing File Question",
		       text="Overwrite existing file %s?" % `file`,
		       bitmap='questhead',
		       default=0,
		       strings=("Yes", "Cancel"))
	    if d.num != 0: file = None
	else:
	    head, tail = os.path.split(file)
	    if not os.path.isdir(head):
		self.master.bell()
		return
	raise SystemExit, file


def test():
    """Simple test program."""
    root = Tk()
    root.withdraw()
    fd = LoadFileDialog(root)
    loadfile = fd.go()
    fd = SaveFileDialog(root)
    savefile = fd.go()
    print loadfile, savefile


if __name__ == '__main__':
    test()
