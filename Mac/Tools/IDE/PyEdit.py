"""A (less & less) simple Python editor"""

import W
import Wtraceback
from Wkeys import *

import macfs
import MACFS
import MacOS
import Win
import Res
import Evt
import os
import imp
import sys
import string
import marshal
import re

try:
	import Wthreading
except ImportError:
	haveThreading = 0
else:
	haveThreading = Wthreading.haveThreading

_scriptuntitledcounter = 1
# _wordchars = string.letters + string.digits + "_"
_wordchars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_'


runButtonLabels = ["Run all", "Stop!"]
runSelButtonLabels = ["Run selection", "Pause!", "Resume"]


class Editor(W.Window):
	
	def __init__(self, path = "", title = ""):
		defaultfontsettings, defaulttabsettings, defaultwindowsize = geteditorprefs()
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
			path = resolvealiases(path)
			dir, name = os.path.split(path)
			self.title = name
			f = open(path, "rb")
			text = f.read()
			f.close()
			fss = macfs.FSSpec(path)
			self._creator, filetype = fss.GetCreatorType()
		else:
			raise IOError, "file '%s' does not exist" % path
		self.path = path
		
		if '\n' in text:
			import EasyDialogs
			if string.find(text, '\r\n') >= 0:
				sourceOS = 'DOS'
				searchString = '\r\n'
			else:
				sourceOS = 'UNIX'
				searchString = '\n'
			change = EasyDialogs.AskYesNoCancel('"%s" contains %s-style line feeds. '
					'Change them to MacOS carriage returns?' % (self.title, sourceOS), 1)
			# bug: Cancel is treated as No
			if change > 0:
				text = string.replace(text, searchString, '\r')
		else:
			change = 0
		
		self.settings = {}
		if self.path:
			self.readwindowsettings()
		if self.settings.has_key("windowbounds"):
			bounds = self.settings["windowbounds"]
		else:
			bounds = defaultwindowsize
		if self.settings.has_key("fontsettings"):
			self.fontsettings = self.settings["fontsettings"]
		else:
			self.fontsettings = defaultfontsettings
		if self.settings.has_key("tabsize"):
			try:
				self.tabsettings = (tabsize, tabmode) = self.settings["tabsize"]
			except:
				self.tabsettings = defaulttabsettings
		else:
			self.tabsettings = defaulttabsettings
		
		W.Window.__init__(self, bounds, self.title, minsize = (330, 120), tabbable = 0)
		self.setupwidgets(text)
		if change > 0:
			self.editgroup.editor.changed = 1
		
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
		if self.settings.has_key("run_with_interpreter"):
			self.run_with_interpreter = self.settings["run_with_interpreter"]
		else:
			self.run_with_interpreter = 0
		self._threadstate = (0, 0)
		self._thread = None
	
	def readwindowsettings(self):
		try:
			resref = Res.FSpOpenResFile(self.path, 1)
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
			resref = Res.FSpOpenResFile(self.path, 3)
		except Res.Error:
			Res.FSpCreateResFile(self.path, self._creator, 'TEXT', MACFS.smAllScripts)
			resref = Res.FSpOpenResFile(self.path, 3)
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
		self.settings["tabsize"] = self.editgroup.editor.gettabsettings()
		self.settings["run_as_main"] = self.run_as_main
		self.settings["run_with_interpreter"] = self.run_with_interpreter
	
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
		editor = W.PyEditor((0, 0, -15,-15), text, 
				fontsettings = self.fontsettings, 
				tabsettings = self.tabsettings,
				file = self.getfilename())
		
		# make the widgets
		self.popfield = ClassFinder((popfieldwidth - 17, -15, 16, 16), [], self.popselectline)
		self.linefield = W.EditText((-1, -15, popfieldwidth - 15, 16), inset = (6, 1))
		self.editgroup._barx = W.Scrollbar((popfieldwidth - 2, -15, -14, 16), editor.hscroll, max = 32767)
		self.editgroup._bary = W.Scrollbar((-15, 14, 16, -14), editor.vscroll, max = 32767)
		self.editgroup.editor = editor	# add editor *after* scrollbars
		
		self.editgroup.optionsmenu = W.PopupMenu((-15, -1, 16, 16), [])
		self.editgroup.optionsmenu.bind('<click>', self.makeoptionsmenu)
		
		self.bevelbox = W.BevelBox((0, 0, 0, topbarheight))
		self.hline = W.HorizontalLine((0, topbarheight, 0, 0))
		self.infotext = W.TextBox((175, 6, -4, 14), backgroundcolor = (0xe000, 0xe000, 0xe000))
		self.runbutton = W.Button((5, 4, 80, 16), runButtonLabels[0], self.run)
		self.runselbutton = W.Button((90, 4, 80, 16), runSelButtonLabels[0], self.runselection)
		
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
		menuitems = [('Font settings\xc9', self.domenu_fontsettings), 
				("Save options\xc9", self.domenu_options),
				'-',
				('\0' + chr(self.run_as_main) + 'Run as __main__', self.domenu_toggle_run_as_main), 
				#('\0' + chr(self.run_with_interpreter) + 'Run with Interpreter', self.domenu_toggle_run_with_interpreter), 
				#'-',
				('Modularize', self.domenu_modularize),
				('Browse namespace\xc9', self.domenu_browsenamespace), 
				'-']
		if self.profiling:
			menuitems = menuitems + [('Disable profiler', self.domenu_toggleprofiler)]
		else:
			menuitems = menuitems + [('Enable profiler', self.domenu_toggleprofiler)]
		if self.editgroup.editor._debugger:
			menuitems = menuitems + [('Disable debugger', self.domenu_toggledebugger),
				('Clear breakpoints', self.domenu_clearbreakpoints),
				('Edit breakpoints\xc9', self.domenu_editbreakpoints)]
		else:
			menuitems = menuitems + [('Enable debugger', self.domenu_toggledebugger)]
		self.editgroup.optionsmenu.set(menuitems)
	
	def domenu_toggle_run_as_main(self):
		self.run_as_main = not self.run_as_main
		self.run_with_interpreter = 0
		self.editgroup.editor.selchanged = 1
	
	def domenu_toggle_run_with_interpreter(self):
		self.run_with_interpreter = not self.run_with_interpreter
		self.run_as_main = 0
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
		globals, file, modname = self.getenvironment()
		if not modname:
			modname = self.title
		PyBrowser.Browser(globals, "Object browser: " + modname)
	
	def domenu_modularize(self, *args):
		modname = _filename_as_modname(self.title)
		if not modname:
			raise W.AlertError, "Can't modularize \"%s\"" % self.title
		run_as_main = self.run_as_main
		self.run_as_main = 0
		self.run()
		self.run_as_main = run_as_main
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
		fontsettings = self.editgroup.editor.getfontsettings()
		tabsettings = self.editgroup.editor.gettabsettings()
		settings = FontSettings.FontDialog(fontsettings, tabsettings)
		if settings:
			fontsettings, tabsettings = settings
			self.editgroup.editor.setfontsettings(fontsettings)
			self.editgroup.editor.settabsettings(tabsettings)
	
	def domenu_options(self, *args):
		rv = SaveOptions(self._creator)
		if rv:
			self.editgroup.editor.selchanged = 1 # ouch...
			self._creator = rv
	
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
			Qd.InitCursor()
			save = EasyDialogs.AskYesNoCancel('Save window "%s" before closing?' % self.title,
					default=1, no="Don\xd5t save")
			if save > 0:
				if self.domenu_save():
					return 1
			elif save < 0:
				return 1
		self.globals = None
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
	
	def domenu_save_as_applet(self, *args):
		import buildtools
		
		buildtools.DEBUG = 0	# ouch.
		
		if self.title[-3:] == ".py":
			destname = self.title[:-3]
		else:
			destname = self.title + ".applet"
		fss, ok = macfs.StandardPutFile('Save as Applet:', destname)
		if not ok: 
			return 1
		W.SetCursor("watch")
		destname = fss.as_pathname()
		if self.path:
			filename = self.path
			if filename[-3:] == ".py":
				rsrcname = filename[:-3] + '.rsrc'
			else:
				rsrcname = filename + '.rsrc'
		else:
			filename = self.title
			rsrcname = ""
		
		pytext = self.editgroup.editor.get()
		pytext = string.split(pytext, '\r')
		pytext = string.join(pytext, '\n') + '\n'
		try:
			code = compile(pytext, filename, "exec")
		except (SyntaxError, EOFError):
			raise buildtools.BuildError, "Syntax error in script %s" % `filename`
		
		# Try removing the output file
		try:
			os.remove(destname)
		except os.error:
			pass
		template = buildtools.findtemplate()
		buildtools.process_common(template, None, code, rsrcname, destname, 0, 1)
	
	def domenu_gotoline(self, *args):
		self.linefield.selectall()
		self.linefield.select(1)
		self.linefield.selectall()
	
	def domenu_selectline(self, *args):
		self.editgroup.editor.expandselection()
	
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
		if self._threadstate == (0, 0):
			self._run()
		else:
			lock = Wthreading.Lock()
			lock.acquire()
			self._thread.postException(KeyboardInterrupt)
			if self._thread.isBlocked():
				self._thread.start()
			lock.release()
	
	def _run(self):
		if self.run_with_interpreter:
			if self.editgroup.editor.changed:
				import EasyDialogs
				import Qd; Qd.InitCursor()
				save = EasyDialogs.AskYesNoCancel('Save "%s" before running?' % self.title, 1)
				if save > 0:
					if self.domenu_save():
						return
				elif save < 0:
					return
			if not self.path:
				raise W.AlertError, "Can't run unsaved file"
			self._run_with_interpreter()
		else:
			pytext = self.editgroup.editor.get()
			globals, file, modname = self.getenvironment()
			self.execstring(pytext, globals, globals, file, modname)
	
	def _run_with_interpreter(self):
		interp_path = os.path.join(sys.exec_prefix, "PythonInterpreter")
		if not os.path.exists(interp_path):
			raise W.AlertError, "Can't find interpreter"
		import findertools
		XXX
	
	def runselection(self):
		if self._threadstate == (0, 0):
			self._runselection()
		elif self._threadstate == (1, 1):
			self._thread.block()
			self.setthreadstate((1, 2))
		elif self._threadstate == (1, 2):
			self._thread.start()
			self.setthreadstate((1, 1))
	
	def _runselection(self):
		if self.run_with_interpreter:
			raise W.AlertError, "Can't run selection with Interpreter"
		globals, file, modname = self.getenvironment()
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
			for i in range(selfirstline - 1, -1, -1):
				line = alllines[i]
				if line[:6] == 'class ':
					classname = string.split(string.strip(line[6:]))[0]
					classend = identifieRE_match(classname)
					if classend < 1:
						raise W.AlertError, "Can't find a class."
					classname = classname[:classend]
					break
				elif line and line[0] not in '\t#':
					raise W.AlertError, "Can't find a class."
			else:
				raise W.AlertError, "Can't find a class."
			if globals.has_key(classname):
				klass = globals[classname]
			else:
				raise W.AlertError, "Can't find class \"%s\"." % classname
			# add class def
			pytext = ("class %s:\n" % classname) + pytext
			selfirstline = selfirstline - 1
		elif indent > 0:
			raise W.AlertError, "Can't run indented code."
		
		# add "newlines" to fool compile/exec: 
		# now a traceback will give the right line number
		pytext = selfirstline * '\r' + pytext
		self.execstring(pytext, globals, locals, file, modname)
		if indent == 1 and globals[classname] is not klass:
			# update the class in place
			klass.__dict__.update(globals[classname].__dict__)
			globals[classname] = klass
	
	def setthreadstate(self, state):
		oldstate = self._threadstate
		if oldstate[0] <> state[0]:
			self.runbutton.settitle(runButtonLabels[state[0]])
		if oldstate[1] <> state[1]:
			self.runselbutton.settitle(runSelButtonLabels[state[1]])
		self._threadstate = state
	
	def _exec_threadwrapper(self, *args, **kwargs):
		apply(execstring, args, kwargs)
		self.setthreadstate((0, 0))
		self._thread = None
	
	def execstring(self, pytext, globals, locals, file, modname):
		tracebackwindow.hide()
		# update windows
		W.getapplication().refreshwindows()
		if self.run_as_main:
			modname = "__main__"
		if self.path:
			dir = os.path.dirname(self.path)
			savedir = os.getcwd()
			os.chdir(dir)
			sys.path.insert(0, dir)
		else:
			cwdindex = None
		try:
			if haveThreading:
				self._thread = Wthreading.Thread(os.path.basename(file), 
							self._exec_threadwrapper, pytext, globals, locals, file, self.debugging, 
							modname, self.profiling)
				self.setthreadstate((1, 1))
				self._thread.start()
			else:
				execstring(pytext, globals, locals, file, self.debugging, 
							modname, self.profiling)
		finally:
			if self.path:
				os.chdir(savedir)
				del sys.path[0]
	
	def getenvironment(self):
		if self.path:
			file = self.path
			dir = os.path.dirname(file)
			# check if we're part of a package
			modname = ""
			while os.path.exists(os.path.join(dir, "__init__.py")):
				dir, dirname = os.path.split(dir)
				modname = dirname + '.' + modname
			subname = _filename_as_modname(self.title)
			if modname:
				if subname == "__init__":
					# strip trailing period
					modname = modname[:-1]
				else:
					modname = modname + subname
			else:
				modname = subname
			if sys.modules.has_key(modname):
				globals = sys.modules[modname].__dict__
				self.globals = {}
			else:
				globals = self.globals
				modname = subname
		else:
			file = '<%s>' % self.title
			globals = self.globals
			modname = file
		return globals, file, modname
	
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
		methodRE = re.compile(r"\r[ \t]+def ")
		findMethod = methodRE.search
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
			m = findMethod(text, pos + 1)
			if m is None:
				break
			pos = m.regs[0][0]
			#pos = find(text, '\r\tdef ', pos + 1)
			append((m.regs[0][1], methodtag))
		list.sort()
		classlist = []
		methodlistappend = None
		offsetToLine = editor.ted.WEOffsetToLine
		getLineRange = editor.ted.WEGetLineRange
		append = classlist.append
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

class _saveoptions:
	
	def __init__(self, creator):
		self.rv = None
		self.w = w = W.ModalDialog((240, 140), 'Save options')
		radiobuttons = []
		w.label = W.TextBox((8, 8, 80, 18), "File creator:")
		w.ide_radio = W.RadioButton((8, 22, 160, 18), "This application", radiobuttons, self.ide_hit)
		w.interp_radio = W.RadioButton((8, 42, 160, 18), "Python Interpreter", radiobuttons, self.interp_hit)
		w.other_radio = W.RadioButton((8, 62, 50, 18), "Other:", radiobuttons)
		w.other_creator = W.EditText((62, 62, 40, 20), creator, self.otherselect)
		w.cancelbutton = W.Button((-180, -30, 80, 16), "Cancel", self.cancelbuttonhit)
		w.okbutton = W.Button((-90, -30, 80, 16), "Done", self.okbuttonhit)
		w.setdefaultbutton(w.okbutton)
		if creator == 'Pyth':
			w.interp_radio.set(1)
		elif creator == W._signature:
			w.ide_radio.set(1)
		else:
			w.other_radio.set(1)
		w.bind("cmd.", w.cancelbutton.push)
		w.open()
	
	def ide_hit(self):
		self.w.other_creator.set(W._signature)
	
	def interp_hit(self):
		self.w.other_creator.set("Pyth")
	
	def otherselect(self, *args):
		sel_from, sel_to = self.w.other_creator.getselection()
		creator = self.w.other_creator.get()[:4]
		creator = creator + " " * (4 - len(creator))
		self.w.other_creator.set(creator)
		self.w.other_creator.setselection(sel_from, sel_to)
		self.w.other_radio.set(1)
	
	def cancelbuttonhit(self):
		self.w.close()
	
	def okbuttonhit(self):
		self.rv = self.w.other_creator.get()[:4]
		self.w.close()


def SaveOptions(creator):
	s = _saveoptions(creator)
	return s.rv


def _escape(where, what) : 
	return string.join(string.split(where, what), '\\' + what)

def _makewholewordpattern(word):
	# first, escape special regex chars
	for esc in "\\[]()|.*^+$?":
		word = _escape(word, esc)
	notwordcharspat = '[^' + _wordchars + ']'
	pattern = '(' + word + ')'
	if word[0] in _wordchars:
		pattern = notwordcharspat + pattern
	if word[-1] in _wordchars:
		pattern = pattern + notwordcharspat
	return re.compile(pattern)

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
					("Don't find",  "cmdd",	 self.dont), 
					("Cancel",	      "cmd.",	 self.cancel)
				]
		for i in range(len(self.buttons)):
			bounds = -90, 22 + i * 24, 80, 16
			title, shortcut, callback = self.buttons[i]
			self.w[title] = W.Button(bounds, title, callback)
			if shortcut:
				self.w.bind(shortcut, self.w[title].push)
		self.w.setdefaultbutton(self.w["Don't find"])
		self.w.find.edit.bind("<key>", self.key)
		self.w.bind("<activate>", self.activate)
		self.w.bind("<close>", self.close)
		self.w.open()
		self.setparms()
		self.w.find.edit.select(1)
		self.w.find.edit.selectall()
		self.checkbuttons()
	
	def close(self):
		self.hide()
		return -1
	
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
				self.w.setdefaultbutton(self.w["Don't find"])
		else:
			for title, cmd, call in self.buttons[:-2]:
				self.w[title].enable(0)
			self.w.setdefaultbutton(self.w["Don't find"])
	
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
				match = wholewordRE.search(text, pos)
				if match:
					pos = match.start(1)
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
			editor.GetWindow().InvalWindowRect(editor._bounds)
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
			match = wholewordRE.search(text, selend)
			if match:
				pos = match.start(1)
			else:
				pos = -1
		else:
			pos = string.find(text, find, selend)
		if pos >= 0:
			editor.setselection(pos, pos + len(find))
			return 1
		elif self.parms["wrap"]:
			if self.parms["wholeword"]:
				match = wholewordRE.search(text, 0)
				if match:
					pos = match.start(1)
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


def execstring(pytext, globals, locals, filename="<string>", debugging=0, 
			modname="__main__", profiling=0):
	if debugging:
		import PyDebugger, bdb
		BdbQuit = bdb.BdbQuit
	else:
		BdbQuit = 'BdbQuitDummyException'
	pytext = string.split(pytext, '\r')
	pytext = string.join(pytext, '\n') + '\n'
	W.SetCursor("watch")
	globals['__name__'] = modname
	globals['__file__'] = filename
	sys.argv = [filename]
	try:
		code = compile(pytext, filename, "exec")
	except:
		# XXXX BAAAADDD.... We let tracebackwindow decide to treat SyntaxError 
		# special. That's wrong because THIS case is special (could be literal 
		# overflow!) and SyntaxError could mean we need a traceback (syntax error 
		# in imported module!!!
		tracebackwindow.traceback(1, filename)
		return
	try:
		if debugging:
			if haveThreading:
				lock = Wthreading.Lock()
				lock.acquire()
				PyDebugger.startfromhere()
				lock.release()
			else:
				PyDebugger.startfromhere()
		elif not haveThreading:
			MacOS.EnableAppswitch(0)
		try:
			if profiling:
				import profile, ProfileBrowser
				p = profile.Profile()
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
			if not haveThreading:
				MacOS.EnableAppswitch(-1)
	except W.AlertError, detail:
		raise W.AlertError, detail
	except (KeyboardInterrupt, BdbQuit):
		pass
	except:
		if haveThreading:
			import continuation
			lock = Wthreading.Lock()
			lock.acquire()
		if debugging:
			sys.settrace(None)
			PyDebugger.postmortem(sys.exc_type, sys.exc_value, sys.exc_traceback)
			return
		else:
			tracebackwindow.traceback(1, filename)
		if haveThreading:
			lock.release()
	if debugging:
		sys.settrace(None)
		PyDebugger.stop()


_identifieRE = re.compile(r"[A-Za-z_][A-Za-z_0-9]*")

def identifieRE_match(str):
	match = _identifieRE.match(str)
	if not match:
		return -1
	return match.end()

def _filename_as_modname(fname):
	if fname[-3:] == '.py':
		modname = fname[:-3]
		match = _identifieRE.match(modname)
		if match and match.start() == 0 and match.end() == len(modname):
			return string.join(string.split(modname, '.'), '_')

def findeditor(topwindow, fromtop = 0):
	wid = Win.FrontWindow()
	if not fromtop:
		if topwindow.w and wid == topwindow.w.wid:
			wid = topwindow.w.wid.GetNextWindow()
	if not wid:
		return
	app = W.getapplication()
	if app._windows.has_key(wid): # KeyError otherwise can happen in RoboFog :-(
		window = W.getapplication()._windows[wid]
	else:
		return
	if not isinstance(window, Editor):
		return
	return window.editgroup.editor


class _EditorDefaultSettings:
	
	def __init__(self):
		self.template = "%s, %d point"
		self.fontsettings, self.tabsettings, self.windowsize = geteditorprefs()
		self.w = W.Dialog((328, 120), "Editor default settings")
		self.w.setfontbutton = W.Button((8, 8, 80, 16), "Set font\xc9", self.dofont)
		self.w.fonttext = W.TextBox((98, 10, -8, 14), self.template % (self.fontsettings[0], self.fontsettings[2]))
		
		self.w.picksizebutton = W.Button((8, 50, 80, 16), "Front window", self.picksize)
		self.w.xsizelabel = W.TextBox((98, 32, 40, 14), "Width:")
		self.w.ysizelabel = W.TextBox((148, 32, 40, 14), "Height:")
		self.w.xsize = W.EditText((98, 48, 40, 20), `self.windowsize[0]`)
		self.w.ysize = W.EditText((148, 48, 40, 20), `self.windowsize[1]`)
		
		self.w.cancelbutton = W.Button((-180, -26, 80, 16), "Cancel", self.cancel)
		self.w.okbutton = W.Button((-90, -26, 80, 16), "Done", self.ok)
		self.w.setdefaultbutton(self.w.okbutton)
		self.w.bind('cmd.', self.w.cancelbutton.push)
		self.w.open()
	
	def picksize(self):
		app = W.getapplication()
		editor = findeditor(self)
		if editor is not None:
			width, height = editor._parentwindow._bounds[2:]
			self.w.xsize.set(`width`)
			self.w.ysize.set(`height`)
		else:
			raise W.AlertError, "No edit window found"
	
	def dofont(self):
		import FontSettings
		settings = FontSettings.FontDialog(self.fontsettings, self.tabsettings)
		if settings:
			self.fontsettings, self.tabsettings = settings
			sys.exc_traceback = None
			self.w.fonttext.set(self.template % (self.fontsettings[0], self.fontsettings[2]))
	
	def close(self):
		self.w.close()
		del self.w
	
	def cancel(self):
		self.close()
	
	def ok(self):
		try:
			width = string.atoi(self.w.xsize.get())
		except:
			self.w.xsize.select(1)
			self.w.xsize.selectall()
			raise W.AlertError, "Bad number for window width"
		try:
			height = string.atoi(self.w.ysize.get())
		except:
			self.w.ysize.select(1)
			self.w.ysize.selectall()
			raise W.AlertError, "Bad number for window height"
		self.windowsize = width, height
		seteditorprefs(self.fontsettings, self.tabsettings, self.windowsize)
		self.close()

def geteditorprefs():
	import MacPrefs
	prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
	try:
		fontsettings = prefs.pyedit.fontsettings
		tabsettings = prefs.pyedit.tabsettings
		windowsize = prefs.pyedit.windowsize
	except:
		fontsettings = prefs.pyedit.fontsettings = ("Python-Sans", 0, 9, (0, 0, 0))
		tabsettings = prefs.pyedit.tabsettings = (8, 1)
		windowsize = prefs.pyedit.windowsize = (500, 250)
		sys.exc_traceback = None
	return fontsettings, tabsettings, windowsize

def seteditorprefs(fontsettings, tabsettings, windowsize):
	import MacPrefs
	prefs = MacPrefs.GetPrefs(W.getapplication().preffilepath)
	prefs.pyedit.fontsettings = fontsettings
	prefs.pyedit.tabsettings = tabsettings
	prefs.pyedit.windowsize = windowsize
	prefs.save()

_defaultSettingsEditor = None

def EditorDefaultSettings():
	global _defaultSettingsEditor
	if _defaultSettingsEditor is None or not hasattr(_defaultSettingsEditor, "w"):
		_defaultSettingsEditor = _EditorDefaultSettings()
	else:
		_defaultSettingsEditor.w.select()

def resolvealiases(path):
	try:
		return macfs.ResolveAliasFile(path)[0].as_pathname()
	except (macfs.error, ValueError), (error, str):
		if error <> -120:
			raise
		dir, file = os.path.split(path)
		return os.path.join(resolvealiases(dir), file)

searchengine = SearchEngine()
tracebackwindow = Wtraceback.TraceBack()
