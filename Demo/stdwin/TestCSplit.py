#! /usr/local/python

# TestCSplit

import stdwin
from WindowParent import WindowParent, MainLoop
from Buttons import PushButton

def main(n):
	from CSplit import CSplit
	#
	stdwin.setdefscrollbars(0, 0)
	#
	the_window = WindowParent().create('TestCSplit', (0, 0))
	the_csplit = CSplit().create(the_window)
	#
	for i in range(n):
		the_child = PushButton().define(the_csplit)
		the_child.settext(`(i+n-1)%n+1`)
	#
	the_window.realize()
	#
	MainLoop()

main(12)
