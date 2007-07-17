from Tkinter import *

# This program shows how to use the "after" function to make animation.

class Test(Frame):
    def printit(self):
        print("hi")

    def createWidgets(self):
        self.QUIT = Button(self, text='QUIT', foreground='red',
                           command=self.quit)
        self.QUIT.pack(side=LEFT, fill=BOTH)

        self.draw = Canvas(self, width="5i", height="5i")

        # all of these work..
        self.draw.create_rectangle(0, 0, 10, 10, tags="thing", fill="blue")
        self.draw.pack(side=LEFT)

    def moveThing(self, *args):
        # move 1/10 of an inch every 1/10 sec (1" per second, smoothly)
        self.draw.move("thing", "0.01i", "0.01i")
        self.after(10, self.moveThing)


    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()
        self.after(10, self.moveThing)


test = Test()

test.mainloop()
