import sys
from socket import *
from gl import *
from GL import *
from DEVICE import *
from time import millitimer

HS = 40 # Header size (must be same as in tv.py)

# Rely on UDP packet (de)fragmentation for smoother images
# (Changed for broadcast)
MAX = 16000

PF = 2 # Default packfactor

# Default receiver station is voorn.
# Kwik has no yellow pages, so...
HOST = '192.16.201.121'
PORT = 5555

if sys.argv[1:]:
	PF = eval(sys.argv[1])

if sys.argv[2:]:
	HOST = sys.argv[2]
	if HOST == 'all':
		HOST = '<broadcast>'
		MAX = 1400

PF2 = PF*PF

def main():
	centerx, centery = 400, 300

	foreground()
	wid = winopen('cam')
	RGBmode()
	doublebuffer()
	gconfig()
	qdevice(ESCKEY)

	w, h = getsize()
	ortho2(0, w, 0, h)
	w = w/PF*PF
	h = h/PF*PF

	readsource(SRC_FRAMEGRABBER)

	s = socket(AF_INET, SOCK_DGRAM)
	if HOST == '<broadcast>':
		s.allowbroadcast(1)
	addr = HOST, PORT

	bytesperline = w/PF2
	linesperchunk = MAX/bytesperline
	linesperchunk = linesperchunk/PF*PF
	nchunks = (h+linesperchunk-1)/linesperchunk

	print 'MAX=', MAX,
	print 'linesperchunk=', linesperchunk,
	print 'nchunks=', nchunks,
	print 'w=', w, 'h=', h

	x1, x2 = 0, w-1

	t1 = millitimer()
	nframes = 0
	fps = 0

	msg = ''

	while 1:
		while qtest():
			dev, val = qread()
			if dev == REDRAW:
				reshapeviewport()
				w, h = getsize()
				ortho2(0, w, 0, h)
				w = w/PF*PF
				h = h/PF*PF

				bytesperline = w/PF2
				linesperchunk = MAX/bytesperline
				linesperchunk = linesperchunk/PF*PF
				nchunks = (h+linesperchunk-1)/linesperchunk

				print 'MAX=', MAX,
				print 'linesperchunk=', linesperchunk,
				print 'nchunks=', nchunks,
				print 'w=', w, 'h=', h

				x1, x2 = 0, w-1

				fps = 0

			elif dev == ESCKEY:
				winclose(wid)
				return

		readsource(SRC_FRAMEGRABBER)

		nframes = nframes+1
		if nframes >= fps:
			t2 = millitimer()
			if t2 <> t1:
				fps = int(10000.0*nframes/(t2-t1)) * 0.1
				msg = `fps` +  ' frames/sec'
				t1 = t2
				nframes = 0

		RGBcolor(255,255,255)
		cmov2i(9,9)
		charstr(msg)

		swapbuffers()
		rectcopy(centerx-w/2, centery-w/2, centerx+w/2, centery+w/2, 0, 0)

		for i in range(nchunks):
			y1 = i*linesperchunk
			y2 = y1 + linesperchunk-1
			if y2 >= h: y2 = h-1
			data = lrectread(x1, y1, x2, y2)
			data2 = packrect(x2-x1+1, y2-y1+1, PF, data)
			prefix = `w, h, PF, x1, y1, x2, y2`
			prefix = prefix + ' ' * (HS-len(prefix))
			data3 = prefix + data2
			s.sendto(data3, addr)

main()
