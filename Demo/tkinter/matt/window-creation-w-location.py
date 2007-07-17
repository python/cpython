from Tkinter import *

import sys
##sys.path.append("/users/mjc4y/projects/python/tkinter/utils")
##from TkinterUtils  import *

# this shows how to create a new window with a button in it that
# can create new windows

class QuitButton(Button):
    def __init__(self, master, *args, **kwargs):
        if "text" not in kwargs:
            kwargs["text"] = "QUIT"
        if "command" not in kwargs:
            kwargs["command"] = master.quit
        Button.__init__(self, master, *args, **kwargs)

class Test(Frame):
    def makeWindow(self, *args):
        fred = Toplevel()

        fred.label = Canvas (fred, width="2i", height="2i")

        fred.label.create_line("0", "0", "2i", "2i")
        fred.label.create_line("0", "2i", "2i", "0")
        fred.label.pack()

        ##centerWindow(fred, self.master)

    def createWidgets(self):
        self.QUIT = QuitButton(self)
        self.QUIT.pack(side=LEFT, fill=BOTH)

        self.makeWindow = Button(self, text='Make a New Window',
                                 width=50, height=20,
                                 command=self.makeWindow)
        self.makeWindow.pack(side=LEFT)

    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()

test = Test()
test.mainloop()
