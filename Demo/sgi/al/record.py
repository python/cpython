# Record mono 16bits samples from the audio device and send them to stdout.
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
	p = al.openport('', 'r', c)
	while 1:
		data = p.readsamps(BUFSIZE)
		sys.stdout.write(data)

try:
	main()
except KeyboardInterrupt:
	sys.exit(1)
