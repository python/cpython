#! /ufs/guido/bin/sgi/python-405
#! /ufs/guido/bin/sgi/python

# Capture a CMIF movie using the Indigo video library and board


# Usage:
#
# makemovie [-a] [-q queuesize] [-r rate] [-w width] [moviefile [audiofile]]


# Options:
#
# -a            : record audio as well
# -q queuesize  : set the capture queue size (default 2)
# -r rate       : capture 1 out of every 'rate' frames (default and min 2)
# -w width      : initial window width (default interactive placement)
# -n            : Don't write to file, only timing info
# -d		: drop fields if needed
# -g		: greyscale
# -m		: monochrome dithered
# -M value	: monochrome tresholded with value
# 
# moviefile     : here goes the movie data (default film.video);
#                 the format is documented in cmif-film.ms
# audiofile     : with -a, here goes the audio data (default film.aiff);
#                 audio data is recorded in AIFF format, using the
#                 input sampling rate, source and volume set by the
#                 audio panel, in mono, 8 bits/sample


# User interface:
#
# Start the application.  Resize the window to the desired movie size.
# Press the left mouse button to start recording, release it to end
# recording.  You can record as many times as you wish, but each time
# you overwrite the output file(s), so only the last recording is
# kept.
#
# Press ESC or select the window manager Quit or Close window option
# to quit.  If you quit before recording anything, the output file(s)
# are not touched.


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
	monotreshold = -1

	opts, args = getopt.getopt(sys.argv[1:], 'aq:r:w:ndgmM:')
	for opt, arg in opts:
		if opt == '-a':
			audio = 1
		elif opt == '-q':
			qsize = string.atoi(arg)
		elif opt == '-r':
			rate = string.atoi(arg)
			if rate < 2:
				sys.stderr.write('-r rate must be >= 2\n')
				sys.exit(2)
		elif opt == '-w':
			width = string.atoi(arg)
		elif opt == '-n':
			norecord = 1
		elif opt == '-d':
			drop = 1
		elif opt == '-g':
			grey = 1
		elif opt == '-m':
			mono = 1
		elif opt == '-M':
			mono = 1
			monotreshold = string.atoi(arg)

	if args[2:]:
		sys.stderr.write('usage: Vrec [options] [file [audiofile]]\n')
		sys.exit(2)

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
		gl.prefsize(width, width*3/4)
	win = gl.winopen(filename)
	if width:
		gl.maxsize(x, y)
		gl.keepaspect(x, y)
		gl.stepunit(8, 6)
		gl.winconstraints()
	x, y = gl.getsize()
	print x, 'x', y

	v.SetSize(x, y)
<<<<<<< Vrec.py

	if drop:
		param = [SV.FIELDDROP, 1, SV.GENLOCK, SV.GENLOCK_OFF]
	else:
		param = [SV.FIELDDROP, 0, SV.GENLOCK, SV.GENLOCK_ON]
	if mono or grey:
		param = param+[SV.COLOR, SV.MONO, SV.INPUT_BYPASS, 1]
	else:
		param = param+[SV.COLOR, SV.DEFAULT_COLOR, SV.INPUT_BYPASS, 0]
	v.SetParam(param)
	
=======

	# VERY IMPORTANT (for PAL at least): don't drop fields!
	param = [SV.FIELDDROP, 0]
	v.SetParam(param)

>>>>>>> 1.7
	v.BindGLWindow(win, SV.IN_REPLACE)

	gl.qdevice(DEVICE.LEFTMOUSE)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.ESCKEY)

	print 'Press left mouse to start recording, release it to stop'

	while 1:
		dev, val = gl.qread()
		if dev == DEVICE.LEFTMOUSE:
			if val == 1:
				info = format, x, y, qsize, rate
				record(v, info, filename, audiofilename,\
					  mono, grey, monotreshold)
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

def record(v, info, filename, audiofilename, mono, grey, monotreshold):
	import thread
	format, x, y, qsize, rate = info
	fps = 59.64 # Fields per second
	# XXX (Strange: need fps of Indigo monitor, not of PAL or NTSC!)
	tpf = 1000.0 / fps # Time per field in msec
	if filename:
		vout = VFile.VoutFile().init(filename)
		if mono:
			vout.format = 'mono'
		elif grey:
			vout.format = 'grey'
		else:
			vout.format = 'rgb8'
		vout.width = x
		vout.height = y
		vout.writeheader()
		MAXSIZE = 20 # XXX should be a user option
		import Queue
		queue = Queue.Queue().init(MAXSIZE)
		done = thread.allocate_lock()
		done.acquire_lock()
		thread.start_new_thread(saveframes, \
			  (vout, queue, done, mono, monotreshold))
		if audiofilename:
			audiodone = thread.allocate_lock()
			audiodone.acquire_lock()
			audiostop = []
			initaudio(audiofilename, audiostop, audiodone)
	gl.wintitle('(rec) ' + filename)
	lastid = 0
	t0 = time.millitimer()
	count = 0
	timestamps = []
	ids = []
	v.InitContinuousCapture(info)
	while not gl.qtest():
		try:
			cd, id = v.GetCaptureData()
		except sv.error:
			time.millisleep(10) # XXX is this necessary?
			continue
		timestamps.append(time.millitimer())
		ids.append(id)
		
		id = id + 2*rate
##		if id <> lastid + 2*rate:
##			print lastid, id
		lastid = id
		data = cd.InterleaveFields(1)
		cd.UnlockCaptureData()
		count = count+1
		if filename:
			queue.put(data, int(id*tpf))
	t1 = time.millitimer()
	gl.wintitle('(busy) ' + filename)
	print lastid, 'fields in', t1-t0, 'msec',
	print '--', 0.1 * int(lastid * 10000.0 / (t1-t0)), 'fields/sec'
	print 'Captured',count*2, 'fields,', 0.1*int(count*20000.0/(t1-t0)), 'f/s',
	print count*200.0/lastid, '%,',
	print count*rate*200.0/lastid, '% of wanted rate'
	t0 = timestamps[0]
	del timestamps[0]
	print 'Times:',
	for t1 in timestamps:
		print t1-t0,
		t0 = t1
	print
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

def saveframes(vout, queue, done, mono, monotreshold):
	while 1:
		x = queue.get()
		if not x:
			break
		data, t = x
		if mono and monotreshold >= 0:
			data = imageop.grey2mono(data, len(data), 1,\
				  monotreshold)
		elif mono:
			data = imageop.dither2mono(data, len(data), 1)
		vout.writeframe(t, data, None)
		del data
	sys.stderr.write('Done writing video\n')
	vout.close()
	done.release_lock()


# Initialize audio recording

AQSIZE = 8000 # XXX should be a user option

def initaudio(filename, stop, done):
	import thread, aiff
	afile = aiff.Aiff().init(filename, 'w')
	afile.nchannels = AL.MONO
	afile.sampwidth = AL.SAMPLE_8
	params = [AL.INPUT_RATE, 0]
	al.getparams(AL.DEFAULT_DEVICE, params)
	print 'audio sampling rate =', params[1]
	afile.samprate = params[1]
	c = al.newconfig()
	c.setchannels(AL.MONO)
	c.setqueuesize(AQSIZE)
	c.setwidth(AL.SAMPLE_8)
	aport = al.openport(filename, 'r', c)
	thread.start_new_thread(audiorecord, (afile, aport, stop, done))


# Thread to record audio samples

# XXX should use writesampsraw for efficiency, but then destroy doesn't
# XXX seem to set the #samples in the header correctly

def audiorecord(afile, aport, stop, done):
	while not stop:
		data = aport.readsamps(AQSIZE/2)
##		afile.writesampsraw(data)
		afile.writesamps(data)
		del data
	afile.destroy()
	print 'Done writing audio'
	done.release_lock()


# Don't forget to call the main program

try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
