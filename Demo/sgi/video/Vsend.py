#!/ufs/guido/bin/sgi/python-405

# Send live video UDP packets.
# Usage: Vsend [host [port]]

import sys
import time
import struct
from socket import *
from SOCKET import *
import gl, GL, DEVICE
sys.path.append('/ufs/guido/src/video')
import LiveVideoIn
import LiveVideoOut
import SV

PKTMAX_UCAST = 16*1024 - 6
PKTMAX_BCAST = 1450
WIDTH = 400
HEIGHT = 300
HOST = '225.0.0.250' # Multicast address!
PORT = 5555

def main():
	if not LiveVideoIn.have_video:
		print 'Sorry, no video (use python-405 on roos)'
		sys.exit(1)

	host = HOST
	port = PORT
	if sys.argv[1:]:
		host = sys.argv[1]
		if host == '-b':
			host = '<broadcast>'
		if sys.argv[2:]:
			port = eval(sys.argv[2])

	if host == '<broadcast>':
		pktmax = PKTMAX_BCAST
	else:
		pktmax = PKTMAX_UCAST

	gl.foreground()
	gl.prefsize(WIDTH, HEIGHT)
	wid = gl.winopen('Vsend')
	gl.keepaspect(WIDTH, HEIGHT)
	gl.stepunit(8, 6)
	gl.maxsize(SV.PAL_XMAX, SV.PAL_YMAX)
	gl.winconstraints()
	gl.qdevice(DEVICE.ESCKEY)
	gl.qdevice(DEVICE.WINSHUT)
	gl.qdevice(DEVICE.WINQUIT)
	gl.qdevice(DEVICE.WINFREEZE)
	gl.qdevice(DEVICE.WINTHAW)
	width, height = gl.getsize()

	x, y = gl.getorigin()
	lvo = LiveVideoOut.LiveVideoOut().init(wid, (x, y, width, height), \
		width, height)

	lvi = LiveVideoIn.LiveVideoIn().init(pktmax, width, height)

	s = socket(AF_INET, SOCK_DGRAM)
	s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)

	frozen = 0

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
					lvi.close()
					width, height = w, h
					lvi = LiveVideoIn.LiveVideoIn() \
						.init(pktmax, width, height)
					lvo.close()
					lvo = LiveVideoOut.LiveVideoOut() \
						.init(wid, \
						      (x, y, width, height), \
						      width, height)

		rv = lvi.getnextpacket()
		if not rv:
			time.millisleep(10)
			continue

		pos, data = rv

		if not frozen:
			lvo.putnextpacket(pos, data)

		hdr = struct.pack('hhh', pos, width, height)
		s.sendto(hdr + data, (host, port))

	lvi.close()
	lvo.close()

main()
