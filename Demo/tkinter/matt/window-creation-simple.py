from Tkinter import *

# this shows how to spawn off new windows at a button press

class Test(Frame):
    def printit(self):
        print "hi"

    def makeWindow(self):
        fred = Toplevel()
        fred.label = Label(fred, text="Here's a new window")
        fred.label.pack()

    def createWidgets(self):
        self.QUIT = Button(self, text='QUIT', foreground='red',
                           command=self.quit)

        self.QUIT.pack(side=LEFT, fill=BOTH)

        # a hello button
        self.hi_there = Button(self, text='Make a New Window',
                               command=self.makeWindow)
        self.hi_there.pack(side=LEFT)

    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()

test = Test()
test.mainloop()
