# A 'WindowParent' is the only module that uses real stdwin functionality.
# It is the root of the tree.
# It should have exactly one child when realized.
#
# There is also an alternative interface to "mainloop" here.

import stdwin
from stdwinevents import *
import mainloop

from TransParent import ManageOneChild

Error = 'WindowParent.Error'	# Exception

class WindowParent(ManageOneChild):
	#
	def create(self, (title, size)):
		self.title = title
		self.size = size		# (width, height)
		self._reset()
		self.close_hook = WindowParent.delayed_destroy
		return self
	#
	def _reset(self):
		self.child = None
		self.win = None
		self.itimer = 0
		self.do_mouse = 0
		self.do_keybd = 0
		self.do_timer = 0
		self.do_altdraw = 0
		self.pending_destroy = 0
		self.close_hook = None
		self.menu_hook = None
	#
	def destroy(self):
		mainloop.unregister(self.win)
		if self.child: self.child.destroy()
		self._reset()
	#
	def delayed_destroy(self):
		# This interface to be used by 'Close' buttons etc.;
		# destroying a window from within a button hook
		# is not a good idea...
		self.pending_destroy = 1
	#
	def close_trigger(self):
		if self.close_hook: self.close_hook(self)
	#
	def menu_trigger(self, (menu, item)):
		if self.menu_hook:
			self.menu_hook(self, menu, item)
	#
	def need_mouse(self, child): self.do_mouse = 1
	def no_mouse(self, child): self.do_mouse = 0
	#
	def need_keybd(self, child):
		self.do_keybd = 1
		self.child.activate()
	def no_keybd(self, child):
		self.do_keybd = 0
		self.child.deactivate()
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
		# Compute suggested size
		self.size = self.child.getminsize(self.beginmeasuring(), \
						  self.size)
		save_defsize = stdwin.getdefwinsize()
		scrwidth, scrheight = stdwin.getscrsize()
		width, height = self.size
		if width > scrwidth:
			width = scrwidth * 2/3
		if height > scrheight:
			height = scrheight * 2/3
		stdwin.setdefwinsize(width, height)
		self.hbar, self.vbar = stdwin.getdefscrollbars()
		self.win = stdwin.open(self.title)
		stdwin.setdefwinsize(save_defsize)
		self.win.setdocsize(self.size)
		if self.itimer:
			self.win.settimer(self.itimer)
		width, height = self.win.getwinsize()
		if self.hbar:
			width = self.size[0]
		if self.vbar:
			height = self.size[1]
		self.child.setbounds((0, 0), (width, height))
		self.child.realize()
		self.win.dispatch = self.dispatch
		mainloop.register(self.win)
	#
	def fixup(self):
		# XXX This could share code with realize() above
		self.size = self.child.getminsize(self.beginmeasuring(), \
					          self.win.getwinsize())
		self.win.setdocsize(self.size)
		width, height = self.win.getwinsize()
		if self.hbar:
			width = self.size[0]
		if self.vbar:
			height = self.size[1]
		self.child.setbounds((0, 0), (width, height))
		# Force a redraw of the entire window:
		self.win.change((0, 0), self.size)
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
		if type == WE_DRAW:
			d = self.win.begindrawing()
			self.child.draw(d, detail)
			del d
			if self.do_altdraw: self.child.altdraw(detail)
		elif type == WE_MOUSE_DOWN:
			if self.do_mouse: self.child.mouse_down(detail)
		elif type == WE_MOUSE_MOVE:
			if self.do_mouse: self.child.mouse_move(detail)
		elif type == WE_MOUSE_UP:
			if self.do_mouse: self.child.mouse_up(detail)
		elif type in (WE_CHAR, WE_COMMAND):
			if self.do_keybd: self.child.keybd(type, detail)
		elif type == WE_TIMER:
			if self.do_timer: self.child.timer()
		elif type == WE_SIZE:
			self.fixup()
		elif type == WE_CLOSE:
			self.close_trigger()
		elif type == WE_MENU:
			self.menu_trigger(detail)
		if self.pending_destroy:
			self.destroy()
	#

def MainLoop():
	mainloop.mainloop()

def Dispatch(event):
	mainloop.dispatch(event)

# Interface used by WindowSched:

def CountWindows():
	return mainloop.countwindows()

def AnyWindow():
	return mainloop.anywindow()
