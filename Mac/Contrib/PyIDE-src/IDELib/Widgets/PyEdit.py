"""a simple python editor"""

import W
import Wtraceback
from SpecialKeys import *

import macfs
import MacOS
import Win
import Res
import Evt
import os
import imp
import sys
import string
import marshal
import regex

_scriptuntitledcounter = 1
_wordchars = string.letters + string.digits + "_"


class Editor(W.Window):
	
	def __init__(self, path = "", title = ""):
		global _scriptuntitledcounter
		if not path:
			if title:
				self.title = title
			else:
				self.title = "Untitled Script " + `_scriptuntitledcounter`
				_scriptuntitledcounter = _scriptuntitledcounter + 1
			text = ""
			self._creator = W._signature
		elif os.path.exists(path):
			self.title = os.path.basename(path)
			f = open(path, "rb")
			text = f.read()
			f.close()
			fss = macfs.FSSpec(path)
			self._creator, filetype = fss.GetCreatorType()
		else:
			raise IOError, "file '%s' does not exist" % path
		self.path = path
		
		self.settings = {}
		if self.path:
			self.readwindowsettings()
		if self.settings.has_key("windowbounds"):
			bounds = self.settings["windowbounds"]
		else:
			bounds = (500, 250)
		if self.settings.has_key("fontsettings"):
			self.fontsettings = self.settings["fontsettings"]
		else:
			self.fontsettings = ("Python-Sans", 0, 9, (0, 0, 0))
		W.Window.__init__(self, bounds, self.title, minsize = (330, 120), tabbable = 0)
		
		self.setupwidgets(text)
		if self.settings.has_key("selection"):
			selstart, selend = self.settings["selection"]
			self.setselection(selstart, selend)
		self.open()
		self.setinfotext()
		self.globals = {}
		self._buf = ""  # for write method
		self.debugging = 0
		self.profiling = 0
		if self.settings.has_key("run_as_main"):
			self.run_as_main = self.settings["run_as_main"]
		else:
			self.run_as_main = 0
	
	def readwindowsettings(self):
		try:
			resref = Res.OpenResFile(self.path)
		except Res.Error:
			return
		try:
			Res.UseResFile(resref)
			data = Res.Get1Resource('PyWS', 128)
			self.settings = marshal.loads(data.data)
		except:
			pass
		Res.CloseResFile(resref)
		
	def writewindowsettings(self):
		try:
			resref = Res.OpenResFile(self.path)
		except Res.Error:
			Res.CreateResFile(self.path)
			resref = Res.OpenResFile(self.path)
		try:
			data = Res.Resource(marshal.dumps(self.settings))
			Res.UseResFile(resref)
			try:
				temp = Res.Get1Resource('PyWS', 128)
				temp.RemoveResource()
			except Res.Error:
				pass
			data.AddResource('PyWS', 128, "window settings")
		finally:
			Res.UpdateResFile(resref)
			Res.CloseResFile(resref)
	
	def getsettings(self):
		self.settings = {}
		self.settings["windowbounds"] = self.getbounds()
		self.settings["selection"] = self.getselection()
		self.settings["fontsettings"] = self.editgroup.editor.getfontsettings()
		self.settings["run_as_main"] = self.run_as_main
	
	def get(self):
		return self.editgroup.editor.get()
	
	def getselection(self):
		return self.editgroup.editor.ted.WEGetSelection()
	
	def setselection(self, selstart, selend):
		self.editgroup.editor.setselection(selstart, selend)
	
	def getfilename(self):
		if self.path:
			return self.path
		return '<%s>' % self.title
	
	def setupwidgets(self, text):
		topbarheight = 24
		popfieldwidth = 80
		self.lastlineno = None
		
		# make an editor
		self.editgroup = W.Group((0, topbarheight + 1, 0, 0))
		editor = W.PyEditor((0, 0, -15,-15), text, fontsettings = self.fontsettings, 
				file = self.getfilename())
		
		# make the widgets
		self.popfield = ClassFinder((popfieldwidth - 17, -15, 16, 16), [], self.popselectline)
		self.linefield = W.EditText((-1, -15, popfieldwidth - 15, 16), inset = (6, 2))
		self.editgroup._barx = W.Scrollbar((popfieldwidth - 2, -15, -14, 16), editor.hscroll, max = 32767)
		self.editgroup._bary = W.Scrollbar((-15, 14, 16, -14), editor.vscroll, max = 32767)
		self.editgroup.editor = editor	# add editor *after* scrollbars
		
		self.editgroup.optionsmenu = W.PopupMenu((-15, -1, 16, 16), [])
		self.editgroup.optionsmenu.bind('<click>', self.makeoptionsmenu)
		self.hline = W.HorizontalLine((0, topbarheight, 0, 0))
		self.infotext = W.TextBox((175, 6, -4, 14))
		self.runbutton = W.Button((5, 4, 80, 16), "Run all", self.run)
		self.runselbutton = W.Button((90, 4, 80, 16), "Run selection", self.runselection)
		
		# bind some keys
		editor.bind("cmdr", self.runbutton.push)
		editor.bind("enter", self.runselbutton.push)
		editor.bind("cmdj", self.domenu_gotoline)
		editor.bind("cmdd", self.domenu_toggledebugger)
		editor.bind("<idle>", self.updateselection)
		
		editor.bind("cmde", searchengine.setfindstring)
		editor.bind("cmdf", searchengine.show)
		editor.bind("cmdg", searchengine.findnext)
		editor.bind("cmdshiftr", searchengine.replace)
		editor.bind("cmdt", searchengine.replacefind)
		
		self.linefield.bind("return", self.dolinefield)
		self.linefield.bind("enter", self.dolinefield)
		self.linefield.bind("tab", self.dolinefield)
		
		# intercept clicks
		editor.bind("<click>", self.clickeditor)
		self.linefield.bind("<click>", self.clicklinefield)
	
	def makeoptionsmenu(self):
		menuitems = [('Font settingsä', self.domenu_fontsettings), 
				('\0' + chr(self.run_as_main) + 'Run as __main__', self.domenu_toggle_run_as_main), 
				('Modularize', self.domenu_modularize),
				('Browse namespaceä', self.domenu_browsenamespace), 
				'-']
		if self.editgroup.editor._debugger:
			menuitems = menuitems + [('Disable debugger', self.domenu_toggledebugger),
				('Clear breakpoints', self.domenu_clearbreakpoints),
				('Edit breakpointsä', self.domenu_editbreakpoints)]
		else:
			menuitems = menuitems + [('Enable debugger', self.domenu_toggledebugger)]
		if self.profiling:
			menuitems = menuitems + [('Disable profiler', self.domenu_toggleprofiler)]
		else:
			menuitems = menuitems + [('Enable profiler', self.domenu_toggleprofiler)]
		self.editgroup.optionsmenu.set(menuitems)
	
	def domenu_toggle_run_as_main(self):
		self.run_as_main = not self.run_as_main
		self.editgroup.editor.selchanged = 1
	
	def showbreakpoints(self, onoff):
		self.editgroup.editor.showbreakpoints(onoff)
		self.debugging = onoff
	
	def domenu_clearbreakpoints(self, *args):
		self.editgroup.editor.clearbreakpoints()
	
	def domenu_editbreakpoints(self, *args):
		self.editgroup.editor.editbreakpoints()
	
	def domenu_toggledebugger(self, *args):
		if not self.debugging:
			W.SetCursor('watch')
		self.debugging = not self.debugging
		self.editgroup.editor.togglebreakpoints()
		
	def domenu_toggleprofiler(self, *args):
		self.profiling = not self.profiling
	
	def domenu_browsenamespace(self, *args):
		import PyBrowser, W
		W.SetCursor('watch')
		globals, file = self.getenvironment()
		modname = _filename_as_modname(self.title)
		if not modname:
			modname = self.title
		PyBrowser.Browser(globals, "Object browser: " + modname)
	
	def domenu_modularize(self, *args):
		modname = _filename_as_modname(self.title)
		if not modname:
			raise W.AlertError, 'Canπt modularize ≥%s≤' % self.title
		self.run()	
		if self.path:
			file = self.path
		else:
			file = self.title
		
		if self.globals and not sys.modules.has_key(modname):
			module = imp.new_module(modname)
			for attr in self.globals.keys():
				setattr(module,attr,self.globals[attr])
			sys.modules[modname] = module
			self.globals = {}
	
	def domenu_fontsettings(self, *args):
		import FontSettings
		fontsettings = FontSettings.FontDialog(self.editgroup.editor.getfontsettings())
		if fontsettings:
			self.editgroup.editor.setfontsettings(fontsettings)
	
	def clicklinefield(self):
		if self._currentwidget <> self.linefield:
			self.linefield.select(1)
			self.linefield.selectall()
			return 1
	
	def clickeditor(self):
		if self._currentwidget <> self.editgroup.editor:
			self.dolinefield()
			return 1
	
	def updateselection(self, force = 0):
		sel = min(self.editgroup.editor.getselection())
		lineno = self.editgroup.editor.offsettoline(sel)
		if lineno <> self.lastlineno or force:
			self.lastlineno = lineno
			self.linefield.set(str(lineno + 1))
			self.linefield.selview()
	
	def dolinefield(self):
		try:
			lineno = string.atoi(self.linefield.get()) - 1
			if lineno <> self.lastlineno:
				self.editgroup.editor.selectline(lineno)
				self.updateselection(1)
		except:
			self.updateselection(1)
		self.editgroup.editor.select(1)
	
	def setinfotext(self):
		if not hasattr(self, 'infotext'):
			return
		if self.path:
			self.infotext.set(self.path)
		else:
			self.infotext.set("")
	
	def close(self):
		if self.editgroup.editor.changed:
			import EasyDialogs
			import Qd
			Qd.InitCursor() # XXX should be done by dialog
			save = EasyDialogs.AskYesNoCancel('Save window ≥%s≤ before closing?' % self.title, 1)
			if save > 0:
				if self.domenu_save():
					return 1
			elif save < 0:
				return 1
		self.globals = None	     # XXX doesn't help... all globals leak :-(
		W.Window.close(self)
	
	def domenu_close(self, *args):
		return self.close()
	
	def domenu_save(self, *args):
		if not self.path:
			# Will call us recursively
			return self.domenu_save_as()
		data = self.editgroup.editor.get()
		fp = open(self.path, 'wb')  # open file in binary mode, data has '\r' line-endings
		fp.write(data)
		fp.close()
		fss = macfs.FSSpec(self.path)
		fss.SetCreatorType(self._creator, 'TEXT')
		self.getsettings()
		self.writewindowsettings()
		self.editgroup.editor.changed = 0
		self.editgroup.editor.selchanged = 0
		import linecache
		if linecache.cache.has_key(self.path):
			del linecache.cache[self.path]
		import macostools
		macostools.touched(self.path)
	
	def can_save(self, menuitem):
		return self.editgroup.editor.changed or self.editgroup.editor.selchanged
	
	def domenu_save_as(self, *args):
		fss, ok = macfs.StandardPutFile('Save as:', self.title)
		if not ok: 
			return 1
		self.showbreakpoints(0)
		self.path = fss.as_pathname()
		self.setinfotext()
		self.title = os.path.split(self.path)[-1]
		self.wid.SetWTitle(self.title)
		self.domenu_save()
		self.editgroup.editor.setfile(self.getfilename())
		app = W.getapplication()
		app.makeopenwindowsmenu()
		if hasattr(app, 'makescriptsmenu'):
			app = W.getapplication()
			fss, fss_changed = app.scriptsfolder.Resolve()
			path = fss.as_pathname()
			if path == self.path[:len(path)]:
				W.getapplication().makescriptsmenu()
	
	def domenu_gotoline(self, *args):
		self.linefield.selectall()
		self.linefield.select(1)
		self.linefield.selectall()
	
	def domenu_selectline(self, *args):
		self.editgroup.editor.expandselection()
	
	def domenu_shiftleft(self, *args):
		self.editgroup.editor.shiftleft()
	
	def domenu_shiftright(self, *args):
		self.editgroup.editor.shiftright()
	
	def domenu_find(self, *args):
		searchengine.show()
	
	def domenu_entersearchstring(self, *args):
		searchengine.setfindstring()
	
	def domenu_replace(self, *args):
		searchengine.replace()
	
	def domenu_findnext(self, *args):
		searchengine.findnext()
	
	def domenu_replacefind(self, *args):
		searchengine.replacefind()
	
	def domenu_run(self, *args):
		self.runbutton.push()
	
	def domenu_runselection(self, *args):
		self.runselbutton.push()
	
	def run(self):
		pytext = self.editgroup.editor.get()
		globals, file = self.getenvironment()
		if self.path:
			cwd = os.getcwd()
			os.chdir(os.path.dirname(self.path) + ':')
		else:
			cwd = None
		execstring(pytext, globals, globals, file, self.debugging, self.run_as_main, self.profiling)
		if cwd:
			os.chdir(cwd)
	
	def runselection(self):
		self._runselection()
	
	def _runselection(self):
		globals, file = self.getenvironment()
		locals = globals
		# select whole lines
		self.editgroup.editor.expandselection()
		
		# get lineno of first selected line
		selstart, selend = self.editgroup.editor.getselection()
		selstart, selend = min(selstart, selend), max(selstart, selend)
		selfirstline = self.editgroup.editor.offsettoline(selstart)
		alltext = self.editgroup.editor.get()
		pytext = alltext[selstart:selend]
		lines = string.split(pytext, '\r')
		indent = getminindent(lines)
		if indent == 1:
			classname = ''
			alllines = string.split(alltext, '\r')
			identifieRE_match = _identifieRE.match
			for i in range(selfirstline - 1, -1, -1):
				line = alllines[i]
				if line[:6] == 'class ':
					classname = string.split(string.strip(line[6:]))[0]
					classend = identifieRE_match(classname)
					if classend < 1:
						raise W.AlertError, 'Canπt find a class.'
					classname = classname[:classend]
					break
				elif line and line[0] not in '\t#':
					raise W.AlertError, 'Canπt find a class.'
			else:
				raise W.AlertError, 'Canπt find a class.'
			if globals.has_key(classname):
				locals = globals[classname].__dict__
			else:
				raise W.AlertError, 'Canπt find class ≥%s≤.' % classname
			for i in range(len(lines)):
				lines[i] = lines[i][1:]
			pytext = string.join(lines, '\r')
		elif indent > 0:
			raise W.AlertError, 'Canπt run indented code.'
		
		# add newlines to fool compile/exec: a traceback will give the right line number
		pytext = selfirstline * '\r' + pytext
		
		if self.path:
			cwd = os.getcwd()
			os.chdir(os.path.dirname(self.path) + ':')
		else:
			cwd = None
		execstring(pytext, globals, locals, file, self.debugging, self.run_as_main, self.profiling)
		if cwd:
			os.chdir(cwd)
	
	def getenvironment(self):
		if self.path:
			file = self.path
			modname = _filename_as_modname(self.title)
			if sys.modules.has_key(modname):
				globals = sys.modules[modname].__dict__
				self.globals = {}
			else:
				globals = self.globals
		else:
			file = '<%s>' % self.title
			globals = self.globals
		return globals, file
	
	def write(self, stuff):
		"""for use as stdout"""
		self._buf = self._buf + stuff
		if '\n' in self._buf:
			self.flush()
	
	def flush(self):
		stuff = string.split(self._buf, '\n')
		stuff = string.join(stuff, '\r')
		end = self.editgroup.editor.ted.WEGetTextLength()
		self.editgroup.editor.ted.WESetSelection(end, end)
		self.editgroup.editor.ted.WEInsert(stuff, None, None)
		self.editgroup.editor.updatescrollbars()
		self._buf = ""
		# ? optional:
		#self.wid.SelectWindow()
	
	def getclasslist(self):
		from string import find, strip
		editor = self.editgroup.editor
		text = editor.get()
		list = []
		append = list.append
		functag = "func"
		classtag = "class"
		methodtag = "method"
		pos = -1
		if text[:4] == 'def ':
			append((pos + 4, functag))
			pos = 4
		while 1:
			pos = find(text, '\rdef ', pos + 1)
			if pos < 0:
				break
			append((pos + 5, functag))
		pos = -1
		if text[:6] == 'class ':
			append((pos + 6, classtag))
			pos = 6
		while 1:
			pos = find(text, '\rclass ', pos + 1)
			if pos < 0:
				break
			append((pos + 7, classtag))
		pos = 0
		while 1:
			pos = find(text, '\r\tdef ', pos + 1)
			if pos < 0:
				break
			append((pos + 6, methodtag))
		list.sort()
		classlist = []
		methodlistappend = None
		offsetToLine = editor.ted.WEOffsetToLine
		getLineRange = editor.ted.WEGetLineRange
		append = classlist.append
		identifieRE_match = _identifieRE.match
		for pos, tag in list:
			lineno = offsetToLine(pos)
			lineStart, lineEnd = getLineRange(lineno)
			line = strip(text[pos:lineEnd])
			line = line[:identifieRE_match(line)]
			if tag is functag:
				append(("def " + line, lineno + 1))
				methodlistappend = None
			elif tag is classtag:
				append(["class " + line])
				methodlistappend = classlist[-1].append
			elif methodlistappend and tag is methodtag:
				methodlistappend(("def " + line, lineno + 1))
		return classlist
	
	def popselectline(self, lineno):
		self.editgroup.editor.selectline(lineno - 1)
	
	def selectline(self, lineno, charoffset = 0):
		self.editgroup.editor.selectline(lineno - 1, charoffset)


class Reporter(Editor):
	
	def close(self):
		self.globals = None
		W.Window.close(self)
	
	def domenu_run(self, *args):
		self.run()
	
	def domenu_runselection(self, *args):
		self.runselection()
	
	def setupwidgets(self, text):
		topbarheight = -1
		popfieldwidth = 80
		self.lastlineno = None
		
		# make an editor
		self.editgroup = W.Group((0, topbarheight + 1, 0, 0))
		self.editgroup.editor = W.PyEditor((0, 0, -15,-15), text)
		
		# make the widgets
		self.editgroup._barx = W.Scrollbar((popfieldwidth-2, -15, -14, 16), self.editgroup.editor.hscroll, max = 32767)
		self.editgroup._bary = W.Scrollbar((-15, -1, 16, -14), self.editgroup.editor.vscroll, max = 32767)
		self.hline = W.HorizontalLine((0, -15, 0, 0))
		
		# bind some keys
		self.editgroup.editor.bind("cmdr", self.run)
		self.editgroup.editor.bind("enter", self.runselection)
		
		self.editgroup.editor.bind("cmde", searchengine.setfindstring)
		self.editgroup.editor.bind("cmdf", searchengine.show)
		self.editgroup.editor.bind("cmdg", searchengine.findnext)
		self.editgroup.editor.bind("cmdshiftr", searchengine.replace)
		self.editgroup.editor.bind("cmdt", searchengine.replacefind)


def _escape(where, what) : 
	return string.join(string.split(where, what), '\\' + what)

def _makewholewordpattern(word):
	# first, escape special regex chars
	for esc in "\\[].*^+$?":
		word = _escape(word, esc)
	import regex
	notwordcharspat = '[^' + _wordchars + ']'
	pattern = '\(' + word + '\)'
	if word[0] in _wordchars:
		pattern = notwordcharspat + pattern
	if word[-1] in _wordchars:
		pattern = pattern + notwordcharspat
	return regex.compile(pattern)

class SearchEngine:
	
	def __init__(self):
		self.visible = 0
		self.w = None
		self.parms = {  "find": "",
					"replace": "",
					"wrap": 1,
					"casesens": 1,
					"wholeword": 1
				}
		import MacPrefs
		prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
		if prefs.searchengine:
			self.parms["casesens"] = prefs.searchengine.casesens
			self.parms["wrap"] = prefs.searchengine.wrap
			self.parms["wholeword"] = prefs.searchengine.wholeword
	
	def show(self):
		self.visible = 1
		if self.w:
			self.w.wid.ShowWindow()
			self.w.wid.SelectWindow()
			self.w.find.edit.select(1)
			self.w.find.edit.selectall()
			return
		self.w = W.Dialog((420, 150), "Find")
		
		self.w.find = TitledEditText((10, 4, 300, 36), "Search for:")
		self.w.replace = TitledEditText((10, 100, 300, 36), "Replace with:")
		
		self.w.boxes = W.Group((10, 50, 300, 40))
		self.w.boxes.casesens = W.CheckBox((0, 0, 100, 16), "Case sensitive")
		self.w.boxes.wholeword = W.CheckBox((0, 20, 100, 16), "Whole word")
		self.w.boxes.wrap = W.CheckBox((110, 0, 100, 16), "Wrap around")
		
		self.buttons = [	("Find",		"cmdf",	 self.find), 
					("Replace",	     "cmdr",	 self.replace), 
					("Replace all",	 None,   self.replaceall), 
					("Donπt find",  "cmdd",	 self.dont), 
					("Cancel",	      "cmd.",	 self.cancel)
				]
		for i in range(len(self.buttons)):
			bounds = -90, 22 + i * 24, 80, 16
			title, shortcut, callback = self.buttons[i]
			self.w[title] = W.Button(bounds, title, callback)
			if shortcut:
				self.w.bind(shortcut, self.w[title].push)
		self.w.setdefaultbutton(self.w["Donπt find"])
		self.w.find.edit.bind("<key>", self.key)
		self.w.bind("<activate>", self.activate)
		self.w.open()
		self.setparms()
		self.w.find.edit.select(1)
		self.w.find.edit.selectall()
		self.checkbuttons()
	
	def key(self, char, modifiers):
		self.w.find.edit.key(char, modifiers)
		self.checkbuttons()
		return 1
	
	def activate(self, onoff):
		if onoff:
			self.checkbuttons()
	
	def checkbuttons(self):
		editor = findeditor(self)
		if editor:
			if self.w.find.get():
				for title, cmd, call in self.buttons[:-2]:
					self.w[title].enable(1)
				self.w.setdefaultbutton(self.w["Find"])
			else:
				for title, cmd, call in self.buttons[:-2]:
					self.w[title].enable(0)
				self.w.setdefaultbutton(self.w["Donπt find"])
		else:
			for title, cmd, call in self.buttons[:-2]:
				self.w[title].enable(0)
			self.w.setdefaultbutton(self.w["Donπt find"])
	
	def find(self):
		self.getparmsfromwindow()
		if self.findnext():
			self.hide()
	
	def replace(self):
		editor = findeditor(self)
		if not editor:
			return
		if self.visible:
			self.getparmsfromwindow()
		text = editor.getselectedtext()
		find = self.parms["find"]
		if not self.parms["casesens"]:
			find = string.lower(find)
			text = string.lower(text)
		if text == find:
			self.hide()
			editor.insert(self.parms["replace"])
	
	def replaceall(self):
		editor = findeditor(self)
		if not editor:
			return
		if self.visible:
			self.getparmsfromwindow()
		W.SetCursor("watch")
		find = self.parms["find"]
		if not find:
			return
		findlen = len(find)
		replace = self.parms["replace"]
		replacelen = len(replace)
		Text = editor.get()
		if not self.parms["casesens"]:
			find = string.lower(find)
			text = string.lower(Text)
		else:
			text = Text
		newtext = ""
		pos = 0
		counter = 0
		while 1:
			if self.parms["wholeword"]:
				wholewordRE = _makewholewordpattern(find)
				wholewordRE.search(text, pos)
				if wholewordRE.regs:
					pos = wholewordRE.regs[1][0]
				else:
					pos = -1
			else:
				pos = string.find(text, find, pos)
			if pos < 0:
				break
			counter = counter + 1
			text = text[:pos] + replace + text[pos + findlen:]
			Text = Text[:pos] + replace + Text[pos + findlen:]
			pos = pos + replacelen
		W.SetCursor("arrow")
		if counter:
			self.hide()
			import EasyDialogs
			import Res
			editor.changed = 1
			editor.selchanged = 1
			editor.ted.WEUseText(Res.Resource(Text))
			editor.ted.WECalText()
			editor.SetPort()
			Win.InvalRect(editor._bounds)
			#editor.ted.WEUpdate(self.w.wid.GetWindowPort().visRgn)
			EasyDialogs.Message("Replaced %d occurrences" % counter)
	
	def dont(self):
		self.getparmsfromwindow()
		self.hide()
	
	def replacefind(self):
		self.replace()
		self.findnext()
	
	def setfindstring(self):
		editor = findeditor(self)
		if not editor:
			return
		find = editor.getselectedtext()
		if not find:
			return
		self.parms["find"] = find
		if self.w:
			self.w.find.edit.set(self.parms["find"])
			self.w.find.edit.selectall()
	
	def findnext(self):
		editor = findeditor(self)
		if not editor:
			return
		find = self.parms["find"]
		if not find:
			return
		text = editor.get()
		if not self.parms["casesens"]:
			find = string.lower(find)
			text = string.lower(text)
		selstart, selend = editor.getselection()
		selstart, selend = min(selstart, selend), max(selstart, selend)
		if self.parms["wholeword"]:
			wholewordRE = _makewholewordpattern(find)
			wholewordRE.search(text, selend)
			if wholewordRE.regs:
				pos = wholewordRE.regs[1][0]
			else:
				pos = -1
		else:
			pos = string.find(text, find, selend)
		if pos >= 0:
			editor.setselection(pos, pos + len(find))
			return 1
		elif self.parms["wrap"]:
			if self.parms["wholeword"]:
				wholewordRE.search(text, 0)
				if wholewordRE.regs:
					pos = wholewordRE.regs[1][0]
				else:
					pos = -1
			else:
				pos = string.find(text, find)
			if selstart > pos >= 0:
				editor.setselection(pos, pos + len(find))
				return 1
	
	def setparms(self):
		for key, value in self.parms.items():
			try:
				self.w[key].set(value)
			except KeyError:
				self.w.boxes[key].set(value)
	
	def getparmsfromwindow(self):
		if not self.w:
			return
		for key, value in self.parms.items():
			try:
				value = self.w[key].get()
			except KeyError:
				value = self.w.boxes[key].get()
			self.parms[key] = value
	
	def cancel(self):
		self.hide()
		self.setparms()
	
	def hide(self):
		if self.w:
			self.w.wid.HideWindow()
			self.visible = 0
	
	def writeprefs(self):
		import MacPrefs
		self.getparmsfromwindow()
		prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
		prefs.searchengine.casesens = self.parms["casesens"]
		prefs.searchengine.wrap = self.parms["wrap"]
		prefs.searchengine.wholeword = self.parms["wholeword"]
		prefs.save()
	

class TitledEditText(W.Group):
	
	def __init__(self, possize, title, text = ""):
		W.Group.__init__(self, possize)
		self.title = W.TextBox((0, 0, 0, 16), title)
		self.edit = W.EditText((0, 16, 0, 0), text)
	
	def set(self, value):
		self.edit.set(value)
	
	def get(self):
		return self.edit.get()


class ClassFinder(W.PopupWidget):
	
	def click(self, point, modifiers):
		W.SetCursor("watch")
		self.set(self._parentwindow.getclasslist())
		W.PopupWidget.click(self, point, modifiers)


def getminindent(lines):
	indent = -1
	for line in lines:
		stripped = string.strip(line)
		if not stripped or stripped[0] == '#':
			continue
		if indent < 0 or line[:indent] <> indent * '\t':
			indent = 0
			for c in line:
				if c <> '\t':
					break
				indent = indent + 1
	return indent


def getoptionkey():
	return not not ord(Evt.GetKeys()[7]) & 0x04


def execstring(pytext, globals, locals, filename = "<string>", debugging = 0, 
			run_as_main = 0, profiling = 0):
	if debugging:
		import PyDebugger, bdb
		BdbQuit = bdb.BdbQuit
	else:
		BdbQuit = 'BdbQuitDummyException'
	pytext = string.split(pytext, '\r')
	pytext = string.join(pytext, '\n') + '\n'
	W.SetCursor("watch")
	modname = os.path.basename(filename)
	if modname[-3:] == '.py':
		modname = modname[:-3]
	if run_as_main:
		globals['__name__'] = '__main__'
	else:
		globals['__name__'] = modname
	globals['__file__'] = filename
	sys.argv = [filename]
	try:
		code = compile(pytext, filename, "exec")
	except:
		tracebackwindow.traceback(1, filename)
		return
	try:
		if debugging:
			PyDebugger.startfromhere()
		else:
			MacOS.EnableAppswitch(0)
		try:
			if profiling:
				import profile, ProfileBrowser
				p = profile.Profile(ProfileBrowser.timer)
				p.set_cmd(filename)
				try:
					p.runctx(code, globals, locals)
				finally:
					import pstats
					
					stats = pstats.Stats(p)
					ProfileBrowser.ProfileBrowser(stats)
			else:
				exec code in globals, locals
		finally:
			MacOS.EnableAppswitch(-1)
	except W.AlertError, detail:
		raise W.AlertError, detail
	except (KeyboardInterrupt, BdbQuit):
		pass
	except:
		if debugging:
			sys.settrace(None)
			PyDebugger.postmortem(sys.exc_type, sys.exc_value, sys.exc_traceback)
			return
		else:
			tracebackwindow.traceback(1, filename)
	if debugging:
		sys.settrace(None)
		PyDebugger.stop()


_identifieRE = regex.compile("[A-Za-z_][A-Za-z_0-9]*")

def _filename_as_modname(fname):
	if fname[-3:] == '.py':
		mname = fname[:-3]
		if _identifieRE.match(mname) == len(mname):
			return mname

def findeditor(topwindow, fromtop = 0):
	wid = Win.FrontWindow()
	if not fromtop:
		if topwindow.w and wid == topwindow.w.wid:
			wid = topwindow.w.wid.GetNextWindow()
	if not wid:
		return
	window = W.getapplication()._windows[wid]
	if not W.HasBaseClass(window, Editor):
		return
	return window.editgroup.editor


searchengine = SearchEngine()
tracebackwindow = Wtraceback.TraceBack()
