#! /usr/local/bin/python

# www4.py -- display the contents of a URL in a Text widget

import sys
import urllib
from Tkinter import *

def main():
	if len(sys.argv) != 2 or sys.argv[1][:1] == '-':
		print "Usage:", sys.argv[0], "url"
		sys.exit(2)
	url = sys.argv[1]
	fp = urllib.urlopen(url)

	text = Text()			# Create text widget
	text.pack()			# Realize it

	while 1:
		line = fp.readline()
		if not line: break
		text.insert('end', line)	# Append line to text widget

	text.mainloop()			# Start Tk main loop

main()
