# Module 'gwin'
# Generic stdwin windows

# This is used as a base class from which to derive other window types.
# The mainloop() function here is an event dispatcher for all window types.

# XXX This is really obsoleted by "mainloop.py".
# XXX Also you should to it class-oriented...

import stdwin, stdwinq
from stdwinevents import *

windows = []				# List of open windows


# Open a window

def open(title):			# Open a generic window
	w = stdwin.open(title)
	stdwin.setdefwinsize(0, 0)
	# Set default event handlers
	w.draw = nop
	w.char = nop
	w.mdown = nop
	w.mmove = nop
	w.mup = nop
	w.m2down = m2down
	w.m2up = m2up
	w.size = nop
	w.move = nop
	w.activate = w.deactivate = nop
	w.timer = nop
	# default command handlers
	w.close = close
	w.tab = tab
	w.enter = enter
	w.backspace = backspace
	w.arrow = arrow
	w.kleft = w.kup = w.kright = w.kdown = nop
	windows.append(w)
	return w


# Generic event dispatching

def mainloop():				# Handle events until no windows left
	while windows:
		treatevent(stdwinq.getevent())

def treatevent(e):			# Handle a stdwin event
	type, w, detail = e
	if type == WE_DRAW:
		w.draw(w, detail)
	elif type == WE_MENU:
		m, item = detail
		m.action[item](w, m, item)
	elif type == WE_COMMAND:
		treatcommand(w, detail)
	elif type == WE_CHAR:
		w.char(w, detail)
	elif type == WE_MOUSE_DOWN:
		if detail[1] > 1: w.m2down(w, detail)
		else: w.mdown(w, detail)
	elif type == WE_MOUSE_MOVE:
		w.mmove(w, detail)
	elif type == WE_MOUSE_UP:
		if detail[1] > 1: w.m2up(w, detail)
		else: w.mup(w, detail)
	elif type == WE_SIZE:
		w.size(w, w.getwinsize())
	elif type == WE_ACTIVATE:
		w.activate(w)
	elif type == WE_DEACTIVATE:
		w.deactivate(w)
	elif type == WE_MOVE:
		w.move(w)
	elif type == WE_TIMER:
		w.timer(w)
	elif type == WE_CLOSE:
		w.close(w)

def treatcommand(w, type):		# Handle a we_command event
	if type == WC_CLOSE:
		w.close(w)
	elif type == WC_RETURN:
		w.enter(w)
	elif type == WC_TAB:
		w.tab(w)
	elif type == WC_BACKSPACE:
		w.backspace(w)
	elif type in (WC_LEFT, WC_UP, WC_RIGHT, WC_DOWN):
		w.arrow(w, type)


# Methods

def close(w):				# Close method
	for i in range(len(windows)):
		if windows[i] is w:
			del windows[i]
			break

def arrow(w, detail):			# Arrow key method
	if detail == WC_LEFT:
		w.kleft(w)
	elif detail == WC_UP:
		w.kup(w)
	elif detail == WC_RIGHT:
		w.kright(w)
	elif detail == WC_DOWN:
		w.kdown(w)


# Trivial methods

def tab(w): w.char(w, '\t')
def enter(w): w.char(w, '\n')		# 'return' is a Python reserved word
def backspace(w): w.char(w, '\b')
def m2down(w, detail): w.mdown(w, detail)
def m2up(w, detail): w.mup(w, detail)
def nop(args): pass
