# Module 'dirwin'

# Directory windows, a subclass of listwin

import os
import gwin
import listwin
import anywin
import dircache

def action(w, string, i, detail):
	(h, v), clicks, button, mask = detail
	if clicks == 2:
		name = os.path.join(w.name, string)
		try:
			w2 = anywin.open(name)
			w2.parent = w
		except os.error, why:
			stdwin.message('Can\'t open ' + name + ': ' + why[1])

def open(name):
	name = os.path.join(name, '')
	list = dircache.opendir(name)[:]
	list.sort()
	dircache.annotate(name, list)
	w = listwin.open(name, list)
	w.name = name
	w.action = action
	return w
