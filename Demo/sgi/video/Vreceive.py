#!/ufs/guido/bin/sgi/python-405

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

MYGROUP = '225.0.0.250'
PORT = 5555

PKTMAX = 16*1024
WIDTH = 400
HEIGHT = 300

def main():

	port = PORT
	if sys.argv[1:]:
		port = eval(sys.argv[1])

	s = opensocket(MYGROUP, port)

	width, height = WIDTH, HEIGHT

	gl.foreground()
	gl.prefsize(width, height)
	wid = gl.winopen('Vreceive')
	gl.qdevice(DEVICE.ESCKEY)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.WINQUIT)

	x, y = gl.getorigin()
	lvo = LiveVideoOut.LiveVideoOut().init(wid, (x, y, width, height), \
		width, height)

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
				gl.clear()
		elif s.avail():
			data = s.recv(16*1024)
			pos, w, h = struct.unpack('hhh', data[:6])
			if (w, h) <> (width, height):
				x, y = gl.getorigin()
				y = y + height - h
				width, height = w, h
				lvo.close()
				lvo = LiveVideoOut.LiveVideoOut() \
				      .init(wid, (x, y, width, height), \
				            width, height)
			lvo.putnextpacket(pos, data[6:])
		else:
			x = select.select(selectargs)

	lvo.close()


# Subroutine to create and properly initialize the receiving socket

def opensocket(group, port):

	# Create the socket
	s = socket(AF_INET, SOCK_DGRAM)

	# Bind the port to it
	s.bind('', port)

	# Allow multiple copies of this program on one machine
	s.setsockopt(SOL_SOCKET, SO_REUSEPORT, 1) # (Not strictly needed)

	# Look up the group once
	group = gethostbyname(group)

	# Ugly: construct binary group address
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
