import W
import Fm
from WASTEconst import *
from SpecialKeys import *
from types import *
import Events
import string
import sys,os
import traceback
import MacOS
import MacPrefs
import PyInteractive

if not hasattr(sys, 'ps1'):
	sys.ps1 = '>>> '
if not hasattr(sys, 'ps2'):
	sys.ps2 = '... '


class ConsoleTextWidget(W.EditText):
	
	def __init__(self, *args, **kwargs):
		apply(W.EditText.__init__, (self,) + args, kwargs)
		self._inputstart = 0
		self._buf = ''
		self.pyinteractive = PyInteractive.PyInteractive()
	
		import __main__
		self._namespace = __main__.__dict__
	
	def insert(self, text):
		self.checkselection()
		self.ted.WEInsert(text, None, None)
		self.changed = 1
		self.selchanged = 1
	
	def set_namespace(self, dict):
		if type(dict) <> DictionaryType:
			raise TypeError, "The namespace needs to be a dictionary"
		self._namespace = dict
	
	def open(self):
		W.EditText.open(self)
		self.write('Python ' + sys.version + '\n' + sys.copyright + '\n')
		self.write(sys.ps1)
		self.flush()
	
	def key(self, char, event):
		(what, message, when, where, modifiers) = event
		if self._enabled and not modifiers & Events.cmdKey or char in arrowkeys:
			if char not in navigationkeys:
				self.checkselection()
			if char == enterkey:
				char = returnkey
			selstart, selend = self.getselection()
			if char == backspacekey:
				if selstart <= (self._inputstart - (selstart <> selend)):
					return
			self.ted.WEKey(ord(char), modifiers)
			if char not in navigationkeys:
				self.changed = 1
			if char not in scrollkeys:
				self.selchanged = 1
			self.updatescrollbars()
			if char == returnkey:
				text = self.get()[self._inputstart:selstart]
				saveyield = MacOS.EnableAppswitch(0)
				self.pyinteractive.executeline(text, self, self._namespace)
				MacOS.EnableAppswitch(saveyield)
				selstart, selend = self.getselection()
				self._inputstart = selstart
	
	def domenu_save_as(self, *args):
		import macfs
		fss, ok = macfs.StandardPutFile('Save console text as:', 'console.txt')
		if not ok:
			return
		f = open(fss.as_pathname(), 'wb')
		f.write(self.get())
		f.close()
		fss.SetCreatorType(W._signature, 'TEXT')
	
	def write(self, text):
		self._buf = self._buf + text
		if '\n' in self._buf:
			self.flush()
	
	def flush(self):
		stuff = string.split(self._buf, '\n')
		stuff = string.join(stuff, '\r')
		self.setselection_at_end()
		self.ted.WEInsert(stuff, None, None)
		selstart, selend = self.getselection()
		self._inputstart = selstart
		self._buf = ""
		self.ted.WEClearUndo()
		self.updatescrollbars()
	
	def selection_ok(self):
		selstart, selend = self.getselection()
		return not (selstart < self._inputstart or selend < self._inputstart)
	
	def checkselection(self):
		if not self.selection_ok():
			self.setselection_at_end()
	
	def setselection_at_end(self):
		end = self.ted.WEGetTextLength()
		self.setselection(end, end)
		self.updatescrollbars()
		
	def domenu_cut(self, *args):
		if not self.selection_ok():
			return
		W.EditText.domenu_cut(self)
	
	def domenu_paste(self, *args):
		if not self.selection_ok():
			self.setselection_at_end()
		W.EditText.domenu_paste(self)
	
	def domenu_clear(self, *args):
		if not self.selection_ok():
			return
		W.EditText.domenu_clear(self)


class PyConsole(W.Window):
	
	def __init__(self, bounds, show = 1, fontsettings = ("Monaco", 0, 9, (0, 0, 0)), unclosable = 0):
		W.Window.__init__(self,
					bounds, 
					"Python Interactive", 
					minsize = (200, 100), 
					tabbable = 0, 
					show = show)
		
		self._unclosable = unclosable
		consoletext = ConsoleTextWidget((-1, -1, -14, 1), inset = (6, 5), fontsettings = fontsettings)
		self._bary = W.Scrollbar((-15, 14, 16, -14), consoletext.vscroll, max = 32767)
		self.consoletext = consoletext
		self.namespacemenu = W.PopupMenu((-15, -1, 16, 16), [], self.consoletext.set_namespace)
		self.namespacemenu.bind('<click>', self.makenamespacemenu)
		self.open()
	
	def makenamespacemenu(self, *args):
		W.SetCursor('watch')
		namespacelist = self.getnamespacelist()
		self.namespacemenu.set([("Font settingsä", self.dofontsettings), 
				["Namespace"] + namespacelist, ("Clear buffer", self.clearbuffer), ("Browse namespaceä", self.browsenamespace)])
		currentname = self.consoletext._namespace["__name__"]
		for i in range(len(namespacelist)):
			if namespacelist[i][0] == currentname:
				break
		else:
			return
		# XXX this functionality should be generally available in Wmenus
		submenuid = self.namespacemenu.menu.menu.GetItemMark(2)
		menu = self.namespacemenu.menu.bar.menus[submenuid]
		menu.menu.CheckItem(i + 1, 1)
	
	def browsenamespace(self):
		import PyBrowser, W
		W.SetCursor('watch')
		PyBrowser.Browser(self.consoletext._namespace, self.consoletext._namespace["__name__"])
	
	def clearbuffer(self):
		import Res
		self.consoletext.ted.WEUseText(Res.Resource(''))
		self.consoletext.write(sys.ps1)
		self.consoletext.flush()
	
	def getnamespacelist(self):
		import __main__
		editors = filter(lambda x: x.__class__.__name__ == "Editor", self.parent._windows.values())
		
		namespaces = [ ("__main__",__main__.__dict__) ]
		for ed in editors:
			modname = os.path.splitext(ed.title)[0]
			if sys.modules.has_key(modname):
				module = sys.modules[modname] 
				namespaces.append((modname, module.__dict__))
			else:
				if ed.title[-3:] == '.py':
					modname = ed.title[:-3]
				else:
					modname = ed.title
				ed.globals["__name__"] = modname
				namespaces.append((modname, ed.globals))
		return namespaces
	
	def dofontsettings(self):
		import FontSettings
		fontsettings = FontSettings.FontDialog(self.consoletext.getfontsettings())
		if fontsettings:
			self.consoletext.setfontsettings(fontsettings)
	
	def show(self, onoff = 1):
		W.Window.show(self, onoff)
		if onoff:
			self.select()
	
	def close(self):
		if self._unclosable:
			self.show(0)
			return -1
		W.Window.close(self)
	
	def writeprefs(self):
		prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
		prefs.console.show = self.isvisible()
		prefs.console.windowbounds = self.getbounds()
		prefs.console.fontsettings = self.consoletext.getfontsettings()
		prefs.save()


class OutputTextWidget(W.EditText):
	
	def _getflags(self):
		import WASTEconst
		return WASTEconst.weDoAutoScroll | WASTEconst.weDoMonoStyled | \
				WASTEconst.weDoUndo

class PyOutput:
	
	def __init__(self):
		self.w = None
		self.closed = 1
		self._buf = ''
		# should be able to set this
		self.savestdout, self.savestderr = sys.stdout, sys.stderr
		sys.stderr = sys.stdout = self
	
	def write(self, text):
		self._buf = self._buf + text
		if '\n' in self._buf:
			self.flush()
	
	def flush(self):
		self.show()
		stuff = string.split(self._buf, '\n')
		stuff = string.join(stuff, '\r')
		end = self.w.outputtext.ted.WEGetTextLength()
		self.w.outputtext.setselection(end, end)
		self.w.outputtext.ted.WEInsert(stuff, None, None)
		self._buf = ""
		self.w.outputtext.ted.WEClearUndo()
		self.w.outputtext.updatescrollbars()
	
	def show(self):
		if self.closed:
			if not self.w:
				self.setupwidgets()
				self.w.open()
				self.closed = 0
			else:
				self.w.show(1)
				self.closed = 0
				self.w.select()
	
	def setupwidgets(self): 
		self.w = W.Window((450, 200), "Output", minsize = (200, 100), tabbable = 0)
		self.w.outputtext = OutputTextWidget((-1, -1, -14, 1), inset = (6, 5))
		self.w._bary = W.Scrollbar((-15, -1, 16, -14), self.w.outputtext.vscroll, max = 32767)
		self.w.bind("<close>", self.close)
		self.w.bind("<activate>", self.activate)
	
	def activate(self, onoff):
		if onoff:
			self.closed = 0
	
	def close(self):
		self.w.show(0)
		self.closed = 1
		return -1


def installconsole(defaultshow = 1):
	global console
	prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
	if not prefs.console or not hasattr(prefs.console, 'show'):
		prefs.console.show = defaultshow
	if not hasattr(prefs.console, "windowbounds"):
		prefs.console.windowbounds = (450, 250)
	if not hasattr(prefs.console, "fontsettings"):
		prefs.console.fontsettings = ("Monaco", 0, 9, (0, 0, 0))
	console = PyConsole(prefs.console.windowbounds, prefs.console.show, prefs.console.fontsettings, 1)

def installoutput():
	global output
	output = PyOutput()


