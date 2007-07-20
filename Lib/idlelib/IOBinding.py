# changes by dscherer@cmu.edu
#   - IOBinding.open() replaces the current window with the opened file,
#     if the current window is both unmodified and unnamed
#   - IOBinding.loadfile() interprets Windows, UNIX, and Macintosh
#     end-of-line conventions, instead of relying on the standard library,
#     which will only understand the local convention.

import os
import types
import sys
import codecs
import tempfile
import tkFileDialog
import tkMessageBox
import re
from Tkinter import *
from .SimpleDialog import SimpleDialog

from .configHandler import idleConf

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
except (ImportError, locale.Error):
    pass

# Encoding for file names
filesystemencoding = sys.getfilesystemencoding()

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
        if encoding is None or encoding is '':
            # situation occurs on Mac OS X
            encoding = 'ascii'
        codecs.lookup(encoding)
    except (NameError, AttributeError, LookupError):
        # Try getdefaultlocale well: it parses environment variables,
        # which may give a clue. Unfortunately, getdefaultlocale has
        # bugs that can cause ValueError.
        try:
            encoding = locale.getdefaultlocale()[1]
            if encoding is None or encoding is '':
                # situation occurs on Mac OS X
                encoding = 'ascii'
            codecs.lookup(encoding)
        except (ValueError, LookupError):
            pass

encoding = encoding.lower()

coding_re = re.compile("coding[:=]\s*([-\w_.]+)")

class EncodingMessage(SimpleDialog):
    "Inform user that an encoding declaration is needed."
    def __init__(self, master, enc):
        self.should_edit = False

        self.root = top = Toplevel(master)
        top.bind("<Return>", self.return_event)
        top.bind("<Escape>", self.do_ok)
        top.protocol("WM_DELETE_WINDOW", self.wm_delete_window)
        top.wm_title("I/O Warning")
        top.wm_iconname("I/O Warning")
        self.top = top

        l1 = Label(top,
            text="Non-ASCII found, yet no encoding declared. Add a line like")
        l1.pack(side=TOP, anchor=W)
        l2 = Entry(top, font="courier")
        l2.insert(0, "# -*- coding: %s -*-" % enc)
        # For some reason, the text is not selectable anymore if the
        # widget is disabled.
        # l2['state'] = DISABLED
        l2.pack(side=TOP, anchor = W, fill=X)
        l3 = Label(top, text="to your file\n"
                   "Choose OK to save this file as %s\n"
                   "Edit your general options to silence this warning" % enc)
        l3.pack(side=TOP, anchor = W)

        buttons = Frame(top)
        buttons.pack(side=TOP, fill=X)
        # Both return and cancel mean the same thing: do nothing
        self.default = self.cancel = 0
        b1 = Button(buttons, text="Ok", default="active",
                    command=self.do_ok)
        b1.pack(side=LEFT, fill=BOTH, expand=1)
        b2 = Button(buttons, text="Edit my file",
                    command=self.do_edit)
        b2.pack(side=LEFT, fill=BOTH, expand=1)

        self._set_transient(master)

    def do_ok(self):
        self.done(0)

    def do_edit(self):
        self.done(1)

def coding_spec(str):
    """Return the encoding declaration according to PEP 263.

    Raise LookupError if the encoding is declared but unknown.
    """
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
                except AttributeError:
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

    eol = r"(\r\n)|\n|\r"  # \r\n (Windows), \n (UNIX), or \r (Mac)
    eol_re = re.compile(eol)
    eol_convention = os.linesep # Default

    def loadfile(self, filename):
        try:
            # open the file in binary mode so that we can handle
            #   end-of-line convention ourselves.
            f = open(filename,'rb')
            chars = f.read()
            f.close()
        except IOError as msg:
            tkMessageBox.showerror("I/O Error", str(msg), master=self.text)
            return False

        chars = self.decode(chars)
        # We now convert all end-of-lines to '\n's
        firsteol = self.eol_re.search(chars)
        if firsteol:
            self.eol_convention = firsteol.group(0)
            if isinstance(self.eol_convention, str):
                # Make sure it is an ASCII string
                self.eol_convention = self.eol_convention.encode("ascii")
            chars = self.eol_re.sub(r"\n", chars)

        self.text.delete("1.0", "end")
        self.set_filename(None)
        self.text.insert("1.0", chars)
        self.reset_undo()
        self.set_filename(filename)
        self.text.mark_set("insert", "1.0")
        self.text.see("insert")
        self.updaterecentfileslist(filename)
        return True

    def decode(self, chars):
        """Create a Unicode string

        If that fails, let Tcl try its best
        """
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
        except LookupError as name:
            tkMessageBox.showerror(
                title="Error loading the file",
                message="The encoding '%s' is not known to this Python "\
                "installation. The file may not display correctly" % name,
                master = self.text)
            enc = None
        if enc:
            try:
                return str(chars, enc)
            except UnicodeError:
                pass
        # If it is ASCII, we need not to record anything
        try:
            return str(chars, 'ascii')
        except UnicodeError:
            pass
        # Finally, try the locale's encoding. This is deprecated;
        # the user should declare a non-ASCII encoding
        try:
            chars = str(chars, encoding)
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
        self.fixlastline()
        chars = self.encode(self.text.get("1.0", "end-1c"))
        if self.eol_convention != "\n":
            chars = chars.replace("\n", self.eol_convention)
        try:
            f = open(filename, "wb")
            f.write(chars)
            f.flush()
            f.close()
            return True
        except IOError as msg:
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
        except LookupError as msg:
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
                    "Cannot save this as '%s' anymore. Saving as UTF-8" \
                    % self.fileencoding,
                    master = self.text)
                return BOM_UTF8 + chars.encode("utf-8")
        # Nothing was declared, and we had not determined an encoding
        # on loading. Recommend an encoding line.
        config_encoding = idleConf.GetOption("main","EditorWindow",
                                             "encoding")
        if config_encoding == 'utf-8':
            # User has requested that we save files as UTF-8
            return BOM_UTF8 + chars.encode("utf-8")
        ask_user = True
        try:
            chars = chars.encode(encoding)
            enc = encoding
            if config_encoding == 'locale':
                ask_user = False
        except UnicodeError:
            chars = BOM_UTF8 + chars.encode("utf-8")
            enc = "utf-8"
        if not ask_user:
            return chars
        dialog = EncodingMessage(self.editwin.top, enc)
        dialog.go()
        if dialog.num == 1:
            # User asked us to edit the file
            encline = "# -*- coding: %s -*-\n" % enc
            firstline = self.text.get("1.0", "2.0")
            if firstline.startswith("#!"):
                # Insert encoding after #! line
                self.text.insert("2.0", encline)
            else:
                self.text.insert("1.0", encline)
            return self.encode(self.text.get("1.0", "end-1c"))
        return chars

    def fixlastline(self):
        c = self.text.get("end-2c")
        if c != '\n':
            self.text.insert("end-1c", "\n")

    def print_window(self, event):
        tempfilename = None
        saved = self.get_saved()
        if saved:
            filename = self.filename
        # shell undo is reset after every prompt, looks saved, probably isn't
        if not saved or filename is None:
            # XXX KBK 08Jun03 Wouldn't it be better to ask the user to save?
            (tfd, tempfilename) = tempfile.mkstemp(prefix='IDLE_tmp_')
            filename = tempfilename
            os.close(tfd)
            if not self.writefile(tempfilename):
                os.unlink(tempfilename)
                return "break"
        platform=os.name
        printPlatform=1
        if platform == 'posix': #posix platform
            command = idleConf.GetOption('main','General',
                                         'print-command-posix')
            command = command + " 2>&1"
        elif platform == 'nt': #win32 platform
            command = idleConf.GetOption('main','General','print-command-win')
        else: #no printing for this platform
            printPlatform=0
        if printPlatform:  #we can try to print for this platform
            command = command % filename
            pipe = os.popen(command, "r")
            # things can get ugly on NT if there is no printer available.
            output = pipe.read().strip()
            status = pipe.close()
            if status:
                output = "Printing failed (exit status 0x%x)\n" % \
                         status + output
            if output:
                output = "Printing command: %s\n" % repr(command) + output
                tkMessageBox.showerror("Print status", output, master=self.text)
        else:  #no printing for this platform
            message="Printing is not enabled for this platform: %s" % platform
            tkMessageBox.showinfo("Print status", message, master=self.text)
        if tempfilename:
            os.unlink(tempfilename)
        return "break"

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
        filename = self.opendialog.show(initialdir=dir, initialfile=base)
        if isinstance(filename, str):
            filename = filename.encode(filesystemencoding)
        return filename

    def defaultfilename(self, mode="open"):
        if self.filename:
            return os.path.split(self.filename)
        elif self.dirname:
            return self.dirname, ""
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
        filename = self.savedialog.show(initialdir=dir, initialfile=base)
        if isinstance(filename, str):
            filename = filename.encode(filesystemencoding)
        return filename

    def updaterecentfileslist(self,filename):
        "Update recent file list on all editor windows"
        self.editwin.update_recent_files_list(filename)

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
    test()
