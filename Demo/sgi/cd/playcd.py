def playaudio(port, type, audio):
	port.writesamps(audio)

def callback(arg, type, data):
	print `type`,`data`

def triple((a, b, c)):
	return zfill(a) + ':' + zfill(b) + ':' + zfill(c)

def zfill(n):
	s = `n`
	return '0' * (2 - len(s)) + s

def prtrackinfo(info):
	for i in range(len(info)):
		start, total = info[i]
		print 'Track', zfill(i+1), triple(start), triple(total)

statedict = ['ERROR', 'NODISK', 'READY', 'PLAYING', 'PAUSED', 'STILL']

def prstatus(status):
	state, track, curtime, abstime, totaltime, first, last, \
		scsi_audio, cur_block, dummy = status
	print 'Status:',
	if 0 <= state < len(statedict):
		print statedict[state]
	else:
		print state
	print 'Track: ', track
	print 'Time:  ', triple(curtime)
	print 'Abs:   ', triple(abstime)
	print 'Total: ', triple(totaltime)
	print 'First: ', first
	print 'Last:  ', last
	print 'SCSI:  ', scsi_audio
	print 'Block: ', cur_block
	print 'Future:', dummy

def main():
	import sys, readcd, al, string, AL, CD
	r = readcd.Readcd().init()
	prstatus(r.getstatus())
	prtrackinfo(r.gettrackinfo())
	l = []
	for arg in sys.argv[1:]:
		x = eval(arg)
		try:
			l = len(x)
			r.appendstretch(x[0], x[1])
		except TypeError:
			r.appendtrack(x)
	try:
		oldparams = [AL.OUTPUT_RATE, 0]
		params = oldparams[:]
		al.getparams(AL.DEFAULT_DEVICE, oldparams)
		params[1] = AL.RATE_44100
		al.setparams(AL.DEFAULT_DEVICE, params)
		config = al.newconfig()
		config.setwidth(AL.SAMPLE_16)
		config.setchannels(AL.STEREO)
		port = al.openport('CD Player', 'w', config)

		for i in range(8):
			r.setcallback(i, callback, None)
		r.removecallback(CD.PTIME)
		r.removecallback(CD.ATIME)
		r.setcallback(CD.AUDIO, playaudio, port)

		data = r.play()
	except KeyboardInterrupt:
		pass
	al.setparams(AL.DEFAULT_DEVICE, oldparams)
	r.player.close()
	r.parser.deleteparser()

main()
