# tst.py
from Tkinter import *
import sys

def do_hello():
	print 'Hello world!'

class Quit(Button):
	def __init__(self, master=None, cnf={}):
		Button.__init__(self, master, 
				({'name': 'quit',
				  'text': 'Quit', 
				  'command': self.quit},
				 cnf))

class Stuff(Canvas):
	def enter(self, e):
		print 'Enter'
		self.itemconfig('current', {'fill': 'red'})
	def leave(self, e):
		print 'Leave'
		self.itemconfig('current', {'fill': 'blue'})
	def __init__(self, master=None, cnf={}):
		Canvas.__init__(self, master, 
				{'width': 100, 'height': 100})
		Canvas.config(self, cnf)
		self.create_rectangle(30, 30, 70, 70, 
				      {'fill': 'blue', 'tags': 'box'})
		Canvas.bind(self, 'box', '<Enter>', self.enter)
		Canvas.bind(self, 'box', '<Leave>', self.leave)

class Test(Frame):
	text = 'Testing'
	num = 1
	def do_xy(self, e):
		print (e.x, e.y)
	def do_test(self):
		if not self.num % 10:
			self.text = 'Testing 1 ...'
		self.text = self.text + ' ' + `self.num`
		self.num = self.num + 1
		self.testing['text'] = self.text
	def do_err(self):
		1/0
	def do_after(self):
		self.testing.invoke()
		self.after(10000, self.do_after)
	def __init__(self, master=None):
		Frame.__init__(self, master)
		self['bd'] = 30
		Pack.config(self)
		self.bind('<Motion>', self.do_xy)
		self.hello = Button(self, {'name': 'hello', 
					   'text': 'Hello', 
					   'command': do_hello,
					   Pack: {'fill': 'both'}})
		self.testing = Button(self)
		self.testing['text'] = self.text
		self.testing['command'] = self.do_test
		Pack.config(self.testing, {'fill': 'both'})
		self.err = Button(self, {'text': 'Error', 
					 'command': self.do_err,
					 Pack: {'fill': 'both'}})
		self.quit = Quit(self, {Pack: {'fill': 'both'}})
		self.exit = Button(self, 
				   {'text': 'Exit', 
				    'command': lambda: sys.exit(0),
				    Pack: {'fill': 'both'}})
		self.stuff = Stuff(self, {Pack: {'padx': 2, 'pady': 2}})
		self.do_after()

test = Test()
test.master.title('Tkinter Test')
test.master.iconname('Test')
test.master.maxsize(500, 500)
test.testing.invoke()

# Use the -i option and type ^C to get a prompt
mainloop()

