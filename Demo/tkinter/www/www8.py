#! /usr/local/bin/python

# www8.py -- display the contents of a URL in a Text widget
# - set window title
# - make window resizable
# - update display while reading
# - vertical scroll bar

import sys
import urllib
from Tkinter import *

def main():
	if len(sys.argv) != 2 or sys.argv[1][:1] == '-':
		print "Usage:", sys.argv[0], "url"
		sys.exit(2)
	url = sys.argv[1]
	fp = urllib.urlopen(url)

	# Create root window
	root = Tk()
	root.title(url)
	root.minsize(1, 1)

	# The Scrollbar *must* be created first -- this is magic for me :-(
	vbar = Scrollbar(root)
	vbar.pack({'fill': 'y', 'side': 'right'})
	text = Text(root, {'yscrollcommand': (vbar, 'set')})
	text.pack({'expand': 1, 'fill': 'both', 'side': 'left'})

	# Link Text widget and Scrollbar -- this is magic for you :-)
	##text['yscrollcommand'] = (vbar, 'set')
	vbar['command'] = (text, 'yview')

	while 1:
		line = fp.readline()
		if not line: break
		text.insert('end', line)
		root.update_idletasks()

	root.mainloop()

main()
