# Text editing widget

from stdwinevents import *

class TextEdit():
	#
	def create(self, (parent, (cols, rows))):
		parent.addchild(self)
		self.parent = parent
		self.cols = cols
		self.rows = rows
		self.text = ''
		# Creation of the editor is done in realize()
		self.editor = 0
		return self
	#
	def settext(self, text):
		self.editor.settext(text)
	#
	# Downcalls from parent to child
	#
	def destroy(self):
		del self.parent
		del self.editor
		del self.window
	#
	def minsize(self, m):
		return self.cols*m.textwidth('in')/2, self.rows*m.lineheight()
	def setbounds(self, bounds):
		self.bounds = bounds
		if self.editor:
			self.editor.move(bounds)
	def getbounds(self, bounds):
		if self.editor:
			return self.editor.getrect()
		else:
			return self.bounds
	def realize(self):
		self.window = self.parent.getwindow()
		self.editor = self.window.textcreate(self.bounds)
		self.editor.settext(self.text)
		self.parent.need_mouse(self)
		self.parent.need_keybd(self)
		self.parent.need_altdraw(self)
	def draw(self, (d, area)):
		pass
	def altdraw(self, area):
		self.editor.draw(area)
	#
	# Event downcalls
	#
	def mouse_down(self, detail):
		x = self.editor.event(WE_MOUSE_DOWN, self.window, detail)
	def mouse_move(self, detail):
		x = self.editor.event(WE_MOUSE_MOVE, self.window, detail)
	def mouse_up(self, detail):
		x = self.editor.event(WE_MOUSE_UP, self.window, detail)
	#
	def keybd(self, (type, detail)):
		x = self.editor.event(type, self.window, detail)
	#
