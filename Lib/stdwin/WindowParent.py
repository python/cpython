# A 'WindowParent' is the only module that uses real stdwin functionality.
# It is the root of the tree.
# It should have exactly one child when realized.
#
# There is also some support for a standard main loop here.

import stdwin
from stdwinevents import *

from TransParent import ManageOneChild

Error = 'WindowParent.Error'	# Exception

WindowList = []			# List containing all windows

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
		self.do_keybd = 0
		self.do_timer = 0
		self.do_altdraw = 0
		self.pending_destroy = 0
	#
	def destroy(self):
		if self.win in WindowList:
			WindowList.remove(self.win)
		if self.child: self.child.destroy()
		self._reset()
	#
	def delayed_destroy(self):
		# This interface to be used by 'Close' buttons etc.;
		# destroying a window from within a button hook
		# is not a good idea...
		self.pending_destroy = 1
	#
	def need_mouse(self, child): self.do_mouse = 1
	def no_mouse(self, child): self.do_mouse = 0
	#
	def need_keybd(self, child): self.do_keybd = 1
	def no_keybd(self, child): self.do_keybd = 0
	#
	def need_timer(self, child): self.do_timer = 1
	def no_timer(self, child): self.do_timer = 0
	#
	def need_altdraw(self, child): self.do_altdraw = 1
	def no_altdraw(self, child): self.do_altdraw = 0
	#
	def realize(self):
		if self.win:
			raise Error, 'realize(): called twice'
		if not self.child:
			raise Error, 'realize(): no child'
		size = self.child.minsize(self.beginmeasuring())
		self.size = max(self.size[0], size[0]), \
					max(self.size[1], size[1])
		stdwin.setdefscrollbars(0, 0)
		# XXX Compensate stdwin bug:
		# XXX should really be stdwin.setdefwinsize(self.size)
		stdwin.setdefwinsize(self.size[0]+4, self.size[1]+2)
		self.win = stdwin.open(self.title)
		self.win.setdocsize(self.size)
		if self.itimer:
			self.win.settimer(self.itimer)
		bounds = (0, 0), self.win.getwinsize()
		self.child.setbounds(bounds)
		self.child.realize()
		self.win.dispatch = self.dispatch
		WindowList.append(self.win)
	#
	def fixup(self):
		self.size = self.child.minsize(self.beginmeasuring())
		self.win.setdocsize(self.size)
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
	def getwindow(self):
		if self.win:
			return self.win
		else:
			raise Error, 'getwindow(): not realized yet'
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
	# Only call dispatch once we are realized
	#
	def dispatch(self, (type, win, detail)):
		if type = WE_DRAW:
			d = self.win.begindrawing()
			self.child.draw(d, detail)
			del d
			if self.do_altdraw: self.child.altdraw(detail)
		elif type = WE_MOUSE_DOWN:
			if self.do_mouse: self.child.mouse_down(detail)
		elif type = WE_MOUSE_MOVE:
			if self.do_mouse: self.child.mouse_move(detail)
		elif type = WE_MOUSE_UP:
			if self.do_mouse: self.child.mouse_up(detail)
		elif type in (WE_CHAR, WE_COMMAND):
			if self.do_keybd: self.child.keybd(type, detail)
		elif type = WE_TIMER:
			if self.do_timer: self.child.timer()
		elif type = WE_SIZE:
			self.fixup()
		elif type = WE_CLOSE:
			self.delayed_destroy()
		if self.pending_destroy:
			self.destroy()
	#

def MainLoop():
	while WindowList:
		Dispatch(stdwin.getevent())

def Dispatch(event):
	window = event[1]
	if window in WindowList:
		window.dispatch(event)
	else:
		stdwin.fleep()

# Interface used by WindowSched:

def CountWindows():
	return len(WindowList)

def AnyWindow():
	if not WindowList:
		return None
	else:
		return WindowList[0]
