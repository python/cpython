# Module 'filewin'
# File windows, a subclass of textwin (which is a subclass of gwin)

import stdwin
import textwin
import path

builtin_open = open

def readfile(fn): # Return a string containing the file's contents
	fp = builtin_open(fn, 'r')
	a = ''
	n = 8096
	while 1:
		b = fp.read(n)
		if not b: break
		a = a + b
	return a


# FILE WINDOW

def open_readonly(fn): # Open a file window
	w = textwin.open_readonly(fn, readfile(fn))
	w.fn = fn
	return w

def open(fn): # Open a file window
	w = textwin.open(fn, readfile(fn))
	w.fn = fn
	return w
