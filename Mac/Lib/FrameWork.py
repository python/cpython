"A sort of application framework for the Mac"

DEBUG=0

import MacOS
import traceback

from addpack import addpack
addpack('Tools')
addpack('bgen')
addpack('ae')
addpack('ctl')
addpack('dlg')
addpack('evt')
addpack('menu')
addpack('qd')
#addpack('res')
#addpack('snd')
addpack('win')

from AE import *
from AppleEvents import *
from Ctl import *
from Controls import *
from Dlg import *
from Dialogs import *
from Evt import *
from Events import *
from Menu import *
from Menus import *
from Qd import *
from QuickDraw import *
#from Res import *
#from Resources import *
#from Snd import *
#from Sound import *
from Win import *
from Windows import *

import EasyDialogs

kHighLevelEvent = 23	# Don't know what header file this should come from


# Map event 'what' field to strings
eventname = {}
eventname[1] = 'mouseDown'
eventname[2] = 'mouseUp'
eventname[3] = 'keyDown'
eventname[4] = 'keyUp'
eventname[5] = 'autoKey'
eventname[6] = 'updateEvt'
eventname[7] = 'diskEvt'
eventname[8] = 'activateEvt'
eventname[15] = 'osEvt'
eventname[23] = 'kHighLevelEvent'

# Map part codes returned by WhichWindow() to strings
partname = {}
partname[0] = 'inDesk'
partname[1] = 'inMenuBar'
partname[2] = 'inSysWindow'
partname[3] = 'inContent'
partname[4] = 'inDrag'
partname[5] = 'inGrow'
partname[6] = 'inGoAway'
partname[7] = 'inZoomIn'
partname[8] = 'inZoomOut'

# A rectangle that's bigger than the screen,
# but not so big that adding the screen size to it will cause 16-bit overflow
everywhere = (-16000, -16000, 16000, 16000)


class Application:
	
	"Application framework -- your application should be a derived class"
	
	def __init__(self):
		self.makemenubar()
	
	def makemenubar(self):
		self.menubar = MenuBar()
		AppleMenu(self.menubar, self.getabouttext(), self.do_about)
		self.makeusermenus()
	
	def getabouttext(self):
		return "About %s..." % self.__class__.__name__
	
	def do_about(self, id, item, window, event):
		EasyDialogs.Message("Hello, world!" + "\015(%s)" % self.__class__.__name__)
	
	# The main event loop is broken up in several simple steps.
	# This is done so you can override each individual part,
	# if you have a need to do extra processing independent of the
	# event type.
	# Normally, however, you'd just define handlers for individual
	# events.
	# (XXX I'm not sure if using default parameter values is the right
	# way to define the mask and wait time passed to WaitNextEvent.)
	
	def mainloop(self, mask = everyEvent, wait = 0):
		saveyield = MacOS.EnableAppswitch(self.yield)
		try:
			while 1:
				try:
					self.do1event(mask, wait)
				except (Application, SystemExit):
					break
		finally:
			MacOS.EnableAppswitch(saveyield)
	
	yield = -1
	
	def do1event(self, mask = everyEvent, wait = 0):
		event = self.getevent(mask, wait)
		if event:
			self.dispatch(event)
	
	def getevent(self, mask = everyEvent, wait = 0):
		ok, event = WaitNextEvent(mask, wait)
		if ok:
			return event
		else:
			return None
	
	def dispatch(self, event):
		(what, message, when, where, modifiers) = event
		if eventname.has_key(what):
			name = "do_" + eventname[what]
		else:
			name = "do_%d" % what
		try:
			handler = getattr(self, name)
		except AttributeError:
			handler = self.do_unknownevent
		handler(event)
	
	def do_mouseDown(self, event):
		(what, message, when, where, modifiers) = event
		partcode, window = FindWindow(where)
		if partname.has_key(partcode):
			name = "do_" + partname[partcode]
		else:
			name = "do_%d" % partcode
		try:
			handler = getattr(self, name)
		except AttributeError:
			handler = self.do_unknownpartcode
		handler(partcode, window, event)
	
	def do_inDrag(self, partcode, window, event):
		where = event[3]
		window.DragWindow(where, self.draglimit)
	
	draglimit = everywhere
	
	def do_inGoAway(self, partcode, window, event):
		where = event[3]
		if window.TrackGoAway(where):
			self.do_close(window)
	
	def do_close(self, window):
		if DEBUG: print "Should close window:", window
	
	def do_inZoom(self, partcode, window, event):
		(what, message, when, where, modifiers) = event
		if window.TrackBox(where, partcode):
			window.ZoomWindow(partcode, 1)
	
	def do_inZoomIn(self, partcode, window, event):
		SetPort(window) # !!!
		self.do_inZoom(partcode, window, event)
	
	def do_inZoomOut(self, partcode, window, event):
		SetPort(window) # !!!
		self.do_inZoom(partcode, window, event)
	
	def do_inSysWindow(self, partcode, window, event):
		MacOS.HandleEvent(event)
		# print "SystemClick", event, window
		# SystemClick(event, window) # XXX useless, window is None
	
	def do_inDesk(self, partcode, window, event):
		# print "inDesk"
		# XXX what to do with it?
		MacOS.HandleEvent(event)
	
	def do_inMenuBar(self, partcode, window, event):
		(what, message, when, where, modifiers) = event
		result = MenuSelect(where)
		id = (result>>16) & 0xffff	# Hi word
		item = result & 0xffff		# Lo word
		self.do_rawmenu(id, item, window, event)
	
	def do_rawmenu(self, id, item, window, event):
		try:
			self.do_menu(id, item, window, event)
		finally:
			HiliteMenu(0)
	
	def do_menu(self, id, item, window, event):
		self.menubar.dispatch(id, item, window, event)
	
	def do_inGrow(self, partcode, window, event):
		(what, message, when, where, modifiers) = event
		result = window.GrowWindow(where, self.growlimit)
		if result:
			height = (result>>16) & 0xffff	# Hi word
			width = result & 0xffff		# Lo word
			self.do_resize(width, height, window)
	
	growlimit = everywhere
	
	def do_resize(self, width, height, window):
		window.SizeWindow(width, height, 0)
		self.do_postresize(width, height, window)
	
	def do_postresize(self, width, height, window):
		SetPort(window)
		InvalRect(everywhere)
	
	def do_inContent(self, partcode, window, event):
		(what, message, when, where, modifiers) = event
		local = GlobalToLocal(where)
		ctltype, control = FindControl(local, window)
		if ctltype and control:
			pcode = control.TrackControl(local)
			if pcode:
				self.do_controlhit(window, control, pcode, event)
		else:
			if DEBUG: print "FindControl(%s, %s) -> (%s, %s)" % \
				(local, window, ctltype, control)
	
	def do_controlhit(self, window, control, pcode, event):
		if DEBUG: print "control hit in", window, "on", control, "; pcode =", pcode
	
	def do_unknownpartcode(self, partcode, window, event):
		(what, message, when, where, modifiers) = event
		if DEBUG: print "Mouse down at global:", where
		if DEBUG: print "\tUnknown part code:", partcode
		MacOS.HandleEvent(event)
	
	def do_keyDown(self, event):
		self.do_key(event)
	
	def do_autoKey(self, event):
		if not event[-1] & cmdKey:
			self.do_key(event)
	
	def do_key(self, event):
		(what, message, when, where, modifiers) = event
		c = chr(message & charCodeMask)
		if modifiers & cmdKey:
			if c == '.':
				raise self
			else:
				result = MenuKey(ord(c))
				id = (result>>16) & 0xffff	# Hi word
				item = result & 0xffff		# Lo word
				if id:
					self.do_rawmenu(id, item, None, event)
				elif c == 'w':
					w = FrontWindow()
					if w:
						self.do_close(w)
					else:
						if DEBUG: print 'Command-W without front window'
				else:
					if DEBUG: print "Command-" +`c`
		else:
			self.do_char(c, event)
	
	def do_char(self, c, event):
		if DEBUG: print "Character", `c`
	
	def do_updateEvt(self, event):
		if DEBUG: 
			print "do_update",
			self.printevent(event)
		window = FrontWindow() # XXX This is wrong!
		if window:
			self.do_rawupdate(window, event)
		else:
			MacOS.HandleEvent(event)
	
	def do_rawupdate(self, window, event):
		if DEBUG: print "raw update for", window
		window.BeginUpdate()
		self.do_update(window, event)
		DrawControls(window)
		window.DrawGrowIcon()
		window.EndUpdate()
	
	def do_update(self, window, event):
		EraseRect(everywhere)
	
	def do_kHighLevelEvent(self, event):
		(what, message, when, where, modifiers) = event
		if DEBUG: 
			print "High Level Event:",
			self.printevent(event)
		try:
			AEProcessAppleEvent(event)
		except:
			print "AEProcessAppleEvent error:"
			traceback.print_exc()
	
	def do_unknownevent(self, event):
		print "Unknown event:",
		self.printevent(event)
	
	def printevent(self, event):
		(what, message, when, where, modifiers) = event
		nicewhat = `what`
		if eventname.has_key(what):
			nicewhat = eventname[what]
		print nicewhat,
		if what == kHighLevelEvent:
			h, v = where
			print `ostypecode(message)`, hex(when), `ostypecode(h | (v<<16))`,
		else:
			print hex(message), hex(when), where,
		print hex(modifiers)


class MenuBar:
	"""Represent a set of menus in a menu bar.
	
	Interface:
	
	- (constructor)
	- (destructor)
	- addmenu
	- addpopup (normally used internally)
	- dispatch (called from Application)
	"""
	
	nextid = 1	# Necessarily a class variable
	
	def getnextid(self):
		id = self.nextid
		self.nextid = id+1
		return id
	
	def __init__(self):
		ClearMenuBar()
		self.bar = GetMenuBar()
		self.menus = {}
	
	def addmenu(self, title, after = 0):
		id = self.getnextid()
		m = NewMenu(id, title)
		m.InsertMenu(after)
		DrawMenuBar()
		return id, m
	
	def addpopup(self, title = ''):
		return self.addmenu(title, -1)
	
	def install(self):
		self.bar.SetMenuBar()
		DrawMenuBar()
	
	def dispatch(self, id, item, window, event):
		if self.menus.has_key(id):
			self.menus[id].dispatch(id, item, window, event)
		else:
			if DEBUG: print "MenuBar.dispatch(%d, %d, %s, %s)" % \
				(id, item, window, event)
	

# XXX Need a way to get menus as resources and bind them to callbacks

class Menu:
	"One menu."
	
	def __init__(self, bar, title, after=0):
		self.bar = bar
		self.id, self.menu = self.bar.addmenu(title, after)
		bar.menus[self.id] = self
		self.items = []
	
	def additem(self, label, shortcut=None, callback=None, kind=None):
		self.menu.AppendMenu('x')		# add a dummy string
		self.items.append(label, shortcut, callback, kind)
		item = len(self.items)
		self.menu.SetMenuItemText(item, label)		# set the actual text
		if shortcut:
			self.menu.SetItemCmd(item, ord(shortcut))
		return item
	
	def addcheck(self, label, shortcut=None, callback=None):
		return self.additem(label, shortcut, callback, 'check')
	
	def addradio(self, label, shortcut=None, callback=None):
		return self.additem(label, shortcut, callback, 'radio')
	
	def addseparator(self):
		self.menu.AppendMenu('(-')
		self.items.append('', None, None, 'separator')
	
	def addsubmenu(self, label, title=''):
		sub = Menu(self.bar, title, -1)
		item = self.additem(label, '\x1B', None, 'submenu')
		self.menu.SetItemMark(item, sub.id)
		return sub
	
	def dispatch(self, id, item, window, event):
		title, shortcut, callback, type = self.items[item-1]
		if callback:
			callback(id, item, window, event)


class MenuItem:
	def __init__(self, menu, title, shortcut=None, callback=None, kind=None):
		self.item = menu.additem(title, shortcut, callback)

class RadioItem(MenuItem):
	def __init__(self, menu, title, shortcut=None, callback=None):
		MenuItem.__init__(self, menu, title, shortcut, callback, 'radio')

class CheckItem(MenuItem):
	def __init__(self, menu, title, shortcut=None, callback=None):
		MenuItem.__init__(self, menu, title, shortcut, callback, 'check')

def Separator(menu):
	menu.addseparator()

def SubMenu(menu, label, title=''):
	return menu.addsubmenu(label, title)


class AppleMenu(Menu):
	
	def __init__(self, bar, abouttext="About me...", aboutcallback=None):
		Menu.__init__(self, bar, "\024")
		self.additem(abouttext, None, aboutcallback)
		self.addseparator()
		self.menu.AppendResMenu('DRVR')
	
	def dispatch(self, id, item, window, event):
		if item == 1:
			Menu.dispatch(self, id, item, window, event)
		else:
			name = self.menu.GetItem(item)
			OpenDeskAcc(name)


def ostypecode(x):
	"Convert a long int to the 4-character code it really is"
	s = ''
	for i in range(4):
		x, c = divmod(x, 256)
		s = chr(c) + s
	return s


class TestApp(Application):
	
	"This class is used by the test() function"
	
	def makeusermenus(self):
		self.filemenu = m = Menu(self.menubar, "File")
		self.saveitem = MenuItem(m, "Save", "S", self.save)
		Separator(m)
		self.optionsmenu = mm = SubMenu(m, "Options")
		self.opt1 = CheckItem(mm, "Arguments")
		self.opt2 = CheckItem(mm, "Being hit on the head lessons")
		self.opt3 = CheckItem(mm, "Complaints")
		Separator(m)
		self.quititem = MenuItem(m, "Quit", "Q", self.quit)
	
	def save(self, *args):
		print "Save"
	
	def quit(self, *args):
		raise self


def test():
	"Test program"
	app = TestApp()
	app.mainloop()


if __name__ == '__main__':
	test()
