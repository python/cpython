"""Editor window that can serve as an output file.
"""

import re

from tkinter import *
import tkinter.messagebox as tkMessageBox

from idlelib.editor import EditorWindow
from idlelib import iomenu


class OutputWindow(EditorWindow):
    """An editor window that can serve as an output file.

    Also the future base class for the Python shell window.
    This class has no input facilities.

    Adds binding to open a file at a line to the text widget.
    """
    def __init__(self, *args):
        EditorWindow.__init__(self, *args)
        self.text.bind("<<goto-file-line>>", self.goto_file_line)

    # Customize EditorWindow

    def ispythonsource(self, filename):
        "Control highlighting of Python source."
        # No colorization needed
        return False

    def short_title(self):
        "Customize EditorWindow title."
        return "Output"

    def maybesave(self):
        "Customize EditorWindow to not display save file messagebox."
        # Override base class method -- don't ask any questions
        if self.get_saved():
            return "yes"
        else:
            return "no"

    # Act as output file

    def write(self, s, tags=(), mark="insert"):
        """Write text to text widget.

        The text is inserted at the given index with the provided
        tags.  The text widget is then scrolled to make it visible
        and updated to display it, giving the effect of seeing each
        line as it is added.

        Args:
            s: Text to insert into text widget.
            tags: Tuple of tag strings to apply on the insert.
            mark: Index for the insert.

        Return:
            Length of text inserted.
        """
        if isinstance(s, (bytes, bytes)):
            s = s.decode(iomenu.encoding, "replace")
        self.text.insert(mark, s, tags)
        self.text.see(mark)
        self.text.update()
        return len(s)

    def writelines(self, lines):
        "Write each item in lines iterable."
        for line in lines:
            self.write(line)

    def flush(self):
        """Flush file.

        Provide file flush functionality for the window.
        """
        pass

    # Our own right-button menu

    rmenu_specs = [
        ("Cut", "<<cut>>", "rmenu_check_cut"),
        ("Copy", "<<copy>>", "rmenu_check_copy"),
        ("Paste", "<<paste>>", "rmenu_check_paste"),
        (None, None, None),
        ("Go to file/line", "<<goto-file-line>>", None),
    ]

    file_line_pats = [
        # order of patterns matters
        r'file "([^"]*)", line (\d+)',
        r'([^\s]+)\((\d+)\)',
        r'^(\s*\S.*?):\s*(\d+):',  # Win filename, maybe starting with spaces
        r'([^\s]+):\s*(\d+):',     # filename or path, ltrim
        r'^\s*(\S.*?):\s*(\d+):',  # Win abs path with embedded spaces, ltrim
    ]

    file_line_progs = None

    def goto_file_line(self, event=None):
        """Handle request to open file/line.

        If the selected or previous line in the output window
        contains a file name and line number, then open that file
        name in a new window and position on the line number.

        Otherwise, display an error messagebox.
        """
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
                    parent=self.text)
                return None
        filename, lineno = result
        edit = self.flist.open(filename)
        edit.gotoline(lineno)
        return None

    def _file_line_helper(self, line):
        """Extract file name and line number from line of text.

        Check if line of text contains one of the file/line patterns.
        If it does and if the file and line are valid, return
        a tuple of the file name and line number.  If it doesn't match
        or if the file or line is invalid, return None.
        """
        for prog in self.file_line_progs:
            match = prog.search(line)
            if match:
                filename, lineno = match.group(1, 2)
                try:
                    f = open(filename, "r")
                    f.close()
                    break
                except OSError:
                    continue
        else:
            return None
        try:
            return filename, int(lineno)
        except TypeError:
            return None


# These classes are currently not used but might come in handy
class OnDemandOutputWindow:

    tagdefs = {
        # XXX Should use IdlePrefs.ColorPrefs
        "stdout":  {"foreground": "blue"},
        "stderr":  {"foreground": "#007700"},
    }

    def __init__(self, flist):
        self.flist = flist
        self.owin = None

    def write(self, s, tags, mark):
        if not self.owin:
            self.setup()
        self.owin.write(s, tags, mark)

    def setup(self):
        self.owin = owin = OutputWindow(self.flist)
        text = owin.text
        for tag, cnf in self.tagdefs.items():
            if cnf:
                text.tag_configure(tag, **cnf)
        text.tag_raise('sel')
        self.write = self.owin.write
