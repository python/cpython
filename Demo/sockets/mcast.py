# Send/receive UDP multicast packets (SGI)
# After /usr/people/4Dgifts/examples/network/mcast.c
# Usage:
#   mcast -s (sender)
#   mcast -b (sender, using broadcast instead multicast)
#   mcast    (receivers)

MYPORT = 8123
MYGROUP_BYTES = 225, 0, 0, 250

import sys
import time
import struct
from socket import *
from SOCKET import *
from IN import *

sender = sys.argv[1:]

s = socket(AF_INET, SOCK_DGRAM)

if sender:
	if sys.argv[1] == '-b':
		s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
		mygroup = '<broadcast>'
	else:
		# Ugly: construct decimal IP address string from MYGROUP_BYTES
		mygroup = ''
		for byte in MYGROUP_BYTES: mygroup = mygroup + '.' + `byte`
		mygroup = mygroup[1:]
		ttl = struct.pack('b', 1)		# Time-to-live
		s.setsockopt(IPPROTO_IP, IP_MULTICAST_TTL, ttl)
	while 1:
		data = `time.time()`
##		data = data + (1400 - len(data)) * '\0'
		s.sendto(data, (mygroup, MYPORT))
		time.sleep(1)
else:
	# Bind the socket to my port
	s.bind('', MYPORT)

	# Allow multiple copies of this program on one machine
	s.setsockopt(SOL_SOCKET, SO_REUSEPORT, 1) # (Not strictly needed)

	# Ugly: construct binary group address from MYGROUP_BYTES
	grpaddr = 0
	for byte in MYGROUP_BYTES: grpaddr = (grpaddr << 8) | byte

	# Construct struct mreq from grpaddr and ifaddr
	ifaddr = INADDR_ANY
	mreq = struct.pack('ll', grpaddr, ifaddr)

	# Add group membership
	s.setsockopt(IPPROTO_IP, IP_ADD_MEMBERSHIP, mreq)

	# Loop, printing any data we receive
	while 1:
		data, sender = s.recvfrom(1500)
		while data[-1:] == '\0': data = data[:-1] # Strip trailing \0's
		print sender, ':', `data`
