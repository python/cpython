# Read CD audio data from the SCSI bus and play it back over the
# built-in speaker or audio jack.

import al
import AL
import cd
import CD

def playaudio(port, type, audio):
##	print 'playaudio'
	port.writesamps(audio)

callbacks = ['audio', 'pnum', 'index', 'ptime', 'atime', 'catalog', 'ident', 'control']

def callback(port, type, data):
	print 'type', callbacks[type], 'data', `data`

def main():
	player = cd.open()
	parser = cd.createparser()

	state, track, min, sec, frame, abs_min, abs_sec, abs_frame, \
		  total_min, total_sec, total_frame, first, last, scsi_audio, \
		  cur_block, dum1, dum2, dum3 = player.getstatus()
	print `state, track, min, sec, frame, abs_min, abs_sec, abs_frame, \
		  total_min, total_sec, total_frame, first, last, scsi_audio, \
		  cur_block, dum1, dum2, dum3`

	if state <> CD.READY:
		player.close()
		raise 'playcd.Error', 'CD not ready'
	if not scsi_audio:
		player.close()
		raise 'playcd.Error', 'not an audio-capable CD-ROM player'

	for i in range(first, last+1):
		trackinfo = player.gettrackinfo(i)
		print `trackinfo`

	size = player.bestreadsize()

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

		parser.setcallback(CD.AUDIO, playaudio, port)
		for i in range(1, 8):
			parser.setcallback(i, callback, port)
		parser.removecallback(CD.ATIME)
		parser.removecallback(CD.PTIME)

		while 1:
			frames = player.readda(size)
			if frames == '':
				break
			parser.parseframe(frames)
	except KeyboardInterrupt:
		pass

	al.setparams(AL.DEFAULT_DEVICE, oldparams)
	player.close()
	parser.deleteparser()

main()
