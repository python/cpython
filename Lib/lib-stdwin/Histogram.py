# Module 'Histogram'

from Buttons import *
from Resize import Resize


# A Histogram displays a histogram of numeric data.
# It reacts to resize events by resizing itself,
# leaving the same amount of space around the borders.
# (This is geometry management, and should really be implemented
# by a different group of classes, but for now this hack is OK.)
#
class HistogramAppearance() = Resize(), LabelAppearance():
	#
	def define(self, (win, bounds, ydata, scale)):
		self.init_appearance(win, bounds)
		self.init_resize()
		self.ydata = ydata
		self.scale = scale # (min, max)
		return self
	#
	def setdata(self, (ydata, scale)):
		self.ydata = ydata
		self.scale = scale # (min, max)
		self.win.change(self.bounds)
	#
	def drawit(self, d):
		ydata = self.ydata
		(left, top), (right, bottom) = self.bounds
		min, max = self.scale
		size = max-min
		width, height = right-left, bottom-top
		for i in range(len(ydata)):
			h0 = left + i * width/len(ydata)
			h1 = left + (i+1) * width/len(ydata)
			v0 = top + height - (self.ydata[i]-min)*height/size
			v1 = top + height
			d.paint((h0, v0), (h1, v1))
	#

class Histogram() = HistogramAppearance(), NoReactivity(): pass
