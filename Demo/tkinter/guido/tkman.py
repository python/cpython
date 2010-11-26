#! /usr/bin/env python

# Tk man page browser -- currently only shows the Tcl/Tk man pages

import os
import re
import sys
from tkinter import *

from manpage import ManPage

MANNDIRLIST = ['/usr/local/man/mann', '/usr/share/man/mann']
MAN3DIRLIST = ['/usr/local/man/man3', '/usr/share/man/man3']

foundmanndir = 0
for dir in MANNDIRLIST:
    if os.path.exists(dir):
        MANNDIR = dir
        foundmanndir = 1

foundman3dir = 0
for dir in MAN3DIRLIST:
    if os.path.exists(dir):
        MAN3DIR = dir
        foundman3dir =  1

if not foundmanndir or not foundman3dir:
    sys.stderr.write('\n')
    if not foundmanndir:
        msg = """\
Failed to find mann directory.
Please add the correct entry to the MANNDIRLIST
at the top of %s script.""" % \
sys.argv[0]
        sys.stderr.write("%s\n\n" % msg)
    if not foundman3dir:
        msg = """\
Failed to find man3 directory.
Please add the correct entry to the MAN3DIRLIST
at the top of %s script.""" % \
sys.argv[0]
        sys.stderr.write("%s\n\n" % msg)
    sys.exit(1)

del foundmanndir
del foundman3dir

def listmanpages(mandir):
    files = os.listdir(mandir)
    names = []
    for file in files:
        if file[-2:-1] == '.' and  (file[-1] in 'ln123456789'):
            names.append(file[:-2])
    names.sort()
    return names

class SelectionBox:

    def __init__(self, master=None):
        self.choices = []

        self.frame = Frame(master, name="frame")
        self.frame.pack(expand=1, fill=BOTH)
        self.master = self.frame.master
        self.subframe = Frame(self.frame, name="subframe")
        self.subframe.pack(expand=0, fill=BOTH)
        self.leftsubframe = Frame(self.subframe, name='leftsubframe')
        self.leftsubframe.pack(side=LEFT, expand=1, fill=BOTH)
        self.rightsubframe = Frame(self.subframe, name='rightsubframe')
        self.rightsubframe.pack(side=RIGHT, expand=1, fill=BOTH)
        self.chaptervar = StringVar(master)
        self.chapter = Menubutton(self.rightsubframe, name='chapter',
                                  text='Directory', relief=RAISED,
                                  borderwidth=2)
        self.chapter.pack(side=TOP)
        self.chaptermenu = Menu(self.chapter, name='chaptermenu')
        self.chaptermenu.add_radiobutton(label='C functions',
                                         value=MAN3DIR,
                                         variable=self.chaptervar,
                                         command=self.newchapter)
        self.chaptermenu.add_radiobutton(label='Tcl/Tk functions',
                                         value=MANNDIR,
                                         variable=self.chaptervar,
                                         command=self.newchapter)
        self.chapter['menu'] = self.chaptermenu
        self.listbox = Listbox(self.rightsubframe, name='listbox',
                               relief=SUNKEN, borderwidth=2,
                               width=20, height=5)
        self.listbox.pack(expand=1, fill=BOTH)
        self.l1 = Button(self.leftsubframe, name='l1',
                         text='Display manual page named:',
                         command=self.entry_cb)
        self.l1.pack(side=TOP)
        self.entry = Entry(self.leftsubframe, name='entry',
                            relief=SUNKEN, borderwidth=2,
                            width=20)
        self.entry.pack(expand=0, fill=X)
        self.l2frame = Frame(self.leftsubframe, name='l2frame')
        self.l2frame.pack(expand=0, fill=NONE)
        self.l2 = Button(self.l2frame, name='l2',
                         text='Search regexp:',
                         command=self.search_cb)
        self.l2.pack(side=LEFT)
        self.casevar = BooleanVar()
        self.casesense = Checkbutton(self.l2frame, name='casesense',
                                     text='Case sensitive',
                                     variable=self.casevar,
                                     relief=FLAT)
        self.casesense.pack(side=LEFT)
        self.search = Entry(self.leftsubframe, name='search',
                            relief=SUNKEN, borderwidth=2,
                            width=20)
        self.search.pack(expand=0, fill=X)
        self.title = Label(self.leftsubframe, name='title',
                           text='(none)')
        self.title.pack(side=BOTTOM)
        self.text = ManPage(self.frame, name='text',
                            relief=SUNKEN, borderwidth=2,
                            wrap=NONE, width=72,
                            selectbackground='pink')
        self.text.pack(expand=1, fill=BOTH)

        self.entry.bind('<Return>', self.entry_cb)
        self.search.bind('<Return>', self.search_cb)
        self.listbox.bind('<Double-1>', self.listbox_cb)

        self.entry.bind('<Tab>', self.entry_tab)
        self.search.bind('<Tab>', self.search_tab)
        self.text.bind('<Tab>', self.text_tab)

        self.entry.focus_set()

        self.chaptervar.set(MANNDIR)
        self.newchapter()

    def newchapter(self):
        mandir = self.chaptervar.get()
        self.choices = []
        self.addlist(listmanpages(mandir))

    def addchoice(self, choice):
        if choice not in self.choices:
            self.choices.append(choice)
            self.choices.sort()
        self.update()

    def addlist(self, list):
        self.choices[len(self.choices):] = list
        self.choices.sort()
        self.update()

    def entry_cb(self, *e):
        self.update()

    def listbox_cb(self, e):
        selection = self.listbox.curselection()
        if selection and len(selection) == 1:
            name = self.listbox.get(selection[0])
            self.show_page(name)

    def search_cb(self, *e):
        self.search_string(self.search.get())

    def entry_tab(self, e):
        self.search.focus_set()

    def search_tab(self, e):
        self.entry.focus_set()

    def text_tab(self, e):
        self.entry.focus_set()

    def updatelist(self):
        key = self.entry.get()
        ok = list(filter(lambda name, key=key, n=len(key): name[:n]==key,
                 self.choices))
        if not ok:
            self.frame.bell()
        self.listbox.delete(0, AtEnd())
        exactmatch = 0
        for item in ok:
            if item == key: exactmatch = 1
            self.listbox.insert(AtEnd(), item)
        if exactmatch:
            return key
        n = self.listbox.size()
        if n == 1:
            return self.listbox.get(0)
        # Else return None, meaning not a unique selection

    def update(self):
        name = self.updatelist()
        if name:
            self.show_page(name)
            self.entry.delete(0, AtEnd())
            self.updatelist()

    def show_page(self, name):
        file = '%s/%s.?' % (self.chaptervar.get(), name)
        fp = os.popen('nroff -man -c %s | ul -i' % file, 'r')
        self.text.kill()
        self.title['text'] = name
        self.text.parsefile(fp)

    def search_string(self, search):
        if not search:
            self.frame.bell()
            print('Empty search string')
            return
        if not self.casevar.get():
            map = re.IGNORECASE
        else:
            map = None
        try:
            if map:
                prog = re.compile(search, map)
            else:
                prog = re.compile(search)
        except re.error as msg:
            self.frame.bell()
            print('Regex error:', msg)
            return
        here = self.text.index(AtInsert())
        lineno = int(here[:here.find('.')])
        end = self.text.index(AtEnd())
        endlineno = int(end[:end.find('.')])
        wraplineno = lineno
        found = 0
        while 1:
            lineno = lineno + 1
            if lineno > endlineno:
                if wraplineno <= 0:
                    break
                endlineno = wraplineno
                lineno = 0
                wraplineno = 0
            line = self.text.get('%d.0 linestart' % lineno,
                                 '%d.0 lineend' % lineno)
            i = prog.search(line)
            if i:
                found = 1
                n = max(1, len(i.group(0)))
                try:
                    self.text.tag_remove('sel',
                                         AtSelFirst(),
                                         AtSelLast())
                except TclError:
                    pass
                self.text.tag_add('sel',
                                  '%d.%d' % (lineno, i.start()),
                                  '%d.%d' % (lineno, i.start()+n))
                self.text.mark_set(AtInsert(),
                                   '%d.%d' % (lineno, i.start()))
                self.text.yview_pickplace(AtInsert())
                break
        if not found:
            self.frame.bell()

def main():
    root = Tk()
    sb = SelectionBox(root)
    if sys.argv[1:]:
        sb.show_page(sys.argv[1])
    root.minsize(1, 1)
    root.mainloop()

main()
