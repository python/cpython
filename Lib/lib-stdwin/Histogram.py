# Module 'Histogram'

from Buttons import *


# A Histogram displays a histogram of numeric data.
# It reacts to resize events by resizing itself,
# leaving the same amount of space around the borders.
#
class HistogramAppearance() = LabelAppearance():
	#
	def define(self, (win, bounds, ydata, scale)):
		self.init_appearance(win, bounds)
		self.ydata = ydata
		self.scale = scale # (min, max)
		self.left_top, (right, bottom) = bounds
		width, height = win.getwinsize()
		self.right_margin = width - right
		self.bottom_margin = height - bottom
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
	def resize(self):
		width, height = self.win.getwinsize()
		right = width - self.right_margin
		bottom = height - self.bottom_margin
		self.setbounds(self.left_top, (right, bottom))
	#

class HistogramReactivity() = NoReactivity(): pass

class Histogram() = HistogramAppearance(), HistogramReactivity(): pass
