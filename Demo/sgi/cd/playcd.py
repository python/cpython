# Play CD audio on speaker or headphones.

callbacktypes = ['audio','pnum','index','ptime','atime','catalog','ident','control']

def playaudio(port, type, audio):
	port.writesamps(audio)

def prtrack(cdinfo, type, pnum):
	if cdinfo.track[pnum] <> '':
		print 'playing "' + cdinfo.track[pnum] + '"'
	else:
		print callbacktypes[type]+': '+`pnum`

def callback(arg, type, data):
	print callbacktypes[type]+': '+`data`

def tcallback(arg, type, data):
	print callbacktypes[type]+': '+triple(data)

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
	import sys, readcd, al, AL, CD, cdplayer
	verbose = 0
	r = readcd.Readcd().init()
	prstatus(r.getstatus())
	prtrackinfo(r.gettrackinfo())
	cdinfo = cdplayer.Cdplayer().init(r.gettrackinfo())
	if cdinfo.title <> '':
		print 'Title: "' + cdinfo.title + '"'
	if cdinfo.artist <> '':
		print 'Artist: ' + cdinfo.artist
	for arg in sys.argv[1:]:
		if arg == '-v':
			verbose = 1
			continue
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
		if verbose:
			r.setcallback(CD.PTIME, tcallback, None)
			r.setcallback(CD.ATIME, tcallback, None)
		else:
			r.removecallback(CD.PTIME)
			r.removecallback(CD.ATIME)
		r.setcallback(CD.PNUM, prtrack, cdinfo)
		r.setcallback(CD.AUDIO, playaudio, port)

		data = r.play()
	except KeyboardInterrupt:
		status = r.getstatus()
		print 'Interrupted at '+triple(status[2])+' into track '+ \
			  `status[1]`+' (absolute time '+triple(status[3])+')'
	al.setparams(AL.DEFAULT_DEVICE, oldparams)

main()
