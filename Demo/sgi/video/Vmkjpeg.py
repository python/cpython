#! /usr/bin/env python

# Compress an rgb or grey video file to jpeg format


# Usage:
#
# Vmkjpeg [infile [outfile]]


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
		sys.stderr.write('usage: Vmkjpeg [infile [outfile]]\n')
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
	if info[0] == 'rgb':
		width, height = vin.getsize()
		bytes = 4
		format = 'jpeg'
	elif info[0] == 'grey':
		width, height = vin.getsize()
		pf = vin.packfactor
		width, height = width / pf, height / pf
		bytes = 1
		format = 'jpeggrey'
	else:
		sys.stderr.write('Vmkjpeg: input not in rgb or grey format\n')
		return 1
	info = (format,) + info[1:]
	vout.setinfo(info)
	vout.writeheader()
	n = 0
	try:
		while 1:
			t, data, cdata = vin.getnextframe()
			n = n + 1
			sys.stderr.write('Frame ' + `n` + '...')
			data = jpeg.compress(data, width, height, bytes)
			vout.writeframe(t, data, None)
			sys.stderr.write('\n')
	except EOFError:
		pass
	return 0


# Don't forget to call the main program

main()
