# Module 'StripChart'


import rect
from Buttons import *
from Resize import *


class StripChart() = LabelAppearance(), NoReactivity(), NoResize():
	#
	def define(self, (win, bounds, scale)):
		self.init_appearance(win, bounds)
		self.init_reactivity()
		self.init_resize()
		self.ydata = []
		self.scale = scale
		self.resetbounds()
		return self
	#
	def setbounds(self, bounds):
		LabelAppearance.setbounds(self, bounds)
		self.resetbounds()
	#
	def resetbounds(self):
		(left, top), (right, bottom) = self.bounds
		self.width = right-left
		self.height = bottom-top
		excess = len(self.ydata) - self.width
		if excess > 0:
			del self.ydata[:excess]
		elif excess < 0:
			while len(self.ydata) < self.width:
				self.ydata.insert(0, 0)
	#
	def append(self, y):
		self.ydata.append(y)
		excess = len(self.ydata) - self.width
		if excess > 0:
			del self.ydata[:excess]
			if not self.limbo:
				self.win.scroll(self.bounds, (-excess, 0))
		if not self.limbo:
			(left, top), (right, bottom) = self.bounds
			i = len(self.ydata)
			area = (left+i-1, top), (left+i, bottom)
			self.draw(self.win.begindrawing(), area)
	#
	def draw(self, (d, area)):
		self.limbo = 0
		area = rect.intersect(area, self.bounds)
		if area = rect.empty:
			return
		d.cliprect(area)
		d.erase(self.bounds)
		(a_left, a_top), (a_right, a_bottom) = area
		(left, top), (right, bottom) = self.bounds
		height = bottom - top
		i1 = a_left - left
		i2 = a_right - left
		for i in range(max(0, i1), min(len(self.ydata), i2)):
			split = bottom-self.ydata[i]*height/self.scale
			d.paint((left+i, split), (left+i+1, bottom))
		if not self.enabled:
			self.flipenable(d)
		if self.hilited:
			self.fliphilite(d)
		d.noclip()
