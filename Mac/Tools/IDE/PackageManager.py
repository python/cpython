import W
import Wapplication
from Carbon import Evt
import EasyDialogs
import FrameWork

import sys
import string
import os

import pimp

ELIPSES = '...'
		
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
##		newitem = FrameWork.MenuItem(m, "Open Standard Database", "N", 'openstandard')
##		openitem = FrameWork.MenuItem(m, "Open"+ELIPSES, "O", 'open')
##		openbynameitem = FrameWork.MenuItem(m, "Open URL"+ELIPSES, "D", 'openbyname')
		FrameWork.Separator(m)
		closeitem = FrameWork.MenuItem(m, "Close", "W", 'close')
##		saveitem = FrameWork.MenuItem(m, "Save", "S", 'save')
##		saveasitem = FrameWork.MenuItem(m, "Save as"+ELIPSES, None, 'save_as')
		FrameWork.Separator(m)
		
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
		self._menustocheck = [closeitem, saveasitem, 
				undoitem, cutitem, copyitem, pasteitem, 
				selallitem,
				runitem, homepageitem]
			
	def quitevent(self, theAppleEvent, theReply):
		from Carbon import AE
		AE.AEInteractWithUser(50000000)
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
			
	def domenu_open(self, *args):
		filename = EasyDialogs.AskFileForOpen(typeList=("TEXT",))
		if filename:
			filename = urllib.pathname2url(filename)
			self.opendoc(filename)
	
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
				rv = 0   # (otherwise, we can get stuck!)
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
		self.pimpdb.appendURL(url)

	def getbrowserdata(self):
		self.packages = self.pimpdb.list()
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
		self.setuppimp(url)
		self.setupwidgets()
		self.updatestatus()
	
	def setupwidgets(self): 
		self.w = W.Window((580, 400), "Python Install Manager", minsize = (300, 200), tabbable = 0)
##		self.w.divline = W.HorizontalLine((0, 20, 0, 0))
		self.w.titlebar = W.TextBox((4, 4, 40, 12), 'Packages:')
		data = self.getbrowserdata()
		self.w.packagebrowser = W.MultiList((4, 20, 0, -70), data, self.listhit, cols=3)
		self.w.installed_l = W.TextBox((4, -66, 60, 12), 'Installed:')
		self.w.installed = W.TextBox((64, -66, 0, 12), '')
		self.w.message_l = W.TextBox((4, -48, 60, 12), 'Status:')
		self.w.message = W.TextBox((64, -48, 0, 12), '')
		self.w.homepage_button = W.Button((4, -28, 96, 18), 'View homepage', self.do_homepage)
		self.w.verbose_button = W.CheckBox((-288, -26, 60, 18), 'Verbose')
		self.w.recursive_button = W.CheckBox((-224, -26, 80, 18), 'Recursive', self.updatestatus)
		self.w.recursive_button.set(1)
		self.w.force_button = W.CheckBox((-140, -26, 60, 18), 'Force', self.updatestatus)
		self.w.install_button = W.Button((-76, -28, 56, 18), 'Install', self.do_install)
		self.w.open()
		
	def updatestatus(self):
		sel = self.w.packagebrowser.getselection()
		data = self.getbrowserdata()
		self.w.packagebrowser.setitems(data)
		if len(sel) != 1:
			self.w.installed.set('')
			self.w.message.set('')
			self.w.install_button.enable(0)
			self.w.homepage_button.enable(0)
			self.w.verbose_button.enable(0)
			self.w.recursive_button.enable(0)
			self.w.force_button.enable(0)
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
		self.updatestatus()
		if messages:
			EasyDialogs.Message('\n'.join(messages))
		
	def do_homepage(self):
		sel = self.w.packagebrowser.getselection()[0]
		if not self.ic:
			import ic
			
			self.ic = ic.IC()
		self.ic.launchurl(self.packages[sel].homepage())
		
if __name__ == '__main__':
	PackageManagerMain()
