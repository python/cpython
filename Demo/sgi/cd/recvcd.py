# Receive UDP packets from sendcd.py and play them on the speaker or
# audio jack.

import al, AL
from socket import *

PORT = 50505 # Must match the port in sendcd.py

def main():
	s = socket(AF_INET, SOCK_DGRAM)
	s.bind('', PORT)

	c = al.newconfig()
	c.setchannels(2)
	c.setwidth(2)
	p = al.openport('Audio from CD', 'w', c)
	al.setparams(AL.DEFAULT_DEVICE, [AL.OUTPUT_RATE, AL.RATE_44100])

	N = 2352
	while 1:
		data = s.recv(N)
		if not data:
			print 'EOF'
			break
		p.writesamps(data)
