from Dlg import *
from Events import *
from Evt import *
from List import *
from Qd import *
import Res
import string
import MacOS

ID = 513

def dodialog(items):
	print 'This is to create a window'
	#
	# Create the dialog
	#
	d = GetNewDialog(ID, -1)
	#
	# Create the list and fill it
	#
	tp, h, rect = d.GetDialogItem(2)
	rect = rect[0], rect[1], rect[2]-15, rect[3]-15  # Space for scrollbars
	length = (len(items)+1) / 2
	list = LNew(rect, (0, 0, 2, length), (0, 0), 0, d, 0, 1, 1, 1)
	for i in range(len(items)):
		list.LSetCell(items[i], (i%2, i/2))
	#
	# Draw it.
	#
	list.LSetDrawingMode(1)
	list.LUpdate(self.wid.GetWindowPort().visRgn)
	#
	# Do the (modeless) dialog
	#
	while 1:
		ok, ev = WaitNextEvent(0xffff, 10)
		if not ok:
			# No event.
			continue
		(what, message, when, where, modifiers) = ev
		if what == updateEvt:
			# XXXX We just always update our list (sigh...)
			SetPort(window)
			list.LUpdate(self.wid.GetWindowPort().visRgn)
		if IsDialogEvent(ev):
			# It is a dialog event. See if it's ours.
			ok, window, item = DialogSelect(ev)
			if ok:
				if window == d:
					# Yes, it is ours.
					if item == 1:	# OK button
						break
					elif item == 2:	# List
						(what, message, when, where, modifiers) = ev
						SetPort(window)
						if what == mouseDown:
							local = GlobalToLocal(where)
							list.LClick(local, modifiers)
					else:
						print 'Unexpected item hit'
				else:
					print 'Unexpected dialog hit'
		else:
			MacOS.HandleEvent(ev)
	sel = []
	for i in range(len(items)):
		ok, dummycell = list.LGetSelect(0, (i%2, i/2))
		if ok:
			sel.append(list.LGetCell(256, (i%2, i/2)))
	print 'Your selection:', sel

def test():
	import os, sys
	Res.FSpOpenResFile('tlist_dialog.rsrc', 1)
	dodialog(os.listdir(':'))
	sys.exit(1)

if __name__ == '__main__':
	test()
