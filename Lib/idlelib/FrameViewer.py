from repr import Repr
from Tkinter import *

class FrameViewer:

    def __init__(self, root, frame):
        self.root = root
        self.frame = frame
        self.top = Toplevel(self.root)
        self.repr = Repr()
        self.repr.maxstring = 60
        self.load_variables()

    def load_variables(self):
        row = 0
        if self.frame.f_locals is not self.frame.f_globals:
            l = Label(self.top, text="Local Variables",
                      borderwidth=2, relief="raised")
            l.grid(row=row, column=0, columnspan=2, sticky="ew")
            row = self.load_names(self.frame.f_locals, row+1)
        l = Label(self.top, text="Global Variables",
                  borderwidth=2, relief="raised")
        l.grid(row=row, column=0, columnspan=2, sticky="ew")
        row = self.load_names(self.frame.f_globals, row+1)

    def load_names(self, dict, row):
        names = dict.keys()
        names.sort()
        for name in names:
            value = dict[name]
            svalue = self.repr.repr(value)
            l = Label(self.top, text=name)
            l.grid(row=row, column=0, sticky="w")
            l = Entry(self.top, width=60, borderwidth=0)
            l.insert(0, svalue)
            l.grid(row=row, column=1, sticky="w")
            row = row+1
        return row
