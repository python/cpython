import os
import tkFileDialog
import tkMessageBox


class IOBinding:

    # Calls to non-standard text methods:
    # reset_undo()
    # set_saved(1)

    def __init__(self, text):
        self.text = text
        self.text.bind("<<open-window-from-file>>", self.open)
        self.text.bind("<<save-window>>", self.save)
        self.text.bind("<<save-window-as-file>>", self.save_as)
        self.text.bind("<<save-copy-of-window-as-file>>", self.save_a_copy)

    filename_change_hook = None

    def set_filename_change_hook(self, hook):
        self.filename_change_hook = hook

    filename = None

    def set_filename(self, filename):
        self.filename = filename
        self.text.set_saved(1)
        if self.filename_change_hook:
            self.filename_change_hook()

    def open(self, event):
        if not self.text.get_saved():
            reply = self.maybesave()
            if reply == "cancel":
                return "break"
        filename = self.askopenfile()
        if filename:
            self.loadfile(filename)
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
        self.text.reset_undo()
        self.set_filename(filename)
        self.text.mark_set("insert", "1.0")
        self.text.see("insert")
        return 1

    def maybesave(self):
        if self.text.get_saved():
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
            if not self.text.get_saved():
                reply = "cancel"
        return reply

    def save(self, event):
        if not self.filename:
            self.save_as(event)
        else:
            if self.writefile(self.filename):
                self.text.set_saved(1)
        return "break"

    def save_as(self, event):
        filename = self.asksavefile()
        if filename:
            if self.writefile(filename):
                self.set_filename(filename)
                self.text.set_saved(1)
        return "break"

    def save_a_copy(self, event):
        filename = self.asksavefile()
        if filename:
            self.writefile(filename)
        return "break"

    def writefile(self, filename):
        try:
            f = open(filename, "w")
            chars = self.text.get("1.0", "end-1c")
            f.write(chars)
            if chars and chars[-1] != "\n":
                f.write("\n")
            f.close()
            ## print "saved to", `filename`
            return 1
        except IOError, msg:
            tkMessageBox.showerror("I/O Error", str(msg),
                                   master=self.text)
            return 0

    opendialog = None
    savedialog = None

    filetypes = [
        ("Python files", "*.py", "TEXT"),
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
            dir, base = os.path.split(self.filename)
        else:
            dir = base = ""
        return dir, base

    def asksavefile(self):
        dir, base = self.defaultfilename("save")
        if not self.savedialog:
            self.savedialog = tkFileDialog.SaveAs(master=self.text,
                                                  filetypes=self.filetypes)
        return self.savedialog.show(initialdir=dir, initialfile=base)


def test():
    from Tkinter import *
    root = Tk()
    class MyText(Text):
        def reset_undo(self): pass
        def set_saved(self, flag): pass
    text = MyText(root)
    text.pack()
    text.focus_set()
    io = IOBinding(text)
    root.mainloop()

if __name__ == "__main__":
    test()
