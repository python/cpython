# Standard main loop for *all* STDWIN applications.
# This requires that applications:
# - register their windows on creation and unregister them when closed
# - have a 'dispatch' function as a window member


import stdwin, stdwinq
from stdwinevents import *


# List of windows known to the main loop.
#
windows = []


# Last window that ever received an event
#
last_window = None


# Function to register a window.
#
def register(win):
	# First test the dispatch function by passing it a null event --
	# this catches registration of unconforming windows.
	win.dispatch((WE_NULL, win, None))
	if win not in windows:
		windows.append(win)


# Function to unregister a window.
# It is not an error to unregister an already unregistered window
# (this is useful for cleanup actions).
#
def unregister(win):
	global last_window
	if win == last_window:
		last_window = None
	if win in windows:
		windows.remove(win) # Not in 0.9.1
		# 0.9.1 solution:
		#for i in range(len(windows)):
		#	if windows[i] = win:
		#		del windows[i]
		#		break


# Interfaces used by WindowSched.
#
def countwindows():
	return len(windows)
#
def anywindow():
	if windows:
		return windows[0]
	else:
		return None


# NEW: register any number of file descriptors
#
fdlist = []
select_args = None
select_handlers = None
#
def registerfd(fd, mode, handler):
	if mode not in ('r', 'w', 'x'):
		raise ValueError, 'mode must be r, w or x'
	if type(fd) <> type(0):
		fd = fd.fileno() # If this fails it's not a proper select arg
	for i in range(len(fdlist)):
		if fdlist[i][:2] == (fd, mode):
			raise ValueError, \
				'(fd, mode) combination already registered'
	fdlist.append((fd, mode, handler))
	make_select_args()
#
def unregisterfd(fd, *args):
	if type(fd) <> type(0):
		fd = fd.fileno() # If this fails it's not a proper select arg
	args = (fd,) + args
	n = len(args)
	for i in range(len(fdlist)):
		if fdlist[i][:n] == args:
			del fdlist[i]
	make_select_args()
#
def make_select_args():
	global select_args, select_handlers
	rlist, wlist, xlist = [], [], []
	rhandlers, whandlers, xhandlers = {}, {}, {}
	for fd, mode, handler in fdlist:
		if mode == 'r':
			rlist.append(fd)
			rhandlers[`fd`] = handler
		if mode == 'w':
			wlist.append(fd)
			whandlers[`fd`] = handler
		if mode == 'x':
			xlist.append(fd)
			xhandlers[`fd`] = handler
	if rlist or wlist or xlist:
		select_args = rlist, wlist, xlist
		select_handlers = rhandlers, whandlers, xhandlers
	else:
		select_args = None
		select_handlers = None
#
def do_select():
	import select
	reply = apply(select.select, select_args)
	for mode in 0, 1, 2:
		list = reply[mode]
		for fd in list:
			handler = select_handlers[mode][`fd`]
			handler(fd, 'rwx'[mode])


# Event processing main loop.
# Return when there are no windows left, or when an unhandled
# exception occurs.  (It is safe to restart the main loop after
# an unsuccessful exit.)
# Python's stdwin.getevent() turns WE_COMMAND/WC_CANCEL events
# into KeyboardInterrupt exceptions; these are turned back in events.
#
recursion_level = 0 # Hack to make it reentrant
def mainloop():
	global recursion_level
	recursion_level = recursion_level + 1
	try:
		stdwin_select_handler() # Process events already in queue
		while 1:
			if windows and not fdlist:
				while windows and not fdlist:
					try:
						event = stdwinq.getevent()
					except KeyboardInterrupt:
						event = (WE_COMMAND, \
							 None, WC_CANCEL)
					dispatch(event)
			elif windows and fdlist:
				fd = stdwin.fileno()
				if recursion_level == 1:
				    registerfd(fd, 'r', stdwin_select_handler)
				try:
					while windows:
						do_select()
						stdwin_select_handler()
				finally:
					if recursion_level == 1:
						unregisterfd(fd)
			elif fdlist:
				while fdlist and not windows:
					do_select()
			else:
				break
	finally:
		recursion_level = recursion_level - 1


# Check for events without ever blocking
#
def check():
	stdwin_select_handler()
	# XXX Should check for socket stuff as well


# Handle stdwin events until none are left
#
def stdwin_select_handler(*args):
	while 1:
		try:
			event = stdwinq.pollevent()
		except KeyboardInterrupt:
			event = (WE_COMMAND, None, WC_CANCEL)
		if event is None:
			break
		dispatch(event)


# Run a modal dialog loop for a window.  The dialog window must have
# been registered first.  This prohibits most events (except size/draw
# events) to other windows.  The modal dialog loop ends when the
# dialog window unregisters itself.
#
passthrough = WE_SIZE, WE_DRAW
beeping = WE_MOUSE_DOWN, WE_COMMAND, WE_CHAR, WE_KEY, WE_CLOSE, WE_MENU
#
def modaldialog(window):
	if window not in windows:
		raise ValueError, 'modaldialog window not registered'
	while window in windows:
		try:
			event = stdwinq.getevent()
		except KeyboardInterrupt:
			event = WE_COMMAND, None, WC_CANCEL
		etype, ewindow, edetail = event
		if etype not in passthrough and ewindow <> window:
			if etype in beeping:
				stdwin.fleep()
			continue
		dispatch(event)


# Dispatch a single event.
# Events for the no window in particular are sent to the active window
# or to the last window that received an event (these hacks are for the
# WE_LOST_SEL event, which is directed to no particular window).
# Windows not in the windows list don't get their events:
# events for such windows are silently ignored.
#
def dispatch(event):
	global last_window
	if event[1] == None:
		active = stdwin.getactive()
		if active: last_window = active
	else:
		last_window = event[1]
	if last_window in windows:
		last_window.dispatch(event)


# Dialog base class
#
class Dialog:
	#
	def init(self, title):
		self.window = stdwin.open(title)
		self.window.dispatch = self.dispatch
		register(self.window)
		return self
	#
	def close(self):
		unregister(self.window)
		del self.window.dispatch
		self.window.close()
	#
	def dispatch(self, event):
		etype, ewindow, edetail = event
		if etype == WE_CLOSE:
			self.close()


# Standard modal dialogs
# XXX implemented using stdwin dialogs for now
#
def askstr(prompt, default):
	return stdwin.askstr(prompt, default)
#
def askync(prompt, yesorno):
	return stdwin.askync(prompt, yesorno)
#
def askfile(prompt, default, new):
	return stdwin.askfile(prompt, default, new)
#
def message(msg):
	stdwin.message(msg)
