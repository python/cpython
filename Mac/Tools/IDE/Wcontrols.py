from Carbon import Ctl, Controls
from Carbon import Evt, Qd, Win
import Wbase


class ControlWidget(Wbase.ClickableWidget):

    """Baseclass for all native controls."""

    def __init__(self, possize, title = "Control", procID = 0, callback = None, value = 0, min = 0, max = 1, viewsize = 0):
        Wbase.ClickableWidget.__init__(self, possize)
        self._control = None
        self._title = title
        self._callback = callback
        self._procID = procID
        self._value = value
        self._min = min
        self._max = max
        self._enabled = 1
        self._viewsize = viewsize

    def open(self):
        self._calcbounds()

        # NewControl doesn't accept 32-bit value, min, or max, so for consistency
        # with the new 32-bit set/get methods, out-of-range values are initially
        # set as zero, followed by a 32-bit set of the actual value.
        # Values not representable in 16 bits will fail on MacOS 8.1, however
        # the vast majority of control usage should still be compatible.
        _value, _min, _max = self._value, self._min, self._max
        if -32768 <= _value <= 32767:
            bigvalue = None
        else:
            bigvalue = _value
            _value = 0
        if -32768 <= _min <= 32767:
            bigmin = None
        else:
            bigmin = _min
            _min = 0
        if -32768 <= _max <= 32767:
            bigmax = None
        else:
            bigmax = _max
            _max = 0
        self._control = Ctl.NewControl(self._parentwindow.wid,
                                        self._bounds,
                                        self._title,
                                        1,
                                        _value,
                                        _min,
                                        _max,
                                        self._procID,
                                        0)
        if bigvalue:
            self._control.SetControl32BitValue(bigvalue)
        if bigmin:
            self._control.SetControl32BitMinimum(bigmin)
        if bigmax:
            self._control.SetControl32BitMaximum(bigmax)
        if self._viewsize:
            try:
                self._control.SetControlViewSize(self._viewsize)
                # Not available in MacOS 8.1, but that's OK since it only affects
                # proportional scrollbars which weren't available in 8.1 either.
            except NotImplementedError:
                pass
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
            if onoff:
                self._control.ActivateControl()
            else:
                self._control.DeactivateControl()

    def draw(self, visRgn = None):
        if self._visible:
            self._control.Draw1Control()

    def test(self, point):
        if Qd.PtInRect(point, self._bounds) and self._enabled:
            return 1
        #ctltype, control = Ctl.FindControl(point, self._parentwindow.wid)
        #if self._enabled and control == self._control:
        #       return 1

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

    def set(self, value):
        if self._control:
            if -32768 <= value <= 32767:
                # No 32-bit control support in MacOS 8.1, so use
                # the 16-bit interface when possible.
                self._control.SetControlValue(value)
            else:
                self._control.SetControl32BitValue(value)
        else:
            self._value = value

    def get(self):
        if self._control:
            try:
                return self._control.GetControl32BitValue()
                # No 32-bit control support in MacOS 8.1, so fall
                # back to the 16-bit interface when needed.
            except NotImplementedError:
                return self._control.GetControlValue()
        else:
            return self._value


class Button(ControlWidget):

    """Standard push button."""

    procID = Controls.pushButProc | Controls.useWFont

    def __init__(self, possize, title = "Button", callback = None):
        ControlWidget.__init__(self, possize, title, self.procID, callback, 0, 0, 1)
        self._isdefault = 0

    def push(self):
        if not self._enabled:
            return
        # emulate the pushing of the button
        import time
        self._control.HiliteControl(Controls.kControlButtonPart)
        self._parentwindow.wid.GetWindowPort().QDFlushPortBuffer(None)  # needed under OSX
        time.sleep(0.1)
        self._control.HiliteControl(0)
        if self._callback:
            Wbase.CallbackCall(self._callback, 0)

    def enable(self, onoff):
        if self._control and self._enabled <> onoff:
            self._control.HiliteControl((not onoff) and 255)
            self._enabled = onoff

    def show(self, onoff):
        ControlWidget.show(self, onoff)

    def draw(self, visRgn = None):
        if self._visible:
            self._control.Draw1Control()

    def open(self):
        ControlWidget.open(self)
        if self._isdefault:
            self._setdefault(self._isdefault)

    def _setdefault(self, onoff):
        c = self._control
        if c is not None:
            if onoff:
                data = "\xFF"
            else:
                data = "\0"
            # hide before changing state, otherwise the button isn't always
            # redrawn correctly, although it's quite different under Aqua
            # and Classic...
            c.HideControl()
            c.SetControlData(Controls.kControlNoPart,
                            Controls.kControlPushButtonDefaultTag, data)
            c.ShowControl()
        self._isdefault = onoff

    def adjust(self, oldbounds):
        if self._isdefault:
            old = Qd.InsetRect(oldbounds, -4, -4)
            new = Qd.InsetRect(self._bounds, -4, -4)
            Qd.EraseRect(old)
            self.GetWindow().InvalWindowRect(old)
            self.GetWindow().InvalWindowRect(new)
        ControlWidget.adjust(self, oldbounds)


class BevelButton(Button):
    procID = Controls.kControlBevelButtonNormalBevelProc | Controls.useWFont


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


class Scrollbar(ControlWidget):

    """Standard scrollbar."""

    def __init__(self, possize, callback=None, value=0, min=0, max=0, livefeedback=1):
        if livefeedback:
            procID = Controls.kControlScrollBarLiveProc
        else:
            procID = Controls.scrollBarProc
        ControlWidget.__init__(self, possize, "", procID, callback, value, min, max)

    # interface
#       def set(self, value):
#               if self._callback:
#                       Wbase.CallbackCall(self._callback, 1, value)

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
        if self._control is not None:
            if -32768 <= min <= 32767:
                # No 32-bit control support in MacOS 8.1, so use
                # the 16-bit interface when possible.
                self._control.SetControlMinimum(min)
            else:
                self._control.SetControl32BitMinimum(min)
        else:
            self._min = min

    def setmax(self, max):
        if self._control is not None:
            if -32768 <= max <= 32767:
                # No 32-bit control support in MacOS 8.1, so use
                # the 16-bit interface when possible.
                self._control.SetControlMaximum(max)
            else:
                self._control.SetControl32BitMaximum(max)
        else:
            self._max = max

    def setviewsize(self, viewsize):
        if self._control is not None:
            try:
                self._control.SetControlViewSize(viewsize)
                # Not available in MacOS 8.1, but that's OK since it only affects
                # proportional scrollbars which weren't available in 8.1 either.
            except NotImplementedError:
                pass
        else:
            self._viewsize = viewsize

    def getmin(self):
        try:
            return self._control.GetControl32BitMinimum()
            # No 32-bit control support in MacOS 8.1, so fall
            # back to the 16-bit interface when needed.
        except NotImplementedError:
            return self._control.GetControlMinimum()

    def getmax(self):
        try:
            return self._control.GetControl32BitMaximum()
            # No 32-bit control support in MacOS 8.1, so fall
            # back to the 16-bit interface when needed.
        except NotImplementedError:
            return self._control.GetControlMaximum()

    # internals
    def click(self, point, modifiers):
        if not self._enabled:
            return
        def hitter(ctl, part, self=self):
            if part:
                self._hit(part)
        part = self._control.TrackControl(point, hitter)

    def _hit(self, part):
        value = None
        if part == Controls.inThumb:
            try:
                value = self._control.GetControl32BitValue()
                # No 32-bit control support in MacOS 8.1, so fall
                # back to the 16-bit interface when needed.
            except NotImplementedError:
                value = self._control.GetControlValue()
        elif part == Controls.inUpButton:
            value = "+"
        elif part == Controls.inDownButton:
            value = "-"
        elif part == Controls.inPageUp:
            value = "++"
        elif part == Controls.inPageDown:
            value = "--"
        if value is not None and self._callback:
            Wbase.CallbackCall(self._callback, 1, value)

    def draw(self, visRgn = None):
        if self._visible:
            self._control.Draw1Control()
            #Qd.FrameRect(self._bounds)

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


def _scalebarvalue(absmin, absmax, curmin, curmax):
    if curmin <= absmin and curmax >= absmax:
        return None
    if curmin <= absmin:
        return 0
    if curmax >= absmax:
        return 32767
    perc = float(curmin-absmin) / float((absmax - absmin) - (curmax - curmin))
    return int(perc*32767)
