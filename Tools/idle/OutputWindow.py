from Tkinter import *
from EditorWindow import EditorWindow
import re
import tkMessageBox

class OutputWindow(EditorWindow):

    """An editor window that can serve as an output file.

    Also the future base class for the Python shell window.
    This class has no input facilities.
    """

    def __init__(self, *args):
        apply(EditorWindow.__init__, (self,) + args)
        self.text.bind("<<goto-file-line>>", self.goto_file_line)

    # Customize EditorWindow

    def ispythonsource(self, filename):
        # No colorization needed
        return 0

    def short_title(self):
        return "Output"

    def maybesave(self):
        # Override base class method -- don't ask any questions
        if self.get_saved():
            return "yes"
        else:
            return "no"

    # Act as output file

    def write(self, s, tags=(), mark="insert"):
        self.text.insert(mark, str(s), tags)
        self.text.see(mark)
        self.text.update()

    def writelines(self, l):
        map(self.write, l)

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
        for prog in self.file_line_progs:
            m = prog.search(line)
            if m:
                break
        else:
            tkMessageBox.showerror("No special line",
                "The line you point at doesn't look like "
                "a file name followed by a line number.",
                master=self.text)
            return
        filename, lineno = m.group(1, 2)
        try:
            f = open(filename, "r")
            f.close()
        except IOError, msg:
            self.text.bell()
            return
        edit = self.flist.open(filename)
        try:
            lineno = int(lineno)
        except ValueError, msg:
            self.text.bell()
            return
        edit.gotoline(lineno)
