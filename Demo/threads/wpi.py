# Display digits of pi in a window, calculating in a separate thread.
# Compare ../scripts/pi.py.

import time
import thread
import stdwin
from stdwinevents import *

digits = []

def worker():
	k, a, b, a1, b1 = 2l, 4l, 1l, 12l, 4l
	while 1:
		# Next approximation
		p, q, k = k*k, 2l*k+1l, k+1l
		a, b, a1, b1 = a1, b1, p*a+q*a1, p*b+q*b1
		# Print common digits
		d, d1 = a/b, a1/b1
		#print a, b, a1, b1
		while d == d1:
			digits.append(`int(d)`)
			a, a1 = 10l*(a%b), 10l*(a1%b1)
			d, d1 = a/b, a1/b1

def main():
	digits_seen = 0
	thread.start_new_thread(worker, ())
	tw = stdwin.textwidth('0 ')
	lh = stdwin.lineheight()
	stdwin.setdefwinsize(20 * tw, 20 * lh)
	win = stdwin.open('digits of pi')
	options = win.menucreate('Options')
	options.additem('Auto scroll')
	autoscroll = 1
	options.check(0, autoscroll)
	while 1:
		win.settimer(1)
		type, w, detail = stdwin.getevent()
		if type == WE_CLOSE:
			thread.exit_prog(0)
		elif type == WE_DRAW:
			(left, top), (right, bottom) = detail
			digits_seen = len(digits)
			d = win.begindrawing()
			for i in range(digits_seen):
				h = (i % 20) * tw
				v = (i / 20) * lh
				if top-lh < v < bottom:
					d.text((h, v), digits[i])
			d.close()
		elif type == WE_TIMER:
			n = len(digits)
			if n > digits_seen:
				win.settitle(`n` + ' digits of pi')
				d = win.begindrawing()
				for i in range(digits_seen, n):
					h = (i % 20) * tw
					v = (i / 20) * lh
					d.text((h, v), digits[i])
				d.close()
				digits_seen = n
				height = (v + 20*lh) / (20*lh) * (20*lh)
				win.setdocsize(0, height)
				if autoscroll:
					win.show((0, v), (h+tw, v+lh))
		elif type == WE_MENU:
			menu, item = detail
			if menu == options:
				if item == 0:
					autoscroll = (not autoscroll)
					options.check(0, autoscroll)
					

main()
