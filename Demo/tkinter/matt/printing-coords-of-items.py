from Tkinter import *

# this file demonstrates the creation of widgets as part of a canvas object

class Test(Frame):
    ###################################################################
    ###### Event callbacks for THE CANVAS (not the stuff drawn on it)
    ###################################################################
    def mouseDown(self, event):
	# see if we're inside a dot. If we are, it
	# gets tagged as CURRENT for free by tk.

	if not event.widget.find_withtag(CURRENT):
	    # there is no dot here, so we can make one,
	    # and bind some interesting behavior to it.
	    # ------
	    # create a dot, and mark it as current
	    fred = self.draw.create_oval(
		event.x - 10, event.y -10, event.x +10, event.y + 10,
		fill="green")
	    self.draw.tag_bind(fred, "<Enter>", self.mouseEnter)
	    self.draw.tag_bind(fred, "<Leave>", self.mouseLeave)
	self.lastx = event.x
	self.lasty = event.y

    def mouseMove(self, event):
	self.draw.move(CURRENT, event.x - self.lastx, event.y - self.lasty)
	self.lastx = event.x
	self.lasty = event.y

    ###################################################################
    ###### Event callbacks for canvas ITEMS (stuff drawn on the canvas)
    ###################################################################
    def mouseEnter(self, event):
        # the "current" tag is applied to the object the cursor is over.
	# this happens automatically.
	self.draw.itemconfig(CURRENT, fill="red")
	print self.draw.coords(CURRENT)
	
    def mouseLeave(self, event):
	# the "current" tag is applied to the object the cursor is over.
	# this happens automatically.
	self.draw.itemconfig(CURRENT, fill="blue")

    def createWidgets(self):
	self.QUIT = Button(self, text='QUIT', foreground='red',
			   command=self.quit)
	self.QUIT.pack(side=LEFT, fill=BOTH)	
	self.draw = Canvas(self, width="5i", height="5i")
	self.draw.pack(side=LEFT)

	Widget.bind(self.draw, "<1>", self.mouseDown)
	Widget.bind(self.draw, "<B1-Motion>", self.mouseMove)

    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()
test.mainloop()



