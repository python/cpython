#! /usr/local/bin/python

# www5.py -- display the contents of a URL in a Text widget
# - set window title

import sys
import urllib
from Tkinter import *

def main():
	if len(sys.argv) != 2 or sys.argv[1][:1] == '-':
		print "Usage:", sys.argv[0], "url"
		sys.exit(2)
	url = sys.argv[1]
	fp = urllib.urlopen(url)

	root = Tk()
	root.title(url)			# Set window manager title
	text = Text(root)
	text.pack()

	while 1:
		line = fp.readline()
		if not line: break
		text.insert('end', line)

	text.mainloop()

main()
