# HVSplit contains generic code for HSplit and VSplit.
# HSplit and VSplit are specializations to either dimension.

# XXX This does not yet stretch/shrink children if there is too much
# XXX or too little space in the split dimension.
# XXX (NB There is no interface to ask children for stretch preferences.)

from Split import Split

class HVSplit() = Split():
	#
	def create(self, (parent, hv)):
		# hv is 0 or 1 for HSplit or VSplit
		self = Split.create(self, parent)
		self.hv = hv
		return self
	#
	def minsize(self, m):
		hv, vh = self.hv, 1 - self.hv
		size = [0, 0]
		for c in self.children:
			csize = c.minsize(m)
			if csize[vh] > size[vh]: size[vh] = csize[vh]
			size[hv] = size[hv] + csize[hv]
		return size[0], size[1]
	#
	def getbounds(self):
		return self.bounds
	#
	def setbounds(self, bounds):
		self.bounds = bounds
		hv, vh = self.hv, 1 - self.hv
		mf = self.parent.beginmeasuring
		size = self.minsize(mf())
		# XXX not yet used!  Later for stretching
		maxsize_hv = bounds[1][hv] - bounds[0][hv]
		origin = [self.bounds[0][0], self.bounds[0][1]]
		for c in self.children:
			size = c.minsize(mf())
			corner = [0, 0]
			corner[vh] = bounds[1][vh]
			corner[hv] = origin[hv] + size[hv]
			c.setbounds((origin[0], origin[1]), \
					(corner[0], corner[1]))
			origin[hv] = corner[hv]
			# XXX stretch
			# XXX too-small
	#

class HSplit() = HVSplit():
	def create(self, parent):
		return HVSplit.create(self, (parent, 0))

class VSplit() = HVSplit():
	def create(self, parent):
		return HVSplit.create(self, (parent, 1))
