# Listen to the input on host argv[1].

import sys, al, AL, posix

BUFSIZE = 2000
QSIZE = 4000

def main():
	if len(sys.argv) <> 2:
		sys.stderr.write('usage: ' + sys.argv[0] + ' hostname\n')
		sys.exit(2)
	hostname = sys.argv[1]
	cmd = 'exec rsh </dev/null ' + hostname + \
		' "cd /ufs/guido/mm/demo/audio; ' + \
		'exec /ufs/guido/bin/sgi/python record.py"'
	pipe = posix.popen(cmd, 'r')
	config = al.newconfig()
	config.setchannels(AL.MONO)
	config.setqueuesize(QSIZE)
	port = al.openport('', 'w', config)
	while 1:
		data = pipe.read(BUFSIZE)
		if not data:
			sts = pipe.close()
			sys.stderr.write(sys.argv[0] + ': end of data\n')
			if sts: sys.stderr.write('rsh exit status '+`sts`+'\n')
			sys.exit(1)
		port.writesamps(data)
		del data

try:
	main()
except KeyboardInterrupt:
	sys.exit(1)
