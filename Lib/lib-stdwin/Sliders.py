# Module 'Sliders'
#
# Sliders are somewhat like buttons but have an extra hook that is
# called whenever their value is changed.

import stdwin
from stdwinevents import *
import rect
from minmax import min, max
from Buttons import ClassicButton


# Field indices in event detail
#
_HV = 0
_CLICKS = 1
_BUTTON = 2
_MASK = 3


# A dragslider is the simplest possible slider.
# It looks like a button but dragging the mouse left or right
# changes the controlled value.
#
class DragSlider() = ClassicButton():
	#
	# INVARIANTS maintained by the define and setval methods:
	#
	#	self.min <= self.val <= self.max
	#	self.text = `self.val`
	#
	# (Notice that unlike in Python ranges, the end point belongs
	# to the range.)
	#
	def define(self, (win, bounds)):
		self.min = 0
		self.val = 50
		self.max = 100
		self.setval_hook = 0
		self.pretext = self.postext = ''
		self.text = self.pretext + `self.val` + self.postext
		self = ClassicButton.define(self, (win, bounds, self.text))
		return self
	#
	def setval(self, val):
		val = min(self.max, max(self.min, val))
		if val <> self.val:
			self.val = val
			self.text = self.pretext + `self.val` + self.postext
			if self.setval_hook:
				self.setval_hook(self)
			self.redraw()
	#
	def settext(self, text):
		pass # shouldn't be called at all
	#
	def mouse_down(self, detail):
		h, v = hv = detail[_HV]
		if self.enabled and self.mousetest(hv):
			self.anchor = h
			self.oldval = self.val
			self.active = 1
	#
	def mouse_move(self, detail):
		if self.active:
			h, v = detail[_HV]
			self.setval(self.oldval + (h - self.anchor))
	#
	def mouse_up(self, detail):
		if self.active:
			h, v = detail[_HV]
			self.setval(self.oldval + (h - self.anchor))
			self.active = 0
	#
