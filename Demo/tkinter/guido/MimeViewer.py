#! /usr/bin/env python

# View a single MIME multipart message.
# Display each part as a box.

import string
from types import *
from Tkinter import *
from ScrolledText import ScrolledText

class MimeViewer:
    def __init__(self, parent, title, msg):
        self.title = title
        self.msg = msg
        self.frame = Frame(parent, {'relief': 'raised', 'bd': 2})
        self.frame.packing = {'expand': 0, 'fill': 'both'}
        self.button = Checkbutton(self.frame,
                             {'text': title,
                              'command': self.toggle})
        self.button.pack({'anchor': 'w'})
        headertext = msg.getheadertext(
                lambda x: x != 'received' and x[:5] != 'x400-')
        height = countlines(headertext, 4)
        if height:
            self.htext = ScrolledText(self.frame,
                              {'height': height,
                               'width': 80,
                               'wrap': 'none',
                               'relief': 'raised',
                               'bd': 2})
            self.htext.packing = {'expand': 1, 'fill': 'both',
                                  'after': self.button}
            self.htext.insert('end', headertext)
        else:
            self.htext = Frame(self.frame,
                               {'relief': 'raised', 'bd': 2})
            self.htext.packing = {'side': 'top',
                                  'ipady': 2,
                                  'fill': 'x',
                                  'after': self.button}
        body = msg.getbody()
        if type(body) == StringType:
            self.pad = None
            height = countlines(body, 10)
            if height:
                self.btext = ScrolledText(self.frame,
                                  {'height': height,
                                   'width': 80,
                                   'wrap': 'none',
                                   'relief': 'raised',
                                   'bd': 2})
                self.btext.packing = {'expand': 1,
                                      'fill': 'both'}
                self.btext.insert('end', body)
            else:
                self.btext = None
            self.parts = None
        else:
            self.pad = Frame(self.frame,
                             {'relief': 'flat', 'bd': 2})
            self.pad.packing = {'side': 'left', 'ipadx': 10,
                                'fill': 'y', 'after': self.htext}
            self.parts = []
            for i in range(len(body)):
                p = MimeViewer(self.frame,
                               '%s.%d' % (title, i+1),
                               body[i])
                self.parts.append(p)
            self.btext = None
        self.collapsed = 1
    def pack(self):
        self.frame.pack(self.frame.packing)
    def destroy(self):
        self.frame.destroy()
    def show(self):
        if self.collapsed:
            self.button.invoke()
    def toggle(self):
        if self.collapsed:
            self.explode()
        else:
            self.collapse()
    def collapse(self):
        self.collapsed = 1
        for comp in self.htext, self.btext, self.pad:
            if comp:
                comp.forget()
        if self.parts:
            for part in self.parts:
                part.frame.forget()
        self.frame.pack({'expand': 0})
    def explode(self):
        self.collapsed = 0
        for comp in self.htext, self.btext, self.pad:
            if comp: comp.pack(comp.packing)
        if self.parts:
            for part in self.parts:
                part.pack()
        self.frame.pack({'expand': 1})

def countlines(str, limit):
    i = 0
    n = 0
    while  n < limit:
        i = string.find(str, '\n', i)
        if i < 0: break
        n = n+1
        i = i+1
    return n

def main():
    import sys
    import getopt
    import mhlib
    opts, args = getopt.getopt(sys.argv[1:], '')
    for o, a in opts:
        pass
    message = None
    folder = 'inbox'
    for arg in args:
        if arg[:1] == '+':
            folder = arg[1:]
        else:
            message = string.atoi(arg)

    mh = mhlib.MH()
    f = mh.openfolder(folder)
    if not message:
        message = f.getcurrent()
    m = f.openmessage(message)

    root = Tk()
    tk = root.tk

    top = MimeViewer(root, '+%s/%d' % (folder, message), m)
    top.pack()
    top.show()

    root.minsize(1, 1)

    tk.mainloop()

if __name__ == '__main__': main()
