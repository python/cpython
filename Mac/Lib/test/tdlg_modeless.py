# Function to display a message and wait for the user to hit OK.
# This uses a DLOG resource with ID=256 which is part of the standard
# Python library.
# The ID can be overridden by passing a second parameter.
# This is the modeless version of this test program, the normal
# modal version is in tdlg.py

import addpack
addpack.addpack(':Tools:bgen:evt')

from Dlg import *
from Evt import *
from Events import *
import MacOS
import string

ID = 256

def message(str = "Hello, modeless world!", id = ID):
	print 'This is to init the console window...'
	d = GetNewDialog(id, -1)
	tp, h, rect = d.GetDialogItem(2)
	SetDialogItemText(h, str)
	while 1:
		ok, ev = WaitNextEvent(0xffff, 10)
		if not ok:
			continue
		if IsDialogEvent(ev):
			ok, window, item = DialogSelect(ev)
			if ok:
				if window == d:
					if item == 1:
						break
					else:
						print 'Unexpected item hit'
				else:
					print 'Unexpected dialog hit'
		else:
			MacOS.HandleEvent(ev)

def test():
	message()

if __name__ == '__main__':
	test()
