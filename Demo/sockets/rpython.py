#! /usr/bin/env python

# Remote python client.
# Execute Python commands remotely and send output back.

import sys
import string
from socket import *

PORT = 4127
BUFSIZE = 1024

def main():
	if len(sys.argv) < 3:
		print "usage: rpython host command"
		sys.exit(2)
	host = sys.argv[1]
	port = PORT
	i = string.find(host, ':')
	if i >= 0:
		port = string.atoi(port[i+1:])
		host = host[:i]
	command = string.join(sys.argv[2:])
	s = socket(AF_INET, SOCK_STREAM)
	s.connect((host, port))
	s.send(command)
	s.shutdown(1)
	reply = ''
	while 1:
		data = s.recv(BUFSIZE)
		if not data: break
		reply = reply + data
	print reply,

main()
