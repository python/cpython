#! /usr/bin/env python

# Send live video UDP packets.
# Usage: Vsend [-b] [-h height] [-p port] [-s size] [-t ttl] [-w width]
#              [host] ..

import sys
import time
import struct
import string
import math
from socket import *
from SOCKET import *
import gl, GL, DEVICE
sys.path.append('/ufs/guido/src/video')
import DisplayVideoIn
import LiveVideoOut
import SV
import getopt
from IN import *

from senddefs import *

def usage(msg):
	print msg
	print 'usage: Vsend [-b] [-h height] [-p port] [-s size] [-t ttl] [-c type] [-m]',
	print '[-w width] [host] ...'
	print '-b        : broadcast on local net'
	print '-h height : window height (default ' + `DEFHEIGHT` + ')'
	print '-p port   : port to use (default ' + `DEFPORT` + ')'
	print '-t ttl    : time-to-live (multicast only; default 1)'
	print '-s size   : max packet size (default ' + `DEFPKTMAX` + ')'
	print '-S size   : use this packet size/window size'
	print '-w width  : window width (default ' + `DEFWIDTH` + ')'
	print '-v        : print packet rate'
	print '-x xpos   : set x position'
	print '-y ypos   : set y position'
	print '[host] ...: host(s) to send to (default multicast to ' + \
		DEFMCAST + ')'
	sys.exit(2)


def main():
	sys.stdout = sys.stderr

	hosts = []
	port = DEFPORT
	ttl = -1
	pktmax = DEFPKTMAX
	width = DEFWIDTH
	height = DEFHEIGHT
	vtype = 'rgb'
	verbose = 0
	xpos = ypos = 0

	try:
		opts, args = getopt.getopt(sys.argv[1:], 'bh:p:s:S:t:w:vx:y:')
	except getopt.error, msg:
		usage(msg)

	try:
		for opt, optarg in opts:
			if opt == '-p':
				port = string.atoi(optarg)
			if opt == '-b':
				host = '<broadcast>'
			if opt == '-t':
				ttl = string.atoi(optarg)
			if opt == '-S':
				pktmax = string.atoi(optarg)
				vidmax = SV.PAL_XMAX*SV.PAL_YMAX
				if vidmax <= pktmax:
					width = SV.PAL_XMAX
					height = SV.PAL_YMAX
					pktmax = vidmax
				else:
					factor = float(vidmax)/float(pktmax)
					factor = math.sqrt(factor)
					width = int(SV.PAL_XMAX/factor)-7
					height = int(SV.PAL_YMAX/factor)-5
					print 'video:',width,'x',height,
					print 'pktsize',width*height,'..',
					print pktmax
			if opt == '-s':
				pktmax = string.atoi(optarg)
			if opt == '-w':
				width = string.atoi(optarg)
			if opt == '-h':
				height = string.atoi(optarg)
			if opt == '-c':
				vtype = optarg
			if opt == '-v':
				verbose = 1
			if opt == '-x':
				xpos = string.atoi(optarg)
			if opt == '-y':
				ypos = string.atoi(optarg)
	except string.atoi_error, msg:
		usage('bad integer: ' + msg)

	for host in args:
		hosts.append(gethostbyname(host))

	if not hosts:
		hosts.append(gethostbyname(DEFMCAST))

	gl.foreground()
	gl.prefsize(width, height)
	gl.stepunit(8, 6)
	wid = gl.winopen('Vsend')
	gl.keepaspect(width, height)
	gl.stepunit(8, 6)
	gl.maxsize(SV.PAL_XMAX, SV.PAL_YMAX)
	gl.winconstraints()
	gl.qdevice(DEVICE.ESCKEY)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINFREEZE)
	gl.qdevice(DEVICE.WINTHAW)
	width, height = gl.getsize()

	lvo = LiveVideoOut.LiveVideoOut(wid, width, height, vtype)

	lvi = DisplayVideoIn.DisplayVideoIn(pktmax, width, height, vtype)

	if xpos or ypos:
		lvi.positionvideo(xpos, ypos)

	s = socket(AF_INET, SOCK_DGRAM)
	s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
	if ttl >= 0:
		s.setsockopt(IPPROTO_IP, IP_MULTICAST_TTL, chr(ttl))

	frozen = 0

	lasttime = int(time.time())
	nframe = 0
	while 1:

		if gl.qtest():
			dev, val = gl.qread()
			if dev in (DEVICE.ESCKEY, \
				DEVICE.WINSHUT, DEVICE.WINQUIT):
				break
			if dev == DEVICE.WINFREEZE:
				frozen = 1
			if dev == DEVICE.WINTHAW:
				frozen = 0
			if dev == DEVICE.REDRAW:
				w, h = gl.getsize()
				x, y = gl.getorigin()
				if (w, h) <> (width, height):
					width, height = w, h
					lvi.resizevideo(width, height)
					lvo.resizevideo(width, height)

		rv = lvi.getnextpacket()
		if not rv:
			time.sleep(0.010)
			continue

		pos, data = rv
		print pos, len(data) # DBG

		if not frozen:
			lvo.putnextpacket(pos, data)

		hdr = struct.pack('hhh', pos, width, height)
		for host in hosts:
			try:
				# print len(hdr+data) # DBG
				s.sendto(hdr + data, (host, port))
			except error, msg: # really socket.error
				if msg[0] <> 121: # no buffer space available
					raise error, msg # re-raise it
				print 'Warning:', msg[1]
		if pos == 0 and verbose:
			nframe = nframe+1
			if int(time.time()) <> lasttime:
				print nframe / (time.time()-lasttime), 'fps'
				nframe = 0
				lasttime = int(time.time())

	lvi.close()
	lvo.close()


main()
