import os
import tkFileDialog
import tkMessageBox

#$ event <<open-window-from-file>>
#$ win <Control-o>
#$ unix <Control-x><Control-f>

#$ event <<save-window>>
#$ win <Control-s>
#$ unix <Control-x><Control-s>

#$ event <<save-window-as-file>>
#$ win <Alt-s>
#$ unix <Control-x><Control-w>

#$ event <<save-copy-of-window-as-file>>
#$ win <Alt-Shift-s>
#$ unix <Control-x><w>


class IOBinding:

    def __init__(self, editwin):
        self.editwin = editwin
        self.text = editwin.text
        self.__id_open = self.text.bind("<<open-window-from-file>>", self.open)
        self.__id_save = self.text.bind("<<save-window>>", self.save)
        self.__id_saveas = self.text.bind("<<save-window-as-file>>",
                                          self.save_as)
        self.__id_savecopy = self.text.bind("<<save-copy-of-window-as-file>>",
                                            self.save_a_copy)

    def close(self):
        # Undo command bindings
        self.text.unbind("<<open-window-from-file>>", self.__id_open)
        self.text.unbind("<<save-window>>", self.__id_save)
        self.text.unbind("<<save-window-as-file>>",self.__id_saveas)
        self.text.unbind("<<save-copy-of-window-as-file>>", self.__id_savecopy)
        # Break cycles
        self.editwin = None
        self.text = None
        self.filename_change_hook = None

    def get_saved(self):
        return self.editwin.get_saved()

    def set_saved(self, flag):
        self.editwin.set_saved(flag)

    def reset_undo(self):
        self.editwin.reset_undo()

    filename_change_hook = None

    def set_filename_change_hook(self, hook):
        self.filename_change_hook = hook

    filename = None

    def set_filename(self, filename):
        self.filename = filename
        self.set_saved(1)
        if self.filename_change_hook:
            self.filename_change_hook()

    def open(self, event):
        if self.editwin.flist:
            filename = self.askopenfile()
            if filename:
                self.editwin.flist.open(filename)
            else:
                self.text.focus_set()
            return "break"
        # Code for use outside IDLE:
        if self.get_saved():
            reply = self.maybesave()
            if reply == "cancel":
                self.text.focus_set()
                return "break"
        filename = self.askopenfile()
        if filename:
            self.loadfile(filename)
        else:
            self.text.focus_set()
        return "break"

    def loadfile(self, filename):
        try:
            f = open(filename)
            chars = f.read()
            f.close()
        except IOError, msg:
            tkMessageBox.showerror("I/O Error", str(msg), master=self.text)
            return 0
        self.text.delete("1.0", "end")
        self.set_filename(None)
        self.text.insert("1.0", chars)
        self.reset_undo()
        self.set_filename(filename)
        self.text.mark_set("insert", "1.0")
        self.text.see("insert")
        return 1

    def maybesave(self):
        if self.get_saved():
            return "yes"
        message = "Do you want to save %s before closing?" % (
            self.filename or "this untitled document")
        m = tkMessageBox.Message(
            title="Save On Close",
            message=message,
            icon=tkMessageBox.QUESTION,
            type=tkMessageBox.YESNOCANCEL,
            master=self.text)
        reply = m.show()
        if reply == "yes":
            self.save(None)
            if not self.get_saved():
                reply = "cancel"
        self.text.focus_set()
        return reply

    def save(self, event):
        if not self.filename:
            self.save_as(event)
        else:
            if self.writefile(self.filename):
                self.set_saved(1)
        self.text.focus_set()
        return "break"

    def save_as(self, event):
        filename = self.asksavefile()
        if filename:
            if self.writefile(filename):
                self.set_filename(filename)
                self.set_saved(1)
        self.text.focus_set()
        return "break"

    def save_a_copy(self, event):
        filename = self.asksavefile()
        if filename:
            self.writefile(filename)
        self.text.focus_set()
        return "break"

    def writefile(self, filename):
        self.fixlastline()
        try:
            f = open(filename, "w")
            chars = self.text.get("1.0", "end-1c")
            f.write(chars)
            f.close()
            ## print "saved to", `filename`
            return 1
        except IOError, msg:
            tkMessageBox.showerror("I/O Error", str(msg),
                                   master=self.text)
            return 0

    def fixlastline(self):
        c = self.text.get("end-2c")
        if c != '\n':
            self.text.insert("end-1c", "\n")

    opendialog = None
    savedialog = None

    filetypes = [
        ("Python and text files", "*.py *.pyw *.txt", "TEXT"),
        ("All text files", "*", "TEXT"),
        ("All files", "*"),
        ]

    def askopenfile(self):
        dir, base = self.defaultfilename("open")
        if not self.opendialog:
            self.opendialog = tkFileDialog.Open(master=self.text,
                                                filetypes=self.filetypes)
        return self.opendialog.show(initialdir=dir, initialfile=base)

    def defaultfilename(self, mode="open"):
        if self.filename:
            return os.path.split(self.filename)
        else:
            try:
                pwd = os.getcwd()
            except os.error:
                pwd = ""
            return pwd, ""

    def asksavefile(self):
        dir, base = self.defaultfilename("save")
        if not self.savedialog:
            self.savedialog = tkFileDialog.SaveAs(master=self.text,
                                                  filetypes=self.filetypes)
        return self.savedialog.show(initialdir=dir, initialfile=base)


def test():
    from Tkinter import *
    root = Tk()
    class MyEditWin:
        def __init__(self, text):
            self.text = text
            self.flist = None
            self.text.bind("<Control-o>", self.open)
            self.text.bind("<Control-s>", self.save)
            self.text.bind("<Alt-s>", self.save_as)
            self.text.bind("<Alt-z>", self.save_a_copy)
        def get_saved(self): return 0
        def set_saved(self, flag): pass
        def reset_undo(self): pass
        def open(self, event):
            self.text.event_generate("<<open-window-from-file>>")
        def save(self, event):
            self.text.event_generate("<<save-window>>")
        def save_as(self, event):
            self.text.event_generate("<<save-window-as-file>>")
        def save_a_copy(self, event):
            self.text.event_generate("<<save-copy-of-window-as-file>>")
    text = Text(root)
    text.pack()
    text.focus_set()
    editwin = MyEditWin(text)
    io = IOBinding(editwin)
    root.mainloop()

if __name__ == "__main__":
    test()
