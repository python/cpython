import FrameWork
import MiniAEFrame
import EasyDialogs
import AE
import AppleEvents
import Res
import sys
import Qd
import Evt
import Events
import Dlg
import Win
import Menu
import TwitCore
import mactwit_mod
import mactwit_stack
import mactwit_browser
import mactwit_edit
import macfs
import string

# Resource-id (for checking existence)
ID_MODULES=500

ID_ABOUT=502

_arrow = Qd.qd.arrow
_watch = Qd.GetCursor(4).data

class Twit(FrameWork.Application, TwitCore.Application, MiniAEFrame.AEServer):
	"""The twit main class - mac-dependent part"""

	def __init__(self, sessiontype, arg=None):
		# First init menus, etc.
		self.app_menu_bar = Menu.GetMenuBar()
		FrameWork.Application.__init__(self)
		MiniAEFrame.AEServer.__init__(self)
		AE.AESetInteractionAllowed(AppleEvents.kAEInteractWithAll)
		self.installaehandler('aevt', 'odoc', self.ae_open_doc)
		self.installaehandler('aevt', 'quit', self.do_quit)
		self.installaehandler('pyth', 'EXEC', self.do_bbpyexec) # BBpy execute event

		self.dbg_menu_bar = Menu.GetMenuBar()
		self.setstate(sessiontype)
		self._quitting = 0
		self.real_quit = 0
		self.window_aware = 1

		# Next create our dialogs
		self.mi_init(sessiontype, arg)
		while 1:
			if self.real_quit:
				break
			if self.initial_cmd:
				self.to_debugger()	# Will get to mainloop via debugger
			else:
				self.one_mainloop()	# Else do it ourselves.
				
	def switch_to_app(self):
		if not self.window_aware:
			return
		self.dbg_menu_bar = Menu.GetMenuBar()
		Menu.SetMenuBar(self.app_menu_bar)
		Menu.DrawMenuBar()
		
	def switch_to_dbg(self):
		if not self.window_aware:
			return
		self.app_menu_bar = Menu.GetMenuBar()
		Menu.SetMenuBar(self.dbg_menu_bar)
		Menu.DrawMenuBar()
		self.run_dialog.force_redraw()
		if self.module_dialog:
			self.module_dialog.force_redraw()

	def makeusermenus(self):
		self.filemenu = m = FrameWork.Menu(self.menubar, "Debug")
		self._openitem = FrameWork.MenuItem(m, "Run File...", "O", self.do_open)
		self._runitem = FrameWork.MenuItem(m, "Run String...", "R", self.do_run)
		FrameWork.Separator(m)
		self._awareitem = FrameWork.MenuItem(m, "Window-aware", "", self.do_aware)
		self._awareitem.check(1)
		FrameWork.Separator(m)
		self._quititem = FrameWork.MenuItem(m, "Quit", "Q", self.do_quit)
		
		self.controlmenu = m = FrameWork.Menu(self.menubar, "Control")
		self._stepitem = FrameWork.MenuItem(m, "Step Next", "N", self.do_step)
		self._stepinitem = FrameWork.MenuItem(m, "Step In", "S", self.do_stepin)
		self._stepoutitem = FrameWork.MenuItem(m, "Step Out", "U", self.do_stepout)
		self._continueitem = FrameWork.MenuItem(m, "Continue", "G", self.do_continue)
		FrameWork.Separator(m)
		self._killitem = FrameWork.MenuItem(m, "Kill", "K", self.do_kill)
		
	def setstate(self, state):
		self.state = state
		if state == 'run':
			self._stepitem.enable(1)
			self._stepoutitem.enable(1)
			self._stepinitem.enable(1)
			self._continueitem.enable(1)
			self._killitem.enable(1)
		else:
			self._stepitem.enable(0)
			self._stepoutitem.enable(0)
			self._stepinitem.enable(0)
			self._continueitem.enable(0)
			self._killitem.enable(0)
			
	def asknewsession(self):
		if self.state == 'none':
			return 1
		if EasyDialogs.AskYesNoCancel("Abort current debug session?") == 1:
			self.quit_bdb()
			return 1
		return 0

	def do_about(self, id, item, window, event):
		import time
		d = Dlg.GetNewDialog(ID_ABOUT, -1)
		if not d:
			return
		w = d.GetDialogWindow()
		port = w.GetWindowPort()
		l, t, r, b = port.portRect
		sl, st, sr, sb = Qd.qd.screenBits.bounds
		x = ((sr-sl) - (r-l)) / 2
		y = ((sb-st-16) - (b-t)) / 5
		w.MoveWindow(x, y, 0)
		w.ShowWindow()
		d.DrawDialog()
		
		tp, h, rect = d.GetDialogItem(2)
		x0, y0, x1, y1 = rect
		ybot = y0 + 32
		
		rgn = Qd.NewRgn()
		Qd.SetPort(d)
		ok, evt = self.getevent(Events.mDownMask|Events.keyDownMask, 1)
		if ok: return
		(what, message, when, where, modifiers) = event
		delta_t = 128
		nexttime = when+delta_t
		while ybot < y1:
			# Do the animation, if it is time
			if when > nexttime:
				Qd.ScrollRect((x0, y0, x1, ybot), 0, 1, rgn)
				y0 = y0 + 1
				ybot = ybot + 1
				# And update next time
				delta_t = int(delta_t*0.6)-1
				if delta_t < 0:
					delta_t = 0
				nexttime = when + delta_t
			# Check for an event.
			ok, evt = self.getevent(Events.mDownMask|Events.keyDownMask, 0)
			if ok: return
			(what, message, when, where, modifiers) = evt
		while 1:
			ok, evt = self.getevent(Events.mDownMask|Events.keyDownMask, -1)
			if ok: return
			
	def do_open(self, *args):
		if not self.asknewsession():
			return
		fss, ok = macfs.StandardGetFile('TEXT')
		if not ok: return
		self.runfile(fss.as_pathname())
		
	def ae_open_doc(self, object=None, **args):
		if not object: return
		if self.state <> 'none':
			if AE.AEInteractWithUser(AppleEvents.kAEDefaultTimeout) == 0:
				if not self.asknewsession():
					return
		if type(object) == type([]):
			object = object[0]
		fss, changed = object.Resolve()
		self.runfile(fss.as_pathname())
		
	def do_bbpyexec(self, object=None, NAME=None, **args):
		if type(object) <> type(''):
			if AE.AEInteractWithUser(AppleEvents.kAEDefaultTimeout) == 0:
				EasyDialogs.Message('EXEC AppleEvent arg should be a string')
			return
		if self.state <> 'none':
			if AE.AEInteractWithUser(AppleEvents.kAEDefaultTimeout) == 0:
				if not self.asknewsession():
					return
		stuff = string.splitfields(object, '\r')
		stuff = string.joinfields(stuff, '\n')
		self.runstring(stuff)
			
	def do_run(self, *args):
		if not self.asknewsession():
			return
		self.run()
		
	def do_aware(self, *args):
		self.window_aware = not self.window_aware
		self._awareitem.check(self.window_aware)
		
	def do_quit(self, *args):
		self._quit()			# Signal FrameWork.Application to stop
		self.real_quit = 1
		self.quit_bdb()			# Tell debugger to quit.

	def do_step(self, *args):
		self.run_dialog.click_step()
		
	def do_stepin(self, *args):
		self.run_dialog.click_step_in()
		
	def do_stepout(self, *args):
		self.run_dialog.click_step_out()
		
	def do_continue(self, *args):
		self.run_dialog.click_continue()
		
	def do_kill(self, *args):
		self.run_dialog.click_kill()
					
	def exit_mainloop(self):
		self._quit()			# Signal FrameWork.Application to stop
		self.real_quit = 0
		
	def one_mainloop(self):
		self.quitting = 0
		self.mainloop()

	def SetCursor(self):
		Qd.SetCursor(_arrow)
	
	def SetWatch(self):
		Qd.SetCursor(_watch)
		
	def AskString(self, *args):
		return apply(EasyDialogs.AskString, args)
		
	def Message(self, *args):
		return apply(EasyDialogs.Message, args)

	def new_module_browser(self, parent):
		return mactwit_mod.ModuleBrowser(parent)
		
	def new_stack_browser(self, parent):
		return mactwit_stack.StackBrowser(parent)
		
	def new_var_browser(self, parent, var):
		return mactwit_browser.VarBrowser(parent).open(var)
	
	def edit(self, file, line):
		return mactwit_edit.edit(file, line)
	
		
def Initialize():
	try:
		# if this doesn't raise an error, we are an applet containing the 
		# necessary resources or we have been initialized already
		# so we don't have to bother opening the resource file
		dummy = Res.GetResource('DLOG', ID_MODULES)
	except Res.Error:
		try:
			Res.FSpOpenResFile("Twit.rsrc", 1)
		except Res.Error, arg:
			EasyDialogs.Message("Cannot open Twit.rsrc: "+arg[1])
			sys.exit(1)

