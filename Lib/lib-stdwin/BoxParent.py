from TransParent import TransParent

class BoxParent(TransParent):
	#
	def create(self, (parent, (dh, dv))):
		self = TransParent.create(self, parent)
		self.dh = dh
		self.dv = dv
		return self
	#
	def getminsize(self, (m, (width, height))):
		width = max(0, width - 2*self.dh)
		height = max(0, height - 2*self.dv)
		width, height = self.child.getminsize(m, (width, height))
		return width + 2*self.dh, height + 2*self.dv
	#
	def setbounds(self, bounds):
		(left, top), (right, bottom) = bounds
		self.bounds = bounds
		left = min(right, left + self.dh)
		top = min(bottom, top + self.dv)
		right = max(left, right - self.dh)
		bottom = max(top, bottom - self.dv)
		self.innerbounds = (left, top), (right, bottom)
		self.child.setbounds(self.innerbounds)
	#
	def getbounds(self):
		return self.bounds
	#
	def draw(self, args):
		d, area = args
		(left, top), (right, bottom) = self.bounds
		left = left + 1
		top = top + 1
		right = right - 1
		bottom = bottom - 1
		d.box((left, top), (right, bottom))
		TransParent.draw(self, args) # XXX clip to innerbounds?
	#
	# XXX should scroll clip to innerbounds???
	# XXX currently the only user restricts itself to child's bounds
