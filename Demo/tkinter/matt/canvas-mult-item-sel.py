from Tkinter import *

# allows moving dots with multiple selection. 

SELECTED_COLOR = "red"
UNSELECTED_COLOR = "blue"

class Test(Frame):
    ###################################################################
    ###### Event callbacks for THE CANVAS (not the stuff drawn on it)
    ###################################################################
    def mouseDown(self, event):
	# see if we're inside a dot. If we are, it
	# gets tagged as CURRENT for free by tk.

	if not event.widget.find_withtag(CURRENT):
	    # we clicked outside of all dots on the canvas. unselect all.
	    
	    # re-color everything back to an unselected color
	    self.draw.itemconfig("selected", fill=UNSELECTED_COLOR)
	    # unselect everything
	    self.draw.dtag("selected")
	else:
	    # mark as "selected" the thing the cursor is under
	    self.draw.addtag("selected", "withtag", CURRENT)
	    # color it as selected
	    self.draw.itemconfig("selected", fill=SELECTED_COLOR)

	self.lastx = event.x
	self.lasty = event.y
		
	
    def mouseMove(self, event):
	self.draw.move("selected", event.x - self.lastx, event.y - self.lasty)
	self.lastx = event.x
	self.lasty = event.y

    def makeNewDot(self):
	# create a dot, and mark it as current
	fred = self.draw.create_oval(0, 0, 20, 20, 
				     fill=SELECTED_COLOR, tags=CURRENT)
	# and make it selected
	self.draw.addtag("selected", "withtag", CURRENT)
	
    def createWidgets(self):
	self.QUIT = Button(self, text='QUIT', foreground='red',
			   command=self.quit)

	################
	# make the canvas and bind some behavior to it
	################
	self.draw = Canvas(self, width="5i", height="5i")
	Widget.bind(self.draw, "<1>", self.mouseDown)
	Widget.bind(self.draw, "<B1-Motion>", self.mouseMove)

	# and other things.....
	self.button = Button(self, text="make a new dot", foreground="blue",
			     command=self.makeNewDot)

	message = ("%s dots are selected and can be dragged.\n"
		   "%s are not selected.\n"
		   "Click in a dot to select it.\n"
		   "Click on empty space to deselect all dots."
		   ) % (SELECTED_COLOR, UNSELECTED_COLOR)
	self.label = Message(self, width="5i", text=message)

	self.QUIT.pack(side=BOTTOM, fill=BOTH)
	self.label.pack(side=BOTTOM, fill=X, expand=1)
	self.button.pack(side=BOTTOM, fill=X)
	self.draw.pack(side=LEFT)

    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()
test.mainloop()



