#! /usr/bin/env python

# Copy a video file, fixing the line width to be a multiple of 4


# Usage:
#
# Vfix [infile [outfile]]


# Options:
#
# infile     : input file (default film.video)
# outfile    : output file (default out.video)


import sys
import imageop
sys.path.append('/ufs/guido/src/video')
import VFile


# Main program -- mostly command line parsing

def main():
	args = sys.argv[1:]
	if len(args) < 1:
		args.append('film.video')
	if len(args) < 2:
		args.append('out.video')
	if len(args) > 2:
		sys.stderr.write('usage: Vfix [infile [outfile]]\n')
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

	info = vin.getinfo()
	if info[0] <> 'grey':
		sys.stderr.write('Vfix: input not in grey format\n')
		return 1
	vout.setinfo(info)
	inwidth, height = vin.getsize()
	pf = vin.packfactor
	if (inwidth/pf)%4 == 0:
		sys.stderr.write('Vfix: fix not necessary\n')
		return 1
	outwidth = (inwidth/pf/4)*4*pf
	print 'inwidth =', inwidth, 'outwidth =', outwidth
	vout.setsize(outwidth, height)
	vout.writeheader()
	n = 0
	try:
		while 1:
			t, data, cdata = vin.getnextframe()
			n = n + 1
			sys.stderr.write('Frame ' + `n` + '...')
			data = imageop.crop(data, 1, inwidth/pf, height/pf, \
				0, 0, outwidth/pf-1, height/pf-1)
			vout.writeframe(t, data, None)
			sys.stderr.write('\n')
	except EOFError:
		pass
	return 0


# Don't forget to call the main program

main()
