# Prelude to allow running this as a main program
def _init():
	import macresource
	import sys, os
	macresource.need('DITL', 468, "PythonIDE.rsrc")
	widgetrespathsegs = [sys.exec_prefix, "Mac", "Tools", "IDE", "Widgets.rsrc"]
	widgetresfile = os.path.join(*widgetrespathsegs)
	if not os.path.exists(widgetresfile):
		widgetrespathsegs = [os.pardir, "Tools", "IDE", "Widgets.rsrc"]
		widgetresfile = os.path.join(*widgetrespathsegs)
	refno = macresource.need('CURS', 468, widgetresfile)
	if os.environ.has_key('PYTHONIDEPATH'):
		# For development set this environment variable
		ide_path = os.environ['PYTHONIDEPATH']
	elif refno:
		# We're not a fullblown application
		idepathsegs = [sys.exec_prefix, "Mac", "Tools", "IDE"]
		ide_path = os.path.join(*idepathsegs)
		if not os.path.exists(ide_path):
			idepathsegs = [os.pardir, "Tools", "IDE"]
			for p in sys.path:
				ide_path = os.path.join(*([p]+idepathsegs))
				if os.path.exists(ide_path):
					break
		
	else:
		# We are a fully frozen application
		ide_path = sys.argv[0]
	if ide_path not in sys.path:
		sys.path.insert(0, ide_path)
		
if __name__ == '__main__':
	_init()
	
import W
import Wapplication
from Carbon import Evt
import EasyDialogs
import FrameWork

import sys
import string
import os
import urllib

import pimp

ELIPSES = '...'
		
USER_INSTALL_DIR = os.path.join(os.environ.get('HOME', ''),
								'Library',
								'Python',
								sys.version[:3],
								'site-packages')
								
class PackageManagerMain(Wapplication.Application):
	
	def __init__(self):
		self.preffilepath = os.path.join("Python", "Package Install Manager Prefs")
		Wapplication.Application.__init__(self, 'Pimp')
		from Carbon import AE
		from Carbon import AppleEvents
		
		AE.AEInstallEventHandler(AppleEvents.kCoreEventClass, AppleEvents.kAEOpenApplication, 
				self.ignoreevent)
		AE.AEInstallEventHandler(AppleEvents.kCoreEventClass, AppleEvents.kAEReopenApplication, 
				self.ignoreevent)
		AE.AEInstallEventHandler(AppleEvents.kCoreEventClass, AppleEvents.kAEPrintDocuments, 
				self.ignoreevent)
		AE.AEInstallEventHandler(AppleEvents.kCoreEventClass, AppleEvents.kAEQuitApplication, 
				self.quitevent)
		if 1:
			import PyConsole
			# With -D option (OSX command line only) keep stderr, for debugging the IDE
			# itself.
			debug_stderr = None
			if len(sys.argv) >= 2 and sys.argv[1] == '-D':
				debug_stderr = sys.stderr
				del sys.argv[1]
			PyConsole.installoutput()
			if debug_stderr:
				sys.stderr = debug_stderr
		self.opendoc(None)
		self.mainloop()
		
	def makeusermenus(self):
		m = Wapplication.Menu(self.menubar, "File")
		newitem = FrameWork.MenuItem(m, "Open Standard Database", "N", 'openstandard')
		openitem = FrameWork.MenuItem(m, "Open"+ELIPSES, "O", 'open')
		openURLitem = FrameWork.MenuItem(m, "Open URL"+ELIPSES, "D", 'openURL')
		FrameWork.Separator(m)
		closeitem = FrameWork.MenuItem(m, "Close", "W", 'close')
##		saveitem = FrameWork.MenuItem(m, "Save", "S", 'save')
##		saveasitem = FrameWork.MenuItem(m, "Save as"+ELIPSES, None, 'save_as')
##		FrameWork.Separator(m)
		
		m = Wapplication.Menu(self.menubar, "Edit")
		undoitem = FrameWork.MenuItem(m, "Undo", 'Z', "undo")
		FrameWork.Separator(m)
		cutitem = FrameWork.MenuItem(m, "Cut", 'X', "cut")
		copyitem = FrameWork.MenuItem(m, "Copy", "C", "copy")
		pasteitem = FrameWork.MenuItem(m, "Paste", "V", "paste")
		FrameWork.MenuItem(m, "Clear", None,  "clear")
		FrameWork.Separator(m)
		selallitem = FrameWork.MenuItem(m, "Select all", "A", "selectall")
		
		m = Wapplication.Menu(self.menubar, "Package")
		runitem = FrameWork.MenuItem(m, "Install", "I", 'install')
		homepageitem = FrameWork.MenuItem(m, "Visit Homepage", None, 'homepage')
		
		self.openwindowsmenu = Wapplication.Menu(self.menubar, 'Windows')
		self.makeopenwindowsmenu()
		self._menustocheck = [closeitem, 
				undoitem, cutitem, copyitem, pasteitem, 
				selallitem,
				runitem, homepageitem]
			
	def quitevent(self, theAppleEvent, theReply):
		self._quit()
		
	def ignoreevent(self, theAppleEvent, theReply):
		pass
	
	def opendocsevent(self, theAppleEvent, theReply):
		W.SetCursor('watch')
		import aetools
		parameters, args = aetools.unpackevent(theAppleEvent)
		docs = parameters['----']
		if type(docs) <> type([]):
			docs = [docs]
		for doc in docs:
			fsr, a = doc.FSResolveAlias(None)
			path = fsr.as_pathname()
			path = urllib.pathname2url(path)
			self.opendoc(path)
	
	def opendoc(self, url):
		PackageBrowser(url)
	
	def getabouttext(self):
		return "About Package Manager"+ELIPSES
	
	def do_about(self, id, item, window, event):
		EasyDialogs.Message("Package Install Manager for Python")
	
	def domenu_openstandard(self, *args):
		self.opendoc(None)
		
	def domenu_open(self, *args):
		filename = EasyDialogs.AskFileForOpen(typeList=("TEXT",))
		if filename:
			filename = urllib.pathname2url(filename)
			self.opendoc(filename)
			
	def domenu_openURL(self, *args):
		ok = EasyDialogs.AskYesNoCancel(
			"Warning: by opening a non-standard database "
			"you are trusting the maintainer of it "
			"to run arbitrary code on your machine.",
			yes="OK", no="")
		if ok <= 0: return
		url = EasyDialogs.AskString("URL of database to open:", ok="Open")
		if url:
			self.opendoc(url)
	
	def domenu_openbyname(self, *args):
		url = EasyDialogs.AskString("Open URL:", ok="Open")
		if url:
			self.opendoc(url)
		
	def makeopenwindowsmenu(self):
		for i in range(len(self.openwindowsmenu.items)):
			self.openwindowsmenu.menu.DeleteMenuItem(1)
			self.openwindowsmenu.items = []
		windows = []
		self._openwindows = {}
		for window in self._windows.keys():
			title = window.GetWTitle()
			if not title:
				title = "<no title>"
			windows.append((title, window))
		windows.sort()
		for title, window in windows:
			shortcut = None
			item = FrameWork.MenuItem(self.openwindowsmenu, title, shortcut, callback = self.domenu_openwindows)
			self._openwindows[item.item] = window
		self._openwindowscheckmark = 0
		self.checkopenwindowsmenu()
		
	def domenu_openwindows(self, id, item, window, event):
		w = self._openwindows[item]
		w.ShowWindow()
		w.SelectWindow()
	
	def domenu_quit(self):
		self._quit()
	
	def domenu_save(self, *args):
		print "Save"
	
	def _quit(self):
##		import PyConsole, PyEdit
		for window in self._windows.values():
			try:
				rv = window.close() # ignore any errors while quitting
			except:
				rv = 0	 # (otherwise, we can get stuck!)
			if rv and rv > 0:
				return
##		try:
##			PyConsole.console.writeprefs()
##			PyConsole.output.writeprefs()
##			PyEdit.searchengine.writeprefs()
##		except:
##			# Write to __stderr__ so the msg end up in Console.app and has
##			# at least _some_ chance of getting read...
##			# But: this is a workaround for way more serious problems with
##			# the Python 2.2 Jaguar addon.
##			sys.__stderr__.write("*** PythonIDE: Can't write preferences ***\n")
		self.quitting = 1
		
class PimpInterface:

	def setuppimp(self, url):
		self.pimpprefs = pimp.PimpPreferences()
		self.pimpdb = pimp.PimpDatabase(self.pimpprefs)
		self.pimpinstaller = pimp.PimpInstaller(self.pimpdb)
		if not url:
			url = self.pimpprefs.pimpDatabase
		try:
			self.pimpdb.appendURL(url)
		except IOError, arg:
			rv = "Cannot open %s: %s\n" % (url, arg)
			rv += "\nSee MacPython Package Manager help page."
			return rv
		# Check whether we can write the installation directory.
		# If not, set to the per-user directory, possibly
		# creating it, if needed.
		installDir = self.pimpprefs.installDir
		if not os.access(installDir, os.R_OK|os.W_OK|os.X_OK):
			rv = self.setuserinstall(1)
			if rv: return rv
		return self.pimpprefs.check()
		
	def closepimp(self):
		self.pimpdb.close()
		self.pimpprefs = None
		self.pimpdb = None
		self.pimpinstaller = None
		self.packages = []

	def setuserinstall(self, onoff):
		rv = ""
		if onoff:
			if not os.path.exists(USER_INSTALL_DIR):
				try:
					os.makedirs(USER_INSTALL_DIR)
				except OSError, arg:
					rv = rv + arg + "\n"
			if not USER_INSTALL_DIR in sys.path:
				import site
				reload(site)
			self.pimpprefs.setInstallDir(USER_INSTALL_DIR)
		else:
			self.pimpprefs.setInstallDir(None)
		rv = rv + self.pimpprefs.check()
		return rv
		
	def getuserinstall(self):
		return self.pimpprefs.installDir == USER_INSTALL_DIR
			
	def getbrowserdata(self, show_hidden=1):
		packages = self.pimpdb.list()
		if show_hidden:
			self.packages = packages
		else:
			self.packages = []
			for pkg in packages:
				name = pkg.fullname()
				if name[0] == '(' and name[-1] == ')' and not show_hidden:
					continue
				self.packages.append(pkg)			
		rv = []
		for pkg in self.packages:
			name = pkg.fullname()
			status, _ = pkg.installed()
			description = pkg.description()
			rv.append((status, name, description))
		return rv
		
	def getstatus(self, number):
		pkg = self.packages[number]
		return pkg.installed()
		
	def installpackage(self, sel, output, recursive, force):
		pkg = self.packages[sel]
		list, messages = self.pimpinstaller.prepareInstall(pkg, force, recursive)
		if messages:
			return messages
		messages = self.pimpinstaller.install(list, output)
		return messages
			
class PackageBrowser(PimpInterface):
	
	def __init__(self, url = None):
		self.ic = None
		messages = self.setuppimp(url)
		self.setupwidgets()
		self.updatestatus()
		self.showmessages(messages)
		
	def close(self):
		self.closepimp()
	
	def setupwidgets(self):
		INSTALL_POS = -30
		STATUS_POS = INSTALL_POS - 70
		self.w = W.Window((580, 400), "Python Install Manager", minsize = (400, 200), tabbable = 0)
		self.w.titlebar = W.TextBox((4, 8, 60, 18), 'Packages:')
		self.w.hidden_button = W.CheckBox((-100, 4, 0, 18), 'Show Hidden', self.updatestatus)
		data = self.getbrowserdata()
		self.w.packagebrowser = W.MultiList((4, 24, 0, STATUS_POS-2), data, self.listhit, cols=3)
		
		self.w.installed_l = W.TextBox((4, STATUS_POS, 60, 12), 'Installed:')
		self.w.installed = W.TextBox((64, STATUS_POS, 0, 12), '')
		self.w.message_l = W.TextBox((4, STATUS_POS+20, 60, 12), 'Status:')
		self.w.message = W.TextBox((64, STATUS_POS+20, 0, 12), '')
		self.w.homepage_button = W.Button((4, STATUS_POS+40, 96, 18), 'View homepage', self.do_homepage)
		
		self.w.divline = W.HorizontalLine((0, INSTALL_POS-4, 0, 0))
		self.w.verbose_button = W.CheckBox((84, INSTALL_POS+4, 60, 18), 'Verbose')
		self.w.recursive_button = W.CheckBox((146, INSTALL_POS+4, 120, 18), 'Install dependencies', self.updatestatus)
		self.w.recursive_button.set(1)
		self.w.force_button = W.CheckBox((268, INSTALL_POS+4, 70, 18), 'Overwrite', self.updatestatus)
		self.w.user_button = W.CheckBox((340, INSTALL_POS+4, 140, 18), 'For Current User Only', self.do_user)
		self.w.install_button = W.Button((4, INSTALL_POS+4, 56, 18), 'Install:', self.do_install)
		self.w.open()
		
	def updatestatus(self):
		sel = self.w.packagebrowser.getselection()
		data = self.getbrowserdata(self.w.hidden_button.get())
		self.w.packagebrowser.setitems(data)
		self.w.user_button.set(self.getuserinstall())
		if len(sel) != 1:
			self.w.installed.set('')
			self.w.message.set('')
			self.w.install_button.enable(0)
			self.w.homepage_button.enable(0)
			self.w.verbose_button.enable(0)
			self.w.recursive_button.enable(0)
			self.w.force_button.enable(0)
			self.w.user_button.enable(0)
		else:
			sel = sel[0]
			self.w.packagebrowser.setselection([sel])
			installed, message = self.getstatus(sel)
			self.w.installed.set(installed)
			self.w.message.set(message)
			self.w.install_button.enable(installed != "yes" or self.w.force_button.get())
			self.w.homepage_button.enable(not not self.packages[sel].homepage())
			self.w.verbose_button.enable(1)
			self.w.recursive_button.enable(1)
			self.w.force_button.enable(1)
			self.w.user_button.enable(1)
		
	def listhit(self, *args, **kwargs):
		self.updatestatus()
		
	def do_install(self):
		sel = self.w.packagebrowser.getselection()[0]
		if self.w.verbose_button.get():
			output = sys.stdout
		else:
			output = None
		recursive = self.w.recursive_button.get()
		force = self.w.force_button.get()
		messages = self.installpackage(sel, output, recursive, force)
		
		# Re-read .pth files
		import site
		reload(site)
		
		self.updatestatus()
		self.showmessages(messages)
		
	def showmessages(self, messages):
		if messages:
			if type(messages) == list:
				messages = '\n'.join(messages)
			if self.w.verbose_button.get():
				sys.stdout.write(messages + '\n')
			EasyDialogs.Message(messages)
		
	def do_homepage(self):
		sel = self.w.packagebrowser.getselection()[0]
		if not self.ic:
			import ic
			
			self.ic = ic.IC()
		self.ic.launchurl(self.packages[sel].homepage())
		
	def do_user(self):
		messages = self.setuserinstall(self.w.user_button.get())
		self.updatestatus()
		self.showmessages(messages)
		
if __name__ == '__main__':
	PackageManagerMain()
