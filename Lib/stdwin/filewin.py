# Module 'filewin'
# File windows, a subclass of textwin (which is a subclass of gwin)

import textwin
from util import readfile


# FILE WINDOW

def open_readonly(fn): # Open a file window
	w = textwin.open_readonly(fn, readfile(fn))
	w.fn = fn
	return w

def open(fn): # Open a file window
	w = textwin.open(fn, readfile(fn))
	w.fn = fn
	return w
