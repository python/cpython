from Tkinter import *

# this program creates a canvas and puts a single polygon on the canvas

class Test(Frame):
    def printit(self):
        print "hi"

    def createWidgets(self):
        self.QUIT = Button(self, text='QUIT', foreground='red',
                           command=self.quit)
        self.QUIT.pack(side=BOTTOM, fill=BOTH)

        self.draw = Canvas(self, width="5i", height="5i")

        # see the other demos for other ways of specifying coords for a polygon
        self.draw.create_rectangle(0, 0, "3i", "3i", fill="black")

        self.draw.pack(side=LEFT)

    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()

test = Test()

test.mainloop()
