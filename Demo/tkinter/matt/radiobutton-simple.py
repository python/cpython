from Tkinter import *

# This is a demo program that shows how to 
# create radio buttons and how to get other widgets to 
# share the information in a radio button. 
# 
# There are other ways of doing this too, but 
# the "variable" option of radiobuttons seems to be the easiest.
#
# note how each button has a value it sets the variable to as it gets hit.


class Test(Frame):
    def printit(self):
	print "hi"

    def createWidgets(self):

	self.flavor = StringVar()
	self.flavor.set("chocolate")

	self.radioframe = Frame(self)
	self.radioframe.pack()

	# 'text' is the label
	# 'variable' is the name of the variable that all these radio buttons share
	# 'value' is the value this variable takes on when the radio button is selected
	# 'anchor' makes the text appear left justified (default is centered. ick)
	self.radioframe.choc = Radiobutton (self.radioframe, {"text" : "Chocolate Flavor", 
							      "variable" : self.flavor,
							      "value" : "chocolate",
							      "anchor" : "w", 
							      Pack : {"side" : "top", "fill" : "x"}})

	self.radioframe.straw = Radiobutton (self.radioframe, {"text" : "Strawberry Flavor", 
							       "variable" : self.flavor,
							      "anchor" : "w", 
							       "value" : "strawberry", 
							       Pack : {"side" : "top", "fill" : "x"}})

	self.radioframe.lemon = Radiobutton (self.radioframe, {"text" : "Lemon Flavor", 
							      "anchor" : "w", 
							       "variable" : self.flavor,
							       "value" : "lemon", 
							       Pack : {"side" : "top", "fill" : "x"}})

	
	# this is a text entry that lets you type in the name of a flavor too.
	self.entry = Entry(self, {"textvariable" : self.flavor, 
				  Pack : {"side" : "top", "fill" : "x"}})
	self.QUIT = Button(self, {'text': 'QUIT', 
				  'fg': 'red', 
				  'command': self.quit})
	
	self.QUIT.pack({'side': 'bottom', 'fill': 'both'})



    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()

test.mainloop()
