import sys
import readcd
import aiff
import AL
import CD

Error = 'cdaiff.Error'

def writeaudio(a, type, data):
	a.writesampsraw(data)

def main():
	if len(sys.argv) > 1:
		a = aiff.Aiff().init(sys.argv[1], 'w')
	else:
		a = aiff.Aiff().init('@', 'w')
	a.sampwidth = AL.SAMPLE_16
	a.nchannels = AL.STEREO
	a.samprate = AL.RATE_44100
	r = readcd.Readcd().init()
	for arg in sys.argv[2:]:
		x = eval(arg)
		try:
			if len(x) <> 2:
				raise Error, 'bad argument'
			r.appendstretch(x[0], x[1])
		except TypeError:
			r.appendtrack(x)
	r.setcallback(CD.AUDIO, writeaudio, a)
	r.play()
	a.destroy()

main()
