#! /usr/local/bin/python

# www9.py -- display the contents of a URL in a Text widget
# - set window title
# - make window resizable
# - update display while reading
# - vertical scroll bar
# - rewritten as class

import sys
import urllib
from Tkinter import *

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

		# The Scrollbar *must* be created first
		self.vbar = Scrollbar(self.root)
		self.vbar.pack({'fill': 'y', 'side': 'right'})
		self.text = Text(self.root)
		self.text.pack({'expand': 1, 'fill': 'both', 'side': 'left'})

		# Link Text widget and Scrollbar
		self.text['yscrollcommand'] = (self.vbar, 'set')
		self.vbar['command'] = (self.text, 'yview')

	def load(self, url):
		# Load a new URL into the window
		fp = urllib.urlopen(url)

		self.root.title(url)

		self.text.delete('0.0', 'end')

		while 1:
			line = fp.readline()
			if not line: break
			self.text.insert('end', line)
			self.root.update_idletasks()

		fp.close()

	def go(self):
		# Start Tk main loop
		self.root.mainloop()

main()
