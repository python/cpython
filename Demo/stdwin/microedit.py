#! /usr/local/bin/python

# A minimal single-window text editor using STDWIN's text objects.
#
# Usage: microedit file
#
# This is not intended as a real application but as an introduction
# to STDWIN programming in Python, especially text objects.
# Once you understand microedit.py, study miniedit.py to learn
# about multiple windows and menus, cut and paste, etc.


import sys
import stdwin
from stdwinevents import *


# Main program
#
def main():
	#
	# Get the filename argument and read its contents as one very
	# large string.
	# An exception will terminate the program if there is no argument
	# or if the file could not be read...
	#
	filename = sys.argv[1]
	fp = open(filename, 'r')
	contents = fp.read()
	del fp				# Close the file
	#
	# Create the window, using the filename as window title
	#
	window = stdwin.open(filename)
	#
	# Add a simple File menu to the window with two items
	#
	filemenu = window.menucreate('File')
	filemenu.additem('Save', 'S')	# Item 0 (shortcut Meta-S)
	filemenu.additem('Save As...')	# Item 1
	#
	# Create a text object occupying the entire window
	# and fill it with the file's contents
	#
	corner = window.getwinsize()	# (width, height)
	area = (0, 0), corner		# Rectangle as large as the window
	text = window.textcreate(area)
	text.settext(contents)
	del contents			# Get rid of contents object
	fix_textsize(window, text)	# Set document size accordingly
	#
	# Main event loop -- stop if a close request comes in.
	#
	# STDWIN applications should regularly call stdwin.getevent()
	# otherwise the windows won't function as expected.
	#
	while 1:
		#
		# Get the next event
		#
		type, w, detail = e = stdwin.getevent()
		#
		# Event decoding switch
		#
		if type == WE_CLOSE:
			break		# Stop (no check for saved file!)
		elif type == WE_SIZE:
			#
			# The window was resized --
			# let the text object recompute the line breaks
			# and change the document size accordingly,
			# so scroll bars will work
			#
			fix_textsize(window, text)
		elif type == WE_MENU:
			#
			# Execute a file menu request (our only menu)
			#
			menu, item = detail
			if item == 0:
				#
				# "Save": save to the current filename
				#
				dummy = save_file(window, text, filename)
			elif item == 1:
				#
				# "Save As": ask a new filename, save to it,
				# and make it the current filename
				#
				# NB: askfile raises KeyboardInterrupt
				# if the user cancels the dialog, hence
				# the try statement
				#
				try:
					newfile = stdwin.askfile( \
						'Save as:', filename, 1)
				except KeyboardInterrupt:
					newfile = ''
				if newfile:
					if save_file(window, text, newfile):
						filename = newfile
						window.settitle(filename)
		elif text.event(e):
			#
			# The text object has handled the event.
			# Fix the document size if necessary.
			# Note: this sometimes fixes the size
			# unnecessarily, e.g., for arrow keys.
			#
			if type in (WE_CHAR, WE_COMMAND):
				fix_docsize(window, text)


# Save the window's contents to the filename.
# If the open() fails, put up a warning message and return 0;
# if the save succeeds, return 1.
#
def save_file(window, text, filename):
	#
	# Open the file for writing, handling exceptions
	#
	try:
		fp = open(filename, 'w')
	except RuntimeError:
		stdwin.message('Cannot create ' + filename)
		return 0
	#
	# Get the contents of the text object as one very long string
	#
	contents = text.gettext()
	#
	# Write the contents to the file
	#
	fp.write(contents)
	#
	# The file is automatically closed when this routine returns
	#
	return 1


# Change the size of the text object to fit in the window,
# and then fix the window's document size to fit around the text object.
#
def fix_textsize(window, text):
	#
	# Compute a rectangle as large as the window
	#
	corner = window.getwinsize()	# (width, height)
	area = (0, 0), (corner)
	#
	# Move the text object to this rectangle.
	# Note: text.move() ignores the bottom coordinate!
	#
	text.move(area)
	#
	# Now fix the document size accordingly
	#
	fix_docsize(window, text)


# Fix the document size, after the text has changed
#
def fix_docsize(window, text):
	#
	# Get the actual rectangle occupied by the text object.
	# This has the same left, top and right, but a different bottom.
	#
	area = text.getrect()
	#
	# Compute the true height of the text object
	#
	origin, corner = area
	width, height = corner
	#
	# Set the document height to the text object's height.
	# The width is zero since we don't want a horizontal scroll bar.
	#
	window.setdocsize(0, height)


# Once all functions are defined, call main()
#
main()
