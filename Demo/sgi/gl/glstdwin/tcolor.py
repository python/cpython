# Try colors -- display all 256 possible colors, with their color index

import stdwingl

import stdwin
from stdwinevents import *

NROWS = 16
NCOLS = 16

def main():
	stdwin.setdefwinsize(NCOLS * stdwin.textwidth('12345'), \
				NROWS * stdwin.lineheight() * 3)
	w = stdwin.open('TestColors')
	#
	while 1:
		type, window, detail = stdwin.getevent()
		if type == WE_CLOSE:
			print 'Bye.'
			break
		elif type == WE_SIZE:
			w.change((0,0), (10000, 10000))
		elif type == WE_DRAW:
			width, height = w.getwinsize()
			d = w.begindrawing()
			for row in range(NROWS):
				for col in range(NCOLS):
					color = row*NCOLS + col
					d.setfgcolor(color)
					p = col*width/NCOLS, row*height/NROWS
					q = (col+1)*width/NCOLS, \
						(row+1)*height/NROWS
					d.paint((p, q))
					d.setfgcolor(0)
					d.box((p, q))
					d.text(p, `color`)
					p = p[0] , p[1]+ d.lineheight()
					d.setfgcolor(7)
					d.text(p, `color`)
			del d
	#

main()
