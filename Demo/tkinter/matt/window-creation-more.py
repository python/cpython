from Tkinter import *

# this shows how to create a new window with a button in it that can create new windows

class Test(Frame):
    def printit(self):
	print "hi"

    def makeWindow(self):
	fred = Toplevel()
	fred.label = Button(fred, {'text': "This is window number " + `self.windownum` + "." , 
				   'command' : self.makeWindow})
	fred.label.pack()
	self.windownum = self.windownum + 1

    def createWidgets(self):
	self.QUIT = Button(self, {'text': 'QUIT', 
				  'fg': 'red', 
				  'command': self.quit})
	
	self.QUIT.pack({'side': 'left', 'fill': 'both'})


	# a hello button
	self.hi_there = Button(self, {'text': 'Make a New Window', 
				      'command' : self.makeWindow})
	self.hi_there.pack({'side': 'left'})


    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.windownum = 0 
	self.createWidgets()

test = Test()
test.mainloop()
