from Tkinter import *

import sys
sys.path.append("/users/mjc4y/projects/python/tkinter/utils")
from TkinterUtils  import *

# this shows how to create a new window with a button in it that can create new windows


class Test(Frame):
    def makeWindow(self, *args):
	fred = Toplevel()

	fred.label = Canvas (fred, {"width" : "2i", 
				       "height" : "2i"})

	fred.label.create_line("0", "0", "2i", "2i")
	fred.label.create_line("0", "2i", "2i", "0")
	fred.label.pack()

	centerWindow(fred, self.master)

    def createWidgets(self):
	self.QUIT = QuitButton(self)
	self.QUIT.pack({'side': 'left', 'fill': 'both'})


	self.makeWindow = Button(self, {'text': 'Make a New Window', 
				  'width' : 50,
				  'height' : 20,
				      'command' : self.makeWindow})
	self.makeWindow.pack({'side': 'left'})

    def __init__(self, master=None):
	Frame.__init__(self, master)
	Pack.config(self)
	self.createWidgets()

test = Test()
test.mainloop()
