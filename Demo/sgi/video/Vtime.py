#! /usr/bin/env python

# Manipulate the time base of CMIF movies


# Possibilities:
#
# - resample at a fixed rate
# - divide the time codes by a speed factor (to make it go faster/slower)
# - drop frames that are less than n msec apart (to accommodate slow players)


# Usage:
#
# Vtime [-m msec] [-r msec] [-s speed] [infile [outfile]]


# Options:
#
# -m n       : drop frames closer than n msec (default 0)
# -r n       : regenerate input time base n msec apart
# -s speed   : speed change factor after other processing (default 1.0)
# infile     : input file (default film.video)
# outfile    : output file (default out.video)


import sys
sys.path.append('/ufs/guido/src/video')
import VFile
import getopt
import string


# Global options

speed = 1.0
mindelta = 0
regen = None


# Main program -- mostly command line parsing

def main():
	global speed, mindelta
	opts, args = getopt.getopt(sys.argv[1:], 'm:r:s:')
	for opt, arg in opts:
		if opt == '-m':
			mindelta = string.atoi(arg)
		elif opt == '-r':
			regen = string.atoi(arg)
		elif opt == '-s':
			speed = float(eval(arg))
	if len(args) < 1:
		args.append('film.video')
	if len(args) < 2:
		args.append('out.video')
	if len(args) > 2:
		sys.stderr.write('usage: Vtime [options] [infile [outfile]]\n')
		sys.exit(2)
	sts = process(args[0], args[1])
	sys.exit(sts)


# Copy one file to another

def process(infilename, outfilename):
	try:
		vin = VFile.BasicVinFile(infilename)
	except IOError, msg:
		sys.stderr.write(infilename + ': I/O error: ' + `msg` + '\n')
		return 1
	except VFile.Error, msg:
		sys.stderr.write(msg + '\n')
		return 1
	except EOFError:
		sys.stderr.write(infilename + ': EOF in video file\n')
		return 1

	try:
		vout = VFile.BasicVoutFile(outfilename)
	except IOError, msg:
		sys.stderr.write(outfilename + ': I/O error: ' + `msg` + '\n')
		return 1

	vout.setinfo(vin.getinfo())
	vout.writeheader()
	
	told = 0
	nin = 0
	nout = 0
	tin = 0
	tout = 0

	while 1:
		try:
			tin, data, cdata = vin.getnextframe()
		except EOFError:
			break
		nin = nin + 1
		if regen:
			tout = nin * regen
		else:
			tout = tin
		tout = int(tout / speed)
		if tout - told < mindelta:
			continue
		told = tout
		vout.writeframe(tout, data, cdata)
		nout = nout + 1

	vout.close()
	vin.close()


# Don't forget to call the main program

main()
