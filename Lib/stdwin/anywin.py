# Module 'anywin'
# Open a file or directory in a window

import dirwin
import filewin
import os

def open(name):
	print 'opening', name, '...'
	if os.path.isdir(name):
		w = dirwin.open(name)
	else:
		w = filewin.open(name)
	return w
