import FrameWork
import EasyDialogs
import Res
import sys
import Qd
import Evt
import Events
import Dlg
import Win

# Resource-id (for checking existence)
ID_MODULES=512

ID_ABOUT=515

_arrow = Qd.qd.arrow
_watch = Qd.GetCursor(4).data

# Made available to TwitCore:
AskString = EasyDialogs.AskString

def SetCursor():
	Qd.SetCursor(_arrow)

def SetWatch():
	Qd.SetCursor(_watch)

# Exception for temporarily exiting the loop and program
ExitMainloop = 'ExitMainloop'
ExitFully = 'ExitFully'

class Application(FrameWork.Application):
	"""The twit main class - mac-dependent part"""

	def __init__(self, run_args, pm_args):
		# First init menus, etc.
		FrameWork.Application.__init__(self)
		self._quitting = 0
		self.real_quit = 0

		# Next create our dialogs
		self.mi_init(run_args, pm_args)
		if self.real_quit:
			return
		
		if not run_args:
			# Go into mainloop once
			self.one_mainloop()
			if self.real_quit:
				return
		
		if not pm_args:
			# And give the debugger control.
			self.to_debugger()

	def makeusermenus(self):
		self.filemenu = m = FrameWork.Menu(self.menubar, "File")
		self._quititem = FrameWork.MenuItem(m, "Quit", "Q", self.do_quit)
	
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
	
	def do_quit(self, *args):
		self._quit()			# Signal FrameWork.Application to stop
		self.real_quit = 1
		self.quit_bdb()			# Tell debugger to quit.
			
	def exit_mainloop(self):
		self._quit()			# Signal FrameWork.Application to stop
		self.real_quit = 0
		
	def one_mainloop(self):
		self.quitting = 0
		self.mainloop()
		
def Initialize():
	try:
		# if this doesn't raise an error, we are an applet containing the 
		# necessary resources or we have been initialized already
		# so we don't have to bother opening the resource file
		dummy = Res.GetResource('DLOG', ID_MODULES)
	except Res.Error:
		try:
			Res.OpenResFile("Twit.rsrc")
		except Res.Error, arg:
			EasyDialogs.Message("Cannot open Twit.rsrc: "+arg[1])
			sys.exit(1)

