# Abstract classes for parents and children.
#
# Do not use as base class -- this is for documentation only.
#
# Note that the tree must be built top down (create the parent,
# then add the children).
#
# Also note that the creation methods are not standardized --
# these have extra parameters dependent on the widget type.
# For historical reasons, button creation methods are called
# define() while split creation methods are called create().

class AbstractParent():
	#
	# Upcalls from child to parent
	#
	def addchild(self, child): unimpl()
	def delchild(self, child): unimpl()
	#
	def need_mouse(self, child): unimpl()
	def no_mouse(self, child): unimpl()
	#
	def need_timer(self, child): unimpl()
	def no_timer(self, child): unimpl()
	#
	# XXX need_kbd, no_kbd; focus???
	#
	def begindrawing(self): return unimpl()
	def beginmeasuring(self): return unimpl()
	#
	def change(self, area): unimpl()
	def scroll(self, (area, (dh, dv))): unimpl()
	def settimer(self, itimer): unimpl()

class AbstractChild():
	#
	# Downcalls from parent to child
	#
	def destroy(self): unimpl()
	#
	def minsize(self, m): return unimpl()
	def getbounds(self): return unimpl()
	def setbounds(self, bounds): unimpl()
	def draw(self, (d, area)): unimpl()
	#
	# Downcalls only made after certain upcalls
	#
	def mouse_down(self, detail): unimpl()
	def mouse_move(self, detail): unimpl()
	def mouse_up(self, detail): unimpl()
	#
	def timer(self): unimpl()

# A "Split" is a child that manages one or more children.
# (This terminology is due to DEC SRC, except for CSplits.)
# A child of a split may be another split, a button, a slider, etc.
# Certain upcalls and downcalls can be handled transparently, but
# for others (e.g., all geometry related calls) this is not possible.

class AbstractSplit() = AbstractChild(), AbstractParent():
	pass
