# changes by dscherer@cmu.edu
#   - IOBinding.open() replaces the current window with the opened file,
#     if the current window is both unmodified and unnamed
#   - IOBinding.loadfile() interprets Windows, UNIX, and Macintosh
#     end-of-line conventions, instead of relying on the standard library,
#     which will only understand the local convention.

import os
import tempfile
import tkFileDialog
import tkMessageBox
import re
from configHandler import idleConf

#$ event <<open-window-from-file>>
#$ win <Control-o>
#$ unix <Control-x><Control-f>

#$ event <<save-window>>
#$ win <Control-s>
#$ unix <Control-x><Control-s>

#$ event <<save-window-as-file>>
#$ win <Alt-s>
#$ unix <Control-x><Control-w>

#$ event <<print-window>>
#$ win <Control-p>
#$ unix <Control-x><Control-p>

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
        self.__id_print = self.text.bind("<<print-window>>", self.print_window)
        
    def close(self):
        # Undo command bindings
        self.text.unbind("<<open-window-from-file>>", self.__id_open)
        self.text.unbind("<<save-window>>", self.__id_save)
        self.text.unbind("<<save-window-as-file>>",self.__id_saveas)
        self.text.unbind("<<save-copy-of-window-as-file>>", self.__id_savecopy)
        self.text.unbind("<<print-window>>", self.__id_print)
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

    def open(self, event=None, editFile=None):
        if self.editwin.flist:
            if not editFile:
                filename = self.askopenfile()
            else:
                filename=editFile
            if filename:
                # If the current window has no filename and hasn't been
                # modified, we replace its contents (no loss).  Otherwise
                # we open a new window.  But we won't replace the
                # shell window (which has an interp(reter) attribute), which
                # gets set to "not modified" at every new prompt.
                try:
                    interp = self.editwin.interp
                except:
                    interp = None
                if not self.filename and self.get_saved() and not interp:
                    self.editwin.flist.open(filename, self.loadfile)
                else:
                    self.editwin.flist.open(filename)
            else:
                self.text.focus_set()
            return "break"
        #
        # Code for use outside IDLE:
        if self.get_saved():
            reply = self.maybesave()
            if reply == "cancel":
                self.text.focus_set()
                return "break"
        if not editFile:
            filename = self.askopenfile()
        else:
            filename=editFile
        if filename:
            self.loadfile(filename)
        else:
            self.text.focus_set()
        return "break"

    def loadfile(self, filename):
        try:
            # open the file in binary mode so that we can handle
            #   end-of-line convention ourselves.
            f = open(filename,'rb')
            chars = f.read()
            f.close()
        except IOError, msg:
            tkMessageBox.showerror("I/O Error", str(msg), master=self.text)
            return 0

        # We now convert all end-of-lines to '\n's
        eol = r"(\r\n)|\n|\r"  # \r\n (Windows), \n (UNIX), or \r (Mac)
        chars = re.compile( eol ).sub( r"\n", chars )

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
            chars = str(self.text.get("1.0", "end-1c"))
            f.write(chars)
            f.close()
            ## print "saved to", `filename`
            return 1
        except IOError, msg:
            tkMessageBox.showerror("I/O Error", str(msg),
                                   master=self.text)
            return 0
 
    def print_window(self, event):
        tempfilename = None
        if self.get_saved():
            filename = self.filename
        else:
            filename = tempfilename = tempfile.mktemp()
            if not self.writefile(filename):
                os.unlink(tempfilename)
                return "break"
        platform=os.name
        printPlatform=1
        if platform == 'posix': #posix platform
            command = idleConf.GetOption('main','General','print-command-posix')
            command = command + " 2>&1"
        elif platform == 'nt': #win32 platform
            command = idleConf.GetOption('main','General','print-command-win')
        else: #no printing for this platform
            printPlatform=0
        if printPlatform:  #we can try to print for this platform
            command = command % filename
            pipe = os.popen(command, "r")
            output = pipe.read().strip()
            status = pipe.close()
            if status:
                output = "Printing failed (exit status 0x%x)\n" % status + output
            if output:
                output = "Printing command: %s\n" % repr(command) + output
                tkMessageBox.showerror("Print status", output, master=self.text)
        else:  #no printing for this platform
            message="Printing is not enabled for this platform: %s" % platform 
            tkMessageBox.showinfo("Print status", message, master=self.text)
        return "break"
    
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
    from Tkinter import *
    test()
