# Module 'Histogram'

from Buttons import *

# A Histogram displays a histogram of numeric data.
#
class HistogramAppearance() = LabelAppearance(), Define():
	#
	def define(self, parent):
		Define.define(self, (parent, ''))
		self.ydata = []
		self.scale = (0, 100)
		return self
	#
	def setdata(self, (ydata, scale)):
		self.ydata = ydata
		self.scale = scale # (min, max)
		self.parent.change(self.bounds)
	#
	def drawpict(self, d):
		(left, top), (right, bottom) = self.bounds
		min, max = self.scale
		size = max-min
		width, height = right-left, bottom-top
		ydata = self.ydata
		npoints = len(ydata)
		v1 = top + height	# constant
		h1 = left		# changed in loop
		for i in range(npoints):
			h0 = h1
			v0 = top + height - (ydata[i]-min)*height/size
			h1 = left + (i+1) * width/npoints
			d.paint((h0, v0), (h1, v1))
	#

class Histogram() = NoReactivity(), HistogramAppearance(): pass
