#! /usr/local/bin/python

# A STDWIN-based front end for the Python interpreter.
#
# This is useful if you want to avoid console I/O and instead
# use text windows to issue commands to the interpreter.
#
# It supports multiple interpreter windows, each with its own context.
#
# BUGS AND CAVEATS:
#
# This was written long ago as a demonstration, and slightly hacked to
# keep it up-to-date, but never as an industry-strength alternative
# interface to Python.  It should be rewritten using more classes, and
# merged with something like wdb.
#
# Although this supports multiple windows, the whole application
# is deaf and dumb when a command is running in one window.
#
# Interrupt is (ab)used to signal EOF on input requests.
#
# On UNIX (using X11), interrupts typed in the window will not be
# seen until the next input or output operation.  When you are stuck
# in an infinite loop, try typing ^C in the shell window where you
# started this interpreter.  (On the Mac, interrupts work normally.)


import sys
import stdwin
from stdwinevents import *
import rand
import mainloop
import os


# Stack of windows waiting for [raw_]input().
# Element [0] is the top.
# If there are multiple windows waiting for input, only the
# one on top of the stack can accept input, because the way
# raw_input() is implemented (using recursive mainloop() calls).
#
inputwindows = []


# Exception raised when input is available
#
InputAvailable = 'input available for raw_input (not an error)'


# Main program -- create the window and call the mainloop
#
def main():
	# Hack so 'import python' won't load another copy
	# of this if we were loaded though 'python python.py'.
	# (Should really look at sys.argv[0]...)
	if 'inputwindows' in dir(sys.modules['__main__']) and \
			sys.modules['__main__'].inputwindows is inputwindows:
		sys.modules['python'] = sys.modules['__main__']
	#
	win = makewindow()
	mainloop.mainloop()


# Create a new window
#
def makewindow():
	# stdwin.setdefscrollbars(0, 1) # Not in Python 0.9.1
	# stdwin.setfont('monaco') # Not on UNIX! and not Python 0.9.1
	# width, height = stdwin.textwidth('in')*40, stdwin.lineheight()*24
	# stdwin.setdefwinsize(width, height)
	win = stdwin.open('Python interpreter ready')
	win.editor = win.textcreate((0,0), win.getwinsize())
	win.globals = {}		# Dictionary for user's globals
	win.command = ''		# Partially read command
	win.busy = 0			# Ready to accept a command
	win.auto = 1			# [CR] executes command
	win.insertOutput = 1		# Insert output at focus
	win.insertError = 1		# Insert error output at focus
	win.setwincursor('ibeam')
	win.filename = ''		# Empty if no file for this window
	makefilemenu(win)
	makeeditmenu(win)
	win.dispatch = pdispatch	# Event dispatch function
	mainloop.register(win)
	return win


# Make a 'File' menu
#
def makefilemenu(win):
	win.filemenu = mp = win.menucreate('File')
	mp.callback = []
	additem(mp, 'New', 'N', do_new)
	additem(mp, 'Open...', 'O', do_open)
	additem(mp, '', '',  None)
	additem(mp, 'Close', 'W', do_close)
	additem(mp, 'Save', 'S', do_save)
	additem(mp, 'Save as...', '', do_saveas)
	additem(mp, '', '', None)
	additem(mp, 'Quit', 'Q', do_quit)


# Make an 'Edit' menu
#
def makeeditmenu(win):
	win.editmenu = mp = win.menucreate('Edit')
	mp.callback = []
	additem(mp, 'Cut', 'X', do_cut)
	additem(mp, 'Copy', 'C', do_copy)
	additem(mp, 'Paste', 'V', do_paste)
	additem(mp, 'Clear', '',  do_clear)
	additem(mp, '', '', None)
	win.iauto = len(mp.callback)
	additem(mp, 'Autoexecute', '', do_auto)
	mp.check(win.iauto, win.auto)
	win.insertOutputNum = len(mp.callback)
	additem(mp, 'Insert Output', '', do_insertOutputOption)
	win.insertErrorNum = len(mp.callback)
	additem(mp, 'Insert Error', '', do_insertErrorOption)
	additem(mp, 'Exec', '\r', do_exec)


# Helper to add a menu item and callback function
#
def additem(mp, text, shortcut, handler):
	if shortcut:
		mp.additem(text, shortcut)
	else:
		mp.additem(text)
	mp.callback.append(handler)


# Dispatch a single event to the interpreter.
# Resize events cause a resize of the editor.
# Some events are treated specially.
# Most other events are passed directly to the editor.
#
def pdispatch(event):
	type, win, detail = event
	if not win:
		win = stdwin.getactive()
		if not win: return
	if type == WE_CLOSE:
		do_close(win)
		return
	elif type == WE_SIZE:
		win.editor.move((0, 0), win.getwinsize())
	elif type == WE_COMMAND and detail == WC_RETURN:
		if win.auto:
			do_exec(win)
		else:
			void = win.editor.event(event)
	elif type == WE_COMMAND and detail == WC_CANCEL:
		if win.busy:
			raise KeyboardInterrupt
		else:
			win.command = ''
			settitle(win)
	elif type == WE_MENU:
		mp, item = detail
		mp.callback[item](win)
	else:
		void = win.editor.event(event)
	if win in mainloop.windows:
		# May have been deleted by close...
		win.setdocsize(0, win.editor.getrect()[1][1])
		if type in (WE_CHAR, WE_COMMAND):
			win.editor.setfocus(win.editor.getfocus())


# Helper to set the title of the window
#
def settitle(win):
	if win.filename == '':
		win.settitle('Python interpreter ready')
	else:
		win.settitle(win.filename)


# Helper to replace the text of the focus
#
def replace(win, text):
	win.editor.replace(text)
	# Resize the window to display the text
	win.setdocsize(0, win.editor.getrect()[1][1]) # update the size before
	win.editor.setfocus(win.editor.getfocus()) # move focus to the change


# File menu handlers
#
def do_new(win):
	win = makewindow()
#
def do_open(win):
	try:
		filename = stdwin.askfile('Open file', '', 0)
		win = makewindow()
		win.filename = filename
		win.editor.replace(open(filename, 'r').read())
		win.editor.setfocus(0, 0)
		win.settitle(win.filename)
		#
	except KeyboardInterrupt:
		pass			# Don't give an error on cancel
#
def do_save(win):
	try:
		if win.filename == '':
			win.filename = stdwin.askfile('Open file', '', 1)
		f = open(win.filename, 'w')
		f.write(win.editor.gettext())
		#
	except KeyboardInterrupt:
		pass			# Don't give an error on cancel
	
def do_saveas(win):
	currentFilename = win.filename
	win.filename = ''
	do_save(win)		# Use do_save with empty filename
	if win.filename == '':	# Restore the name if do_save did not set it
		win.filename = currentFilename
#
def do_close(win):
	if win.busy:
		stdwin.message('Can\'t close busy window')
		return		# need to fail if quitting??
	win.editor = None # Break circular reference
	#del win.editmenu	# What about the filemenu??
	mainloop.unregister(win)
	win.close()
#
def do_quit(win):
	# Call win.dispatch instead of do_close because there
	# may be 'alien' windows in the list.
	for win in mainloop.windows[:]:
		mainloop.dispatch((WE_CLOSE, win, None))
		# need to catch failed close


# Edit menu handlers
#
def do_cut(win):
	text = win.editor.getfocustext()
	if not text:
		stdwin.fleep()
		return
	stdwin.setcutbuffer(0, text)
	replace(win, '')
#
def do_copy(win):
	text = win.editor.getfocustext()
	if not text:
		stdwin.fleep()
		return
	stdwin.setcutbuffer(0, text)
#
def do_paste(win):
	text = stdwin.getcutbuffer(0)
	if not text:
		stdwin.fleep()
		return
	replace(win, text)
#
def do_clear(win):
	replace(win, '')


# These would be better in a preferences dialog:
#
def do_auto(win):
	win.auto = (not win.auto)
	win.editmenu.check(win.iauto, win.auto)
#
def do_insertOutputOption(win):
	win.insertOutput = (not win.insertOutput)
	title = ['Append Output', 'Insert Output'][win.insertOutput]
	win.editmenu.setitem(win.insertOutputNum, title)
#
def do_insertErrorOption(win):
	win.insertError = (not win.insertError)
	title = ['Error Dialog', 'Insert Error'][win.insertError]
	win.editmenu.setitem(win.insertErrorNum, title)


# Extract a command from the editor and execute it, or pass input to
# an interpreter waiting for it.
# Incomplete commands are merely placed in the window's command buffer.
# All exceptions occurring during the execution are caught and reported.
# (Tracebacks are currently not possible, as the interpreter does not
# save the traceback pointer until it reaches its outermost level.)
#
def do_exec(win):
	if win.busy:
		if win not in inputwindows:
			stdwin.message('Can\'t run recursive commands')
			return
		if win <> inputwindows[0]:
			stdwin.message('Please complete recursive input first')
			return
	#
	# Set text to the string to execute.
	a, b = win.editor.getfocus()
	alltext = win.editor.gettext()
	n = len(alltext)
	if a == b:
		# There is no selected text, just an insert point;
		# so execute the current line.
		while 0 < a and alltext[a-1] <> '\n': # Find beginning of line
			a = a-1
		while b < n and alltext[b] <> '\n': # Find end of line after b
			b = b+1
		text = alltext[a:b] + '\n'
	else:
		# Execute exactly the selected text.
		text = win.editor.getfocustext()
		if text[-1:] <> '\n': # Make sure text ends with \n
			text = text + '\n'
		while b < n and alltext[b] <> '\n': # Find end of line after b
			b = b+1
	#
	# Set the focus to expect the output, since there is always something.
	# Output will be inserted at end of line after current focus,
	# or appended to the end of the text.
	b = [n, b][win.insertOutput]
	win.editor.setfocus(b, b)
	#
	# Make sure there is a preceeding newline.
	if alltext[b-1:b] <> '\n':
		win.editor.replace('\n')
	#
	#
	if win.busy:
		# Send it to raw_input() below
		raise InputAvailable, text
	#
	# Like the real Python interpreter, we want to execute
	# single-line commands immediately, but save multi-line
	# commands until they are terminated by a blank line.
	# Unlike the real Python interpreter, we don't do any syntax
	# checking while saving up parts of a multi-line command.
	#
	# The current heuristic to determine whether a command is
	# the first line of a multi-line command simply checks whether
	# the command ends in a colon (followed by a newline).
	# This is not very robust (comments and continuations will
	# confuse it), but it is usable, and simple to implement.
	# (It even has the advantage that single-line loops etc.
	# don't need te be terminated by a blank line.)
	#
	if win.command:
		# Already continuing
		win.command = win.command + text
		if win.command[-2:] <> '\n\n':
			win.settitle('Unfinished command...')
			return # Need more...
	else:
		# New command
		win.command = text
		if text[-2:] == ':\n':
			win.settitle('Unfinished command...')
			return
	command = win.command
	win.command = ''
	win.settitle('Executing command...')
	#
	# Some hacks:
	# - The standard files are replaced by an IOWindow instance.
	# - A 2nd argument to exec() is used to specify the directory
	#   holding the user's global variables.  (If this wasn't done,
	#   the exec would be executed in the current local environment,
	#   and the user's assignments to globals would be lost...)
	#
	save_stdin = sys.stdin
	save_stdout = sys.stdout
	save_stderr = sys.stderr
	try:
		sys.stdin = sys.stdout = sys.stderr = IOWindow(win)
		win.busy = 1
		try:
			exec(command, win.globals)
		except KeyboardInterrupt:
			print '[Interrupt]'
		except:
			msg = sys.exc_type
			if sys.exc_value <> None:
				msg = msg + ': ' + `sys.exc_value`
			if win.insertError:
				stdwin.fleep()
				replace(win, msg + '\n')
			else:
				win.settitle('Unhandled exception')
				stdwin.message(msg)
	finally:
		# Restore redirected I/O in *all* cases
		win.busy = 0
		sys.stderr = save_stderr
		sys.stdout = save_stdout
		sys.stdin = save_stdin
		settitle(win)


# Class emulating file I/O from/to a window
#
class IOWindow:
	#
	def __init__(self, win):
		self.win = win
	#
	def readline(self, *unused_args):
		n = len(inputwindows)
		save_title = self.win.gettitle()
		title = n*'(' + 'Requesting input...' + ')'*n
		self.win.settitle(title)
		inputwindows.insert(0, self.win)
		try:
			try:
				mainloop.mainloop()
			finally:
				del inputwindows[0]
				self.win.settitle(save_title)
		except InputAvailable, val: # See do_exec above
			return val
		except KeyboardInterrupt:
			raise EOFError # Until we have a "send EOF" key
		# If we didn't catch InputAvailable, something's wrong...
		raise EOFError
	#
	def write(self, text):
		mainloop.check()
		replace(self.win, text)
		mainloop.check()


# Currently unused function to test a command's syntax without executing it
#
def testsyntax(s):
	import string
	lines = string.splitfields(s, '\n')
	for i in range(len(lines)): lines[i] = '\t' + lines[i]
	lines.insert(0, 'if 0:')
	lines.append('')
	exec(string.joinfields(lines, '\n'))


# Call the main program
#
main()
