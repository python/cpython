from Tkinter import *

# note that there is no explicit call to start Tk. 
# Tkinter is smart enough to start the system if it's not already going. 

class Test(Frame):
    def printit(self):
	print "hi"

    def createWidgets(self):
	self.QUIT = Button(self, text='QUIT', foreground='red', 
			   command=self.quit)
	
	self.QUIT.pack(side=LEFT, fill=BOTH)

	# a hello button
	self.hi_there = Button(self, text='Hello', 
			       command=self.printit)
	self.hi_there.pack(side=LEFT)

    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()
test.mainloop()
