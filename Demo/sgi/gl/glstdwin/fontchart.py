import stdwingl

import stdwin
from stdwinevents import *

def main():
	size = 12
	w = stdwin.open('Font chart ' + `size`)
	while 1:
		type, window, detail = stdwin.getevent()
		if type == WE_CLOSE:
			break
		if type == WE_DRAW:
			width, height = w.getwinsize()
			d = w.begindrawing()
			d.setsize(size)
			h, v = 0, 0
			for c in range(32, 256):
				ch = chr(c)
				chw = d.textwidth(ch)
				if h + chw > width:
					v = v + d.lineheight()
					h = 0
					if v >= height:
						break
				d.text((h, v), ch)
				h = h + chw
			del d
		if type == WE_MOUSE_UP:
			size = size + 1
			w.settitle('Font chart ' + `size`)
			w.change((0, 0), (2000, 2000))

main()
