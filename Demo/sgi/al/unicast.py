#! /usr/bin/env python

# unicast host [port]
#
# Similar to "broadcast.py" but sends to a specific host only;
# use "radio.py" on the designated host to receive.
# This is less stressful on other hosts on the same ethernet segment
# if you need to send to one host only.

import sys, al
from socket import *

host = sys.argv[1]

port = 5555
if sys.argv[2:]: port = eval(sys.argv[1])

s = socket(AF_INET, SOCK_DGRAM)

p = al.openport('unicast', 'r')

address = host, port
while 1:
	# 700 samples equals 1400 bytes, or about the max packet size!
	data = p.readsamps(700)
	s.sendto(data, address)
