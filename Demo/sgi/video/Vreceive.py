#! /usr/bin/env python

# Receive live video UDP packets.
# Usage: Vreceive [port]

import sys
import struct
from socket import *			# syscalls and support functions
from SOCKET import *			# <sys/socket.h>
from IN import *			# <netinet/in.h>
import select
import struct
import gl, GL, DEVICE
sys.path.append('/ufs/guido/src/video')
import LiveVideoOut
import regsub
import getopt

from senddefs import *


# Print usage message and exit(2).

def usage(msg):
	print msg
	print 'usage: Vreceive [-m mcastgrp] [-p port] [-c type]'
	print '-m mcastgrp: multicast group (default ' + `DEFMCAST` + ')'
	print '-p port    : port (default ' + `DEFPORT` + ')'
	print '-c type    : signal type: rgb8, grey or mono (default rgb8)'
	sys.exit(2)


# Main program: parse options and main loop.

def main():

	sys.stdout = sys.stderr

	group = DEFMCAST
	port = DEFPORT
	width = DEFWIDTH
	height = DEFHEIGHT
	vtype = 'rgb8'

	try:
		opts, args = getopt.getopt(sys.argv[1:], 'm:p:c:')
	except getopt.error, msg:
		usage(msg)

	try:
		for opt, optarg in opts:
			if opt == '-p':
				port = string.atoi(optarg)
			if opt == '-m':
				group = gethostbyname(optarg)
			if opt == '-c':
				vtype = optarg
	except string.atoi_error, msg:
		usage('bad integer: ' + msg)

	s = opensocket(group, port)

	gl.foreground()
	gl.prefsize(width, height)
	wid = gl.winopen('Vreceive')
	gl.winconstraints()
	gl.qdevice(DEVICE.ESCKEY)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.WINQUIT)

	lvo = LiveVideoOut.LiveVideoOut(wid, width, height, vtype)

	ifdlist = [gl.qgetfd(), s.fileno()]
	ofdlist = []
	xfdlist = []
	timeout = 1.0
	selectargs = (ifdlist, ofdlist, xfdlist, timeout)

	while 1:

		if gl.qtest():
			dev, val = gl.qread()
			if dev in (DEVICE.ESCKEY, \
				DEVICE.WINSHUT, DEVICE.WINQUIT):
				break
			if dev == DEVICE.REDRAW:
				lvo.reshapewindow()
		elif s.avail():
			data = s.recv(16*1024)
			pos, w, h = struct.unpack('hhh', data[:6])
			if (w, h) <> (width, height):
				x, y =  gl.getorigin()
				y = y + height - h
				gl.winposition(x, x+w-1, y, y+h-1)
				width, height = w, h
				lvo.resizevideo(width, height)
			lvo.putnextpacket(pos, data[6:])
		else:
			x = select.select(selectargs)

	lvo.close()


# Subroutine to create and properly initialize the receiving socket

def opensocket(group, port):

	# Create the socket
	s = socket(AF_INET, SOCK_DGRAM)

	# Allow multiple copies of this program on one machine
	s.setsockopt(SOL_SOCKET, SO_REUSEPORT, 1) # (Not strictly needed)

	# Bind the port to it
	s.bind('', port)

	# Look up the group once
	group = gethostbyname(group)

	# Construct binary group address
	group_bytes = eval(regsub.gsub('\.', ',', group))
	grpaddr = 0
	for byte in group_bytes: grpaddr = (grpaddr << 8) | byte

	# Construct struct mreq from grpaddr and ifaddr
	ifaddr = INADDR_ANY
	mreq = struct.pack('ll', grpaddr, ifaddr)

	# Add group membership
	s.setsockopt(IPPROTO_IP, IP_ADD_MEMBERSHIP, mreq)

	return s


main()
