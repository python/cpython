"""A simple but flexible modal dialog box."""


from Tkinter import *
import tktools


class SimpleDialog:

    def __init__(self, master,
		 text='', buttons=[], default=None, cancel=None,
		 title=None, class_=None):
	self.root = tktools.make_toplevel(master, title=title, class_=class_)
	self.message = Message(self.root, text=text, aspect=400)
	self.message.pack(expand=1, fill=BOTH)
	self.frame = Frame(self.root)
	self.frame.pack()
	self.num = default
	self.cancel = cancel
	self.default = default
	self.root.bind('<Return>', self.return_event)
	for num in range(len(buttons)):
	    s = buttons[num]
	    b = Button(self.frame, text=s,
		       command=(lambda self=self, num=num: self.done(num)))
	    if num == default:
		b.config(relief=RIDGE, borderwidth=8)
	    b.pack(side=LEFT, fill=BOTH, expand=1)
	self.root.protocol('WM_DELETE_WINDOW', self.wm_delete_window)
	tktools.set_transient(self.root, master)

    def go(self):
	self.root.grab_set()
	self.root.mainloop()
	self.root.destroy()
	return self.num

    def return_event(self, event):
	if self.default is None:
	    self.root.bell()
	else:
	    self.done(self.default)

    def wm_delete_window(self):
	if self.cancel is None:
	    self.root.bell()
	else:
	    self.done(self.cancel)

    def done(self, num):
	self.num = num
	self.root.quit()


def test():
    root = Tk()
    def doit(root=root):
	d = SimpleDialog(root,
			 text="This is a test dialog.  "
		              "Would this have been an actual dialog, "
			      "the buttons below would have glowed "
			      "in soft pink light. "
			      "Do you believe this?",
			 buttons=["Yes", "No", "Cancel"],
			 default=0,
			 cancel=2,
			 title="Test Dialog")
	print d.go()
    t = Button(root, text='Test', command=doit)
    t.pack()
    q = Button(root, text='Quit', command=t.quit)
    q.pack()
    t.mainloop()


if __name__ == '__main__':
    test()
