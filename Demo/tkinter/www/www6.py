#! /usr/local/bin/python

# www6.py -- display the contents of a URL in a Text widget
# - set window title
# - make window resizable

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
	root.title(url)
	root.minsize(1, 1)		# Set minimum size
	text = Text(root)
	text.pack({'expand': 1, 'fill': 'both'}) # Expand into available space

	while 1:
		line = fp.readline()
		if not line: break
		text.insert('end', line)

	root.mainloop()			# Start Tk main loop (for root!)

main()
