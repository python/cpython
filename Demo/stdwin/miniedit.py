#! /usr/local/python

# A miniature multi-window editor using STDWIN's text objects.
#
# Usage: miniedit [file] ...
#
# The user interface is similar to that of the miniedit demo application
# in C that comes with STDWIN.
#
# XXX need to comment the functions
# XXX Not yet implemented:
#	disabling menu entries for inapplicable actions
#	Find operations


import sys
import stdwin
from stdwinevents import *


# Constant: list of WE_COMMAND events that (may) change the text buffer
# so we can decide whether to set the 'changed' flag.
# Note that it is possible for such a command to fail (a backspace
# at the beginning of the buffer) but we'll set the changed flag anyway
# -- it's too complicated to check this condition right now.
#
changing = [WC_RETURN, WC_TAB, WC_BACKSPACE]


# The list of currently open windows;
# this is maintained so we can stop when there are no windows left
#
windows = []


# A note on window data attributes (set by open_window):
#
# w.textobject	the window's text object
# w.changed	true when the window's text is changed
# w.filename	filename connected to the window; '' if none


# Main program
#
def main():
	#
	# Set a reasonable default window size.
	# If we are using a fixed-width font this will open a 80x24 window;
	# for variable-width fonts we approximate this based on an average
	#
	stdwin.setdefwinsize(40*stdwin.textwidth('in'), 24*stdwin.lineheight())
	#
	# Create global menus (as local variables)
	#
	filemenu = make_file_menu(stdwin)
	editmenu = make_edit_menu(stdwin)
	findmenu = make_find_menu(stdwin)
	#
	# Get the list of files from the command line (maybe none)
	#
	files = sys.argv[1:]
	#
	# Open any files -- errors will be reported but do won't stop us
	#
	for filename in files:
		open_file(filename)
	#
	# If there were no files, or none of them could be opened,
	# put up a dialog asking for a filename
	#
	if not windows:
		try:
			open_dialog(None)
		except KeyboardInterrupt:
			pass		# User cancelled
	#
	# If the dialog was cancelled, create an empty new window
	#
	if not windows:
		new_window(None)
	#
	# Main event loop -- stop when we have no open windows left
	#
	while windows:
		#
		# Get the next event -- ignore interrupts
		#
		try:
			type, window, detail = event = stdwin.getevent()
		except KeyboardInterrupt:
			type, window, detail = event = WE_NONE, None, None
		#
		# Event decoding switch
		#
		if not window:
			pass		# Ignore such events
		elif type == WE_MENU:
			#
			# Execute menu operation
			#
			menu, item = detail
			try:
				menu.actions[item](window)
			except KeyboardInterrupt:
				pass	# User cancelled
		elif type == WE_CLOSE:
			#
			# Close a window
			#
			try:
				close_dialog(window)
			except KeyboardInterrupt:
				pass	# User cancelled
		elif type == WE_SIZE:
			#
			# A window was resized --
			# let the text object recompute the line breaks
			# and change the document size accordingly,
			# so scroll bars will work
			#
			fix_textsize(window)
		elif window.textobject.event(event):
			#
			# The event was eaten by the text object --
			# set the changed flag if not already set
			#
			if type == WE_CHAR or \
			   type == WE_COMMAND and detail in changing:
				window.changed = 1
				fix_docsize(window)
		#
		# Delete all objects that may still reference the window
		# in the event -- this is needed otherwise the window
		# won't actually be closed and may receive further
		# events, which will confuse the event decoder
		#
		del type, window, detail, event


def make_file_menu(object):
	menu = object.menucreate('File')
	menu.actions = []
	additem(menu, 'New',		'N', new_window)
	additem(menu, 'Open..',		'O', open_dialog)
	additem(menu, '',		'',  None)
	additem(menu, 'Save',		'S', save_dialog)
	additem(menu, 'Save As..',	'',  save_as_dialog)
	additem(menu, 'Save a Copy..',	'',  save_copy_dialog)
	additem(menu, 'Revert',		'R', revert_dialog)
	additem(menu, 'Quit',		'Q', quit_dialog)
	return menu


def make_edit_menu(object):
	menu = object.menucreate('Edit')
	menu.actions = []
	additem(menu, 'Cut',		'X', do_cut)
	additem(menu, 'Copy',		'C', do_copy)
	additem(menu, 'Paste',		'V', do_paste)
	additem(menu, 'Clear',		'B', do_clear)
	additem(menu, 'Select All',	'A', do_select_all)
	return menu


def make_find_menu(object):
	menu = object.menucreate('Find')
	menu.actions = []
	# XXX
	return menu


def additem(menu, text, shortcut, function):
	if shortcut:
		menu.additem(text, shortcut)
	else:
		menu.additem(text)
	menu.actions.append(function)


def open_dialog(current_ignored):
	filename = stdwin.askfile('Open file:', '', 0)
	open_file(filename)


def open_file(filename):
	try:
		fp = open(filename, 'r')
	except RuntimeError:
		stdwin.message(filename + ': cannot open')
		return			# Error, forget it
	try:
		contents = fp.read()
	except RuntimeError:
		stdwin.message(filename + ': read error')
		return			# Error, forget it
	del fp				# Close the file
	open_window(filename, filename, contents)


def new_window(current_ignored):
	open_window('', 'Untitled', '')


def open_window(filename, title, contents):
	try:
		window = stdwin.open(title)
	except RuntimeError:
		stdwin.message('cannot open new window')
		return			# Error, forget it
	window.textobject = window.textcreate((0, 0), window.getwinsize())
	window.textobject.settext(contents)
	window.changed = 0
	window.filename = filename
	fix_textsize(window)
	windows.append(window)


def quit_dialog(window):
	for window in windows[:]:
		close_dialog(window)


def close_dialog(window):
	if window.changed:
		prompt = 'Save changes to ' + window.gettitle() + ' ?'
		if stdwin.askync(prompt, 1):
			save_dialog(window)
			if window.changed:
				return	# Save failed (not) cancelled
	windows.remove(window)
	del window.textobject


def save_dialog(window):
	if not window.filename:
		save_as_dialog(window)
		return
	if save_file(window, window.filename):
		window.changed = 0


def save_as_dialog(window):
	prompt = 'Save ' + window.gettitle() + ' as:'
	filename = stdwin.askfile(prompt, window.filename, 1)
	if save_file(window, filename):
		window.filename = filename
		window.settitle(filename)
		window.changed = 0


def save_copy_dialog(window):
	prompt = 'Save a copy of ' + window.gettitle() + ' as:'
	filename = stdwin.askfile(prompt, window.filename, 1)
	void = save_file(window, filename)


def save_file(window, filename):
	try:
		fp = open(filename, 'w')
	except RuntimeError:
		stdwin.message(filename + ': cannot create')
		return 0
	contents = window.textobject.gettext()
	try:
		fp.write(contents)
	except RuntimeError:
		stdwin.message(filename + ': write error')
		return 0
	return 1


def revert_dialog(window):
	if not window.filename:
		stdwin.message('This window has no file to revert from')
		return
	if window.changed:
		prompt = 'Really read ' + window.filename + ' back from file?'
		if not stdwin.askync(prompt, 1):
			return
	try:
		fp = open(window.filename, 'r')
	except RuntimeError:
		stdwin.message(filename + ': cannot open')
		return
	contents = fp.read()
	del fp				# Close the file
	window.textobject.settext(contents)
	window.changed = 0
	fix_docsize(window)


def fix_textsize(window):
	corner = window.getwinsize()
	area = (0, 0), (corner)
	window.textobject.move(area)
	fix_docsize(window)


def fix_docsize(window):
	area = window.textobject.getrect()
	origin, corner = area
	width, height = corner
	window.setdocsize(0, height)


def do_cut(window):
	selection = window.textobject.getfocustext()
	if not selection:
		stdwin.fleep()		# Nothing to cut
	elif not window.setselection(WS_PRIMARY, selection):
		stdwin.fleep()		# Window manager glitch...
	else:
		stdwin.rotatecutbuffers(1)
		stdwin.setcutbuffer(0, selection)
		window.textobject.replace('')
		window.changed = 1
		fix_docsize(window)


def do_copy(window):
	selection = window.textobject.getfocustext()
	if not selection:
		stdwin.fleep()		# Nothing to cut
	elif not window.setselection(WS_PRIMARY, selection):
		stdwin.fleep()		# Window manager glitch...
	else:
		stdwin.rotatecutbuffers(1)
		stdwin.setcutbuffer(0, selection)


def do_paste(window):
	selection = stdwin.getselection(WS_PRIMARY)
	if not selection:
		selection = stdwin.getcutbuffer(0)
	if not selection:
		stdwin.fleep()		# Nothing to paste
	else:
		window.textobject.replace(selection)
		window.changed = 1
		fix_docsize(window)

def do_clear(window):
	first, last = window.textobject.getfocus()
	if first == last:
		stdwin.fleep()		# Nothing to clear
	else:
		window.textobject.replace('')
		window.changed = 1
		fix_docsize(window)


def do_select_all(window):
	window.textobject.setfocus(0, 0x7fffffff) # XXX Smaller on the Mac!


main()
