import os
from Tkinter import *
import tkMessageBox

import WindowList

#$ event <<open-new-window>>
#$ win <Control-n>
#$ unix <Control-x><Control-n>

# (This is labeled as 'Exit'in the File menu)
#$ event <<close-all-windows>>
#$ win <Control-q>
#$ unix <Control-x><Control-c>

class FileList:

    from EditorWindow import EditorWindow
    EditorWindow.Toplevel = WindowList.ListedToplevel # XXX Patch it!

    def __init__(self, root):
        self.root = root
        self.dict = {}
        self.inversedict = {}
        self.vars = {} # For EditorWindow.getrawvar (shared Tcl variables)


    def goodname(self, filename):
            filename = self.canonize(filename)
            key = os.path.normcase(filename)
            if self.dict.has_key(key):
                edit = self.dict[key]
                filename = edit.io.filename or filename
            return filename

    def open(self, filename):
        assert filename
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
            edit.wakeup()
            return edit
        if not os.path.exists(filename):
            tkMessageBox.showinfo(
                "New File",
                "Opening non-existent file %s" % `filename`,
                master=self.root)
        return self.EditorWindow(self, filename, key)

    def new(self):
        return self.EditorWindow(self)

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
    from EditorWindow import fixwordbreaks
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
