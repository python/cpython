# Play old style sound files (Guido's private format)

import al, sys, time
import AL

BUFSIZE = 8000

def main():
	if len(sys.argv) < 2:
		f = sys.stdin
		filename = sys.argv[0]
	else:
		if len(sys.argv) <> 2:
			sys.stderr.write('usage: ' + \
					 sys.argv[0] + ' filename\n')
			sys.exit(2)
		filename = sys.argv[1]
		f = open(filename, 'r')
	#
	magic = f.read(4)
	extra = ''
	if magic == '0008':
		rate = 8000
	elif magic == '0016':
		rate = 16000
	elif magic == '0032':
		rate = 32000
	else:
		sys.stderr.write('no magic header; assuming 8k samples/sec.\n')
		rate = 8000
		extra = magic
	#
	pv = [AL.OUTPUT_RATE, rate]
	al.setparams(AL.DEFAULT_DEVICE, pv)
	c = al.newconfig()
	c.setchannels(AL.MONO)
	c.setwidth(AL.SAMPLE_8)
	port = al.openport(filename, 'w', c)
	if extra:
		port.writesamps(extra)
	while 1:
		buf = f.read(BUFSIZE)
		if not buf: break
		port.writesamps(buf)
	while port.getfilled() > 0:
		time.sleep(0.1)

try:
	main()
except KeyboardInterrupt:
	sys.exit(1)
