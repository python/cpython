# Window interface to (some of) the CD player's vital audio functions

import cd
import stdwin
from stdwinevents import *
import mainloop

def main():
	player = cd.open()
	stdwin.setdefscrollbars(0, 0)
	win = stdwin.open('CD')
	win.player = player
	win.dispatch = cddispatch
	mainloop.register(win)
	win.settimer(10)
	mainloop.mainloop()

def cddispatch(type, win, detail):
	if type == WE_NULL:
		pass
	elif type == WE_CLOSE:
		mainloop.unregister(win)
		win.close()
	elif type == WE_DRAW:
		draw(win)
	elif type == WE_TIMER:
		update(win)
	elif type == WE_MOUSE_UP:
		left, top, right, bottom, v1, v2 = getgeo(win)
		h, v = detail[0]
		if left < h < right:
			if top < v < v1:
				but1(win)
			elif v1 < v < v2:
				but2(win)
			elif v2 < v < bottom:
				but3(win)
			else:
				stdwin.fleep()

def but1(win):
	update(win)

def but2(win):
	win.player.togglepause()
	update(win)

def but3(win):
	win.player.stop()
	update(win)

def update(win):
	d = win.begindrawing()
	drawstatus(win, d)
	d.enddrawing()
	win.settimer(10)

statedict = ['ERROR', 'NODISK', 'READY', 'PLAYING', 'PAUSED', 'STILL']

def draw(win):
	left, top, right, bottom, v1, v2 = getgeo(win)
	d = win.begindrawing()
	drawstatus(win, d)
	box(d, left, v1, right, v2, 'Play/Pause')
	box(d, left, v2, right, bottom, 'Stop')
	d.enddrawing()

def drawstatus(win, d):
	left, top, right, bottom, v1, v2 = getgeo(win)
	status = win.player.getstatus()
	state = status[0]
	if 0 <= state < len(statedict):
		message = statedict[state]
	else:
		message = `status`
	message = message + ' track ' + `status[1]` + ' of ' + `status[12]`
	d.erase((left, top), (right, v1))
	box(d, left, top, right, v1, message)

def box(d, left, top, right, bottom, label):
	R = (left+1, top+1), (right-1, bottom-1)
	width = d.textwidth(label)
	height = d.lineheight()
	h = (left + right - width) / 2
	v = (top + bottom - height) / 2
	d.box(R)
	d.cliprect(R)
	d.text((h, v), label)
	d.noclip()

def getgeo(win):
	(left, top), (right, bottom) = (0, 0), win.getwinsize()
	v1 = top + (bottom - top) / 3
	v2 = top + (bottom - top) * 2 / 3
	return left, top, right, bottom, v1, v2

main()
