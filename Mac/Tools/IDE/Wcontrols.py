import Ctl
import Controls
import Win
import Wbase
import Qd
import Evt

class ControlWidget(Wbase.ClickableWidget):
	
	"""Baseclass for all native controls."""
	
	def __init__(self, possize, title = "Control", procID = 0, callback = None, value = 0, min = 0, max = 1):
		Wbase.ClickableWidget.__init__(self, possize)
		self._control = None
		self._title = title
		self._callback = callback
		self._procID = procID
		self._value = value
		self._min = min
		self._max = max
		self._enabled = 1
	
	def open(self):
		self._calcbounds()
		self._control = Ctl.NewControl(self._parentwindow.wid, 
						self._bounds, 
						self._title, 
						1, 
						self._value, 
						self._min, 
						self._max, 
						self._procID, 
						0)
		self.SetPort()
		#self.GetWindow().ValidWindowRect(self._bounds)
		self.enable(self._enabled)
	
	def adjust(self, oldbounds):
		self.SetPort()
		self._control.HideControl()
		self._control.MoveControl(self._bounds[0], self._bounds[1])
		self._control.SizeControl(self._bounds[2] - self._bounds[0], self._bounds[3] - self._bounds[1])
		if self._visible:
			Qd.EraseRect(self._bounds)
			self._control.ShowControl()
			self.GetWindow().ValidWindowRect(self._bounds)
	
	def close(self):
		self._control.HideControl()
		self._control = None
		Wbase.ClickableWidget.close(self)
	
	def enable(self, onoff):
		if self._control and self._enabled <> onoff:
			self._control.HiliteControl((not onoff) and 255)
			self._enabled = onoff
	
	def show(self, onoff):
		self._visible = onoff
		for w in self._widgets:
			w.show(onoff)
		if onoff:
			self._control.ShowControl()
		else:
			self._control.HideControl()
	
	def activate(self, onoff):
		self._activated = onoff
		if self._enabled:
			self._control.HiliteControl((not onoff) and 255)
	
	def draw(self, visRgn = None):
		if self._visible:
			self._control.Draw1Control()
	
	def test(self, point):
		ctltype, control = Ctl.FindControl(point, self._parentwindow.wid)
		if self._enabled and control == self._control:
			return 1
	
	def click(self, point, modifiers):
		if not self._enabled:
			return
		part = self._control.TrackControl(point)
		if part:
			if self._callback:
				Wbase.CallbackCall(self._callback, 0)
	
	def settitle(self, title):
		if self._control:
			self._control.SetControlTitle(title)
		self._title = title
	
	def gettitle(self):
		return self._title

class Button(ControlWidget):
	
	"""Standard push button."""
	
	def __init__(self, possize, title = "Button", callback = None):
		procID = Controls.pushButProc | Controls.useWFont
		ControlWidget.__init__(self, possize, title, procID, callback, 0, 0, 1)
		self._isdefault = 0
	
	def push(self):
		if not self._enabled:
			return
		import time
		self._control.HiliteControl(1)
		time.sleep(0.1)
		self._control.HiliteControl(0)
		if self._callback:
			Wbase.CallbackCall(self._callback, 0)
	
	def enable(self, onoff):
		if self._control and self._enabled <> onoff:
			self._control.HiliteControl((not onoff) and 255)
			self._enabled = onoff
			if self._isdefault and self._visible:
				self.SetPort()
				self.drawfatframe(onoff)
	
	def activate(self, onoff):
		self._activated = onoff
		if self._enabled:
			self._control.HiliteControl((not onoff) and 255)
			if self._isdefault and self._visible:
				self.SetPort()
				self.drawfatframe(onoff)
	
	def show(self, onoff):
		ControlWidget.show(self, onoff)
		if self._isdefault:
			self.drawfatframe(onoff and self._enabled)
	
	def draw(self, visRgn = None):
		if self._visible:
			self._control.Draw1Control()
			if self._isdefault and self._activated:
				self.drawfatframe(self._enabled)
	
	def drawfatframe(self, onoff):
		state = Qd.GetPenState()
		if onoff:
			Qd.PenPat(Qd.qd.black)
		else:
			Qd.PenPat(Qd.qd.white)
		fatrect = Qd.InsetRect(self._bounds, -4, -4)
		Qd.PenSize(3, 3)
		Qd.FrameRoundRect(fatrect, 16, 16)
		Qd.SetPenState(state)
	
	def _setdefault(self, onoff):
		self._isdefault = onoff
		if self._control and self._enabled:
			self.SetPort()
			self.drawfatframe(onoff)
	
	def adjust(self, oldbounds):
		if self._isdefault:
			old = Qd.InsetRect(oldbounds, -4, -4)
			new = Qd.InsetRect(self._bounds, -4, -4)
			Qd.EraseRect(old)
			self.GetWindow().InvalWindowRect(old)
			self.GetWindow().InvalWindowRect(new)
		ControlWidget.adjust(self, oldbounds)


class CheckBox(ControlWidget):
	
	"""Standard checkbox."""
	
	def __init__(self, possize, title = "Checkbox", callback = None, value = 0):
		procID = Controls.checkBoxProc | Controls.useWFont
		ControlWidget.__init__(self, possize, title, procID, callback, value, 0, 1)
	
	def click(self, point, modifiers):
		if not self._enabled:
			return
		part = self._control.TrackControl(point)
		if part:
			self.toggle()
			if self._callback:
				Wbase.CallbackCall(self._callback, 0, self.get())
	
	def push(self):
		if not self._enabled:
			return
		self.toggle()
		if self._callback:
			Wbase.CallbackCall(self._callback, 0, self.get())
	
	def toggle(self):
		self.set(not self.get())
	
	def set(self, value):
		if self._control:
			self._control.SetControlValue(value)
		else:
			self._value = value
	
	def get(self):
		if self._control:
			return self._control.GetControlValue()
		else:
			return self._value
	

class RadioButton(ControlWidget):
	
	"""Standard radiobutton."""
	
	# XXX We need a radiogroup widget; this is too kludgy.
	
	def __init__(self, possize, title, thebuttons, callback = None, value = 0):
		procID = Controls.radioButProc | Controls.useWFont
		ControlWidget.__init__(self, possize, title, procID, callback, value, 0, 1)
		self.thebuttons = thebuttons
		thebuttons.append(self)
	
	def close(self):
		self.thebuttons = None
		ControlWidget.close(self)
	
	def click(self, point, modifiers):
		if not self._enabled:
			return
		part = self._control.TrackControl(point)
		if part:
			self.set(1)
			if self._callback:
				Wbase.CallbackCall(self._callback, 0, 1)
	
	def push(self):
		if not self._enabled:
			return
		self.set(1)
		if self._callback:
			Wbase.CallbackCall(self._callback, 0, 1)
	
	def set(self, value):
		for button in self.thebuttons:
			if button._control:
				button._control.SetControlValue(button == self)
			else:
				button._value = (button == self)
	
	def get(self):
		if self._control:
			return self._control.GetControlValue()
		else:
			return self._value
	

class Scrollbar(ControlWidget):
	
	"""Standard scrollbar."""
	
	def __init__(self, possize, callback = None, value = 0, min = 0, max = 0):
		procID = Controls.scrollBarProc
		ControlWidget.__init__(self, possize, "", procID, callback, value, min, max)
	
	# interface
	def set(self, value):
		if self._callback:
			Wbase.CallbackCall(self._callback, 1, value)
	
	def up(self):
		if self._callback:
			Wbase.CallbackCall(self._callback, 1, '+')
	
	def down(self):
		if self._callback:
			Wbase.CallbackCall(self._callback, 1, '-')
	
	def pageup(self):
		if self._callback:
			Wbase.CallbackCall(self._callback, 1, '++')
	
	def pagedown(self):
		if self._callback:
			Wbase.CallbackCall(self._callback, 1, '--')
	
	def setmin(self, min):
		self._control.SetControlMinimum(min)
	
	def setmax(self, min):
		self._control.SetControlMinimum(max)
	
	def getmin(self):
		return self._control.GetControlMinimum()
	
	def getmax(self):
		return self._control.GetControlMinimum()
	
	# internals
	def click(self, point, modifiers):
		if not self._enabled:
			return
		# custom TrackControl. A mousedown in a scrollbar arrow or page area should
		# generate _control hits as long as the mouse is a) down, b) still in the same part
		part = self._control.TestControl(point)
		if Controls.inUpButton <= part <= Controls.inPageDown:	
			self._control.HiliteControl(part)
			self._hit(part)
			oldpart = part
			# slight delay before scrolling at top speed...
			now = Evt.TickCount()
			while Evt.StillDown():
				if (Evt.TickCount() - now) > 18: # 0.3 seconds
					break
			while Evt.StillDown():
				part = self._control.TestControl(point)
				if part == oldpart:
					self._control.HiliteControl(part)
					self._hit(part)
				else:
					self._control.HiliteControl(0)
				self.SetPort()
				point = Evt.GetMouse()
			self._control.HiliteControl(0)
		elif part == Controls.inThumb:
			part = self._control.TrackControl(point)
			if part:
				self._hit(part)
	
	def _hit(self, part):
		if part == Controls.inThumb:
			value = self._control.GetControlValue()
		elif part == Controls.inUpButton:
			value = "+"
		elif part == Controls.inDownButton:
			value = "-"
		elif part == Controls.inPageUp:
			value = "++"
		elif part == Controls.inPageDown:
			value = "--"
		if self._callback:
			Wbase.CallbackCall(self._callback, 1, value)
	
	def draw(self, visRgn = None):
		if self._visible:
			self._control.Draw1Control()
			Qd.FrameRect(self._bounds)
	
	def adjust(self, oldbounds):
		self.SetPort()
		self.GetWindow().InvalWindowRect(oldbounds)
		self._control.HideControl()
		self._control.MoveControl(self._bounds[0], self._bounds[1])
		self._control.SizeControl(self._bounds[2] - self._bounds[0], self._bounds[3] - self._bounds[1])
		if self._visible:
			Qd.EraseRect(self._bounds)
			if self._activated:
				self._control.ShowControl()
			else:
				Qd.FrameRect(self._bounds)
			self.GetWindow().ValidWindowRect(self._bounds)
	
	def activate(self, onoff):
		self._activated = onoff
		if self._visible:
			if onoff:
				self._control.ShowControl()
			else:
				self._control.HideControl()
				self.draw(None)
				self.GetWindow().ValidWindowRect(self._bounds)
		
	def set(self, value):
		if self._control:
			self._control.SetControlValue(value)
		else:
			self._value = value
	
	def get(self):
		if self._control:
			return self._control.GetControlValue()
		else:
			return self._value
	

def _scalebarvalue(absmin, absmax, curmin, curmax):
	if curmin <= absmin and curmax >= absmax:
		return None
	if curmin <= absmin:
		return 0
	if curmax >= absmax:
		return 32767
	perc = float(curmin-absmin) / float((absmax - absmin) - (curmax - curmin))
	return int(perc*32767)

