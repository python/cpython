#! /usr/local/bin/python

# www12.py -- display the contents of a URL in a Text widget
# - set window title
# - make window resizable
# - update display while reading
# - vertical scroll bar
# - rewritten as class
# - editable url entry and reload button
# - error dialog
# - menu bar; added 'master' option to constructor

import sys
import urllib
from Tkinter import *
import Dialog

def main():
	if len(sys.argv) != 2 or sys.argv[1][:1] == '-':
		print "Usage:", sys.argv[0], "url"
		sys.exit(2)
	url = sys.argv[1]
	tk = Tk()
	tk.withdraw()
	viewer = Viewer(tk)
	viewer.load(url)
	viewer.go()

class Viewer:

	def __init__(self, master = None):
		# Create root window
		if master is None:
			self.root = self.master = Tk()
		else:
			self.master = master
			self.root = Toplevel(self.master)
		self.root.minsize(1, 1)

		# Create menu bar
		self.mbar = Frame(self.root,
				  {'relief': 'raised',
				   'border': 2})
		self.mbar.pack({'fill': 'x'})

		# Create File menu
		self.filebutton = Menubutton(self.mbar, {'text': 'File'})
		self.filebutton.pack({'side': 'left'})

		self.filemenu = Menu(self.filebutton)
		self.filebutton['menu'] = self.filemenu

		# Create Edit menu
		self.editbutton = Menubutton(self.mbar, {'text': 'Edit'})
		self.editbutton.pack({'side': 'left'})

		self.editmenu = Menu(self.editbutton)
		self.editbutton['menu'] = self.editmenu

		# Magic so you can swipe from one button to the next
		self.mbar.tk_menuBar(self.filebutton, self.editbutton)

		# Populate File menu
		self.filemenu.add('command', {'label': 'New',
					      'command': self.new_command})
		self.filemenu.add('command', {'label': 'Open...',
					      'command': self.open_command})
		self.filemenu.add('command', {'label': 'Clone',
					      'command': self.clone_command})
		self.filemenu.add('separator')
		self.filemenu.add('command', {'label': 'Close',
					      'command': self.close_command})
		self.filemenu.add('command', {'label': 'Quit',
					      'command': self.quit_command})

		# Populate Edit menu
		pass

		# Create topframe for the entry and button
		self.topframe = Frame(self.root)
		self.topframe.pack({'fill': 'x'})

		# Create a label in front of the entry
		self.urllabel = Label(self.topframe, {'text': 'URL:'})
		self.urllabel.pack({'side': 'left'})

		# Create the entry containing the URL
		self.entry = Entry(self.topframe,
				   {'relief': 'sunken', 'border': 2})
		self.entry.pack({'side': 'left', 'fill': 'x', 'expand': 1})
		self.entry.bind('<Return>', self.loadit)

		# Create the button
		self.reload = Button(self.topframe,
				     {'text': 'Reload',
				      'command': self.reload})
		self.reload.pack({'side': 'right'})

		# Create botframe for the text and scrollbar
		self.botframe = Frame(self.root)
		self.botframe.pack({'fill': 'both', 'expand': 1})

		# The Scrollbar *must* be created first
		self.vbar = Scrollbar(self.botframe)
		self.vbar.pack({'fill': 'y', 'side': 'right'})
		self.text = Text(self.botframe)
		self.text.pack({'expand': 1, 'fill': 'both', 'side': 'left'})

		# Link Text widget and Scrollbar
		self.text['yscrollcommand'] = (self.vbar, 'set')
		self.vbar['command'] = (self.text, 'yview')

		self.url = None

	def load(self, url):
		# Load a new URL into the window
		fp, url = self.urlopen(url)
		if not fp:
			return

		self.url = url

		self.root.title(url)

		self.entry.delete('0', 'end')
		self.entry.insert('end', url)

		self.text.delete('0.0', 'end')

		while 1:
			line = fp.readline()
			if not line: break
			if line[-2:] == '\r\n': line = line[:-2] + '\n'
			self.text.insert('end', line)
			self.root.update_idletasks()

		fp.close()

	def urlopen(self, url):
		# Open a URL --
		# return (fp, url) if successful
		# display dialog and return (None, url) for errors
		try:
			fp = urllib.urlopen(url)
		except IOError, msg:
			import types
			if type(msg) == types.TupleType and len(msg) == 4:
				if msg[1] == 302:
					m = msg[3]
					if m.has_key('location'):
						url = m['location']
						return self.urlopen(url)
					elif m.has_key('uri'):
						url = m['uri']
						return self.urlopen(url)
			self.errordialog(IOError, msg)
			fp = None
		return fp, url

	def errordialog(self, exc, msg):
		# Display an error dialog -- return when the user clicks OK
		Dialog.Dialog(self.root, {
			'text': str(msg),
			'title': exc,
			'bitmap': 'error',
			'default': 0,
			'strings': ('OK',),
			})

	def go(self):
		# Start Tk main loop
		self.root.mainloop()

	def reload(self, *args):
		# Callback for Reload button
		if self.url:
			self.load(self.url)

	def loadit(self, *args):
		# Callback for <Return> event in entry
		self.load(self.entry.get())

	def new_command(self):
		# File/New...
		Viewer(self.master)

	def clone_command(self):
		# File/Clone
		v = Viewer(self.master)
		v.load(self.url)

	def open_command(self):
		# File/Open...
		print "File/Open...: Not implemented"

	def close_command(self):
		# File/Close
		self.destroy()

	def quit_command(self):
		# File/Quit
		self.root.quit()

	def destroy(self):
		# Destroy this window
		self.root.destroy()
		if self.master is not self.root and not self.master.children:
			self.master.quit()

main()
