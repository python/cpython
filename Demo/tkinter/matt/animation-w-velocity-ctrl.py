from Tkinter import *

# this is the same as simple-demo-1.py, but uses 
# subclassing. 
# note that there is no explicit call to start Tk. 
# Tkinter is smart enough to start the system if it's not already going. 




class Test(Frame):
    def printit(self):
	print "hi"

    def createWidgets(self):
	self.QUIT = Button(self, {'text': 'QUIT', 
				  'fg': 'red', 
				  'command': self.quit})
	self.QUIT.pack({'side': 'bottom', 'fill': 'both'})	

	self.draw = Canvas(self, {"width" : "5i", "height" : "5i"})

	self.speed = Scale(self, {"orient":  "horiz", 
				  "from" : -100, 
				  "to" : 100})

	self.speed.pack({'side': 'bottom', "fill" : "x"})

	# all of these work..
	self.draw.create_polygon("0", "0", "10", "0", "10", "10", "0" , "10", {"tags" : "thing"})
	self.draw.pack({'side': 'left'})

    def moveThing(self, *args):
	velocity = self.speed.get()
	str = float(velocity) / 1000.0
	str = `str` + "i"
	self.draw.move("thing",  str, str)
	self.after(10, self.moveThing)



    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()
	self.after(10, self.moveThing)


test = Test()

test.mainloop()
