# Function to display a message and wait for the user to hit OK.
# This uses a DLOG resource with ID=256 which is part of the standard
# Python library.
# The ID can be overridden by passing a second parameter.

from Dlg import *
from Events import *
import string

ID = 256

def f(d, event):
	what, message, when, where, modifiers = event
	if what == keyDown and modifiers & cmdKey and \
	   string.lower(chr(message & charCodeMask)) == 'o':
		return 1

def message(str = "Hello, world!", id = ID):
	d = GetNewDialog(id, -1)
	tp, h, rect = d.GetDItem(2)
	SetIText(h, str)
	while 1:
		n = ModalDialog(f)
		if n == 1: break

def test():
	message()

if __name__ == '__main__':
	test()
