#! /usr/bin/env python

"""Play with the new Tk 8.0 toplevel menu option."""

from Tkinter import *

class App:

    def __init__(self, master):
        self.master = master

        self.menubar = Menu(self.master)

        self.filemenu = Menu(self.menubar)

        self.filemenu.add_command(label="New")
        self.filemenu.add_command(label="Open...")
        self.filemenu.add_command(label="Close")
        self.filemenu.add_separator()
        self.filemenu.add_command(label="Quit", command=self.master.quit)

        self.editmenu = Menu(self.menubar)

        self.editmenu.add_command(label="Cut")
        self.editmenu.add_command(label="Copy")
        self.editmenu.add_command(label="Paste")

        self.helpmenu = Menu(self.menubar, name='help')

        self.helpmenu.add_command(label="About...")

        self.menubar.add_cascade(label="File", menu=self.filemenu)
        self.menubar.add_cascade(label="Edit", menu=self.editmenu)
        self.menubar.add_cascade(label="Help", menu=self.helpmenu)

        self.top = Toplevel(menu=self.menubar)

        # Rest of app goes here...

def main():
    root = Tk()
    root.withdraw()
    app = App(root)
    root.mainloop()

if __name__ == '__main__':
    main()
