# Send/receive UDP multicast packets.
# Requires that your OS kernel supports IP multicast.
# This is built-in on SGI, still optional for most other vendors.
#
# Usage:
#   mcast -s (sender)
#   mcast -b (sender, using broadcast instead multicast)
#   mcast    (receivers)

MYPORT = 8123
MYGROUP = '225.0.0.250'

import sys
import time
import struct
import regsub
from socket import *


# Main program
def main():
	flags = sys.argv[1:]
	#
	if flags:
		sender(flags[0])
	else:
		receiver()


# Sender subroutine (only one per local area network)
def sender(flag):
	s = socket(AF_INET, SOCK_DGRAM)
	if flag == '-b':
		s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
		mygroup = '<broadcast>'
	else:
		mygroup = MYGROUP
		ttl = struct.pack('b', 1)		# Time-to-live
		s.setsockopt(IPPROTO_IP, IP_MULTICAST_TTL, ttl)
	while 1:
		data = `time.time()`
##		data = data + (1400 - len(data)) * '\0'
		s.sendto(data, (mygroup, MYPORT))
		time.sleep(1)


# Receiver subroutine (as many as you like)
def receiver():
	# Open and initialize the socket
	s = openmcastsock(MYGROUP, MYPORT)
	#
	# Loop, printing any data we receive
	while 1:
		data, sender = s.recvfrom(1500)
		while data[-1:] == '\0': data = data[:-1] # Strip trailing \0's
		print sender, ':', `data`


# Open a UDP socket, bind it to a port and select a multicast group
def openmcastsock(group, port):
	# Import modules used only here
	import string
	import struct
	#
	# Create a socket
	s = socket(AF_INET, SOCK_DGRAM)
	#
	# Allow multiple copies of this program on one machine
	# (not strictly needed)
	s.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
	#
	# Bind it to the port
	s.bind(('', port))
	#
	# Look up multicast group address in name server
	# (doesn't hurt if it is already in ddd.ddd.ddd.ddd format)
	group = gethostbyname(group)
	#
	# Construct binary group address
	bytes = map(int, string.split(group, "."))
	grpaddr = 0
	for byte in bytes: grpaddr = (grpaddr << 8) | byte
	#
	# Construct struct mreq from grpaddr and ifaddr
	ifaddr = INADDR_ANY
	mreq = struct.pack('ll', htonl(grpaddr), htonl(ifaddr))
	#
	# Add group membership
	s.setsockopt(IPPROTO_IP, IP_ADD_MEMBERSHIP, mreq)
	#
	return s


main()
