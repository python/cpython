# Module 'dirwin'

# Directory windows, a subclass of listwin

import gwin
import listwin
import anywin
import path
import dircache

def action(w, string, i, detail):
	(h, v), clicks, button, mask = detail
	if clicks = 2:
		name = path.cat(w.name, string)
		try:
			w2 = anywin.open(name)
			w2.parent = w
		except posix.error, why:
			stdwin.message('Can\'t open ' + name + ': ' + why[1])

def open(name):
	name = path.cat(name, '')
	list = dircache.opendir(name)[:]
	list.sort()
	dircache.annotate(name, list)
	w = listwin.open(name, list)
	w.name = name
	w.action = action
	return w
