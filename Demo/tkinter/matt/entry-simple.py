from Tkinter import *
import string 

# This program  shows how to use a simple type-in box

class App(Frame):
    def __init__(self, master=None):
	Frame.__init__(self, master)
	self.pack()

	self.entrythingy = Entry()
	self.entrythingy.pack()

	# and here we get a callback when the user hits return. we could
	# make the key that triggers the callback anything we wanted to.
	# other typical options might be <Key-Tab> or <Key> (for anything)
	self.entrythingy.bind('<Key-Return>', self.print_contents)

    def print_contents(self, event):
	print "hi. contents of entry is now ---->", self.entrythingy.get()

root = App()
root.master.title("Foo")
root.mainloop()

