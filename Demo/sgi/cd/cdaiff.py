import sys
import readcd
import aifc
import AL
import cd

Error = 'cdaiff.Error'

def writeaudio(a, type, data):
	a.writeframesraw(data)

def main():
	if len(sys.argv) > 1:
		a = aifc.open(sys.argv[1], 'w')
	else:
		a = aifc.open('@', 'w')
	a.setsampwidth(AL.SAMPLE_16)
	a.setnchannels(AL.STEREO)
	a.setframerate(AL.RATE_44100)
	r = readcd.Readcd()
	for arg in sys.argv[2:]:
		x = eval(arg)
		try:
			if len(x) <> 2:
				raise Error, 'bad argument'
			r.appendstretch(x[0], x[1])
		except TypeError:
			r.appendtrack(x)
	r.setcallback(cd.audio, writeaudio, a)
	r.play()
	a.close()

main()
