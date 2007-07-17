from Tkinter import *

# this file demonstrates the creation of widgets as part of a canvas object

class Test(Frame):
    def printhi(self):
        print("hi")

    def createWidgets(self):
        self.QUIT = Button(self, text='QUIT', foreground='red',
                           command=self.quit)
        self.QUIT.pack(side=BOTTOM, fill=BOTH)

        self.draw = Canvas(self, width="5i", height="5i")

        self.button = Button(self, text="this is a button",
                             command=self.printhi)

        # note here the coords are given in pixels (form the
        # upper right and corner of the window, as usual for X)
        # but might just have well been given in inches or points or
        # whatever...use the "anchor" option to control what point of the
        # widget (in this case the button) gets mapped to the given x, y.
        # you can specify corners, edges, center, etc...
        self.draw.create_window(300, 300, window=self.button)

        self.draw.pack(side=LEFT)

    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()

test = Test()

test.mainloop()
