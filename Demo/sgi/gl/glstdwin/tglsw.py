import sys

if len(sys.argv) < 2:
	import stdwingl
	color = 1
	needclose = 1
else:
	color = 0
	needclose = 0

import stdwin
import time
from stdwinevents import *
from GL import BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE

def main():
	#
	stdwin.setdefwinsize(300, 300)
	stdwin.setdefwinpos(0, 0)
	if color: stdwin.setbgcolor(YELLOW)
	w1 = stdwin.open('Hello, world')
	w1.box = (10, 10), (90, 90)
	#
	stdwin.setdefwinsize(0, 0)
	stdwin.setdefwinpos(50, 50)
	if color: stdwin.setbgcolor(GREEN)
	w2 = stdwin.open('Second window')
	w2.box = (10, 10), (90, 90)
	#
	while w1 or w2:
		type, window, detail = stdwin.getevent()
		if type == WE_DRAW:
			d = window.begindrawing()
			if window == w1:
				if color: d.setfgcolor(BLACK)
				d.box(((50, 50), (250, 250)))
				if color: d.setfgcolor(RED)
				d.cliprect(((50, 50), (250, 250)))
				d.paint(w1.box)
				d.noclip()
				if color: d.setfgcolor(BLUE)
				d.line((0, 0), w1.box[0])
			elif window == w2:
				if color: d.setfgcolor(WHITE)
				d.box(w2.box)
				if color: d.setfgcolor(BLACK)
				d.text(w2.box[0], 'Hello world')
			else:
				print 'Strange draw???', window, detail
			del d
		elif type == WE_CLOSE:
			if needclose: window.close()
			if window == w1:
				w1 = None
			elif window == w2:
				w2 = None
			else:
				print 'weird close event???', window, detail
		elif type in (WE_MOUSE_DOWN, WE_MOUSE_MOVE, WE_MOUSE_UP):
			h, v = detail[0]
			window.box = (h, v), (h+80, v+80)
			window.change(((0,0), (2000, 2000)))
		elif type == WE_CHAR:
			print 'character', `detail`
		else:
			print type, window, detail
	#

main()
print 'Done.'
