# Receive UDP packets from sendcd.py and play them on the speaker or
# audio jack.

import al, AL
from socket import *
from cd import DATASIZE

PORT = 50505				# Must match the port in sendcd.py

def main():
	s = socket(AF_INET, SOCK_DGRAM)
	s.bind('', PORT)

	oldparams = [AL.OUTPUT_RATE, 0]
	params = oldparams[:]
	al.getparams(AL.DEFAULT_DEVICE, oldparams)
	params[1] = AL.RATE_44100
	try:
		al.setparams(AL.DEFAULT_DEVICE, params)
		config = al.newconfig()
		config.setwidth(AL.SAMPLE_16)
		config.setchannels(AL.STEREO)
		port = al.openport('CD Player', 'w', config)

		while 1:
			data = s.recv(DATASIZE)
			if not data:
				print 'EOF'
				break
			port.writesamps(data)
	except KeyboardInterrupt:
		pass

	al.setparams(AL.DEFAULT_DEVICE, oldparams)

main()
