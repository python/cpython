# Module 'Sliders'
#
# XXX Should split caller interface, appearance and reactivity better


import stdwin
from stdwinevents import *
import rect
from Buttons import *
from Resize import *


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
class DragSliderReactivity() = NoReactivity():
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

class DragSliderAppearance() = NoResize(), ButtonAppearance():
	#
	def define(self, (win, bounds)):
		self.min = 0
		self.val = -1 # Changed by next setval call
		self.max = 100
		self.setval_hook = 0
		self.pretext = self.postext = ''
		self = ClassicButton.define(self, (win, bounds, ''))
		self.setval(50)
		return self
	#
	# INVARIANTS maintained by the setval method:
	#
	#	self.min <= self.val <= self.max
	#	self.text = self.pretext + `self.val` + self.postext
	#
	# (Notice that unlike in Python ranges, the end point belongs
	# to the range.)
	#
	def setval(self, val):
		val = min(self.max, max(self.min, val))
		if val <> self.val:
			self.val = val
			self.setval_trigger()
			# (The trigger may change val, pretext and postext)
			self.settext(self.pretext + `self.val` + self.postext)
	#
	def setval_trigger(self):
		if self.setval_hook:
			self.setval_hook(self)
	#

class DragSlider() = DragSliderReactivity(), DragSliderAppearance(): pass


# Auxiliary class for DragSlider incorporated in ComplexSlider
#
class _SubDragSlider() = DragSlider():
	def define(self, (win, bounds, parent)):
		self.parent = parent
		return DragSlider.define(self, (win, bounds))
	def setval_trigger(self):
		self.parent.val = self.val
		self.parent.setval_trigger()

# Auxiliary class for ClassicButton incorporated in ComplexSlider
#
class _SubClassicButton() = ClassicButton():
	def define(self, (win, bounds, text, step, parent)):
		self.parent = parent
		self.step = step
		return ClassicButton.define(self, (win, bounds, text))
	def down_trigger(self):
		self.parent.setval(self.parent.val + self.step)
		self.delay = 5
		self.win.settimer(self.delay)
	def move_trigger(self):
		self.win.settimer(self.delay)
	def timer_trigger(self):
		self.delay = 1
		self.parent.setval(self.parent.val + self.step)
		self.win.settimer(self.delay)

# A complex slider is a wrapper around three buttons:
# One to step down, a dragslider, and one to step up.
#
class ComplexSlider() = NoResize(), LabelAppearance(), NoReactivity():
	#
	def define(self, (win, bounds)):
		#
		self.win = win
		self.bounds = bounds
		self.setval_hook = 0
		#
		(left, top), (right, bottom) = bounds
		size = bottom - top
		#
		downbox = (left, top), (left+size, bottom)
		sliderbox = (left+size, top), (right-size, bottom)
		upbox = (right-size, top), (right, bottom)
		#
		self.downbutton = \
			_SubClassicButton().define(win, downbox, '-', -1, self)
		#
		self.sliderbutton = \
			_SubDragSlider().define(win, sliderbox, self)
		#
		self.upbutton = \
			_SubClassicButton().define(win, upbox, '+', 1, self)
		#
		self.min = self.sliderbutton.min
		self.val = self.sliderbutton.val
		self.max = self.sliderbutton.max
		self.pretext = self.sliderbutton.pretext
		self.postext = self.sliderbutton.postext
		#
		self.children = \
			[self.downbutton, self.sliderbutton, self.upbutton]
		#
		return self
	#
	def mouse_down(self, detail):
		for b in self.children:
			b.mouse_down(detail)
	#
	def mouse_move(self, detail):
		for b in self.children:
			b.mouse_move(detail)
	#
	def mouse_up(self, detail):
		for b in self.children:
			b.mouse_up(detail)
	#
	def timer(self):
		for b in self.children:
			b.timer()
	#
	def draw(self, area):
		for b in self.children:
			b.draw(area)
	#
	def setval(self, val):
		self.sliderbutton.min = self.min
		self.sliderbutton.max = self.max
		self.sliderbutton.pretext = self.pretext
		self.sliderbutton.postext = self.postext
		self.sliderbutton.setval(val)
	#
	def setval_trigger(self):
		if self.setval_hook:
			self.setval_hook(self)
	#
