# Module 'Buttons'


# Import module 'rect' renamed as '_rect' to avoid exporting it on
# 'from Buttons import *'
#
import rect
_rect = rect
del rect


# Field indices in mouse event detail
#
_HV = 0
_CLICKS = 1
_BUTTON = 2
_MASK = 3


# LabelAppearance provides defaults for all appearance methods.
# selected state not visible
# disabled --> crossed out
# hilited  --> inverted
#
class LabelAppearance():
	#
	# Initialization
	#
	def init_appearance(self):
		self.bounds = _rect.empty
		self.enabled = 1
		self.hilited = 0
		self.selected = 0
		self.text = ''
	#
	# Size enquiry
	#
	def minsize(self, m):
		try:
			self.text = self.text
		except NameError:
			self.text = ''
		return m.textwidth(self.text) + 6, m.lineheight() + 6
	#
	def getbounds(self):
		return self.bounds
	#
	# Changing the parameters
	#
	def settext(self, text):
		self.text = text
		if self.bounds <> _rect.empty:
			self.recalctextpos()
			self.redraw()
	#
	def setbounds(self, bounds):
		if self.bounds <> _rect.empty:
			self.parent.change(self.bounds)
		self.bounds = bounds
		if self.bounds <> _rect.empty:
			self.recalc()
			self.parent.change(bounds)
	#
	# Changing the state bits
	#
	def enable(self, flag):
		if flag <> self.enabled:
			self.enabled = flag
			if self.bounds <> _rect.empty:
				self.flipenable(self.parent.begindrawing())
	#
	def hilite(self, flag):
		if flag <> self.hilited:
			self.hilited = flag
			if self.bounds <> _rect.empty:
				self.fliphilite(self.parent.begindrawing())
	#
	def select(self, flag):
		if flag <> self.selected:
			self.selected = flag
			if self.bounds <> _rect.empty:
				self.redraw()
	#
	# Recalculate the box bounds and text position.
	# This can be overridden by buttons that draw different boxes
	# or want their text in a different position.
	#
	def recalc(self):
		if self.bounds <> _rect.empty:
			self.recalcbounds()
			self.recalctextpos()
	#
	def recalcbounds(self):
		self.hilitebounds = _rect.inset(self.bounds, (3, 3))
		self.crossbounds = self.bounds
	#
	def recalctextpos(self):
		(left, top), (right, bottom) = self.bounds
		m = self.parent.beginmeasuring()
		h = (left + right - m.textwidth(self.text)) / 2
		v = (top + bottom - m.lineheight()) / 2
		self.textpos = h, v
	#
	# Generic drawing interface.
	# Do not override redraw() or draw() methods; override drawit() c.s.
	#
	def redraw(self):
		if self.bounds <> _rect.empty:
			self.draw(self.parent.begindrawing(), self.bounds)
	#
	def draw(self, (d, area)):
		area = _rect.intersect(area, self.bounds)
		if area = _rect.empty:
			return
		d.cliprect(area)
		d.erase(self.bounds)
		self.drawit(d)
		d.noclip()
	#
	# The drawit() method is fairly generic but may be overridden.
	#
	def drawit(self, d):
		self.drawpict(d)
		if self.text:
			d.text(self.textpos, self.text)
		if not self.enabled:
			self.flipenable(d)
		if self.hilited:
			self.fliphilite(d)
	#
	# Default drawing detail functions.
	# Overriding these is normally sufficient to get different
	# appearances.
	#
	def drawpict(self, d):
		pass
	#
	def flipenable(self, d):
		_xorcross(d, self.crossbounds)
	#
	def fliphilite(self, d):
		d.invert(self.hilitebounds)


# ButtonAppearance displays a centered string in a box.
# selected --> bold border
# disabled --> crossed out
# hilited  --> inverted
#
class ButtonAppearance() = LabelAppearance():
	#
	def drawpict(self, d):
		d.box(_rect.inset(self.bounds, (1, 1)))
		if self.selected:
			# Make a thicker box
			d.box(self.bounds)
			d.box(_rect.inset(self.bounds, (2, 2)))
			d.box(_rect.inset(self.bounds, (3, 3)))
	#


# CheckAppearance displays a small square box and a left-justified string.
# selected --> a cross appears in the box
# disabled --> whole button crossed out
# hilited  --> box is inverted
#
class CheckAppearance() = LabelAppearance():
	#
	def minsize(self, m):
		width, height = m.textwidth(self.text) + 6, m.lineheight() + 6
		return width + height + m.textwidth(' '), height
	#
	def drawpict(self, d):
		d.box(self.boxbounds)
		if self.selected: _xorcross(d, self.boxbounds)
	#
	def recalcbounds(self):
		LabelAppearance.recalcbounds(self)
		(left, top), (right, bottom) = self.bounds
		self.size = bottom - top - 4
		self.boxbounds = (left+2, top+2), (left+2+self.size, bottom-2)
		self.hilitebounds = self.boxbounds
	#
	def recalctextpos(self):
		m = self.parent.beginmeasuring()
		(left, top), (right, bottom) = self.boxbounds
		h = right + m.textwidth(' ')
		v = top + (self.size - m.lineheight()) / 2
		self.textpos = h, v
	#


# RadioAppearance displays a round indicator and a left-justified string.
# selected --> a dot appears in the indicator
# disabled --> whole button crossed out
# hilited  --> indicator is inverted
#
class RadioAppearance() = CheckAppearance():
	#
	def drawpict(self, d):
		(left, top), (right, bottom) = self.boxbounds
		radius = self.size / 2
		h, v = left + radius, top + radius
		d.circle((h, v), radius)
		if self.selected:
			some = radius/3
			d.paint((h-some, v-some), (h+some, v+some))
	#


# NoReactivity ignores mouse events.
#
class NoReactivity():
	def init_reactivity(self): pass


# BaseReactivity defines hooks and asks for mouse events,
# but provides only dummy mouse event handlers.
# The trigger methods call the corresponding hooks set by the user.
# Hooks (and triggers) mean the following:
# down_hook	called on some mouse-down events
# move_hook	called on some mouse-move events
# up_hook	called on mouse-up events
# on_hook	called for buttons with on/off state, when it goes on
# hook		called when a button 'fires' or a radiobutton goes on
# There are usually extra conditions, e.g., hooks are only called
# when the button is enabled, or active, or selected (on).
#
class BaseReactivity():
	#
	def init_reactivity(self):
		self.down_hook = self.move_hook = self.up_hook = \
			self.on_hook = self.off_hook = \
			self.hook = self.active = 0
		self.parent.need_mouse(self)
	#
	def mousetest(self, hv):
		return _rect.pointinrect(hv, self.bounds)
	#
	def mouse_down(self, detail):
		pass
	#
	def mouse_move(self, detail):
		pass
	#
	def mouse_up(self, detail):
		pass
	#
	def down_trigger(self):
		if self.down_hook: self.down_hook(self)
	#
	def move_trigger(self):
		if self.move_hook: self.move_hook(self)
	#
	def up_trigger(self):
		if self.up_hook: self.up_hook(self)
	#
	def on_trigger(self):
		if self.on_hook: self.on_hook(self)
	#
	def off_trigger(self):
		if self.off_hook: self.off_hook(self)
	#
	def trigger(self):
		if self.hook: self.hook(self)


# ToggleReactivity acts like a simple pushbutton.
# It toggles its hilite state on mouse down events.
#
class ToggleReactivity() = BaseReactivity():
	#
	def mouse_down(self, detail):
		if self.enabled and self.mousetest(detail[_HV]):
			self.active = 1
			self.hilite(not self.hilited)
			self.down_trigger()
	#
	def mouse_move(self, detail):
		if self.active:
			self.move_trigger()
	#
	def mouse_up(self, detail):
		if self.active:
			self.up_trigger()
			self.active = 0
	#
	def down_trigger(self):
		if self.hilited:
			self.on_trigger()
		else:
			self.off_trigger()
		self.trigger()
	#


# TriggerReactivity acts like a fancy pushbutton.
# It hilites itself while the mouse is down within its bounds.
#
class TriggerReactivity() = BaseReactivity():
	#
	def mouse_down(self, detail):
		if self.enabled and self.mousetest(detail[_HV]):
			self.active = 1
			self.hilite(1)
			self.down_trigger()
	#
	def mouse_move(self, detail):
		if self.active:
			self.hilite(self.mousetest(detail[_HV]))
			if self.hilited:
				self.move_trigger()
	#
	def mouse_up(self, detail):
		if self.active:
			self.hilite(self.mousetest(detail[_HV]))
			if self.hilited:
				self.up_trigger()
				self.trigger()
			self.active = 0
			self.hilite(0)
	#


# CheckReactivity handles mouse events like TriggerReactivity,
# It overrides the up_trigger method to flip its selected state.
#
class CheckReactivity() = TriggerReactivity():
	#
	def up_trigger(self):
		self.select(not self.selected)
		if self.selected:
			self.on_trigger()
		else:
			self.off_trigger()
		self.trigger()


# RadioReactivity turns itself on and the other buttons in its group
# off when its up_trigger method is called.
#
class RadioReactivity() = TriggerReactivity():
	#
	def init_reactivity(self):
		TriggerReactivity.init_reactivity(self)
		self.group = []
	#
	def up_trigger(self):
		for b in self.group:
			if b <> self:
				if b.selected:
					b.select(0)
					b.off_trigger()
		self.select(1)
		self.on_trigger()
		self.trigger()


# Auxiliary class for 'define' method.
# Call the initializers in the right order.
#
class Define():
	#
	def define(self, parent):
		self.parent = parent
		parent.addchild(self)
		self.init_appearance()
		self.init_reactivity()
		return self
	#
	def destroy(self):
		self.parent = 0
	#
	def definetext(self, (parent, text)):
		self = self.define(parent)
		self.settext(text)
		return self


# Subroutine to cross out a rectangle.
#
def _xorcross(d, bounds):
	((left, top), (right, bottom)) = bounds
	# This is s bit funny to make it look better
	left = left + 2
	right = right - 2
	top = top + 2
	bottom = bottom - 3
	d.xorline(((left, top), (right, bottom)))
	d.xorline((left, bottom), (right, top))


# Ready-made button classes.
#
class Label() = NoReactivity(), LabelAppearance(), Define(): pass
class PushButton() = TriggerReactivity(), ButtonAppearance(), Define(): pass
class CheckButton() = CheckReactivity(), CheckAppearance(), Define(): pass
class RadioButton() = RadioReactivity(), RadioAppearance(), Define(): pass
class ToggleButton() = ToggleReactivity(), ButtonAppearance(), Define(): pass
