# Capture a movie using the Indigo video library and board

import sv, SV
import VFile
import gl, GL, DEVICE

import time
import sys
import posix
import getopt
import string

def main():
	QSIZE = 16
	TIME = 5

	opts, args = getopt.getopt(sys.argv[1:], 'q:t:')
	for opt, arg in opts:
		if opt == '-q':
			QSIZE = string.atoi(arg)
		elif opt == '-t':
			TIME = string.atoi(arg)

	if args:
		filename = args[0]
		if args[1:]:
			print 'Warning: only first filename arg is used'
	else:
		filename = 'film.video'

	gl.foreground()

	x, y = SV.PAL_XMAX / 4, SV.PAL_YMAX / 4
	print x, 'x', y

	gl.minsize(40, 30)
	gl.stepunit(8, 6)
	gl.maxsize(SV.PAL_XMAX, SV.PAL_YMAX)
	gl.keepaspect(SV.PAL_XMAX, SV.PAL_YMAX)
	win = gl.winopen(filename)
	x, y = gl.getsize()
	print x, 'x', y

	v = sv.OpenVideo()
	v.BindGLWindow(win, SV.IN_REPLACE)
	v.SetSize(x, y)
	v.BindGLWindow(win, SV.IN_REPLACE)

	v.SetCaptureFormat(SV.RGB_FRAMES)
	v.SetCaptureMode(SV.BLOCKING_CAPTURE)
	v.SetQueueSize(QSIZE)
	v.InitCapture()
	if v.GetQueueSize() != QSIZE:
		QSIZE = v.GetQueueSize()
		print 'Warning: QSIZE reduced to', QSIZE

	gl.qdevice(DEVICE.LEFTMOUSE)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.ESCKEY)

	print 'Click left mouse to start recording', TIME, 'seconds'
	ofile = None
	# Mouse down opens the file & freezes window
	# Mouse up starts recording frames

	while 1:
		dev, val = gl.qread()
		if dev == DEVICE.LEFTMOUSE:
			# Start recording
			if val == 1:
				# Mouse down -- preparations
				if ofile == None:
					ofile = VFile.VoutFile().init(filename)
					ofile.format = 'rgb8'
					ofile.width = x
					ofile.height = y
					ofile.writeheader()
					# XXX other format bits?
				# The window can't be resized from now
				gl.prefsize(x, y)
				gl.winconstraints()
				gl.wintitle('* ' + filename)
				continue

			# Mouse up -- actual recording
			t0 = time.millitimer()
			v.StartCapture()
			while 1:
				t = time.millitimer() - t0
				if t >= TIME*1000:
					break
				if v.GetCaptured() > 2:
					print '(1)',
					doframe(v, ofile, x, y, t)
			v.StopCapture()
			while v.GetCaptured() > 0:
				print '(2)',
				doframe(v, ofile, x, y, t)
				t = time.millitimer() - t0
			gl.wintitle(filename)
		elif dev == DEVICE.REDRAW:
			# Window resize (or move)
			x, y = gl.getsize()
			print x, 'x', y
			v.SetSize(x, y)
			v.BindGLWindow(win, SV.IN_REPLACE)
		elif dev in (DEVICE.ESCKEY, DEVICE.WINQUIT, DEVICE.WINSHUT):
			# Quit
			if ofile:
				ofile.close()
			posix._exit(0)
			# EndCapture dumps core...
			v.EndCapture()
			v.CloseVideo()
			gl.winclose(win)

def doframe(v, ofile, x, y, t):
	cd, start = v.GetCaptureData()
	data = cd.interleave(x, y)
	cd.UnlockCaptureData()
	ofile.writeframe(t, data, None)

main()
