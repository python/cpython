# Module 'Sliders'


import stdwin
from stdwinevents import *
import rect
from Buttons import *
from HVSplit import HSplit


# Field indices in event detail
#
_HV = 0
_CLICKS = 1
_BUTTON = 2
_MASK = 3


# DragSlider is the simplest possible slider.
# It looks like a button but dragging the mouse left or right
# changes the controlled value.
# It does not support any of the triggers or hooks defined by Buttons,
# but defines its own setval_trigger and setval_hook.
#
class DragSliderReactivity() = BaseReactivity():
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

class DragSliderAppearance() = ButtonAppearance():
	#
	# INVARIANTS maintained by the setval method:
	#
	#	self.min <= self.val <= self.max
	#	self.text = self.pretext + `self.val` + self.postext
	#
	# (Notice that unlike Python ranges, the end point belongs
	# to the range.)
	#
	def init_appearance(self):
		ButtonAppearance.init_appearance(self)
		self.min = 0
		self.val = 0
		self.max = 100
		self.hook = 0
		self.pretext = self.postext = ''
		self.recalctext()
	#
	# The 'get*' and 'set*' methods belong to the generic slider interface
	#
	def getval(self): return self.val
	#
	def sethook(self, hook):
		self.hook = hook
	#
	def setminvalmax(self, (min, val, max)):
		self.min = min
		self.max = max
		self.setval(val)
	#
	def settexts(self, (pretext, postext)):
		self.pretext = pretext
		self.postext = postext
		self.recalctext()
	#
	def setval(self, val):
		val = min(self.max, max(self.min, val))
		if val <> self.val:
			self.val = val
			self.recalctext()
			self.trigger()
	#
	def trigger(self):
		if self.hook:
			self.hook(self)
	#
	def recalctext(self):
		self.settext(self.pretext + `self.val` + self.postext)
	#

class DragSlider() = DragSliderReactivity(), DragSliderAppearance(), Define():
	def definetext(self, (parent, text)):
		raise RuntimeError, 'DragSlider.definetext() not supported'


# Auxiliary class for PushButton incorporated in ComplexSlider
#
class _StepButton() = PushButton():
	def define(self, parent):
		self = PushButton.define(self, parent)
		self.step = 0
		return self
	def setstep(self, step):
		self.step = step
	def definetextstep(self, (parent, text, step)):
		self = self.definetext(parent, text)
		self.setstep(step)
		return self
	def init_reactivity(self):
		PushButton.init_reactivity(self)
		self.parent.need_timer(self)
	def step_trigger(self):
		self.parent.setval(self.parent.getval() + self.step)
	def down_trigger(self):
		self.step_trigger()
		self.parent.settimer(5)
	def timer(self):
		if self.hilited:
			self.step_trigger()
		if self.active:
			self.parent.settimer(1)


# A complex slider is an HSplit initialized to three buttons:
# one to step down, a dragslider, and one to step up.
#
class ComplexSlider() = HSplit():
	#
	# Override Slider define() method
	#
	def define(self, parent):
		self = self.create(parent) # HSplit
		#
		self.downbutton = _StepButton().definetextstep(self, '-', -1)
		self.dragbutton = DragSlider().define(self)
		self.upbutton = _StepButton().definetextstep(self, '+', 1)
		#
		return self
	#
	# Override HSplit methods
	#
	def minsize(self, m):
		w1, h1 = self.downbutton.minsize(m)
		w2, h2 = self.dragbutton.minsize(m)
		w3, h3 = self.upbutton.minsize(m)
		height = max(h1, h2, h3)
		w1 = max(w1, height)
		w3 = max(w3, height)
		return w1+w2+w3, height
	#
	def setbounds(self, bounds):
		(left, top), (right, bottom) = self.bounds = bounds
		size = bottom - top
		self.downbutton.setbounds((left, top), (left+size, bottom))
		self.dragbutton.setbounds((left+size, top), \
						(right-size, bottom))
		self.upbutton.setbounds((right-size, top), (right, bottom))
	#
	# Pass other Slider methods on to dragbutton
	#
	def getval(self): return self.dragbutton.getval()
	def sethook(self, hook): self.dragbutton.sethook(hook)
	def setminvalmax(self, args): self.dragbutton.setminvalmax(args)
	def settexts(self, args): self.dragbutton.settexts(args)
	def setval(self, val): self.dragbutton.setval(val)
	def enable(self, flag):
		self.downbutton.enable(flag)
		self.dragbutton.enable(flag)
		self.upbutton.enable(flag)
