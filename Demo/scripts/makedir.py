#! /usr/local/bin/python

# Like mkdir, but also make intermediate directories if necessary.
# It is not an error if the given directory already exists (as long
# as it is a directory).
# Errors are not treated specially -- you just get a Python exception.

import sys, os

def main():
	for p in sys.argv[1:]:
		makedirs(p)

def makedirs(p):
	if not os.path.isdir(p):
		head, tail = os.path.split(p)
		makedirs(head)
		os.mkdir(p, 0777)

main()
