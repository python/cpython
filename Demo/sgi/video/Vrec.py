#! /usr/bin/env python
#! /ufs/guido/bin/sgi/python-405

# Capture a CMIF movie using the Indigo video library and board

# The CMIF video file format is documented in cmif-film.ms.
# Audio data is recorded in AIFF format, using the input sampling
# rate, source and volume set by the audio panel, in mono, 8
# bits/sample.


# Usage and help functions (keep this up-to-date if you change the program!)

def usage():
	print 'Usage: Vrec [options] [moviefile [audiofile]]'
	print
	print 'Options:'
	print '-a            : record audio as well'
	print '-q queuesize  : set the capture queue size (default 2)'
	print '-r rate       : capture 1 out of every "rate" frames', \
	                     '(default and min 2)'
	print '-w width      : initial window width', \
                             '(default 256, use 0 for interactive placement)'
	print '-n            : Don\'t write to file, only timing info'
	print '-d            : drop fields if needed'
	print '-g bits       : greyscale (2, 4 or 8 bits)'
	print '-G            : 2-bit greyscale dithered'
	print '-m            : monochrome dithered'
	print '-M value      : monochrome thresholded with value'
	print '-f            : Capture fields (in stead of frames)'
	print '-P frames     : preallocate space for "frames" frames'
	print 'moviefile     : here goes the movie data (default film.video)'
	print 'audiofile     : with -a, here goes the audio data', \
		  	     '(default film.aiff)'

def help():
	print 'Press the left mouse button to start recording, release it to'
	print 'end recording.  You can record as many times as you wish, but'
	print 'each recording overwrites the output file(s) -- only the last'
	print 'recording is kept.'
	print
	print 'Press ESC or use the window manager Quit or Close window option'
	print 'to quit.  If you quit before recording anything, the output'
	print 'file(s) are not touched.'


# Imported modules

import sys
sys.path.append('/ufs/guido/src/video')
import sv, SV
import VFile
import gl, GL, DEVICE
import al, AL
import time
import posix
import getopt
import string
import imageop
import sgi


# Main program

def main():
	format = SV.RGB8_FRAMES
	qsize = 2
	audio = 0
	rate = 2
	width = 0
	norecord = 0
	drop = 0
	mono = 0
	grey = 0
	greybits = 0
	monotreshold = -1
	fields = 0
	preallocspace = 0

	# Parse command line
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'aq:r:w:ndg:mM:GfP:')
	except getopt.error, msg:
		sys.stdout = sys.stderr
		print 'Error:', msg, '\n'
		usage()
		sys.exit(2)

	# Interpret options
	try:
		for opt, arg in opts:
			if opt == '-a':
				audio = 1
			elif opt == '-q':
				qsize = string.atoi(arg)
			elif opt == '-r':
				rate = string.atoi(arg)
				if rate < 2:
					sys.stderr.write( \
						  '-r rate must be >= 2\n')
					sys.exit(2)
			elif opt == '-w':
				width = string.atoi(arg)
			elif opt == '-n':
				norecord = 1
			elif opt == '-d':
				drop = 1
			elif opt == '-g':
				grey = 1
				greybits = string.atoi(arg)
				if not greybits in (2, 4, 8):
					sys.stderr.write( \
				'Only 2, 4 or 8 bit greyscale supported\n')
					sys.exit(2)
			elif opt == '-G':
				grey = 1
				greybits = -2
			elif opt == '-m':
				mono = 1
			elif opt == '-M':
				mono = 1
				monotreshold = string.atoi(arg)
			elif opt == '-f':
				fields = 1
			elif opt == '-P':
				preallocspace = string.atoi(arg)
	except string.atoi_error:
		sys.stdout = sys.stderr
		print 'Option', opt, 'requires integer argument'
		sys.exit(2)

	# Check excess arguments
	# If norecord is on, refuse filename arguments
	if norecord:
		if args:
			sys.stdout = sys.stderr
			print 'With -n, no filename arguments are used\n'
			usage()
			sys.exit(2)
	elif args[2:]:
		sys.stdout = sys.stderr
		print 'Too many filename arguments\n'
		usage()
		sys.exit(2)

	# Process file arguments
	if args:
		filename = args[0]
	else:
		filename = 'film.video'

	if args[1:] and not audio:
		sys.stderr.write('-a turned on by appearance of 2nd file\n')
		audio = 1

	if audio:
		if args[1:]:
			audiofilename = args[1]
		else:
			audiofilename = 'film.aiff'
	else:
		audiofilename = None

	if norecord:
		filename = audiofilename = ''

	# Open video
	v = sv.OpenVideo()
	# Determine maximum window size based on signal standard
	param = [SV.BROADCAST, 0]
	v.GetParam(param)
	if param[1] == SV.PAL:
		x = SV.PAL_XMAX
		y = SV.PAL_YMAX
	elif param[1] == SV.NTSC:
		x = SV.NTSC_XMAX
		y = SV.NTSC_YMAX
	else:
		print 'Unknown video standard', param[1]
		sys.exit(1)

	gl.foreground()
	gl.maxsize(x, y)
	gl.keepaspect(x, y)
	gl.stepunit(8, 6)
	if width:
		height = width*3/4
		x1 = 150
		x2 = x1 + width-1
		y2 = 768-150
		y1 = y2-height+1
		gl.prefposition(x1, x2, y1, y2)
	win = gl.winopen(filename)
	if width:
		gl.maxsize(x, y)
		gl.keepaspect(x, y)
		gl.stepunit(8, 6)
		gl.winconstraints()
	x, y = gl.getsize()
	print x, 'x', y

	v.SetSize(x, y)

	if drop:
		param = [SV.FIELDDROP, 1, SV.GENLOCK, SV.GENLOCK_OFF]
	else:
		param = [SV.FIELDDROP, 0, SV.GENLOCK, SV.GENLOCK_ON]
	if mono or grey:
		param = param+[SV.COLOR, SV.MONO, SV.DITHER, 0, \
			       SV.INPUT_BYPASS, 1]
	else:
		param = param+[SV.COLOR, SV.DEFAULT_COLOR, SV.INPUT_BYPASS, 0]

	v.BindGLWindow(win, SV.IN_REPLACE)
	v.SetParam(param)

	gl.qdevice(DEVICE.LEFTMOUSE)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.ESCKEY)

	help()

	while 1:
		dev, val = gl.qread()
		if dev == DEVICE.LEFTMOUSE:
			if val == 1:
				info = format, x, y, qsize, rate
				record(v, info, filename, audiofilename,\
					  mono, grey, greybits, monotreshold, \
					  fields, preallocspace)
		elif dev == DEVICE.REDRAW:
			# Window resize (or move)
			x, y = gl.getsize()
			print x, 'x', y
			v.SetSize(x, y)
			v.BindGLWindow(win, SV.IN_REPLACE)
		elif dev in (DEVICE.ESCKEY, DEVICE.WINQUIT, DEVICE.WINSHUT):
			# Quit
			v.CloseVideo()
			gl.winclose(win)
			break


# Record until the mouse is released (or any other GL event)
# XXX audio not yet supported

def record(v, info, filename, audiofilename, mono, grey, greybits, \
	  monotreshold, fields, preallocspace):
	import thread
	format, x, y, qsize, rate = info
	fps = 59.64 # Fields per second
	# XXX (Strange: need fps of Indigo monitor, not of PAL or NTSC!)
	tpf = 1000.0 / fps # Time per field in msec
	if filename:
		vout = VFile.VoutFile(filename)
		if mono:
			format = 'mono'
		elif grey and greybits == 8:
			format = 'grey'
		elif grey:
			format = 'grey'+`abs(greybits)`
		else:
			format = 'rgb8'
		vout.setformat(format)
		vout.setsize(x, y)
		if fields:
			vout.setpf((1, -2))
		vout.writeheader()
		if preallocspace:
			print 'Preallocating space...'
			vout.prealloc(preallocspace)
			print 'done.'
		MAXSIZE = 20 # XXX should be a user option
		import Queue
		queue = Queue.Queue(MAXSIZE)
		done = thread.allocate_lock()
		done.acquire_lock()
		convertor = None
		if grey:
			if greybits == 2:
				convertor = imageop.grey2grey2
			elif greybits == 4:
				convertor = imageop.grey2grey4
			elif greybits == -2:
				convertor = imageop.dither2grey2
		thread.start_new_thread(saveframes, \
			  (vout, queue, done, mono, monotreshold, convertor))
		if audiofilename:
			audiodone = thread.allocate_lock()
			audiodone.acquire_lock()
			audiostop = []
			initaudio(audiofilename, audiostop, audiodone)
	gl.wintitle('(rec) ' + filename)
	lastid = 0
	t0 = time.time()
	count = 0
	ids = []
	v.InitContinuousCapture(info)
	while not gl.qtest():
		try:
			cd, id = v.GetCaptureData()
		except sv.error:
			#time.sleep(0.010) # XXX is this necessary?
			sgi.nap(1)	# XXX Try by Jack
			continue
		ids.append(id)
		
		id = id + 2*rate
##		if id <> lastid + 2*rate:
##			print lastid, id
		lastid = id
		count = count+1
		if fields:
			data1, data2 = cd.GetFields()
			cd.UnlockCaptureData()
			if filename:
				queue.put((data1, int(id*tpf)))
				queue.put((data2, int((id+1)*tpf)))
		else:
			data = cd.InterleaveFields(1)
			cd.UnlockCaptureData()
			if filename:
				queue.put((data, int(id*tpf)))
	t1 = time.time()
	gl.wintitle('(busy) ' + filename)
	print lastid, 'fields in', round(t1-t0, 3), 'sec',
	print '--', round(lastid/(t1-t0), 1), 'fields/sec'
	print 'Captured',count*2, 'fields,',
	print round(count*2/(t1-t0), 1), 'f/s',
	if lastid:
		print '(',
		print round(count*200.0/lastid), '%, or',
		print round(count*rate*200.0/lastid), '% of wanted rate )',
	print
	if ids:
		print 'Ids:',
		t0 = ids[0]
		del ids[0]
		for t1 in ids:
			print t1-t0,
			t0 = t1
		print
	if filename and audiofilename:
		audiostop.append(None)
		audiodone.acquire_lock()
	v.EndContinuousCapture()
	if filename:
		queue.put(None) # Sentinel
		done.acquire_lock()
	gl.wintitle('(done) ' + filename)


# Thread to save the frames to the file

def saveframes(vout, queue, done, mono, monotreshold, convertor):
	while 1:
		x = queue.get()
		if not x:
			break
		data, t = x
		if convertor:
			data = convertor(data, len(data), 1)
		elif mono and monotreshold >= 0:
			data = imageop.grey2mono(data, len(data), 1,\
				  monotreshold)
		elif mono:
			data = imageop.dither2mono(data, len(data), 1)
		vout.writeframe(t, data, None)
	sys.stderr.write('Done writing video\n')
	vout.close()
	done.release_lock()


# Initialize audio recording

AQSIZE = 8000 # XXX should be a user option

def initaudio(filename, stop, done):
	import thread, aifc
	afile = aifc.open(filename, 'w')
	afile.setnchannels(AL.MONO)
	afile.setsampwidth(AL.SAMPLE_8)
	params = [AL.INPUT_RATE, 0]
	al.getparams(AL.DEFAULT_DEVICE, params)
	print 'audio sampling rate =', params[1]
	afile.setframerate(params[1])
	c = al.newconfig()
	c.setchannels(AL.MONO)
	c.setqueuesize(AQSIZE)
	c.setwidth(AL.SAMPLE_8)
	aport = al.openport(filename, 'r', c)
	thread.start_new_thread(audiorecord, (afile, aport, stop, done))


# Thread to record audio samples

def audiorecord(afile, aport, stop, done):
	while not stop:
		data = aport.readsamps(AQSIZE/2)
		afile.writesampsraw(data)
		del data
	afile.close()
	print 'Done writing audio'
	done.release_lock()


# Don't forget to call the main program

try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
