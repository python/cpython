#! /usr/local/python

# TestSched

import stdwin
from WindowParent import WindowParent, MainLoop
import WindowSched
from Buttons import PushButton

def my_ringer(child):
	child.id = None
	stdwin.fleep()

def my_hook(child):
	# schedule for the bell to ring in N seconds; cancel previous
	if child.my_id:
		WindowSched.cancel(child.my_id)
	child.my_id = \
		WindowSched.enter(child.my_number*1000, 0, my_ringer, child)

def main(n):
	from CSplit import CSplit
	
	window = WindowParent().create('TestSched', (0, 0))
	csplit = CSplit().create(window)
	
	for i in range(n):
		child = PushButton().define(csplit)
		child.my_number = i
		child.my_id = None
		child.settext(`(i+n-1)%n+1`)
		child.hook = my_hook
	
	window.realize()
	
	WindowSched.run()

main(12)
