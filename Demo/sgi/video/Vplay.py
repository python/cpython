import sys
import VFile
import time
import gl, GL
from DEVICE import *

def main():
	if sys.argv[1:]:
		for filename in sys.argv[1:]:
			process(filename)
	else:
		process('film.video')

def process(filename):
	vin = VFile.VinFile().init(filename)
	print 'File:    ', filename
	print 'Version: ', vin.version
	print 'Size:    ', vin.width, 'x', vin.height
	print 'Pack:    ', vin.packfactor, '; chrom:', vin.chrompack
	print 'Bits:    ', vin.c0bits, vin.c1bits, vin.c2bits
	print 'Format:  ', vin.format
	print 'Offset:  ', vin.offset
	
	gl.foreground()
	gl.prefsize(vin.width, vin.height)
	win = gl.winopen('* ' + filename)
	vin.initcolormap()

	gl.qdevice(ESCKEY)
	gl.qdevice(WINSHUT)
	gl.qdevice(WINQUIT)

	t0 = time.millitimer()
	running = 1
	data = None
	while 1:
		if running:
			try:
				t, data, chromdata = vin.getnextframe()
			except EOFError:
				running = 0
				gl.wintitle(filename)
		if running:
			dt = t + t0 - time.millitimer()
			if dt > 0:
				time.millisleep(dt)
				vin.showframe(data, chromdata)
		if not running or gl.qtest():
			dev, val = gl.qread()
			if dev in (ESCKEY, WINSHUT, WINQUIT):
				break
			if dev == REDRAW:
				gl.reshapeviewport()
				if data:
					vin.showframe(data, chromdata)

main()
