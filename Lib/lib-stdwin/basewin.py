# basewin.py

import stdwin
import mainloop
from stdwinevents import *

class BaseWindow:
	
	def __init__(self, title):
		self.win = stdwin.open(title)
		self.win.dispatch = self.dispatch
		mainloop.register(self.win)
	
#	def reopen(self):
#		title = self.win.gettitle()
#		winpos = self.win.getwinpos()
#		winsize = self.win.getwinsize()
#		origin = self.win.getorigin()
#		docsize = self.win.getdocsize()
#		mainloop.unregister(self.win)
#		del self.win.dispatch
#		self.win.close()
#		stdwin.setdefwinpos(winpos)
#		stdwin.setdefwinsize(winsize)
#		self.win = stdwin.open(title)
#		stdwin.setdefwinpos(0, 0)
#		stdwin.setdefwinsize(0, 0)
#		self.win.setdocsize(docsize)
#		self.win.setorigin(origin)
#		self.win.dispatch = self.dispatch
#		mainloop.register(self.win)
	
	def popup(self):
		if self.win is not stdwin.getactive():
			self.win.setactive()
	
	def close(self):
		mainloop.unregister(self.win)
		del self.win.dispatch
		self.win.close()
	
	def dispatch(self, event):
		type, win, detail = event
		if type == WE_CHAR:
			self.char(detail)
		elif type == WE_COMMAND:
			self.command(detail)
		elif type == WE_MOUSE_DOWN:
			self.mouse_down(detail)
		elif type == WE_MOUSE_MOVE:
			self.mouse_move(detail)
		elif type == WE_MOUSE_UP:
			self.mouse_up(detail)
		elif type == WE_DRAW:
			self.draw(detail)
		elif type == WE_CLOSE:
			self.close()
	
	def no_op(self, detail):
		pass
	char = command = mouse_down = mouse_move = mouse_up = draw = no_op
	
	def refreshall(self):
		self.win.change((-10, 0), (10000, 30000))
