#! /usr/local/bin/python

# www11.py -- display the contents of a URL in a Text widget
# - set window title
# - make window resizable
# - update display while reading
# - vertical scroll bar
# - rewritten as class
# - editable url entry and reload button
# - error dialog

import sys
import urllib
from Tkinter import *
import Dialog

def main():
	if len(sys.argv) != 2 or sys.argv[1][:1] == '-':
		print "Usage:", sys.argv[0], "url"
		sys.exit(2)
	url = sys.argv[1]
	viewer = Viewer()
	viewer.load(url)
	viewer.go()

class Viewer:

	def __init__(self):
		# Create root window
		self.root = Tk()
		self.root.minsize(1, 1)

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

main()
