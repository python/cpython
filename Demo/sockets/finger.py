#! /usr/bin/env python

# Python interface to the Internet finger daemon.
#
# Usage: finger [options] [user][@host] ...
#
# If no host is given, the finger daemon on the local host is contacted.
# Options are passed uninterpreted to the finger daemon!


import sys, string
from socket import *


# Hardcode the number of the finger port here.
# It's not likely to change soon...
#
FINGER_PORT = 79


# Function to do one remote finger invocation.
# Output goes directly to stdout (although this can be changed).
#
def finger(host, args):
	s = socket(AF_INET, SOCK_STREAM)
	s.connect((host, FINGER_PORT))
	s.send(args + '\n')
	while 1:
		buf = s.recv(1024)
		if not buf: break
		sys.stdout.write(buf)
	sys.stdout.flush()


# Main function: argument parsing.
#
def main():
	options = ''
	i = 1
	while i < len(sys.argv) and sys.argv[i][:1] == '-':
		options = options + sys.argv[i] + ' '
		i = i+1
	args = sys.argv[i:]
	if not args:
		args = ['']
	for arg in args:
		if '@' in arg:
			at = string.index(arg, '@')
			host = arg[at+1:]
			arg = arg[:at]
		else:
			host = ''
		finger(host, options + arg)


# Call the main function.
#
main()
