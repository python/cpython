#! /usr/bin/env python

# Decompress a jpeg or jpeggrey video file to rgb format


# Usage:
#
# Vunjpeg [infile [outfile]]


# Options:
#
# infile     : input file (default film.video)
# outfile    : output file (default out.video)


import sys
import jpeg
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
		sys.stderr.write('usage: Vunjpeg [infile [outfile]]\n')
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
	if info[0] == 'jpeg':
		format = 'rgb'
		width, height = vin.getsize()
		bytes = 4
	elif info[0] == 'jpeggrey':
		format = 'grey'
		width, height = vin.getsize()
		pf = vin.packfactor
		width, height = width/pf, height/pf
		bytes = 1
	else:
		sys.stderr.write('Vunjpeg: input not in jpeg[grey] format\n')
		return 1
	info = (format,) + info[1:]
	vout.setinfo(info)
	vout.writeheader()
	sts = 0
	n = 0
	try:
		while 1:
			t, data, cdata = vin.getnextframe()
			n = n + 1
			sys.stderr.write('Frame ' + `n` + '...')
			data, w, h, b = jpeg.decompress(data)
			if (w, h, b) <> (width, height, bytes):
				sys.stderr.write('jpeg data has wrong size\n')
				sts = 1
			else:
				vout.writeframe(t, data, None)
				sys.stderr.write('\n')
	except EOFError:
		pass
	return sts


# Don't forget to call the main program

main()
