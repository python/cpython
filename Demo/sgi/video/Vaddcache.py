#! /usr/bin/env python

# Add a cache to each of the files given as command line arguments


# Usage:
#
# Vaddcache [file] ...


# Options:
#
# file ... : file(s) to modify; default film.video


import sys
sys.path.append('/ufs/guido/src/video')
import VFile
import getopt


# Global options

# None


# Main program -- mostly command line parsing

def main():
	opts, args = getopt.getopt(sys.argv[1:], '')
	if not args:
		args = ['film.video']
	sts = 0
	for filename in args:
		if process(filename):
			sts = 1
	sys.exit(sts)


# Process one file

def process(filename):
	try:
		fp = open(filename, 'r+')
		vin = VFile.RandomVinFile(fp)
		vin.filename = filename
	except IOError, msg:
		sys.stderr.write(filename + ': I/O error: ' + `msg` + '\n')
		return 1
	except VFile.Error, msg:
		sys.stderr.write(msg + '\n')
		return 1
	except EOFError:
		sys.stderr.write(filename + ': EOF in video file\n')
		return 1

	try:
		vin.readcache()
		hascache = 1
	except VFile.Error:
		hascache = 0

	if hascache:
		sys.stderr.write(filename + ': already has a cache\n')
		vin.close()
		return 1

	vin.printinfo()
	vin.warmcache()
	vin.writecache()
	vin.close()
	return 0


# Don't forget to call the main program

try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
