# Standard main loop for *all* STDWIN applications.
# This requires that applications:
# - register their windows on creation and unregister them when closed
# - have a 'dispatch' function as a window member


import stdwin, stdwinq
from stdwinevents import *


# List of windows known to the main loop.
#
windows = []


# Function to register a window.
#
def register(win):
	# First test the dispatch function by passing it a null event --
	# this catches registration of unconforming windows.
	win.dispatch(WE_NULL, win, None)
	if win not in windows:
		windows.append(win)


# Function to unregister a window.
# It is not an error to unregister an already unregistered window
# (this is useful for cleanup actions).
#
def unregister(win):
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


# Event processing main loop.
# Return when there are no windows left, or when an unhandled
# exception occurs.  (It is safe to restart the main loop after
# an unsuccessful exit.)
# Python's stdwin.getevent() turns WE_COMMAND/WC_CANCEL events
# into KeyboardInterrupt exceptions; these are turned back in events.
#
def mainloop():
	while windows:
		try:
			dispatch(stdwinq.getevent())
		except KeyboardInterrupt:
			dispatch(WE_COMMAND, stdwin.getactive(), WC_CANCEL)


# Dispatch a single event.
# Windows not in the windows list don't get their events:
# events for such windows are silently ignored.
#
def dispatch(event):
	if event[1] in windows:
		event[1].dispatch(event)
