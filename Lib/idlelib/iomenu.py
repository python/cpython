import codecs
from codecs import BOM_UTF8
import os
import re
import shlex
import sys
import tempfile

import tkinter.filedialog as tkFileDialog
import tkinter.messagebox as tkMessageBox
from tkinter.simpledialog import askstring

import idlelib
from idlelib.config import idleConf

encoding = 'utf-8'
if sys.platform == 'win32':
    errors = 'surrogatepass'
else:
    errors = 'surrogateescape'


coding_re = re.compile(r'^[ \t\f]*#.*?coding[:=][ \t]*([-\w.]+)', re.ASCII)
blank_re = re.compile(r'^[ \t\f]*(?:[#\r\n]|$)', re.ASCII)

def coding_spec(data):
    """Return the encoding declaration according to PEP 263.

    When checking encoded data, only the first two lines should be passed
    in to avoid a UnicodeDecodeError if the rest of the data is not unicode.
    The first two lines would contain the encoding specification.

    Raise a LookupError if the encoding is declared but unknown.
    """
    if isinstance(data, bytes):
        # This encoding might be wrong. However, the coding
        # spec must be ASCII-only, so any non-ASCII characters
        # around here will be ignored. Decoding to Latin-1 should
        # never fail (except for memory outage)
        lines = data.decode('iso-8859-1')
    else:
        lines = data
    # consider only the first two lines
    if '\n' in lines:
        lst = lines.split('\n', 2)[:2]
    elif '\r' in lines:
        lst = lines.split('\r', 2)[:2]
    else:
        lst = [lines]
    for line in lst:
        match = coding_re.match(line)
        if match is not None:
            break
        if not blank_re.match(line):
            return None
    else:
        return None
    name = match.group(1)
    try:
        codecs.lookup(name)
    except LookupError:
        # The standard encoding error does not indicate the encoding
        raise LookupError("Unknown encoding: "+name)
    return name


class IOBinding:
# One instance per editor Window so methods know which to save, close.
# Open returns focus to self.editwin if aborted.
# EditorWindow.open_module, others, belong here.

    def __init__(self, editwin):
        self.editwin = editwin
        self.text = editwin.text
        self.__id_open = self.text.bind("<<open-window-from-file>>", self.open)
        self.__id_save = self.text.bind("<<save-window>>", self.save)
        self.__id_saveas = self.text.bind("<<save-window-as-file>>",
                                          self.save_as)
        self.__id_savecopy = self.text.bind("<<save-copy-of-window-as-file>>",
                                            self.save_a_copy)
        self.fileencoding = None
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
    dirname = None

    def set_filename(self, filename):
        if filename and os.path.isdir(filename):
            self.filename = None
            self.dirname = filename
        else:
            self.filename = filename
            self.dirname = None
            self.set_saved(1)
            if self.filename_change_hook:
                self.filename_change_hook()

    def open(self, event=None, editFile=None):
        flist = self.editwin.flist
        # Save in case parent window is closed (ie, during askopenfile()).
        if flist:
            if not editFile:
                filename = self.askopenfile()
            else:
                filename=editFile
            if filename:
                # If editFile is valid and already open, flist.open will
                # shift focus to its existing window.
                # If the current window exists and is a fresh unnamed,
                # unmodified editor window (not an interpreter shell),
                # pass self.loadfile to flist.open so it will load the file
                # in the current window (if the file is not already open)
                # instead of a new window.
                if (self.editwin and
                        not getattr(self.editwin, 'interp', None) and
                        not self.filename and
                        self.get_saved()):
                    flist.open(filename, self.loadfile)
                else:
                    flist.open(filename)
            else:
                if self.text:
                    self.text.focus_set()
            return "break"

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

    eol = r"(\r\n)|\n|\r"  # \r\n (Windows), \n (UNIX), or \r (Mac)
    eol_re = re.compile(eol)
    eol_convention = os.linesep  # default

    def loadfile(self, filename):
        try:
            # open the file in binary mode so that we can handle
            # end-of-line convention ourselves.
            with open(filename, 'rb') as f:
                two_lines = f.readline() + f.readline()
                f.seek(0)
                bytes = f.read()
        except OSError as msg:
            tkMessageBox.showerror("I/O Error", str(msg), parent=self.text)
            return False
        chars, converted = self._decode(two_lines, bytes)
        if chars is None:
            tkMessageBox.showerror("Decoding Error",
                                   "File %s\nFailed to Decode" % filename,
                                   parent=self.text)
            return False
        # We now convert all end-of-lines to '\n's
        firsteol = self.eol_re.search(chars)
        if firsteol:
            self.eol_convention = firsteol.group(0)
            chars = self.eol_re.sub(r"\n", chars)
        self.text.delete("1.0", "end")
        self.set_filename(None)
        self.text.insert("1.0", chars)
        self.reset_undo()
        self.set_filename(filename)
        if converted:
            # We need to save the conversion results first
            # before being able to execute the code
            self.set_saved(False)
        self.text.mark_set("insert", "1.0")
        self.text.yview("insert")
        self.updaterecentfileslist(filename)
        return True

    def _decode(self, two_lines, bytes):
        "Create a Unicode string."
        chars = None
        # Check presence of a UTF-8 signature first
        if bytes.startswith(BOM_UTF8):
            try:
                chars = bytes[3:].decode("utf-8")
            except UnicodeDecodeError:
                # has UTF-8 signature, but fails to decode...
                return None, False
            else:
                # Indicates that this file originally had a BOM
                self.fileencoding = 'BOM'
                return chars, False
        # Next look for coding specification
        try:
            enc = coding_spec(two_lines)
        except LookupError as name:
            tkMessageBox.showerror(
                title="Error loading the file",
                message="The encoding '%s' is not known to this Python "\
                "installation. The file may not display correctly" % name,
                parent = self.text)
            enc = None
        except UnicodeDecodeError:
            return None, False
        if enc:
            try:
                chars = str(bytes, enc)
                self.fileencoding = enc
                return chars, False
            except UnicodeDecodeError:
                pass
        # Try ascii:
        try:
            chars = str(bytes, 'ascii')
            self.fileencoding = None
            return chars, False
        except UnicodeDecodeError:
            pass
        # Try utf-8:
        try:
            chars = str(bytes, 'utf-8')
            self.fileencoding = 'utf-8'
            return chars, False
        except UnicodeDecodeError:
            pass
        # Finally, try the locale's encoding. This is deprecated;
        # the user should declare a non-ASCII encoding
        try:
            # Wait for the editor window to appear
            self.editwin.text.update()
            enc = askstring(
                "Specify file encoding",
                "The file's encoding is invalid for Python 3.x.\n"
                "IDLE will convert it to UTF-8.\n"
                "What is the current encoding of the file?",
                initialvalue = encoding,
                parent = self.editwin.text)

            if enc:
                chars = str(bytes, enc)
                self.fileencoding = None
            return chars, True
        except (UnicodeDecodeError, LookupError):
            pass
        return None, False  # None on failure

    def maybesave(self):
        if self.get_saved():
            return "yes"
        message = "Do you want to save %s before closing?" % (
            self.filename or "this untitled document")
        confirm = tkMessageBox.askyesnocancel(
                  title="Save On Close",
                  message=message,
                  default=tkMessageBox.YES,
                  parent=self.text)
        if confirm:
            reply = "yes"
            self.save(None)
            if not self.get_saved():
                reply = "cancel"
        elif confirm is None:
            reply = "cancel"
        else:
            reply = "no"
        self.text.focus_set()
        return reply

    def save(self, event):
        if not self.filename:
            self.save_as(event)
        else:
            if self.writefile(self.filename):
                self.set_saved(True)
                try:
                    self.editwin.store_file_breaks()
                except AttributeError:  # may be a PyShell
                    pass
        self.text.focus_set()
        return "break"

    def save_as(self, event):
        filename = self.asksavefile()
        if filename:
            if self.writefile(filename):
                self.set_filename(filename)
                self.set_saved(1)
                try:
                    self.editwin.store_file_breaks()
                except AttributeError:
                    pass
        self.text.focus_set()
        self.updaterecentfileslist(filename)
        return "break"

    def save_a_copy(self, event):
        filename = self.asksavefile()
        if filename:
            self.writefile(filename)
        self.text.focus_set()
        self.updaterecentfileslist(filename)
        return "break"

    def writefile(self, filename):
        text = self.fixnewlines()
        chars = self.encode(text)
        try:
            with open(filename, "wb") as f:
                f.write(chars)
                f.flush()
                os.fsync(f.fileno())
            return True
        except OSError as msg:
            tkMessageBox.showerror("I/O Error", str(msg),
                                   parent=self.text)
            return False

    def fixnewlines(self):
        "Return text with final \n if needed and os eols."
        if (self.text.get("end-2c") != '\n'
            and not hasattr(self.editwin, "interp")):  # Not shell.
            self.text.insert("end-1c", "\n")
        text = self.text.get("1.0", "end-1c")
        if self.eol_convention != "\n":
            text = text.replace("\n", self.eol_convention)
        return text

    def encode(self, chars):
        if isinstance(chars, bytes):
            # This is either plain ASCII, or Tk was returning mixed-encoding
            # text to us. Don't try to guess further.
            return chars
        # Preserve a BOM that might have been present on opening
        if self.fileencoding == 'BOM':
            return BOM_UTF8 + chars.encode("utf-8")
        # See whether there is anything non-ASCII in it.
        # If not, no need to figure out the encoding.
        try:
            return chars.encode('ascii')
        except UnicodeError:
            pass
        # Check if there is an encoding declared
        try:
            # a string, let coding_spec slice it to the first two lines
            enc = coding_spec(chars)
            failed = None
        except LookupError as msg:
            failed = msg
            enc = None
        else:
            if not enc:
                # PEP 3120: default source encoding is UTF-8
                enc = 'utf-8'
        if enc:
            try:
                return chars.encode(enc)
            except UnicodeError:
                failed = "Invalid encoding '%s'" % enc
        tkMessageBox.showerror(
            "I/O Error",
            "%s.\nSaving as UTF-8" % failed,
            parent = self.text)
        # Fallback: save as UTF-8, with BOM - ignoring the incorrect
        # declared encoding
        return BOM_UTF8 + chars.encode("utf-8")

    def print_window(self, event):
        confirm = tkMessageBox.askokcancel(
                  title="Print",
                  message="Print to Default Printer",
                  default=tkMessageBox.OK,
                  parent=self.text)
        if not confirm:
            self.text.focus_set()
            return "break"
        tempfilename = None
        saved = self.get_saved()
        if saved:
            filename = self.filename
        # shell undo is reset after every prompt, looks saved, probably isn't
        if not saved or filename is None:
            (tfd, tempfilename) = tempfile.mkstemp(prefix='IDLE_tmp_')
            filename = tempfilename
            os.close(tfd)
            if not self.writefile(tempfilename):
                os.unlink(tempfilename)
                return "break"
        platform = os.name
        printPlatform = True
        if platform == 'posix': #posix platform
            command = idleConf.GetOption('main','General',
                                         'print-command-posix')
            command = command + " 2>&1"
        elif platform == 'nt': #win32 platform
            command = idleConf.GetOption('main','General','print-command-win')
        else: #no printing for this platform
            printPlatform = False
        if printPlatform:  #we can try to print for this platform
            command = command % shlex.quote(filename)
            pipe = os.popen(command, "r")
            # things can get ugly on NT if there is no printer available.
            output = pipe.read().strip()
            status = pipe.close()
            if status:
                output = "Printing failed (exit status 0x%x)\n" % \
                         status + output
            if output:
                output = "Printing command: %s\n" % repr(command) + output
                tkMessageBox.showerror("Print status", output, parent=self.text)
        else:  #no printing for this platform
            message = "Printing is not enabled for this platform: %s" % platform
            tkMessageBox.showinfo("Print status", message, parent=self.text)
        if tempfilename:
            os.unlink(tempfilename)
        return "break"

    opendialog = None
    savedialog = None

    filetypes = (
        ("Python files", "*.py *.pyw", "TEXT"),
        ("Text files", "*.txt", "TEXT"),
        ("All files", "*"),
        )

    defaultextension = '.py' if sys.platform == 'darwin' else ''

    def askopenfile(self):
        dir, base = self.defaultfilename("open")
        if not self.opendialog:
            self.opendialog = tkFileDialog.Open(parent=self.text,
                                                filetypes=self.filetypes)
        filename = self.opendialog.show(initialdir=dir, initialfile=base)
        return filename

    def defaultfilename(self, mode="open"):
        if self.filename:
            return os.path.split(self.filename)
        elif self.dirname:
            return self.dirname, ""
        else:
            try:
                pwd = os.getcwd()
            except OSError:
                pwd = ""
            return pwd, ""

    def asksavefile(self):
        dir, base = self.defaultfilename("save")
        if not self.savedialog:
            self.savedialog = tkFileDialog.SaveAs(
                    parent=self.text,
                    filetypes=self.filetypes,
                    defaultextension=self.defaultextension)
        filename = self.savedialog.show(initialdir=dir, initialfile=base)
        return filename

    def updaterecentfileslist(self,filename):
        "Update recent file list on all editor windows"
        if self.editwin.flist:
            self.editwin.update_recent_files_list(filename)

def _io_binding(parent):  # htest #
    from tkinter import Toplevel, Text

    root = Toplevel(parent)
    root.title("Test IOBinding")
    x, y = map(int, parent.geometry().split('+')[1:])
    root.geometry("+%d+%d" % (x, y + 175))
    class MyEditWin:
        def __init__(self, text):
            self.text = text
            self.flist = None
            self.text.bind("<Control-o>", self.open)
            self.text.bind('<Control-p>', self.print)
            self.text.bind("<Control-s>", self.save)
            self.text.bind("<Alt-s>", self.saveas)
            self.text.bind('<Control-c>', self.savecopy)
        def get_saved(self): return 0
        def set_saved(self, flag): pass
        def reset_undo(self): pass
        def open(self, event):
            self.text.event_generate("<<open-window-from-file>>")
        def print(self, event):
            self.text.event_generate("<<print-window>>")
        def save(self, event):
            self.text.event_generate("<<save-window>>")
        def saveas(self, event):
            self.text.event_generate("<<save-window-as-file>>")
        def savecopy(self, event):
            self.text.event_generate("<<save-copy-of-window-as-file>>")

    text = Text(root)
    text.pack()
    text.focus_set()
    editwin = MyEditWin(text)
    IOBinding(editwin)

if __name__ == "__main__":
    from unittest import main
    main('idlelib.idle_test.test_iomenu', verbosity=2, exit=False)

    from idlelib.idle_test.htest import run
    run(_io_binding)
