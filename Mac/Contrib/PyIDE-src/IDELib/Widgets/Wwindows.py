import Qd
import Win
import Evt
import Fm
import FrameWork
import Windows
import Events
import Wbase
import Wlist
import Dlg
import MacOS
import Menu
import struct

from types import *
from SpecialKeys import *


class Window(FrameWork.Window, Wbase.SelectableWidget):
	
	windowkind = Windows.documentProc
	
	def __init__(self, possize, title = "", minsize = None, maxsize = None, tabbable = 1, show = 1):
		Wbase.SelectableWidget.__init__(self, possize)
		self._globalbounds = l, t, r, b = self.getwindowbounds(possize, minsize)
		self._bounds = (0, 0, r - l, b - t)
		self._tabchain = []
		self._currentwidget = None
		self.title = title
		self._parentwindow = self
		self._tabbable = tabbable
		self._defaultbutton = None
		self._drawwidgetbounds = 0
		self._show = show
		self._lastrollover = None
		if minsize:
			self._hasgrowbox = 1
			self.windowkind = self.windowkind | 8
			l, t = minsize
			if maxsize:
				r, b = maxsize[0] + 1, maxsize[1] + 1
			else:
				r, b = 32000, 32000
			self.growlimit = (l, t, r, b)
		else:
			self._hasgrowbox = 0
			if self.windowkind == 0 or self.windowkind >=8:
				self.windowkind = self.windowkind | 4
		import W
		if not W._application:
			raise W.WidgetsError, 'W not properly initialized: unknown Application'
		FrameWork.Window.__init__(self, W._application)
	
	def gettitle(self):
		return self.title
	
	def settitle(self, title):
		self.title = title
		if self.wid:
			self.wid.SetWTitle(title)
	
	def getwindowbounds(self, size, minsize = None):
		return windowbounds(size, minsize)	
	
	def getcurrentwidget(self):
		return self._currentwidget
	
	def show(self, onoff):
		if onoff:
			self.wid.ShowWindow()
		else:
			self.wid.HideWindow()
	
	def isvisible(self):
		return self.wid.IsWindowVisible()
	
	def getbounds(self):
		if 0:	#self.isvisible():
			self.wid.GetWindowContentRgn(scratchRegion)
			self._globalbounds = GetRgnBounds(scratchRegion)
		return self._globalbounds
		
	def select(self):
		self.wid.SelectWindow()
	
	def open(self):
		self.wid = Win.NewCWindow(self._globalbounds, self.title, self._show,
			self.windowkind, -1, 1, 0)
		self.SetPort()
		fnum = Fm.GetFNum("Python-Sans")
		if fnum == 0:
			fnum = Fm.GetFNum("Geneva")
		Qd.TextFont(fnum)	# XXX font&size from a function?
		Qd.TextSize(9)	# XXX font&size from a function?
		if self._bindings.has_key("<open>"):
			callback = self._bindings["<open>"]
			callback()
		for w in self._widgets:
			w.forall_frombottom("open")
		self._maketabchain()
		if self._tabchain:
			self._tabchain[0].select(1)
		if self._tabbable:
			self.bind('tab', self.nextwidget)
			self.bind('shifttab', self.previouswidget)
		self.do_postopen()
	
	def close(self):
		if not self.wid:
			return	# we are already closed
		if self._bindings.has_key("<close>"):
			callback = self._bindings["<close>"]
			rv = callback()
			if rv:
				return rv
		#for key in self._widgetsdict.keys():
		#	self._removewidget(key)
		self.forall_butself("close")
		Wbase.SelectableWidget.close(self)
		self._tabchain = []
		self._currentwidget = None
		self.wid.HideWindow()
		self.do_postclose()
	
	def domenu_close(self, *args):
		self.close()
	
	def move(self, x, y = None):
		"""absolute move"""
		if y == None:
			x, y = x
		self.wid.MoveWindow(x, y, 0)
	
	def resize(self, x, y = None):
		if y == None:
			x, y = x
		if self._hasgrowbox:
			self.SetPort()
			Win.InvalRect(self.getgrowrect())
		self.wid.SizeWindow(x, y, 1)
		self._calcbounds()
	
	def test(self, point):
		return 1
	
	def draw(self, visRgn = None):
		if self._hasgrowbox:
			self.tempcliprect(self.getgrowrect())
			self.wid.DrawGrowIcon()
			self.restoreclip()
	
	def idle(self, *args):
		self.SetPort()
		point = Evt.GetMouse()
		widget = self.findwidget(point, 0)
		if widget is not None and hasattr(widget, "rollover"):
			if 1:	#self._lastrollover <> widget:
				if self._lastrollover:
					self._lastrollover.rollover(point, 0)
				self._lastrollover = widget
				self._lastrollover.rollover(point, 1)
		else:
			if self._lastrollover:
				self._lastrollover.rollover(point, 0)
			self._lastrollover = None
			Wbase.SetCursor("arrow")
		if self._bindings.has_key("<idle>"):
			callback = self._bindings["<idle>"]
			if callback():
				return
		if self._currentwidget is not None and hasattr(self._currentwidget, "idle"):
			if self._currentwidget._bindings.has_key("<idle>"):
				callback = self._currentwidget._bindings["<idle>"]
				if callback():
					return
			self._currentwidget.idle()

	def xxx___select(self, widget):
		if self._currentwidget == widget:
			return
		if self._bindings.has_key("<select>"):
			callback = self._bindings["<select>"]
			if callback(widget):
				return
		if widget is None:
			if self._currentwidget is not None:
				self._currentwidget.select(0)
		elif type(widget) == InstanceType and widget._selectable:
			widget.select(1)
		elif widget == -1 or widget == 1:
			if len(self._tabchain) <= 1:
				return
			temp = self._tabchain[(self._tabchain.index(self._currentwidget) + widget) % len(self._tabchain)]
			temp.select(1)
		else:
			raise TypeError, "Widget is not selectable"
	
	def setdefaultbutton(self, newdefaultbutton = None, *keys):
		if newdefaultbutton == self._defaultbutton:
			return
		if self._defaultbutton:
			self._defaultbutton._setdefault(0)
		if not newdefaultbutton:
			self.bind("return", None)
			self.bind("enter", None)
			return
		import Wcontrols
		if not Wbase.HasBaseClass(newdefaultbutton, Wcontrols.Button):
			raise TypeError, "widget is not a button"
		self._defaultbutton = newdefaultbutton
		self._defaultbutton._setdefault(1)
		if not keys:
			self.bind("return", self._defaultbutton.push)
			self.bind("enter", self._defaultbutton.push)
		else:
			for key in keys:
				self.bind(key, self._defaultbutton.push)
	
	def nextwidget(self):
		self.xxx___select(1)
	
	def previouswidget(self):
		self.xxx___select(-1)
	
	def drawwidgetbounds(self, onoff):
		self._drawwidgetbounds = onoff
		self.SetPort()
		Win.InvalRect(self._bounds)
	
	def _drawbounds(self):
		pass

	def _maketabchain(self):
		# XXX This has to change, it's no good when we are adding or deleting widgets.
		# XXX Perhaps we shouldn't keep a "tabchain" at all.
		self._hasselframes = 0
		self._collectselectablewidgets(self._widgets)
		if self._hasselframes and len(self._tabchain) > 1:
			self._hasselframes = 1
		else:
			self._hasselframes = 0
	
	def _collectselectablewidgets(self, widgets):
		for w in widgets:
			if w._selectable:
				self._tabchain.append(w)
				if Wbase.HasBaseClass(w, Wlist.List):
					self._hasselframes = 1
			self._collectselectablewidgets(w._widgets)
	
	def _calcbounds(self):
		self._possize = self.wid.GetWindowPort().portRect[2:]
		w, h = self._possize
		self._bounds = (0, 0, w, h)
		self.wid.GetWindowContentRgn(scratchRegion)
		l, t, r, b = GetRgnBounds(scratchRegion)
		self._globalbounds = l, t, l + w, t + h
		for w in self._widgets:
			w._calcbounds()
	
	# FrameWork override methods
	def do_inDrag(self, partcode, window, event):
		where = event[3]
		self.wid.GetWindowContentRgn(scratchRegion)
		was_l, was_t, r, b = GetRgnBounds(scratchRegion)
		window.DragWindow(where, self.draglimit)
		self.wid.GetWindowContentRgn(scratchRegion)
		is_l, is_t, r, b = GetRgnBounds(scratchRegion)
		self._globalbounds = Qd.OffsetRect(self._globalbounds, 
					is_l - was_l, is_t - was_t)
	
	def do_char(self, char, event):
		(what, message, when, where, modifiers) = event
		key = char
		if keynames.has_key(key):
			key = keynames[char]
		if modifiers & Events.shiftKey:
			key = 'shift' + key
		if modifiers & Events.cmdKey:
			key = 'cmd' + key
		if modifiers & Events.controlKey:
			key = 'control' + key
		if self._bindings.has_key("<key>"):
			callback = self._bindings["<key>"]
			if Wbase.CallbackCall(callback, 0, char, event):
				return
		if self._bindings.has_key(key):
			callback = self._bindings[key]
			Wbase.CallbackCall(callback, 0, char, event)
		elif self._currentwidget is not None:
			if self._currentwidget._bindings.has_key(key):
				callback = self._currentwidget._bindings[key]
				Wbase.CallbackCall(callback, 0, char, event)
			else:
				if self._currentwidget._bindings.has_key("<key>"):
					callback = self._currentwidget._bindings["<key>"]
					if Wbase.CallbackCall(callback, 0, char, event):
						return
				self._currentwidget.key(char, event)
	
	def do_contentclick(self, point, modifiers, event):
		widget = self.findwidget(point)
		if widget is not None:
			if self._bindings.has_key("<click>"):
				callback = self._bindings["<click>"]
				if Wbase.CallbackCall(callback, 0, point, modifiers):
					return
			if widget._bindings.has_key("<click>"):
				callback = widget._bindings["<click>"]
				if Wbase.CallbackCall(callback, 0, point, modifiers):
					return
			if widget._selectable:
				widget.select(1, 1)
			widget.click(point, modifiers)
	
	def do_update(self, window, event):
		Qd.EraseRgn(window.GetWindowPort().visRgn)
		self.forall("draw", window.GetWindowPort().visRgn)
		if self._drawwidgetbounds:
			self.forall("_drawbounds")
	
	def do_activate(self, onoff, event):
		if not onoff:
			if self._lastrollover:
				self._lastrollover.rollover((0, 0), 0)
				self._lastrollover = None
		self.SetPort()
		self.forall("activate", onoff)
		self.draw()
	
	def do_postresize(self, width, height, window):
		Win.InvalRect(self.getgrowrect())
		self._calcbounds()
	
	# utilities
	def tempcliprect(self, tempcliprect):
		tempclip = Qd.NewRgn()
		Qd.RectRgn(tempclip, tempcliprect)
		self.tempclip(tempclip)
	
	def tempclip(self, tempclip):
		if hasattr(self, "saveclip"):
			raise Wbase.WidgetsError, "already have saveclip"
		self.saveclip = Qd.NewRgn()
		Qd.GetClip(self.saveclip)
		Qd.SectRgn(self.wid.GetWindowPort().visRgn, tempclip, tempclip)
		Qd.SetClip(tempclip)
	
	def restoreclip(self):
		Qd.SetClip(self.saveclip)
		del self.saveclip
	
	def getgrowrect(self):
		l, t, r, b = self.wid.GetWindowPort().portRect
		return (r - 15, b - 15, r, b)
	
	def has_key(self, key):
		return self._widgetsdict.has_key(key)
	
	def __getattr__(self, attr):
		global _successcount, _failcount, _magiccount
		if self._widgetsdict.has_key(attr):
			_successcount = _successcount + 1
			return self._widgetsdict[attr]
		if self._currentwidget is None or (attr[:7] <> 'domenu_' and 
				attr[:4] <> 'can_' and attr <> 'insert'):
			_failcount = _failcount + 1
			raise AttributeError, attr
		# special case: if a domenu_xxx, can_xxx or insert method is asked for, 
		# see if the active widget supports it
		_magiccount = _magiccount + 1
		return getattr(self._currentwidget, attr)

_successcount = 0
_failcount = 0
_magiccount = 0

class Dialog(Window):
	
	windowkind = Windows.movableDBoxProc
	
	# this __init__ seems redundant, but it's not: it has less args
	def __init__(self, possize, title = ""):
		Window.__init__(self, possize, title)
	
	def can_close(self, *args):
		return 0
	
	def getwindowbounds(self, size, minsize = None):
		screenbounds = sl, st, sr, sb = Qd.qd.screenBits.bounds
		w, h = size
		l = sl + (sr - sl - w) / 2
		t = st + (sb - st - h) / 3
		return l, t, l + w, t + h
	
class ModalDialog(Dialog):
	
	def __init__(self, possize, title = ""):
		Dialog.__init__(self, possize, title)
		if title:
			self.windowkind = Windows.movableDBoxProc
		else:
			self.windowkind = Windows.dBoxProc
	
	def open(self):
		import W
		if not W._application:
			raise W.WidgetsError, 'W not properly initialized: unknown Application'
		Dialog.open(self)
		self.app = W._application
		self.done = 0
		Menu.HiliteMenu(0)
		self.mainloop()
	
	def close(self):
		if not self.wid:
			return	# we are already closed
		self.done = 1
		del self.app
		Dialog.close(self)
	
	def mainloop(self):
		saveyield = MacOS.EnableAppswitch(-1)
		while not self.done:
			#self.do1event()
			self.do1event(	Events.keyDownMask + 
						Events.autoKeyMask + 
						Events.activMask + 
						Events.updateMask + 
						Events.mDownMask +
						Events.mUpMask, 
						10)
		MacOS.EnableAppswitch(saveyield)
	
	def do1event(self, mask = Events.everyEvent, wait = 0):
		ok, event = self.app.getevent(mask, wait)
		if Dlg.IsDialogEvent(event):
			if self.app.do_dialogevent(event):
				return
		if ok:
			self.dispatch(event)
		else:
			self.app.idle(event)
	
	def do_keyDown(self, event):
		self.do_key(event)
	
	def do_autoKey(self, event):
		if not event[-1] & Events.cmdKey:
			self.do_key(event)
	
	def do_key(self, event):
		(what, message, when, where, modifiers) = event
		w = Win.FrontWindow()
		if w <> self.wid:
			return
		c = chr(message & Events.charCodeMask)
		if modifiers & Events.cmdKey:
			self.app.checkmenus(self)
			result = Menu.MenuKey(ord(c))
			id = (result>>16) & 0xffff	# Hi word
			item = result & 0xffff		# Lo word
			if id:
				self.app.do_rawmenu(id, item, None, event)
				return
		self.do_char(c, event)
	
	def do_mouseDown(self, event):
		(what, message, when, where, modifiers) = event
		partcode, wid = Win.FindWindow(where)
		#
		# Find the correct name.
		#
		if FrameWork.partname.has_key(partcode):
			name = "do_" + FrameWork.partname[partcode]
		else:
			name = "do_%d" % partcode
		
		if name == "do_inDesk":
			MacOS.HandleEvent(event)
			return
		if wid == self.wid:
			try:
				handler = getattr(self, name)
			except AttributeError:
				handler = self.app.do_unknownpartcode
		else:
			#MacOS.HandleEvent(event)
			return		
		handler(partcode, wid, event)
	
	def dispatch(self, event):
		(what, message, when, where, modifiers) = event
		if FrameWork.eventname.has_key(what):
			name = "do_" + FrameWork.eventname[what]
		else:
			name = "do_%d" % what
		try:
			handler = getattr(self, name)
		except AttributeError:
			try:
				handler = getattr(self.app, name)
			except AttributeError:
				handler = self.app.do_unknownevent
		handler(event)
	

def FrontWindowInsert(stuff):
	if not stuff:
		return
	if type(stuff) <> StringType:
		raise TypeError, 'string expected'
	import W
	app = W.getapplication()
	wid = Win.FrontWindow()
	if wid and app._windows.has_key(wid):
		window = app._windows[wid]
		if hasattr(window, "insert"):
			try:
				window.insert(stuff)
				return
			except:
				pass
	import EasyDialogs
	if EasyDialogs.AskYesNoCancel(
			"Can¹t find window or widget to insert text into; copy to clipboard instead?", 
			1) == 1:
		import Scrap
		Scrap.ZeroScrap()
		Scrap.PutScrap('TEXT', stuff)


# not quite based on the same function in FrameWork	
_windowcounter = 0

def getnextwindowpos():
	global _windowcounter
	rows = 8
	l = 4 * (rows + 1 - (_windowcounter % rows) + _windowcounter / rows)
	t = 44 + 20 * (_windowcounter % rows)
	_windowcounter = _windowcounter + 1
	return l, t

def windowbounds(preferredsize, minsize = None):
	"Return sensible window bounds"
	
	global _windowcounter
	if len(preferredsize) == 4:
		bounds = l, t, r, b = preferredsize
		union = Qd.UnionRect(bounds, Qd.qd.screenBits.bounds)
		if union == Qd.qd.screenBits.bounds:
			return bounds
		else:
			preferredsize = r - l, b - t
	if not minsize:
		minsize = preferredsize
	minwidth, minheight = minsize
	width, height = preferredsize
	
	sl, st, sr, sb = screenbounds = Qd.InsetRect(Qd.qd.screenBits.bounds, 4, 4)
	l, t = getnextwindowpos()
	if (l + width) > sr:
		_windowcounter = 0
		l, t = getnextwindowpos()
	r = l + width
	b = t + height
	if (t + height) > sb:
		b = sb
		if (b - t) < minheight:
			b = t + minheight
	return l, t, r, b

scratchRegion = Qd.NewRgn()

# util -- move somewhere convenient???
def GetRgnBounds(the_Rgn):
	(t, l, b, r) = struct.unpack("hhhh", the_Rgn.data[2:10])
	return (l, t, r, b)
