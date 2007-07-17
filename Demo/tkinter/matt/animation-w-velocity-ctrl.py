from Tkinter import *

# this is the same as simple-demo-1.py, but uses
# subclassing.
# note that there is no explicit call to start Tk.
# Tkinter is smart enough to start the system if it's not already going.


class Test(Frame):
    def printit(self):
        print("hi")

    def createWidgets(self):
        self.QUIT = Button(self, text='QUIT', foreground='red',
                           command=self.quit)
        self.QUIT.pack(side=BOTTOM, fill=BOTH)

        self.draw = Canvas(self, width="5i", height="5i")

        self.speed = Scale(self, orient=HORIZONTAL, from_=-100, to=100)

        self.speed.pack(side=BOTTOM, fill=X)

        # all of these work..
        self.draw.create_rectangle(0, 0, 10, 10, tags="thing", fill="blue")
        self.draw.pack(side=LEFT)

    def moveThing(self, *args):
        velocity = self.speed.get()
        str = float(velocity) / 1000.0
        str = "%ri" % (str,)
        self.draw.move("thing",  str, str)
        self.after(10, self.moveThing)

    def __init__(self, master=None):
        Frame.__init__(self, master)
        Pack.config(self)
        self.createWidgets()
        self.after(10, self.moveThing)


test = Test()

test.mainloop()
