#!/usr/bin/env python3

"""Basic regular expression demonstration facility (Perl style syntax)."""

from tkinter import *
import re

class ReDemo:

    def __init__(self, master):
        self.master = master

        self.promptdisplay = Label(self.master, anchor=W,
                text="Enter a Perl-style regular expression:")
        self.promptdisplay.pack(side=TOP, fill=X)

        self.regexdisplay = Entry(self.master)
        self.regexdisplay.pack(fill=X)
        self.regexdisplay.focus_set()

        self.addoptions()

        self.statusdisplay = Label(self.master, text="", anchor=W)
        self.statusdisplay.pack(side=TOP, fill=X)

        self.labeldisplay = Label(self.master, anchor=W,
                text="Enter a string to search:")
        self.labeldisplay.pack(fill=X)
        self.labeldisplay.pack(fill=X)

        self.showframe = Frame(master)
        self.showframe.pack(fill=X, anchor=W)

        self.showvar = StringVar(master)
        self.showvar.set("first")

        self.showfirstradio = Radiobutton(self.showframe,
                                         text="Highlight first match",
                                          variable=self.showvar,
                                          value="first",
                                          command=self.recompile)
        self.showfirstradio.pack(side=LEFT)

        self.showallradio = Radiobutton(self.showframe,
                                        text="Highlight all matches",
                                        variable=self.showvar,
                                        value="all",
                                        command=self.recompile)
        self.showallradio.pack(side=LEFT)

        self.stringdisplay = Text(self.master, width=60, height=4)
        self.stringdisplay.pack(fill=BOTH, expand=1)
        self.stringdisplay.tag_configure("hit", background="yellow")

        self.grouplabel = Label(self.master, text="Groups:", anchor=W)
        self.grouplabel.pack(fill=X)

        self.grouplist = Listbox(self.master)
        self.grouplist.pack(expand=1, fill=BOTH)

        self.regexdisplay.bind('<Key>', self.recompile)
        self.stringdisplay.bind('<Key>', self.reevaluate)

        self.compiled = None
        self.recompile()

        btags = self.regexdisplay.bindtags()
        self.regexdisplay.bindtags(btags[1:] + btags[:1])

        btags = self.stringdisplay.bindtags()
        self.stringdisplay.bindtags(btags[1:] + btags[:1])

    def addoptions(self):
        self.frames = []
        self.boxes = []
        self.vars = []
        for name in ('IGNORECASE',
                     'MULTILINE',
                     'DOTALL',
                     'VERBOSE'):
            if len(self.boxes) % 3 == 0:
                frame = Frame(self.master)
                frame.pack(fill=X)
                self.frames.append(frame)
            val = getattr(re, name).value
            var = IntVar()
            box = Checkbutton(frame,
                    variable=var, text=name,
                    offvalue=0, onvalue=val,
                    command=self.recompile)
            box.pack(side=LEFT)
            self.boxes.append(box)
            self.vars.append(var)

    def getflags(self):
        flags = 0
        for var in self.vars:
            flags = flags | var.get()
        flags = flags
        return flags

    def recompile(self, event=None):
        try:
            self.compiled = re.compile(self.regexdisplay.get(),
                                       self.getflags())
            bg = self.promptdisplay['background']
            self.statusdisplay.config(text="", background=bg)
        except re.error as msg:
            self.compiled = None
            self.statusdisplay.config(
                    text="re.error: %s" % str(msg),
                    background="red")
        self.reevaluate()

    def reevaluate(self, event=None):
        try:
            self.stringdisplay.tag_remove("hit", "1.0", END)
        except TclError:
            pass
        try:
            self.stringdisplay.tag_remove("hit0", "1.0", END)
        except TclError:
            pass
        self.grouplist.delete(0, END)
        if not self.compiled:
            return
        self.stringdisplay.tag_configure("hit", background="yellow")
        self.stringdisplay.tag_configure("hit0", background="orange")
        text = self.stringdisplay.get("1.0", END)
        last = 0
        nmatches = 0
        while last <= len(text):
            m = self.compiled.search(text, last)
            if m is None:
                break
            first, last = m.span()
            if last == first:
                last = first+1
                tag = "hit0"
            else:
                tag = "hit"
            pfirst = "1.0 + %d chars" % first
            plast = "1.0 + %d chars" % last
            self.stringdisplay.tag_add(tag, pfirst, plast)
            if nmatches == 0:
                self.stringdisplay.yview_pickplace(pfirst)
                groups = list(m.groups())
                groups.insert(0, m.group())
                for i in range(len(groups)):
                    g = "%2d: %r" % (i, groups[i])
                    self.grouplist.insert(END, g)
            nmatches = nmatches + 1
            if self.showvar.get() == "first":
                break

        if nmatches == 0:
            self.statusdisplay.config(text="(no match)",
                                      background="yellow")
        else:
            self.statusdisplay.config(text="")


# Main function, run when invoked as a stand-alone Python program.

def main():
    root = Tk()
    demo = ReDemo(root)
    root.protocol('WM_DELETE_WINDOW', root.quit)
    root.mainloop()

if __name__ == '__main__':
    main()
