import sys
import re

from Tkinter import *

class PopupMenu:

    def __init__(self, text, flist):
        self.text = text
        self.flist = flist
        self.text.bind("<3>", self.right_menu_event)

    rmenu = None

    def right_menu_event(self, event):
        if not self.rmenu:
            self.make_menu()
        rmenu = self.rmenu
        self.event = event
        iswin = sys.platform[:3] == 'win'
        if iswin:
            self.text.config(cursor="arrow")
        rmenu.tk_popup(event.x_root, event.y_root)
        if iswin:
            self.text.config(cursor="ibeam")

    def make_menu(self):
        rmenu = Menu(self.text, tearoff=0)
        rmenu.add_command(label="Go to line from traceback",
                          command=self.goto_traceback_line)
        rmenu.add_command(label="Open stack viewer",
                          command=self.open_stack_viewer)
        rmenu.add_command(label="Help", command=self.help)
        self.rmenu = rmenu
    
    file_line_pats = [
        r'File "([^"]*)", line (\d+)',
        r'([^\s]+)\((\d+)\)',
        r'([^\s]+):\s*(\d+):',
    ]
    
    file_line_progs = None
    
    def goto_traceback_line(self):
        if self.file_line_progs is None:
            l = []
            for pat in self.file_line_pats:
                l.append(re.compile(pat))
            self.file_line_progs = l
        x, y = self.event.x, self.event.y
        self.text.mark_set("insert", "@%d,%d" % (x, y))
        line = self.text.get("insert linestart", "insert lineend")
        for prog in self.file_line_progs:
            m = prog.search(line)
            if m:
                break
        else:
            self.text.bell()
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
    
    def open_stack_viewer(self):
        try:
            sys.last_traceback
        except:
            print "No stack trace yet"
            return
        from StackViewer import StackViewer
        sv = StackViewer(self.text._root(), self.flist)

    def help(self):
        from HelpWindow import HelpWindow
        HelpWindow(root=self.flist.root)
