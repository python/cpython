from Tkinter import *

# this is the same as simple-demo-1.py, but uses
# subclassing.
# note that there is no explicit call to start Tk.
# Tkinter is smart enough to start the system if it's not already going.

class Test(Frame):
    def printit(self):
        print("hi")

    def createWidgets(self):
        self.QUIT = Button(self, text='QUIT',
                                  background='red',
                                  foreground='white',
                                  height=3,
                                  command=self.quit)
        self.QUIT.pack(side=BOTTOM, fill=BOTH)

        self.canvasObject = Canvas(self, width="5i", height="5i")
        self.canvasObject.pack(side=LEFT)

    def mouseDown(self, event):
        # canvas x and y take the screen coords from the event and translate
        # them into the coordinate system of the canvas object
        self.startx = self.canvasObject.canvasx(event.x, self.griddingSize)
        self.starty = self.canvasObject.canvasy(event.y, self.griddingSize)

    def mouseMotion(self, event):
        # canvas x and y take the screen coords from the event and translate
        # them into the coordinate system of the canvas object
        x = self.canvasObject.canvasx(event.x, self.griddingSize)
        y = self.canvasObject.canvasy(event.y, self.griddingSize)

        if (self.startx != event.x)  and (self.starty != event.y) :
            self.canvasObject.delete(self.rubberbandBox)
            self.rubberbandBox = self.canvasObject.create_rectangle(
                self.startx, self.starty, x, y)
            # this flushes the output, making sure that
            # the rectangle makes it to the screen
            # before the next event is handled
            self.update_idletasks()

    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()

        # this is a "tagOrId" for the rectangle we draw on the canvas
        self.rubberbandBox = None

        # this is the size of the gridding squares
        self.griddingSize = 50

        Widget.bind(self.canvasObject, "<Button-1>", self.mouseDown)
        Widget.bind(self.canvasObject, "<Button1-Motion>", self.mouseMotion)


test = Test()

test.mainloop()
