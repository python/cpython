# Capture a movie using the Indigo video library and board

import sv, SV
import VFile
import gl, GL, DEVICE
import al, AL

import time
import sys
import posix
import getopt
import string

def main():
	QSIZE = 16
	TIME = 5
	audio = 0

	opts, args = getopt.getopt(sys.argv[1:], 'aq:t:')
	for opt, arg in opts:
		if opt == '-a':
			audio = 1
		elif opt == '-q':
			QSIZE = string.atoi(arg)
		elif opt == '-t':
			TIME = string.atoi(arg)

	if args:
		filename = args[0]
	else:
		filename = 'film.video'

	if audio:
		if args[1:]:
			audiofilename = args[1]
		else:
			audiofilename = 'film.aiff'

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
	afile = None
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
				if audio:
					afile = initaudio(audiofilename)
				continue
			# Mouse up -- start actual recording
			global recording, stop_recording
			if audio:
				stop_recording = 0
				recording.release()
			t0 = time.millitimer()
			v.StartCapture()
			while 1:
				t = time.millitimer() - t0
				if t >= TIME*1000:
					break
				if v.GetCaptured() > 2:
					doframe(v, ofile, x, y, t)
			v.StopCapture()
			stop_recording = 1
			while v.GetCaptured() > 0:
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
			if afile:
				afile.destroy()
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

AQSIZE = 16000

def initaudio(filename):
	import thread, aiff
	global recording, stop_recording
	afile = aiff.Aiff().init(filename, 'w')
	afile.nchannels = AL.MONO
	afile.sampwidth = AL.SAMPLE_8
	params = [AL.INPUT_RATE, 0]
	al.getparams(AL.DEFAULT_DEVICE, params)
	print 'rate =', params[1]
	afile.samprate = params[1]
	c = al.newconfig()
	c.setchannels(AL.MONO)
	c.setqueuesize(AQSIZE)
	c.setwidth(AL.SAMPLE_8)
	aport = al.openport(filename, 'r', c)
	recording = thread.allocate_lock()
	recording.acquire()
	stop_recording = 0
	thread.start_new_thread(recorder, (afile, aport))
	return afile

def recorder(afile, aport):
	# XXX recording more than one fragment doesn't work
	# XXX (the thread never dies)
	recording.acquire()
	while not stop_recording:
		data = aport.readsamps(AQSIZE/2)
		afile.writesampsraw(data)
		del data

main()
