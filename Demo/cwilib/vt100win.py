# VT100 terminal emulator in a STDWIN window.

import stdwin
from stdwinevents import *
from vt100 import VT100

class VT100win(VT100):

	def __init__(self):
		VT100.__init__(self)
		self.window = None
##		self.last_x = -1
##		self.last_y = -1

	def __del__(self):
		self.close()

	def open(self, title):
		stdwin.setfont('7x14')
		self.docwidth = self.width * stdwin.textwidth('m')
		self.docheight = self.height * stdwin.lineheight()
		stdwin.setdefwinsize(self.docwidth + 2, self.docheight + 2)
		stdwin.setdefscrollbars(0, 0)
		self.window = stdwin.open(title)
		self.window.setdocsize(self.docwidth + 2, self.docheight + 2)

	def close(self):
		if self.window:
			self.window.close()
		self.window = None

	def show(self):
		if not self.window: return
		self.draw(((-10, -10), (self.docwidth+10, self.docheight+10)))

	def draw(self, detail):
		d = self.window.begindrawing()
		fg = stdwin.getfgcolor()
		red = stdwin.fetchcolor('red')
		d.cliprect(detail)
		d.erase(detail)
		lh = d.lineheight()
		cw = d.textwidth('m')
		for y in range(self.height):
			d.text((0, y*lh), self.lines[y].tostring())
			if self.attrs[y] <> self.blankattr:
				for x in range(len(self.attrs[y])):
					if self.attrs[y][x] == 7:
						p1 = x*cw, y*lh
						p2 = (x+1)*cw, (y+1)*lh
						d.invert((p1, p2))
		x = self.x * cw
		y = self.y * lh
		d.setfgcolor(red)
		d.invert((x, y), (x+cw, y+lh))
		d.setfgcolor(fg)
		d.close()

##	def move_to(self, x, y):
##		VT100.move_to(self, x, y)
##		if self.y != self.last_y:
##			self.show()
##		self.last_x = self.x
##		self.last_y = y

	def send(self, str):
		VT100.send(self, str)
		self.show()
