# Generic Split implementation.
# Use as a base class for other splits.
# Derived classes should at least implement the methods that call
# unimpl() below: minsize(), getbounds() and setbounds().

Error = 'Split.Error'	# Exception

import rect
from util import remove

class Split():
	#
	# Calls from creator
	# NB derived classes may add parameters to create()
	#
	def create(self, parent):
		parent.addchild(self)
		self.parent = parent
		self.children = []
		self.mouse_interest = []
		self.timer_interest = []
		self.mouse_focus = 0
		return self
	#
	# Downcalls from parent to child
	#
	def destroy(self):
		self.parent = None
		for child in self.children:
			child.destroy()
		del self.children[:]
		del self.mouse_interest[:]
		del self.timer_interest[:]
		self.mouse_focus = None
	#
	def minsize(self, m): return unimpl()
	def getbounds(self): return unimpl()
	def setbounds(self, bounds): unimpl()
	#
	def draw(self, d_detail):
		# (Could avoid calls to children outside the area)
		for child in self.children:
			child.draw(d_detail)
	#
	# Downcalls only made after certain upcalls
	#
	def mouse_down(self, detail):
		if self.mouse_focus:
			self.mouse_focus.mouse_down(detail)
		p = detail[0]
		for child in self.mouse_interest:
			if rect.pointinrect(p, child.getbounds()):
				self.mouse_focus = child
				child.mouse_down(detail)
	def mouse_move(self, detail):
		if self.mouse_focus:
			self.mouse_focus.mouse_move(detail)
	def mouse_up(self, detail):
		if self.mouse_focus:
			self.mouse_focus.mouse_up(detail)
			self.mouse_focus = 0
	#
	def timer(self):
		for child in self.timer_interest:
			child.timer()
	#
	# Upcalls from child to parent
	#
	def addchild(self, child):
		if child in self.children:
			raise Error, 'addchild: child already inlist'
		self.children.append(child)
	def delchild(self, child):
		if child not in self.children:
			raise Error, 'delchild: child not in list'
		remove(child, self.children)
		if child in self.mouse_interest:
			remove(child, self.mouse_interest)
		if child in self.timer_interest:
			remove(child, self.timer_interest)
		if child = self.mouse_focus:
			self.mouse_focus = 0
	#
	def need_mouse(self, child):
		if child not in self.mouse_interest:
			self.mouse_interest.append(child)
			self.parent.need_mouse(self)
	def no_mouse(self, child):
		if child in self.mouse_interest:
			remove(child, self.mouse_interest)
			if not self.mouse_interest:
				self.parent.no_mouse(self)
	#
	def need_timer(self, child):
		if child not in self.timer_interest:
			self.timer_interest.append(child)
			self.parent.need_timer(self)
	def no_timer(self, child):
		if child in self.timer_interest:
			remove(child, self.timer_interest)
			if not self.timer_interest:
				self.parent.no_timer(self)
	#
	# The rest are transparent:
	#
	def begindrawing(self):
		return self.parent.begindrawing()
	def beginmeasuring(self):
		return self.parent.beginmeasuring()
	#
	def change(self, area):
		self.parent.change(area)
	def scroll(self, area_vector):
		self.parent.scroll(area_vector)
	def settimer(self, itimer):
		self.parent.settimer(itimer)
