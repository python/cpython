# Read CD audio data from the SCSI CD player and send it as UDP
# packets to "recvcd.py" on another host.
#
# Usage: python sendcd.py [options] host [track | minutes seconds [frames]]
#
# Options:
# "-l"		list track info and quit.
# "-s"		display status and quit.
#
# Arguments:
# host		host to send the audio data to (required unless -l or -s).
# track		track number where to start; alternatively,
# min sec [frames]	absolute address where to start;
#		default is continue at current point according to status.

import cd
import sys
from socket import *
import getopt

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

	if not args:
		sys.stderr.write('usage: ' + sys.argv[0] + ' host [track]\n')
		sys.exit(2)
	host, args = args[0], args[1:]

	sys.stdout.write('waiting for socket... ')
	sys.stdout.flush()
	port = socket(AF_INET, SOCK_DGRAM)
	port.connect(host, PORT)
	print 'socket connected'

	parser = cd.createparser()
	parser.setcallback(cd.audio, audiocallback, port)
	parser.setcallback(cd.pnum, pnumcallback, player)
	parser.setcallback(cd.index, indexcallback, None)
	## cd.ptime: too many calls
	## cd.atime: too many calls
	parser.setcallback(cd.catalog, catalogcallback, None)
	parser.setcallback(cd.ident, identcallback, None)
	parser.setcallback(cd.control, controlcallback, None)

	if len(args) >= 2:
		if len(args) >= 3:
			[min, sec, frame] = args[:3]
		else:
			[min, sec] = args
			frame = '0'
		min, sec, frame = eval(min), eval(sec), eval(frame)
		print 'Seek to', triple(min, sec, frame)
		dummy = player.seek(min, sec, frame)
	elif len(args) == 1:
		track = eval(args[0])
		print 'Seek to track', track
		dummy = player.seektrack(track)
	else:
		min, sec, frame = player.getstatus()[3]
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
		start, total = info[i]
		print 'Track', zfill(i+1), triple(start), triple(total)

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
	state, track, curtime, abstime, totaltime, first, last, \
		scsi_audio, cur_block, dummy = player.getstatus()
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

def triple((a, b, c)):
	return zfill(a) + ':' + zfill(b) + ':' + zfill(c)

def zfill(n):
	s = `n`
	return '0' * (2 - len(s)) + s

main()
