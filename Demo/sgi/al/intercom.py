# intercom -- use mike and headset to *talk* to a person on another host.
# For SGI 4D/35 or Indigo running IRIX 4.0.
# Uses 16 bit sampling at 16000 samples/sec, or 32000 bytes/sec,
# tranmitted in 32 1000-byte UDP packets.  (In each direction!)
#
# usage:
#	intercom hostname	- start talking to person on other host
#	intercom -r hostname	- called remotely to do the setup

from names import *
import sys, time, posix, gl, fl, FL, al, AL, getopt, rand
from socket import *

# UDP port numbers used (one for each direction!)
PORT1 = 51042
PORT2 = PORT1+1

# Figure out the user name
try:
	user = posix.environ['LOGNAME']
except:
	user = posix.environ['USER']

# Debug flags (Implemented as a list; non-empty means debugging is on)
debug = []

def main():
	remote = 0
	opts, args = getopt.getopt(sys.argv[1:], 'rd')
	for opt, arg in opts:
		if opt == '-r': remote = 1
		elif opt == '-d': debug.append(opt)
	if len(args) <> 1:
		msg = 'usage: intercom [-d] [-r] hostname'
		msg = msg + ' (-r is for internal use only!)\n'
		sys.stderr.write(msg)
		sys.exit(2)
	if remote:
		server(args[0])
	else:
		client(args[0])

def client(hostname):
	print 'client starting'
	cmd = 'rsh ' + hostname + ' "cd ' + AUDIODIR
	cmd = cmd + '; DISPLAY=:0; export DISPLAY'
	cmd = cmd + '; ' + PYTHON + ' intercom.py -r '
	for flag in debug: cmd = cmd + flag + ' '
	cmd = cmd + gethostname()
	cmd = cmd + '"'
	if debug: print cmd
	pipe = posix.popen(cmd, 'r')
	ack = 0
	nak = 0
	while 1:
		line = pipe.readline()
		if not line: break
		sys.stdout.write('remote: ' + line)
		if line == 'NAK\n':
			nak = 1
			break
		elif line == 'ACK\n':
			ack = 1
			break
	if nak:
		print 'Remote user doesn\'t want to talk to you.'
		return
	if not ack:
		print 'No acknowledgement (remote side crashed?).'
		return
	#
	print 'Ready...'
	#
	s = socket(AF_INET, SOCK_DGRAM)
	s.bind('', PORT2)
	#
	otheraddr = gethostbyname(hostname), PORT1
	try:
		try:
			ioloop(s, otheraddr)
		except KeyboardInterrupt:
			log('client got intr')
		except error:
			log('client got error')
	finally:
		s.sendto('', otheraddr)
		log('client finished sending empty packet to server')
	#
	log('client exit')
	print 'Done.'

def server(hostname):
	print 'server starting'
	sys.stdout.flush()
	# 
	if not remotedialog():
		print 'NAK'
		return
	#
	print 'ACK'
	#
	s = socket(AF_INET, SOCK_DGRAM)
	s.bind('', PORT1)
	#
	# Close std{in,out,err} so rsh will exit; reopen them as dummies
	#
	sys.stdin.close()
	sys.stdin = open('/dev/null', 'r')
	sys.stdout.close()
	sys.stdout = open('/dev/null', 'w')
	sys.stderr.close()
	if debug:
		sys.stderr = open('/tmp/intercom.err', 'a')
	else:
		sys.stderr = open('/dev/null', 'w')
	#
	ioloop(s, (gethostbyname(hostname), PORT2))
	log('server exit')
	sys.exit(0)

def remotedialog():
	gl.foreground()
	gl.ringbell()
	m1 = user + ' wants to talk to you over the audio channel.'
	m2 = 'If it\'s OK, put on your headset and click Yes.'
	m3 = 'If you\'re too busy, click No.'
	return fl.show_question(m1, m2, m3)

def ioloop(s, otheraddr):
	#
	dev = AL.DEFAULT_DEVICE
	params = al.queryparams(dev)
	al.getparams(dev, params)
	time.sleep(1)
	saveparams = params[:]
	for i in range(0, len(params), 2):
		if params[i] in (AL.INPUT_RATE, AL.OUTPUT_RATE):
			params[i+1] = AL.RATE_16000
		elif params[i] == AL.INPUT_SOURCE:
			params[i+1] = AL.INPUT_MIC
	try:
		al.setparams(dev, params)
		ioloop1(s, otheraddr)
	finally:
		al.setparams(dev, saveparams)

def ioloop1(s, otheraddr):
	#
	# Watch out! data is in bytes, but the port counts in samples,
	# which are two bytes each (for 16-bit samples).
	# Luckily, we use mono, else it would be worse (2 samples/frame...)
	#
	SAMPSPERBUF = 500
	BYTESPERSAMP = 2 # AL.SAMPLE_16
	BUFSIZE = BYTESPERSAMP*SAMPSPERBUF
	QSIZE = 4*SAMPSPERBUF
	#
	config = al.newconfig()
	config.setqueuesize(QSIZE)
	config.setwidth(AL.SAMPLE_16)
	config.setchannels(AL.MONO)
	#
	pid = posix.fork()
	if pid:
		# Parent -- speaker/headphones handler
		log('parent started')
		spkr = al.openport('spkr', 'w', config)
		while 1:
			data = s.recv(BUFSIZE)
			if len(data) == 0:
				# EOF packet
				log('parent got empty packet; killing child')
				posix.kill(pid, 15)
				return
			# Discard whole packet if we are too much behind
			if spkr.getfillable() > len(data) / BYTESPERSAMP:
				if len(debug) >= 2:
					log('parent Q full; dropping packet')
				spkr.writesamps(data)
	else:
		# Child -- microphone handler
		log('child started')
		try:
		    try:
			    mike = al.openport('mike', 'r', config)
			    # Sleep a while to let the other side get started
			    time.sleep(1)
			    # Drain the queue before starting to read
			    data = mike.readsamps(mike.getfilled())
			    # Loop, sending packets from the mike to the net
			    while 1:
				    data = mike.readsamps(SAMPSPERBUF)
				    s.sendto(data, otheraddr)
		    except KeyboardInterrupt:
			    log('child got interrupt; exiting')
			    posix._exit(0)
		    except error:
			    log('child got error; exiting')
			    posix._exit(1)
		finally:
			log('child got unexpected error; leaving w/ traceback')

def log(msg):
	if not debug: return
	if type(msg) <> type(''):
		msg = `msg`
	
	f = open('/tmp/intercom.log', 'a')
	f.write(`sys.argv` + ' ' + `posix.getpid()` + ': ' + msg + '\n')
	f.close()

main()
