import os
import types
import sys
import codecs
import re
import tempfile
import tkFileDialog
import tkMessageBox
from IdleConf import idleconf

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

#$ event <<print-window>>
#$ win <Control-p>
#$ unix <Control-x><Control-p>

try:
    from codecs import BOM_UTF8
except ImportError:
    # only available since Python 2.3
    BOM_UTF8 = '\xef\xbb\xbf'

# Try setting the locale, so that we can find out
# what encoding to use
try:
    import locale
    locale.setlocale(locale.LC_CTYPE, "")
except ImportError:
    pass

encoding = "ascii"
if sys.platform == 'win32':
    # On Windows, we could use "mbcs". However, to give the user
    # a portable encoding name, we need to find the code page
    try:
        encoding = locale.getdefaultlocale()[1]
        codecs.lookup(encoding)
    except LookupError:
        pass
else:
    try:
        # Different things can fail here: the locale module may not be
        # loaded, it may not offer nl_langinfo, or CODESET, or the
        # resulting codeset may be unknown to Python. We ignore all
        # these problems, falling back to ASCII
        encoding = locale.nl_langinfo(locale.CODESET)
        codecs.lookup(encoding)
    except (NameError, AttributeError, LookupError):
        # Try getdefaultlocale well: it parses environment variables,
        # which may give a clue. Unfortunately, getdefaultlocale has
        # bugs that can cause ValueError.
        try:
            encoding = locale.getdefaultlocale()[1]
            codecs.lookup(encoding)
        except (ValueError, LookupError):
            pass

encoding = encoding.lower()

coding_re = re.compile("coding[:=]\s*([-\w_.]+)")
def coding_spec(str):

    """Return the encoding declaration according to PEP 263.
    Raise LookupError if the encoding is declared but unknown."""

    # Only consider the first two lines
    str = str.split("\n")[:2]
    str = "\n".join(str)

    match = coding_re.search(str)
    if not match:
        return None
    name = match.group(1)
    # Check whether the encoding is known
    import codecs
    try:
        codecs.lookup(name)
    except LookupError:
        # The standard encoding error does not indicate the encoding
        raise LookupError, "Unknown encoding "+name
    return name

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
        self.fileencoding = None

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
            return False

        chars = self.decode(chars)

        self.text.delete("1.0", "end")
        self.set_filename(None)
        self.text.insert("1.0", chars)
        self.reset_undo()
        self.set_filename(filename)
        self.text.mark_set("insert", "1.0")
        self.text.see("insert")
        return True

    def decode(self, chars):
        # Try to create a Unicode string. If that fails, let Tcl try
        # its best

        # Check presence of a UTF-8 signature first
        if chars.startswith(BOM_UTF8):
            try:
                chars = chars[3:].decode("utf-8")
            except UnicodeError:
                # has UTF-8 signature, but fails to decode...
                return chars
            else:
                # Indicates that this file originally had a BOM
                self.fileencoding = BOM_UTF8
                return chars

        # Next look for coding specification
        try:
            enc = coding_spec(chars)
        except LookupError, name:
            tkMessageBox.showerror(
                title="Error loading the file",
                message="The encoding '%s' is not known to this Python "\
                "installation. The file may not display correctly" % name,
                master = self.text)
            enc = None

        if enc:
            try:
                return unicode(chars, enc)
            except UnicodeError:
                pass

        # If it is ASCII, we need not to record anything
        try:
            return unicode(chars, 'ascii')
        except UnicodeError:
            pass

        # Finally, try the locale's encoding. This is deprecated;
        # the user should declare a non-ASCII encoding
        try:
            chars = unicode(chars, encoding)
            self.fileencoding = encoding
        except UnicodeError:
            pass
        return chars

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

    def print_window(self, event):
        tempfilename = None
        if self.get_saved():
            filename = self.filename
        else:
            (tfd, tfn) = tempfile.mkstemp()
            os.close(tfd)
            filename = tfn
            if not self.writefile(filename):
                os.unlink(tfn)
                return "break"
        edconf = idleconf.getsection('EditorWindow')
        command = edconf.get('print-command')
        command = command % filename
        if os.name == 'posix':
            command = command + " 2>&1"
        pipe = os.popen(command, "r")
        output = pipe.read().strip()
        status = pipe.close()
        if status:
            output = "Printing failed (exit status 0x%x)\n" % status + output
        if output:
            output = "Printing command: %s\n" % repr(command) + output
            tkMessageBox.showerror("Print status", output, master=self.text)
        return "break"

    def writefile(self, filename):
        self.fixlastline()
        chars = self.encode(self.text.get("1.0", "end-1c"))
        try:
            f = open(filename, "w")
            f.write(chars)
            f.close()
            ## print "saved to", `filename`
            return True
        except IOError, msg:
            tkMessageBox.showerror("I/O Error", str(msg),
                                   master=self.text)
            return False

    def encode(self, chars):
        if isinstance(chars, types.StringType):
            # This is either plain ASCII, or Tk was returning mixed-encoding
            # text to us. Don't try to guess further.
            return chars

        # See whether there is anything non-ASCII in it.
        # If not, no need to figure out the encoding.
        try:
            return chars.encode('ascii')
        except UnicodeError:
            pass

        # If there is an encoding declared, try this first.
        try:
            enc = coding_spec(chars)
            failed = None
        except LookupError, msg:
            failed = msg
            enc = None
        if enc:
            try:
                return chars.encode(enc)
            except UnicodeError:
                failed = "Invalid encoding '%s'" % enc

        if failed:
            tkMessageBox.showerror(
                "I/O Error",
                "%s. Saving as UTF-8" % failed,
                master = self.text)

        # If there was a UTF-8 signature, use that. This should not fail
        if self.fileencoding == BOM_UTF8 or failed:
            return BOM_UTF8 + chars.encode("utf-8")

        # Try the original file encoding next, if any
        if self.fileencoding:
            try:
                return chars.encode(self.fileencoding)
            except UnicodeError:
                tkMessageBox.showerror(
                    "I/O Error",
                    "Cannot save this as '%s' anymore. Saving as UTF-8" % self.fileencoding,
                    master = self.text)
                return BOM_UTF8 + chars.encode("utf-8")

        # Nothing was declared, and we had not determined an encoding
        # on loading. Recommend an encoding line.
        try:
            chars = chars.encode(encoding)
            enc = encoding
        except UnicodeError:
            chars = BOM_UTF8 + chars.encode("utf-8")
            enc = "utf-8"
        tkMessageBox.showerror(
            "I/O Error",
            "Non-ASCII found, yet no encoding declared. Add a line like\n"
            "# -*- coding: %s -*- \nto your file" % enc,
            master = self.text)
        return chars

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
