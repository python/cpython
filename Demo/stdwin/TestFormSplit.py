#! /usr/local/python

# TestFormSplit

import stdwin
from WindowParent import WindowParent, MainLoop
from Buttons import PushButton

def main(n):
	from FormSplit import FormSplit
	#
	stdwin.setdefscrollbars(1, 1)
	#
	the_window = WindowParent().create('TestFormSplit', (0, 0))
	the_form = FormSplit().create(the_window)
	#
	for i in range(n):
		if i % 3 == 0:
			the_form.placenext(i*40, 0)
		the_child = PushButton().define(the_form)
		the_child.settext('XXX-' + `i` + '-YYY')
	#
	the_window.realize()
	#
	MainLoop()

main(6)
