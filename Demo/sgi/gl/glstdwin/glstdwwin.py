# Define window operations for STDWIN

import gl
from stdwinevents import *
from glstdwin import G			# Global variables
from glstdwin import MASK		# Tunable constant

class WindowObject:
	#
	def _init(self, title):
		self._docsize = (0, 0)
		self._fg = G.fg
		self._bg = G.bg
		self._title = title
		self._font = G.font
		self._size = G.size
		self._menus = []
		self._gid = gl.winopen(title)
		gl.winconstraints() # To remove prefsize() effect
		self._fixviewport()
		self._needredraw()
		return self
	#
	def close(self):
		del G.windowmap[`self._gid`]
		gl.winclose(self._gid)
		self._gid = 0
	#
	def _needredraw(self):
		if self in G.drawqueue:
			G.drawqueue.remove(self)
		G.drawqueue.append(self)
	#
	def begindrawing(self):
		from glstdwdraw import DrawingObject
		return DrawingObject()._init(self)
	#
	def change(self, area):
		self._needredraw()
		# XXX Should record the area to be drawn?
	#
	def gettitle(self):
		return self._title
	#
	def getdocsize(self):
		return self._docsize
	#
	def getorigin(self):
		return self._area[0]
	#
	def getwinsize(self):
		return self._area[1]
	#
	def scroll(self, (area, by)):
		# XXX ought to use gl.rectcopy()
		if by <> (0, 0):
			self.change(area)
	#
	def setdocsize(self, docsize):
		self._docsize = docsize
	#
	def setorigin(self, origin):
		pass # XXX
	#
	def settimer(self, decisecs):
		pass # XXX
	#
	def settitle(self, title):
		self._title = title
		gl.wintitle(title)
	#
	def show(self, area):
		pass # XXX
	#
	def _fixviewport(self):
		#
		# Called after redraw or resize, and initially.
		#
		# Fix the coordinate system so that (0, 0) is top left,
		# units are pixels, and positive axes point right and down.
		#
		# Make the viewport slightly larger than the window,
		# and set the screenmask exactly to the window; this
		# help fixing character clipping.
		#
		# Set self._area to the window rectangle in STDWIN coords.
		#
		gl.winset(self._gid)
		gl.reshapeviewport()
		x0, x1, y0, y1 = gl.getviewport()
		width, height = x1-x0, y1-y0
		gl.viewport(x0-MASK, x1+MASK, y0-MASK, y1+MASK)
		gl.scrmask(x0, x1, y0, y1)
		gl.ortho2(-MASK, width+MASK, height+MASK, -MASK)
		self._area = (0, 0), (width, height)
	#
	def menucreate(self, title):
		from glstdwmenu import MenuObject
		menu = MenuObject()._init(self, title)
		self._menus.append(menu)
		return menu
	#
	def _domenu(self):
		if not self._menus:
			return None
		if len(self._menus) == 1:
			pup = self._menus[0]._makepup(0)
			val = gl.dopup(pup)
			gl.freepup(pup)
			if val < 0:
				return None
			return WE_MENU, self, (self._menus[0], val)
		#
		# More than one menu: use nested menus.
		#
		pups = []
		firstitem = 0
		for menu in self._menus:
			pups.append(menu._makepup(firstitem))
			firstitem = firstitem + 100
		pup = gl.newpup()
		for i in range(len(self._menus)):
			gl.addtopup(pup, self._menus[i]._title + '%m', pups[i])
		val = gl.dopup(pup)
		gl.freepup(pup)
		for pup in pups:
			gl.freepup(pup)
		if val < 0:
			return None
		i_menu, i_item = divmod(val, 100)
		return WE_MENU, self, (self._menus[i_menu], i_item)
	#
	def _doshortcut(self, char):
		for menu in self._menus:
			i = menu._checkshortcut(char)
			if i >= 0:
				return WE_MENU, self, (menu, i)
		return None
	#
