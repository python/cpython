from Tkinter import *


class Test(Frame):
    def printit(self):
	print "hi"

    def createWidgets(self):
	self.QUIT = Button(self, text='QUIT', foreground='red', 
			   command=self.quit)
	self.QUIT.pack(side=BOTTOM, fill=BOTH)

	self.drawing = Canvas(self, width="5i", height="5i")

	# make a shape
	pgon = self.drawing.create_polygon(
	    10, 10, 110, 10, 110, 110, 10 , 110,
	    fill="red", tags=("weee", "foo", "groo"))

	# this is how you query an object for its attributes 
	# config options FOR CANVAS ITEMS always come back in tuples of length 5.
	# 0 attribute name
	# 1 BLANK 
	# 2 BLANK 
	# 3 default value
	# 4 current value
	# the blank spots are for consistency with the config command that 
	# is used for widgets. (remember, this is for ITEMS drawn 
	# on a canvas widget, not widgets)
	option_value = self.drawing.itemconfig(pgon, "stipple")
	print "pgon's current stipple value is -->", option_value[4], "<--"
	option_value = self.drawing.itemconfig(pgon,  "fill")
	print "pgon's current fill value is -->", option_value[4], "<--"
	print "  when he is usually colored -->", option_value[3], "<--"

	## here we print out all the tags associated with this object
	option_value = self.drawing.itemconfig(pgon,  "tags")
	print "pgon's tags are", option_value[4]

	self.drawing.pack(side=LEFT)

    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()

test.mainloop()
