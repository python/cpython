from Carbon import App, Evt, Qd, QuickDraw, Win
import string
from types import *
import sys

class WidgetsError(Exception): pass

DEBUG = 0


def _intRect((l, t, r, b)):
    return (int(l), int(t), int(r), int(b))


class Widget:

    """Base class for all widgets."""

    _selectable = 0

    def __init__(self, possize):
        self._widgets = []
        self._widgetsdict = {}
        self._possize = possize
        self._bounds = None
        self._visible = 1
        self._enabled = 0
        self._selected = 0
        self._activated = 0
        self._callback = None
        self._parent = None
        self._parentwindow = None
        self._bindings = {}
        self._backcolor = None

    def show(self, onoff):
        self._visible = onoff
        for w in self._widgets:
            w.show(onoff)
        if self._parentwindow is not None and self._parentwindow.wid is not None:
            self.SetPort()
            if onoff:
                self.draw()
            else:
                Qd.EraseRect(self._bounds)

    def draw(self, visRgn = None):
        if self._visible:
            # draw your stuff here
            pass

    def getpossize(self):
        return self._possize

    def getbounds(self):
        return self._bounds

    def move(self, x, y = None):
        """absolute move"""
        if y == None:
            x, y = x
        if type(self._possize) <> TupleType:
            raise WidgetsError, "can't move widget with bounds function"
        l, t, r, b = self._possize
        self.resize(x, y, r, b)

    def rmove(self, x, y = None):
        """relative move"""
        if y == None:
            x, y = x
        if type(self._possize) <> TupleType:
            raise WidgetsError, "can't move widget with bounds function"
        l, t, r, b = self._possize
        self.resize(l + x, t + y, r, b)

    def resize(self, *args):
        if len(args) == 1:
            if type(args[0]) == FunctionType or type(args[0]) == MethodType:
                self._possize = args[0]
            else:
                apply(self.resize, args[0])
        elif len(args) == 2:
            self._possize = (0, 0) + args
        elif len(args) == 4:
            self._possize = args
        else:
            raise TypeError, "wrong number of arguments"
        self._calcbounds()

    def open(self):
        self._calcbounds()

    def close(self):
        del self._callback
        del self._possize
        del self._bindings
        del self._parent
        del self._parentwindow

    def bind(self, key, callback):
        """bind a key or an 'event' to a callback"""
        if callback:
            self._bindings[key] = callback
        elif self._bindings.has_key(key):
            del self._bindings[key]

    def adjust(self, oldbounds):
        self.SetPort()
        self.GetWindow().InvalWindowRect(oldbounds)
        self.GetWindow().InvalWindowRect(self._bounds)

    def _calcbounds(self):
        # calculate absolute bounds relative to the window origin from our
        # abstract _possize attribute, which is either a 4-tuple or a callable object
        oldbounds = self._bounds
        pl, pt, pr, pb = self._parent._bounds
        if callable(self._possize):
            # _possize is callable, let it figure it out by itself: it should return
            # the bounds relative to our parent widget.
            width = pr - pl
            height = pb - pt
            self._bounds = Qd.OffsetRect(_intRect(self._possize(width, height)), pl, pt)
        else:
            # _possize must be a 4-tuple. This is where the algorithm by Peter Kriens and
            # Petr van Blokland kicks in. (*** Parts of this algorithm are applied for
            # patents by Ericsson, Sweden ***)
            l, t, r, b = self._possize
            # depending on the values of l(eft), t(op), r(right) and b(ottom),
            # they mean different things:
            if l < -1:
                # l is less than -1, this mean it measures from the *right* of it's parent
                l = pr + l
            else:
                # l is -1 or greater, this mean it measures from the *left* of it's parent
                l = pl + l
            if t < -1:
                # t is less than -1, this mean it measures from the *bottom* of it's parent
                t = pb + t
            else:
                # t is -1 or greater, this mean it measures from the *top* of it's parent
                t = pt + t
            if r > 1:
                # r is greater than 1, this means r is the *width* of the widget
                r = l + r
            else:
                # r is less than 1, this means it measures from the *right* of it's parent
                r = pr + r
            if b > 1:
                # b is greater than 1, this means b is the *height* of the widget
                b = t + b
            else:
                # b is less than 1, this means it measures from the *bottom* of it's parent
                b = pb + b
            self._bounds = (l, t, r, b)
        if oldbounds and oldbounds <> self._bounds:
            self.adjust(oldbounds)
        for w in self._widgets:
            w._calcbounds()

    def test(self, point):
        if Qd.PtInRect(point, self._bounds):
            return 1

    def click(self, point, modifiers):
        pass

    def findwidget(self, point, onlyenabled = 1):
        if self.test(point):
            for w in self._widgets:
                widget = w.findwidget(point)
                if widget is not None:
                    return widget
            if self._enabled or not onlyenabled:
                return self

    def forall(self, methodname, *args):
        for w in self._widgets:
            rv = apply(w.forall, (methodname,) + args)
            if rv:
                return rv
        if self._bindings.has_key("<" + methodname + ">"):
            callback = self._bindings["<" + methodname + ">"]
            rv = apply(callback, args)
            if rv:
                return rv
        if hasattr(self, methodname):
            method = getattr(self, methodname)
            return apply(method, args)

    def forall_butself(self, methodname, *args):
        for w in self._widgets:
            rv = apply(w.forall, (methodname,) + args)
            if rv:
                return rv

    def forall_frombottom(self, methodname, *args):
        if self._bindings.has_key("<" + methodname + ">"):
            callback = self._bindings["<" + methodname + ">"]
            rv = apply(callback, args)
            if rv:
                return rv
        if hasattr(self, methodname):
            method = getattr(self, methodname)
            rv = apply(method, args)
            if rv:
                return rv
        for w in self._widgets:
            rv = apply(w.forall_frombottom, (methodname,) + args)
            if rv:
                return rv

    def _addwidget(self, key, widget):
        if widget in self._widgets:
            raise ValueError, "duplicate widget"
        if self._widgetsdict.has_key(key):
            self._removewidget(key)
        self._widgets.append(widget)
        self._widgetsdict[key] = widget
        widget._parent = self
        self._setparentwindow(widget)
        if self._parentwindow and self._parentwindow.wid:
            widget.forall_frombottom("open")
            self.GetWindow().InvalWindowRect(widget._bounds)

    def _setparentwindow(self, widget):
        widget._parentwindow = self._parentwindow
        for w in widget._widgets:
            self._setparentwindow(w)

    def _removewidget(self, key):
        if not self._widgetsdict.has_key(key):
            raise KeyError, "no widget with key %r" % (key,)
        widget = self._widgetsdict[key]
        for k in widget._widgetsdict.keys():
            widget._removewidget(k)
        if self._parentwindow._currentwidget == widget:
            widget.select(0)
            self._parentwindow._currentwidget = None
        self.SetPort()
        self.GetWindow().InvalWindowRect(widget._bounds)
        widget.close()
        del self._widgetsdict[key]
        self._widgets.remove(widget)

    def __setattr__(self, attr, value):
        if type(value) == InstanceType and isinstance(value, Widget) and        \
                        attr not in ("_currentwidget", "_lastrollover",
                        "_parent", "_parentwindow", "_defaultbutton"):
            if hasattr(self, attr):
                raise ValueError, "Can't replace existing attribute: " + attr
            self._addwidget(attr, value)
        self.__dict__[attr] = value

    def __delattr__(self, attr):
        if attr == "_widgetsdict":
            raise AttributeError, "cannot delete attribute _widgetsdict"
        if self._widgetsdict.has_key(attr):
            self._removewidget(attr)
            if self.__dict__.has_key(attr):
                del self.__dict__[attr]
        elif self.__dict__.has_key(attr):
            del self.__dict__[attr]
        else:
            raise AttributeError, attr

    def __setitem__(self, key, value):
        self._addwidget(key, value)

    def __getitem__(self, key):
        if not self._widgetsdict.has_key(key):
            raise KeyError, key
        return self._widgetsdict[key]

    def __delitem__(self, key):
        self._removewidget(key)

    def SetPort(self):
        self._parentwindow.SetPort()


    def GetWindow(self):
        return self._parentwindow.GetWindow()

    def __del__(self):
        if DEBUG:
            print "%s instance deleted" % self.__class__.__name__

    def _drawbounds(self):
        Qd.FrameRect(self._bounds)


class ClickableWidget(Widget):

    """Base class for clickable widgets. (note: self._enabled must be true to receive click events.)"""

    def click(self, point, modifiers):
        pass

    def enable(self, onoff):
        self._enabled = onoff
        self.SetPort()
        self.draw()

    def callback(self):
        if self._callback:
            return CallbackCall(self._callback, 1)


class SelectableWidget(ClickableWidget):

    """Base class for selectable widgets."""

    _selectable = 1

    def select(self, onoff, isclick = 0):
        if onoff == self._selected:
            return 1
        if self._bindings.has_key("<select>"):
            callback = self._bindings["<select>"]
            if callback(onoff):
                return 1
        self._selected = onoff
        if onoff:
            if self._parentwindow._currentwidget is not None:
                self._parentwindow._currentwidget.select(0)
            self._parentwindow._currentwidget = self
        else:
            self._parentwindow._currentwidget = None

    def key(self, char, event):
        pass

    def drawselframe(self, onoff):
        if not self._parentwindow._hasselframes:
            return
        App.DrawThemeFocusRect(self._bounds, onoff)

    def adjust(self, oldbounds):
        self.SetPort()
        if self._selected:
            self.GetWindow().InvalWindowRect(Qd.InsetRect(oldbounds, -3, -3))
            self.GetWindow().InvalWindowRect(Qd.InsetRect(self._bounds, -3, -3))
        else:
            self.GetWindow().InvalWindowRect(oldbounds)
            self.GetWindow().InvalWindowRect(self._bounds)


class _Line(Widget):

    def __init__(self, possize, thickness = 1):
        Widget.__init__(self, possize)
        self._thickness = thickness

    def open(self):
        self._calcbounds()
        self.SetPort()
        self.draw()

    def draw(self, visRgn = None):
        if self._visible:
            Qd.PaintRect(self._bounds)

    def _drawbounds(self):
        pass

class HorizontalLine(_Line):

    def _calcbounds(self):
        Widget._calcbounds(self)
        l, t, r, b = self._bounds
        self._bounds = l, t, r, t + self._thickness

class VerticalLine(_Line):

    def _calcbounds(self):
        Widget._calcbounds(self)
        l, t, r, b = self._bounds
        self._bounds = l, t, l + self._thickness, b


class Frame(Widget):

    def __init__(self, possize, pattern = Qd.GetQDGlobalsBlack(), color = (0, 0, 0)):
        Widget.__init__(self, possize)
        self._framepattern = pattern
        self._framecolor = color

    def setcolor(self, color):
        self._framecolor = color
        self.SetPort()
        self.draw()

    def setpattern(self, pattern):
        self._framepattern = pattern
        self.SetPort()
        self.draw()

    def draw(self, visRgn = None):
        if self._visible:
            penstate = Qd.GetPenState()
            Qd.PenPat(self._framepattern)
            Qd.RGBForeColor(self._framecolor)
            Qd.FrameRect(self._bounds)
            Qd.RGBForeColor((0, 0, 0))
            Qd.SetPenState(penstate)

def _darkencolor((r, g, b)):
    return int(0.75 * r), int(0.75 * g), int(0.75 * b)

class BevelBox(Widget):

    """'Platinum' beveled rectangle."""

    def __init__(self, possize, color = (0xe000, 0xe000, 0xe000)):
        Widget.__init__(self, possize)
        self._color = color
        self._darkercolor = _darkencolor(color)

    def setcolor(self, color):
        self._color = color
        self.SetPort()
        self.draw()

    def draw(self, visRgn = None):
        if self._visible:
            l, t, r, b = Qd.InsetRect(self._bounds, 1, 1)
            Qd.RGBForeColor(self._color)
            Qd.PaintRect((l, t, r, b))
            Qd.RGBForeColor(self._darkercolor)
            Qd.MoveTo(l, b)
            Qd.LineTo(r, b)
            Qd.LineTo(r, t)
            Qd.RGBForeColor((0, 0, 0))


class Group(Widget):

    """A container for subwidgets"""


class HorizontalPanes(Widget):

    """Panes, a.k.a. frames. Works a bit like a group. Devides the widget area into "panes",
    which can be resized by the user by clicking and dragging between the subwidgets."""

    _direction = 1

    def __init__(self, possize, panesizes = None, gutter = 8):
        """panesizes should be a tuple of numbers. The length of the tuple is the number of panes,
        the items in the tuple are the relative sizes of these panes; these numbers should add up
        to 1 (the total size of all panes)."""
        Widget.__init__(self, possize)
        self._panesizes = panesizes
        self._gutter = gutter
        self._enabled = 1
        self.setuppanes()

    #def open(self):
    #       self.installbounds()
    #       ClickableWidget.open(self)

    def _calcbounds(self):
        # hmmm. It should not neccesary be override _calcbounds :-(
        self.installbounds()
        Widget._calcbounds(self)

    def setuppanes(self):
        panesizes = self._panesizes
        total = 0
        if panesizes is not None:
            #if len(self._widgets) <> len(panesizes):
            #       raise TypeError, 'number of widgets does not match number of panes'
            for panesize in panesizes:
                if not 0 < panesize < 1:
                    raise TypeError, 'pane sizes must be between 0 and 1, not including.'
                total = total + panesize
            if round(total, 4) <> 1.0:
                raise TypeError, 'pane sizes must add up to 1'
        else:
            # XXX does not work!
            step = 1.0 / len(self._widgets)
            panesizes = []
            for i in range(len(self._widgets)):
                panesizes.append(step)
        current = 0
        self._panesizes = []
        self._gutters = []
        for panesize in panesizes:
            if current:
                self._gutters.append(current)
            self._panesizes.append((current, current + panesize))
            current = current + panesize
        self.makepanebounds()

    def getpanesizes(self):
        return map(lambda (fr, to): to-fr,  self._panesizes)

    boundstemplate = "lambda width, height: (0, height * %s + %d, width, height * %s + %d)"

    def makepanebounds(self):
        halfgutter = self._gutter / 2
        self._panebounds = []
        for i in range(len(self._panesizes)):
            panestart, paneend = self._panesizes[i]
            boundsstring = self.boundstemplate % (repr(panestart), panestart and halfgutter,
                                            repr(paneend), (paneend <> 1.0) and -halfgutter)
            self._panebounds.append(eval(boundsstring))

    def installbounds(self):
        #self.setuppanes()
        for i in range(len(self._widgets)):
            w = self._widgets[i]
            w._possize = self._panebounds[i]
            #if hasattr(w, "setuppanes"):
            #       w.setuppanes()
            if hasattr(w, "installbounds"):
                w.installbounds()

    def rollover(self, point, onoff):
        if onoff:
            orgmouse = point[self._direction]
            halfgutter = self._gutter / 2
            l, t, r, b = self._bounds
            if self._direction:
                begin, end = t, b
            else:
                begin, end = l, r

            i = self.findgutter(orgmouse, begin, end)
            if i is None:
                SetCursor("arrow")
            else:
                SetCursor(self._direction and 'vmover' or 'hmover')

    def findgutter(self, orgmouse, begin, end):
        tolerance = max(4, self._gutter) / 2
        for i in range(len(self._gutters)):
            pos = begin + (end - begin) * self._gutters[i]
            if abs(orgmouse - pos) <= tolerance:
                break
        else:
            return
        return i

    def click(self, point, modifiers):
        # what a mess...
        orgmouse = point[self._direction]
        halfgutter = self._gutter / 2
        l, t, r, b = self._bounds
        if self._direction:
            begin, end = t, b
        else:
            begin, end = l, r

        i = self.findgutter(orgmouse, begin, end)
        if i is None:
            return

        pos = orgpos = begin + (end - begin) * self._gutters[i] # init pos too, for fast click on border, bug done by Petr

        minpos = self._panesizes[i][0]
        maxpos = self._panesizes[i+1][1]
        minpos = begin + (end - begin) * minpos + 64
        maxpos = begin + (end - begin) * maxpos - 64
        if minpos > orgpos and maxpos < orgpos:
            return

        #SetCursor("fist")
        self.SetPort()
        if self._direction:
            rect = l, orgpos - 1, r, orgpos
        else:
            rect = orgpos - 1, t, orgpos, b

        # track mouse --- XXX  move to separate method?
        Qd.PenMode(QuickDraw.srcXor)
        Qd.PenPat(Qd.GetQDGlobalsGray())
        Qd.PaintRect(_intRect(rect))
        lastpos = None
        while Evt.Button():
            pos = orgpos - orgmouse + Evt.GetMouse()[self._direction]
            pos = max(pos, minpos)
            pos = min(pos, maxpos)
            if pos == lastpos:
                continue
            Qd.PenPat(Qd.GetQDGlobalsGray())
            Qd.PaintRect(_intRect(rect))
            if self._direction:
                rect = l, pos - 1, r, pos
            else:
                rect = pos - 1, t, pos, b
            Qd.PenPat(Qd.GetQDGlobalsGray())
            Qd.PaintRect(_intRect(rect))
            lastpos = pos
            self._parentwindow.wid.GetWindowPort().QDFlushPortBuffer(None)
            Evt.WaitNextEvent(0, 3)
        Qd.PaintRect(_intRect(rect))
        Qd.PenNormal()
        SetCursor("watch")

        newpos = (pos - begin) / float(end - begin)
        self._gutters[i] = newpos
        self._panesizes[i] = self._panesizes[i][0], newpos
        self._panesizes[i+1] = newpos, self._panesizes[i+1][1]
        self.makepanebounds()
        self.installbounds()
        self._calcbounds()


class VerticalPanes(HorizontalPanes):
    """see HorizontalPanes"""
    _direction = 0
    boundstemplate = "lambda width, height: (width * %s + %d, 0, width * %s + %d, height)"


class ColorPicker(ClickableWidget):

    """Color picker widget. Allows the user to choose a color."""

    def __init__(self, possize, color = (0, 0, 0), callback = None):
        ClickableWidget.__init__(self, possize)
        self._color = color
        self._callback = callback
        self._enabled = 1

    def click(self, point, modifiers):
        if not self._enabled:
            return
        import ColorPicker
        newcolor, ok = ColorPicker.GetColor("", self._color)
        if ok:
            self._color = newcolor
            self.SetPort()
            self.draw()
            if self._callback:
                return CallbackCall(self._callback, 0, self._color)

    def set(self, color):
        self._color = color
        self.SetPort()
        self.draw()

    def get(self):
        return self._color

    def draw(self, visRgn=None):
        if self._visible:
            if not visRgn:
                visRgn = self._parentwindow.wid.GetWindowPort().visRgn
            Qd.PenPat(Qd.GetQDGlobalsGray())
            rect = self._bounds
            Qd.FrameRect(rect)
            rect = Qd.InsetRect(rect, 3, 3)
            Qd.PenNormal()
            Qd.RGBForeColor(self._color)
            Qd.PaintRect(rect)
            Qd.RGBForeColor((0, 0, 0))


# misc utils

def CallbackCall(callback, mustfit, *args):
    """internal helper routine for W"""
    # XXX this function should die.
    if type(callback) == FunctionType:
        func = callback
        maxargs = func.func_code.co_argcount
    elif type(callback) == MethodType:
        func = callback.im_func
        maxargs = func.func_code.co_argcount - 1
    else:
        if callable(callback):
            return apply(callback, args)
        else:
            raise TypeError, "uncallable callback object"

    if func.func_defaults:
        minargs = maxargs - len(func.func_defaults)
    else:
        minargs = maxargs
    if minargs <= len(args) <= maxargs:
        return apply(callback, args)
    elif not mustfit and minargs == 0:
        return callback()
    else:
        if mustfit:
            raise TypeError, "callback accepts wrong number of arguments: %r" % len(args)
        else:
            raise TypeError, "callback accepts wrong number of arguments: 0 or %r" % len(args)


def HasBaseClass(obj, class_):
    try:
        raise obj
    except class_:
        return 1
    except:
        pass
    return 0


#
# To remove the dependence of Widgets.rsrc we hardcode the cursor
# data below.
#_cursors = {
#       "watch" : Qd.GetCursor(QuickDraw.watchCursor).data,
#       "arrow" : Qd.GetQDGlobalsArrow(),
#       "iBeam" : Qd.GetCursor(QuickDraw.iBeamCursor).data,
#       "cross" : Qd.GetCursor(QuickDraw.crossCursor).data,
#       "plus"          : Qd.GetCursor(QuickDraw.plusCursor).data,
#       "hand"  : Qd.GetCursor(468).data,
#       "fist"          : Qd.GetCursor(469).data,
#       "hmover"        : Qd.GetCursor(470).data,
#       "vmover"        : Qd.GetCursor(471).data,
#       "zoomin"        : Qd.GetCursor(472).data,
#       "zoomout"       : Qd.GetCursor(473).data,
#       "zoom"  : Qd.GetCursor(474).data,
#}

_cursors = {
        'arrow':
        '\x00\x00\x40\x00\x60\x00\x70\x00\x78\x00\x7c\x00\x7e\x00\x7f\x00'
        '\x7f\x80\x7c\x00\x6c\x00\x46\x00\x06\x00\x03\x00\x03\x00\x00\x00'
        '\xc0\x00\xe0\x00\xf0\x00\xf8\x00\xfc\x00\xfe\x00\xff\x00\xff\x80'
        '\xff\xc0\xff\xe0\xfe\x00\xef\x00\xcf\x00\x87\x80\x07\x80\x03\x80'
        '\x00\x01\x00\x01',
        'cross':
        '\x04\x00\x04\x00\x04\x00\x04\x00\x04\x00\xff\xe0\x04\x00\x04\x00'
        '\x04\x00\x04\x00\x04\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        '\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        '\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
        '\x00\x05\x00\x05',
        'fist':
        '\x00\x00\x00\x00\x0d\x80\x12\x70\x12\x4c\x12\x4a\x28\x0a\x28\x02'
        '\x48\x02\x40\x02\x20\x02\x20\x04\x10\x04\x08\x08\x04\x08\x04\x08'
        '\x00\x00\x00\x00\x0d\x80\x1f\xf0\x1f\xfc\x1f\xfe\x3f\xfe\x3f\xfe'
        '\x7f\xfe\x7f\xfe\x3f\xfe\x3f\xfc\x1f\xfc\x0f\xf8\x07\xf8\x07\xf8'
        '\x00\x09\x00\x08',
        'hand':
        '\x01\x80\x1a\x70\x26\x48\x26\x4a\x12\x4d\x12\x49\x68\x09\x98\x01'
        '\x88\x02\x40\x02\x20\x02\x20\x04\x10\x04\x08\x08\x04\x08\x04\x08'
        '\x01\x80\x1b\xf0\x3f\xf8\x3f\xfa\x1f\xff\x1f\xff\x6f\xff\xff\xff'
        '\xff\xfe\x7f\xfe\x3f\xfe\x3f\xfc\x1f\xfc\x0f\xf8\x07\xf8\x07\xf8'
        '\x00\x09\x00\x08',
        'hmover':
        '\x00\x00\x01\x80\x01\x80\x01\x80\x01\x80\x11\x88\x31\x8c\x7f\xfe'
        '\x31\x8c\x11\x88\x01\x80\x01\x80\x01\x80\x01\x80\x00\x00\x00\x00'
        '\x03\xc0\x03\xc0\x03\xc0\x03\xc0\x1b\xd8\x3b\xdc\x7f\xfe\xff\xff'
        '\x7f\xfe\x3b\xdc\x1b\xd8\x03\xc0\x03\xc0\x03\xc0\x03\xc0\x00\x00'
        '\x00\x07\x00\x07',
        'iBeam':
        '\x0c\x60\x02\x80\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00'
        '\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x02\x80\x0c\x60'
        '\x0c\x60\x02\x80\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00'
        '\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x02\x80\x0c\x60'
        '\x00\x04\x00\x07',
        'plus':
        '\x00\x00\x07\xc0\x04\x60\x04\x60\x04\x60\x7c\x7c\x43\x86\x42\x86'
        '\x43\x86\x7c\x7e\x3c\x7e\x04\x60\x04\x60\x07\xe0\x03\xe0\x00\x00'
        '\x0f\xc0\x0f\xe0\x0f\xf0\x0f\xf0\xff\xff\xff\xfe\xfc\x7f\xfc\x7f'
        '\xfc\x7f\xff\xff\x7f\xff\x7f\xff\x0f\xf0\x0f\xf0\x07\xf0\x03\xe0'
        '\x00\x08\x00\x08',
        'vmover':
        '\x00\x00\x01\x00\x03\x80\x07\xc0\x01\x00\x01\x00\x01\x00\x7f\xfc'
        '\x7f\xfc\x01\x00\x01\x00\x01\x00\x07\xc0\x03\x80\x01\x00\x00\x00'
        '\x01\x00\x03\x80\x07\xc0\x0f\xe0\x0f\xe0\x03\x80\xff\xfe\xff\xfe'
        '\xff\xfe\xff\xfe\x03\x80\x0f\xe0\x0f\xe0\x07\xc0\x03\x80\x01\x00'
        '\x00\x07\x00\x07',
        'watch':
        '\x3f\x00\x3f\x00\x3f\x00\x3f\x00\x40\x80\x84\x40\x84\x40\x84\x60'
        '\x9c\x60\x80\x40\x80\x40\x40\x80\x3f\x00\x3f\x00\x3f\x00\x3f\x00'
        '\x3f\x00\x3f\x00\x3f\x00\x3f\x00\x7f\x80\xff\xc0\xff\xc0\xff\xc0'
        '\xff\xc0\xff\xc0\xff\xc0\x7f\x80\x3f\x00\x3f\x00\x3f\x00\x3f\x00'
        '\x00\x08\x00\x08',
        'zoom':
        '\x0f\x00\x30\xc0\x40\x20\x40\x20\x80\x10\x80\x10\x80\x10\x80\x10'
        '\x40\x20\x40\x20\x30\xf0\x0f\x38\x00\x1c\x00\x0e\x00\x07\x00\x02'
        '\x0f\x00\x3f\xc0\x7f\xe0\x7f\xe0\xff\xf0\xff\xf0\xff\xf0\xff\xf0'
        '\x7f\xe0\x7f\xe0\x3f\xf0\x0f\x38\x00\x1c\x00\x0e\x00\x07\x00\x02'
        '\x00\x06\x00\x06',
        'zoomin':
        '\x0f\x00\x30\xc0\x40\x20\x46\x20\x86\x10\x9f\x90\x9f\x90\x86\x10'
        '\x46\x20\x40\x20\x30\xf0\x0f\x38\x00\x1c\x00\x0e\x00\x07\x00\x02'
        '\x0f\x00\x3f\xc0\x7f\xe0\x7f\xe0\xff\xf0\xff\xf0\xff\xf0\xff\xf0'
        '\x7f\xe0\x7f\xe0\x3f\xf0\x0f\x38\x00\x1c\x00\x0e\x00\x07\x00\x02'
        '\x00\x06\x00\x06',
        'zoomout':
        '\x0f\x00\x30\xc0\x40\x20\x40\x20\x80\x10\x9f\x90\x9f\x90\x80\x10'
        '\x40\x20\x40\x20\x30\xf0\x0f\x38\x00\x1c\x00\x0e\x00\x07\x00\x02'
        '\x0f\x00\x3f\xc0\x7f\xe0\x7f\xe0\xff\xf0\xff\xf0\xff\xf0\xff\xf0'
        '\x7f\xe0\x7f\xe0\x3f\xf0\x0f\x38\x00\x1c\x00\x0e\x00\x07\x00\x02'
        '\x00\x06\x00\x06',
}

def SetCursor(what):
    """Set the cursorshape to any of these: 'arrow', 'cross', 'fist', 'hand', 'hmover', 'iBeam',
    'plus', 'vmover', 'watch', 'zoom', 'zoomin', 'zoomout'."""
    Qd.SetCursor(_cursors[what])
