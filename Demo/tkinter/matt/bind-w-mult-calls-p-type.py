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

	# Note that here is where we bind a completely different callback to 
	# the same event. We pass "+" here to indicate that we wish to ADD 
	# this callback to the list associated with this event type.
	# Not specifying "+" would simply override whatever callback was
	# defined on this event.
	self.entrythingy.bind('<Key-Return>', self.print_something_else, "+")

    def print_contents(self, event):
	print "hi. contents of entry is now ---->", self.entrythingy.get()


    def print_something_else(self, event):
	print "hi. Now doing something completely different"


root = App()
root.master.title("Foo")
root.mainloop()



# secret tip for experts: if you pass *any* non-false value as 
# the third parameter to bind(), Tkinter.py will accumulate 
# callbacks instead of overwriting. I use "+" here because that's
# the Tk notation for getting this sort of behavior. The perfect GUI 
# interface would use a less obscure notation.
