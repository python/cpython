# A CSplit is a Clock-shaped split: the children are grouped in a circle.
# The numbering is a little different from a real clock: the 12 o'clock
# position is called 0, not 12.  This is a little easier since Python
# usually counts from zero.  (BTW, there needn't be exactly 12 children.)


from math import pi, sin, cos
from Split import Split

class CSplit() = Split():
	#
	def minsize(self, m):
		# Since things look best if the children are spaced evenly
		# along the circle (and often all children have the same
		# size anyway) we compute the max child size and assume
		# this is each child's size.
		width, height = 0, 0
		for child in self.children:
			wi, he = child.minsize(m)
			width = max(width, wi)
			height = max(height, he)
		# In approximation, the diameter of the circle we need is
		# (diameter of box) * (#children) / pi.
		# We approximate pi by 3 (so we slightly overestimate
		# our minimal size requirements -- not so bad).
		# Because the boxes stick out of the circle we add the
		# box size to each dimension.
		# Because we really deal with ellipses, do everything
		# separate in each dimension.
		n = len(self.children)
		return width + (width*n + 2)/3, height + (height*n + 2)/3
	#
	def getbounds(self):
		return self.bounds
	#
	def setbounds(self, bounds):
		self.bounds = bounds
		# Place the children.  This involves some math.
		# Compute center positions for children as if they were
		# ellipses with a diameter about 1/N times the
		# circumference of the big ellipse.
		# (There is some rounding involved to make it look
		# reasonable for small and large N alike.)
		# XXX One day Python will have automatic conversions...
		n = len(self.children)
		fn = float(n)
		if n = 0: return
		(left, top), (right, bottom) = bounds
		width, height = right-left, bottom-top
		child_width, child_height = width*3/(n+4), height*3/(n+4)
		half_width, half_height = \
			float(width-child_width)/2.0, \
			float(height-child_height)/2.0
		center_h, center_v = center = (left+right)/2, (top+bottom)/2
		fch, fcv = float(center_h), float(center_v)
		alpha = 2.0 * pi / fn
		for i in range(n):
			child = self.children[i]
			fi = float(i)
			fh, fv = \
				fch + half_width*sin(fi*alpha), \
				fcv - half_height*cos(fi*alpha)
			left, top = \
				int(fh) - child_width/2, \
				int(fv) - child_height/2
			right, bottom = \
				left + child_width, \
				top + child_height
			child.setbounds((left, top), (right, bottom))
	#
