#! /usr/bin/env python

# Print some info about a CMIF movie file


# Usage:
#
# Vinfo [-d] [-q] [-s] [-t] [file] ...


# Options:
#
# -d       : print deltas between frames instead of frame times
# -q       : quick: don't read the frames
# -s       : don't print times (but do count frames and print the total)
# -t       : terse (one line/file, implies -s)
# file ... : file(s) to inspect; default film.video


import sys
sys.path.append('/ufs/guido/src/video')
import VFile
import getopt
import string


# Global options

short = 0
quick = 0
delta = 0
terse = 0
maxwidth = 10


# Main program -- mostly command line parsing

def main():
	global short, quick, delta, terse, maxwidth
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'dqst')
	except getopt.error, msg:
		sys.stdout = sys.stderr
		print msg
		print 'usage: Vinfo [-d] [-q] [-s] [-t] [file] ...'
		sys.exit(2)
	for opt, arg in opts:
		if opt == '-q':
			quick = 1
		if opt == '-d':
			delta = 1
		if opt == '-s':
			short = 1
		if opt == '-t':
			terse = short = 1
	if not args:
		args = ['film.video']
	for filename in args:
		maxwidth = max(maxwidth, len(filename))
	sts = 0
	for filename in args:
		if process(filename):
			sts = 1
	sys.exit(sts)


# Process one file

def process(filename):
	try:
		vin = VFile.RandomVinFile(filename)
	except IOError, msg:
		sys.stderr.write(filename + ': I/O error: ' + `msg` + '\n')
		return 1
	except VFile.Error, msg:
		sys.stderr.write(msg + '\n')
		return 1
	except EOFError:
		sys.stderr.write(filename + ': EOF in video file\n')
		return 1

	if terse:
		print string.ljust(filename, maxwidth),
		kbytes = (VFile.getfilesize(filename) + 1023) / 1024
		print string.rjust(`kbytes`, 5) + 'K',
		print ' ', string.ljust(`vin.version`, 5),
		print string.ljust(vin.format, 8),
		print string.rjust(`vin.width`, 4),
		print string.rjust(`vin.height`, 4),
		if type(vin.packfactor) == type(()):
			xpf, ypf = vin.packfactor
			s = string.rjust(`xpf`, 2) + ',' + \
				  string.rjust(`ypf`, 2)
		else:
			s = string.rjust(`vin.packfactor`, 2)
			if type(vin.packfactor) == type(0) and \
				  vin.format not in ('rgb', 'jpeg') and \
				  (vin.width/vin.packfactor) % 4 <> 0:
				s = s + '!  '
			else:
				s = s + '   '
		print s,
		sys.stdout.flush()
	else:
		vin.printinfo()

	if quick:
		if terse:
			print
		vin.close()
		return 0

	try:
		vin.readcache()
		if not terse:
			print '[Using cached index]'
	except VFile.Error:
		if not terse:
			print '[Constructing index on the fly]'

	if not short:
		if delta:
			print 'Frame time deltas:',
		else:
			print 'Frame times:',

	n = 0
	t = 0
	told = 0
	datasize = 0
	while 1:
		try:
			t, ds, cs = vin.getnextframeheader()
			vin.skipnextframedata(ds, cs)
		except EOFError:
			break
		datasize = datasize + ds
		if cs: datasize = datasize + cs
		if not short:
			if n%8 == 0:
				sys.stdout.write('\n')
			if delta:
				sys.stdout.write('\t' + `t - told`)
				told = t
			else:
				sys.stdout.write('\t' + `t`)
		n = n+1

	if not short: print

	if terse:
		print string.rjust(`n`, 6),
		if t: print string.rjust(`int(n*10000.0/t)*0.1`, 5),
		print
	else:
		print 'Total', n, 'frames in', t*0.001, 'sec.',
		if t: print '-- average', int(n*10000.0/t)*0.1, 'frames/sec',
		print
		print 'Total data', 0.1 * int(datasize / 102.4), 'Kbytes',
		if t:
			print '-- average',
			print 0.1 * int(datasize / 0.1024 / t), 'Kbytes/sec',
		print

	vin.close()
	return 0


# Don't forget to call the main program

try:
	main()
except KeyboardInterrupt:
	print '[Interrupt]'
