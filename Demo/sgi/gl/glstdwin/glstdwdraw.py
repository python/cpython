# Define drawing operations for GL stdwin

import gl
import fm
from GL import LO_XOR, LO_SRC
from glstdwin import MASK

class DrawingObject:
	#
	def _init(self, win):
		self.fg = win._fg
		self.bg = win._bg
		self.font = win._font
		self.size = win._size
		self.width, self.height = win._area[1]
		gl.winset(win._gid)
		gl.color(self.fg)
		return self
	#
	def setfont(self, fontname):
		self.font = fm.findfont(fontname).scalefont(self.size)
	#
	def setsize(self, size):
		ratio = float(size) / float(self.size)
		self.size = size
		self.font = self.font.scalefont(ratio)
	#
	def setfgcolor(self, color):
		self.fg = color
		gl.color(self.fg)
	#
	def setbgcolor(self, color):
		self.bg = color
	#
	def cliprect(self, area):
		#print 'cliprect', area
		(left, top), (right, bottom) = area
		gl.scrmask(left, right, self.height-bottom, self.height-top)
	#
	def noclip(self):
		#print 'noclip()'
		gl.scrmask(0, self.width, 0, self.height)
	#
	def paint(self, ((left, top), (right, bottom))):
		gl.rectf(left, top, right, bottom)
	#
	def box(self, ((left, top), (right, bottom))):
		#print 'box', ((left, top), (right, bottom))
		gl.rect(left, top, right, bottom)
	#
	def circle(self, ((h, v), radius)):
		gl.circ(h, v, radius)
	#
	def elarc(self, (center, (rh, rv), a1, a2)):
		pass # XXX
	#
	def erase(self, ((left, top), (right, bottom))):
		#print 'erase', ((left, top), (right, bottom))
		gl.color(self.bg)
		gl.rectf(left, top, right, bottom)
		gl.color(self.fg)
	#
	def invert(self, ((left, top), (right, bottom))):
		#print 'invert', ((h0, v0), (h1, v1))
		gl.logicop(LO_XOR)
		gl.color(self.bg)
		gl.rectf(left, top, right, bottom)
		gl.color(self.fg)
		gl.logicop(LO_SRC)
	#
	def line(self, ((h0, v0), (h1, v1))):
		#print 'line', ((h0, v0), (h1, v1))
		gl.bgnline()
		gl.v2i(h0, v0)
		gl.v2i(h1, v1)
		gl.endline()
	#
	def xorline(self, ((h0, v0), (h1, v1))):
		#print 'xorline', ((h0, v0), (h1, v1))
		gl.logicop(LO_XOR)
		gl.color(self.bg)
		gl.bgnline()
		gl.v2i(h0, v0)
		gl.v2i(h1, v1)
		gl.endline()
		gl.color(self.fg)
		gl.logicop(LO_SRC)
	#
	def point(self, (h, v)):
		#print 'point', (h, v)
		gl.bgnpoint()
		gl.v2i(h, v)
		gl.endpoint()
	#
	def text(self, ((h, v), string)):
		#print 'text', ((h, v), string)
		if h < 0:
			# If the point is outside the window
			# the whole string isn't drawn.
			# Skip the beginning of the string.
			# XXX What if the font is bigger than 20 pixels?
			i, n = 0, len(string)
			while h < -MASK and i < n:
				h = h + self.font.getstrwidth(string[i])
				i = i + 1
			string = string[i:]
		gl.cmov2(h, v + self.baseline())
		self.font.setfont()
		fm.prstr(string)
	#
	def shade(self, ((h, v), percent)):
		pass # XXX
	#
	def baseline(self):
		(printermatched, fixed_width, xorig, yorig, xsize, ysize, \
			height, nglyphs) = self.font.getfontinfo()
		return height - yorig
	#
	def lineheight(self):
		(printermatched, fixed_width, xorig, yorig, xsize, ysize, \
			height, nglyphs) = self.font.getfontinfo()
		return height
	#
	def textbreak(self, (string, width)):
		# XXX Slooooow!
		n = len(string)
		nwidth = self.textwidth(string[:n])
		while nwidth > width:
			n = n-1
			nwidth = self.textwidth(string[:n])
		return n
	#
	def textwidth(self, string):
		return self.font.getstrwidth(string)
	#
