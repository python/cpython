from Tkinter import *

# this program creates a canvas and puts a single polygon on the canvas

class Test(Frame):
    def printit(self):
	print "hi"

    def createWidgets(self):
	self.QUIT = Button(self, {'text': 'QUIT', 
				  'fg': 'red', 
				  'command': self.quit})
	self.QUIT.pack({'side': 'bottom', 'fill': 'both'})	

	self.draw = Canvas(self, {"width" : "5i", "height" : "5i"})

	# see the other demos for other ways of specifying coords for a polygon
	self.draw.create_polygon("0i", "0i", "3i", "0i", "3i", "3i", "0i" , "3i")

	self.draw.pack({'side': 'left'})


    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()

test.mainloop()
