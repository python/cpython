from Tkinter import *

# This is a program that makes a simple two button application


class New_Button(Button):
    def callback(self):
        print self.counter
        self.counter = self.counter + 1

def createWidgets(top):
    f = Frame(top)
    f.pack()
    f.QUIT = Button(f, text='QUIT', foreground='red', command=top.quit)

    f.QUIT.pack(side=LEFT, fill=BOTH)

    # a hello button
    f.hi_there = New_Button(f, text='Hello')
    # we do this on a different line because we need to reference f.hi_there
    f.hi_there.config(command=f.hi_there.callback)
    f.hi_there.pack(side=LEFT)
    f.hi_there.counter = 43


root = Tk()
createWidgets(root)
root.mainloop()
