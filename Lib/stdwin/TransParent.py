# A class that sits transparently between a parent and one child.
# First create the parent, then this thing, then the child.
# Use this as a base class for objects that are almost transparent.
# Don't use as a base class for parents with multiple children.

Error = 'TransParent.Error'	# Exception

class ManageOneChild():
	#
	# Upcalls shared with other single-child parents
	#
	def addchild(self, child):
		if self.child:
			raise Error, 'addchild: one child only'
		if not child:
			raise Error, 'addchild: bad child'
		self.child = child
	#
	def delchild(self, child):
		if not self.child:
			raise Error, 'delchild: no child'
		if child <> self.child:
			raise Error, 'delchild: not my child'
		self.child = 0

class TransParent() = ManageOneChild():
	#
	# Calls from creator
	# NB derived classes may add parameters to create()
	#
	def create(self, parent):
		parent.addchild(self)
		self.parent = parent
		self.child = 0 # No child yet
	#
	# Downcalls from parent to child
	#
	def destroy(self):
		del self.parent
		if self.child: self.child.destroy()
		del self.child
	#
	def minsize(self, m):
		if not self.child:
			return 0, 0
		else:
			return self.child.minsize(m)
	def getbounds(self, bounds):
		if not self.child:
			raise Error, 'getbounds w/o child'
		else:
			return self.child.getbounds()
	def setbounds(self, bounds):
		if not self.child:
			raise Error, 'setbounds w/o child'
		else:
			self.child.setbounds(bounds)
	def draw(self, args):
		if self.child:
			self.child.draw(args)
	#
	# Downcalls only made after certain upcalls
	#
	def mouse_down(self, detail):
		if self.child: self.child.mouse_down(detail)
	def mouse_move(self, detail):
		if self.child: self.child.mouse_move(detail)
	def mouse_up(self, detail):
		if self.child: self.child.mouse_up(detail)
	#
	def timer(self):
		if self.child: self.child.timer()
	#
	# Upcalls from child to parent
	#
	def need_mouse(self, child):
		self.parent.need_mouse(self)
	def no_mouse(self, child):
		self.parent.no_mouse(self)
	#
	def need_timer(self, child):
		self.parent.need_timer(self)
	def no_timer(self, child):
		self.parent.no_timer(self)
	#
	def begindrawing(self):
		return self.parent.begindrawing()
	def beginmeasuring(self):
		return self.parent.beginmeasuring()
	#
	def change(self, area):
		self.parent.change(area)
	def scroll(self, args):
		self.parent.scroll(args)
	def settimer(self, itimer):
		self.parent.settimer(itimer)
