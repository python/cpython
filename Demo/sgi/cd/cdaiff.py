# Dump CD audio on disk (in AIFF format; stereo, 16 bit samples, 44.1 kHz).
#
# Each argument is either a track to play or a quoted 7-tuple:
#	'(track, min1, sec1, frame1, min2, sec2, frame2)'
# to play the track from min1:sec1:frame1 to min2:sec2:frame2.
# If track is zero, times are absolute instead.

import sys
import string
import readcd
import aiff
import AL
import CD

def writeaudio(a, type, data):
	a.writesampsraw(data)

def ptimecallback(a, type, (min, sec, frame)):
	if frame == 0:
		print 'T =', min, ':', sec

def main():
	a = aiff.Aiff().init(sys.argv[1], 'w')
	a.sampwidth = AL.SAMPLE_16
	a.nchannels = AL.STEREO
	a.samprate = AL.RATE_44100
	r = readcd.Readcd().init()
	l = []
	for arg in sys.argv[2:]:
		x = eval(arg)
		try:
			l = len(x)
			r.appendstretch(x[0], x[1])
		except TypeError:
			r.appendtrack(x)
	r.setcallback(CD.AUDIO, writeaudio, a)
	r.setcallback(CD.PTIME, ptimecallback, None)
	try:
		r.play()
	except KeyboardInterrupt:
		pass
	a.destroy()

main()
