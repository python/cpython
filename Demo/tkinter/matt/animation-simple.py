from Tkinter import *

# This program shows how to use the "after" function to make animation.

class Test(Frame):
    def printit(self):
	print "hi"

    def createWidgets(self):
	self.QUIT = Button(self, {'text': 'QUIT', 
				  'fg': 'red', 
				  'command': self.quit})
	self.QUIT.pack({'side': 'left', 'fill': 'both'})	

	self.draw = Canvas(self, {"width" : "5i", "height" : "5i"})

	# all of these work..
	self.draw.create_polygon("0", "0", "10", "0", "10", "10", "0" , "10", {"tags" : "thing"})
	self.draw.pack({'side': 'left'})

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
