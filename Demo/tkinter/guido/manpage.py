# Widget to display a man page

import os
import re
import sys

from tkinter import *
from tkinter.font import Font
from tkinter.scrolledtext import ScrolledText

# XXX Recognizing footers is system dependent
# (This one works for IRIX 5.2 and Solaris 2.2)
footerprog = re.compile(
        '^     Page [1-9][0-9]*[ \t]+\|^.*Last change:.*[1-9][0-9]*\n')
emptyprog = re.compile('^[ \t]*\n')
ulprog = re.compile('^[ \t]*[Xv!_][Xv!_ \t]*\n')


class EditableManPage(ScrolledText):
    """Basic Man Page class -- does not disable editing."""

    def __init__(self, master=None, **cnf):
        ScrolledText.__init__(self, master, **cnf)

        bold = Font(font=self['font']).copy()
        bold.config(weight='bold')
        italic = Font(font=self['font']).copy()
        italic.config(slant='italic')

        # Define tags for formatting styles
        self.tag_config('X', underline=1)
        self.tag_config('!', font=bold)
        self.tag_config('_', font=italic)

        # Set state to idle
        self.fp = None
        self.lineno = 0

    def busy(self):
        """Test whether we are busy parsing a file."""
        return self.fp != None

    def kill(self):
        """Ensure we're not busy."""
        if self.busy():
            self._endparser()

    def asyncparsefile(self, fp):
        """Parse a file, in the background."""
        self._startparser(fp)
        self.tk.createfilehandler(fp, READABLE,
                                  self._filehandler)

    parsefile = asyncparsefile   # Alias

    def _filehandler(self, fp, mask):
        """I/O handler used by background parsing."""
        nextline = self.fp.readline()
        if not nextline:
            self._endparser()
            return
        self._parseline(nextline)

    def syncparsefile(self, fp):
        """Parse a file, now (cannot be aborted)."""
        self._startparser(fp)
        while True:
            nextline = fp.readline()
            if not nextline:
                break
            self._parseline(nextline)
        self._endparser()

    def _startparser(self, fp):
        """Initialize parsing from a particular file -- must not be busy."""
        if self.busy():
            raise RuntimeError('startparser: still busy')
        fp.fileno()             # Test for file-ness
        self.fp = fp
        self.lineno = 0
        self.ok = 0
        self.empty = 0
        self.buffer = None
        savestate = self['state']
        self['state'] = NORMAL
        self.delete('1.0', END)
        self['state'] = savestate

    def _endparser(self):
        """End parsing -- must be busy, need not be at EOF."""
        if not self.busy():
            raise RuntimeError('endparser: not busy')
        if self.buffer:
            self._parseline('')
        try:
            self.tk.deletefilehandler(self.fp)
        except TclError:
            pass
        self.fp.close()
        self.fp = None
        del self.ok, self.empty, self.buffer

    def _parseline(self, nextline):
        """Parse a single line."""
        if not self.buffer:
            # Save this line -- we need one line read-ahead
            self.buffer = nextline
            return
        if emptyprog.match(self.buffer):
            # Buffered line was empty -- set a flag
            self.empty = 1
            self.buffer = nextline
            return
        textline = self.buffer
        if ulprog.match(nextline):
            # Next line is properties for buffered line
            propline = nextline
            self.buffer = None
        else:
            # Next line is read-ahead
            propline = None
            self.buffer = nextline
        if not self.ok:
            # First non blank line after footer must be header
            # -- skip that too
            self.ok = 1
            self.empty = 0
            return
        if footerprog.match(textline):
            # Footer -- start skipping until next non-blank line
            self.ok = 0
            self.empty = 0
            return
        savestate = self['state']
        self['state'] = NORMAL
        if TkVersion >= 4.0:
            self.mark_set('insert', 'end-1c')
        else:
            self.mark_set('insert', END)
        if self.empty:
            # One or more previous lines were empty
            # -- insert one blank line in the text
            self._insert_prop('\n')
            self.lineno = self.lineno + 1
            self.empty = 0
        if not propline:
            # No properties
            self._insert_prop(textline)
        else:
            # Search for properties
            p = ''
            j = 0
            for i in range(min(len(propline), len(textline))):
                if propline[i] != p:
                    if j < i:
                        self._insert_prop(textline[j:i], p)
                        j = i
                    p = propline[i]
            self._insert_prop(textline[j:])
        self.lineno = self.lineno + 1
        self['state'] = savestate

    def _insert_prop(self, str, prop = ' '):
        """Insert a string at the end, with at most one property (tag)."""
        here = self.index(AtInsert())
        self.insert(AtInsert(), str)
        if TkVersion <= 4.0:
            tags = self.tag_names(here)
            for tag in tags:
                self.tag_remove(tag, here, AtInsert())
        if prop != ' ':
            self.tag_add(prop, here, AtInsert())


class ReadonlyManPage(EditableManPage):
    """Readonly Man Page class -- disables editing, otherwise the same."""

    def __init__(self, master=None, **cnf):
        cnf['state'] = DISABLED
        EditableManPage.__init__(self, master, **cnf)

# Alias
ManPage = ReadonlyManPage

# usage: ManPage [manpage]; or ManPage [-f] file
# -f means that the file is nroff -man output run through ul -i
def main():
    # XXX This directory may be different on your system
    MANDIR = ''
    DEFAULTPAGE = 'Tcl'
    formatted = 0
    if sys.argv[1:] and sys.argv[1] == '-f':
        formatted = 1
        del sys.argv[1]
    if sys.argv[1:]:
        name = sys.argv[1]
    else:
        name = DEFAULTPAGE
    if not formatted:
        if name[-2:-1] != '.':
            name = name + '.n'
        name = os.path.join(MANDIR, name)
    root = Tk()
    root.minsize(1, 1)
    manpage = ManPage(root, relief=SUNKEN, borderwidth=2)
    manpage.pack(expand=1, fill=BOTH)
    if formatted:
        fp = open(name, 'r')
    else:
        fp = os.popen('nroff -man -c %s | ul -i' % name, 'r')
    manpage.parsefile(fp)
    root.mainloop()

if __name__ == '__main__':
    main()
