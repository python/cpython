# A FormSplit lets you place its children exactly where you want them
# (including silly places!).
# It does no explicit geometry management except moving its children
# when it is moved.
# The interface to place children is as follows.
# Before you add a child, you may specify its (left, top) position
# relative to the FormSplit.  If you don't specify a position for
# a child, it goes right below the previous child; the first child
# goes to (0, 0) by default.
# NB: This places data attributes named form_* on its children.
# XXX Yes, I know, there should be options to do all sorts of relative
# placement, but for now this will do.

from Split import Split

class FormSplit() = Split():
	#
	def create(self, parent):
		self.next_left = self.next_top = 0
		self.last_child = None
		return Split.create(self, parent)
	#
	def getminsize(self, (m, sugg_size)):
		max_width, max_height = 0, 0
		for c in self.children:
			c.form_width, c.form_height = c.getminsize(m, (0, 0))
			max_width = max(max_width, c.form_width + c.form_left)
			max_height = max(max_height, \
					 c.form_height + c.form_top)
		return max_width, max_height
	#
	def getbounds(self):
		return self.bounds
	#
	def setbounds(self, bounds):
		self.bounds = bounds
		fleft, ftop = bounds[0]
		for c in self.children:
			left, top = c.form_left + fleft, c.form_top + ftop
			right, bottom = left + c.form_width, top + c.form_height
			c.setbounds((left, top), (right, bottom))
	#
	def placenext(self, (left, top)):
		self.next_left = left
		self.next_top = top
		self.last_child = None
	#
	def addchild(self, child):
		if self.last_child:
			width, height = \
			    self.last_child.getminsize(self.beginmeasuring(), \
			    			       (0, 0))
			self.next_top = self.next_top + height
		child.form_left = self.next_left
		child.form_top = self.next_top
		Split.addchild(self, child)
		self.last_child = child
	#
