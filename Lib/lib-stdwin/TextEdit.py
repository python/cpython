# Text editing widget

# NB: this always assumes fixed bounds.
# For auto-growing TextEdit windows, different code would be needed.

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
		self.editor = None
		self.dh = self.dv = 0
		return self
	#
	def createboxed(self, (parent, (cols, rows), (dh, dv))):
		self = self.create(parent, (cols, rows))
		self.dh = max(0, dh)
		self.dv = max(0, dv)
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
	def getminsize(self, (m, (width, height))):
		width = max(0, width - 2*self.dh)
		height = max(0, height - 2*self.dv)
		if width > 0 and self.editor:
			(left, top), (right, bottom) = self.editor.getrect()
			act_width, act_height = right - left, bottom - top
			if width >= act_width:
				width = width + 2*self.dh
				height = max(height, act_height) + 2*self.dv
				return width, height
		width = max(width, self.cols*m.textwidth('in')/2) + 2*self.dh
		height = max(height, self.rows*m.lineheight()) + 2*self.dv
		return width, height
	#
	def setbounds(self, bounds):
		self.bounds = bounds
		if self.editor:
			(left, top), (right, bottom) = bounds
			left = left + self.dh
			top = top + self.dv
			right = right - self.dh
			bottom = bottom - self.dv
			self.editor.move((left, top), (right, bottom))
			if self.dh and self.dv:
				(left, top), (right, bottom) = bounds
				left = left + 1
				top = top + 1
				right = right - 1
				bottom = bottom - 1
				bounds = (left, top), (right, bottom)
			self.editor.setview(bounds)
	#
	def getbounds(self):
		return self.bounds
	#
	def realize(self):
		self.window = self.parent.getwindow()
		(left, top), (right, bottom) = self.bounds
		left = left + self.dh
		top = top + self.dv
		right = right - self.dh
		bottom = bottom - self.dv
		self.editor = \
			self.window.textcreate((left, top), (right, bottom))
		self.editor.setactive(0)
		bounds = self.bounds
		if self.dh and self.dv:
			(left, top), (right, bottom) = bounds
			left = left + 1
			top = top + 1
			right = right - 1
			bottom = bottom - 1
			bounds = (left, top), (right, bottom)
		self.editor.setview(bounds)
		self.editor.settext(self.text)
		self.parent.need_mouse(self)
		self.parent.need_keybd(self)
		self.parent.need_altdraw(self)
	#
	def draw(self, (d, area)):
		if self.dh and self.dv:
			d.box(self.bounds)
	#
	def altdraw(self, area):
		self.editor.draw(area)
	#
	# Event downcalls
	#
	def mouse_down(self, detail):
		x = self.editor.event(WE_MOUSE_DOWN, self.window, detail)
	#
	def mouse_move(self, detail):
		x = self.editor.event(WE_MOUSE_MOVE, self.window, detail)
	#
	def mouse_up(self, detail):
		x = self.editor.event(WE_MOUSE_UP, self.window, detail)
	#
	def keybd(self, (type, detail)):
		x = self.editor.event(type, self.window, detail)
	#
	def activate(self):
		self.editor.setfocus(0, 30000)
		self.editor.setactive(1)
	#
	def deactivate(self):
		self.editor.setactive(0)
	#
