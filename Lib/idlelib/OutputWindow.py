# changes by dscherer@cmu.edu
#   - OutputWindow and OnDemandOutputWindow have been hastily
#     extended to provide readline() support, an "iomark" separate
#     from the "insert" cursor, and scrolling to clear the window.
#     These changes are used by the ExecBinding module to provide
#     standard input and output for user programs.  Many of the new
#     features are very similar to features of PyShell, which is a
#     subclass of OutputWindow.  Someone should make some sense of
#     this.

from Tkinter import *
from EditorWindow import EditorWindow
import re
import tkMessageBox

from UndoDelegator import UndoDelegator

class OutputUndoDelegator(UndoDelegator):
    reading = 0
    # Forbid insert/delete before the I/O mark, in the blank lines after
    #   the output, or *anywhere* if we are not presently doing user input
    def insert(self, index, chars, tags=None):
        try:
            if (self.delegate.compare(index, "<", "iomark") or
                self.delegate.compare(index, ">", "endmark") or
                (index!="iomark" and not self.reading)):
                self.delegate.bell()
                return
        except TclError:
            pass
        UndoDelegator.insert(self, index, chars, tags)
    def delete(self, index1, index2=None):
        try:
            if (self.delegate.compare(index1, "<", "iomark") or
                self.delegate.compare(index1, ">", "endmark") or
                (index2 and self.delegate.compare(index2, ">=", "endmark")) or
                not self.reading):
                self.delegate.bell()
                return
        except TclError:
            pass
        UndoDelegator.delete(self, index1, index2)

class OutputWindow(EditorWindow):
    """An editor window that can serve as an input and output file.
       The input support has been rather hastily hacked in, and should
       not be trusted.
    """

    UndoDelegator = OutputUndoDelegator
    source_window = None

    def __init__(self, *args, **keywords):
        if keywords.has_key('source_window'):
            self.source_window = keywords['source_window']
        apply(EditorWindow.__init__, (self,) + args)
        self.text.bind("<<goto-file-line>>", self.goto_file_line)
        self.text.bind("<<newline-and-indent>>", self.enter_callback)
        self.text.mark_set("iomark","1.0")
        self.text.mark_gravity("iomark", LEFT)
        self.text.mark_set("endmark","1.0")

    # Customize EditorWindow

    def ispythonsource(self, filename):
        # No colorization needed
        return 0

    def short_title(self):
        return "Output"

    def long_title(self):
        return ""

    def maybesave(self):
        # Override base class method -- don't ask any questions
        if self.get_saved():
            return "yes"
        else:
            return "no"

    # Act as input file - incomplete

    def set_line_and_column(self, event=None):
        index = self.text.index(INSERT)
        if (self.text.compare(index, ">", "endmark")):
          self.text.mark_set("insert", "endmark")
        self.text.see("insert")
        EditorWindow.set_line_and_column(self)

    reading = 0
    canceled = 0
    endoffile = 0

    def readline(self):
        save = self.reading
        try:
            self.reading = self.undo.reading = 1
            self.text.mark_set("insert", "iomark")
            self.text.see("insert")
            self.top.mainloop()
        finally:
            self.reading = self.undo.reading = save
        line = self.text.get("input", "iomark")
        if self.canceled:
            self.canceled = 0
            raise KeyboardInterrupt
        if self.endoffile:
            self.endoffile = 0
            return ""
        return line or '\n'

    def close(self):
        self.interrupt()
        return EditorWindow.close(self)

    def interrupt(self):
        if self.reading:
            self.endoffile = 1
            self.top.quit()

    def enter_callback(self, event):
        if self.reading and self.text.compare("insert", ">=", "iomark"):
            self.text.mark_set("input", "iomark")
            self.text.mark_set("iomark", "insert")
            self.write('\n',"iomark")
            self.text.tag_add("stdin", "input", "iomark")
            self.text.update_idletasks()
            self.top.quit() # Break out of recursive mainloop() in raw_input()

        return "break"

    # Act as output file

    def write(self, s, tags=(), mark="iomark"):
        self.text.mark_gravity(mark, RIGHT)
        self.text.insert(mark, str(s), tags)
        self.text.mark_gravity(mark, LEFT)
        self.text.see(mark)
        self.text.update()

    def writelines(self, l):
        map(self.write, l)

    def flush(self):
        pass

    # Our own right-button menu

    rmenu_specs = [
        ("Go to file/line", "<<goto-file-line>>"),
    ]

    file_line_pats = [
        r'file "([^"]*)", line (\d+)',
        r'([^\s]+)\((\d+)\)',
        r'([^\s]+):\s*(\d+):',
    ]

    file_line_progs = None

    def goto_file_line(self, event=None):
        if self.file_line_progs is None:
            l = []
            for pat in self.file_line_pats:
                l.append(re.compile(pat, re.IGNORECASE))
            self.file_line_progs = l
        # x, y = self.event.x, self.event.y
        # self.text.mark_set("insert", "@%d,%d" % (x, y))
        line = self.text.get("insert linestart", "insert lineend")
        result = self._file_line_helper(line)
        if not result:
            # Try the previous line.  This is handy e.g. in tracebacks,
            # where you tend to right-click on the displayed source line
            line = self.text.get("insert -1line linestart",
                                 "insert -1line lineend")
            result = self._file_line_helper(line)
            if not result:
                tkMessageBox.showerror(
                    "No special line",
                    "The line you point at doesn't look like "
                    "a valid file name followed by a line number.",
                    master=self.text)
                return
        filename, lineno = result
        edit = self.untitled(filename) or self.flist.open(filename)
        edit.gotoline(lineno)
        edit.wakeup()

    def untitled(self, filename):
        if filename!='Untitled' or not self.source_window or self.source_window.io.filename:
            return None
        return self.source_window

    def _file_line_helper(self, line):
        for prog in self.file_line_progs:
            m = prog.search(line)
            if m:
                break
        else:
            return None
        filename, lineno = m.group(1, 2)
        if not self.untitled(filename):
            try:
                f = open(filename, "r")
                f.close()
            except IOError:
                return None
        try:
            return filename, int(lineno)
        except TypeError:
            return None

# This classes now used by ExecBinding.py:

class OnDemandOutputWindow:
    source_window = None

    tagdefs = {
        # XXX Should use IdlePrefs.ColorPrefs
        "stdin":   {"foreground": "black"},
        "stdout":  {"foreground": "blue"},
        "stderr":  {"foreground": "red"},
    }   
    
    def __init__(self, flist):
        self.flist = flist
        self.owin = None
        self.title = "Output"
        self.close_hook = None
        self.old_close = None

    def owclose(self):
        if self.close_hook:
            self.close_hook()
        if self.old_close:
            self.old_close()

    def set_title(self, title):
        self.title = title
        if self.owin and self.owin.text:
          self.owin.saved_change_hook()

    def write(self, s, tags=(), mark="iomark"):
        if not self.owin or not self.owin.text:
            self.setup()
        self.owin.write(s, tags, mark)

    def readline(self):
        if not self.owin or not self.owin.text:
            self.setup()
        return self.owin.readline()

    def scroll_clear(self):
        if self.owin and self.owin.text:
           lineno = self.owin.getlineno("endmark")
           self.owin.text.mark_set("insert","endmark")
           self.owin.text.yview(float(lineno))
           self.owin.wakeup()
    
    def setup(self):
        self.owin = owin = OutputWindow(self.flist, source_window = self.source_window)
        owin.short_title = lambda self=self: self.title
        text = owin.text

        self.old_close = owin.close_hook
        owin.close_hook = self.owclose

        # xxx Bad hack: 50 blank lines at the bottom so that
        #     we can scroll the top of the window to the output
        #     cursor in scroll_clear().  There must be a better way...
        owin.text.mark_gravity('endmark', LEFT)
        owin.text.insert('iomark', '\n'*50)
        owin.text.mark_gravity('endmark', RIGHT)
        
        for tag, cnf in self.tagdefs.items():
            if cnf:
                apply(text.tag_configure, (tag,), cnf)
        text.tag_raise('sel')
