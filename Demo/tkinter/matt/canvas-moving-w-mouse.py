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
	# whatever the mouse is over gets tagged as "current" for free by tk.
	self.draw.move("current", event.x - self.lastx, event.y - self.lasty)
	self.lastx = event.x
	self.lasty = event.y

    ###################################################################
    ###### Event callbacks for canvas ITEMS (stuff drawn on the canvas)
    ###################################################################
    def mouseEnter(self, event):
        # the "current" tag is applied to the object the cursor is over.
	# this happens automatically.
	self.draw.itemconfig("current", {"fill" : "red"})
	
    def mouseLeave(self, event):
	# the "current" tag is applied to the object the cursor is over.
	# this happens automatically.
	self.draw.itemconfig("current", {"fill" : "blue"})


    def createWidgets(self):
	self.QUIT = Button(self, {'text': 'QUIT', 
				  'fg': 'red', 
				  'command': self.quit})
	self.QUIT.pack({'side': 'left', 'fill': 'both'})	
	self.draw = Canvas(self, {"width" : "5i", "height" : "5i"})
	self.draw.pack({'side': 'left'})
	

	fred = self.draw.create_oval(0, 0, 20, 20,
				     {"fill" : "green", "tag" : "selected"})

	self.draw.bind(fred, "<Any-Enter>", self.mouseEnter)
	self.draw.bind(fred, "<Any-Leave>", self.mouseLeave)
	    

	Widget.bind(self.draw, "<1>", self.mouseDown)
	Widget.bind(self.draw, "<B1-Motion>", self.mouseMove)

    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()
test.mainloop()
