#! /ufs/guido/bin/sgi/python-405
#! /ufs/guido/bin/sgi/python

# Capture a CMIF movie using the Indigo video library and board


# Usage:
#
# makemovie [-a] [-q queuesize] [-r n/d] [moviefile [audiofile]]


# Options:
#
# -a            : record audio as well
# -q queuesize  : set the capture queue size (default and max 16)
# -r n/d        : capture n out of every d frames (default 1/1)
#                 XXX doesn't work yet
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
# recording.  XXX For now, you must restart the program to record again.
# You can't resize the window once you have made a recording.
#
# Press ESC or select the window manager Quit or Close window option
# to quit.  (You can do this without recording -- then the output
# files are untouched.)  XXX Don't press ESC before the program has
# finished writing the output file -- it prints "Done writing" when it
# is done.


# XXX To do:
#
# add audio


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


# Main program

def main():
	QSIZE = 16
	TIME = 5
	audio = 0
	num, den = 1, 1

	opts, args = getopt.getopt(sys.argv[1:], 'aq:r:t:')
	for opt, arg in opts:
		if opt == '-a':
			audio = 1
		elif opt == '-q':
			QSIZE = string.atoi(arg)
		elif opt == '-r':
			[nstr, dstr] = string.splitfields(arg, '/')
			num, den = string.atoi(nstr), string.atoi(dstr)
		elif opt == '-t':
			TIME = string.atoi(arg)

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

###	v.SetSamplingRate(num, den) # XXX dumps core

	gl.qdevice(DEVICE.LEFTMOUSE)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.ESCKEY)

	print 'Press left mouse to start recording, release it to stop'

	recorded = 0

	while 1:
		dev, val = gl.qread()
		if dev == DEVICE.LEFTMOUSE:
			if val == 1:
				if recorded:
					# XXX This would dump core
					gl.ringbell()
					continue
				# Fix the window's size now
				gl.prefsize(x, y)
				gl.winconstraints()
				record(v, filename, audiofilename)
				recorded = 1
				print 'Wait until "Done writing" is printed!'
		elif dev == DEVICE.REDRAW:
			# Window resize (or move)
			if not recorded:
				x, y = gl.getsize()
				print x, 'x', y
				v.SetSize(x, y)
				v.BindGLWindow(win, SV.IN_REPLACE)
		elif dev in (DEVICE.ESCKEY, DEVICE.WINQUIT, DEVICE.WINSHUT):
			# Quit
			if not recorded:
				# XXX Avoid core dump in EndCapture
				posix._exit(0)
			v.EndCapture()
			v.CloseVideo()
			gl.winclose(win)
			break


# Record until the mouse is released (or any other GL event)
# XXX audio not yet supported

def record(v, filename, audiofilename):
	import thread
	x, y = gl.getsize()
	vout = VFile.VoutFile().init(filename)
	vout.format = 'rgb8'
	vout.width = x
	vout.height = y
	vout.writeheader()
	buffer = []
	thread.start_new_thread(saveframes, (vout, buffer))
	if audiofilename:
		initaudio(audiofilename, buffer)
	gl.wintitle('(rec) ' + filename)
	v.StartCapture()
	t0 = time.millitimer()
	while not gl.qtest():
		if v.GetCaptured() > 2:
			t = time.millitimer() - t0
			cd, st = v.GetCaptureData()
			data = cd.interleave(x, y)
			cd.UnlockCaptureData()
			buffer.append(data, t)
		else:
			time.millisleep(10)
	v.StopCapture()
	while v.GetCaptured() > 0:
		t = time.millitimer() - t0
		cd, st = v.GetCaptureData()
		data = cd.interleave(x, y)
		cd.UnlockCaptureData()
		buffer.append(data, t)
	buffer.append(None) # Sentinel
	gl.wintitle('(done) ' + filename)


# Thread to save the frames to the file

def saveframes(vout, buffer):
	while 1:
		if not buffer:
			time.millisleep(10)
		else:
			x = buffer[0]
			if not x:
				break
			del buffer[0]
			data, t = x
			vout.writeframe(t, data, None)
			del data
	sys.stderr.write('Done writing\n')
	vout.close()


# Initialize audio recording

AQSIZE = 8000

def initaudio(filename, buffer):
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
	thread.start_new_thread(audiorecord, (afile, aport, buffer))


# Thread to record audio samples

# XXX should use writesampsraw for efficiency, but then destroy doesn't
# XXX seem to set the #samples in the header correctly

def audiorecord(afile, aport, buffer):
	while buffer[-1:] <> [None]:
		data = aport.readsamps(AQSIZE/2)
##		afile.writesampsraw(data)
		afile.writesamps(data)
		del data
	afile.destroy()
	print 'Done writing audio'


# Don't forget to call the main program

main()
