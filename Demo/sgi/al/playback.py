# Read mono 16bit samples from stdin and write them to the audio device.
# Assume the sampling rate is compatible.
# Use a small queue size to minimize delays.

import al, sys
import AL

BUFSIZE = 2000
QSIZE = 4000

def main():
	c = al.newconfig()
	c.setchannels(AL.MONO)
	c.setqueuesize(QSIZE)
	p = al.openport('', 'w', c)
	while 1:
		data = sys.stdin.read(BUFSIZE)
		p.writesamps(data)

try:
	main()
except KeyboardInterrupt:
	sys.exit(1)
