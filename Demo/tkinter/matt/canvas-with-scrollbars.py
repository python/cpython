from Tkinter import *

# This example program creates a scroling canvas, and demonstrates 
# how to tie scrollbars and canvses together. The mechanism
# is analogus for listboxes and other widgets with
# "xscroll" and "yscroll" configuration options.

class Test(Frame):
    def printit(self):
	print "hi"

    def createWidgets(self):
	self.question = Label(self, {"text":  "Can Find The BLUE Square??????", 
				     Pack : {"side" : "top"}})
	
	self.QUIT = Button(self, {'text': 'QUIT', 
				  'bg': 'red', 
				  "height" : "3", 
				  'command': self.quit})
	self.QUIT.pack({'side': 'bottom', 'fill': 'both'})	
	spacer = Frame(self, {"height" : "0.25i", 
			      Pack : {"side" : "bottom"}})

	# notice that the scroll region (20" x 20") is larger than 
	# displayed size of the widget (5" x 5")
	self.draw = Canvas(self, {"width" : "5i", 
				  "height" : "5i", 
				  "bg" : "white", 
				  "scrollregion" : "0i 0i 20i 20i"})

	
	self.draw.scrollX = Scrollbar(self, {"orient" : "horizontal"}) 
	self.draw.scrollY = Scrollbar(self, {"orient" : "vertical"}) 

	# now tie the three together. This is standard boilerplate text
	self.draw['xscroll'] = self.draw.scrollX.set
	self.draw['yscroll'] = self.draw.scrollY.set
	self.draw.scrollX['command'] = self.draw.xview
	self.draw.scrollY['command'] = self.draw.yview

	# draw something. Note that the first square 
	# is visible, but you need to scroll to see the second one.
	self.draw.create_polygon("0i", "0i", "3.5i", "0i", "3.5i", "3.5i", "0i" , "3.5i")
	self.draw.create_polygon("10i", "10i", "13.5i", "10i", "13.5i", "13.5i", "10i" , "13.5i", "-fill", "blue")

	
	# pack 'em up
	self.draw.scrollX.pack({'side': 'bottom', 
				"fill" : "x"})
	self.draw.scrollY.pack({'side': 'right', 
				"fill" : "y"})
	self.draw.pack({'side': 'left'})


    def scrollCanvasX(self, *args): 
	print "scrolling", args
	print self.draw.scrollX.get()


    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()

test.mainloop()
