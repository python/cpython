import FrameWork
import Win
import Qd
import MacOS
import Events
import traceback
from types import *

import Menu; MenuToolbox = Menu; del Menu


class Application(FrameWork.Application):
	
	def __init__(self, signature = 'Pyth'):
		import W
		W.setapplication(self, signature)
		FrameWork.Application.__init__(self)
		self._suspended = 0
		self.quitting = 0
		self.debugger_quitting = 1
		self.DebuggerQuit = 'DebuggerQuitDummyException'
	
	def mainloop(self, mask = FrameWork.everyEvent, wait = 0):
		import W
		self.quitting = 0
		saveyield = MacOS.EnableAppswitch(-1)
		try:
			while not self.quitting:
				try:
					self.do1event(mask, wait)
				except W.AlertError, detail:
					MacOS.EnableAppswitch(-1)
					W.Message(detail)
				except self.DebuggerQuit:
					MacOS.EnableAppswitch(-1)
				except:
					MacOS.EnableAppswitch(-1)
					import PyEdit
					PyEdit.tracebackwindow.traceback()
		finally:
			MacOS.EnableAppswitch(1)
	
	def debugger_mainloop(self, mask = FrameWork.everyEvent, wait = 0):
		import W
		self.debugger_quitting = 0
		saveyield = MacOS.EnableAppswitch(-1)
		try:
			while not self.quitting and not self.debugger_quitting:
				try:
					self.do1event(mask, wait)
				except W.AlertError, detail:
					W.Message(detail)
				except:
					import PyEdit
					PyEdit.tracebackwindow.traceback()
		finally:
			MacOS.EnableAppswitch(saveyield)
	
	def idle(self, event):
		if not self._suspended:
			if not self.do_frontWindowMethod("idle", event):
				Qd.InitCursor()
	
	def do_frontWindowMethod(self, attr, *args):
		wid = Win.FrontWindow()
		if wid and self._windows.has_key(wid):
			window = self._windows[wid]
			if hasattr(window, attr):
				handler = getattr(window, attr)
				apply(handler, args)
				return 1
	
	def appendwindow(self, wid, window):
		self._windows[wid] = window
		self.makeopenwindowsmenu()
		
	def removewindow(self, wid):
		del self._windows[wid]
		self.makeopenwindowsmenu()
	
	def do_key(self, event):
		(what, message, when, where, modifiers) = event
		ch = chr(message & FrameWork.charCodeMask)
		rest = message & ~FrameWork.charCodeMask
		wid = Win.FrontWindow()
		if modifiers & FrameWork.cmdKey:
			if wid and self._windows.has_key(wid):
				self.checkmenus(self._windows[wid])
			else:
				self.checkmenus(None)
			event = (what, ord(ch) | rest, when, where, modifiers)
			result = MenuToolbox.MenuKey(ord(ch))
			id = (result>>16) & 0xffff	# Hi word
			item = result & 0xffff		# Lo word
			if id:
				self.do_rawmenu(id, item, None, event)
				return	# here! we had a menukey! 
			#else:
			#	print "XXX Command-" +`ch`
		# See whether the front window wants it
		if wid and self._windows.has_key(wid):
			window = self._windows[wid]
			try:
				do_char = window.do_char
			except AttributeError:
				do_char = self.do_char
			do_char(ch, event)
		# else it wasn't for us, sigh...
	
	def do_inMenuBar(self, partcode, window, event):
		Qd.InitCursor()
		(what, message, when, where, modifiers) = event
		self.checkopenwindowsmenu()
		wid = Win.FrontWindow()
		if wid and self._windows.has_key(wid):
			self.checkmenus(self._windows[wid])
		else:
			self.checkmenus(None)
		result = MenuToolbox.MenuSelect(where)
		id = (result>>16) & 0xffff	# Hi word
		item = result & 0xffff		# Lo word
		self.do_rawmenu(id, item, window, event)
	
	def do_updateEvt(self, event):
		(what, message, when, where, modifiers) = event
		wid = Win.WhichWindow(message)
		if wid and self._windows.has_key(wid):
			window = self._windows[wid]
			window.do_rawupdate(wid, event)
		else:
			if wid:
				wid.HideWindow()
				import sys
				sys.stderr.write("XXX killed unknown (crashed?) Python window.\n")
			else:
				MacOS.HandleEvent(event)
	
	def suspendresume(self, onoff):
		pass
	
	def do_suspendresume(self, event):
		# Is this a good idea???
		(what, message, when, where, modifiers) = event
		self._suspended = not message & 1
		self.suspendresume(message & 1)
		w = Win.FrontWindow()
		if w:
			# XXXX Incorrect, should stuff windowptr into message field
			nev = (Events.activateEvt, w, when, where, message&1)
			self.do_activateEvt(nev)
	
	def checkopenwindowsmenu(self):
		if self._openwindowscheckmark:
			self.openwindowsmenu.menu.CheckItem(self._openwindowscheckmark, 0)
		window = Win.FrontWindow()
		if window:
			for item, wid in self._openwindows.items():
				if wid == window:
					#self.pythonwindowsmenuitem.check(1)
					self.openwindowsmenu.menu.CheckItem(item, 1)
					self._openwindowscheckmark = item
					break
		else:
			self._openwindowscheckmark = 0
		#if self._openwindows:
		#	self.pythonwindowsmenuitem.enable(1)
		#else:
		#	self.pythonwindowsmenuitem.enable(0)
	
	def checkmenus(self, window):
		for item in self._menustocheck:
			callback = item.menu.items[item.item-1][2]
			if type(callback) <> StringType:
				item.enable(1)
			elif hasattr(window, "domenu_" + callback):
				if hasattr(window, "can_" + callback):
					canhandler = getattr(window, "can_" + callback)
					if canhandler(item):
						item.enable(1)
					else:
						item.enable(0)
				else:
					item.enable(1)
			else:
				item.enable(0)
	
	def makemenubar(self):
		self.menubar = MenuBar(self)
		FrameWork.AppleMenu(self.menubar, self.getabouttext(), self.do_about)
		self.makeusermenus()

	def scriptswalk(self, top, menu):
		import os, macfs, string
		try:
			names = os.listdir(top)
		except os.error:
			FrameWork.MenuItem(menu, '(Scripts Folder not found)', None, None)
			return
		for name in names:
			path = os.path.join(top, name)
			name = string.strip(name)
			if name[-3:] == '---':
				menu.addseparator()
			elif os.path.isdir(path):
				submenu = FrameWork.SubMenu(menu, name)
				self.scriptswalk(path, submenu)
			else:
				fss = macfs.FSSpec(path)
				creator, type = fss.GetCreatorType()
				if type == 'TEXT':
					if name[-3:] == '.py':
						name = name[:-3]
					item = FrameWork.MenuItem(menu, name, None, self.domenu_script)
					self._scripts[(menu.id, item.item)] = path
	
	def domenu_script(self, id, item, window, event):
		(what, message, when, where, modifiers) = event
		path = self._scripts[(id, item)]
		import os
		if not os.path.exists(path):
			self.makescriptsmenu()
			import W
			raise W.AlertError, "File not found."
		if modifiers & FrameWork.optionKey:
			self.openscript(path)
		else:
			import W, MacOS, sys
			W.SetCursor("watch")
			sys.argv = [path]
			#cwd = os.getcwd()
			#os.chdir(os.path.dirname(path) + ':')
			try:
				# xxx if there is a script window for this file,
				# exec in that window's namespace.
				# xxx what to do when it's not saved???
				# promt to save?
				MacOS.EnableAppswitch(0)
				execfile(path, {'__name__': '__main__', '__file__': path})
			except W.AlertError, detail:
				MacOS.EnableAppswitch(-1)
				raise W.AlertError, detail
			except KeyboardInterrupt:
				MacOS.EnableAppswitch(-1)
			except:
				MacOS.EnableAppswitch(-1)
				import PyEdit
				PyEdit.tracebackwindow.traceback(1)
			else:
				MacOS.EnableAppswitch(-1)
			#os.chdir(cwd)
	
	def openscript(self, filename, lineno = None, charoffset = 0):
		import os, PyEdit, W
		editor = self.getscript(filename)
		if editor:
			editor.select()
		elif os.path.exists(filename):
			editor = PyEdit.Editor(filename)
		elif filename[-3:] == '.py':
			import imp
			modname = os.path.basename(filename)[:-3]
			try:
				f, filename, (suff, mode, dummy) = imp.find_module(modname)
			except ImportError:
				raise W.AlertError, "Can¹t find file for ³%s²" % modname
			else:
				f.close()
			if suff == '.py':
				self.openscript(filename, lineno, charoffset)
				return
			else:
				raise W.AlertError, "Can¹t find file for ³%s²" % modname
		else:
			raise W.AlertError, "Can¹t find file Œ%s¹" % filename
		if lineno is not None:
			editor.selectline(lineno, charoffset)
		return editor
	
	def getscript(self, filename):
		if filename[:1] == '<' and filename[-1:] == '>':
			filename = filename[1:-1]
		import string
		lowpath = string.lower(filename)
		for wid, window in self._windows.items():
			if hasattr(window, "path") and lowpath == string.lower(window.path):
				return window
			elif hasattr(window, "path") and filename == wid.GetWTitle():
				return window
	
	def getprefs(self):
		import MacPrefs
		return MacPrefs.GetPrefs(self.preffilepath)



class MenuBar(FrameWork.MenuBar):
	
	possibleIDs = range(10, 256)
	
	def getnextid(self):
		id = self.possibleIDs[0]
		del self.possibleIDs[0]
		return id
	
	def __init__(self, parent = None):
		self.bar = MenuToolbox.GetMenuBar()
		MenuToolbox.ClearMenuBar()
		self.menus = {}
		self.parent = parent
	
	def dispatch(self, id, item, window, event):
		if self.menus.has_key(id):
			self.menus[id].dispatch(id, item, window, event)
	
	def delmenu(self, id):
		MenuToolbox.DeleteMenu(id)
		if id in self.possibleIDs:
			print "XXX duplicate menu ID!", id
		self.possibleIDs.append(id)
	

class Menu(FrameWork.Menu):
	
	def dispatch(self, id, item, window, event):
		title, shortcut, callback, kind = self.items[item-1]
		if type(callback) == StringType:
			callback = self._getmenuhandler(callback)
		if callback:
			import W
			W.CallbackCall(callback, 0, id, item, window, event)
	
	def _getmenuhandler(self, callback):
		menuhandler = None
		wid = Win.FrontWindow()
		if wid and self.bar.parent._windows.has_key(wid):
			window = self.bar.parent._windows[wid]
			if hasattr(window, "domenu_" + callback):
				menuhandler = getattr(window, "domenu_" + callback)
			elif hasattr(self.bar.parent, "domenu_" + callback):
				menuhandler = getattr(self.bar.parent, "domenu_" + callback)
		elif hasattr(self.bar.parent, "domenu_" + callback):
			menuhandler = getattr(self.bar.parent, "domenu_" + callback)
		return menuhandler

