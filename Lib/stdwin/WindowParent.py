# A 'WindowParent' is the only module that uses real stdwin functionality.
# It is the root of the tree.
# It should have exactly one child when realized.

import stdwin
from stdwinevents import *

from TransParent import ManageOneChild

Error = 'WindowParent.Error'	# Exception

class WindowParent() = ManageOneChild():
	#
	def create(self, (title, size)):
		self.title = title
		self.size = size		# (width, height)
		self._reset()
		return self
	#
	def _reset(self):
		self.child = 0
		self.win = 0
		self.itimer = 0
		self.do_mouse = 0
		self.do_timer = 0
	#
	def destroy(self):
		if self.child: self.child.destroy()
		self._reset()
	#
	def need_mouse(self, child): self.do_mouse = 1
	def no_mouse(self, child): self.do_mouse = 0
	#
	def need_timer(self, child): self.do_timer = 1
	def no_timer(self, child): self.do_timer = 0
	#
	def realize(self):
		if self.win:
			raise Error, 'realize(): called twice'
		if not self.child:
			raise Error, 'realize(): no child'
		size = self.child.minsize(self.beginmeasuring())
		self.size = max(self.size[0], size[0]), \
						max(self.size[1], size[1])
		#stdwin.setdefwinsize(self.size)
		# XXX Compensate stdwin bug:
		stdwin.setdefwinsize(self.size[0]+4, self.size[1]+2)
		self.win = stdwin.open(self.title)
		if self.itimer:
			self.win.settimer(self.itimer)
		bounds = (0, 0), self.win.getwinsize()
		self.child.setbounds(bounds)
	#
	def beginmeasuring(self):
		# Return something with which a child can measure text
		if self.win:
			return self.win.begindrawing()
		else:
			return stdwin
	#
	def begindrawing(self):
		if self.win:
			return self.win.begindrawing()
		else:
			raise Error, 'begindrawing(): not realized yet'
	#
	def change(self, area):
		if self.win:
			self.win.change(area)
	#
	def scroll(self, args):
		if self.win:
			self.win.scroll(args)
	#
	def settimer(self, itimer):
		if self.win:
			self.win.settimer(itimer)
		else:
			self.itimer = itimer
	#
	# Only call dispatch if we have a child
	#
	def dispatch(self, (type, win, detail)):
		if win <> self.win:
			return
		elif type = WE_DRAW:
			d = self.win.begindrawing()
			self.child.draw(d, detail)
		elif type = WE_MOUSE_DOWN:
			if self.do_mouse: self.child.mouse_down(detail)
		elif type = WE_MOUSE_MOVE:
			if self.do_mouse: self.child.mouse_move(detail)
		elif type = WE_MOUSE_UP:
			if self.do_mouse: self.child.mouse_up(detail)
		elif type = WE_TIMER:
			if self.do_timer: self.child.timer()
		elif type = WE_SIZE:
			self.win.change((0, 0), (10000, 10000)) # XXX
			bounds = (0, 0), self.win.getwinsize()
			self.child.setbounds(bounds)
	#
