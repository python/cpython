from Tkinter import *

# this shows how to create a new window with a button in it that can create new windows

class Test(Frame):
    def printit(self):
	print "hi"

    def makeWindow(self):
	# there is no Tkinter interface to the dialog box. Making one would mean putting 
	# a few wrapper functions in the Tkinter.py file.
	# even better is to put in a SUIT-like selection of commonly-used dialogs.
	# the parameters to this call are as follows: 

	fred = Toplevel()               # a toplevel window that the dialog goes into


	# this function returns the index of teh button chosen. In this case, 0 for "yes" and 1 for "no"

	print self.tk.call("tk_dialog",           # the command name
			   fred,                  # the name of a toplevel window
			   "fred the dialog box", # the title on the window
			   "click on a choice",   # the message to appear in the window
			   "info",                # the bitmap (if any) to appear. If no bitmap is desired, pass ""
			                          #     legal values here are:
			                          #        string      what it looks like
			                          #        ----------------------------------------------
			                          #        error       a circle with a slash through it
						  #	   grey25      grey square
						  #	   grey50      darker grey square
						  #	   hourglass   use for "wait.."
						  #	   info        a large, lower case "i"
						  #	   questhead   a human head with a "?" in it
						  #	   question    a large "?"
						  #	   warning     a large "!" 
			                          #        @fname      any X bitmap where fname is the path to the file  
			                          #
			   "0",                   # the index of the default button choice. hitting return selects this
			   "yes", "no")           # all remaining parameters are the labels for the 
	                                          # buttons that appear left to right in the dialog box

      

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
