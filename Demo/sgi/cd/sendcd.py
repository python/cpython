# Read CD audio data from the SCSI CD player and send it as UDP
# packets to "readcd.py" on another host.
# Option:
# "-l" lists track info and quits.
# "-s" displays status and quits.

import cd
import sys
from socket import *
import getopt

HOST = 'voorn.cwi.nl'			# The host where readcd.py is run
PORT = 50505				# Must match the port in readcd.py

def main():
	try:
		optlist, args = getopt.getopt(sys.argv[1:], 'ls')
	except getopt.error, msg:
		sys.stderr.write(msg + '\n')
		sys.exit(2)

	player = cd.open()
	prstatus(player)
	size = player.bestreadsize()

	if optlist:
		for opt, arg in optlist:
			if opt == '-l':
				prtrackinfo(player)
			elif opt == '-s':
				prstatus(player)
		return

	sys.stdout.write('waiting for socket... ')
	sys.stdout.flush()
	port = socket(AF_INET, SOCK_DGRAM)
	port.connect(HOST, PORT)
	print 'socket connected'

	parser = cd.createparser()
	parser.setcallback(0, audiocallback, port)
	parser.setcallback(1, pnumcallback, player)
	parser.setcallback(2, indexcallback, None)
	## 3 = ptime: too many calls
	## 4 = atime: too many calls
	parser.setcallback(5, catalogcallback, None)
	parser.setcallback(6, identcallback, None)
	parser.setcallback(7, controlcallback, None)

	if len(args) >= 2:
		if len(args) >= 3:
			[min, sec, frame] = args[:3]
		else:
			[min, sec] = args
			frame = 0
		min, sec, frame = eval(min), eval(sec), eval(frame)
		print 'Seek to', triple(min, sec, frame)
		dummy = player.seek(min, sec, frame)
	elif len(args) == 1:
		track = eval(args[0])
		print 'Seek to track', track
		dummy = player.seektrack(track)
	else:
		min, sec, frame = player.getstatus()[5:8]
		print 'Try to seek back to', triple(min, sec, frame)
		try:
			player.seek(min, sec, frame)
		except RuntimeError:
			print 'Seek failed'

	try:
		while 1:
			frames = player.readda(size)
			if frames == '':
				print 'END OF CD'
				break
			parser.parseframe(frames)
	except KeyboardInterrupt:
		print '[Interrupted]'
		pass

def prtrackinfo(player):
	info = []
	while 1:
		try:
			info.append(player.gettrackinfo(len(info) + 1))
		except RuntimeError:
			break
	for i in range(len(info)):
		start_min, start_sec, start_frame, \
			total_min, total_sec, total_frame = info[i]
		print 'Track', zfill(i+1), \
			triple(start_min, start_sec, start_frame), \
			triple(total_min, total_sec, total_frame)

def audiocallback(port, type, data):
##	sys.stdout.write('#')
##	sys.stdout.flush()
	port.send(data)

def pnumcallback(player, type, data):
	print 'pnum =', `data`
	prstatus(player)

def indexcallback(arg, type, data):
	print 'index =', `data`

def catalogcallback(arg, type, data):
	print 'catalog =', `data`

def identcallback(arg, type, data):
	print 'ident =', `data`

def controlcallback(arg, type, data):
	print 'control =', `data`

statedict = ['ERROR', 'NODISK', 'READY', 'PLAYING', 'PAUSED', 'STILL']

def prstatus(player):
	state, track, min, sec, frame, abs_min, abs_sec, abs_frame, \
		total_min, total_sec, total_frame, first, last, scsi_audio, \
		cur_block, dum1, dum2, dum3 = player.getstatus()
	print 'Status:',
	if 0 <= state < len(statedict):
		print statedict[state]
	else:
		print state
	print 'Track: ', track
	print 'Time:  ', triple(min, sec, frame)
	print 'Abs:   ', triple(abs_min, abs_sec, abs_frame)
	print 'Total: ', triple(total_min, total_sec, total_frame)
	print 'First: ', first
	print 'Last:  ', last
	print 'SCSI:  ', scsi_audio
	print 'Block: ', cur_block
	print 'Future:', (dum1, dum2, dum3)

def triple(a, b, c):
	return zfill(a) + ':' + zfill(b) + ':' + zfill(c)

def zfill(n):
	s = `n`
	return '0' * (2 - len(s)) + s

main()
