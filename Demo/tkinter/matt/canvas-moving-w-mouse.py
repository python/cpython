from Tkinter import *

# this file demonstrates the movement of a single canvas item under mouse control

class Test(Frame):
    ###################################################################
    ###### Event callbacks for THE CANVAS (not the stuff drawn on it)
    ###################################################################
    def mouseDown(self, event):
	# remember where the mouse went down
	self.lastx = event.x
	self.lasty = event.y

    def mouseMove(self, event):
	# whatever the mouse is over gets tagged as CURRENT for free by tk.
	self.draw.move(CURRENT, event.x - self.lastx, event.y - self.lasty)
	self.lastx = event.x
	self.lasty = event.y

    ###################################################################
    ###### Event callbacks for canvas ITEMS (stuff drawn on the canvas)
    ###################################################################
    def mouseEnter(self, event):
        # the CURRENT tag is applied to the object the cursor is over.
	# this happens automatically.
	self.draw.itemconfig(CURRENT, fill="red")
	
    def mouseLeave(self, event):
	# the CURRENT tag is applied to the object the cursor is over.
	# this happens automatically.
	self.draw.itemconfig(CURRENT, fill="blue")

    def createWidgets(self):
	self.QUIT = Button(self, text='QUIT', foreground='red', 
			   command=self.quit)
	self.QUIT.pack(side=LEFT, fill=BOTH)
	self.draw = Canvas(self, width="5i", height="5i")
	self.draw.pack(side=LEFT)

	fred = self.draw.create_oval(0, 0, 20, 20,
				     fill="green", tags="selected")

	self.draw.tag_bind(fred, "<Any-Enter>", self.mouseEnter)
	self.draw.tag_bind(fred, "<Any-Leave>", self.mouseLeave)

	Widget.bind(self.draw, "<1>", self.mouseDown)
	Widget.bind(self.draw, "<B1-Motion>", self.mouseMove)

    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()
test.mainloop()
