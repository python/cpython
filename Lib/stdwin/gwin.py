# Module 'gwin'
# Generic stdwin windows

# This is used as a base class from which to derive other window types.
# The mainloop() function here is an event dispatcher for all window types.

import stdwin
from stdwinevents import *

# XXX Old version of stdwinevents, should go
import stdwinsupport
S = stdwinsupport			# Shorthand

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
		treatevent(stdwin.getevent())

def treatevent(e):			# Handle a stdwin event
	type, w, detail = e
	if type = S.we_draw:
		w.draw(w, detail)
	elif type = S.we_menu:
		m, item = detail
		m.action[item](w, m, item)
	elif type = S.we_command:
		treatcommand(w, detail)
	elif type = S.we_char:
		w.char(w, detail)
	elif type = S.we_mouse_down:
		if detail[1] > 1: w.m2down(w, detail)
		else: w.mdown(w, detail)
	elif type = S.we_mouse_move:
		w.mmove(w, detail)
	elif type = S.we_mouse_up:
		if detail[1] > 1: w.m2up(w, detail)
		else: w.mup(w, detail)
	elif type = S.we_size:
		w.size(w, w.getwinsize())
	elif type = S.we_activate:
		w.activate(w)
	elif type = S.we_deactivate:
		w.deactivate(w)
	elif type = S.we_move:
		w.move(w)
	elif type = S.we_timer:
		w.timer(w)
	elif type = WE_CLOSE:
		w.close(w)

def treatcommand(w, type):		# Handle a we_command event
	if type = S.wc_close:
		w.close(w)
	elif type = S.wc_return:
		w.enter(w)
	elif type = S.wc_tab:
		w.tab(w)
	elif type = S.wc_backspace:
		w.backspace(w)
	elif type in (S.wc_left, S.wc_up, S.wc_right, S.wc_down):
		w.arrow(w, type)


# Methods

def close(w):				# Close method
	for i in range(len(windows)):
		if windows[i] is w:
			del windows[i]
			break

def arrow(w, detail):			# Arrow key method
	if detail = S.wc_left:
		w.kleft(w)
	elif detail = S.wc_up:
		w.kup(w)
	elif detail = S.wc_right:
		w.kright(w)
	elif detail = S.wc_down:
		w.kdown(w)


# Trivial methods

def tab(w): w.char(w, '\t')
def enter(w): w.char(w, '\n')		# 'return' is a Python reserved word
def backspace(w): w.char(w, '\b')
def m2down(w, detail): w.mdown(w, detail)
def m2up(w, detail): w.mup(w, detail)
def nop(args): pass
