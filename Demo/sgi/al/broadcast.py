#! /usr/bin/env python

# broadcast [port]
#
# Broadcast audio input on the network as UDP packets;
# they can be received on any SGI machine with "radio.py".
# This uses the input sampling rate, input source etc. set by apanel.
# It uses the default sample width and #channels (16 bit/sample stereo).
# (This is 192,000 Bytes at a sampling speed of 48 kHz, or ~137
# packets/second -- use with caution!!!)

import sys, al
from socket import *

port = 5555
if sys.argv[1:]: port = eval(sys.argv[1])

s = socket(AF_INET, SOCK_DGRAM)
s.allowbroadcast(1)

p = al.openport('broadcast', 'r')

address = '<broadcast>', port
while 1:
	# 700 samples equals 1400 bytes, or about the max packet size!
	data = p.readsamps(700)
	s.sendto(data, address)
