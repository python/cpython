#! /usr/bin/env python

# radio [port]
#
# Receive audio packets broadcast by "broadcast.py" on another SGI machine.
# Use apanel to set the output sampling rate to match that of the broadcast.

import sys, al
from socket import *

port = 5555
if sys.argv[1:]: port = eval(sys.argv[1])

s = socket(AF_INET, SOCK_DGRAM)
s.bind('', port)

p = al.openport('radio', 'w')

while 1:
	data = s.recv(1400)
	p.writesamps(data)
