# HVSplit contains generic code for HSplit and VSplit.
# HSplit and VSplit are specializations to either dimension.

# XXX This does not yet stretch/shrink children if there is too much
# XXX or too little space in the split dimension.
# XXX (NB There is no interface to ask children for stretch preferences.)

from Split import Split

class HVSplit(Split):
	#
	def create(self, parent, hv):
		# hv is 0 for HSplit, 1 for VSplit
		self = Split.create(self, parent)
		self.hv = hv
		return self
	#
	def getminsize(self, m, sugg_size):
		hv, vh = self.hv, 1 - self.hv
		size = [0, 0]
		sugg_size = [sugg_size[0], sugg_size[1]]
		sugg_size[hv] = 0
		sugg_size = sugg_size[0], sugg_size[1] # Make a tuple
		for c in self.children:
			csize = c.getminsize(m, sugg_size)
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
		begin, end = bounds
		sugg_size = end[0] - begin[0], end[1] - begin[1]
		size = self.getminsize(mf(), sugg_size)
		origin = [begin[0], begin[1]]
		sugg_size = [sugg_size[0], sugg_size[1]] # Make a list
		sugg_size[hv] = 0
		sugg_size = sugg_size[0], sugg_size[1] # Make a tuple
		for c in self.children:
			size = c.getminsize(mf(), sugg_size)
			corner = [0, 0]
			corner[vh] = end[vh]
			corner[hv] = origin[hv] + size[hv]
			c.setbounds(((origin[0], origin[1]), \
					(corner[0], corner[1])))
			origin[hv] = corner[hv]
			# XXX stretch
			# XXX too-small
	#

class HSplit(HVSplit):
	def create(self, parent):
		return HVSplit.create(self, parent, 0)

class VSplit(HVSplit):
	def create(self, parent):
		return HVSplit.create(self, parent, 1)
