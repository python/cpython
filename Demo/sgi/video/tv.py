import string

from socket import *
from gl import *
from GL import *
from DEVICE import *
from time import millisleep, millitimer

PORT = 5555

PF = 2 # packfactor
HS = 40 # Header size

def testimage():
	RGBcolor(0, 0, 0)
	clear()
	RGBcolor(0, 255, 0)
	cmov2i(10, 10)
	charstr('Waiting...')

def reshape():
	reshapeviewport()
	w, h = getsize()
	ortho2(0, w, 0, h)
	testimage()
	return w, h

def main():
	s = socket(AF_INET, SOCK_DGRAM)
	s.bind('', PORT)

	foreground()
	wid = winopen('tv')
	RGBmode()
	gconfig()
	qdevice(ESCKEY)

	oldw, oldh = getsize()
	ortho2(0, oldw, 0, oldh)
	testimage()

	t1 = millitimer()

	while 1:
		if qtest():
			dev, val = qread()
			if dev = ESCKEY:
				winclose(wid)
				return
			elif dev = REDRAW:
				oldw, oldh = reshape()
		elif s.avail():
			data = s.recv(17000)
			header = string.strip(data[:HS])
			w, h, pf, x1, y1, x2, y2 = eval(header)
			if (w, h) <> (oldw, oldh):
				x, y = getorigin()
				x, y = x-1, y+21 # TWM correction
				winposition(x, x+w-1, y+oldh-h, y+oldh-1)
				oldw, oldh = reshape()
			data2 = data[HS:]
			dx = (x2-x1+1)/pf
			dy = (y2-y1+1)/pf
			data3 = unpackrect(dx, dy, 1, data2)
			rectzoom(pf, pf)
			lrectwrite(x1, y1, x1+dx-1, y1+dy-1, data3)
			t1 = millitimer()
		else:
			t2 = millitimer()
			if t2-t1 >= 5000:
				testimage()
				t1 = t2
			else:
				millisleep(10)

	winclose(wid)
	return data

main()
