# wdbframewin.py -- frame window for wdb.py

# XXX To do:
# - display function name in window title
# - execute arbitrary statements instead of just evaluating expressions
# - allow setting variables by editing their values


import stdwin
from stdwinevents import *
import basewin
import sys

WIDTH = 40
MINHEIGHT = 8
MAXHEIGHT = 16

class FrameWindow(basewin.BaseWindow):

	def init(self, debugger, frame, dict, name):
		self.debugger = debugger
		self.frame = frame # Not used except for identity tests
		self.dict = dict
		self.name = name
		nl = max(MINHEIGHT, len(self.dict) + 5)
		nl = min(nl, MAXHEIGHT)
		width = WIDTH*stdwin.textwidth('0')
		height = nl*stdwin.lineheight()
		stdwin.setdefwinsize(width, height)
		self = basewin.BaseWindow.init(self, '--Frame ' + name + '--')
		# XXX Should use current function name
		self.initeditor()
		self.displaylist = ['>>>', '', '-'*WIDTH]
		self.refreshframe()
		return self
	
	def initeditor(self):
		r = (stdwin.textwidth('>>> '), 0), (30000, stdwin.lineheight())
		self.editor = self.win.textcreate(r)
	
	def closeeditor(self):
		self.editor.close()
	
	def dispatch(self, event):
		type, win, detail = event
		if type == WE_NULL: return # Dummy tested by mainloop
		if type in (WE_DRAW, WE_COMMAND) \
				or not self.editor.event(event):
			basewin.BaseWindow.dispatch(self, event)
	
	def close(self):
		del self.debugger.framewindows[self.name]
		del self.debugger, self.dict
		self.closeeditor()
		basewin.BaseWindow.close(self)
	
	def command(self, detail):
		if detail == WC_RETURN:
			self.re_eval()
		else:
			dummy = self.editor.event(WE_COMMAND, \
						self.win, detail)
	
	def mouse_down(self, detail):
		(h, v), clicks, button, mask = detail
		i = v / stdwin.lineheight()
		if 5 <= i < len(self.displaylist):
			import string
			name = string.splitfields(self.displaylist[i],' = ')[0]
			if not self.dict.has_key(name):
				stdwin.fleep()
				return
			value = self.dict[name]
			if not hasattr(value, '__dict__'):
				stdwin.fleep()
				return
			name = 'instance ' + `value`
			if self.debugger.framewindows.has_key(name):
				self.debugger.framewindows[name].popup()
			else:
				self.debugger.framewindows[name] = \
					  FrameWindow().init(self.debugger,
						  self.frame, value.__dict__,
						  name)
			return
		stdwin.fleep()

	def re_eval(self):
		import string, repr
		expr = string.strip(self.editor.gettext())
		if expr == '':
			output = ''
		else:
			globals = self.frame.f_globals
			locals = self.dict
			try:
				value = eval(expr, globals, locals)
				output = repr.repr(value)
			except:
				output = sys.exc_type + ': ' + `sys.exc_value`
		self.displaylist[1] = output
		lh = stdwin.lineheight()
		r = (-10, 0), (30000, 2*lh)
		self.win.change(r)
		self.editor.setfocus(0, len(expr))
	
	def draw(self, detail):
		(left, top), (right, bottom) = detail
		dummy = self.editor.draw(detail)
		d = self.win.begindrawing()
		try:
			lh = d.lineheight()
			h, v = 0, 0
			for line in self.displaylist:
				if v+lh > top and v < bottom:
					d.text((h, v), line)
				v = v + lh
		finally:
			d.close()
	
	def refreshframe(self):
		import repr
		del self.displaylist[3:]
		self.re_eval()
		names = self.dict.keys()
		for key, label in ('__args__', 'Args: '), \
				  ('__return__', 'Return: '):
			if self.dict.has_key(key):
				names.remove(key)
				value = self.dict[key]
				label = label + repr.repr(value)
			self.displaylist.append(label)
		names.sort()
		for name in names:
			value = self.dict[name]
			line = name + ' = ' + repr.repr(value)
			self.displaylist.append(line)
		self.win.setdocsize(0, \
			stdwin.lineheight() * len(self.displaylist))
		self.refreshall() # XXX Be more subtle later
