import os
from Tkinter import *
import tkMessageBox

from EditorWindow import EditorWindow, fixwordbreaks
from IOBinding import IOBinding


class MultiIOBinding(IOBinding):

    def open(self, event):
        filename = self.askopenfile()
        if filename:
            self.flist.open(filename, self.edit)
        return "break"


class MultiEditorWindow(EditorWindow):

    IOBinding = MultiIOBinding
    from PopupMenu import PopupMenu

    def __init__(self, flist, filename, key):
        self.flist = flist
        flist.inversedict[self] = key
        if key:
            flist.dict[key] = self
        EditorWindow.__init__(self, flist.root, filename)
        self.io.flist = flist
        self.io.edit = self
        self.popup = self.PopupMenu(self.text, self.flist)
        self.text.bind("<<open-new-window>>", self.flist.new_callback)
        self.text.bind("<<close-all-windows>>", self.flist.close_all_callback)

    def close_hook(self):
        self.flist.close_edit(self)

    def filename_change_hook(self):
        self.flist.filename_changed_edit(self)
        EditorWindow.filename_change_hook(self)


class FileList:

    def __init__(self, root):
        self.root = root
        self.dict = {}
        self.inversedict = {}

    def new(self):
        return self.open(None)

    def open(self, filename, edit=None):
        if filename:
            filename = self.canonize(filename)
            if os.path.isdir(filename):
                tkMessageBox.showerror(
                    "Is A Directory",
                    "The path %s is a directory." % `filename`,
                    master=self.root)
                return None
            key = os.path.normcase(filename)
            if self.dict.has_key(key):
                edit = self.dict[key]
                edit.top.tkraise()
                edit.top.wm_deiconify()
                edit.text.focus_set()
                return edit
            if not os.path.exists(filename):
                tkMessageBox.showinfo(
                    "New File",
                    "Opening non-existent file %s" % `filename`,
                    master=self.root)
            if edit and not edit.io.filename and edit.undo.get_saved():
                # Reuse existing Untitled window for new file
                edit.io.loadfile(filename)
                self.dict[key] = edit
                self.inversedict[edit] = key
                edit.top.tkraise()
                edit.top.wm_deiconify()
                edit.text.focus_set()
                return edit
        else:
            key = None
        edit = MultiEditorWindow(self, filename, key)
        return edit

    def new_callback(self, event):
        self.new()
        return "break"

    def close_all_callback(self, event):
        for edit in self.inversedict.keys():
            reply = edit.close()
            if reply == "cancel":
                break
        return "break"

    def close_edit(self, edit):
        try:
            key = self.inversedict[edit]
        except KeyError:
            print "Don't know this EditorWindow object.  (close)"
            return
        if key:
            del self.dict[key]
        del self.inversedict[edit]
        if not self.inversedict:
            self.root.quit()

    def filename_changed_edit(self, edit):
        edit.saved_change_hook()
        try:
            key = self.inversedict[edit]
        except KeyError:
            print "Don't know this EditorWindow object.  (rename)"
            return
        filename = edit.io.filename
        if not filename:
            if key:
                del self.dict[key]
            self.inversedict[edit] = None
            return
        filename = self.canonize(filename)
        newkey = os.path.normcase(filename)
        if newkey == key:
            return
        if self.dict.has_key(newkey):
            conflict = self.dict[newkey]
            self.inversedict[conflict] = None
            tkMessageBox.showerror(
                "Name Conflict",
                "You now have multiple edit windows open for %s" % `filename`,
                master=self.root)
        self.dict[newkey] = edit
        self.inversedict[edit] = newkey
        if key:
            try:
                del self.dict[key]
            except KeyError:
                pass

    def canonize(self, filename):
        if not os.path.isabs(filename):
            try:
                pwd = os.getcwd()
            except os.error:
                pass
            else:
                filename = os.path.join(pwd, filename)
        return os.path.normpath(filename)


def test():
    import sys
    root = Tk()
    fixwordbreaks(root)
    root.withdraw()
    flist = FileList(root)
    if sys.argv[1:]:
        for filename in sys.argv[1:]:
            flist.open(filename)
    else:
        flist.new()
    if flist.inversedict:
        root.mainloop()

if __name__ == '__main__':
    test()
