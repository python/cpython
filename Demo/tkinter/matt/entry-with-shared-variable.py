from Tkinter import *
import string 

# This program  shows how to make a typein box shadow a program variable.

class App(Frame):
    def __init__(self, master=None):
	Frame.__init__(self, master)
	self.pack()

	self.entrythingy = Entry(self)
	self.entrythingy.pack()

	self.button = Button(self, text="Uppercase The Entry",
			     command=self.upper)
	self.button.pack()

	# here we have the text in the entry widget tied to a variable.
	# changes in the variable are echoed in the widget and vice versa. 
	# Very handy.
	# there are other Variable types. See Tkinter.py for all
	# the other variable types that can be shadowed
	self.contents = StringVar()
	self.contents.set("this is a variable")
	self.entrythingy.config(textvariable=self.contents)

	# and here we get a callback when the user hits return. we could
	# make the key that triggers the callback anything we wanted to.
	# other typical options might be <Key-Tab> or <Key> (for anything)
	self.entrythingy.bind('<Key-Return>', self.print_contents)

    def upper(self):
	# notice here, we don't actually refer to the entry box.
	# we just operate on the string variable and we 
        # because it's being looked at by the entry widget, changing
	# the variable changes the entry widget display automatically.
	# the strange get/set operators are clunky, true...
	str = string.upper(self.contents.get())
	self.contents.set(str)

    def print_contents(self, event):
	print "hi. contents of entry is now ---->", self.contents.get()

root = App()
root.master.title("Foo")
root.mainloop()

