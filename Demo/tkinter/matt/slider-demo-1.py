from Tkinter import *

# shows how to make a slider, set and get its value under program control


class Test(Frame):
    def print_value(self, val):
	print "slider now at", val

    def reset(self): 
	self.slider.set(0)

    def createWidgets(self):
	self.slider = Scale(self, {"from" : 0,
				   'to': 100, 
				   "orient" : "horizontal", 
				   "length" : "3i", 
				   "label" : "happy slider", 
				   'command' : self.print_value})

	self.reset = Button(self, {'text': 'reset slider', 
				  'command': self.reset})
	

	self.QUIT = Button(self, {'text': 'QUIT', 
				  'fg': 'red', 
				  'command': self.quit})
	

	self.slider.pack({'side': 'left'})
	self.reset.pack({'side': 'left'})
	self.QUIT.pack({'side': 'left', 'fill': 'both'})

    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()
test.mainloop()
