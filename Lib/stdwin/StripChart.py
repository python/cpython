# Module 'StripChart'

import rect
from Buttons import LabelAppearance, NoReactivity

# A StripChart doesn't really look like a label but it needs a base class.
# LabelAppearance allows it to be disabled and hilited.

class StripChart() = LabelAppearance(), NoReactivity():
	#
	def define(self, (parent, scale)):
		self.parent = parent
		parent.addchild(self)
		self.init_appearance()
		self.init_reactivity()
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
			if self.bounds <> rect.empty:
				self.parent.scroll(self.bounds, (-excess, 0))
		if self.bounds <> rect.empty:
			(left, top), (right, bottom) = self.bounds
			i = len(self.ydata)
			area = (left+i-1, top), (left+i, bottom)
			self.draw(self.parent.begindrawing(), area)
	#
	def draw(self, (d, area)):
		area = rect.intersect(area, self.bounds)
		if area = rect.empty:
			print 'mt'
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
