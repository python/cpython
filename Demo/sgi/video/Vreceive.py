#!/ufs/guido/bin/sgi/python-405

# Receive live video UDP packets.
# Usage: Vreceive [port]

import sys
import struct
from socket import *
import select
import gl, GL, DEVICE
sys.path.append('/ufs/guido/src/video')
import LiveVideoOut

PKTMAX = 16*1024
WIDTH = 400
HEIGHT = 300
HOST = ''
PORT = 5555

def main():

	port = PORT
	if sys.argv[1:]:
		port = eval(sys.argv[1])

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

	s = socket(AF_INET, SOCK_DGRAM)
	s.bind(HOST, port)

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

main()
